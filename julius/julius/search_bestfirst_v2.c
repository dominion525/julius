/**
 * @file   search_bestfirst_v2.c
 * @author Akinobu Lee
 * @date   Mon Sep 12 00:58:50 2005
 * 
 * <JA>
 * @brief  ��2�ѥ��ǲ����Viterbi�黻����ӥ������׻���Ԥ���nextscan�ѡ�
 *
 * �����Ǥϡ���2�ѥ��ˤ�����õ����β����Viterbi�������ι����黻��
 * ��ñ��ȤΥȥ�ꥹ��³������Ӳ���Υ������׻���Ԥ��ؿ�����������
 * ���ޤ���
 *
 * ñ����³����ñ��ֲ��ǴĶ���¸���ϡ����Τ� nextscan ���르�ꥺ����Ѥ��ޤ���
 * ���Υե�������������Ƥ���ؿ��ϡ�config.h �ˤ����� PASS2_STRICT_IWCD
 * �� define �Ǥ���Ȥ��˻��Ѥ���ޤ����դ˾嵭�� undef �Ǥ���Ȥ��ϡ�
 * search_bestfirst_v1.c �δؿ����Ѥ����ޤ���
 *
 * Backscan �Ǥϡ��ǥ����ǥ��󥰤����٤�Ż뤷�ơ���ñ��Ȥ�������ñ���
 * ������ñ��ֲ��ǥ���ƥ����Ȥϲ���Ÿ�����ˤ��٤Ƹ�̩�˷׻�����ޤ���
 * Backscan ��Ԥʤ� search_bestfirst_v1.c ��������� POP ���˹Ԥʤ��Τ�
 * ��٤ơ������Ǥϲ��������λ��������Τʥ�������׻����뤿�ᡤ
 * ���������٤Ϲ⤤������������������뤹�٤Ƥβ�����Ф���
 * (���Ȥ������å�������ʤ�����Ǥ��äƤ�)�ȥ饤�ե���κƷ׻���Ԥʤ����ᡤ
 * �׻��̤� backscan ����٤����礷�ޤ���
 * </JA>
 * 
 * <EN>
 * @brief  Viterbi path update and hypothesis score calculation on the 2nd
 * pass, using nextscan algorithm.
 *
 * This file has functions for score calculations on the 2nd pass.
 * It includes Viterbi path update calculation of a hypothesis, calculations
 * of scores and word trellis connection at word expansion.
 * 
 * The cross-word triphone will be computed just at word expansion time,
 * for precise scoring.  This is called "nextscan" altgorithm. These
 * functions are enabled when PASS2_STRICT_IWCD is DEFINED in config.h.
 * If undefined, the "backscan" functions in search_bestfirst_v1.c will be
 * used instead.
 *
 * Here in nextscan algorithm, all cross-word context dependencies between
 * next word and source hypothesis are computed as soon as a new hypotheses
 * is expanded.  As the precise cross-word triphone score is applied on
 * hypothesis generation with no delay, more accurate search-time score can
 * be obtained than the delayed backscan method in search_bestfirst_v1.c.
 * On the other hand, the computational cost grows much by re-calculating
 * forward score of cross-word triphones for all the generated hypothethes,
 * even non-promising ones.
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

/* By "fast" setting (default), search_bestfirst_v1.c is used for faster
   decoding.  Please specify option "--enable-setup=standard" or
   "--enable-strict-iwcd2" at "./configure" to activate this. */

#include <julius.h>

#ifdef PASS2_STRICT_IWCD

#undef TCD			///< Define if want triphone debug messages


/**********************************************************************/
/************ ����Ρ��ɤδ������                         ************/
/************ Basic functions for hypothesis node handling ************/
/**********************************************************************/

#undef STOCKER_DEBUG

static NODE *stocker_root = NULL; ///< Node stocker for recycle

#ifdef STOCKER_DEBUG
static int stocked_num = 0;
static int reused_num = 0;
static int new_num = 0;
static int request_num = 0;
#endif

/** 
 * <JA>
 * ����Ρ��ɤ�ºݤ˥���夫��������롥
 * 
 * @param node [in] ����Ρ���
 * </JA>
 * <EN>
 * Free a hypothesis node actually.
 * 
 * @param node [in] hypothesis node
 * </EN>
 */
void
free_node_exec(NODE *node)
{
  if (node == NULL) return;
  free(node->g);
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  free(node->wordend_frame);
  free(node->wordend_gscore);
#endif
#endif
  free(node);
}

/** 
 * <JA>
 * ����Ρ��ɤ����Ѥ�λ���ƥꥵ�������Ѥ˥��ȥå�����
 * 
 * @param node [in] ����Ρ���
 * </JA>
 * <EN>
 * Stock an unused hypothesis node for recycle.
 * 
 * @param node [in] hypothesis node
 * </EN>
 */
void
free_node(NODE *node)
{
  if (node == NULL) return;

#ifdef GRAPHOUT
  if (node->prevgraph != NULL && node->prevgraph->saved == FALSE) {
    wordgraph_free(node->prevgraph);
  }
#endif

  /* save to stocker */
  node->next = stocker_root;
  stocker_root = node;

#ifdef STOCKER_DEBUG
  stocked_num++;
#endif
}

/** 
 * <JA>
 * �ꥵ�������ѥΡ��ɳ�Ǽ�ˤ���ˤ��롥
 * 
 * </JA>
 * <EN>
 * Clear the node stocker for recycle.
 * 
 * </EN>
 */
void
clear_stocker()
{
  NODE *node, *tmp;
  node = stocker_root;
  while(node) {
    tmp = node->next;
    free_node_exec(node);
    node = tmp;
  }
  stocker_root = NULL;

#ifdef STOCKER_DEBUG
  printf("%d times requested, %d times newly allocated, %d times reused\n", request_num, new_num, reused_num);
  stocked_num = 0;
  reused_num = 0;
  new_num = 0;
  request_num = 0;
#endif
}

/** 
 * <JA>
 * ����򥳥ԡ����롥
 * 
 * @param dst [out] ���ԡ���β���
 * @param src [in] ���ԡ����β���
 * 
 * @return @a dst ���֤���
 * </JA>
 * <EN>
 * Copy the content of node to another.
 * 
 * @param dst [out] target hypothesis
 * @param src [in] source hypothesis
 * 
 * @return the value of @a dst.
 * </EN>
 */
