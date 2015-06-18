/**
 * @file   realtime-1stpass.c
 * @author Akinobu Lee
 * @date   Tue Aug 23 11:44:14 2005
 * 
 * <JA>
 * @brief  実時間認識のための第1パスの平行処理
 *
 * 第1パスを入力開始と同時にスタートし，入力と平行して認識処理を行うための
 * 関数が定義されている．
 * 
 * 通常，Julius の音声認識処理は以下の手順で main_recognition_loop() 内で
 * 実行される．
 *
 *  -# 音声入力 adin_go()  → 入力音声が speech[] に格納される
 *  -# 特徴量抽出 new_wav2mfcc() →speechから特徴パラメータを param に格納
 *  -# 第1パス実行 get_back_trellis() →param とモデルから単語トレリスの生成
 *  -# 第2パス実行 wchmm_fbs()
 *  -# 認識結果出力
 *
 * 第1パスを平行処理する場合，上記の 1 〜 3 が平行して行われる．
 * Julius では，この並行処理を，音声入力の断片が得られるたびに
 * 認識処理をその分だけ漸次的に進めることで実装している．
 * 
 *  - 特徴量抽出と第1パス実行を，一つにまとめてコールバック関数として定義．
 *  - 音声入力関数 adin_go() のコールバックとして上記の関数を与える
 *
 * 具体的には，ここで定義されている RealTimePipeLine() がコールバックとして
 * adin_go() に与えられる．adin_go() は音声入力がトリガするとその得られた入力
 * 断片ごとに RealTimePipeLine() を呼び出す．RealTimePipeLine() は得られた
 * 断片分について特徴量抽出と第1パスの計算を進める．
 *
 * CMN について注意が必要である．CMN は通常発話単位で行われるが，
 * マイク入力やネットワーク入力のように，第1パスと平行に認識を行う
 * 処理時は発話全体のケプストラム平均を得ることができない．バージョン 3.5
 * 以前では直前の発話5秒分(棄却された入力を除く)の CMN がそのまま次発話に
 * 流用されていたが，3.5.1 からは，上記の直前発話 CMN を初期値として
 * 発話内 CMN を MAP-CMN を持ちいて計算するようになった．なお，
 * 最初の発話用の初期CMNを "-cmnload" で与えることもでき，また
 * "-cmnnoupdate" で入力ごとの CMN 更新を行わないようにできる．
 * "-cmnnoupdate" と "-cmnload" と組み合わせることで, 最初にグローバルな
 * ケプストラム平均を与え，それを常に初期値として MAP-CMN することができる．
 *
 * 主要な関数は以下の通りである．
 *
 *  - RealTimeInit() - 起動時の初期化
 *  - RealTimePipeLinePrepare() - 入力ごとの初期化
 *  - RealTimePipeLine() - 第1パス平行処理用コールバック（上述）
 *  - RealTimeResume() - ショートポーズセグメンテーション時の認識復帰
 *  - RealTimeParam() - 入力ごとの第1パス終了処理
 *  - RealTimeCMNUpdate() - CMN の更新
 *  
 * </JA>
 * 
 * <EN>
 * @brief  On-the-fly decoding of the 1st pass.
 *
 * These are functions to perform on-the-fly decoding of the 1st pass
 * (frame-synchronous beam search).  These function can be used
 * instead of new_wav2mfcc() and get_back_trellis().  These functions enable
 * recognition as soon as an input triggers.  The 1st pass processing
 * will be done concurrently with the input.
 *
 * The basic recognition procedure of Julius in main_recognition_loop()
 * is as follows:
 *
 *  -# speech input: (adin_go())  ... buffer `speech' holds the input
 *  -# feature extraction: (new_wav2mfcc()) ... compute feature vector
 *     from `speech' and store the vector sequence to `param'.
 *  -# recognition 1st pass: (get_back_trellis()) ... frame-wise beam decoding
 *     to generate word trellis index from `param' and models.
 *  -# recognition 2nd pass: (wchmm_fbs())
 *  -# Output result.
 *
 * At on-the-fly decoding, procedures from 1 to 3 above will be performed
 * in parallel.  It is implemented by a simple scheme, processing the captured
 * small speech fragments one by one progressively:
 *
 *  - Define a callback function that can do feature extraction and 1st pass
 *    processing progressively.
 *  - The callback will be given to A/D-in function adin_go().
 *
 * Actual procedure is as follows. The function RealTimePipeLine()
 * will be given to adin_go() as callback.  Then adin_go() will watch
 * the input, and if speech input starts, it calls RealTimePipeLine()
 * for every captured input fragments.  RealTimePipeLine() will
 * compute the feature vector of the given fragment and proceed the
 * 1st pass processing for them, and return to the capture function.
 * The current status will be hold to the next call, to perform
 * inter-frame processing (computing delta coef. etc.).
 *
 * Note about CMN: With acoustic models trained with CMN, Julius performs
 * CMN to the input.  On file input, the whole sentence mean will be computed
 * and subtracted.  At the on-the-fly decoding, the ceptral mean will be
 * performed using the cepstral mean of last 5 second input (excluding
 * rejected ones).  This was a behavier earlier than 3.5, and 3.5.1 now
 * applies MAP-CMN at on-the-fly decoding, using the last 5 second cepstrum
 * as initial mean.  Initial cepstral mean at start can be given by option
 * "-cmnload", and you can also prohibit the updates of initial cepstral
 * mean at each input by "-cmnnoupdate".  The last option is useful to always
 * use static global cepstral mean as initial mean for each input.
 *
 * The primary functions in this file are:
 *  - RealTimeInit() - initialization at application startup
 *  - RealTimePipeLinePrepare() - initialization before each input
 *  - RealTimePipeLine() - callback for on-the-fly 1st pass decoding
 *  - RealTimeResume() - recognition resume procedure for short-pause segmentation.
 *  - RealTimeParam() - finalize the on-the-fly 1st pass when input ends.
 *  - RealTimeCMNUpdate() - update CMN data for next input
 * 
 * </EN>
 * 
 * $Revision: 1.12 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

#undef RDEBUG			///< Define if you want local debug message

/* input parameter */
static HTK_Param *param = NULL;	///< Computed MFCC parameter vectors 
static float *bf;		///< Work space for FFT
static DeltaBuf *db;		///< Work space for delta MFCC cycle buffer
static DeltaBuf *ab;		///< Work space for accel MFCC cycle buffer
static VECT *tmpmfcc;		///< Work space to hold temporarl MFCC vector
static int maxframelen;		///< Maximum allowed input frame length
static int last_time;		///< Last processed frame

