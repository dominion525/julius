/**
 * @file   result_msock.c
 * @author Akinobu Lee
 * @date   Tue Sep 06 14:46:49 2005
 * 
 * <JA>
 * @brief  ǧ����̤򥽥��åȤؽ��Ϥ��롥
 * </JA>
 * 
 * <EN>
 * @brief  Output recoginition result via module socket.
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
#include <time.h>

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
msock_status_process_online()
{
  module_send(module_sd, "<STARTPROC/>\n.\n");
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
msock_status_process_offline()
{
  module_send(module_sd, "<STOPPROC/>\n.\n");
}

/**********************************************************************/
/* decode outcode "WLPSwlps" to each boolean value */
/* default: "WLPS" */
static boolean out1_word = FALSE, out1_lm = FALSE, out1_phone = FALSE, out1_score = FALSE;
static boolean out2_word = TRUE, out2_lm = TRUE, out2_phone = TRUE, out2_score = TRUE;
static boolean out1_never = TRUE, out2_never = FALSE;
#ifdef CONFIDENCE_MEASURE
static boolean out2_cm = TRUE;
#endif

/** 
 * <JA>
 * ǧ����̤Ȥ��Ƥɤ����ä�ñ��������Ϥ��뤫�򥻥åȤ��롣
 * 
 * @param str [in] ���Ϲ��ܻ���ʸ���� ("WLPSCwlps"�ΰ���)
 * </JA>
 * <EN>
 * Setup which word information to be output as a recognition result.
 * 
 * @param str [in] output selection string (part of "WLPSCwlps")
 * </EN>
 */
void
decode_output_selection(char *str)
{
  int i;
  out1_word = out1_lm = out1_phone = out1_score = FALSE;
  out2_word = out2_lm = out2_phone = out2_score = FALSE;
#ifdef CONFIDENCE_MEASURE
  out2_cm = FALSE;
#endif
  for(i = strlen(str) - 1; i >= 0; i--) {
    switch(str[i]) {
    case 'W': out2_word  = TRUE; break;
    case 'L': out2_lm    = TRUE; break;
    case 'P': out2_phone = TRUE; break;
    case 'S': out2_score = TRUE; break;
    case 'w': out1_word  = TRUE; break;
    case 'l': out1_lm    = TRUE; break;
    case 'p': out1_phone = TRUE; break;
    case 's': out1_score = TRUE; break;
#ifdef CONFIDENCE_MEASURE
    case 'C': out2_cm    = TRUE; break;
#endif
    default:
      j_printerr("Error: unknown outcode `%c', ignored\n", str[i]);
      break;
    }
  }
  out1_never = ! (out1_word | out1_lm | out1_phone | out1_score);
  out2_never = ! (out2_word | out2_lm | out2_phone | out2_score
#ifdef CONFIDENCE_MEASURE
		  | out2_cm
#endif
		  );

}