NODE *
cpy_node(NODE *dst, NODE *src)
{
  
  dst->next = src->next;
  dst->prev = src->prev;
  memcpy(dst->g, src->g, sizeof(LOGPROB) * peseqlen);
  memcpy(dst->seq, src->seq, sizeof(WORD_ID) * MAXSEQNUM);
#ifdef CM_SEARCH
#ifdef CM_MULTIPLE_ALPHA
  {
    int w;
    for(w=0;w<src->seqnum;w++) {
      memcpy(dst->cmscore[w], src->cmscore[w], sizeof(LOGPROB) * cm_alpha_num);
    }
  }     
#else
  memcpy(dst->cmscore, src->cmscore, sizeof(LOGPROB) * MAXSEQNUM);
#endif
#endif /* CM_SEARCH */
  dst->seqnum = src->seqnum;
  dst->score = src->score;
  dst->bestt = src->bestt;
  dst->estimated_next_t = src->estimated_next_t;
  dst->endflag = src->endflag;
#ifdef USE_DFA
  dst->state = src->state;
#endif
  dst->tre = src->tre;
  if (ccd_flag) {
    dst->last_ph = src->last_ph;
#ifdef MULTIPATH_VERSION
    dst->last_ph_sp_attached = src->last_ph_sp_attached;
#endif
  }
#ifdef USE_NGRAM
  dst->totallscore = src->totallscore;
#endif
#ifdef MULTIPATH_VERSION
  dst->final_g = src->final_g;
#endif
#ifdef VISUALIZE
  dst->popnode = src->popnode;
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  memcpy(dst->wordend_frame, src->wordend_frame, sizeof(short)*peseqlen);
  memcpy(dst->wordend_gscore, src->wordend_gscore, sizeof(LOGPROB)*peseqlen);
#endif
  dst->prevgraph = src->prevgraph;
  dst->lastcontext = src->lastcontext;
#ifndef GRAPHOUT_PRECISE_BOUNDARY
  dst->tail_g_score = src->tail_g_score;
#endif
#endif
  return(dst);
}

/** 
 * <JA>
 * �����ʲ���Ρ��ɤ����դ��롥�⤷��Ǽ�ˤ˰������Ѥ���ʤ��ʤä�
 * �Ρ��ɤ�������Ϥ��������Ѥ��롥�ʤ���п����˳���դ��롥
 * 
 * @return �����˳���դ���줿����Ρ��ɤؤΥݥ��󥿤��֤���
 * </JA>
 * <EN>
 * Allocate a new hypothesis node.  If the node stocker is not empty,
 * the one in the stocker is re-used.  Otherwise, allocate as new.
 * 
 * @return pointer to the newly allocated node.
 * </EN>
 */
NODE *
newnode()
{
  NODE *tmp;
  int i;

#ifdef STOCKER_DEBUG
  request_num++;
#endif
  if (stocker_root != NULL) {
    /* re-use ones in the stocker */
    tmp = stocker_root;
    stocker_root = tmp->next;
#ifdef STOCKER_DEBUG
    stocked_num--;
    reused_num++;
#endif
  } else {
    /* allocate new */
    if ((tmp=(NODE *)mymalloc(sizeof(NODE)))==NULL) {
      j_error("can't malloc\n");
    }
    tmp->g = (LOGPROB *)mymalloc(sizeof(LOGPROB)*peseqlen);
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    tmp->wordend_frame = (short *)mymalloc(sizeof(short)*peseqlen);
    tmp->wordend_gscore = (LOGPROB *)mymalloc(sizeof(LOGPROB)*peseqlen);
#endif
#endif
#ifdef STOCKER_DEBUG
    new_num++;
#endif
  }

  /* clear the data */
  /*bzero(tmp,sizeof(NODE));*/
  tmp->next=NULL;
  tmp->prev=NULL;
  tmp->last_ph = NULL;
#ifdef MULTIPATH_VERSION
  tmp->last_ph_sp_attached = FALSE;
#endif  
  if (ccd_flag) {
#ifdef USE_NGRAM
    tmp->totallscore = LOG_ZERO;
#endif
  }
  tmp->endflag = FALSE;
  tmp->seqnum = 0;
  for(i=0;i<peseqlen;i++) {
    tmp->g[i] = LOG_ZERO;
  }
#ifdef MULTIPATH_VERSION
  tmp->final_g = LOG_ZERO;
#endif
#ifdef VISUALIZE
  tmp->popnode = NULL;
#endif
#ifdef GRAPHOUT
  tmp->prevgraph = NULL;
  tmp->lastcontext = NULL;
#endif

  return(tmp);
}


/**********************************************************************/
/************ �������ȥ�ꥹŸ�������ٷ׻�             ****************/
/************ Expand trellis and update forward score *****************/
/**********************************************************************/

static LOGPROB *wordtrellis[2];	///< Buffer to compute viterbi path of a word
static int tn;		       ///< Temporal pointer to current buffer
static int tl;		       ///< Temporal pointer to previous buffer
static LOGPROB *g;		///< Buffer to hold source viterbi scores
static HMM_Logical **phmmseq;	///< Phoneme sequence to be computed
static int phmmlen_max;		///< Maximum length of @a phmmseq.
static HMM_Logical *tailph;	///< Last applied tail phone HMM
#ifdef MULTIPATH_VERSION
static boolean *has_sp;		///< Mark which phoneme allow short pause
#endif

#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
static short *wend_token_frame[2]; ///< Propagating token of word-end frame to detect corresponding end-of-words at word head
static LOGPROB *wend_token_gscore[2]; ///< Propagating token of scores at word-end to detect corresponding end-of-words at word head
static short *wef;		///< Work area for word-end frame tokens
static LOGPROB *wes;		///< Work area for word-end score tokens
#endif
#endif

/** 
 * <JA>
 * 1ñ��ʬ�Υȥ�ꥹ�׻��ѤΥ�����ꥢ����ݡ�
 * 
 * </JA>
 * <EN>
 * Allocate work area for trellis computation of a word.
 * 
 * </EN>
 */
void
malloc_wordtrellis()
{
  int maxwn;

  maxwn = winfo->maxwn + 10;	/* CCD�ˤ����ư���θ */
  wordtrellis[0] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);
  wordtrellis[1] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);

  g = (LOGPROB *)mymalloc(sizeof(LOGPROB) * peseqlen);

  phmmlen_max = winfo->maxwlen + 2;
  phmmseq = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * phmmlen_max);
#ifdef MULTIPATH_VERSION
  has_sp = (boolean *)mymalloc(sizeof(boolean) * phmmlen_max);
#endif

