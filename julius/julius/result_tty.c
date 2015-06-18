/**
 * @file   result_tty.c
 * @author Akinobu Lee
 * @date   Tue Sep 06 17:18:46 2005
 * 
 * <JA>
 * @brief  認識結果を標準出力へ出力する．
 * </JA>
 * 
 * <EN>
 * @brief  Output recoginition result to standard output
 * </EN>
 * 
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

/**********************************************************************/
/* process online/offline status  */

/** 
 * <JA>
 * 認識可能な状態になったときに呼ばれる
 * 
 * </JA>
 * <EN>
 * Called when it becomes ready to recognize the input.
 * 
 * </EN>
 */
static void
ttyout_status_process_online()
{
  /* no output */
}
/** 
 * <JA>
 * 認識を一時中断状態になったときに呼ばれる
 * 
 * </JA>
 * <EN>
 * Called when process paused and recognition is stopped.
 * 
 * </EN>
 */
static void
ttyout_status_process_offline()
{
  /* no output */
}

/**********************************************************************/
/* 1st pass output */

static int wst;			///< Number of words at previous output line

/** 
 * <JA>
 * 第1パス：音声認識を開始する際の出力（音声入力開始時に呼ばれる）．
 * 
 * </JA>
 * <EN>
 * 1st pass: output when recognition begins (will be called at input start).
 * 
 * </EN>
 */
static void
ttyout_pass1_begin()
{
  wst = 0;
  /* moved from adin-cut.c */
  j_printerr("\r                    \r");
}


/** 
 * <JA>
 * 第1パス：途中結果を出力する（第1パスの一定時間ごとに呼ばれる）
 * 
 * @param t [in] 現在の時間フレーム
 * @param seq [in] 現在の一位候補単語列
 * @param num [in] @a seq の長さ
 * @param score [in] 上記のこれまでの累積スコア
 * @param LMscore [in] 上記の最後の単語の信頼度
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * 1st pass: output current result while search (called periodically while 1st pass).
 * 
 * @param t [in] current time frame
 * @param seq [in] current best word sequence at time @a t.
 * @param num [in] length of @a seq.
 * @param score [in] accumulated score of the current best sequence at @a t.
 * @param LMscore [in] confidence score of last word on the sequence
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
ttyout_pass1_current(int t, WORD_ID *seq, int num, LOGPROB score, LOGPROB LMscore, WORD_INFO *winfo)
{
  int i,bgn;
  int len;

  /* update output line with folding */
  j_printf("\r");

  len = 0;
  if (wst == 0) {		/* first line */
    len += 11;
    j_printf("pass1_best:");
  }
  
  bgn = wst;			/* output only the last line */
  for (i=bgn;i<num;i++) {
    len += strlen(winfo->woutput[seq[i]]) + 1;
    if (len > FILLWIDTH) {	/* fold line */
      wst = i;
      j_printf("\n");
      len = 0;
    }
    j_printf(" %s",winfo->woutput[seq[i]]);
  }
  
  j_flushprint();		/* flush */
}

/** 
 * <JA>
 * 第1パス：終了時に第1パスの結果を出力する（第1パス終了後、第2パスが
 * 始まる前に呼ばれる．認識に失敗した場合は呼ばれない）．
 * 
 * @param seq [in] 第1パスの1位候補の単語列
 * @param num [in] 上記の長さ
 * @param score [in] 1位の累積仮説スコア
 * @param LMscore [in] @a score のうち言語スコア
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * 1st pass: output final result of the 1st pass (will be called just after
 * the 1st pass ends and before the 2nd pass begins, and will not if search
 * failed).
 * 
 * @param seq [in] word sequence of the best hypothesis at the 1st pass.
 * @param num [in] length of @a seq.
 * @param score [in] accumulated hypothesis score of @a seq.
 * @param LMscore [in] language score in @a score.
 * @param winfo [in] word dictionary.
 * </EN>
 */