static boolean last_is_segmented; ///<  TRUE if last pass was a segmented input
#ifdef SP_BREAK_CURRENT_FRAME
static SP16 *rest_Speech = NULL; ///< Speech samples left unprocessed by segmentation at previous segment
static int rest_alloc_len = 0;	///< Allocated length of rest_Speech
static int rest_len;		///< Current stored length of rest_Speech
#endif

static int f_raw;		///< Frame pointer of current base MFCC
static int f;			///< Frame pointer where all MFCC computation has been done
static SP16 *window;		///< Window buffer for MFCC calculation
static int windowlen;		///< Buffer length of @a window
static int windownum;		///< Currently left samples in @a window

/* 計算結果の MFCC を保存する param 構造体を準備する
   これは1回の認識ごとに繰り返し呼ばれる */
/* prepare new parameter vector holder for RealTime*
   This will be called each time a recognition begins */
/** 
 * <JA>
 * 漸次的に計算される特徴ベクトル列を保持するための領域を準備する．
 * これは入力/認識1回ごとに繰り返し呼ばれる．
 * </JA>
 * <EN>
 * Prepare parameter vector holder to incrementally store the
 * calculated MFCC vectors.  This function will be called each time
 * after a recognition ends and new input begins.
 * </EN>
 */
static void
init_param()
{
  /* param 構造体を新たにアロケート */
  /* allocate parameter data */
  param = new_param();
  /* これから計算されるパラメータの型をヘッダに設定 */
  /* set header types */
  param->header.samptype = F_MFCC;
  if (para.delta) param->header.samptype |= F_DELTA;
  if (para.acc) param->header.samptype |= F_ACCL;
  if (para.energy) param->header.samptype |= F_ENERGY;
  if (para.c0) param->header.samptype |= F_ZEROTH;
  if (para.absesup) param->header.samptype |= F_ENERGY_SUP;
  if (para.cmn) param->header.samptype |= F_CEPNORM;

  param->header.wshift = para.smp_period * para.frameshift;
  param->header.sampsize = para.veclen * sizeof(VECT); /* not compressed */
  param->veclen = para.veclen;
  /* フレームごとのパラメータベクトル保存のためのヘッダ領域を確保 */
  /* 最大時間分を固定でアロケート */
  /* assign max (safe with free_param)*/
  param->parvec = (VECT **)mymalloc(sizeof(VECT *) * maxframelen);
  /* 認識処理中/終了後にセットされる変数:
     param->parvec (パラメータベクトル系列)
     param->header.samplenum, param->samplenum (全フレーム数)
  */
  /* variables that will be set while/after computation has been done:
     param->parvec (parameter vector sequence)
     param->header.samplenum, param->samplenum (total number of frames)
  */
}

/** 
 * <JA>
 * 第1パス平行認識処理の初期化（起動後1回だけ呼ばれる）
 * </JA>
 * <EN>
 * Initializations for on-the-fly 1st pass decoding (will be called once
 * on startup)
 * </EN>
 */