#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  wef = (short *)mymalloc(sizeof(short) * peseqlen);
  wes = (LOGPROB *)mymalloc(sizeof(LOGPROB) * peseqlen);
  wend_token_frame[0] = (short *)mymalloc(sizeof(short) * maxwn);
  wend_token_frame[1] = (short *)mymalloc(sizeof(short) * maxwn);
  wend_token_gscore[0] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);
  wend_token_gscore[1] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);
#endif
#endif
}

/** 
 * <JA>
 * 1ñ��ʬ�Υȥ�ꥹ�׻��ѤΥ�������ꥢ�����
 * 
 * </JA>
 * <EN>
 * Free the work area for trellis computation of a word.
 * 
 * </EN>
 */
void
free_wordtrellis()
{
  free(wordtrellis[0]);
  free(wordtrellis[1]);
  free(g);
  free(phmmseq);
#ifdef MULTIPATH_VERSION
  free(has_sp);
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  free(wef);
  free(wes);
  free(wend_token_frame[0]);
  free(wend_token_frame[1]);
  free(wend_token_gscore[0]);
  free(wend_token_gscore[1]);
#endif
#endif
}


/**********************************************************************/
/************ ��������������ٷ׻�                  *******************/
/************ Compute forward score of a hypothesis *******************/
/**********************************************************************/

/* Ϳ����줿���ǤΤʤ�� phmmseq[0..phmmlen-1]���Ф���viterbi�׻���Ԥ���
   g[0..framelen-1] �Υ����������ͤȤ��� g_new[0..framelen-1]�˹����ͤ�������
   ���� least_frame �ޤǤ�scan���롥*/
/* Viterbi computation for the given phoneme sequence 'phmmseq[0..phmmlen-1]'
   with g[0..framelen-1] as initial values.  The results are stored in
   g_new[0..framelen-1].  Scan should not terminate at least it reaches
   'least_frame'. */
/** 
 * <JA>
 * Ϳ����줿���Ǥ��¤Ӥ��Ф��� Viterbi �׻���Ԥ�����������������
 * �����������Ѵؿ���
 * 
 * @param g [in] ���ߤλ��֤��Ȥ�������������
 * @param g_new [out] ������ο��������������������Ǽ����Хåե�
 * @param phmmseq [in] ����HMM���¤�
 * @param phmmlen [in] @a phmmseq ��Ĺ��
 * @param param [in] ���ϥѥ�᡼��
 * @param framelen [in] ���ϥե졼��Ĺ
 * @param least_frame [in] �ӡ�������������Υե졼����ʾ�� Viterbi�׻�����
 * @param wordend_frame_src [in] ���ߤ�ñ�콪ü�ե졼��ȡ�����
 * @param wordend_frame_dst [out] ������ο�����ñ�콪ü�ե졼��ȡ�����
 * @param wordend_gscore_src [in] ���ߤ�ñ�콪ü�������ȡ�����
 * @param wordend_gscore_dst [out] ������ο�����ñ�콪ü�������ȡ�����
 * </JA>
 * <EN>
 * Generic function to perform Viterbi path updates for given phoneme
 * sequence.
 * 
 * @param g [in] current forward scores at each input frame
 * @param g_new [out] buffer to save the resulting score updates
 * @param phmmseq [in] phoneme sequence to perform Viterbi
 * @param phmmlen [in] length of @a phmmseq.
 * @param param [in] input parameter vector
 * @param framelen [in] input frame length to compute
 * @param least_frame [in] Least frame length to force viterbi even with beam
 * @param wordend_frame_src [in] current word-end frame tokens
 * @param wordend_frame_dst [out] buffer to store updated word-end frame tokens
 * @param wordend_gscore_src [in] current word-end score tokens
 * @param wordend_gscore_dst [out] buffer to store updated word-end score tokens
 * </EN>
 */
static void
do_viterbi(LOGPROB *g, LOGPROB *g_new, HMM_Logical **phmmseq
#ifdef MULTIPATH_VERSION
	   , boolean *has_sp
#endif
	   , int phmmlen, HTK_Param *param, int framelen, int least_frame
#ifdef MULTIPATH_VERSION
	   , LOGPROB *final_g
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	   , short *wordend_frame_src, short *wordend_frame_dst
	   , LOGPROB *wordend_gscore_src, LOGPROB *wordend_gscore_dst
#endif
#endif
	   )
{
  HMM *whmm;			/* HMM */
  int wordhmmnum;		/* length of above */
  int startt;			/* scan start frame */
  LOGPROB tmpmax,tmpscore;	/* variables for Viterbi process */
  A_CELL *ac;
  int t,i,j;
  boolean node_exist_p;

#ifdef TCD
  j_printf("scan for:");
  for (i=0;i<phmmlen;i++) {
    j_printf(" %s", phmmseq[i]->name);
  }
  j_printf("\n");
#endif
  
  /* ñ��HMM���� */
  /* make word HMM */
  whmm = new_make_word_hmm(hmminfo, phmmseq, phmmlen
#ifdef MULTIPATH_VERSION
			   , has_sp
#endif
			   );
  wordhmmnum = whmm->len;
  if (wordhmmnum >= winfo->maxwn + 10) {
    j_error("scan_word: word too long\n");
  }

  /* scan�������򸡺� -> startt��*/
  /* search for the start frame -> set to startt */
  for(t = framelen-1; t >=0 ; t--) {
    if (
#ifdef SCAN_BEAM
	g[t] > framemaxscore[t] - scan_beam_thres &&
#endif
	g[t] > LOG_ZERO) {
      break;
    }
  }
  if (t < 0) {			/* no node has score > LOG_ZERO */
    /* reset all scores and end */
    for(t=0;t<framelen;t++) {
      g_new[t] = LOG_ZERO;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      wordend_frame_dst[t] = -1;
      wordend_gscore_dst[t] = LOG_ZERO;
#endif
#endif
    }
    free_hmm(whmm);
    return;
  }
  startt = t;
  
  /* �������ʹ�[startt+1..framelen-1] �� g_new[] ��ꥻ�å� */
  /* clear g_new[] for [startt+1..framelen-1] */
  for(t=framelen-1;t>startt;t--) {
    g_new[t] = LOG_ZERO;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    wordend_frame_dst[t] = -1;
    wordend_gscore_dst[t] = LOG_ZERO;
#endif
#endif
  }

  /*****************/
  /* viterbi start */
  /*****************/

  /* set initial swap buffer */
  tn = 0; tl = 1;

#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  for(i=0;i<wordhmmnum;i++) {
    wend_token_frame[tn][i] = -1;
    wend_token_gscore[tn][i] = LOG_ZERO;
  }    