static void
ttyout_pass1_final(WORD_ID *seq, int num, LOGPROB score, LOGPROB LMscore, WORD_INFO *winfo)
{
  int i,j;
  static char buf[MAX_HMMNAME_LEN];

  /* words */
  j_printf("\n");
  j_printf("pass1_best:");
  for (i=0;i<num;i++) {
    j_printf(" %s",winfo->woutput[seq[i]]);
  }
  j_printf("\n");

  if (verbose_flag) {		/* output further info */
    /* N-gram entries */
    j_printf("pass1_best_wordseq:");
    for (i=0;i<num;i++) {
      j_printf(" %s",winfo->wname[seq[i]]);
    }
    j_printf("\n");
    /* phoneme sequence */
    j_printf("pass1_best_phonemeseq:");
    for (i=0;i<num;i++) {
      for (j=0;j<winfo->wlen[seq[i]];j++) {
	center_name(winfo->wseq[seq[i]][j]->name, buf);
	j_printf(" %s", buf);
      }
      if (i < num-1) j_printf(" |");
    }
    j_printf("\n");
    if (debug2_flag) {
      /* logical HMMs */
      j_printf("pass1_best_HMMseq_logical:");
      for (i=0;i<num;i++) {
	for (j=0;j<winfo->wlen[seq[i]];j++) {
	  j_printf(" %s", winfo->wseq[seq[i]][j]->name);
	}
	if (i < num-1) j_printf(" |");
      }
      j_printf("\n");
    }
  }
  /* score */
  j_printf("pass1_best_score: %f", score);
#ifdef USE_NGRAM
  if (separate_score_flag) {
    j_printf(" (AM: %f  LM: %f)", score-LMscore, LMscore);
  }
#endif
  j_printf("\n");
}

/** 
 * <JA>
 * 第1パス：終了時の出力（第1パスの終了時に必ず呼ばれる）
 * 
 * </JA>
 * <EN>
 * 1st pass: end of output (will be called at the end of the 1st pass).
 * 
 * </EN>
 */
static void
ttyout_pass1_end()
{
  /* no op */
  j_printf("\n");
}

/**********************************************************************/
/* 2nd pass output */

/** 
 * <JA>
 * 仮説中の単語情報を出力する
 * 
 * @param hypo [in] 仮説
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output word sequence of a hypothesis.
 * 
 * @param hypo [in] sentence hypothesis
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_hypo_woutput(NODE *hypo, WORD_INFO *winfo)
{
  int i,w;

  if (hypo != NULL) {
    for (i=hypo->seqnum-1;i>=0;i--) {
      w = hypo->seq[i];
      j_printf(" %s",winfo->woutput[w]);
    }
  }
  j_printf("\n");  
}

/** 
 * <JA>
 * 仮説のN-gram情報（Julianではカテゴリ番号列）を出力する．
 * 
 * @param hypo [in] 文仮説
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output LM word sequence (N-gram entry/DFA category) of a hypothesis.
 * 
 * @param hypo [in] sentence hypothesis
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_hypo_wname(NODE *hypo, WORD_INFO *winfo)
{
  int i,w;

  if (hypo != NULL) {
    for (i=hypo->seqnum-1;i>=0;i--) {
      w = hypo->seq[i];
      j_printf(" %s",winfo->wname[w]);
    }
  }
  j_printf("\n");  
}

/** 
 * <JA>
 * 仮説の音素系列を出力する．
 * 
 * @param hypo [in] 文仮説
 * @param winfo [in] 単語情報
 * </JA>
 * <EN>
 * Output phoneme sequence of a hypothesis.
 * 
 * @param hypo [in] sentence hypothesis
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_hypo_phoneme(NODE *hypo, WORD_INFO *winfo)
{
  int i,j,w;
  static char buf[MAX_HMMNAME_LEN];

  if (hypo != NULL) {
    for (i=hypo->seqnum-1;i>=0;i--) {
      w = hypo->seq[i];
      for (j=0;j<winfo->wlen[w];j++) {
	center_name(winfo->wseq[w][j]->name, buf);
	j_printf(" %s", buf);
      }
      if (i > 0) j_printf(" |");
    }
  }
  j_printf("\n");  
}
#ifdef CONFIDENCE_MEASURE
/** 
 * <JA>
 * 仮説の単語ごとの信頼度を出力する．
 * 
 * @param hypo [in] 文仮説
 * </JA>
 * <EN>
 * Output confidence score of words in a sentence hypothesis.
 * 
 * @param hypo 
 * </EN>
 */