void
RealTimeInit()
{
  /* -ssload 指定時, SS用のノイズスペクトルをファイルから読み込む */
  /* if "-ssload", load noise spectrum for spectral subtraction from file */
  if (ssload_filename && ssbuf == NULL) {
    if ((ssbuf = new_SS_load_from_file(ssload_filename, &sslen)) == NULL) {
      j_error("Error: failed to read \"%s\"\n", ssload_filename);
    }
  }
  /* MFCC計算関数を初期化 */
  /* initialize MFCC computation functions */
  WMP_init(para, &bf, ssbuf, sslen);
  /* 対数エネルギー正規化のための初期値 */
  /* set initial value for log energy normalization */
  if (para.energy && para.enormal) energy_max_init();
  /* デルタ計算のためのサイクルバッファを用意 */
  /* initialize cycle buffers for delta and accel coef. computation */
  if (para.delta) db = WMP_deltabuf_new(para.baselen, para.delWin);
  if (para.acc) ab = WMP_deltabuf_new(para.baselen * 2, para.accWin);
  /* デルタ計算のためのワークエリアを確保 */
  /* allocate work area for the delta computation */
  tmpmfcc = (VECT *)mymalloc(sizeof(VECT) * para.vecbuflen);
  /* 最大フレーム長を最大入力時間数から計算 */
  /* set maximum allowed frame length */
  maxframelen = MAXSPEECHLEN / para.frameshift;
  /* 窓長をセット */
  /* set window length */
  windowlen = para.framesize + 1;
  /* 窓かけ用バッファを確保 */
  /* set window buffer */
  window = mymalloc(sizeof(SP16) * windowlen);
  /* MAP-CMN 用の初期ケプストラム平均を読み込んで初期化する */
  /* Initialize the initial cepstral mean data from file for MAP-CMN */
  if (para.cmn) CMN_realtime_init(para.mfcc_dim, cmn_map_weight);
  /* -cmnload 指定時, CMN用のケプストラム平均の初期値をファイルから読み込む */
  /* if "-cmnload", load initial cepstral mean data from file for CMN */
  if (cmnload_filename) {
    if (para.cmn) {
      if ((cmn_loaded = CMN_load_from_file(cmnload_filename, para.mfcc_dim))== FALSE) {
	j_printf("Warning: failed to read cepstral mean from \"%s\"\n", cmnload_filename);
      }
    } else {
      j_printf("Warning: CMN not required, file \"%s\" ignored\n", cmnload_filename);
    }
  }
}

/* ON-THE-FLY デコーディング関数: 準備 (認識開始時ごとに呼ばれる) */
/* ON-THE-FLY DECODING FUNCTION: prepare (on start of every input segment) */
/** 
 * <JA>
 * 第1パス平行認識処理のデータ準備（認識開始ごとに呼ばれる）
 * </JA>
 * <EN>
 * Data preparation for on-the-fly 1st pass decoding (will be called on the
 * start of each sentence input)
 * </EN>
 */
void
RealTimePipeLinePrepare()
{
  /* 計算結果の MFCC を保存する param 構造体を準備する */
  /* prepare param to hold the resulting MFCC parameters */
  init_param();
  /* 対数エネルギー正規化のための初期値をセット */
  /* set initial value for log energy normalization */
  if (para.energy && para.enormal) energy_max_prepare(&para);
  /* デルタ計算用バッファを準備 */
  /* set the delta cycle buffer */
  if (para.delta) WMP_deltabuf_prepare(db);
  if (para.acc) WMP_deltabuf_prepare(ab);
  /* 音響尤度計算用キャッシュを準備
     最大長をここであらかじめ確保してしまう */
  /* prepare cache area for acoustic computation of HMM states and mixtures
     pre-fetch for maximum length here */
  outprob_prepare(maxframelen);
  /* 準備した param 構造体のデータのパラメータ型を音響モデルとチェックする */
  /* check type coherence between param and hmminfo here */
  if (!check_param_coherence(hmminfo, param)) {
    j_error("parameter type not match?\n");
  }
  /* 計算用の変数を初期化 */
  /* initialize variables for computation */
  f_raw = 0;
  f = 0;
  windownum = 0;

  /* MAP-CMN の初期化 */
  /* Prepare for MAP-CMN */
  if (para.cmn) CMN_realtime_prepare();

  /* -record 指定時, 音声データを保存するファイルをここで開く */
  /* if "-record", open the file to write the incoming speech data here */
  if (record_dirname != NULL) {
    record_sample_open();
  }
  
#ifdef VISUALIZE
  /* 音声波形の画面出力用に, 音声データを別に保存する．
     ここではポインタを初期化するのみ   */
  /* record data for waveform printing */
  speechlen = 0;
#endif
}