#endif
#endif

#ifndef MULTIPATH_VERSION
  /* ���� [startt] ����ͤ����� */
  /* initialize scores on frame [startt] */
  for(i=0;i<wordhmmnum-1;i++) wordtrellis[tn][i] = LOG_ZERO;
  wordtrellis[tn][wordhmmnum-1] = g[startt] + outprob(startt, &(whmm->state[wordhmmnum-1]), param);
  g_new[startt] = wordtrellis[tn][0];
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  wend_token_frame[tn][wordhmmnum-1] = wordend_frame_src[startt];
  wend_token_gscore[tn][wordhmmnum-1] = wordend_gscore_src[startt];
  wordend_frame_dst[startt] = wend_token_frame[tn][0];
  wordend_gscore_dst[startt] = wend_token_gscore[tn][0];
#endif
#endif
#endif /* ~MULTIPATH_VERSION */
  
  /* �ᥤ��롼��: startt ����Ϥޤ� 0 �˸����ä� Viterbi �׻� */
  /* main loop: start from [startt], and compute Viterbi toward [0] */
  for(t=
#ifdef MULTIPATH_VERSION
	startt
#else
	startt-1
#endif
	; t>=0; t--) {
    
    /* wordtrellis�Υ�����ꥢ�򥹥�å� */
    /* swap workarea of wordtrellis */
    i = tn; tn = tl; tl = i;

    node_exist_p = FALSE;	/* TRUE if there is at least 1 survived node in this frame */

#ifndef MULTIPATH_VERSION
    
    /* ü�ΥΡ��� [t][wordhmmnum-1]�ϡ��������� �� g[]�ι⤤���ˤʤ� */
    /* the edge node [t][wordhmmnum-1] is either internal transitin or g[] */
    tmpscore = LOG_ZERO;
    for (ac=whmm->state[wordhmmnum-1].ac;ac;ac=ac->next) {
      if (tmpscore < wordtrellis[tl][ac->arc] + ac->a)
	j = ac->arc;
	tmpscore = wordtrellis[tl][ac->arc] + ac->a;
    }
    if (g[t] > tmpscore) {
      tmpmax = g[t];
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      wend_token_frame[tn][wordhmmnum-1] = wordend_frame_src[t];
      wend_token_gscore[tn][wordhmmnum-1] = wordend_gscore_src[t];
#endif
#endif
    } else {
      tmpmax = tmpscore;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      wend_token_frame[tn][wordhmmnum-1] = wend_token_frame[tl][j];
      wend_token_gscore[tn][wordhmmnum-1] = wend_token_gscore[tl][j];
#endif
#endif
    }

    /* ü�ΥΡ��ɤΥ���������٥��ץ����å�: ���������ʤ���Ȥ� */
    /* check if the edge node is within score envelope */
    if (
#ifdef SCAN_BEAM
	tmpmax <= framemaxscore[t] - scan_beam_thres ||
#endif
	tmpmax <= LOG_ZERO
	) {
      wordtrellis[tn][wordhmmnum-1] = LOG_ZERO;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      wend_token_frame[tn][wordhmmnum-1] = -1;
      wend_token_gscore[tn][wordhmmnum-1] = LOG_ZERO;
#endif
#endif
    } else {
      node_exist_p = TRUE;
      wordtrellis[tn][wordhmmnum-1] = tmpmax + outprob(t, &(whmm->state[wordhmmnum-1]), param);
    }

#endif /* ~MULTIPATH_VERSION */

    /* node[wordhmmnum-2..0]�ˤĤ��ƥȥ�ꥹ��Ÿ�� */
    /* expand trellis for node [t][wordhmmnum-2..0] */
    for(i=wordhmmnum-2;i>=0;i--) {
      
      /* ����ѥ��Ⱥ��ॹ���� tmpmax �򸫤Ĥ��� */
      /* find most likely path and the max score 'tmpmax' */
      tmpmax = LOG_ZERO;
      for (ac=whmm->state[i].ac;ac;ac=ac->next) {
#ifdef MULTIPATH_VERSION
	if (ac->arc == wordhmmnum-1) tmpscore = g[t];
	else if (t + 1 > startt) tmpscore = LOG_ZERO;
	else tmpscore = wordtrellis[tl][ac->arc];
	tmpscore += ac->a;
#else
	tmpscore = wordtrellis[tl][ac->arc] + ac->a;
#endif
	if (tmpmax < tmpscore) {
	  tmpmax = tmpscore;
	  j = ac->arc;
	}
      }
      
      /* ����������٥��ץ����å�: ���������ʤ���Ȥ� */
      /* check if score of this node is within the score envelope */
      if (
#ifdef SCAN_BEAM
	  tmpmax <= framemaxscore[t] - scan_beam_thres ||
#endif
	  tmpmax <= LOG_ZERO
	  ) {
	/* invalid node */
	wordtrellis[tn][i] = LOG_ZERO;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	wend_token_frame[tn][i] = -1;
	wend_token_gscore[tn][i] = LOG_ZERO;
#endif
#endif
      } else {
	/* survived node */
	node_exist_p = TRUE;
#ifdef MULTIPATH_VERSION
 	wordtrellis[tn][i] = tmpmax;
 	if (i > 0) wordtrellis[tn][i] += outprob(t, &(whmm->state[i]), param);
#else
	wordtrellis[tn][i] = tmpmax + outprob(t, &(whmm->state[i]), param);
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
#ifdef MULTIPATH_VERSION
	if (j == wordhmmnum-1) {
	  wend_token_frame[tn][i] = wordend_frame_src[t];
	  wend_token_gscore[tn][i] = wordend_gscore_src[t];
	} else {
	  wend_token_frame[tn][i] = wend_token_frame[tl][j];
	  wend_token_gscore[tn][i] = wend_token_gscore[tl][j];
	}
#else
	wend_token_frame[tn][i] = wend_token_frame[tl][j];
	wend_token_gscore[tn][i] = wend_token_gscore[tl][j];
#endif
#endif
#endif
      }
    } /* end of node loop */

    /* ���� t ��Viterbi�׻���λ�������������������� g_new[t] �򥻥å� */
    /* Viterbi end for frame [t].  set the new forward score g_new[t] */
    g_new[t] = wordtrellis[tn][0];
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    /* new wordend */
    wordend_frame_dst[t] = wend_token_frame[tn][0];
    wordend_gscore_dst[t] = wend_token_gscore[tn][0];
#endif
#endif
    /* ���ꤵ�줿 least_frame �����ޤ� t ���ʤ�Ǥ��ꡤ���Ĥ��� t �ˤ�����
       ����������٥��פˤ�ä������Ĥä��Ρ��ɤ���Ĥ�̵���ä����,
       ���Υե졼��Ƿ׻����Ǥ��ڤꤽ��ʾ���([0..t-1])�Ϸ׻����ʤ� */
    /* if frame 't' already reached the 'least_frame' and no node was
       survived in this frame (all nodes pruned by score envelope),
       terminate computation at this frame and do not computer further
       frame ([0..t-1]). */
    if (t < least_frame && (!node_exist_p)) {
      /* crear the rest scores */
      for (i=t-1;i>=0;i--) {
	g_new[i] = LOG_ZERO;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	wordend_frame_dst[i] = -1;
	wordend_gscore_dst[i] = LOG_ZERO;
#endif
#endif
      }
      /* terminate loop */
      break;
    }
    
  } /* end of time loop */

