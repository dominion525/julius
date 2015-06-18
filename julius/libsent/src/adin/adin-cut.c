/**
 * @file   adin-cut.c
 * @author Akinobu LEE
 * @date   Sat Feb 12 13:20:53 2005
 *
 * <JA>
 * @brief  音声取り込みおよび音声区間検出
 *
 * 音声入力デバイスからの音声データの取り込み，および音声区間の検出を
 * 行ないます．
 *
 * 音声区間の検出は，振幅レベルと零交差数を用いて行なっています．
 * 入力断片ごとに，レベルしきい値を越える振幅について零交差数をカウントし，
 * それが指定した数以上になれば，音声区間開始検出として
 * 取り込みを開始します．取り込み中に零交差数が指定数以下になれば，
 * 取り込みを停止します．実際には頑健に切り出しを行なうため，開始部と
 * 停止部の前後にマージンを持たせて切り出します．
 * また必要であれば DC offset の調整を行ないます．
 *
 * 音声データの取り込みと並行して入力音声の処理を行ないます．このため，
 * 取り込んだ音声データはその取り込み単位（live入力では一定時間，音声ファイル
 * ではバッファサイズ）ごとに，それらを引数としてコールバック関数が呼ばれます．
 * このコールバック関数としてデータの保存や特徴量抽出，
 * （フレーム同期の）認識処理を進める関数を指定しておきます．
 *
 * マイク入力や NetAudio 入力などの Live 入力を直接読み込む場合，
 * コールバック内の処理が重く処理が入力の速度に追い付かないと，
 * デバイスのバッファが溢れ，入力断片がロストする場合があります．
 * このエラーを防ぐために，もし実行環境で pthread が使用可能であれば，
 * 音声取り込み・音声区間検出部は本体と独立したスレッドとして動作します．
 * この場合，このスレッドは本スレッドとバッファ @a speech を介して以下のように
 * 協調動作します．
 * 
 *    - Thread 1: 音声取り込み・音声区間検出スレッド
 *        - デバイスから音声データを読み込みながら音声区間検出を行なう．
 *          検出した音声区間のサンプルはバッファ @a speech の末尾に逐次
 *          追加される．
 *        - このスレッドは起動時から本スレッドから独立して動作し，
 *          上記の動作を行ない続ける．
 *    - Thread 2: 音声処理・認識処理を行なう本スレッド
 *        - バッファ @a speech を一定時間ごとに監視し，新たなサンプルが
 *          Thread 1 によって追加されたらそれらを処理し，処理が終了した
 *          分バッファを詰める．
 *
 * 定義される関数の概要は以下のとおりです．
 * Juliusのメイン部から呼び出される関数は adin_go() です．
 * 音声取り込みと区間検出処理の本体は adin_cut() です．
 * 音声入力ソースの切替えは， adin_setup_func() を対象となる入力ストリームの
 * 開始・読み込み・停止の関数を引数として呼び出すことで行なわれます．
 * また切り出し処理のための各種パラメータは adin_setup_param() でセットします．
 * </JA>
 * <EN>
 * @brief  Read in speech waveform and detect speech segment
 *
 * This file contains functions to get speech waveform from an audio device
 * and detect speech segment.
 *
 * Speech detection is based on level threshold and zero cross count.
 * The number of zero cross are counted for each incoming speech fragment.
 * If the number becomes larger than specified threshold, the fragment
 * is treated as a beginning of speech input (trigger on).  If the number goes
 * below the threshold, the fragment will be treated as an
 * end of speech input (trigger off).  In actual
 * detection, margins are considered on the beginning and ending point, which
 * will be treated as head and tail silence part.  DC offset normalization
 * will be also performed if configured so.
 *
 * The triggered input speech data should be processed concurrently with the
 * detection for real-time recognition.  For this purpose, after the
 * beginning of speech input has been detected, the following triggered input
 * fragments (samples of a certain period in live input, or buffer size in
 * file input) are passed sequencially in turn to a callback function.
 * The callback function should be specified by the caller, typicaly to
 * store the recoded speech, or to process them into a frame-synchronous
 * recognition process.
 *
 * When source is a live input such as microphone, the device buffer will
 * overflow if the processing callback is slow.  In that case, some input
 * fragments may be lost.  To prevent this, the A/D-in part together with
 * speech detection will become an independent thread if @em pthread functions
 * are supported.  The A/D-in and detection thread will cooperate with
 * the original main thread through @a speech buffer, like the followings:
 *
 *    - Thread 1: A/D-in and speech detection thread
 *        - reads audio input from source device and perform speech detection.
 *          The detected fragments are immediately appended
 *          to the @a speech buffer.
 *        - will be detached after created, and run forever till the main
 *          thread dies.
 *    - Thread 2: Main thread
 *        - performs speech processing and recognition.
 *        - watches @a speech buffer, and if detect appendings of new samples
 *          by the Thread 1, proceed the processing for the appended samples
 *          and purge the finished samples from @a speech buffer.
 *
 * adin_setup_func() is used to switch audio input by specifying device-dependent
 * open/read/close functions, and should be called at first.
 * Function adin_setup_param() should be called after adin_setup_func() to
 * set various parameters for speech detection.
 * The adin_go() function is the top function that will be called from
 * outside, to perform actual input processing.  adin_cut() is
 * the main function to read audio input and detect speech segment.
 * </EN>
 *
 * @sa adin.c
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/// Define this if you want to output a debug message for threading
#undef THREAD_DEBUG
/// Enable some fixes relating adinnet+module
#define TMP_FIX_200602		

/**
 * @name Variables of zero-cross parameters and buffer sizes
 * 
 */