/** 
 * <JA>
 * @brief  第1パス平行音声認識処理のメイン
 *
 * この関数は, 音声入力ルーチンのコールバックとして，取り込んだ
 * 音声データの断片を引数として呼び出されます．音声入力が開始されると，
 * 取り込んだ音声データは数千サンプルごとに，その都度この関数が呼び出されます．
 * 呼び出しは，音声区間終了か入力ストリームの終了まで続きます．
 * 
 * この関数内では，漸次的な特徴量抽出および第1パスの認識が行われます．
 * 
 * @param Speech [in] 音声データへのバッファへのポインタ
 * @param nowlen [in] 音声データの長さ
 * 
 * @return エラー時 -1 を，正常時 0 を返す．また，この入力断片までで，
 * 文章の区切りとして第1パスを終了したいときには 1 を返す．
 * </JA>
 * <EN>
 * @brief  Main function of on-the-fly 1st pass decoding
 *
 * This function will be called each time a new speech sample comes, as
 * as callback from A/D-in routine.  When a speech input begins, the captured
 * speech will be passed to this function for every sample segments.  This
 * process will continue until A/D-in routine detects an end of speech or
 * input stream reached to an end.
 *
 * This function will perform feture vector extraction and beam decording as
 * 1st pass recognition simultaneously, in frame-wise mannar.
 * 
 * @param Speech [in] pointer to the speech sample segments
 * @param nowlen [in] length of above
 * 
 * @return -1 on error (tell caller to terminate), 0 on success (allow caller
 * to call me for the next segment), or 1 when an input segmentation is
 * required at this point (in that case caller will stop input and go to
 * 2nd pass)
 * </EN>
 */