#ifdef MULTIPATH_VERSION
  /* �������������κǽ��ͤ�׻� (���� 0 ������� 0 �ؤ�����) */
  /* compute the total forward score (transition from state 0 to frame 0 */
  if (t < 0) {			/* computed till the end */
    tmpmax = LOG_ZERO;
    for(ac=whmm->state[0].ac;ac;ac=ac->next) {
      tmpscore = wordtrellis[tn][ac->arc] + ac->a;
      if (tmpmax < tmpscore) tmpmax = tmpscore;
    }
    *final_g = tmpmax;
  } else {
    *final_g = LOG_ZERO;
  }
#endif

  /* free work area */
  free_hmm(whmm);
}

/** 
 * <JA>
 * �Ǹ��1���Ǥ��Ф��� Viterbi �׻���ʤ�롥
 * 
 * @param now [in] Ÿ������ʸ���⡥�첻�������������������� g[] �ˤ���Ȥ��롥
 * @param new [out] �׻������������������ g[] �˳�Ǽ����롥
 * @param lastphone [in] Viterbi�׻���Ԥ�����HMM
 * @param param [in] ���ϥ٥��ȥ���
 * </JA>
 * <EN>
 * Proceed Viterbi for the last one phoneme.
 * 
 * @param now [in] source hypothesis where the forward scores prior to the
 * last one phone is stored at g[]
 * @param new [out] the resulting updated forward scores will be saved to g[]
 * @param lastphone [in] phone HMM for the Viterbi processing
 * @param param [in] input vectors
 * </EN>
 */
static void
do_viterbi_next_word(NODE *now, NODE *new, HMM_Logical *lastphone
#ifdef MULTIPATH_VERSION
		     , boolean sp
#endif
		     , HTK_Param *param)
{
  int t, n;
#ifndef MULTIPATH_VERSION
  LOGPROB a_value;

  /* �⤷Ÿ��������κǸ��ñ��β���Ĺ�� 1 �Ǥ���С����β��Ǥ�
     ľ���� scan_word �Ƿ׻�����Ƥ��ʤ������ξ��, now->g[] �˰�����
     ����ͤ���Ǽ����Ƥ��롥
     �⤷����Ĺ�����ʾ�Ǥ���С�now->g[] �Ϥ��μ����ޤǷ׻���������
     �Υ����������äƤ���Τ�,now->g[t] �������ͤ����ꤹ��ɬ�פ����� */
  /* If the length of last word is 1, it means the last phone was not
     scanned in the last call of scan_word().  In this case, now->g[]
     keeps the previous initial value, so start viterbi with the old scores.
     If the length is more than 1, the now->g[] keeps the values of the
     scan result till the previous phone, so make initial value
     considering last transition probability. */
  if (winfo->wlen[now->seq[now->seqnum-1]] > 1) {
    n = hmm_logical_state_num(lastphone);
    a_value = (hmm_logical_trans(lastphone))->a[n-2][n-1];
    for(t=0; t<peseqlen-1; t++) g[t] = now->g[t+1] + a_value;
    g[peseqlen-1] = LOG_ZERO;
  } else {
    for(t=0; t<peseqlen; t++) g[t] = now->g[t];
  }
  
# else /* MULTIPATH_VERSION */
  
  for(t=0; t<peseqlen; t++) g[t] = now->g[t];
  phmmseq[0] = lastphone;
  has_sp[0] = sp;
  
#endif /* MULTIPATH_VERSION */
  
  do_viterbi(g, new->g
#ifdef MULTIPATH_VERSION
	     , phmmseq, has_sp, 1, param
#else
	     , &lastphone, 1, param
#endif
	     , peseqlen, now->estimated_next_t
#ifdef MULTIPATH_VERSION
	     , &(new->final_g)
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	     , now->wordend_frame, new->wordend_frame
	     , now->wordend_gscore, new->wordend_gscore
#endif
#endif
	     );

#ifndef MULTIPATH_VERSION
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  /* ����� next_word �Ѥ˶��������Ĵ�� */
  /* proceed word boundary for one step for next_word */
  new->wordend_frame[peseqlen-1] = new->wordend_frame[0];
  new->wordend_gscore[peseqlen-1] = new->wordend_gscore[0];
  for (t=0;t<peseqlen-1;t++) {
    new->wordend_frame[t] = new->wordend_frame[t+1];
    new->wordend_gscore[t] = new->wordend_gscore[t+1];
  }
#endif
#endif
#endif
}

/** 
 * <JA>
 * �Ǹ��1ñ����������ȥ�ꥹ��׻����ơ�ʸ��������������٤򹹿����롥
 * 
 * @param now [i/o] ʸ����
 * @param param [in] ���ϥѥ�᡼����
 * </JA>
 * <EN>
 * Compute the forward viterbi for the last word to update forward scores
 * and ready for word connection.
 * 
 * @param now [i/o] hypothesis
 * @param param [in] input parameter vectors
 * </EN>
 */