//@{
static int c_length  = 5000;	///< Computed length of cycle buffer for zero-cross, actually equals to head margin length
static int c_offset  = 0;	///< Static data DC offset (obsolute, should be 0)
static int wstep   = DEFAULT_WSTEP;	///< Data fragment size
static int thres;		///< Input Level threshold (0-32767)
static int noise_zerocross;	///< Computed threshold of zerocross num in the cycle buffer
static int nc_max;		///< Computed number of fragments for tail margin
//@}

/**
 * @name Variables for delayed tail silence processing
 * 
 */
//@{
static SP16 *swapbuf;		///< Buffer for re-triggering in tail margin
static int sbsize, sblen;	///< Size and current length of @a swapbuf
static int rest_tail;		///< Samples not processed yet in swap buffer
//@}

/**
 * @name Work area for device configurations for local use
 * 
 */
//@{
static boolean (*ad_resume)();	///< Function pointer to (re)start input
static boolean (*ad_pause)();	///< Function pointer to stop input
static int (*ad_read)(SP16 *, int); ///< Function pointer to read in input samples
static boolean adin_cut_on;	///< TRUE if do input segmentation by silence
static boolean silence_cut_default; ///< Device-dependent default value of adin_cut_on()
static boolean strip_flag;	///< TRUE if skip invalid zero samples
static boolean enable_thread = FALSE;	///< TRUE if input device needs threading
static boolean ignore_speech_while_recog = TRUE; ///< TRUE if ignore speech input between call, while waiting recognition process
static boolean need_zmean;	///< TRUE if perform zmeansource
//@}

#ifdef HAVE_PTHREAD
static void adin_thread_create(); ///< create and start A/D-in and detection thread 
#endif

/** 
 * Store the given device-dependent functions and configuration values
 * to local work area.  This function will be called from adin_select()
 * via adin_register_func().
 * 
 */
void
adin_setup_func(int (*cad_read)(SP16 *, int), ///< [in] function to read input samples
		boolean (*cad_pause)(), ///< [in] function to stop input
		boolean (*cad_resume)(), ///< [in] function to (re-)start input
		boolean use_cut_def, ///< [in] TRUE if the device needs speech segment detection by default
		boolean need_thread ///< [in] TRUE if the device is live input and needs threading
		)
{
  ad_read = cad_read;
  ad_pause = cad_pause;
  ad_resume = cad_resume;
  silence_cut_default = use_cut_def;
#ifdef HAVE_PTHREAD
  enable_thread = need_thread;
#else
  if (need_thread == TRUE) {
    j_printerr("Warning: thread not supported, input may be corrupted on slow machines\n");
  }
#endif
}

/** 
 * Setup silence detection parameters (should be called after adin_select()).
 * If using pthread, the A/D-in and detection thread will be started at the end
 * of this function.
 * 
 * @param silence_cut [in] whether to perform silence cutting.
 *                 0=force off, 1=force on, 2=keep device-specific default
 * @param strip_zero [in] TRUE if enables stripping of zero samples 
 * @param cthres [in]  input level threshold (0-32767)
 * @param czc [in] zero-cross count threshold in a second
 * @param head_margin [in] header margin length in msec
 * @param tail_margin [in] tail margin length in msec
 * @param sample_freq [in] sampling frequency: just providing value for computing other variables
 * @param ignore_speech [in] TRUE if ignore speech input between call, while waiting recognition process
 * @param need_zeromean [in] TRUE if perform zero-mean subtraction
 */
void
adin_setup_param(int silence_cut, boolean strip_zero, int cthres, int czc, int head_margin, int tail_margin, int sample_freq, boolean ignore_speech, boolean need_zeromean)
{
  float samples_in_msec;
  if (silence_cut < 2) {
    adin_cut_on = (silence_cut == 1) ? TRUE : FALSE;
  } else {
    adin_cut_on = silence_cut_default;
  }
  strip_flag = strip_zero;
  thres = cthres;
  ignore_speech_while_recog = ignore_speech;
  need_zmean = need_zeromean;
  /* calc & set internal parameter from configuration */
  samples_in_msec = (float) sample_freq / (float)1000.0;
  /* cycle buffer length = head margin length */
  c_length = (int)((float)head_margin * samples_in_msec);	/* in msec. */
  /* compute zerocross trigger count threshold in the cycle buffer */
  noise_zerocross = czc * c_length / sample_freq;
  /* process step */
  wstep = DEFAULT_WSTEP;
  /* variables that comes from the tail margin length (in wstep) */
  nc_max = (int)((float)(tail_margin * samples_in_msec / (float)wstep)) + 2;
  sbsize = tail_margin * samples_in_msec + (c_length * czc / 200);

#ifdef HAVE_PTHREAD
  if (enable_thread) {
    /* create A/D-in thread here */
    adin_thread_create();
  }
#endif
}