int
RealTimePipeLine(SP16 *Speech, int nowlen) /* Speech[0...nowlen] = input */
{
  int i, now;
  boolean ret;
  LOGPROB f;

  /* window[0..windownum-1] は前回の呼び出しで残った音声データが格納されている */
  /* window[0..windownum-1] are speech data left from previous call */

  /* 処理用ポインタを初期化 */
  /* initialize pointer for local processing */
  now = 0;
  
  /* 認識処理がセグメント要求で終わったのかどうかのフラグをリセット */
  /* reset flag which indicates whether the input has ended with segmentation request */
  last_is_segmented = FALSE;

  /* 初めて呼び出されたときに, 認識開始のメッセージを出力 */
  /* If this is the first call, output start recording/processing message */
  if (f_raw == 0) status_recstart();

#ifdef RDEBUG
  printf("got %d samples\n", nowlen);
#endif

  /* -record 時, 音声データをファイルに追加する */
  /* if "-record", append the incoming samples to a file */
  if (record_dirname != NULL) {
    record_sample_write(Speech, nowlen);
  }

#ifdef VISUALIZE
  /* record data for waveform printing */
  adin_cut_callback_store_buffer(Speech, nowlen);
#endif

  while (now < nowlen) {	/* till whole input is processed */
    /* 入力長が maxframelen に達したらここで強制終了 */
    /* if input length reaches maximum buffer size, terminate 1st pass here */
    if (f_raw >= maxframelen) return(1);
    /* 窓バッファを埋められるだけ埋める */
    /* fill window buffer as many as possible */
    for(i = min(windowlen - windownum, nowlen - now); i > 0 ; i--)
      window[windownum++] = (float) Speech[now++];
    /* もし窓バッファが埋まらなければ, このセグメントの処理はここで終わる．
       処理されなかったサンプル (window[0..windownum-1]) は次回に持ち越し．*/
    /* if window buffer was not filled, end processing here, keeping the
       rest samples (window[0..windownum-1]) in the window buffer. */
    if (windownum < windowlen) break;
#ifdef RDEBUG
    /*    printf("%d used, %d rest\n", now, nowlen - now);

	  printf("[f_raw = %d, f = %d]\n", f_raw, f);*/
#endif

    /* 音声波形から base MFCC を計算 */
    /* calculate base MFCC from waveform */
    for (i=0; i < windowlen; i++) {
      bf[i+1] = (float) window[i];
    }
    WMP_calc(tmpmfcc, bf, para, ssbuf);

    if (para.energy && para.enormal) {
      /* 対数エネルギー項を正規化する */
      /* normalize log energy */
      /* リアルタイム入力では発話ごとの最大エネルギーが得られないので
	 直前の発話のパワーで代用する */
      /* Since the maximum power of the whole input utterance cannot be
	 obtained at real-time input, the maximum of last input will be
	 used to normalize.
      */
      tmpmfcc[para.baselen-1] = energy_max_normalize(tmpmfcc[para.baselen-1], &para);
    }

    if (para.delta) {
      /* デルタを計算する */
      /* calc delta coefficients */
      ret = WMP_deltabuf_proceed(db, tmpmfcc);
#ifdef RDEBUG
      printf("DeltaBuf: ret=%d, status=", ret);
      for(i=0;i<db->len;i++) {
	printf("%d", db->is_on[i]);
      }
      printf(", nextstore=%d\n", db->store);
#endif
      /* ret == FALSE のときはまだディレイ中なので認識処理せず次入力へ */
      /* if ret == FALSE, there is no available frame.  So just wait for
	 next input */
      if (! ret) {
	goto next_input;
      }
      /* db->vec に現在の元データとデルタ係数が入っているので tmpmfcc にコピー */
      /* now db->vec holds the current base and full delta, so copy them to tmpmfcc */
      memcpy(tmpmfcc, db->vec, sizeof(VECT) * para.baselen * 2);
    }

    if (para.acc) {
      /* Accelerationを計算する */
      /* calc acceleration coefficients */
      /* base+delta をそのまま入れる */
      /* send the whole base+delta to the cycle buffer */
      ret = WMP_deltabuf_proceed(ab, tmpmfcc);
#ifdef RDEBUG
      printf("AccelBuf: ret=%d, status=", ret);
      for(i=0;i<ab->len;i++) {
	printf("%d", ab->is_on[i]);
      }
      printf(", nextstore=%d\n", ab->store);
#endif
      /* ret == FALSE のときはまだディレイ中なので認識処理せず次入力へ */
      /* if ret == FALSE, there is no available frame.  So just wait for
	 next input */
      if (! ret) {
	goto next_input;
      }
      /* ab->vec には，(base+delta) とその差分係数が入っている．
	 [base] [delta] [delta] [acc] の順で入っているので,
	 [base] [delta] [acc] を tmpmfcc にコピーする．*/
      /* now ab->vec holds the current (base+delta) and their delta coef. 
	 it holds a vector in the order of [base] [delta] [delta] [acc], 
	 so copy the [base], [delta] and [acc] to tmpmfcc.  */
      memcpy(tmpmfcc, ab->vec, sizeof(VECT) * para.baselen * 2);
      memcpy(&(tmpmfcc[para.baselen*2]), &(ab->vec[para.baselen*3]), sizeof(VECT) * para.baselen);
    }

    if (para.delta && (para.energy || para.c0) && para.absesup) {
      /* 絶対値パワーを除去 */
      /* suppress absolute power */
      memmove(&(tmpmfcc[para.baselen-1]), &(tmpmfcc[para.baselen]), sizeof(VECT) * (para.vecbuflen - para.baselen));
    }

    /* この時点で tmpmfcc に現時点での最新の特徴ベクトルが格納されている */
    /* tmpmfcc[] now holds the latest parameter vector */

#ifdef RDEBUG
      printf("DeltaBuf: got frame %d\n", f_raw);
#endif
      /* CMN を計算 */
      /* perform CMN */
      if (para.cmn) CMN_realtime(tmpmfcc, para.mfcc_dim);

      /* MFCC完成，登録 */
      /* now get the MFCC vector of current frame, now store it to param */
      param->parvec[f_raw] = (VECT *)mymalloc(sizeof(VECT) * param->veclen);
      memcpy(param->parvec[f_raw], tmpmfcc, sizeof(VECT) * param->veclen);
      f = f_raw;

      /* ここでフレーム "f" に最新のMFCCが保存されたことになる */
      /* now we got the most recent MFCC parameter for frame 'f' */
      /* この "f" のフレームについて認識処理(フレーム同期ビーム探索)を進める */
      /* proceed beam search for this frame [f] */
      if (f == 0) {
	/* 最初のフレーム: 探索処理を初期化 */
	/* initial frame: initialize search process */
	get_back_trellis_init(param, wchmm, &backtrellis);
      }
#ifndef MULTIPATH_VERSION
      if (f != 0) {
#endif
	/* 1フレーム探索を進める */
	/* proceed search for 1 frame */
	if (get_back_trellis_proceed(f, param, wchmm, &backtrellis
#ifdef MULTIPATH_VERSION
				     ,FALSE
#endif
				     ) == FALSE) {
	  /* 探索処理の終了が発生したのでここで認識を終える．
	     最初のフレームから [f-1] 番目までが認識されたことになる
	  */
	  /* the recognition process tells us to stop recognition, so
	     recognition should be terminated here.
	     the recognized data are [0..f-1] */
	  /* 認識処理は f-1 で終わったが, MFCCの計算は f まで，初期のMFCC計算
	     は f_raw まで進んでいることに注意: 後で再開処理が必要*/
	  /* notice: recognition process has done for "f-1" frame, but MFCC
	     computation has already done for "f", and first 12-dim. MFCC
	     has been alsp computed till "f_raw".  Some restarting operation
	     is needed later */

	  /* 認識処理のセグメント要求で終わったことをフラグにセット */
	  /* set flag which indicates that the input has ended with segmentation request */
	  last_is_segmented = TRUE;
	  /* 最終フレームを last_time にセット */
	  /* set the last frame to last_time */
	  last_time = f-1;
#ifdef SP_BREAK_CURRENT_FRAME
	  /* ショートポーズセグメンテーション: バッファに残っているデータを
	     別に保持して，次回の最初に処理する */
	  /* short pause segmentation: there is some data left in buffer, so
	   we should keep them for next processing */
	  param->header.samplenum = f_raw+1;/* len = lastid + 1 */
	  param->samplenum = f_raw+1;
	  rest_len = nowlen - now;
	  if (rest_len > 0) {
	    /* copy rest samples to rest_Speech */
	    if (rest_Speech == NULL) {
	      rest_alloc_len = rest_len;
	      rest_Speech = (SP16 *)mymalloc(sizeof(SP16)*rest_alloc_len);
	    } else if (rest_alloc_len < rest_len) {
	      rest_alloc_len = rest_len;
	      rest_Speech = (SP16 *)myrealloc(rest_Speech, sizeof(SP16)*rest_alloc_len);
	    }
	    memcpy(rest_Speech, &(Speech[now]), sizeof(SP16) * rest_len);
	  }
#else
	  /* param に格納されたフレーム長をセット */
	  /* set number of frames to param */
	  param->header.samplenum = f;
	  param->samplenum = f;
#endif
	  /* tell the caller to be segmented by this function */
	  /* 呼び出し元に，ここで入力を切るよう伝える */
	  return(1);
	}
#ifndef MULTIPATH_VERSION
      }
#endif
      /* 1フレーム処理が進んだのでポインタを進める */
      /* proceed frame pointer */
      f_raw++;

  next_input:

    /* 窓バッファを処理が終わった分シフト */
    /* shift window */
    memmove(window, &(window[para.frameshift]), sizeof(SP16) * (windowlen - para.frameshift));
    windownum -= para.frameshift;
  }

  /* 与えられた音声セグメントに対する認識処理が全て終了
     呼び出し元に, 入力を続けるよう伝える */
  /* input segment is fully processed
     tell the caller to continue input */
  return(0);			
}