void
scan_word(NODE *now, HTK_Param *param)
{
  int   i,t;
  WORD_ID word;
  int phmmlen;
#ifdef MULTIPATH_VERSION
  boolean tail_ph_sp_attached;
#endif
  
#ifdef GRAPHOUT
#ifndef GRAPHOUT_PRECISE_BOUNDARY
  if (ccd_flag) {
    now->tail_g_score = now->g[now->bestt];
  }
#endif
#endif

  /* ----------------------- prepare phoneme sequence ------------------ */
  /* triphone�ʤ���Ƭ��1���ǤϤ����Ǥ��оݳ�(���Ȥ�next_word�Ǥ��) */
  /*             ������1���Ǥϥ���ƥ����Ȥˤ������ä��ִ� */
  /* with triphone, modify the tail phone of the last word according to the
     previous word, and do not compute the head phone here (that will be
     computed later in next_word() */
  word = now->seq[now->seqnum-1];
  
#ifdef TCD
    j_printf("w=");
    for(i=0;i<winfo->wlen[word];i++) {
      j_printf(" %s",(winfo->wseq[word][i])->name);
    }
    if (ccd_flag) {
      if (now->last_ph != NULL) {
	j_printf(" | %s", (now->last_ph)->name);
      }
    }
    j_printf("\n");
#endif /* TCD */
    
  if (ccd_flag) {
    
    /* the tail triphone of the last word varies by context */
    if (now->last_ph != NULL) {
      tailph = get_right_context_HMM(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name, hmminfo);
      if (tailph == NULL) {
	/* fallback to the original bi/mono-phone */
	/* error if the original is pseudo phone (not explicitly defined
	   in hmmdefs/hmmlist) */
	/* exception: word with 1 phone (triphone may exist in the next expansion */
	if (winfo->wlen[word] > 1 && winfo->wseq[word][winfo->wlen[word]-1]->is_pseudo){
	  error_missing_right_triphone(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name);
	}

	tailph = winfo->wseq[word][winfo->wlen[word]-1];
      }
    } else {
      tailph = winfo->wseq[word][winfo->wlen[word]-1];
    }
    /* Ĺ��1��ñ��ϼ���nextword�Ǥ�����Ѳ�����ΤǤ����Ǥ�scan���ʤ� */
    /* do not scan word if the length is 1, as it further varies in the
       following next_word() */
    if (winfo->wlen[word] == 1) {
      now->last_ph = tailph;
#ifdef MULTIPATH_VERSION
      now->last_ph_sp_attached = TRUE;
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      /* ñ�춭�����¾�������� */
      /* initialize word boundary propagation info */
      for (t=0;t<peseqlen;t++) {
	now->wordend_frame[t] = t;
	now->wordend_gscore[t] = now->g[t];
      }
#endif
#endif
#ifdef TCD
      j_printf("suspended as %s\n", (now->last_ph)->name);
#endif
      return;
    }

    /* scan�ϰϤβ��������� */
    /* prepare HMM of the scan range */
    phmmlen = winfo->wlen[word] - 1;
    if (phmmlen > phmmlen_max) {
      j_error("short of phmmlen\n");
    }
    for (i=0;i<phmmlen-1;i++) {
      phmmseq[i] = winfo->wseq[word][i+1];
#ifdef MULTIPATH_VERSION
      has_sp[i] = FALSE;
#endif
    }
    phmmseq[phmmlen-1] = tailph;
#ifdef MULTIPATH_VERSION
    has_sp[phmmlen-1] = (enable_iwsp) ? TRUE : FALSE;
#endif
  } else {
    phmmlen = winfo->wlen[word];
#ifdef MULTIPATH_VERSION
    for (i=0;i<phmmlen;i++) {
      phmmseq[i] = winfo->wseq[word][i];
      has_sp[i] = FALSE;
    }
    if (enable_iwsp) has_sp[phmmlen-1] = TRUE;
#else
    for (i=0;i<phmmlen;i++) phmmseq[i] = winfo->wseq[word][i];
#endif
  }

  /* ����g[]�򤤤ä������򤷤Ƥ��� */
  /* temporally keeps the original g[] */
  for (t=0;t<peseqlen;t++) g[t] = now->g[t];

#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  /* ñ�춭�����¾�������� */
  /* initialize word boundary propagation info */
  for (t=0;t<peseqlen;t++) {
    wef[t] = t;
    wes[t] = now->g[t];
  }
#endif
#endif

  /* viterbi��¹Ԥ��� g[] ���� now->g[] �򹹿����� */
  /* do viterbi computation for phmmseq from g[] to now->g[] */
  do_viterbi(g, now->g, phmmseq
#ifdef MULTIPATH_VERSION
	     , has_sp
#endif
	     , phmmlen, param, peseqlen, now->estimated_next_t
#ifdef MULTIPATH_VERSION
	     , &(now->final_g)
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	     /* ñ�춭������ we[] ���� now->wordend_frame[] �򹹿����� */
	     /* propagate word boundary info from we[] to now->wordend_frame[] */
	     , wef, now->wordend_frame
	     , wes, now->wordend_gscore
#endif
#endif
	     );
#ifndef MULTIPATH_VERSION
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  /* ����� next_word �Ѥ˶��������Ĵ�� */
  /* proceed word boundary for one step for next_word */
  now->wordend_frame[peseqlen-1] = now->wordend_frame[0];
  now->wordend_gscore[peseqlen-1] = now->wordend_gscore[0];
  for (t=0;t<peseqlen-1;t++) {
    now->wordend_frame[t] = now->wordend_frame[t+1];
    now->wordend_gscore[t] = now->wordend_gscore[t+1];
  }
#endif
#endif
#endif

  if (ccd_flag) {
    /* ����Τ���� now->last_ph �򹹿� */
    /* update 'now->last_ph' for future scan_word() */
    now->last_ph = winfo->wseq[word][0];
#ifdef MULTIPATH_VERSION
    now->last_ph_sp_attached = FALSE; /* wlen > 1 here */
#endif
#ifdef TCD
    j_printf("last_ph = %s\n", (now->last_ph)->name);
#endif
  }
}


/**************************************************************************/
/*** �������Ÿ���ȥҥ塼�ꥹ�ƥ��å���Ҥ������Υ�������׻�           ***/
/*** Expand new hypothesis and compute the total score (with heuristic) ***/
/**************************************************************************/

static HMM_Logical *lastphone, *newphone;
static LOGPROB *g_src;

/** 
 * <JA>
 * Ÿ��������˼�ñ�����³���ƿ�����������������롥��ñ���ñ��ȥ�ꥹ���
 * ���������������³�����ᡤ���⥹������׻����롥
 * 
 * @param now [in] Ÿ��������
 * @param new [out] �������������줿���⤬��Ǽ�����
 * @param nword [in] ��³���뼡ñ��ξ���
 * @param param [in] ���ϥѥ�᡼����
 * @param backtrellis [in] ñ��ȥ�ꥹ
 * </JA>
 * <EN>
 * Connect a new word to generate a next hypothesis.  The optimal connection
 * point and new sentence score of the new hypothesis will be estimated by
 * looking up the corresponding words on word trellis.
 * 
 * @param now [in] source hypothesis
 * @param new [out] pointer to save the newly generated hypothesis
 * @param nword [in] next word to be connected
 * @param param [in] input parameter vector
 * @param backtrellis [in] word trellis
 * </EN>
 */