/** 
 * Query function to check whether the input speech detection is on or off.
 * 
 * @return TRUE if on, FALSE if off.
 */
boolean
query_segment_on()
{
  return adin_cut_on;
}

/** 
 * Query function to check whether the input threading is on or off.
 * 
 * @return TRUE if on, FALSE if off.
 */
boolean
query_thread_on()
{
  return enable_thread;
}

/** 
 * Reset zero mean data to re-estimate zero mean at the next input.
 * 
 */
void
adin_reset_zmean()
{
  if (need_zmean) zmean_reset();
}


#ifdef HAVE_PTHREAD
/**
 * @name Variables related to POSIX threading
 *
 */
//@{
static pthread_t adin_thread;	///< Thread information
static pthread_mutex_t mutex;	///< Lock primitive
static SP16 *speech;		///< Unprocessed samples recorded by A/D-in thread
static int speechlen;		///< Current length of @a speech
/**
 * @brief  Semaphore to start/stop recognition.
 * 
 * If TRUE, A/D-in thread will store incoming samples to @a speech and
 * main thread will detect and process them.
 * If FALSE, A/D-in thread will still get input and check trigger as the same
 * as TRUE case, but does not store them to @a speech.
 * 
 */
static boolean transfer_online = FALSE;	
static boolean adinthread_buffer_overflowed = FALSE; ///< Will be set to TRUE if @a speech has been overflowed.
//@}
#endif

/**
 * @name Input data buffer
 * 
 */
//@{
static SP16 *buffer = NULL;	///< Temporary buffer to hold input samples
static int bpmax;		///< Maximum length of @a buffer
static int bp;			///< Current point to store the next data
static int current_len;		///< Current length of stored samples
static SP16 *cbuf;		///< Buffer for flushing cycle buffer just after detecting trigger 
static boolean down_sample = FALSE; ///< TRUE if perform down sampling from 48kHz to 16kHz
static SP16 *buffer48 = NULL; ///< Another temporary buffer to hold 48kHz inputs
static int io_rate; ///< frequency rate (should be 3 always for 48/16 conversion
//@}


/** 
 * Purge samples already processed in the temporary buffer @a buffer.
 * 
 * @param from [in] Purge samples in range [0..from-1].
 */
static void
adin_purge(int from)
{
  if (from > 0 && current_len-from > 0) {
    memmove(buffer, &(buffer[from]), (current_len - from) * sizeof(SP16));
  }
  bp = current_len - from;
}

/** 
 * @brief  Main A/D-in function
 *
 * In threaded mode, this function will detach and loop forever in ad-in
 * thread, storing triggered samples in @a speech, and telling the status
 * to another process thread via @a transfer_online.
 * The process thread, called from adin_go(), polls the length of
 * @a speech and @a transfer_online, and if there are stored samples,
 * process them.
 *
 * In non-threaded mode, this function will be called directly from
 * adin_go(), and triggered samples are immediately processed within here.
 *
 * In module mode, the function argument @a ad_check should be specified
 * to poll the status of incoming command from client while recognition.
 * 
 * @return -1 on error, 0 on end of stream, >0 when paused by external process.
 */
