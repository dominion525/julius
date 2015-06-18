/**
 * @file   main.c
 * @author Akinobu Lee
 * @date   Wed May 18 15:02:55 2005
 * 
 * <JA>
 * @brief  Julius/Julian メイン
 * </JA>
 * 
 * <EN>
 * @brief  Main function of Julius/Julian
 * </EN>
 * 
 * $Revision: 1.10 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#define GLOBAL_VARIABLE_DEFINE	///< Actually make global vars in global.h
#include <julius.h>
#include <signal.h>
#if defined(_WIN32) && !defined(__CYGWIN32__)
#include <mbctype.h>
#include <mbstring.h>
#endif


/* ---------- utility functions -----------------------------------------*/
#ifdef REPORT_MEMORY_USAGE
/** 
 * <JA>
 * 通常終了時に使用メモリ量を調べて出力する (Linux, sol2
 * 
 * </JA>
 * <EN>
 * Get process size and output on normal exit. (Linux, sol2)
 * 
 * </EN>
 */
static void
print_mem()
{
  char buf[200];
  sprintf(buf,"ps -o vsz,rss -p %d",getpid());
  system(buf);
  j_flushprint();
  fflush(stderr);
}
#endif
	  

/* --------------------- speech buffering ------------------ */
/**
 * Temporal buffer to save the recorded-but-unprocessed samples
 * when the length of a speech segment exceeds the limit
 * (i.e. MAXSPEECHLEN samples).  They will be restored on the
 * next input at the top of the recording buffer.
 * 
 */
static SP16 *overflowed_samples = NULL;
/**
 * Length of above.
 * 
 */
static int overflowed_samplenum;
/** 
 * <JA>
 * @brief  検出区間の音声データをバッファに保存するための adin_go() callback
 *
 * この関数は，検出された音声入力を逐次バッファ @a speech に記録して
 * いきます．バッファ処理モード（＝非リアルタイムモード）で認識を行なう
 * ときに用いられます．
 * 
 * @param now [in] 検出された音声波形データの断片
 * @param len [in] @a now の長さ(サンプル数)
 * 
 * @return エラー時 -1 (adin_go は即時中断する)，通常時 0 (adin_go は
 * 続行する)，区間終了要求時 1 (adin_go は現在の音声区間を閉じる)．
 * 
 * </JA>
 * <EN>
 * @brief  adin_go() callback to score each detected speech segment to buffer.
 *
 * This function records the incomping speech segments detected in adin_go()
 * to a buffer @a speech.  This function will be used when recognition runs
 * in buffered mode (= non-realtime mode).
 * 
 * @param now [in] input speech samples.
 * @param len [in] length of @a now in samples
 * 
 * @return -1 on error (tell adin_go() to terminate), 0 on success (tell
 * adin_go() to continue recording), or 1 when this function requires
 * input segmentation.
 * </EN>
 */