void
next_word(NODE *now, NODE *new, NEXTWORD *nword, HTK_Param *param, BACKTRELLIS *backtrellis)
{
  int   t;
  int lastword;
  int   i;
  LOGPROB a_value;
  LOGPROB tmpp;
  int   startt;
  int word;
  TRELLIS_ATOM *tre;
  LOGPROB totalscore;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  short *wendf;
  LOGPROB *wends;
#endif
#endif


  word = nword->id;
  lastword = now->seq[now->seqnum-1];

  /* lastphone (ľ��ñ�����Ƭ����) ����� */
  /* prepare lastphone (head phone of previous word) */
  if (ccd_flag) {
    /* �ǽ����� triphone ����³ñ��˲�碌���Ѳ� */
    /* modify triphone of last phone according to the next word */
    lastphone = get_left_context_HMM(now->last_ph, winfo->wseq[word][winfo->wlen[word]-1]->name, hmminfo);
    if (lastphone == NULL) {
      /* fallback to the original bi/mono-phone */
      /* error if the original is pseudo phone (not explicitly defined
	 in hmmdefs/hmmlist) */
      /* exception: word with 1 phone (triphone may exist in the next expansion */
      if (now->last_ph->is_pseudo){
	error_missing_left_triphone(now->last_ph, winfo->wseq[word][winfo->wlen[word]-1]->name);
      }
      lastphone = now->last_ph;
    }
  }

  /* newphone (��³ñ�����������) ����� */
  /* prepare newphone (tail phone of next word) */
  if (ccd_flag) {
    newphone = get_right_context_HMM(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name, hmminfo);
    if (newphone == NULL) {
      /* fallback to the original bi/mono-phone */
      /* error if the original is pseudo phone (not explicitly defined
	 in hmmdefs/hmmlist) */
      /* exception: word with 1 phone (triphone may exist in the next expansion */
      if (winfo->wlen[word] > 1 && winfo->wseq[word][winfo->wlen[word]-1]->is_pseudo){
	error_missing_right_triphone(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name);
      }
      newphone = winfo->wseq[word][winfo->wlen[word]-1];
    }
  } else {
    newphone = winfo->wseq[word][winfo->wlen[word]-1];
  }
  
  /* ñ���¤ӡ�DFA�����ֹ桢���쥹������ new �طѾ������� */
  /* inherit and update word sequence, DFA state and total LM score to 'new' */
  new->score = LOG_ZERO;
  for (i=0;i< now->seqnum;i++){
    new->seq[i] = now->seq[i];
#ifdef CM_SEARCH
#ifdef CM_MULTIPLE_ALPHA
    memcpy(new->cmscore[i], now->cmscore[i], sizeof(LOGPROB) * cm_alpha_num);
#else
    new->cmscore[i] = now->cmscore[i];
#endif
#endif /* CM_SEARCH */
  }
  new->seq[i] = word;
  new->seqnum = now->seqnum+1;
#ifdef USE_DFA
  new->state = nword->next_state;
#endif
#ifdef USE_NGRAM
  new->totallscore = now->totallscore + nword->lscore;
#endif  
  if (ccd_flag) {
    /* ��������������Ȥ�����¸ */
    /* keep the lastphone for next scan_word() */
    new->last_ph = lastphone;
#ifdef MULTIPATH_VERSION
    new->last_ph_sp_attached = now->last_ph_sp_attached;
#endif
  }

  if (ccd_flag) {
    /* �Ǹ��1����(lastphone)ʬ��scan�������������������� new ����¸ */
    /* scan the lastphone and set the updated score to new->g[] */
    do_viterbi_next_word(now, new, lastphone
#ifdef MULTIPATH_VERSION
			 , now->last_ph_sp_attached
#endif
			 , param);
    g_src = new->g;
  } else {
    g_src = now->g;
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    memcpy(new->wordend_frame, now->wordend_frame, sizeof(short)*peseqlen);
    memcpy(new->wordend_gscore, now->wordend_gscore, sizeof(LOGPROB)*peseqlen);
#endif
#endif
  }
      
  /* ����� scan_word �������� new->g[] ���ѹ����Ƥ��� */
  /* prepare new->g[] for next scan_word() */
#ifdef MULTIPATH_VERSION
  startt = peseqlen-1;
#else
  startt = peseqlen-2;
#endif
  i = hmm_logical_state_num(newphone);
  a_value = (hmm_logical_trans(newphone))->a[i-2][i-1];
  for(t=0; t <= startt; t++) {
    new->g[t] =
#ifdef MULTIPATH_VERSION
      g_src[t]
#else
      g_src[t+1] + a_value
#endif
#ifdef USE_NGRAM
      + nword->lscore
#else
      + penalty2
#endif
      ;
  }

  /***************************************************************************/
  /* ������(�裲�ѥ�),������(�裱�ѥ�)�ȥ�ꥹ����³��������³���򸫤Ĥ��� */
  /* connect forward/backward trellis to look for the best connection time   */
  /***************************************************************************/
  /*-----------------------------------------------------------------*/
  /* ñ��ȥ�ꥹ��õ����, ��ñ��κ�����³����ȯ������ */
  /* determine the best connection time of the new word, seeking the word
     trellis */
  /*-----------------------------------------------------------------*/

#ifdef USE_DFA
  if (!looktrellis_flag) {
    /* ���٤ƤΥե졼��ˤ錄�äƺ����õ�� */
    /* search for best trellis word throughout all frame */
    for(t = startt; t >= 0; t--) {
      tre = bt_binsearch_atom(backtrellis, t, (WORD_ID) word);
      if (tre == NULL) continue;
#ifdef MULTIPATH_VERSION
      totalscore = new->g[t] + tre->backscore;
#else
      if (newphone->is_pseudo) {
	tmpp = outprob_cd(t, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
      } else {
	tmpp = outprob_state(t, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
      }
      totalscore = new->g[t] + tre->backscore + tmpp;
#endif
      if (new->score < totalscore) {
	new->score = totalscore;
	new->bestt = t;
	new->estimated_next_t = tre->begintime - 1;
	new->tre = tre;
      }
    }
  } else {
#endif /* USE_DFA */
    
  /* �Ǹ�˻��Ȥ���TRELLIS_ATOM�ν�ü���֤����� */
  /* new�ο�����֤ϡ��嵭�Ǻ��Ѥ���TRELLIS_ATOM�λ�ü���� */

  /* ����Ÿ��ñ��Υȥ�ꥹ��ν�ü���֤�����Τߥ�����󤹤�
     �����Ϣ³����¸�ߤ���ե졼��ˤĤ��ƤΤ߷׻� */
  /* search for best trellis word only around the estimated time */
  /* 1. search forward */
  for(t = (nword->tre)->endtime; t >= 0; t--) {
    tre = bt_binsearch_atom(backtrellis, t, (WORD_ID) word);
    if (tre == NULL) break;	/* go to 2 if the trellis word disappear */
#ifdef MULTIPATH_VERSION
    totalscore = new->g[t] + tre->backscore;
#else
    if (newphone->is_pseudo) {
      tmpp = outprob_cd(t, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
    } else {
      tmpp = outprob_state(t, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
    }
    totalscore = new->g[t] + tre->backscore + tmpp;
#endif
    if (new->score < totalscore) {
      new->score = totalscore;
      new->bestt = t;
      new->estimated_next_t = tre->begintime - 1;
      new->tre = tre;
    }
  }
  /* 2. search bckward */
  for(t = (nword->tre)->endtime + 1; t <= startt; t++) {
    tre = bt_binsearch_atom(backtrellis, t, (WORD_ID) word);
    if (tre == NULL) break;	/* end if the trellis word disapper */
#ifdef MULTIPATH_VERSION
    totalscore = new->g[t] + tre->backscore;
#else
    if (newphone->is_pseudo) {
      tmpp = outprob_cd(t, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
    } else {
      tmpp = outprob_state(t, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
    }
    totalscore = new->g[t] + tre->backscore + tmpp;
#endif
    if (new->score < totalscore) {
      new->score = totalscore;
      new->bestt = t;
      new->estimated_next_t = tre->begintime - 1;
      new->tre = tre;
    }
  }

#ifdef USE_DFA
  }
#endif

#ifdef USE_NGRAM
  /* set current LM score */
  new->lscore = nword->lscore;
#endif
  
}


/**********************************************************************/
/********** ������������                 ****************************/
/********** Generate an initial hypothesis ****************************/
/**********************************************************************/

/** 
 * <JA>
 * Ϳ����줿ñ�줫����������������롥
 * 
 * @param new [out] �������������줿���⤬��Ǽ�����
 * @param nword [in] �������ñ��ξ���
 * @param param [in] ���ϥѥ�᡼����
 * @param backtrellis [in] ñ��ȥ�ꥹ
 * </JA>
 * <EN>
 * Generate an initial hypothesis from given word.
 * 
 * @param new [out] pointer to save the newly generated hypothesis
 * @param nword [in] words of the first candidates
 * @param param [in] input parameter vector
 * @param backtrellis [in] word trellis
 *
 * </EN>
 */
void
start_word(NODE *new, NEXTWORD *nword, HTK_Param *param, BACKTRELLIS *backtrellis)
{
  HMM_Logical *newphone;
  WORD_ID word;
  TRELLIS_ATOM *tre = NULL;
  LOGPROB tmpp;
  int t;

  /* initialize data */
  word = nword->id;
  new->score = LOG_ZERO;
  new->seqnum = 1;
  new->seq[0] = word;

#ifdef USE_DFA
  new->state = nword->next_state;
#endif
#ifdef USE_NGRAM
  new->totallscore = nword->lscore;
#endif  

#ifdef USE_NGRAM
  /* set current LM score */
  new->lscore = nword->lscore;
#endif

  /* cross-word triphone need not be handled on startup */
  newphone = winfo->wseq[word][winfo->wlen[word]-1];
  if (ccd_flag) {
    new->last_ph = NULL;
  }
  
#ifdef USE_NGRAM
  new->g[peseqlen-1] = nword->lscore;
#else  /* USE_DFA */
  new->g[peseqlen-1] = 0;
#endif
  
  for (t=peseqlen-1; t>=0; t--) {
    tre = bt_binsearch_atom(backtrellis, t, word);
    if (tre != NULL) {
#ifdef GRAPHOUT
      new->bestt = peseqlen-1;
#else
      new->bestt = t;
#endif
#ifdef MULTIPATH_VERSION
      new->score = new->g[peseqlen-1] + tre->backscore;
#else
      if (newphone->is_pseudo) {
	tmpp = outprob_cd(peseqlen-1, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
      } else {
	tmpp = outprob_state(peseqlen-1, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
      }
      new->score = new->g[peseqlen-1] + tre->backscore + tmpp;
#endif
      new->estimated_next_t = tre->begintime - 1;
      new->tre = tre;
      break;
    }
  }
  if (tre == NULL) {		/* no word in backtrellis */
    new->score = LOG_ZERO;
  }
}

/** 
 * <JA>
 * ��ü��������ü�ޤ�ã����ʸ����κǽ�Ū�ʥ������򥻥åȤ��롥
 * 
 * @param now [in] ��ü�ޤ�ã��������
 * @param new [out] �ǽ�Ū��ʸ����Υ��������Ǽ������ؤΥݥ���
 * @param param [in] ���ϥѥ�᡼����
 * </JA>
 * <EN>
 * Hypothesis termination: set the final sentence scores of hypothesis
 * that has already reached to the end.
 * 
 * @param now [in] hypothesis that has already reached to the end
 * @param new [out] pointer to save the final sentence information
 * @param param [in] input parameter vectors
 * </EN>
 */
void
last_next_word(NODE *now, NODE *new, HTK_Param *param)
{
  cpy_node(new, now);
  if (ccd_flag) {
    /* �ǽ�����ʬ�� viterbi ���ƺǽ������������� */
    /* scan the last phone and update the final score */
    do_viterbi_next_word(now, new, now->last_ph
#ifdef MULTIPATH_VERSION
			 , now->last_ph_sp_attached
#endif
			 , param);
#ifdef MULTIPATH_VERSION
    new->score = new->final_g;
#else
    new->score = new->g[0];
#endif
  } else {
#ifdef MULTIPATH_VERSION
    new->score = now->final_g;
#else
    new->score = now->g[0];
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    /* last boundary has moved to [peseqlen-1] in last scan_word() */
    memcpy(new->wordend_frame, now->wordend_frame, sizeof(short)*peseqlen);
    memcpy(new->wordend_gscore, now->wordend_gscore, sizeof(LOGPROB)*peseqlen);
#endif
#endif
  }
}

#endif /* PASS2_STRICT_IWCD */
