/**
 * @file   result_tty.c
 * @author Akinobu Lee
 * @date   Tue Sep 06 17:18:46 2005
 * 
 * <JA>
 * @brief  ǧ����̤�ɸ����Ϥؽ��Ϥ��롥
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
 * ǧ����ǽ�ʾ��֤ˤʤä��Ȥ��˸ƤФ��
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
 * ǧ���������Ǿ��֤ˤʤä��Ȥ��˸ƤФ��
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
 * ��1�ѥ�������ǧ���򳫻Ϥ���ݤν��ϡʲ������ϳ��ϻ��˸ƤФ��ˡ�
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
 * ��1�ѥ��������̤���Ϥ������1�ѥ��ΰ�����֤��Ȥ˸ƤФ���
 * 
 * @param t [in] ���ߤλ��֥ե졼��
 * @param seq [in] ���ߤΰ�̸���ñ����
 * @param num [in] @a seq ��Ĺ��
 * @param score [in] �嵭�Τ���ޤǤ����ѥ�����
 * @param LMscore [in] �嵭�κǸ��ñ��ο�����
 * @param winfo [in] ñ�켭��
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
 * ��1�ѥ�����λ������1�ѥ��η�̤���Ϥ������1�ѥ���λ�塢��2�ѥ���
 * �Ϥޤ����˸ƤФ�롥ǧ���˼��Ԥ������ϸƤФ�ʤ��ˡ�
 * 
 * @param seq [in] ��1�ѥ���1�̸����ñ����
 * @param num [in] �嵭��Ĺ��
 * @param score [in] 1�̤����Ѳ��⥹����
 * @param LMscore [in] @a score �Τ������쥹����
 * @param winfo [in] ñ�켭��
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
 * ��1�ѥ�����λ���ν��ϡ���1�ѥ��ν�λ����ɬ���ƤФ���
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
 * �������ñ��������Ϥ���
 * 
 * @param hypo [in] ����
 * @param winfo [in] ñ�켭��
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
 * �����N-gram�����Julian�Ǥϥ��ƥ����ֹ���ˤ���Ϥ��롥
 * 
 * @param hypo [in] ʸ����
 * @param winfo [in] ñ�켭��
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
 * ����β��Ƿ������Ϥ��롥
 * 
 * @param hypo [in] ʸ����
 * @param winfo [in] ñ�����
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
 * �����ñ�줴�Ȥο����٤���Ϥ��롥
 * 
 * @param hypo [in] ʸ����
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
 * ��2�ѥ�������줿ʸ��������1�Ľ��Ϥ��롥
 * 
 * @param hypo [in] ����줿ʸ����
 * @param rank [in] @a hypo �ν��
 * @param winfo [in] ñ�켭��
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
 * ��2�ѥ�������ǧ����̤ν��Ϥ򳫻Ϥ���ݤν��ϡ�ǧ����̤���Ϥ���ݤˡ�
 * ���ֺǽ�˽��Ϥ���롥
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
 * ��2�ѥ�����λ��
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
 * ����줿ñ�쥰������Τ���Ϥ��롥
 * 
 * @param root [in] �����ñ�콸�����Ƭ���ǤؤΥݥ���
 * @param winfo [in] ñ�켭��
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
 * ��2�ѥ���õ���˼��Ԥ����Ȥ��ν��ϡ�
 * 
 * @param winfo [in] ñ�켭��
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
 * ���Ϥ����Ѥ��줿�Ȥ��ν��ϡ�GMM ������Ĺ�����Ϥ����Ѥ��줿�Ȥ��˸ƤФ�롥
 * 
 * @param s [in] ��ͳ�򤢤�魯ʸ����
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
 * ��������λ���ơ�ǧ����ǽ���֡������Ԥ����֡ˤ����ä��Ȥ��ν���
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
 * ���Ϥγ��Ϥ򸡽Ф����Ȥ��ν���
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
 * ���Ͻ�λ�򸡽Ф����Ȥ��ν���
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
 * ����Ĺ�ʤɤ����ϥѥ�᡼���������ϡ�
 * 
 * @param param [in] ���ϥѥ�᡼����¤��
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
 * �⥸�塼����Ϥ�Ԥ��褦�ؿ�����Ͽ���롥
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