int
adin_cut_callback_store_buffer(SP16 *now, int len)
{
  if (module_mode) {
    /* poll the command buffer for each input fragment */
    msock_check_and_process_command();
  }
  if (speechlen == 0) {		/* first part of a segment */
    /* output start recording/processing message */
    status_recstart();
    if (module_mode) {
      /* module termination check */
      if (module_wants_terminate() ||/* TERMINATE ... force termination */
	  !module_is_active()) { /* PAUSE ... keep recording when *triggering */
	return(-2);
      }
    }
    if (overflowed_samples) {	/* last input was overflowed */
      /* restore last overflowed samples */
      memcpy(&(speech[0]), overflowed_samples, sizeof(SP16)*overflowed_samplenum);
      speechlen += overflowed_samplenum;
      free(overflowed_samples);
      overflowed_samples = NULL;
    }
  }
  if (speechlen + len > MAXSPEECHLEN) {
    /*j_printerr("Error: too long input (> %d samples)\n", MAXSPEECHLEN);*/
    j_printerr("Warning: too long input (> %d samples), segmented now\n", MAXSPEECHLEN);
    /* store the overflowed samples for next segment, and end segment */
    {
      int getlen, restlen;
      getlen = MAXSPEECHLEN - speechlen;
      restlen = len - getlen;
      overflowed_samples = (SP16 *)mymalloc(sizeof(SP16)*restlen);
      memcpy(overflowed_samples, &(now[getlen]), restlen * sizeof(SP16));
      if (record_dirname != NULL) {
	record_sample_write(&(now[getlen]), restlen);
      }
      overflowed_samplenum = restlen;
      memcpy(&(speech[speechlen]), now, getlen * sizeof(SP16));
      if (record_dirname != NULL) {
	record_sample_write(now, getlen);
      }
      speechlen += getlen;
    }
    return(1);			/* tell adin_go to end segment */
  }
  if (module_mode) {
    /* poll module command and terminate here if requested */
    if (module_wants_terminate()) {/* TERMINATE ... force termination */
      speechlen = 0;
      return(-2);
    }
  }
  /* store now[0..len] to speech[speechlen] */
  memcpy(&(speech[speechlen]), now, len * sizeof(SP16));
  if (record_dirname != NULL) {
    record_sample_write(now, len);
  }
  speechlen += len;
  return(0);			/* tell adin_go to continue reading */
}

/* return malloced next file name from inputlist (return NULL on error) */
/** 
 * <JA>
 * @brief  入力ファイルリストから入力ファイル名を1行読み込んで返す．
 *
 * この関数は MFCC ファイル入力時に入力リストファイル inputlist_filename
 * から入力ファイル名を1行読み込み，その値を新たに malloc されたバッファ
 * に格納して返します．空行や "#" で始まる行は無視されます．
 *
 * この関数は MFCC ファイル入力時 (-input mfcfile) のときのみ用いられます．
 * 音声ファイル入力の場合は入力ファイルリストは adin_go() 内で扱われます．
 * 
 * @return 次の入力ファイル名が格納された，新たに malloc されたバッファへ
 * のポインタ，あるいは次の入力ファイル名がなければ（EOF に達したら）NULL．
 * </JA>
 * <EN>
 * @brief  Get the next input filename from input list file and return
 * the filename.
 *
 * This function is used when "-input mfcfile" and "-filelist" is both
 * specified, to get input filenames successively from a given input list
 * file "inputlist_filename".  When called, the next filename will be
 * read from the list file
 * and stored in a newly malloced buffer.  Blank lines or lines starting
 * with "#" in the list file will be discarded.
 *
 * This function will only be used when "-filelist" is used when MFCC
 * input is specified ("-input mfcfile").  When speech waveform input,
 * functions in adin_go() will handles the list file instead of this
 * function.
 * 
 * @return pointer to a newly malloced buffer that holds the next input
 * filename, or NULL when no next file availbale (the list file reached
 * EOF).
 * </EN>
 */
static char *
mfcfilelist_nextfile()
{
  static FILE *mfclist = NULL;	/* current filelist of MFC file (-filelist) */
  static char *buf;
  
  if (mfclist == NULL) {	/* not opened yet */
    if ((mfclist = fopen(inputlist_filename, "r")) == NULL) { /* open error */
      j_error("inputlist open error\n");
    }
  }
  buf = mymalloc(MAXLINELEN);
  while(getl_fp(buf, MAXLINELEN, mfclist) != NULL) {
    if (buf[0] == '\0') continue; /* skip blank line */
    if (buf[0] == '#') continue; /* skip commented-out line */
    /* read a filename */
    return buf;
  }
  /* end of inputfile list */
  free(buf);
  fclose(mfclist);
  mfclist = NULL;
  return NULL;
}

				       
/* ------------------------------------------------------------------- */
/* ------------- Main Recognition Loop ------------------------------- */
/* ------------------------------------------------------------------- */