#ifdef CM_MULTIPLE_ALPHA
static void
put_hypo_cmscore(NODE *hypo, int id)
{
  int i;
  int j;
  
  if (hypo != NULL) {
    for (i=hypo->seqnum-1;i>=0;i--) {
      j_printf(" %5.3f", hypo->cmscore[i][id]);
    }
  }
  j_printf("\n");  
}
#else
static void
put_hypo_cmscore(NODE *hypo)
{
  int i;
  
  if (hypo != NULL) {
    for (i=hypo->seqnum-1;i>=0;i--) {
      j_printf(" %5.3f", hypo->cmscore[i]);
    }
  }
  j_printf("\n");
}
#endif
#endif /* CONFIDENCE_MEASURE */

/** 
 * <JA>
 * 第2パス：得られた文仮説候補を1つ出力する．
 * 
 * @param hypo [in] 得られた文仮説
 * @param rank [in] @a hypo の順位
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * 2nd pass: output a sentence hypothesis found in the 2nd pass.
 * 
 * @param hypo [in] sentence hypothesis to be output
 * @param rank [in] rank of @a hypo
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
ttyout_pass2(NODE *hypo, int rank, WORD_INFO *winfo)
{
  char ec[5] = {0x1b, '[', '1', 'm', 0};
		
  if (debug2_flag) {
    j_printf("\n%s",ec);		/* newline & bold on */
  }
  j_printf("sentence%d:",rank);
  put_hypo_woutput(hypo, winfo);
  if (verbose_flag) {
    j_printf("wseq%d:",rank);
    put_hypo_wname(hypo, winfo);
    j_printf("phseq%d:", rank);
    put_hypo_phoneme(hypo, winfo);
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
    {
      int i;
      for(i=0;i<cm_alpha_num;i++) {
	j_printf("cmscore%d[%f]:", rank, cm_alpha_bgn + i * cm_alpha_step);
	put_hypo_cmscore(hypo, i);
      }
    }
#else
    j_printf("cmscore%d:", rank);
    put_hypo_cmscore(hypo);
#endif
#endif /* CONFIDENCE_MEASURE */
  }
  if (debug2_flag) {
    ec[2] = '0';
    j_printf("%s\n",ec);		/* bold off & newline */
  }
  if (verbose_flag) {
    j_printf("score%d: %f",rank, (hypo != NULL) ? hypo->score : LOG_ZERO);
#ifdef USE_NGRAM
    if (separate_score_flag) {
      if (hypo == NULL) {
	j_printf(" (AM: %f  LM: %f)", LOG_ZERO, LOG_ZERO);
      } else {
	j_printf(" (AM: %f  LM: %f)", hypo->score - hypo->totallscore, hypo->totallscore);
      }
    }
#endif
    j_printf("\n");
#ifdef USE_DFA
    /* output which grammar the hypothesis belongs to on multiple grammar */
    /* determine only by the last word */
    if (multigram_get_all_num() > 1) {
      j_printf("grammar%d: %d\n", rank, multigram_get_gram_from_category(winfo->wton[hypo->seq[0]]));
    }
#endif
  }
  j_flushprint();
}

/** 
 * <JA>
 * 第2パス：音声認識結果の出力を開始する際の出力．認識結果を出力する際に、
 * 一番最初に出力される．
 * 
 * </JA>
 * <EN>
 * 2nd pass: output at the start of result output (will be called before
 * all the result output in the 2nd pass).
 * 
 * </EN>
 */
static void
ttyout_pass2_begin()
{
  /* no output */
}

/** 
 * <JA>
 * 第2パス：終了時
 * 
 * </JA>
 * <EN>
 * 2nd pass: end output
 * 
 * </EN>
 */
static void
ttyout_pass2_end()
{
#ifdef SP_BREAK_CURRENT_FRAME
  if (rest_param != NULL) {
    if (verbose_flag) {
      j_printf("Segmented by short pause, continue to next...\n");
    } else {
      j_printf("-->\n");
    }
  }
  j_flushprint();
#endif
}

#ifdef GRAPHOUT

/**********************************************************************/
/* word graph output */

#define TEXTWIDTH 70