/** 
 * <JA>
 * ǧ��ñ��ξ������Ϥ��륵�֥롼�������1�ѥ��ѡˡ�
 * 
 * @param w [in] ñ��ID
 * @param winfo [in] ñ�켭��
 * </JA>
 * <EN>
 * Subroutine to output information of a recognized word at 1st pass.
 * 
 * @param w [in] word ID
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
msock_word_out1(WORD_ID w, WORD_INFO *winfo)
{
  int j;
  static char buf[MAX_HMMNAME_LEN];

  if (out1_word) {
    module_send(module_sd, " WORD=\"%s\"", winfo->woutput[w]);
  }
  if (out1_lm) {
    module_send(module_sd, " CLASSID=\"%s\"", winfo->wname[w]);
  }
  if (out1_phone) {
    module_send(module_sd, " PHONE=\"");
    for(j=0;j<winfo->wlen[w];j++) {
      center_name(winfo->wseq[w][j]->name, buf);
      if (j == 0) module_send(module_sd, "%s", buf);
      else module_send(module_sd, " %s", buf);
    }
    module_send(module_sd, "\"");
  }
}

/** 
 * <JA>
 * ǧ��ñ��ξ������Ϥ��륵�֥롼�������2�ѥ��ѡˡ�
 * 
 * @param w [in] ñ��ID
 * @param winfo [in] ñ�켭��
 * </JA>
 * <EN>
 * Subroutine to output information of a recognized word at 2nd pass.
 * 
 * @param w [in] word ID
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
msock_word_out2(WORD_ID w, WORD_INFO *winfo)
{
  int j;
  static char buf[MAX_HMMNAME_LEN];

  if (out2_word) {
    module_send(module_sd, " WORD=\"%s\"", winfo->woutput[w]);
  }
  if (out2_lm) {
    module_send(module_sd, " CLASSID=\"%s\"", winfo->wname[w]);
  }
  if (out2_phone) {
    module_send(module_sd, " PHONE=\"");
    for(j=0;j<winfo->wlen[w];j++) {
      center_name(winfo->wseq[w][j]->name, buf);
      if (j == 0) module_send(module_sd, "%s", buf);
      else module_send(module_sd, " %s", buf);
    }
    module_send(module_sd, "\"");
  }
}


/**********************************************************************/
/* 1st pass output */

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
msock_pass1_begin()
{
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
msock_pass1_current(int t, WORD_ID *seq, int num, LOGPROB score, LOGPROB LMscore, WORD_INFO *winfo)
{
  int i;

  if (out1_never) return;	/* no output specified */

  if (out1_score) {
    module_send(module_sd, "<RECOGOUT>\n  <PHYPO PASS=\"1\" SCORE=\"%f\" FRAME=\"%d\" TIME=\"%ld\">\n", score, t, time(NULL));
  } else {
    module_send(module_sd, "<RECOGOUT>\n  <PHYPO PASS=\"1\" FRAME=\"%d\" TIME=\"%ld\">\n", t, time(NULL));
  }
  for (i=0;i<num;i++) {
    module_send(module_sd, "    <WHYPO");
    msock_word_out1(seq[i], winfo);
    module_send(module_sd, "/>\n");
  }
  module_send(module_sd, "  </PHYPO>\n</RECOGOUT>\n.\n");
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
msock_pass1_final(WORD_ID *seq, int num, LOGPROB score, LOGPROB LMscore, WORD_INFO *winfo)
{
  int i;

  if (out1_never) return;	/* no output specified */

  if (out1_score) {
    module_send(module_sd, "<RECOGOUT>\n  <SHYPO PASS=\"1\" SCORE=\"%f\">\n", score);
  } else {
    module_send(module_sd, "<RECOGOUT>\n  <SHYPO PASS=\"1\">\n", score);
  }
  for (i=0;i<num;i++) {
    module_send(module_sd, "    <WHYPO");
    msock_word_out1(seq[i], winfo);
    module_send(module_sd, "/>\n");
  }
  module_send(module_sd, "  </SHYPO>\n</RECOGOUT>\n.\n");
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
msock_pass1_end()
{
  /* no op */
}

/**********************************************************************/
/* 2nd pass output */

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
msock_pass2_begin()
{
  if (out2_never) return;	/* no output */
  module_send(module_sd, "<RECOGOUT>\n");
}

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
msock_pass2(NODE *hypo, int rank, WORD_INFO *winfo)
{
  int i;

  if (out2_never) return;	/* no output specified */

  if (hypo == NULL) return;

  module_send(module_sd, "  <SHYPO RANK=\"%d\"", rank);
  if (out2_score) {
    module_send(module_sd, " SCORE=\"%f\"", hypo->score);
#ifdef USE_NGRAM
    if (separate_score_flag) {
      module_send(module_sd, " AMSCORE=\"%f\" LMSCORE=\"%f\"", hypo->score - hypo->totallscore, hypo->totallscore);
    }
#endif
  }
#ifdef USE_DFA
  /* output which grammar the best hypothesis belongs to */
  /* determine only by the last word */
  module_send(module_sd, " GRAM=\"%d\"", multigram_get_gram_from_category(winfo->wton[hypo->seq[0]]));
#endif
  
  module_send(module_sd, ">\n");
  for (i=hypo->seqnum-1;i>=0;i--) {
    module_send(module_sd, "    <WHYPO");
    msock_word_out2(hypo->seq[i], winfo);
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
    /* currently not handle multiple alpha output */
#else
    if (out2_cm) {
      module_send(module_sd, " CM=\"%5.3f\"", hypo->cmscore[i]);
    }
#endif
#endif /* CONFIDENCE_MEASURE */
    module_send(module_sd, "/>\n");
  }
  
  module_send(module_sd, "  </SHYPO>\n");
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
msock_pass2_end()
{
  if (out2_never) return;	/* no output */
  module_send(module_sd, "</RECOGOUT>\n.\n");
}


#ifdef GRAPHOUT

/**********************************************************************/
/* word graph output */

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
msock_graph(WordGraph *root, WORD_INFO *winfo)
{
  WordGraph *wg;
  int i;
  int nodenum, arcnum;

  nodenum = graph_totalwordnum;
  arcnum = 0;
  for(wg=root;wg;wg=wg->next) {
    arcnum += wg->rightwordnum;
  }
  
  module_send(module_sd, "<GRAPHOUT NODENUM=\"%d\" ARCNUM=\"%d\">\n", nodenum, arcnum);

  for(wg=root;wg;wg=wg->next) {
    module_send(module_sd, "    <NODE GID=\"%d\"", wg->id);
    msock_word_out2(wg->wid, winfo);
    module_send(module_sd, " BEGIN=\"%d\"", wg->lefttime);
    module_send(module_sd, " END=\"%d\"", wg->righttime);
    module_send(module_sd, "/>\n");
  }
  for(wg=root;wg;wg=wg->next) {
    for(i=0;i<wg->rightwordnum;i++) {
      module_send(module_sd, "    <ARC FROM=\"%d\" TO=\"%d\"/>\n", wg->id, wg->rightword[i]->id);
    }
  }
  module_send(module_sd, "</GRAPHOUT>\n.\n");
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
msock_pass2_failed(WORD_INFO *winfo)
{
  if (out2_never) return;	/* no output */
  module_send(module_sd, "<RECOGFAIL/>\n.\n");
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
msock_rejected(const char *s)
{
  module_send(module_sd, "<REJECTED REASON=\"%s\"/>\n.\n", s);
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
msock_status_recready()
{
  module_send(module_sd, "<INPUT STATUS=\"LISTEN\" TIME=\"%ld\"/>\n.\n", time(NULL));
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
msock_status_recstart()
{
  module_send(module_sd, "<INPUT STATUS=\"STARTREC\" TIME=\"%ld\"/>\n.\n", time(NULL));
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
msock_status_recend()
{
  module_send(module_sd, "<INPUT STATUS=\"ENDREC\" TIME=\"%ld\"/>\n.\n", time(NULL));
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
msock_status_param(HTK_Param *param)
{
  module_send(module_sd, "<INPUTPARAM FRAMES=\"%d\" MSEC=\"%d\"/>\n.\n", param->samplenum, param->samplenum * (int)((float)para.smp_period * (float)para.frameshift / 10000.0));
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
setup_result_msock()
{
  if (!module_mode) {
    j_error("Error: result_msock: not in module mode!\n");
  }
  status_process_online = msock_status_process_online;
  status_process_offline = msock_status_process_offline;
  status_recready      = msock_status_recready;
  status_recstart      = msock_status_recstart;
  status_recend        = msock_status_recend;
  status_param         = msock_status_param;
  result_pass1_begin   = msock_pass1_begin;
  result_pass1_current = msock_pass1_current;
  result_pass1_final   = msock_pass1_final;
  result_pass1_end     = msock_pass1_end;
  result_pass2_begin   = msock_pass2_begin;
  result_pass2         = msock_pass2;
  result_pass2_end     = msock_pass2_end;
  result_pass2_failed  = msock_pass2_failed;
  result_rejected      = msock_rejected;
  result_gmm           = msock_gmm;
#ifdef GRAPHOUT
  result_graph         = msock_graph;
#endif
}