/** 
 * <JA>
 * @brief  メインの音声認識実行ループ
 *
 * この関数は音声認識を実行するメインループです．最初にシステムの初期化を
 * 行ない，その後イベントループに入ります．イベントループでは音声区間が検出
 * されると，その音声区間について認識処理の第1パス，第2パスを行ない，結果を
 * 出力します．またモジュールモードの場合は命令コマンドの処理の呼び出しを
 * 行ないます．
 * </JA>
 * <EN>
 * @brief  Main recognition process loop.
 *
 * This function is a main loop to execute recognition.  First the whole
 * recognition system is setup, and then it enters a main event loop.
 * If the start of speech input is detected, the recognition process of the
 * first pass and second pass will be performed in turn, and the result will
 * be output, and return to the event loop.  In module mode, the module
 * commands from module client will also be received and processed here.
 * </EN>
 */
void
main_recognition_loop()
{
  char *speechfilename;	/* pathname of speech file or MFCC file */
  HTK_Param *param = NULL;		/* parameter vector */
  HTK_Param *selected_param;
  int ret;
  int file_counter;
  float seclen, mseclen;
  boolean process_online = FALSE; /* TRUE if audio stream is now open and engine is either listening audio stream or recognizing a speech.  FALSE on startup or when in pause specified by a module command. */

  /***************************/
  /* Model/IO initialization */
  /***************************/
  /* initialize all models, work area, parameters to bootup system */
  final_fusion();

  /* initialize and standby audio input device */
  adin_initialize();
  
  /* print out system information */
  print_info();

  /* reset file count */
  file_counter = 0;

#ifdef VISUALIZE
  /* Visualize: initialize GTK */
  visual_init();
#endif

  /***************************/
  /***************************/
  /** Main Recognition Loop **/
  /***************************/
  /***************************/
  for (;;) {

    j_printf("\n");
    if (verbose_flag) j_printf("------\n");
    j_flushprint();

    /*********************/
    /* open input stream */
    /*********************/
    if (speech_input == SP_MFCFILE) {
      /************************************/
      /* read pre-analyzed parameter file */
      /************************************/
      VERMES("### read analyzed parameter\n");
      /* from MFCC parameter file (in HTK format) */
      if (inputlist_filename != NULL) {	/* has filename list */
	speechfilename = mfcfilelist_nextfile();
      } else {
	speechfilename = get_line("enter MFCC filename->");
      }
      if (speechfilename == NULL) {
	/* end */
	j_printerr("%d files processed\n", file_counter);
#ifdef REPORT_MEMORY_USAGE
	print_mem();
#endif
	j_exit();
      }
      if (verbose_flag) j_printf("\ninput MFCC file: %s\n",speechfilename);
      /* read parameter file */
      param = new_param();
      if (rdparam(speechfilename, param) == FALSE) {
	j_printerr("error in reading parameter file: %s\n",speechfilename);
	free(speechfilename);
	free_param(param);
	continue;
      }
      /* check and strip invalid frames */
      if (strip_zero_sample) {
	param_strip_zero(param);
      }
      free(speechfilename);
      /* output frame length */
      status_param(param);
      /* count number of processed files */
      file_counter++;
    } else {			/* raw speech input */
      /********************************/
      /* ready to read waveform input */
      /********************************/
      VERMES("### read waveform input\n");
      /* begin A/D input */
      if (adin_begin() == FALSE) {
	/* failed to begin stream, terminate */
	if (speech_input == SP_RAWFILE) {
	  j_printerr("%d files processed\n", file_counter);
	  j_exit();  /* end of file list */
	} else if (speech_input == SP_STDIN) {
	  j_exit();  /* end of input */
	} else {
	  j_error("failed to begin input stream\n");
	}
      }
      /* count number of processed files */
      if (speech_input == SP_RAWFILE) {
	file_counter++;
      }
    }
    
#ifdef USE_DFA
    /* if no grammar specified on startup, start with pause status */
    if (module_mode) {
      if (dfa == NULL || winfo == NULL) { /* stop when no grammar found */
	msock_exec_command("PAUSE");
      }
    }
#endif

    if (!module_mode) {
      /* if not module mode, process becomes online after all initialize done */
      process_online = TRUE;
      status_process_online();
    }
  
    /******************************************************************/
    /* do recognition for each incoming segment from the input stream */
    /******************************************************************/
    while (1) {

    start_recog:

      if (module_mode) {
	/*****************************/
	/* module command processing */
	/*****************************/
	/* If recognition is running (active), commands are polled only once
	   here, and if any, process the command, and continue the recognition.
	   If recognition is sleeping (inactive), wait here for any command to
	   come, and process them until recognition is activated by the
	   commands
	 */
	/* Output process status when status change occured by module command */
	if (process_online != module_is_active()) {
	  process_online = module_is_active();
	  if (process_online) status_process_online();
	  else status_process_offline();
	}
	if (module_is_active()) {
	  /* process is now active: check a command in buffer and process if any */
	  msock_check_and_process_command();
	}
	module_reset_reload();	/* reset reload flag here */
	while (! module_is_active()) {    
	  /* now sleeping, wait for another command */
	  /* we will stop here and wait for another command */
	  /* until status turns to active */
	  msock_process_command();
	}
	/* update process status */
	if (process_online != module_is_active()) {
	  process_online = module_is_active();
	  if (process_online) status_process_online();
	  else status_process_offline();
	}
#ifdef USE_DFA
	/*********************************************************/
	/* check for grammar to change, and rebuild if necessary */
	/*********************************************************/
	multigram_exec();
	if (dfa == NULL || winfo == NULL) { /* stop when no grammar found */
	  msock_exec_command("PAUSE");
	  goto start_recog;
	}
#endif
      }

      if (speech_input == SP_MFCFILE) {
	/************************/
	/* parameter file input */
	/************************/
	/********************************/
	/* check the analized parameter */
	/********************************/
	/* parameter type check --- compare the type to that of HMM,
	   and adjust them if necessary */
	if (paramtype_check_flag) {
	  /* return param itself or new malloced param */
	  selected_param = new_param_check_and_adjust(hmminfo, param, verbose_flag);
	  if (selected_param == NULL) { /* failed */
	    free_param(param);
	    param = NULL;
	    goto end_recog;
	  }
	  param = selected_param;
	}
	/* whole input is already read, so set input status to end of stream */
	/* and jump to the start point of 1st pass */
	ret = 0;
      } else {
	/****************************************************/
	/* raw wave data input (mic, file, adinnet, etc...) */
	/****************************************************/
	if (realtime_flag) {
	  /********************************************/
	  /* REALTIME ON-THE-FLY DECODING OF 1ST-PASS */
	  /********************************************/
	  /* store, analysis and search in a pipeline  */
	  /* main function is RealTimePipeLine() at realtime-1stpass.c, and
	     it will be periodically called for each incoming input segment
	     from the AD-in function adin_go().  RealTimePipeLine() will be
	     called as a callback function from adin_go() */
	  /* after this part, directly jump to the beginning of the 2nd pass */
#ifdef SP_BREAK_CURRENT_FRAME
	  if (rest_param) {
	    /*****************************************************************/
	    /* short-pause segmentation: process last remaining frames first */
	    /*****************************************************************/
	    /* last was segmented by short pause */
	    /* the margin segment in the last input should be re-processed first */
	    /* process the last remaining parameters */
	    ret = RealTimeResume();
	    if (ret < 0) {		/* error end in the margin */
	      j_error("error in resuming last fragment\n"); /* exit now! */
	    }
	    if (ret != 1) {	/* if segmented again in the margin, not process the rest */
	      /* last parameters has been processed, so continue with the
		 current input as normal */
	      /* output listening start message */
	      status_recready();
	      if (module_mode) {
		/* tell module to start recording, and command check in adin */
		ret = adin_go(RealTimePipeLine, msock_check_in_adin);
	      } else {
		/* process the incoming input as normal */
		ret = adin_go(RealTimePipeLine, NULL);
	      }
	      if (ret < 0) {		/* error end in adin_go */
		if (module_mode && (ret == -2 || module_wants_terminate())) {	/* terminated by module */
		  RealTimeTerminate();
		  param = NULL;
		  goto end_recog; /* cancel this recognition */
		}
		j_error("error in adin_go\n");          /* exit now! */
	      }
	    }
	    
	  } else {
	    /* last was not segmented, process the incoming input  */
#endif
	    /**********************************/
	    /* process incoming speech stream */
	    /**********************************/
	    /* end of this input will be determined by either end of stream
	       (in case of file input), or silence detection by adin_go(), or
	       'TERMINATE' command from module (if module mode) */
	    /* prepare work area for on-the-fly processing */
	    RealTimePipeLinePrepare();
	    /* output 'listening start' message */
	    status_recready();
	    /* process the incoming input */
	    if (module_mode) {
	      ret = adin_go(RealTimePipeLine, msock_check_in_adin);
	    } else {
	      ret = adin_go(RealTimePipeLine, NULL); 
	    }
	    if (ret < 0) {		/* error end in adin_go */
	      if (module_mode && (ret == -2 || module_wants_terminate())) {	/* terminated by module */
		RealTimeTerminate();
		param = NULL;
		goto end_recog;
	      }
	      j_error("error in adin_go\n");            /* exit now! */
	    }
#ifdef SP_BREAK_CURRENT_FRAME
	  }
#endif
	  /******************************************************************/
	  /* speech stream has been processed on-the-fly, and 1st pass ends */
	  /******************************************************************/
	  /* last procedure of 1st-pass */
	  param = RealTimeParam(&backmax);
	  /* output stopped recording message */
	  status_recend();
	  /* output frame length */
	  status_param(param);
	  if (module_mode) {
	    /* if terminate signal has been received, discard this input */
	    if (module_wants_terminate()) goto end_recog;
	  }
	  /* end of 1st pass, jump to 2nd pass */
	  goto end_1pass;
	  
	} /* end of realtime_flag && speech stream input */
	
	/******************************************/
	/* buffered speech input (not on-the-fly) */
	/******************************************/
#ifdef SP_BREAK_CURRENT_FRAME
	if (rest_param == NULL) { /* no segment left */
#endif
	  /****************************************/
	  /* store raw speech samples to speech[] */
	  /****************************************/
	  speechlen = 0;
	  param = NULL;
	  /* if needed, begin recording the incoming speech data to a file */
	  if (record_dirname != NULL) {
	    record_sample_open();
	  }
	  /* output 'listening start' message */
	  status_recready();
	  if (module_mode) {
	    /* tell module to start recording */
	    /* the "adin_cut_callback_store_buffer" simply stores
	       the input speech to a buffer "speech[]" */
	    /* end of this input will be determined by either end of stream
	       (in case of file input), or silence detection by adin_go(), or
	       'TERMINATE' command from module (if module mode) */
	    ret = adin_go(adin_cut_callback_store_buffer, msock_check_in_adin);
	  } else {
	    ret = adin_go(adin_cut_callback_store_buffer, NULL);
	  }
	  if (ret < 0) {		/* error end in adin_go */
	    if (module_mode && (ret == -2 || module_wants_terminate())) {	/* terminated by module */
	      goto end_recog;
	    }
	    j_error("error in adin_go\n");              /* exit now! */
	  }
	  /* output stopped recording message */
	  status_recend();

	  /* output recorded length */
	  seclen = (float)speechlen / (float)para.smp_freq;
	  j_printf("%d samples (%.2f sec.)\n", speechlen, seclen);

	  /* -rejectshort 指定時, 入力が指定時間以下であれば
	     ここで入力を棄却する */
	  /* when using "-rejectshort", and input was shorter than
	     specified, reject the input here */
	  if (rejectshortlen > 0) {
	    if (seclen * 1000.0 < rejectshortlen) {
	      result_rejected("too short input");
	      goto end_recog;
	    }
	  }
      
	  /**********************************************/
	  /* acoustic analysis and encoding of speech[] */
	  /**********************************************/
	  VERMES("### speech analysis (waveform -> MFCC)\n");
	  /* CMN will be computed for the whole buffered input */
	  param = new_wav2mfcc(speech, speechlen);
	  if (param == NULL) {
	    ret = -1;
	    goto end_recog;
	  }

	  /* if terminate signal has been received, cancel this input */
	  if (module_mode && module_wants_terminate()) goto end_recog;

	  /* output frame length */
	  status_param(param);

#ifdef SP_BREAK_CURRENT_FRAME
	}
#endif
      }	/* end of data input */
      /* parameter has been got in 'param' */

      /******************************************************/
      /* 1st-pass --- backward search to compute heuristics */
      /******************************************************/
      /* (for buffered speech input and HTK parameter file input) */
#ifdef USE_NGRAM
      VERMES("### Recognition: 1st pass (LR beam with 2-gram)\n");
#else
      VERMES("### Recognition: 1st pass (LR beam with word-pair grammar)\n");
#endif
/* 
 * #ifdef WPAIR
 *     VERMES("with word-pair approx. ");
 * #else
 *     VERMES("with 1-best approx. ");
 * #endif
 *     VERMES("generating ");
 * #ifdef WORD_GRAPH
 *     VERMES("word graph\n");
 * #else
 *     VERMES("back trellis\n");
 * #endif
 */

      if (!realtime_flag) {
	/* prepare for outprob cache for each HMM state and time frame */
	outprob_prepare(param->samplenum);
      }

      if (module_mode) {
	/* if terminate signal has been received, cancel this input */
	if (module_wants_terminate()) goto end_recog;
      }

      /* execute computation of left-to-right backtrellis */
      get_back_trellis(param, wchmm, &backtrellis, &backmax);

    end_1pass:

      /**********************************/
      /* end processing of the 1st-pass */
      /**********************************/
      /* on-the-fly 1st pass processing will join here */
      
      /* -rejectshort 指定時, 入力が指定時間以下であれば探索失敗として */
      /* 第２パスを実行せずにここで終了する */
      /* when using "-rejectshort", and input was shorter than the specified
	 length, terminate search here and output recognition failure */
      if (rejectshortlen > 0) {
	mseclen = (float)param->samplenum * (float)para.smp_period * (float)para.frameshift / 10000.0;
	if (mseclen < rejectshortlen) {
	  result_rejected("too short input");
	  goto end_recog;
	}
      }
  
      /* if [-1pass] is specified, terminate search here */
      if (compute_only_1pass) {
	goto end_recog;
      }

      /* if backtrellis function returns with bad status, terminate search */
      if (backmax == LOG_ZERO) {
	/* j_printerr("Terminate 2nd pass.\n"); */
	result_pass2_failed(wchmm->winfo);
	ret = -1;
	goto end_recog;
      }

      /* if terminate signal has been received, cancel this input */
      if (module_mode && module_wants_terminate()) goto end_recog;

      /* if GMM is specified and result are to be rejected, terminate search here */
      if (gmm_reject_cmn_string != NULL) {
	if (! gmm_valid_input()) {
	  result_rejected("by GMM");
	  goto end_recog;
	}
      }
      /***********************************************/
      /* 2nd-pass --- forward search with heuristics */
      /***********************************************/
#if !defined(PASS2_STRICT_IWCD) || defined(FIX_35_PASS2_STRICT_SCORE)    
      /* adjust trellis score not to contain outprob of the last frames */
      bt_discount_pescore(wchmm, &backtrellis, param);
#endif

#ifdef USE_NGRAM
      VERMES("### Recognition: 2nd pass (RL heuristic best-first with 3-gram)\n");
#else
      VERMES("### Recognition: 2nd pass (RL heuristic best-first with DFA)\n");
#endif

      /* execute stack-decoding search */
#ifdef USE_NGRAM
      wchmm_fbs(param, &backtrellis, backmax, stack_size, nbest, hypo_overflow, 0, 0);
#else  /* USE_DFA */
      if (multigramout_flag) {
	/* execute 2nd pass multiple times for each grammar sequencially */
	/* to output result for each grammar */
	MULTIGRAM *m;
	for(m = gramlist; m; m = m->next) {
	  if (m->active) {
	    j_printf("## search for gram #%d\n", m->id);
	    wchmm_fbs(param, &backtrellis, backmax, stack_size, nbest, hypo_overflow, m->cate_begin, m->dfa->term_num);
	  }
	}
      } else {
	/* only the best among all grammar will be output */
	wchmm_fbs(param, &backtrellis, backmax, stack_size, nbest, hypo_overflow, 0, dfa->term_num);
      }
#endif

    end_recog:
      /**********************/
      /* end of recognition */
      /**********************/

      /* update CMN info for next input (in case of realtime wave input) */
      if (speech_input != SP_MFCFILE && realtime_flag && param != NULL) {
	RealTimeCMNUpdate(param);
      }

#ifdef VISUALIZE
      /* execute visualization */
      visual_show(&backtrellis);
#endif

      /* free parameter work area */
      if (param != NULL) free_param(param);

      /* if needed, close the recording files */
      if (record_dirname != NULL) {
	record_sample_close();
      }

      VERMES("\n");

#ifdef SP_BREAK_CURRENT_FRAME
      /* param is now shrinked to hold only the processed input, and */
      /* the rests are holded in (newly allocated) "rest_param" */
      /* if this is the last segment, rest_param is NULL */
      if (rest_param != NULL) {
	/* process the rest parameters in the next loop */
	VERMES("<<<restart the rest>>>\n\n");
	param = rest_param;
      } else {
	/* input has reached end of stream, terminate program */
	if (ret <= 0 && ret != -2) break;
      }
#else
      /* input has reached end of stream, terminate program */
      if (ret <= 0 && ret != -2) break;
#endif

      /* recognition continues for next (silence-aparted) segment */
      
    } /* END OF STREAM LOOP */
    
    /* input stream ended. it will happen when
       - input speech file has reached the end of file, 
       - adinnet input has received end of segment mark from client,
       - adinnet input has received end of input from client,
       - adinnet client disconnected.
    */

    if (speech_input != SP_MFCFILE) {
      /* close the stream */
      adin_end();
    }
    /* return to the opening of input stream */
  }

}