#ifdef SP_BREAK_CURRENT_FRAME
/** 
 * <JA>
 * ショートポーズセグメンテーションの再開処理:
 * 入力の認識開始の前に,前回のオーバーラップ分と残りを処理する．
 *
 * ショートポーズセグメンテーションでは, 前区間での末尾の sp に対応した
 * 区間分を遡って認識を再開する．この前区間の末尾の sp 区間のMFCCパラメー
 * タが rest_param に入っているので, まずはそこから認識処理を開始する．
 * 次に，前回の認識終了時に残った未処理の音声データが rest_Speech に
 * あるので，続けてそれらの認識処理を行う．
 * この処理のあと,通常の RealTimePipeLine() が呼び出される．
 * 
 * @return エラー時 -1，正常時 0 を返す．また，この入力断片の処理中に
 * 文章の区切りが見つかったときは第1パスをここで中断するために 1 を返す．
 * </JA>
 * </JA>
 * <EN>
 * Resuming recognition for short pause segmentation:
 * process the overlapped data and remaining speech prior to the next input.
 *
 * When short-pause segmentation is enabled, we restart(resume)
 * the recognition process from the beginning of last short-pause segment
 * at the last input.  The corresponding parameters are in "rest_param",
 * so we should first process the parameters.  Further, we have remaining
 * speech data that has not been converted to MFCC at the last input in
 * "rest_Speech[]", so next we should process the rest speech.
 * After this funtion, the usual RealTimePileLine() can be called for the
 * next incoming new speeches.
 * 
 * @return -1 on error (tell caller to terminate), 0 on success (allow caller
 * to call me for the next segment), or 1 when an end-of-sentence detected
 * at this point (in that case caller will stop input and go to 2nd pass)
 * </EN>
 */
int
RealTimeResume()
{
  int t;

  /* rest_param に最後のsp区間のパラメータが入っている */
  /* rest_param holds the last MFCC parameters */
  param = rest_param;

  /* paramを準備 */
  /* prepare param by expanding the last input param */
  outprob_prepare(maxframelen);
  param->parvec = (VECT **)myrealloc(param->parvec, sizeof(VECT *) * maxframelen);
  /* param にある全パラメータを処理する: f_raw をあらかじめセット */
  /* process all data in param: pre-set the resulting f_raw */
  f_raw = param->samplenum - 1;

  /* param 内の全フレームについて認識処理を進める */
  /* proceed recognition for all frames in param */
  if (f_raw >= 0) {
    f = f_raw;
#ifdef RDEBUG
    printf("Resume: f=%d,f_raw=%d\n", f, f_raw);
#endif
    get_back_trellis_init(param, wchmm, &backtrellis);
    for (t=
#ifdef MULTIPATH_VERSION
	   0
#else
	   1
#endif
	   ;t<=f;t++) {
      if (get_back_trellis_proceed(t, param, wchmm, &backtrellis
#ifdef MULTIPATH_VERSION
				   ,FALSE
#endif
				   ) == FALSE) {
	/* segmented, end procs ([0..f])*/
	last_is_segmented = TRUE;
	last_time = t-1;
	return(1);		/* segmented by this function */
      }
    }
  }

  f_raw++;
  f = f_raw;

  /* シフトしておく */
  /* do last shift */
  memmove(window, &(window[para.frameshift]), sizeof(SP16) * (windowlen - para.frameshift));
  windownum -= para.frameshift;

  /* これで再開の準備が整ったので,まずは前回の処理で残っていた音声データから
     処理する */
  /* now that the search status has been prepared for the next input, we
     first process the rest unprocessed samples at the last session */
  if (rest_len > 0) {
#ifdef RDEBUG
    printf("Resume: rest %d samples\n", rest_len);
#endif
    return(RealTimePipeLine(rest_Speech, rest_len));
  }

  /* 新規の入力に対して認識処理は続く… */
  /* the recognition process will continue for the newly incoming samples... */
  return 0;
}
#endif /* SP_BREAK_CURRENT_FRAME */