/** 
 * <JA>
 * 得られた単語グラフ全体を出力する．
 * 
 * @param root [in] グラフ単語集合の先頭要素へのポインタ
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output the whole word graph.
 * 
 * @param root [in] pointer to the first element of graph words
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
ttyout_graph(WordGraph *root, WORD_INFO *winfo)
{
  WordGraph *wg;
  int tw1, tw2, i;

  j_printf("-------------------------- begin wordgraph show -------------------------\n");
  for(wg=root;wg;wg=wg->next) {
    tw1 = (TEXTWIDTH * wg->lefttime) / peseqlen;
    tw2 = (TEXTWIDTH * wg->righttime) / peseqlen;
    j_printf("%4d:", wg->id);
    for(i=0;i<tw1;i++) j_printf(" ");
    j_printf(" %s\n", wchmm->winfo->woutput[wg->wid]);
    j_printf("%4d:", wg->lefttime);
    for(i=0;i<tw1;i++) j_printf(" ");
    j_printf("|");
    for(i=tw1+1;i<tw2;i++) j_printf("-");
    j_printf("|\n");
  }
  j_printf("-------------------------- end wordgraph show ---------------------------\n");
}

#endif /* GRAPHOUT */

/**********************************************************************/
/* when search failed */

/** 
 * <JA>
 * 第2パスで探索に失敗したときの出力．
 * 
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output when search failed and no sentence candidate has been found.
 * 
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
ttyout_pass2_failed(WORD_INFO *winfo)
{
  j_printf("second pass failed, the first pass was:\n");
  ttyout_pass2((NODE *)NULL, 0, winfo); /* print NULL result */
}

/** 
 * <JA>
 * 入力が棄却されたときの出力．GMM や入力長で入力が棄却されたときに呼ばれる．
 * 
 * @param s [in] 理由をあらわす文字列
 * </JA>
 * <EN>
 * Output when input has been rejected and no recognition result is given.
 * This will be called when input was rejected by speech detection such as
 * GMM or input length.
 * 
 * @param s [in] string that describes the result of rejection
 * </EN>
 */
static void
ttyout_rejected(const char *s)
{
  j_printf("input rejected: %s\n", s);
}

/**********************************************************************/
/* output recording status changes */

/** 
 * <JA>
 * 準備が終了して、認識可能状態（入力待ち状態）に入ったときの出力
 * 
 * </JA>
 * <EN>
 * Output when ready to recognize and start waiting speech input.
 * 
 * </EN>
 */
void
ttyout_status_recready()
{
  if (speech_input == SP_MIC || speech_input == SP_NETAUDIO) {
    /* moved from adin-cut.c */
    j_printerr("<<< please speak >>>");
  }
}
/** 
 * <JA>
 * 入力の開始を検出したときの出力
 * 
 * </JA>
 * <EN>
 * Output when input starts.
 * 
 * </EN>
 */
void
ttyout_status_recstart()
{
  /* do nothing */
}
/** 
 * <JA>
 * 入力終了を検出したときの出力
 * 
 * </JA>
 * <EN>
 * Output when input ends.
 * 
 * </EN>
 */
void
ttyout_status_recend()
{
  /* do nothing */
}
/** 
 * <JA>
 * 入力長などの入力パラメータ情報を出力．
 * 
 * @param param [in] 入力パラメータ構造体
 * </JA>
 * <EN>
 * Output input parameter status such as length.
 * 
 * @param param [in] input parameter structure
 * </EN>
 */
void
ttyout_status_param(HTK_Param *param)
{
  if (verbose_flag) {
    put_param_info(param);
  }
}

/**********************************************************************/
/* register functions for module output */

/** 
 * <JA>
 * モジュール出力を行うよう関数を登録する．
 * 
 * </JA>
 * <EN>
 * Register output functions to enable module output.
 * 
 * </EN>
 */
void
setup_result_tty()
{
  status_process_online = ttyout_status_process_online;
  status_process_offline = ttyout_status_process_offline;
  status_recready      = ttyout_status_recready;
  status_recstart      = ttyout_status_recstart;
  status_recend        = ttyout_status_recend;
  status_param         = ttyout_status_param;
  result_pass1_begin   = ttyout_pass1_begin;
  result_pass1_current = ttyout_pass1_current;
  result_pass1_final   = ttyout_pass1_final;
  result_pass1_end     = ttyout_pass1_end;
  result_pass2_begin   = ttyout_pass2_begin;
  result_pass2         = ttyout_pass2;
  result_pass2_end     = ttyout_pass2_end;
  result_pass2_failed  = ttyout_pass2_failed;
  result_rejected      = ttyout_rejected;
  result_gmm           = ttyout_gmm;
#ifdef GRAPHOUT
  result_graph         = ttyout_graph;
#endif
}