/* ------------------------------------------------------------------- */
/* ------------- The Main Routine ------------------------------------ */
/* ------------------------------------------------------------------- */
/** 
 * <JA>
 * @brief  メイン関数
 *
 * 初期化を行ない，イベントループを呼び出す．
 * モジュールモードのときは main_module_loop()，通常のスタンドアローンモード
 * のときは main_recognition_loop() を呼ぶ．
 * 
 * @param argc [in] コマンド引数の数
 * @param argv [in] コマンド引数の配列
 * 
 * @return 通常終了時 0 を返す．
 * </JA>
 * <EN>
 * @brief  Main application function.
 *
 * This function does some boot-time initialization, and calls event loop.
 * main_module_loop() will be called when module mode, and
 * main_recognition_loop() will be called when normal standalone mode.
 * 
 * @param argc [in] number of command argument
 * @param argv [in] array of command argument
 * 
 * @return 0 on normal exit.
 * </EN>
 */
int
main(int argc, char *argv[])
{
  /*************************/
  /* System initialization */
  /*************************/
  /* initialize, set some default parameter before all */
  system_bootup();
  /* parse options and set variables */
  opt_parse(argc,argv,NULL);

#ifdef CHARACTER_CONVERSION
  if (j_printf_set_charconv(from_code, to_code) == FALSE) {
    j_error("Error: character set conversion setup failed\n");
  }
#endif /* CHARACTER_CONVERSION */

  /* check option values */
  check_specs();

  /*********************/
  /* Server/Standalone */
  /*********************/
  if (module_mode) {
    /* server mode */
    /* main_recognition_loop() will be called in main_module_loop() */
    main_module_loop();
  } else {
    /* standalone mode */
    main_recognition_loop();
  }

#ifdef CHARACTER_CONVERSION
  /* release character conversion instance */
  j_printf_clear_charconv();
#endif /* CHARACTER_CONVERSION */

  /* release global variables allocated when parsing options */
  opt_release();

  return 0;
}