/* ON-THE-FLY デコーディング関数: 終了処理 */
/* ON-THE-FLY DECODING FUNCTION: end processing */
/** 
 * <JA>
 * 第1パス平行認識処理の終了処理を行う．
 * 
 * @param backmax [out] 第1パスの最終フレームでの最大尤度を格納する
 * 
 * @return この入力の特徴パラメータを格納した構造体を返す．
 * </JA>
 * <EN>
 * Finalize the 1st pass on-the-fly decoding.
 * 
 * @param backmax [out] pointer to store the maximum score of last frame.
 * 
 * @return newly allocated input parameter data for this input.
 * </EN>
 */
HTK_Param *
RealTimeParam(LOGPROB *backmax)
{
  boolean ret1, ret2;

  if (last_is_segmented) {
    /* RealTimePipeLine で認識処理側の理由により認識が中断した場合,
       現状態のMFCC計算データをそのまま次回へ保持する必要があるので,
       MFCC計算終了処理を行わずに第１パスの結果のみ出力して終わる．*/
    /* When input segmented by recognition process in RealTimePipeLine(),
       we have to keep the whole current status of MFCC computation to the
       next call.  So here we only output the 1st pass result. */
    *backmax = finalize_1st_pass(&backtrellis, winfo, last_time);
#ifdef SP_BREAK_CURRENT_FRAME
    finalize_segment(&backtrellis, param, last_time);
#endif
    /* この区間の param データを第２パスのために返す */
    /* return obtained parameter for 2nd pass */
    return(param);
  }

  /* MFCC計算の終了処理を行う: 最後の遅延フレーム分を処理 */
  /* finish MFCC computation for the last delayed frames */

  if (para.delta || para.acc) {

    /* look until all data has been flushed */
    while(f_raw < maxframelen) {

      /* check if there is data in cycle buffer of delta */
      ret1 = WMP_deltabuf_flush(db);
#ifdef RDEBUG
      {
	int i;
	printf("DeltaBufLast: ret=%d, status=", ret1);
	for(i=0;i<db->len;i++) {
	  printf("%d", db->is_on[i]);
	}
	printf(", nextstore=%d\n", db->store);
      }
#endif
      if (ret1) {
	/* uncomputed delta has flushed, compute it with tmpmfcc */
	if (para.energy && para.absesup) {
	  memcpy(tmpmfcc, db->vec, sizeof(VECT) * (para.baselen - 1));
	  memcpy(&(tmpmfcc[para.baselen-1]), &(db->vec[para.baselen]), sizeof(VECT) * para.baselen);
	} else {
	  memcpy(tmpmfcc, db->vec, sizeof(VECT) * para.baselen * 2);
	}
	if (para.acc) {
	  /* this new delta should be given to the accel cycle buffer */
	  ret2 = WMP_deltabuf_proceed(ab, tmpmfcc);
#ifdef RDEBUG
	  printf("AccelBuf: ret=%d, status=", ret2);
	  for(i=0;i<ab->len;i++) {
	    printf("%d", ab->is_on[i]);
	  }
	  printf(", nextstore=%d\n", ab->store);
#endif
	  if (ret2) {
	    /* uncomputed accel was given, compute it with tmpmfcc */
	    memcpy(tmpmfcc, ab->vec, sizeof(VECT) * (para.veclen - para.baselen));
	    memcpy(&(tmpmfcc[para.veclen - para.baselen]), &(ab->vec[para.veclen - para.baselen]), sizeof(VECT) * para.baselen);
	  } else {
	    /* no input is still available: */
	    /* in case of very short input: go on to the next input */
	    continue;
	  }
	}
      } else {
	/* no data left in the delta buffer */
	if (para.acc) {
	  /* no new data, just flush the accel buffer */
	  ret2 = WMP_deltabuf_flush(ab);
#ifdef RDEBUG
	  printf("AccelBuf: ret=%d, status=", ret2);
	  for(i=0;i<ab->len;i++) {
	    printf("%d", ab->is_on[i]);
	  }
	  printf(", nextstore=%d\n", ab->store);
#endif
	  if (ret2) {
	    /* uncomputed data has flushed, compute it with tmpmfcc */
	    memcpy(tmpmfcc, ab->vec, sizeof(VECT) * (para.veclen - para.baselen));
	    memcpy(&(tmpmfcc[para.veclen - para.baselen]), &(ab->vec[para.veclen - para.baselen]), sizeof(VECT) * para.baselen);
	  } else {
	    /* actually no data exists in both delta and accel */
	    break;		/* end this loop */
	  }
	} else {
	  /* only delta: input fully flushed, end this loop */
	  break;
	}
      }
      if(para.cmn) CMN_realtime(tmpmfcc, para.mfcc_dim);
      param->parvec[f_raw] = (VECT *)mymalloc(sizeof(VECT) * param->veclen);
      memcpy(param->parvec[f_raw], tmpmfcc, sizeof(VECT) * param->veclen);
      f = f_raw;
      if (f == 0) {
	get_back_trellis_init(param, wchmm, &backtrellis);
      }
#ifdef MULTIPATH_VERSION
      get_back_trellis_proceed(f, param, wchmm, &backtrellis, FALSE);
#else
      if (f != 0) {
	get_back_trellis_proceed(f, param, wchmm, &backtrellis);
      }
#endif
      f_raw++;
    }
  }

  /* フレーム長をセット */
  /* set frame length */
  param->header.samplenum = f_raw;
  param->samplenum = f_raw;

  /* 入力長がデルタの計算に十分でない場合,
     MFCC が CMN まで正しく計算できないため，エラー終了とする．*/
  /* if input is short for compute all the delta coeff., terminate here */
  if (f_raw == 0) {
    j_printf("Error: too short input to compute delta coef! (%d frames)\n", f_raw);
    *backmax = finalize_1st_pass(&backtrellis, winfo, param->samplenum);
  } else {
    /* 第１パスの終了処理を行う */
    /* finalize 1st pass */
    get_back_trellis_end(param, wchmm, &backtrellis);
    *backmax = finalize_1st_pass(&backtrellis, winfo, param->samplenum);
#ifdef SP_BREAK_CURRENT_FRAME
    finalize_segment(&backtrellis, param, param->samplenum);
#endif
  }

  /* この区間の param データを第２パスのために返す */
  /* return obtained parameter for 2nd pass */
  return(param);
}