static int
adin_cut(
	 int (*ad_process)(SP16 *, int), ///< function to process the triggered samples
	 int (*ad_check)())	///< function periodically called while input processing
{
  static int i;
  static boolean is_valid_data;	///< TRUE if we are now triggered
  int ad_process_ret;
  int imax, len, cnt;
  static boolean end_of_stream;	/* will be set to TRUE if current input stream has reached the end (in case of file input or adinnet input).  If TRUE, no more input will be got by ad_read, but just process the already stored samples until it becomes empty */
  static int need_init = TRUE;	/* if TRUE, initialize buffer on startup */
  static int end_status;	/* return value */
  static boolean transfer_online_local;	/* local repository of transfer_online */
  
  static int zc;		/* count of zero cross */
  static int nc;		/* count of current tail silence segments */

  /*
   * there are 3 buffers:
   *   temporary storage queue: buffer[]
   *   cycle buffer for zero-cross counting: (in zc_e)
   *   swap buffer for re-starting after short tail silence
   *
   * Each samples are first read to buffer[], then passed to count_zc_e()
   * to find trigger.  Samples between trigger and end of speech are 
   * passed to (*ad_process) with pointer to the first sample and its length.
   *
   */

  /**********************/
  /* initialize buffers */
  /**********************/
  if (buffer == NULL) {		/* beginning of stream */
    buffer = (SP16 *)mymalloc(sizeof(SP16) * MAXSPEECHLEN);
    cbuf = (SP16 *)mymalloc(sizeof(SP16) * c_length);
    swapbuf = (SP16 *)mymalloc(sizeof(SP16) * sbsize);
  }
  if (down_sample) {
    io_rate = 3;		/* 48 / 16 (fixed) */
    if (buffer48 == NULL) {
      buffer48 = (SP16 *)mymalloc(sizeof(SP16) * MAXSPEECHLEN * io_rate);
    }
  }
  if (need_init) {
    bpmax = MAXSPEECHLEN;
    bp = 0;
    is_valid_data = FALSE;
    /* reset zero-cross status */
    if (adin_cut_on) {
      init_count_zc_e(thres, c_length, c_offset);
    }
    end_of_stream = FALSE;
    nc = 0;
    sblen = 0;
    need_init = FALSE;		/* for next call */
  }
      
  /****************/
  /* resume input */
  /****************/
  /* restart speech input if paused on the last call */
  if (ad_resume != NULL) {
    if ((*ad_resume)() == FALSE)  return(-1);
  }

  /*************/
  /* main loop */
  /*************/
  for (;;) {

    /****************************/
    /* read in new speech input */
    /****************************/
    if (end_of_stream) {
      /* already reaches end of stream, just process the rest */
      current_len = bp;
    } else {
      /*****************************************************/
      /* get samples from input device to temporary buffer */
      /*****************************************************/
      /* buffer[0..bp] is the current remaining samples */
      /*
	mic input - samples exist in a device buffer
        tcpip input - samples exist in a socket
        file input - samples in a file
	   
	Return value is the number of read samples.
	If no data exists in the device (in case of mic input), ad_read()
	will return 0.  If reached end of stream (in case end of file or
	receive end ack from tcpip client), it will return -1.
	If error, returns -2.
      */
      if (down_sample) {
	/* get 48kHz samples to temporal buffer */
	cnt = (*ad_read)(buffer48, (bpmax - bp) * io_rate);
      } else {
	cnt = (*ad_read)(&(buffer[bp]), bpmax - bp);
      }
      if (cnt < 0) {		/* end of stream or error */
	/* set the end status */
	if (cnt == -2) end_status = -1; /* end by error */
	else if (cnt == -1) end_status = 0; /* end by normal end of stream */
	/* now the input has been ended, 
	   we should not get further speech input in the next loop, 
	   instead just process the samples in the temporary buffer until
	   the entire data is processed. */
	end_of_stream = TRUE;		
	cnt = 0;			/* no new input */
	/* in case the first trial of ad_read() fails, exit this loop */
	if (bp == 0) break;
      }
      if (down_sample && cnt != 0) {
	/* convert to 16kHz  */
	cnt = ds48to16(&(buffer[bp]), buffer48, cnt, bpmax - bp);
	if (cnt < 0) {		/* conversion error */
	  j_printerr("adin_mic_read: error in down sampling\n");
	  end_status = -1;
	  end_of_stream = TRUE;
	  cnt = 0;
	  if (bp == 0) break;
	}
      }

      /*************************************************/
      /* some speech processing for the incoming input */
      /*************************************************/
      if (cnt > 0) {
	if (strip_flag) {
	  /* strip off successive zero samples */
	  len = strip_zero(&(buffer[bp]), cnt);
	  if (len != cnt) cnt = len;
	}
	if (need_zmean) {
	  /* remove DC offset */
	  sub_zmean(&(buffer[bp]), cnt);
	}
      }
      
      /* current len = current samples in buffer */
      current_len = bp + cnt;
    }
#ifdef THREAD_DEBUG
    if (end_of_stream) {
      printf("stream already ended\n");
    }
    printf("input: get %d samples [%d-%d]\n", current_len - bp, bp, current_len);
#endif

    /**************************************************/
    /* call the periodic callback (non threaded mode) */
    /*************************************************/
    /* this function is mainly for periodic checking of incoming command
       in module mode */
    /* in threaded mode, this will be done in process thread, not here in adin thread */
    if (ad_check != NULL
#ifdef HAVE_PTHREAD
	&& !enable_thread
#endif
	) {
      /* if ad_check() returns value < 0, termination of speech input is required */
      if ((i = (*ad_check)()) < 0) { /* -1: soft termination -2: hard termination */
	//	if ((i == -1 && current_len == 0) || i == -2) {
 	if (i == -2 ||
 	    (i == -1 && adin_cut_on && is_valid_data == FALSE) ||
 	    (i == -1 && !adin_cut_on && current_len == 0)) {
	  end_status = -2;	/* recognition terminated by outer function */
	  goto break_input;
	}
      }
    }

    /***********************************************************************/
    /* if no data has got but not end of stream, repeat next input samples */
    /***********************************************************************/
    if (current_len == 0) continue;

    /* When not adin_cut mode, all incoming data is valid.
       So is_valid_data should be set to TRUE when some input first comes
       till this input ends.  So, if some data comes, set is_valid_data to
       TRUE here. */ 
    if (!adin_cut_on && is_valid_data == FALSE && current_len > 0) {
      is_valid_data = TRUE;
    }

    /******************************************************/
    /* prepare for processing samples in temporary buffer */
    /******************************************************/
    
    wstep = DEFAULT_WSTEP;	/* process unit (should be smaller than cycle buffer) */

    /* imax: total length that should be processed at one ad_read() call */
    /* if in real-time mode and not threaded, recognition process 
       will be called and executed as the ad_process() callback within
       this function.  If the recognition speed is over the real time,
       processing all the input samples at the loop below may result in the
       significant delay of getting next input, that may result in the buffer
       overflow of the device (namely a microphone device will suffer from
       this). So, in non-threaded mode, in order to avoid buffer overflow and
       input frame dropping, we will leave here by processing 
       only one segment [0..wstep], and leave the rest in the temporary buffer.
    */
#ifdef HAVE_PTHREAD
    if (enable_thread) imax = current_len; /* process whole */
    else imax = (current_len < wstep) ? current_len : wstep; /* one step */
#else
    imax = (current_len < wstep) ? current_len : wstep;	/* one step */
#endif
    
    /* wstep: unit length for the loop below */
    if (wstep > current_len) wstep = current_len;

#ifdef THREAD_DEBUG
    printf("process %d samples by %d step\n", imax, wstep);
#endif

#ifdef HAVE_PTHREAD
    if (enable_thread) {
      /* get transfer status to local */
      pthread_mutex_lock(&mutex);
      transfer_online_local = transfer_online;
      pthread_mutex_unlock(&mutex);
    }
#endif

    /*********************************************************/
    /* start processing buffer[0..current_len] by wstep step */
    /*********************************************************/
    i = 0;
    while (i + wstep <= imax) {
      
      if (adin_cut_on) {

	/********************/
	/* check triggering */
	/********************/
	/* the cycle buffer in count_zc_e() holds the last
	   samples of (head_margin) miliseconds, and the zerocross
	   over the threshold level are counted within the cycle buffer */
	
	/* store the new data to cycle buffer and update the count */
	/* return zero-cross num in the cycle buffer */
	zc = count_zc_e(&(buffer[i]), wstep);
	
	if (zc > noise_zerocross) { /* now triggering */
	  
	  if (is_valid_data == FALSE) {
	    /*****************************************************/
	    /* process off, trigger on: detect speech triggering */
	    /*****************************************************/
	    
	    is_valid_data = TRUE;   /* start processing */
	    nc = 0;
#ifdef THREAD_DEBUG
	    printf("detect on\n");
#endif
	    /****************************************/
	    /* flush samples stored in cycle buffer */
	    /****************************************/
	    /* (last (head_margin) msec samples */
	    /* if threaded mode, processing means storing them to speech[].
	       if ignore_speech_while_recog is on (default), ignore the data
	       if transfer is offline (=while processing second pass).
	       Else, datas are stored even if transfer is offline */
	    if ( ad_process != NULL
#ifdef HAVE_PTHREAD
		 && (!enable_thread || !ignore_speech_while_recog || transfer_online_local)
#endif
		 ) {
	      /* copy content of cycle buffer to cbuf */
	      zc_copy_buffer(cbuf, &len);
	      /* Note that the last 'wstep' samples are the same as
		 the current samples 'buffer[i..i+wstep]', and
		 they will be processed later.  So, here only the samples
		 cbuf[0...len-wstep] will be processed
	      */
	      if (len - wstep > 0) {
#ifdef THREAD_DEBUG
		printf("callback for buffered samples (%d bytes)\n", len - wstep);
#endif
		ad_process_ret = (*ad_process)(cbuf, len - wstep);
		switch(ad_process_ret) {
		case 1:		/* segmentation notification from process callback */
#ifdef HAVE_PTHREAD
		  if (enable_thread) {
		    /* in threaded mode, just stop transfer */
		    pthread_mutex_lock(&mutex);
		    transfer_online = transfer_online_local = FALSE;
		    pthread_mutex_unlock(&mutex);
		  } else {
		    /* in non-threaded mode, set end status and exit loop */
		    end_status = 1;
		    adin_purge(i);
		    goto break_input;
		  }
		  break;
#else
		  /* in non-threaded mode, set end status and exit loop */
		  end_status = 1;
		  adin_purge(i);
		  goto break_input;
#endif
		case -1:		/* error occured in callback */
		  /* set end status and exit loop */
		  end_status = -1;
		  goto break_input;
		}
	      }
	    }
	    
	  } else {		/* is_valid_data == TRUE */
	    /******************************************************/
	    /* process on, trigger on: we are in a speech segment */
	    /******************************************************/
	    
	    if (nc > 0) {
	      
	      /*************************************/
	      /* re-triggering in trailing silence */
	      /*************************************/
	      
#ifdef THREAD_DEBUG
	      printf("re-triggered\n");
#endif
	      /* reset noise counter */
	      nc = 0;

#ifdef TMP_FIX_200602
	      if (ad_process != NULL
#ifdef HAVE_PTHREAD
		  && (!enable_thread || !ignore_speech_while_recog || transfer_online_local)
#endif
		  ) {
#endif
	      
	      /*************************************************/
	      /* process swap buffer stored while tail silence */
	      /*************************************************/
	      /* In trailing silence, the samples within the tail margin length
		 will be processed immediately, but samples after the tail
		 margin will not be processed, instead stored in swapbuf[].
		 If re-triggering occurs while in the trailing silence,
		 the swapped samples should be processed now to catch up
		 with current input
	      */
	      if (sblen > 0) {
#ifdef THREAD_DEBUG
		printf("callback for swapped %d samples\n", sblen);
#endif
		ad_process_ret = (*ad_process)(swapbuf, sblen);
		sblen = 0;
		switch(ad_process_ret) {
		case 1:		/* segmentation notification from process callback */
#ifdef HAVE_PTHREAD
		  if (enable_thread) {
		    /* in threaded mode, just stop transfer */
		    pthread_mutex_lock(&mutex);
		    transfer_online = transfer_online_local = FALSE;
		    pthread_mutex_unlock(&mutex);
		  } else {
		    /* in non-threaded mode, set end status and exit loop */
		    end_status = 1;
		    adin_purge(i);
		    goto break_input;
		  }
		  break;
#else
		  /* in non-threaded mode, set end status and exit loop */
		  end_status = 1;
		  adin_purge(i);
		  goto break_input;
#endif
		case -1:		/* error occured in callback */
		  /* set end status and exit loop */
		  end_status = -1;
		  goto break_input;
		}
	      }
#ifdef TMP_FIX_200602
	      }
#endif
	    }
	  } 
	} else if (is_valid_data == TRUE) {
	  
	  /*******************************************************/
	  /* process on, trigger off: processing tailing silence */
	  /*******************************************************/
	  
#ifdef THREAD_DEBUG
	  printf("TRAILING SILENCE\n");
#endif
	  if (nc == 0) {
	    /* start of tail silence: prepare valiables for start swapbuf[] */
	    rest_tail = sbsize - c_length;
	    sblen = 0;
#ifdef THREAD_DEBUG
	    printf("start tail silence, rest_tail = %d\n", rest_tail);
#endif
	  }

	  /* increment noise counter */
	  nc++;
	}
      }	/* end of triggering handlers */
      
      
      /********************************************************************/
      /* process the current segment buffer[i...i+wstep] if process == on */
      /********************************************************************/
      
      if (adin_cut_on && is_valid_data && nc > 0 && rest_tail == 0) {
	
	/* The current trailing silence is now longer than the user-
	   specified tail margin length, so the current samples
	   should not be processed now.  But if 're-triggering'
	   occurs in the trailing silence later, they should be processed
	   then.  So we just store the overed samples in swapbuf[] and
	   not process them now */
	
#ifdef THREAD_DEBUG
	printf("tail silence over, store to swap buffer (nc=%d, rest_tail=%d, sblen=%d-%d)\n", nc, rest_tail, sblen, sblen+wstep);
#endif
	if (sblen + wstep > sbsize) {
	  j_printerr("Error: swapbuf exceeded!\n");
	}
	memcpy(&(swapbuf[sblen]), &(buffer[i]), wstep * sizeof(SP16));
	sblen += wstep;
	
      } else {

	/* we are in a normal speech segment (nc == 0), or
	   trailing silence (shorter than tail margin length) (nc>0,rest_tail>0)
	   The current trailing silence is shorter than the user-
	   specified tail margin length, so the current samples
	   should be processed now as same as the normal speech segment */
	
#ifdef TMP_FIX_200602
	if (!adin_cut_on || is_valid_data == TRUE) {
#else
	if(
	   (!adin_cut_on || is_valid_data == TRUE)
#ifdef HAVE_PTHREAD
	   && (!enable_thread || !ignore_speech_while_recog || transfer_online_local)
#endif
	   ) {
#endif
	  if (nc > 0) {
	    /* if we are in a trailing silence, decrease the counter to detect
	     start of swapbuf[] above */
	    if (rest_tail < wstep) rest_tail = 0;
	    else rest_tail -= wstep;
#ifdef THREAD_DEBUG
	    printf("%d processed, rest_tail=%d\n", wstep, rest_tail);
#endif
	  }
#ifdef TMP_FIX_200602
	  if (ad_process != NULL
#ifdef HAVE_PTHREAD
	      && (!enable_thread || !ignore_speech_while_recog || transfer_online_local)
#endif
	      ) {

#else
	  if ( ad_process != NULL ) {
#endif
#ifdef THREAD_DEBUG
	    printf("callback for input sample [%d-%d]\n", i, i+wstep);
#endif
	    /* call external function */
	    ad_process_ret = (*ad_process)(&(buffer[i]),  wstep);
	    switch(ad_process_ret) {
	    case 1:		/* segmentation notification from process callback */
#ifdef HAVE_PTHREAD
	      if (enable_thread) {
		/* in threaded mode, just stop transfer */
		pthread_mutex_lock(&mutex);
		transfer_online = transfer_online_local = FALSE;
		pthread_mutex_unlock(&mutex);
	      } else {
		/* in non-threaded mode, set end status and exit loop */
		adin_purge(i+wstep);
		end_status = 1;
		goto break_input;
	      }
	      break;
#else
	      /* in non-threaded mode, set end status and exit loop */
	      adin_purge(i+wstep);
	      end_status = 1;
	      goto break_input;
#endif
	    case -1:		/* error occured in callback */
	      /* set end status and exit loop */
	      end_status = -1;
	      goto break_input;
	    }
	  }
	}
      }	/* end of current segment processing */

      
      if (adin_cut_on && is_valid_data && nc >= nc_max) {
	/*************************************/
	/* process on, trailing silence over */
	/* = end of input segment            */
	/*************************************/
#ifdef THREAD_DEBUG
	printf("detect off\n");
#endif
	/* end input by silence */
	is_valid_data = FALSE;	/* turn off processing */
	sblen = 0;
#ifdef HAVE_PTHREAD
	if (enable_thread) { /* just stop transfer */
	  pthread_mutex_lock(&mutex);
	  transfer_online = transfer_online_local = FALSE;
	  pthread_mutex_unlock(&mutex);
	} else {
	  adin_purge(i+wstep);
	  end_status = 1;
	  goto break_input;
	}
#else
	adin_purge(i+wstep);
	end_status = 1;
	goto break_input;
#endif
      }

      /*********************************************************/
      /* end of processing buffer[0..current_len] by wstep step */
      /*********************************************************/
      i += wstep;		/* increment to next wstep samples */
    }
    
    /* purge processed samples and update queue */
    adin_purge(i);

    /* end of input by end of stream */
    if (end_of_stream && bp == 0) break;
  }

break_input:

  /****************/
  /* pause input */
  /****************/
  /* stop speech input */
  if (ad_pause != NULL) {
    if ((*ad_pause)() == FALSE) {
      j_printerr("Error: failed to pause recording\n");
      end_status = -1;
    }
  }

  if (end_of_stream) {			/* input already ends */
    if (bp == 0) {		/* rest buffer successfully flushed */
      /* reset status */
      if (adin_cut_on) end_count_zc_e();
      need_init = TRUE;		/* bufer status shoule be reset at next call */
    }
    end_status = (bp) ? 1 : 0;
  }
  
  return(end_status);
}








#ifdef HAVE_PTHREAD
/***********************/
/* threading functions */
/***********************/

/*************************/
/* adin thread functions */
/*************************/

/** 
 * Callback for storing triggered samples to @a speech in A/D-in thread.
 * 
 * @param now [in] triggered fragment
 * @param len [in] length of above
 * 
 * @return always 0, to tell caller to continue recording.
 */
static int
adin_store_buffer(SP16 *now, int len)
{
  if (speechlen + len > MAXSPEECHLEN) {
    /* just mark as overflowed, and continue this thread */
    pthread_mutex_lock(&mutex);
    adinthread_buffer_overflowed = TRUE;
    pthread_mutex_unlock(&mutex);
    return(0);
  }
  pthread_mutex_lock(&mutex);
  memcpy(&(speech[speechlen]), now, len * sizeof(SP16));
  speechlen += len;
  pthread_mutex_unlock(&mutex);
#ifdef THREAD_DEBUG
  printf("input: stored %d samples, total=%d\n", len, speechlen);
#endif
  /* output progress bar in dots */
  /*if ((++dotcount) % 3 == 1) j_printerr(".");*/
  return(0);			/* continue */
}

/** 
 * A/D-in thread main function: just call adin_cut() with storing function.
 * 
 * @param dummy [in] a dummy data, not used.
 */
void
adin_thread_input_main(void *dummy)
{
  adin_cut(adin_store_buffer, NULL);
}

/** 
 * Start new A/D-in thread, and also initialize buffer @a speech.
 * 
 */
static void
adin_thread_create()
{
  /* init storing buffer */
  speechlen = 0;
  speech = (SP16 *)mymalloc(sizeof(SP16) * MAXSPEECHLEN);

  transfer_online = FALSE; /* tell adin-mic thread to wait at initial */
  adinthread_buffer_overflowed = FALSE;

  if (pthread_mutex_init(&(mutex), NULL) != 0) { /* error */
    j_error("Error: pthread: cannot initialize mutex\n");
  }
  if (pthread_create(&adin_thread, NULL, (void *)adin_thread_input_main, NULL) != 0) {
    j_error("Error: pthread: failed to create AD-in thread\n");
  }
  if (pthread_detach(adin_thread) != 0) { /* not join, run forever */
    j_error("Error: pthread: failed to detach AD-in thread\n");
  }
  j_printerr("AD-in thread created\n");
}

/****************************/
/* process thread functions */
/****************************/
/* used for module mode: return value: -2 = input cancellation forced by control module */

/** 
 * @brief  Main function of processing triggered samples at main thread.
 *
 * Wait for the new samples to be stored in @a speech by A/D-in thread,
 * and if found, process them.
 * 
 * @param ad_process [in] function to process the recorded fragments
 * @param ad_check [in] function to be called periodically for checking
 * incoming user command in module mode.
 * 
 * @return -2 when input terminated by result of the @a ad_check function,
 * -1 on error, 0 on end of stream, >0 if successfully segmented.
 */
static int
adin_thread_process(int (*ad_process)(SP16 *, int), int (*ad_check)())
{
  int prev_len, nowlen;
  int ad_process_ret;
  int i;
  boolean overflowed_p;
  boolean transfer_online_local;

  /* reset storing buffer --- input while recognition will be ignored */
  pthread_mutex_lock(&mutex);
  /*if (speechlen == 0) transfer_online = TRUE;*/ /* tell adin-mic thread to start recording */
  transfer_online = TRUE;
#ifdef THREAD_DEBUG
  printf("process: reset, speechlen = %d, online=%d\n", speechlen, transfer_online);
#endif
  pthread_mutex_unlock(&mutex);

  /* main processing loop */
  prev_len = 0;
  for(;;) {
    /* get current length (locking) */
    pthread_mutex_lock(&mutex);
    nowlen = speechlen;
    overflowed_p = adinthread_buffer_overflowed;
    transfer_online_local = transfer_online;
    pthread_mutex_unlock(&mutex);
    /* check if other input thread has overflowed */
    if (overflowed_p) {
      j_printerr("Warning: too long input (> %d samples), segmented now\n", MAXSPEECHLEN);
      /* segment input here */
      pthread_mutex_lock(&mutex);
      adinthread_buffer_overflowed = FALSE;
      speechlen = 0;
      transfer_online = transfer_online_local = FALSE;
      pthread_mutex_unlock(&mutex);
      return(1);		/* return with segmented status */
    }
    /* callback poll */
    if (ad_check != NULL) {
      if ((i = (*ad_check)()) < 0) {
	if ((i == -1 && nowlen == 0) || i == -2) {
	  pthread_mutex_lock(&mutex);
	  transfer_online = transfer_online_local = FALSE;
	  speechlen = 0;
	  pthread_mutex_unlock(&mutex);
	  return(-2);
	}
      }
    }
    if (prev_len < nowlen) {
#ifdef THREAD_DEBUG
      printf("process: proceed [%d-%d]\n",prev_len, nowlen);
#endif
      /* got new sample, process */
      /* As the speech[] buffer is monotonously increase,
	 content of speech buffer [prev_len..nowlen] would not alter
	 in both threads
	 So locking is not needed while processing.
       */
      /*printf("main: read %d-%d\n", prev_len, nowlen);*/
      if (ad_process != NULL) {
	ad_process_ret = (*ad_process)(&(speech[prev_len]),  nowlen - prev_len);
#ifdef THREAD_DEBUG
	printf("ad_process_ret=%d\n",ad_process_ret);
#endif
	switch(ad_process_ret) {
	case 1:			/* segmented */
	  /* segmented by callback function */
	  /* purge processed samples and keep transfering */
	  pthread_mutex_lock(&mutex);
	  if(speechlen > nowlen) {
	    memmove(buffer, &(buffer[nowlen]), (speechlen - nowlen) * sizeof(SP16));
	    speechlen = speechlen - nowlen;
	  } else {
	    speechlen = 0;
	  }
	  transfer_online = transfer_online_local = FALSE;
	  pthread_mutex_unlock(&mutex);
	  /* keep transfering */
	  return(1);		/* return with segmented status */
	case -1:		/* error */
	  pthread_mutex_lock(&mutex);
	  transfer_online = transfer_online_local = FALSE;
	  pthread_mutex_unlock(&mutex);
	  return(-1);		/* return with error */
	}
      }
      prev_len = nowlen;
    } else {
      if (transfer_online_local == FALSE) {
	/* segmented by zero-cross */
	/* reset storing buffer for next input */
	pthread_mutex_lock(&mutex);
	speechlen = 0;
	pthread_mutex_unlock(&mutex);
        break;
      }
      usleep(100000);   /* wait = 0.1sec*/            
    }
  }

  /* as threading assumes infinite input */
  /* return value should be 1 (segmented) */
  return(1);
}
#endif /* HAVE_PTHREAD */




/**
 * @brief  Top function to start input processing
 *
 * If threading mode is enabled, this function simply enters to
 * adin_thread_process() to process triggered samples detected by
 * another running A/D-in thread.
 *
 * If threading mode is not available or disabled by either device requirement
 * or OS capability, this function simply calls adin_cut() to detect speech
 * segment from input device and process them concurrently by one process.
 * 
 * @param ad_process [in] function to process the recorded fragments
 * @param ad_check [in] function to be called periodically for checking
 * incoming user command in module mode.
 * 
 * @return the same as adin_thread_process() in threading mode, or
 * same as adin_cut() when non-threaded mode.
 */
int
adin_go(int (*ad_process)(SP16 *, int), int (*ad_check)())
{
#ifdef HAVE_PTHREAD
  if (enable_thread) {
    return(adin_thread_process(ad_process, ad_check));
  }
#endif
  return(adin_cut(ad_process, ad_check));
}


/** 
 * Setup functions and parameters for 48kHz-to-16kHz down sampling.
 * 
 */
void
adin_setup_48to16()
{
  /* initialize filters */
  ds48to16_setup();
  
  /* set switch */
  down_sample = TRUE;
}