/** 
 * <JA>
 * 次回の認識に備えて CMN用にケプストラム平均を更新する．
 * 
 * @param param [in] 現在の入力パラメータ
 * </JA>
 * <EN>
 * Update cepstral mean of CMN to prepare for the next input.
 * 
 * @param param [in] current input parameter
 * </EN>
 */
void
RealTimeCMNUpdate(HTK_Param *param)
{
  float mseclen;
  boolean cmn_update_p;
  
  /* update CMN vector for next speech */
  if(para.cmn) {
    if (cmn_update) {
      cmn_update_p = TRUE;
      if (rejectshortlen > 0) {
	/* not update if rejected by short input */
	mseclen = (float)param->samplenum * (float)para.smp_period * (float)para.frameshift / 10000.0;
	if (mseclen < rejectshortlen) {
	  cmn_update_p = FALSE;
	}
      }
      if (gmm_reject_cmn_string != NULL) {
	/* if using gmm, try avoiding update of CMN for noise input */
	if(! gmm_valid_input()) {
	  cmn_update_p = FALSE;
	}
      }
      if (cmn_update_p) {
	/* update last CMN parameter for next spech */
	CMN_realtime_update();
      } else {
	/* do not update, because the last input is bogus */
	j_printf("CMN not updated\n");
      }
    }
    /* if needed, save the updated CMN parameter to a file */
    if (cmnsave_filename) {
      if (CMN_save_to_file(cmnsave_filename) == FALSE) {
	j_printf("Warning: failed to save cmn data to \"%s\"\n", cmnsave_filename);
      }
    }
  }
}

/** 
 * <JA>
 * 第1パス平行認識処理の中断時の終了処理を行う．
 * </JA>
 * <EN>
 * Finalize the 1st pass on-the-fly decoding when terminated.
 * </EN>
 */
void
RealTimeTerminate()
{
  param->header.samplenum = f_raw;
  param->samplenum = f_raw;

  /* 入力長がデルタの計算に十分でない場合,
     MFCC が CMN まで正しく計算できないため，エラー終了とする．*/
  /* if input is short for compute all the delta coeff., terminate here */
  if (f_raw == 0) {
    finalize_1st_pass(&backtrellis, winfo, param->samplenum);
  } else {
    /* 第１パスの終了処理を行う */
    /* finalize 1st pass */
    status_recend();
    get_back_trellis_end(param, wchmm, &backtrellis);
    finalize_1st_pass(&backtrellis, winfo, param->samplenum);
#ifdef SP_BREAK_CURRENT_FRAME
    finalize_segment(&backtrellis, param, param->samplenum);
#endif
  }
  /* パラメータを解放 */
  /* free parameter */
  free_param(param);
}
