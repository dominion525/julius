/**
 * @file   search_bestfirst_main.c
 * @author Akinobu Lee
 * @date   Thu Sep 08 11:51:12 2005
 * 
 * <JA>
 * @brief  ��2�ѥ��Υ����å��ǥ����ǥ���
 *
 * Julius/Julian ����2�ѥ��Ǥ��륹���å��ǥ����ǥ��󥰥��르�ꥺ�ब
 * ���Ҥ���Ƥ��ޤ�����1�ѥ��η�̤�ñ��ȥ�ꥹ����򸵤ˡ���1�ѥ��Ȥ�
 * �ո����� right-to-left ��õ����Ԥ��ޤ�������Υ������ϡ���1�ѥ��Υȥ�ꥹ
 * �Ȥ��Υ�������̤õ�����Υҥ塼�ꥹ�ƥ��å��Ȥ�����³���뤳�Ȥǡ�
 * ʸ���Τβ��⥹�������θ���ʤ���õ����Ԥ��ޤ���
 *
 * ñ���Ÿ���Ǥϡ�Julius �Ǥ� ngram_decode.c ��δؿ�����Julian���Ǥ�
 * dfa_decode.c �δؿ�����ñ�콸���������뤿��˻��Ѥ���ޤ���
 * 
 * ñ�쿮���٤η׻���ӡ�����θ�����������׻���enveloped beam õ����
 * �����ǵ��Ҥ���Ƥ��ޤ���
 * 
 * </JA>
 * 
 * <EN>
 * @brief  Main search function for stack decoding on the 2nd pass.
 *
 * This file implements search algorithm based on best-first stack
 * decoding on the 2nd pass.  The search will be performed on backward
 * (i.e. right-to-left) direction, using the result of 1st pass (word
 * trellis) as heuristics of unreached area.  Hypothesis are stored
 * in a global stack, and the best one will be expanded according to
 * the survived words in the word trellis and language constraint.
 *
 * The expanding words will be given by ngram_decode.c for N-gram based
 * recognition (Julius), with their langugage probabilities, or by
 * dfa_decode.c for grammar-based recognition (Julian), with their emitting
 * DFA state information.
 *
 * The dynamic confidence scoring using local posterior probability,
 * score envelope beaming, and other techniques are also implemented
 * within this file.
 * </EN>
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

#include <julius.h>

/* declaration of local functions */
static NODE *get_best_from_stack(NODE **start, int *stacknum);
static int put_to_stack(NODE *new, NODE **start, NODE **bottom, int *stacknum, int stacksize);
static void put_all_in_stack(NODE **start, int *stacknum);
static void free_all_nodes(NODE *node);
static void put_hypo_woutput(NODE *hypo);
static void put_hypo_wname(NODE *hypo);

/**********************************************************************/
/********** ��ñ���Ǽ�ΰ�γ������          *************************/
/********** allocate memory for nextword data *************************/
/**********************************************************************/

/** 
 * <JA>
 * ��ñ�������Ǽ���뤿��� NEXTWORD ����˥�������դ��롥
 * 
 * @param maxlen [out] ��Ǽ��ǽ��ñ���
 * @param root [out] ����դ��ΰ����Ƭ�ؤΥݥ���
 * 
 * @return ����դ���줿��ñ������ؤΥݥ��󥿤��֤���
 * </JA>
 * <EN>
 * Allocate NEXTWORD array for storing list of candidate next words
 * 
 * @param maxlen [out] maximum number of words that can be stored
 * @param root [out] pointer to the top address of allocated data
 * 
 * @return the newly allocated pointer of NEXTWORD array.
 * </EN>
 */
static NEXTWORD **
nw_malloc(int *maxlen, NEXTWORD **root)
{
  NEXTWORD *nwtmp;
  NEXTWORD **nw;
  int i;
  int max;

  /* the initial maximum number of nextwords is the size of vocabulary */
  max = winfo->num;
  
  nw = (NEXTWORD **)mymalloc(max * sizeof(NEXTWORD *));
  nwtmp = (NEXTWORD *)mymalloc(max * sizeof(NEXTWORD));
  for (i=0;i<max; i++) {
    nw[i] = &(nwtmp[i]);
  }
  *maxlen = max;
  *root = nwtmp;
  return nw;
}

/** 
 * <JA>
 * ��ñ�������Ǽ���� NEXTWORD ����Υ����������롥
 * 
 * @param nw [in] NEXTWORD����
 * @param root [in] nw_malloc() ��Ϳ����줿�ΰ���Ƭ�ؤΥݥ���
 * </JA>
 * <EN>
 * Free given NEXTWORD data.
 * 
 * @param nw [in] pointer to NEXTWORD structure to be free.
 * @param root [in] pointer to the top address of allocated data previously
 * returned by nw_malloc()
 * </EN>
 */
static void
nw_free(NEXTWORD **nw, NEXTWORD *root)
{
  free(root);
  free(nw);
}

#ifdef USE_DFA
/** 
 * <JA>
 * @brief  ��ñ������Ǽ�Ѥ� NEXTWORD ����Υ����ΰ��ĥ���롥
 *
 * ���δؿ���õ����˼�ñ����佸�礬��줿�ݤ˸ƤФ졤����ˤ��¿����
 * ��ñ�������Ǽ�Ǥ���褦 NEXTWORD ����Ȥ� realloc() ���롥
 * �ºݤˤϺǽ�� nw_malloc() �Ǽ����ñ���ʬ�����ΰ����ݤ��Ƥ��ꡤ
 * Julius �ǤϸƤФ�뤳�ȤϤʤ���Julian �Ǥϡ����硼�ȥݡ�����
 * �����å׽����ˤ����֤ΰۤʤ�����Ʊ����Ÿ������Τǡ�
 * ��ñ��������ÿ������礭�����Ȥ������ꤦ�롥
 * 
 * @param nwold [i/o] NEXTWORD����
 * @param maxlen [i/o] �����Ǽ�����Ǽ����ݥ��󥿡����ߤκ����Ǽ����
 * ����ƸƤӡ��ؿ���ǿ����˳��ݤ��줿�����ѹ�����롥
 * @param root [i/o] �ΰ���Ƭ�ؤΥݥ��󥿤��Ǽ���륢�ɥ쥹���ؿ����
 * �񤭴������롥
 * 
 * @return ��ĥ���줿�����ʼ�ñ������ؤΥݥ��󥿤��֤���
 * </JA>
 * <EN>
 * @brief  expand data area of NEXTWORD.
 *
 * In DFA mode, the number of nextwords can exceed the vocabulary size when
 * more than one DFA states are expanded by short-pause skipping.
 * In such case, the nextword data area should expanded here.
 * 
 * @param nwold [i/o] NEXTWORD array
 * @param maxlen [i/o] pointer to the maximum number of words that can be
 * stored.  The current number should be stored before calling this function,
 * and the resulting new number will be stored within this function.
 * @param root [i/o] address to the pointer of the allocated data.  The
 * value will be updated by reallocation in this function.
 * 
 * @return the newlly re-allocated pointer of NEXTWORD array.
 * </EN>
 */
static NEXTWORD **
nw_expand(NEXTWORD **nwold, int *maxlen, NEXTWORD **root)
{
  NEXTWORD *nwtmp;
  NEXTWORD **nw;
  int i;
  int nwmaxlen;

  nwmaxlen = *maxlen + winfo->num;

  nwtmp = (NEXTWORD *)myrealloc(*root, nwmaxlen * sizeof(NEXTWORD));
  nw = (NEXTWORD **)myrealloc(nwold, nwmaxlen * sizeof(NEXTWORD *));
  nw[0] = nwtmp;
  for (i=1;i<nwmaxlen; i++) {
    nw[i] = &(nwtmp[i]);
  }
  *maxlen = nwmaxlen;
  *root = nwtmp;
  return nw;
}
#endif


/**********************************************************************/
/********** ���⥹���å������         ********************************/
/********** Hypothesis stack operation ********************************/
/**********************************************************************/

/* ʸ���⥹���å��� double linked list �Ǥ��롥�ޤ���˥������礬�ݤ���� */
/* The hypothesis stack is double-linked list, and holds entries in score sorted order */

/** 
 * <JA>
 * �����å��ȥåפκ��ಾ�����Ф���
 * 
 * @param start [i/o] �����å�����Ƭ�Ρ��ɤؤΥݥ��󥿡ʽ񴹤������礢���
 * @param stacknum [i/o] ���ߤΥ����å��������ؤΥݥ��󥿡ʽ񤭴��������
 * 
 * @return ���Ф������ಾ��Υݥ��󥿤��֤���
 * </JA>
 * <EN>
 * Pop the best hypothesis from stack.
 * 
 * @param start [i/o] pointer to stack top node (will be modified if necessary)
 * @param stacknum [i/o] pointer to the current stack size (will be modified
 * if necessary)
 * 
 * @return pointer to the popped hypothesis.
 * </EN>
 */
static NODE *
get_best_from_stack(NODE **start, int *stacknum)
{
  NODE *tmp;

  /* return top */
  tmp=(*start);
  if ((*start)!=NULL) {		/* delete it from stack */
    (*start)=(*start)->next;
    if ((*start)!=NULL) (*start)->prev=NULL;
    (*stacknum)--;
    return(tmp);
  }
  else {
    return(NULL);
  }
}

/** 
 * <JA>
 * ���벾�⤬�����å���˳�Ǽ����뤫�ɤ��������å����롥
 * 
 * @param new [in] �����å����벾��
 * @param bottom [in] �����å�����Ρ��ɤؤΥݥ���
 * @param stacknum [in] �����å��˸��߳�Ǽ����Ƥ���Ρ��ɿ��ؤΥݥ���
 * @param stacksize [in] �����å��ΥΡ��ɿ��ξ��
 * 
 * @return �����å��Υ���������¤�ã���Ƥ��ʤ���������������Ρ��ɤ���
 * �褱��г�Ǽ�����Ȥ��� 0 �򡤤���ʳ��Ǥ���г�Ǽ�Ǥ��ʤ��Ȥ��� -1 ��
 * �֤���
 * </JA>
 * <EN>
 * Check whether a hypothesis will be stored in the stack.
 * 
 * @param new [in] hypothesis to be checked
 * @param bottom [in] pointer to stack bottom node
 * @param stacknum [in] pointer to current stack size
 * @param stacksize [in] pointer to maximum stack size limit
 * 
 * @return 0 if it will be stored in the stack (in case @a stacknum <
 * @a stacksize or the score of @a new is better than bottom.  Otherwise
 * returns -1, which means it can not be pushed to the stack.
 * </EN>
 */
static int
can_put_to_stack(NODE *new, NODE **bottom, int *stacknum, int stacksize)
{
  /* stack size check */
  if ((*stacknum + 1) > stacksize && (*bottom)->score >= new->score) {
    /* new node is below the bottom: discard it */
    return(-1);
  }
  return(0);
}

/** 
 * <JA>
 * �����å��˿����ʲ�����Ǽ���롥
 * �����å���Υ���������θ�������֤���������롥
 * ��Ǽ�Ǥ��ʤ��ä���硤Ϳ����줿����� free_node() ����롥
 * 
 * @param new [in] �����å����벾��
 * @param start [i/o] �����å��ΥȥåץΡ��ɤؤΥݥ���
 * @param bottom [i/o] �����å�����Ρ��ɤؤΥݥ���
 * @param stacknum [i/o] �����å��˸��߳�Ǽ����Ƥ���Ρ��ɿ��ؤΥݥ���
 * @param stacksize [in] �����å��ΥΡ��ɿ��ξ��
 * 
 * @return ��Ǽ�Ǥ���� 0 �򡤤Ǥ��ʤ��ä����� -1 ���֤���
 * </JA>
 * <EN>
 * Push a new hypothesis into the stack, keeping score order.
 * If not succeeded, the given new hypothesis will be freed by free_node().
 * 
 * @param new [in] hypothesis to be checked
 * @param start [i/o] pointer to stack top node
 * @param bottom [i/o] pointer to stack bottom node
 * @param stacknum [i/o] pointer to current stack size
 * @param stacksize [in] pointer to maximum stack size limit
 * 
 * @return 0 if succeded, or -1 if failed to push because of number
 * limitation or too low score.
 * </EN>
 */
static int
put_to_stack(NODE *new, NODE **start, NODE **bottom, int *stacknum, int stacksize)
{
  NODE *tmp;

  /* stack size is going to increase... */
  (*stacknum)++;

  /* stack size check */
  if ((*stacknum)>stacksize) {
    /* stack size overflow */
    (*stacknum)--;
    if ((*bottom)->score < new->score) {
      /* new node will be inserted in the stack: free the bottom */
      tmp=(*bottom);
      (*bottom)->prev->next=NULL;
      (*bottom)=(*bottom)->prev;
      free_node(tmp);
    } else {
      /* new node is below the bottom: discard it */
      free_node(new);
      return(-1);
    }
  }

  /* insert new node on edge */
  if ((*start)==NULL) {		/* no node in stack */
    /* new node is the only node */
    (*start)=new;
    (*bottom)=new;
    new->next=NULL;
    new->prev=NULL;
    return(0);
  }
  if ((*start)->score <= new->score) {
    /* insert on the top */
    new->next = (*start);
    new->next->prev = new;
    (*start)=new;
    new->prev=NULL;
    return(0);
  }
  if ((*bottom)->score >= new->score) {
    /* insert on the bottom */
    new->prev = (*bottom);
    new->prev->next = new;
    (*bottom)=new;
    new->next=NULL;
    return(0);
  }
    
  /* now the new node is between (*start) and (*bottom) */
  if (((*start)->score + (*bottom)->score) / 2 > new->score) {
    /* search from bottom */
    tmp=(*bottom);
    while(tmp->score < new->score) tmp=tmp->prev;
    new->prev=tmp;
    new->next=tmp->next;
    tmp->next->prev=new;
    tmp->next=new;
  } else {
    /* search from start */
    tmp=(*start);
    while(tmp->score > new->score) tmp=tmp->next;
    new->next=tmp;
    new->prev=tmp->prev;
    tmp->prev->next=new;
    tmp->prev=new;
  }
  return(0);
}

/** 
 * <JA>
 * �����å�����Ȥ����ƽ��Ϥ��롥�����å�����Ȥϼ����롥(�ǥХå���)
 * 
 * @param start [i/o] �����å��ΥȥåץΡ��ɤؤΥݥ���
 * @param stacknum [i/o] �����å��˸��߳�Ǽ����Ƥ���Ρ��ɿ��ؤΥݥ���
 * </JA>
 * <EN>
 * Output all nodes in the stack. All nodes will be lost (for debug).
 * 
 * @param start [i/o] pointer to stack top node
 * @param stacknum [i/o] pointer to current stack size
 * </EN>
 */
static void
put_all_in_stack(NODE **start, int *stacknum)
{
  NODE *ntmp;
  
  j_printf("stack remains::\n");
  while ((ntmp = get_best_from_stack(start, stacknum)) != NULL) {
    j_printf("%3d: s=%f ",*stacknum, ntmp->score);
    put_hypo_woutput(ntmp);
    free_node(ntmp);
  }
}

/** 
 * <JA>
 * �����å�����������������롥
 * 
 * @param start [i/o] �����å��ΥȥåץΡ���
 * </JA>
 * <EN>
 * Free all nodes in a stack.
 * 
 * @param start [i/o] stack top node
 * </EN>
 */
static void
free_all_nodes(NODE *start)
{
  NODE *tmp;
  NODE *next;

  tmp=start;
  while(tmp) {
    next=tmp->next;
    free_node(tmp);
    tmp=next;
  }
}


#ifdef CONFIDENCE_MEASURE

/**********************************************************************/
/********** ñ�쿮���٤η׻� ******************************************/
/********** Confidence scoring ****************************************/
/**********************************************************************/
/* scoring using multiple cmalpha value will be enabled by CM_MULTIPLE_ALPHA */

#ifdef CM_MULTIPLE_ALPHA
static LOGPROB *cmsumlist = NULL; ///< Sum of cm score for each alpha coef.
#endif


#ifdef CM_SEARCH
/**************************************************************/
/**** CM computation method 1(default):                  ******/
/****   - Use local posterior probabilities while search ******/
/****   - Obtain CM at hypothesis expansion time         ******/
/**************************************************************/

static LOGPROB cm_tmpbestscore;	///< Temporal best score for summing up scores
#ifndef CM_MULTIPLE_ALPHA
static LOGPROB cm_tmpsum;	///< Sum of CM score
#endif

/* local stack for CM */
static int l_stacksize;		///< Local stack size for CM
static int l_stacknum;		///< Num of hypo. in local stack for CM
static NODE *l_start = NULL;	///< Top node of local stack for CM
static NODE *l_bottom = NULL;	///< bottom node of local stack for CM

/** 
 * <JA>
 * CM�׻��ѤΥѥ�᡼���������������2�ѥ����ϻ��˸ƤӽФ�����
 * 
 * @param winfo [in] ñ�켭��
 * </JA>
 * <EN>
 * Initialize parameters for confidence scoring (will be called at
 * each startup of 2nd pass)
 * 
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
cm_init(WORD_INFO *winfo)
{
  l_stacksize = winfo->num;
  l_start = l_bottom = NULL;
  l_stacknum = 0;
#ifdef CM_MULTIPLE_ALPHA
  if (cmsumlist == NULL) {	/* allocate if not yet */
    cmsumlist = (LOGPROB *)mymalloc(sizeof(LOGPROB) * cm_alpha_num);
  }
#endif    
}

/** 
 * <JA>
 * CM�׻��Τ���˥����륹���å���Ÿ���������Ū����¸���롥
 * 
 * @param new [in] Ÿ������
 * </JA>
 * <EN>
 * Store an expanded hypothesis to the local stack for later CM scoring
 * 
 * @param new [in] expanded hypothesis
 * </EN>
 */
static void
cm_store(NODE *new)
{
  /* store the generated hypo into local stack */
  put_to_stack(new, &l_start, &l_bottom, &l_stacknum, l_stacksize);
}

/** 
 * <JA>
 * CM�׻��Τ���˥����륹���å���β���νи���Ψ�ι�פ���롥
 * 
 * </JA>
 * <EN>
 * Compute sum of probabilities for hypotheses in the local stack for
 * CM scoring.
 * 
 * </EN>
 */
static void
cm_sum_score()
{
  NODE *node;
  LOGPROB sum;
#ifdef CM_MULTIPLE_ALPHA
  LOGPROB cm_alpha;
  int j;
#endif

  if (l_start == NULL) return;	/* no hypo */
  cm_tmpbestscore = l_start->score; /* best hypo is at the top of the stack */

#ifdef CM_MULTIPLE_ALPHA
  for (j = 0, cm_alpha = cm_alpha_bgn; cm_alpha <= cm_alpha_end; cm_alpha += cm_alpha_step) {
#endif
    sum = 0.0;
    for(node = l_start; node; node = node->next) {
      sum += pow(10, cm_alpha * (node->score - cm_tmpbestscore));
    }
#ifdef CM_MULTIPLE_ALPHA
    cmsumlist[j++] = sum;	/* store sums for each alpha coef. */
#else
    cm_tmpsum = sum;		/* store sum */
#endif

#ifdef CM_MULTIPLE_ALPHA
  }
#endif
}

/* compute CM scores of the new word in the local stack from the sum above */
/** 
 * <JA>
 * Ÿ�����줿����ʸ����ˤĤ��ơ�����Ÿ��ñ��ο����٤򡤻����Ψ��
 * ��Ť��Ʒ׻����롥
 * 
 * @param node [i/o] Ÿ�����줿����ʸ����
 * </JA>
 * <EN>
 * Compute confidence score of a new word at the end of the given hypothesis,
 * based on the local posterior probabilities.
 * 
 * @param node [i/o] expanded hypothesis
 * </EN>
 */
static void
cm_set_score(NODE *node)
{
#ifdef CM_MULTIPLE_ALPHA
  int j;
  LOGPROB cm_alpha;
#endif

#ifdef CM_MULTIPLE_ALPHA
  for (j = 0, cm_alpha = cm_alpha_bgn; cm_alpha <= cm_alpha_end; cm_alpha += cm_alpha_step) {
    node->cmscore[node->seqnum-1][j] = pow(10, cm_alpha * (node->score - cm_tmpbestscore)) / cmsumlist[j];
    j++;
  }
#else
  node->cmscore[node->seqnum-1] = pow(10, cm_alpha * (node->score - cm_tmpbestscore)) / cm_tmpsum;
#endif
}

/** 
 * <JA>
 * CM�׻��ѤΥ����륹���å����鲾�����Ф���
 * 
 * @return ���Ф��줿ʸ������֤���
 * </JA>
 * <EN>
 * Pop one node from local stack for confidence scoring.
 * 
 * @return the popped hypothesis.
 * </EN>
 */
static NODE *
cm_get_node()
{
  return(get_best_from_stack(&l_start, &l_stacknum));
}

#endif /* CM_SEARCH */

#ifdef CM_NBEST
/*****************************************************************/
/**** CM computation method 2: conventional N-best scoring *******/
/**** NOTE: enough N-best should be computed (-n 10 ~ -n 100) ****/
/*****************************************************************/

/* CM of each word is calculated after all search had finished */

static LOGPROB *sentcm = NULL;	///< Confidence score of each sentence
static LOGPROB *wordcm = NULL;	///< Confidence score of each word voted from @a sentcm
static int sentnum;		///< Allocated length of @a sentcm

/** 
 * <JA>
 * �����å���ˤ���ʸ���䤫��ñ�쿮���٤�׻����롥
 * 
 * @param start [in] �����å�����Ƭ�Ρ���
 * @param stacknum [in] �����å�������
 * </JA>
 * <EN>
 * Compute confidence scores from N-best sentence candidates in the
 * given stack.
 * 
 * @param start [in] stack top node 
 * @param stacknum [in] current stack size
 * </EN>
 */
static void
cm_compute_from_nbest(NODE *start, int stacknum)
{
  NODE *node;
  LOGPROB bestscore, sum, s;
  WORD_ID w;
  int i;
#ifdef CM_MULTIPLE_ALPHA
  LOGPROB cm_alpha;
  int j;
#endif

  /* prepare buffer */
#ifdef CM_MULTIPLE_ALPHA
  if (cmsumlist == NULL) {
    cmsumlist = (LOGPROB *)mymalloc(sizeof(LOGPROB) * cm_alpha_num);
  }
#endif    
  if (sentcm == NULL) {		/* not allocated yet */
    sentcm = (LOGPROB *)mymalloc(sizeof(LOGPROB)*stacknum);
    sentnum = stacknum;
  } else if (sentnum < stacknum) { /* need expanded */
    sentcm = (LOGPROB *)myrealloc(sentcm, sizeof(LOGPROB)*stacknum);
    sentnum = stacknum;
  }
  if (wordcm == NULL) {
    wordcm = (LOGPROB *)mymalloc(sizeof(LOGPROB) * winfo->num);
  }
#ifdef CM_MULTIPLE_ALPHA
  for (j = 0, cm_alpha = cm_alpha_bgn; cm_alpha <= cm_alpha_end; cm_alpha += cm_alpha_step) {
#endif
    /* clear whole word cm buffer */
    for(w=0;w<winfo->num;w++) {
      wordcm[w] = 0.0;
    }
    /* get best score */
    bestscore = start->score;
    /* compute sum score of all hypothesis */
    sum = 0.0;
    for (node = start; node != NULL; node = node->next) {
      sum += pow(10, cm_alpha * (node->score - bestscore));
    }
    /* compute sentence posteriori probabilities */
    i = 0;
    for (node = start; node != NULL; node = node->next) {
      sentcm[i] = pow(10, cm_alpha * (node->score - bestscore)) / sum;
      i++;
    }
    /* compute word posteriori probabilities */
    i = 0;
    for (node = start; node != NULL; node = node->next) {
      for (w=0;w<node->seqnum;w++) {
	wordcm[node->seq[w]] += sentcm[i];
      }
      i++;
    }
    /* store the probabilities to node */
    for (node = start; node != NULL; node = node->next) {
      for (w=0;w<node->seqnum;w++) {
#ifdef CM_MULTIPLE_ALPHA
	node->cmscore[w][j] = wordcm[node->seq[w]];
#else	
	node->cmscore[w] = wordcm[node->seq[w]];
#endif
      }
    }
#ifdef CM_MULTIPLE_ALPHA
    j++;
  }
#endif
}

#endif /* CM_NBEST */

#endif /* CONFIDENCE_MEASURE */


/**********************************************************************/
/********** Enveloped best-first search *******************************/
/**********************************************************************/

/*
 * 1. Word envelope
 *
 * ���β���ӡ�����������: Ÿ�����Ȥʤä�����ο��򤽤β���Ĺ(ñ���)
 * ���Ȥ˥�����Ȥ��롥�������ۤ����餽����û������ϰʸ�Ÿ�����ʤ���
 * 
 * Introduce a kind of beam width to search tree: count the number of
 * popped hypotheses per the depth of the hypotheses, and when a count
 * in a certain depth reaches the threshold, all hypotheses shorter than
 * the depth will be dropped from candidates.
 *
 */

static int hypo_len_count[MAXSEQNUM+1];	///< Count of popped hypothesis per each length
static int maximum_filled_length; ///< Current least beam-filled depth

/** 
 * <JA>
 * Word envelope �Ѥ˥����󥿤��������롥
 * 
 * </JA>
 * <EN>
 * Initialize counters fro word enveloping.
 * 
 * </EN>
 */
static void
wb_init()
{
  int i;
  for(i=0;i<=MAXSEQNUM;i++) hypo_len_count[i] = 0;
  maximum_filled_length = -1;
}

/** 
 * <JA>
 * Word envelope �򻲾Ȥ��ơ�Ϳ����줿�����Ÿ�����Ƥ褤���ɤ������֤���
 * �ޤ���Word envelope �Υ����󥿤򹹿����롥
 * 
 * @param now [in] ������Ÿ�����褦�Ȥ��Ƥ��벾��
 * 
 * @return Ÿ����ǽ�ʥӡ��५����Ȥ���¤�ã���Ƥ��ʤ��ˤʤ� TRUE,
 * Ÿ���Բ�ǽ�ʥ�����Ȥ�ã���Ƥ���ˤʤ� FALSE ���֤���
 * </JA>
 * <EN>
 * Consult the current word envelope to check if word expansion from
 * the hypothesis node is allowed or not.  Also increment the counter
 * of word envelope if needed.
 * 
 * @param now [in] popped hypothesis
 * 
 * @return TRUE if word expansion is allowed (in case word envelope count
 * of the corresponding hypothesis depth does not reach the limit), or
 * FALSE if already prohibited.
 * </EN>
 */
static boolean
wb_ok(NODE *now)
{
  if (now->seqnum <= maximum_filled_length) {
    /* word expansion is not allowed because a word expansion count
       at deeper level has already reached the limit */
    if (debug2_flag) {
      j_printf("popped but pruned by word envelope: ");
      put_hypo_woutput(now);
    }
    return FALSE;
  } else {
    /* word expansion is possible.  Increment the word expansion count
       of the given depth */
    hypo_len_count[now->seqnum]++;
    if (hypo_len_count[now->seqnum] > enveloped_bestfirst_width) {
      /* the word expansion count of this level has reached the limit, so
	 set the beam-filled depth to this level to inhibit further
	 expansion of shorter hypotheses. */
      if (maximum_filled_length < now->seqnum) maximum_filled_length = now->seqnum;
    }
    return TRUE;
  }
}

#ifdef SCAN_BEAM
/*
 * 2. Score envelope
 *
 * Viterbi�׻��̤κ︺: ���ϥե졼�ऴ�Ȥκ������� (score envelope) ��
 * ������ˤ錄�äƵ�Ͽ���Ƥ�������������������ٷ׻����ˡ����� envelope
 * ����������ʾ她�����������Ȥ���Viterbi �ѥ��α黻�����Ǥ��롥
 *
 * �����Ǥϡ����Ф������⤫��ե졼�ऴ�Ȥ� score envelope �򹹿�����
 * ��ʬ�����Ҥ���Ƥ��롥Envelope ���θ���� Viterbi �׻��μºݤ�
 * scan_word() �򻲾ȤΤ��ȡ�
 *
 * Reduce computation cost of hypothesis Viterbi processing by setting a
 * "score envelope" that holds the maximum scores at every frames
 * throughout the expanded hypotheses.  When calculating Viterbi path
 * on HMM trellis for updating score of popped hypothesis, Viterbi paths
 * that goes below a certain range from the score envelope are dropped.
 *
 * These functions are for updating the score envelope according to the
 * popped hypothesis.  For actual Viterbi process with score envelope, 
 * see scan_word().
 *
 */

/* the score envelope is kept in framemaxscore[0..framelen-1] */

/** 
 * <JA>
 * Score envelope ���������롥��2�ѥ��γ��ϻ��˸ƤФ�롥
 * 
 * @param framenum [in] ���ϥե졼��Ĺ
 * </JA>
 * <EN>
 * Initialize score envelope.  This will be called once at the beginning
 * of 2nd pass.
 * 
 * @param framenum [in] input frame length
 * </EN>
 */
static void
envl_init(int framenum)
{
  int i;
  for(i=0;i<framenum;i++) framemaxscore[i] = LOG_ZERO;
}

/** 
 * <JA>
 * ��������������������� score envelope �򹹿����롥
 * 
 * @param n [in] ����
 * @param framenum [in] ���ϥե졼��Ĺ
 * </JA>
 * <EN>
 * Update the score envelope using forward score of the given hypothesis.
 * 
 * @param n [in] hypothesis
 * @param framenum [in] input frame length
 * </EN>
 */
static void
envl_update(NODE *n, int framenum)
{
  int t;
  for(t=framenum-1;t>=0;t--) {
    if (framemaxscore[t] < n->g[t]) framemaxscore[t] = n->g[t];
  }
}
#endif /* SCAN_BEAM */


#ifdef USE_DFA
/**
 * Dummy NEXTWORD data for short-pause insertion handling.
 * 
 */
static NEXTWORD fornoise;
#endif


#ifdef SP_BREAK_CURRENT_FRAME
/**********************************************************************/
/********** Short pause segmentation **********************************/
/**********************************************************************/

/** 
 * <JA>
 * ǧ����̤��顤�������϶�֤�ǧ���򳫻Ϥ���ݤν��ñ������򥻥åȤ��롥
 * Ʃ��줪��Ӳ���ν�ʣ���θ���ƽ��ñ�����򤬷��ꤵ��롥
 * 
 * @param hypo [in] ���ߤ����϶�֤�ǧ����̤Ȥ��Ƥ�ʸ����
 * </JA>
 * <EN>
 * Set the previous word context for the recognition of the next input
 * segment from the current recognition result.  The initial context word
 * will be chosen from the current recognition result skipping transparent
 * word and multiplied words.
 * 
 * @param hypo [in] sentence candidate as a recognition result of current
 * input segment
 * </EN>
 */
void
sp_segment_set_last_nword(NODE *hypo)
{
  int i;
  WORD_ID w;
  for(i=0;i<hypo->seqnum;i++) {
    w = hypo->seq[i];
    if (w != sp_break_last_word
       && !is_sil(w, winfo, hmminfo)
       && !winfo->is_transparent[w]
       ) {
      sp_break_last_nword = w;
      break;
    }
  }
#ifdef SP_BREAK_DEBUG
  printf("sp_break_last_nword=%d[%s]\n", sp_break_last_nword, winfo->woutput[sp_break_last_nword]);
#endif
}
#endif


/**********************************************************************/
/********* Debug output of hypothesis while search ********************/
/**********************************************************************/

/* �̾�ν��ϴؿ��� result_*.c �ˤ��� */
/* normal output functions are in result_*.c */

static HTK_Param *tparam;	///< Temporal parameter for forced alignment

/* output word sequence of a hypothesis */
/** 
 * <JA>
 * �ǥХå��Ѥ˲����ñ�����ɽ�����롥
 * 
 * @param hypo [in] ����
 * </JA>
 * <EN>
 * Output word sequence of a hypothesis for debug.
 * 
 * @param hypo [in] hypothesis
 * </EN>
 */
static void
put_hypo_woutput(NODE *hypo)
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
 * �ǥХå��Ѥ˲����ñ��N-gram����ȥ�̾��Julian�Ǥϥ��ƥ����ֹ�ˤ���Ϥ��롥
 * 
 * @param hypo [in] ����
 * </JA>
 * <EN>
 * Output N-gram entries (or DFA category IDs) of a hypothesis for debug.
 * 
 * @param hypo [in] hypothesis
 * </EN>
 */
static void
put_hypo_wname(NODE *hypo)
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

/**********************************************************************/
/******** Output top 'ncan' hypotheses in a stack and free all ********/
/**********************************************************************/

/** 
 * <JA>
 * �����å������̤β������Ф���ǧ����̤Ȥ��ƽ��Ϥ��롥����ˡ�
 * �����å��˳�Ǽ����Ƥ������Ƥβ����������롥
 *
 * Julius/Julian �Ǥϡ���2�ѥ��ˤ���������줿ʸ����ϡ����ä����̳�Ǽ��
 * �Υ����å��˳�Ǽ����롥õ����λ��"-n" �ο�����ʸ���䤬���Ĥ��뤫��
 * õ�������Ǥ����ˤθ塤���Ū������줿ʸ������椫����N��
 * ��"-output" �ǻ��ꤵ�줿���ˤβ������Ϥ��롥
 * 
 * @param r_start [i/o] ��̳�Ǽ�ѥ����å�����Ƭ�Ρ��ɤؤΥݥ���
 * @param r_bottom [i/o] ��̳�Ǽ�ѥ����å�����Ρ��ɤؤΥݥ���
 * @param r_stacknum [i/o] �����å��˳�Ǽ����Ƥ���Ρ��ɿ��ؤΥݥ���
 * @param ncan [in] ���Ϥ����̲����
 * @param winfo [in] ñ�켭��
 * </JA>
 * <EN>
 * Output top N-best hypotheses in a stack as a recognition result, and
 * free all hypotheses.
 *
 * In Julius/Julian, the sentence candidates found at the 2nd pass will
 * be pushed to the "result stack" instead of immediate output.  After
 * recognition is over (in case the number of found sentences reaches
 * the number specified by "-n", or search has been terminated in other
 * reason), the top N sentence candidates in the stack will be output
 * as a final result (where N should be specified by "-output").  After
 * then, all the hypotheses in the stack will be freed.
 * 
 * @param r_start [i/o] pointer to the top node of the result stack
 * @param r_bottom [i/o] pointer to the bottom node of the result stack
 * @param r_stacknum [i/o] number of candidates in the current result stack
 * @param ncan [in] number of sentence candidates to be output
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
result_reorder_and_output(NODE **r_start, NODE **r_bottom, int *r_stacknum, int ncan, WORD_INFO *winfo)
{
  NODE *now;
  int num;

#ifdef CM_NBEST 
  /* compute CM from the N-best sentence candidates */
  cm_compute_from_nbest(*r_start, *r_stacknum);
#endif
  num = 0;
  result_pass2_begin();
  while ((now = get_best_from_stack(r_start,r_stacknum)) != NULL && num < ncan) {
    num++;
    /* output result */
    result_pass2(now, num, winfo);
#ifdef VISUALIZE
    visual2_best(now, winfo);
#endif
#ifdef SP_BREAK_CURRENT_FRAME
    /* set the last context-aware word for next recognition */
    if (sp_break_last_nword_allow_override && num == 1) sp_segment_set_last_nword(now);
#endif
    /* do forced alignment if needed */
    if (align_result_word_flag) word_rev_align(now->seq, now->seqnum, tparam);
    if (align_result_phoneme_flag) phoneme_rev_align(now->seq, now->seqnum, tparam);
    if (align_result_state_flag) state_rev_align(now->seq, now->seqnum, tparam);
    free_node(now);
  }
  result_pass2_end();
  /* free the rest */
  if (now != NULL) free_node(now);
  free_all_nodes(*r_start);
}  


/**********************************************************************/
/********* Main stack decoding function *******************************/
/**********************************************************************/

/* subfunctions called by this main function:
     for word prediction: ngram_decode.c or dfa_decode.c
     for hypothesis expansion and forward viterbi: search_bestfirst_v{1,2}.c
                                  (v1: for efficiency   v2: for accuracy)
*/

/** 
 * <JA>
 * ��2õ���ѥ��Ǥ��륹���å��ǥ����ǥ��󥰤�Ԥ��ᥤ��ؿ�
 * 
 * @param param [in] ���ϥѥ�᡼���٥��ȥ���
 * @param backtrellis [in] ��1�ѥ�������줿ñ��ȥ�ꥹ
 * @param backmax [in] ��1�ѥ�������줿���Ѳ��⥹�����κ�����
 * @param stacksize [in] ���⥹���å��Υ�����
 * @param ncan [in] ����٤�ʸ�����
 * @param maxhypo [in] õ������ǰ����Ÿ��������
 * @param cate_bgn [in] Ÿ���оݤȤ��٤����ƥ���γ����ֹ��Julius�Ǥ�̵���
 * @param cate_num [in] Ÿ���оݤȤ��٤����ƥ���ο���Julius�Ǥ�̵���
 * </JA>
 * <EN>
 * Main function to perform stack decoding of the 2nd search pass.
 * 
 * @param param [in] input parameter vector
 * @param backtrellis [in] backward word trellis obtained at the 1st pass
 * @param backmax [in] maximum score in the word trellis
 * @param stacksize [in] maximum size of hypothesis stack in the search
 * @param ncan [in] num of hypothesis to be found in the search
 * @param maxhypo [in] threshold num of hypothesis expansion to be treated as "search overflow"
 * @param cate_bgn [in] category id to allow word expansion from (ignored in Julius)
 * @param cate_num [in] num of category to allow word expansion from (ignored in Julius)
 * </EN>
 */
void
wchmm_fbs(HTK_Param *param, BACKTRELLIS *backtrellis, LOGPROB backmax, int stacksize, int ncan, int maxhypo, int cate_bgn, int cate_num)
{
  /* ʸ���⥹���å� */
  /* hypothesis stack (double-linked list) */
  int stacknum;			/* current stack size */
  NODE *start = NULL;		/* top node */
  NODE *bottom = NULL;		/* bottom node */

  /* ǧ����̳�Ǽ�����å�(��̤Ϥ����ؤ��ä��󽸤����) */
  /* result sentence stack (found results will be stored here and then re-ordered) */
  int r_stacksize = ncan;
  int r_stacknum;
  NODE *r_start = NULL;
  NODE *r_bottom = NULL;

  /* ��Υ����󥿡� */
  /* monitoring counters */
  int popctr = 0;		/* num of popped hypotheses from stack */
  int genectr = 0;		/* num of generated hypotheses */
  int pushctr = 0;		/* num of hypotheses actually pushed to stack */
  int finishnum = 0;		/* num of found sentence hypothesis */

  /* ������ꥢ */
  /* work area */
  NEXTWORD **nextword, *nwroot;	/* buffer to store predicted words */
  int maxnwnum;			/* current allocated number of words in nextword */
  int nwnum;			/* current number of words in nextword */
  NODE *now, *new;		/* popped current hypo., expanded new hypo. */
#ifdef USE_DFA
  NODE *now_noise;	       /* for inserting/deleting noise word */
  boolean now_noise_calced;
  int t;
#endif
  int w,i,j;
  LOGPROB last_score = LOG_ZERO;
#ifdef GRAPHOUT
  LOGPROB prev_score;
  WordGraph *wordgraph_root = NULL;
  boolean merged_p;
#ifdef GRAPHOUT_DYNAMIC
  int dynamic_merged_num = 0;
  WordGraph *wtmp;
#endif
#ifdef GRAPHOUT_SEARCH
  int terminate_search_num = 0;
#endif
#endif

  /* copy parameter pointer beforehand for forced alignment */
  if (align_result_word_flag || align_result_phoneme_flag || align_result_state_flag) tparam = param;

#ifdef USE_DFA
  if (debug2_flag) j_printf("limiting category: %d-%d\n", cate_bgn, cate_bgn + cate_num-1);
#endif

  /*
   * �����
   * Initialize
   */
  peseqlen = backtrellis->framelen; /* (just for quick access) */
  /* ͽ¬ñ���Ǽ�ΰ����� */
  /* malloc area for word prediction */
  nextword = nw_malloc(&maxnwnum, &nwroot);
  /* �������������׻��Ѥ��ΰ����� */
  /* malloc are for forward viterbi (scan_word()) */
  malloc_wordtrellis();		/* scan_word���ΰ� */
  /* ���⥹���å������ */
  /* initialize hypothesis stack */
  start = bottom = NULL;
  stacknum = 0;
  /* ��̳�Ǽ�����å������ */
  /* initialize result stack */
  if (result_reorder_flag) {
    r_start = r_bottom = NULL;
    r_stacknum = 0;
  }
  
#ifdef CM_SEARCH
  /* initialize local stack */
  cm_init(winfo);
#endif
#ifdef SCAN_BEAM
  /* prepare and initialize score envelope */
  framemaxscore = (LOGPROB *)mymalloc(sizeof(LOGPROB)*peseqlen);
  envl_init(peseqlen);
#endif /* SCAN_BEAM */
  
  /* ����٥���õ���Ѥ�ñ��Ĺ��Ÿ���������󥿤����� */
  /* initialize counters for envelope search */
  if (enveloped_bestfirst_width >= 0) wb_init();

  if (verbose_flag) j_printf("samplenum=%d\n", peseqlen);
  if (result_reorder_flag) {
    if (debug2_flag) VERMES("getting %d candidates...\n",ncan);
  }

#ifdef VISUALIZE
  visual2_init(maxhypo);
#endif

  /* 
   * �������(1ñ�줫��ʤ�)����, ʸ���⥹���å��ˤ����
   * get a set of initial words from LM function and push them as initial
   * hypotheses
   */
  /* the first words will be stored in nextword[] */
#ifdef USE_NGRAM
  nwnum = ngram_firstwords(nextword, peseqlen, maxnwnum, winfo, backtrellis);
#else  /* USE_DFA */
  nwnum = dfa_firstwords(nextword, peseqlen, maxnwnum, dfa);
  /* ��줿�顢�Хåե������䤷�ƺƥ����� */
  /* If the number of nextwords can exceed the buffer size, expand the
     nextword data area */
  while (nwnum < 0) {
    nextword = nw_expand(nextword, &maxnwnum, &nwroot);
    nwnum = dfa_firstwords(nextword, peseqlen, maxnwnum, dfa);
  }
#endif

  if (debug2_flag) {
    j_printf("%d words in wordtrellis as first hypothesis\n", nwnum);
  }
  
  /* store them to stack */
  for (w = 0; w < nwnum; w++) {
#ifdef USE_DFA
    /* limit word hypothesis */
    if (! (winfo->wton[nextword[w]->id] >= cate_bgn && winfo->wton[nextword[w]->id] < cate_bgn + cate_num)) {
      continue;
    }
#endif
    /* generate new hypothesis */
    new = newnode();
    start_word(new,nextword[w],param,backtrellis);
#ifdef USE_DFA
    if (new->score <= LOG_ZERO) { /* not on trellis */
      free_node(new);
      continue;
    }
#endif
    genectr++;
#ifdef CM_SEARCH
    /* store the local hypothesis to temporal stack */
    cm_store(new);
#else 
    /* put to stack */
    if (put_to_stack(new, &start, &bottom, &stacknum, stacksize) != -1) {
#ifdef VISUALIZE
      visual2_next_word(new, NULL, popctr);
#endif
#ifdef GRAPHOUT
      new->prevgraph = NULL;
      new->lastcontext = NULL;
#endif
      pushctr++;
    }
#endif
  }
#ifdef CM_SEARCH
  /* compute score sum */
  cm_sum_score();
  /* compute CM and put the generated hypotheses to global stack */
  while ((new = cm_get_node()) != NULL) {
    cm_set_score(new);
#ifdef CM_SEARCH_LIMIT
    if (new->cmscore[new->seqnum-1] < cm_cut_thres
#ifdef CM_SEARCH_LIMIT_AFTER
	&& finishnum > 0
#endif
	) {
      free_node(new);
      continue;
    }
#endif /* CM_SEARCH_LIMIT */
    
    if (put_to_stack(new, &start, &bottom, &stacknum, stacksize) != -1) {
#ifdef VISUALIZE
      visual2_next_word(new, NULL, popctr);
#endif
#ifdef GRAPHOUT
      new->prevgraph = NULL;
      new->lastcontext = NULL;
#endif
      pushctr++;
    }
  }
#endif

  if (debug2_flag) {
    j_printf("%d pushed\n", pushctr);
  }
  
  /********************/
  /* main search loop */
  /********************/

  for (;;) {

    if (module_mode) {
      /* if terminate signal has been received, cancel this input */
      if (module_wants_terminate()) {
	j_printf("terminated\n");
	break;
      }
    }
    
    /* 
     * ���⥹���å�����Ǥ⥹�����ι⤤�������Ф�
     * pop the top hypothesis from stack
     */
#ifdef DEBUG
    VERMES("get one hypothesis\n");
#endif
    now = get_best_from_stack(&start,&stacknum);
    if (now == NULL) {  /* stack empty ---> õ����λ*/
      j_printf("stack empty, search terminate now\n");
      j_printf("%d sentences have found\n", finishnum);
      break;
    }
    /* (bogus score check) */
    if (now->score <= LOG_ZERO) {
      free_node(now);
      continue;
    }

#ifdef GRAPHOUT
    /* ñ�쥰����Ѥ� pop ����� f ������������¸ */
    prev_score = now->score;
#endif

    /* word envelope �����å� */
    /* consult word envelope */
    if (enveloped_bestfirst_width >= 0) {
      if (!wb_ok(now)) {
	/* ���β���Ĺ�ˤ�����Ÿ������������߷׿��ϴ������ͤ�ۤ��Ƥ��롥
	   ���Τ��ᡤ���β���ϼΤƤ롥*/
	/* the number of popped hypotheses at the length already
	   reaches its limit, so the current popped hypothesis should
	   be discarded here with no expansion */
	free_node(now);
	continue;
      }
    }
    
#ifdef CM_SEARCH_LIMIT_POP
    if (now->cmscore[now->seqnum-1] < cm_cut_thres_pop) {
      free_node(now);
      continue;
    }
#endif /* CM_SEARCH_LIMIT_POP */

    popctr++;

    /* (for debug) ���Ф�������Ȥ��Υ���������� */
    /*             output information of the popped hypothesis to stdout */
    if (debug2_flag) {
      j_printf("--- pop %d:\n", popctr);
      j_printf("  "); put_hypo_woutput(now);
      j_printf("  "); put_hypo_wname(now);
      j_printf("  %d words, f=%f, g=%f\n", now->seqnum, now->score, now->g[now->bestt]);
      j_printf("  last word on trellis: [%d-%d]\n", now->estimated_next_t + 1, now->bestt);
    }

#ifdef VISUALIZE
    visual2_popped(now, popctr);
#endif

#ifdef GRAPHOUT
#ifdef GRAPHOUT_DYNAMIC
    /* merge last word in popped hypo if possible */
    wtmp = wordgraph_check_merge(now->prevgraph, &wordgraph_root, now->seq[now->seqnum-1], &merged_p);
    if (wtmp != NULL) {		/* wtmp holds merged word */
      dynamic_merged_num++;
      if (now->prevgraph != NULL) {
	if (now->prevgraph->saved) {
	  j_error("ERROR: already saved??\n");
	}
	wordgraph_free(now->prevgraph);
      }
      now->prevgraph = wtmp;	/* change word to the merged one */

      if (now->lastcontext != NULL
	  && now->lastcontext != now->prevgraph	/* avoid self loop */
	  ) {
	wordgraph_check_and_add_leftword(now->lastcontext, now->prevgraph);
#ifdef GRAPHOUT_SEARCH_CONSIDER_RIGHT
	if (merged_p) {
	  if (wordgraph_check_and_add_rightword(now->prevgraph, now->lastcontext) == FALSE) {
	    merged_p = TRUE;
	  } else {
	    merged_p = FALSE;
	  }
	} else {
	  wordgraph_check_and_add_rightword(now->prevgraph, now->lastcontext);
	}
#else
	wordgraph_check_and_add_rightword(now->prevgraph, now->lastcontext);
#endif	  
      }
      /*printf("last word merged\n");*/
      /* previous still remains at memory here... (will be purged later) */
    } else {
      wordgraph_save(now->prevgraph, now->lastcontext, &wordgraph_root);
    }
#ifdef GRAPHOUT_SEARCH
    /* if recent hypotheses are included in the existing graph, terminate */
    if (merged_p && now->endflag == FALSE
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
	/* Do not apply search termination by graph merging
	   until the first sentence candidate is found. */
	&& (graphout_search_delay == FALSE || finishnum > 0)
#endif
	) {
      terminate_search_num++;
      free_node(now);
      continue;
    }
#endif
#else  /* ~GRAPHOUT_DYNAMIC */
    /* always save */
    wordgraph_save(now->prevgraph, now->lastcontext, &wordgraph_root);
#endif /* ~GRAPHOUT_DYNAMIC */
#endif /* GRAPHOUT */

    /* ���Ф�������Υ������򸵤� score envelope �򹹿� */
    /* update score envelope using the popped hypothesis */
    envl_update(now, peseqlen);

    /* 
     * ���Ф�������μ����ե饰������Ω�äƤ���С�
     * ���β����õ����λ�Ȥߤʤ�����̤Ȥ��ƽ��Ϥ��Ƽ��Υ롼�פء�
     *
     * If the popped hypothesis already reached to the end, 
     * we can treat it as a recognition result.
     */
#ifdef DEBUG
    VERMES("endflag check\n");
#endif
    
    if (now->endflag) {
      if (debug2_flag) {
	j_printf("  This is a final sentence candidate\n");
      }
      /* quick, dirty hack */
      if (now->score == last_score) {
	free_node(now);
	continue;
      } else {
	last_score = now->score;
      }
      
      finishnum++;
      if (debug2_flag) {
	j_printf("  %d-th sentence found\n", finishnum);
      }

      if (result_reorder_flag) {
	/* ������β��⤬����줿���ȥ������ǥ����Ȥ��뤿�ᡤ
	   ���Ū���̤Υ����å��˳�Ǽ���Ƥ��� */
	/* store the result to result stack
	   after search is finished, they will be re-ordered and output */
	put_to_stack(now, &r_start, &r_bottom, &r_stacknum, r_stacksize);
	/* �������ʸ���⤬����줿�ʤ�õ����λ���� */
	/* finish search if specified number of results are found */
	if (finishnum >= ncan) {
	  if (debug2_flag) VERMES("%d\n",finishnum);
	  break;
	} else {
	  if (debug2_flag) VERMES("%d..", finishnum);
	  continue;
	}
      } else {			/* result_reorder �Ǥʤ����� */
	/* ��̤ϸ��Ĥ��꼡�褹���˽��� */
	/* result are directly output as soon as found */
	if (finishnum == 1) result_pass2_begin(); /* first time */
	result_pass2(now, finishnum, winfo);
#ifdef VISUALIZE
	visual2_best(now, winfo);
#endif
#ifdef SP_BREAK_CURRENT_FRAME
	/* ���硼�ȥݡ����������ơ��������ϼ��Υ�������ǧ�����ϻ���
	   ñ����������¤����� */
	/* set the last context-aware word for next recognition */
	if (sp_break_last_nword_allow_override && finishnum == 1) sp_segment_set_last_nword(now);
#endif
	/* ɬ�פǤ����, ǧ����̤��Ѥ������ϤΥ������ơ�������Ԥʤ� */
	/* do forced alignment with the result if needed */
	if (align_result_word_flag) word_rev_align(now->seq, now->seqnum, tparam);
	if (align_result_phoneme_flag) phoneme_rev_align(now->seq, now->seqnum, tparam);
	if (align_result_state_flag) state_rev_align(now->seq, now->seqnum, tparam);
	/* �����������Ƽ��β���� */
	/* free the hypothesis and continue to next */
	free_node(now);
	/* ����ǻ������ʸ���⤬����줿�ʤ�, ������õ����λ���� */
	/* finish search if specified number of results are found */
	if (finishnum >= ncan) {
	  result_pass2_end();
	  break;		/* end of search */
	}
	else continue;
      }
      
    } /* end of now->endflag */

    
    /* 
     * õ�����Ԥ򸡽Ф��롥
     * ������� maxhypo �ʾ�Ÿ�����줿��, �⤦����ʾ��õ�����ʤ�
     *
     * detecting search failure:
     * if the number of expanded hypotheses reaches maxhypo, giveup further search
     */
#ifdef DEBUG
    VERMES("loop end check\n");
#endif
    if (popctr >= maxhypo) {
      j_printf("num of hypotheses overflow\n");
      /* (for debug) õ�����Ի��ˡ������å��˻Ĥä�������Ǥ��Ф� */
      /* (for debug) output all hypothesis remaining in the stack */
      if (debug2_flag) put_all_in_stack(&start, &stacknum);
      free_node(now);
      break;			/* end of search */
    }
    /* ����Ĺ�������ͤ�ۤ����Ȥ������β�����˴����� */
    /* check hypothesis word length overflow */
    if (now->seqnum >= MAXSEQNUM) {
      j_printerr("sentence length exceeded ( > %d)\n",MAXSEQNUM);
      free_node(now);
      continue;
    }

#ifdef GRAPHOUT
#ifndef GRAPHOUT_PRECISE_BOUNDARY
    /* if monophone (= no backscan), the tail g score should be kept here */
    /* else, updated tail g score will be computed in scan_word()  */
    if(!ccd_flag) {
      now->tail_g_score = now->g[now->bestt];
    }
#endif
#endif

    /*
     * �������������򹹿����롧 �Ǹ��ñ�����ʬ����������������׻����롥
     * update forward score: compute forward trellis for the last word
     */
#ifdef DEBUG
    VERMES("scan_word\n");
#endif
    scan_word(now, param);
    if (now->score < backmax + LOG_ZERO) { /* another end-of-search detecter */
      j_printf("now->score = %f?\n",now->score);
      put_hypo_woutput(now);
      free_node(now);
      continue;
    }

    /* 
     * ���Ф������⤬ʸ�Ȥ��Ƽ�����ǽ�Ǥ���С�
     * �����ե饰��Ω�ƤƤ򥹥��å��ˤ���ľ���Ƥ�����
     * (���˼��Ф��줿���Ȥʤ�)
     *
     * if the current popped hypothesis is acceptable, set endflag
     * and return it to stack: it will become the recognition result
     * when popped again.
     */
#ifdef DEBUG
    VERMES("accept check\n");
#endif
    if (
#ifdef USE_NGRAM
	ngram_acceptable(now, winfo)
#else  /* USE_DFA */
	dfa_acceptable(now, dfa)
#endif
	&& now->estimated_next_t <= 5) {
      new = newnode();
      /* new �� now ����Ȥ򥳥ԡ����ơ��ǽ�Ū�ʥ�������׻� */
      /* copy content of 'now' to 'new', and compute the final score */
      last_next_word(now, new, param);
      if (debug2_flag) {
	j_printf("  This is acceptable as a sentence candidate\n");
      }
      /* g[] �����ϻ�ü��ã���Ƥ��ʤ���д��� */
      /* reject this sentence candidate if g[] does not reach the end */
      if (new->score <= LOG_ZERO) {
	if (debug2_flag) {
	  j_printf("  But invalid because Viterbi pass does not reach the 0th frame\n");
	}
	free_node(new);
	free_node(now);
	continue;
      }
      /* �����ե饰��Ω�Ƥ�����ľ�� */
      /* set endflag and push again  */
      if (debug2_flag) {
	j_printf("  This hypo itself was pushed with final score=%f\n", new->score);
      }
      new->endflag = TRUE;
      if (put_to_stack(new, &start, &bottom, &stacknum, stacksize) != -1) {
#ifdef GRAPHOUT
	if (new->score > LOG_ZERO) {
	  new->lastcontext = now->prevgraph;
	  new->prevgraph = wordgraph_assign(new->seq[new->seqnum-1],
					    WORD_INVALID,
					    (new->seqnum >= 2) ? new->seq[new->seqnum-2] : WORD_INVALID,
					    0,
#ifdef GRAPHOUT_PRECISE_BOUNDARY
					    /* wordend are shifted to the last */
#ifdef PASS2_STRICT_IWCD
					    new->wordend_frame[0],
#else
					    now->wordend_frame[0],
#endif
#else
					    now->bestt,
#endif
					    new->score,
					    prev_score,
					    now->g[0],
#ifdef GRAPHOUT_PRECISE_BOUNDARY
#ifdef PASS2_STRICT_IWCD
					    new->wordend_gscore[0],
#else
					    now->wordend_gscore[0],
#endif
#else
					    now->tail_g_score,
#endif
#ifdef USE_NGRAM
					    now->lscore,
#else
					    LOG_ZERO,
#endif
#ifdef CM_SEARCH
					    new->cmscore[new->seqnum-1]
#else
					    LOG_ZERO
#endif
					    );
	} else {
	  new->lastcontext = now->lastcontext;
	  new->prevgraph = now->prevgraph;
	}
#endif
      }
      /* ���β���Ϥ����ǽ���餺��, �������餵���ñ��Ÿ������ */
      /* continue with the 'now' hypothesis, not terminate here */
    }

    /*
     * ���β��⤫�顤��ñ�콸�����ꤹ�롥
     * ��ñ�콸���, ���β���ο����ü�ե졼����դ�¸�ߤ���
     * �裱�ѥ��Υȥ�ꥹñ�콸�硥
     *
     * N-gram�ξ��ϳ�ñ��� n-gram ��³��Ψ���ޤޤ�롥
     * DFA �ξ���, ������Ǥ���� DFA �����³��ǽ�ʤ�ΤΤߤ��֤äƤ���
     */
    /*
     * Determine next word set that can connect to this hypothesis.
     * They come from the trellis word that has been survived at near the
     * beginning of the last word.
     *
     * In N-gram mode, they also contain N-gram probabilities toward the
     * source hypothesis.  In DFA mode, the word set is further reduced
     * by the grammatical constraint
     */
#ifdef DEBUG
    VERMES("get next words\n");
#endif
#ifdef USE_NGRAM
    nwnum = ngram_nextwords(now, nextword, maxnwnum, ngram, winfo, backtrellis);
#else  /* USE_DFA */
    nwnum = dfa_nextwords(now, nextword, maxnwnum, dfa);
    /* nextword ����줿�顢�Хåե������䤷�ƺƥ����� */
    /* If the number of nextwords can exceed the buffer size, expand the
       nextword data area */
    while (nwnum < 0) {
      nextword = nw_expand(nextword, &maxnwnum, &nwroot);
      nwnum = dfa_nextwords(now, nextword, maxnwnum, dfa);
    }
#endif
    if (debug2_flag) {
      j_printf("  %d words extracted from wordtrellis\n", nwnum);
    }

    /* 
     * ����ȼ�ñ�콸�礫�鿷����ʸ������������������å��ˤ���롥
     */
    /*
     * generate new hypotheses from 'now' and 'nextword', 
     * and push them to stack
     */
#ifdef DEBUG
    VERMES("generate hypo\n");
#endif

#ifdef USE_DFA
    now_noise_calced = FALSE;	/* TRUE is noise-inserted score has been calculated */
#endif
    i = pushctr;		/* store old value */

#ifdef CM_SEARCH
    /* initialize local stack */
    cm_init(winfo);
#endif

    /* for each nextword, generate a new hypothesis */
    for (w = 0; w < nwnum; w++) {
#ifdef USE_DFA
      /* limit word hypothesis */
      if (! (winfo->wton[nextword[w]->id] >= cate_bgn && winfo->wton[nextword[w]->id] < cate_bgn + cate_num)) {
	continue;
      }
#endif
      new = newnode();
#ifdef USE_DFA
      if (nextword[w]->can_insert_sp == TRUE) {
	/* �Υ����򶴤���ȥ�ꥹ��������׻��������ޤʤ����Ȥκ����ͤ��� */
	/* compute hypothesis score with noise inserted */
	
	if (now_noise_calced == FALSE) {
	  /* now �� sp ��Ĥ������� now_noise ����,���Υ�������׻� */
	  /* generate temporal hypothesis 'now_noise' which has short-pause
	     word after the original 'now' */
	  fornoise.id = dfa->sp_id;
	  now_noise = newnode();
	  cpy_node(now_noise, now);
#if 0
	  now_noise_tmp = newnode();
	  next_word(now, now_noise_tmp, &fornoise, param, backtrellis);
	  scan_word(now_noise_tmp, param);
	  for(t=0;t<peseqlen;t++) {
	    now_noise->g[t] = max(now_noise_tmp->g[t], now->g[t]);
	  }
	  free_node(now_noise_tmp);
#else
	  /* expand NOISE only if it exists in backward trellis */
	  /* begin patch by kashima */
	  if (looktrellis_flag) {
	    if(!dfa_look_around(&fornoise, now, backtrellis)){
	      free_node(now_noise);
	      free_node(new);
	      continue;
	    }
	  }
	  /* end patch by kashima */

	  /* now_nosie �� ������ g[] ��׻��������� now �� g[] ����Ӥ���
	     �⤤������� */
	  /* compute trellis score g[], and adopt the maximum score
	     for each frame compared with now->g[] */
	  next_word(now, now_noise, &fornoise, param, backtrellis);
	  scan_word(now_noise, param);
	  for(t=0;t<peseqlen;t++) {
	    now_noise->g[t] = max(now_noise->g[t], now->g[t]);
	  }
	  /* �Υ����򶴤���ݤ��θ������������׻������Τǡ�
	     �����ǺǸ�ΥΥ���ñ��� now_noise ����ä� */
	  /* now that score has been computed considering pause insertion,
	     we can delete the last noise word from now_noise here */
	  now_noise->seqnum--;
#endif
	  now_noise_calced = TRUE;
	}

	/* expand word only if it exists in backward trellis */
	/* begin patch by kashima */
	if (looktrellis_flag) {
	  if(!dfa_look_around(nextword[w], now_noise, backtrellis)){
	    free_node(new);
	    continue;
	  }
	}
	/* end patch by kashima */
	
	/* ����������' new' �� 'now_noise' �������� */
	/* generate a new hypothesis 'new' from 'now_noise' */
	next_word(now_noise, new, nextword[w], param, backtrellis);
	
      } else {
	
	/* expand word only if it exists in backward trellis */
	/* begin patch by kashima */
	if (looktrellis_flag) {
	  if(!dfa_look_around(nextword[w], now, backtrellis)){
	    free_node(new);
	    continue;
	  }
	}
	/* end patch by kashima */
	
	/* ����������' new' �� 'now_noise' �������� */
	/* generate a new hypothesis 'new' from 'now_noise' */
	next_word(now, new, nextword[w], param, backtrellis);
	
      }
#else  /* not USE_DFA */

      /* ����������' new' �� 'now_noise' ��������
	 N-gram �ξ��ϥΥ��������̰������ʤ� */
      /* generate a new hypothesis 'new' from 'now'.
	 pause insertion is treated as same as normal words in N-gram mode. */
      next_word(now, new, nextword[w], param, backtrellis);

#endif /* USE_DFA */

      if (new->score <= LOG_ZERO) { /* not on trellis */
	free_node(new);
	continue;
      }
	
      genectr++;

#ifdef CM_SEARCH
      /* store the local hypothesis to temporal stack */
      cm_store(new);
#else 
      /* ������������ 'new' �򥹥��å�������� */
      /* push the generated hypothesis 'new' to stack */

      /* stack overflow */
      if (can_put_to_stack(new, &bottom, &stacknum, stacksize) == -1) {
	free_node(new);
	continue;
      }
      
#ifdef GRAPHOUT
      /* assign a word arc to the last fixed word */
      new->lastcontext = now->prevgraph;
      new->prevgraph = wordgraph_assign(new->seq[new->seqnum-2],
					new->seq[new->seqnum-1],
					(new->seqnum >= 3) ? new->seq[new->seqnum-3] : WORD_INVALID,
					new->bestt + 1,
#ifdef GRAPHOUT_PRECISE_BOUNDARY
#ifdef PASS2_STRICT_IWCD
					/* most up-to-date wordend_gscore is on new, because the last phone of 'now' will be computed at next_word() */
					new->wordend_frame[new->bestt],
#else
					now->wordend_frame[new->bestt],
#endif
#else
					now->bestt,
#endif
					new->score, prev_score,
					now->g[new->bestt+1],
#ifdef GRAPHOUT_PRECISE_BOUNDARY
#ifdef PASS2_STRICT_IWCD
					/* most up-to-date wordend_gscore is on new, because the last phone of 'now' will be computed at next_word() */
					new->wordend_gscore[new->bestt],
#else
					now->wordend_gscore[new->bestt],
#endif
#else
					now->tail_g_score,
#endif
#ifdef USE_NGRAM
					now->lscore,
#else
					LOG_ZERO,
#endif
#ifdef CM_SEARCH
					new->cmscore[new->seqnum-2]
#else
					LOG_ZERO
#endif
					);
#endif /* GRAPHOUT */
      put_to_stack(new, &start, &bottom, &stacknum, stacksize);
      if (debug2_flag) {
	j = new->seq[new->seqnum-1];
	j_printf("  %15s [%15s](id=%5d)(%f) [%d-%d] pushed\n",winfo->wname[j], winfo->woutput[j], j, new->score, new->estimated_next_t + 1, new->bestt);
      }
#ifdef VISUALIZE
      visual2_next_word(new, now, popctr);
#endif
      pushctr++;
#endif
    } /* end of nextword loop */

#ifdef CM_SEARCH
    /* compute score sum */
    cm_sum_score();
    /* compute CM and put the generated hypotheses to global stack */
    while ((new = cm_get_node()) != NULL) {
      cm_set_score(new);
#ifdef CM_SEARCH_LIMIT
      if (new->cmscore[new->seqnum-1] < cm_cut_thres
#ifdef CM_SEARCH_LIMIT_AFTER
	  && finishnum > 0
#endif
	  ) {
	free_node(new);
	continue;
      }
#endif /* CM_SEARCH_LIMIT */
      /*      j = new->seq[new->seqnum-1];
	      printf("  %15s [%15s](id=%5d)(%f) [%d-%d] cm=%f\n",winfo->wname[j], winfo->woutput[j], j, new->score, new->estimated_next_t + 1, new->bestt, new->cmscore[new->seqnum-1]);*/

      /* stack overflow */
      if (can_put_to_stack(new, &bottom, &stacknum, stacksize) == -1) {
	free_node(new);
	continue;
      }

#ifdef GRAPHOUT

      /* assign a word arc to the last fixed word */
      new->lastcontext = now->prevgraph;
      new->prevgraph = wordgraph_assign(new->seq[new->seqnum-2],
					new->seq[new->seqnum-1],
					(new->seqnum >= 3) ? new->seq[new->seqnum-3] : WORD_INVALID,
					new->bestt + 1,
#ifdef GRAPHOUT_PRECISE_BOUNDARY
#ifdef PASS2_STRICT_IWCD
					new->wordend_frame[new->bestt],
#else
					now->wordend_frame[new->bestt],
#endif
#else
					now->bestt,
#endif
					new->score, prev_score,
					now->g[new->bestt+1],
#ifdef GRAPHOUT_PRECISE_BOUNDARY
#ifdef PASS2_STRICT_IWCD
					new->wordend_gscore[new->bestt],
#else
					now->wordend_gscore[new->bestt],
#endif
#else
					now->tail_g_score,
#endif
#ifdef USE_NGRAM
					now->lscore,
#else
					LOG_ZERO,
#endif
#ifdef CM_SEARCH
					new->cmscore[new->seqnum-2]
#else
					LOG_ZERO
#endif
					);
#endif /* GRAPHOUT */
      
      put_to_stack(new, &start, &bottom, &stacknum, stacksize);
      if (debug2_flag) {
	j = new->seq[new->seqnum-1];
	j_printf("  %15s [%15s](id=%5d)(%f) [%d-%d] pushed\n",winfo->wname[j], winfo->woutput[j], j, new->score, new->estimated_next_t + 1, new->bestt);
      }
#ifdef VISUALIZE
      visual2_next_word(new, now, popctr);
#endif
      pushctr++;
    }
#endif

    if (debug2_flag) {
      j_printf("%d pushed\n",pushctr-i);
    }
#ifdef USE_DFA
    if (now_noise_calced == TRUE) free_node(now_noise);
#endif

    /* 
     * ���Ф��������ΤƤ�
     * free the source hypothesis
     */
    free_node(now);

  }
  /***************/
  /* End of Loop */
  /***************/

  /* output */
  if (!result_reorder_flag && finishnum < ncan) {
    result_pass2_end();
  }
  /* õ�����Ԥǰ�Ĥ���䤬�����ʤ���С���1�ѥ��η�̤򤽤Τޤ޽��Ϥ��� */
  /* If search failed and no candidate obtained in this pass, output
     the result of the previous 1st pass as a final result. */
  if (finishnum == 0) {		/* if search failed */
    if (verbose_flag) {
      j_printf("got no candidates, output 1st pass result as a final result\n",finishnum);
    }
    /* make hypothesis data from the result of previous 1st pass */
    now = newnode();
    for (i=0;i<pass1_wnum;i++) {
      now->seq[i] = pass1_wseq[pass1_wnum-1-i];
    }
    now->seqnum = pass1_wnum;
    now->score = pass1_score;
#ifdef CONFIDENCE_MEASURE
    /* fill in null values */
#ifdef CM_MULTIPLE_ALPHA
    for(j=0;j<cm_alpha_num;j++) {
      for(i=0;i<now->seqnum;i++) now->cmscore[i][j] = 0.0;
    }
#else
    for(i=0;i<now->seqnum;i++) now->cmscore[i] = 0.0;
#endif
#endif /* CONFIDENCE_MEASURE */
    
    /* output it */
    if (module_mode) {
      /* �⥸�塼��⡼�ɤǤϡ���2�ѥ������Ԥ����鲿����Ϥ��ʤ� */
      result_pass2_failed(winfo);
    } else {
      /* output the result of the first pass as the final output */
      result_pass2_begin();
      result_pass2(now, 1, winfo);
      result_pass2_end();
    }
#ifdef SP_BREAK_CURRENT_FRAME
    /* segment restart processing */
    if (sp_break_last_nword_allow_override) sp_segment_set_last_nword(now);
#endif
    /* do forced alignment if needed */
    if (align_result_word_flag) word_rev_align(now->seq, now->seqnum, tparam);
    if (align_result_phoneme_flag) phoneme_rev_align(now->seq, now->seqnum, tparam);
    if (align_result_state_flag) state_rev_align(now->seq, now->seqnum, tparam);
    free_node(now);
  } else {			/* if at least 1 candidate found */
    if (debug2_flag) {
      j_printf("got %d candidates\n",finishnum);
    }
    if (result_reorder_flag) {
      /* ��̤Ϥޤ����Ϥ���Ƥ��ʤ��Τǡ�ʸ�����ѥ����å���򥽡��Ȥ���
	 �����ǽ��Ϥ��� */
      /* As all of the found candidate are in result stack, we sort them
	 and output them here  */
      if (debug2_flag) VERMES("done\n");
      result_reorder_and_output(&r_start, &r_bottom, &r_stacknum, output_hypo_maxnum, winfo);
    }
  }

  /* �Ƽ參���󥿤���� */
  /* output counters */
  if (verbose_flag) {
    j_printf("%d generated, %d pushed, %d nodes popped in %d\n",
	     genectr, pushctr, popctr, backtrellis->framelen);
    j_flushprint();
#ifdef GRAPHOUT_DYNAMIC
    j_printf("graph: %d merged", dynamic_merged_num);
#ifdef GRAPHOUT_SEARCH
    j_printf(", %d terminated", terminate_search_num);
#endif
    j_printf(" in %d\n", popctr);
#endif
  }
    
#ifdef GRAPHOUT
  printf("------ wordgraph post-processing begin ------\n");
  /* garbage collection and word merging */
  /* words with no following word (except end of sentence) will be erased */
  wordgraph_purge_leaf_nodes(&wordgraph_root);
#ifdef GRAPHOUT_DEPTHCUT
  /* perform word graph depth cutting here */
  wordgraph_depth_cut(&wordgraph_root);
#endif
  /* if GRAPHOUT_PRECISE_BOUNDARY defined,
     propagate exact time boundary to the right context.
     words of different boundary will be duplicated here.
  */
  wordgraph_adjust_boundary(&wordgraph_root);
  /* merge words with the same time and same score */
  wordgraph_compaction_thesame(&wordgraph_root);
  /* merge words with the same time (skip if "-graphrange -1") */
  wordgraph_compaction_exacttime(&wordgraph_root);
  /* merge words of near time (skip if "-graphrange" value <= 0 */
  wordgraph_compaction_neighbor(&wordgraph_root);
  /* finalize: sort and annotate ID */
  wordgraph_sort_and_annotate_id(&wordgraph_root);
  /* check coherence */
  wordgraph_check_coherence(wordgraph_root);
  printf("------ wordgraph post-processing end ------\n");
  /* debug: output all graph word info */
  wordgraph_dump(wordgraph_root);
  /* output graph */
  result_graph(wordgraph_root, winfo);
  /* clear all wordgraph */
  wordgraph_clean(&wordgraph_root);
#endif
  
  /* ��λ���� */
  /* finalize */
  nw_free(nextword, nwroot);
  free_all_nodes(start);
  free_wordtrellis();
#ifdef SCAN_BEAM
  free(framemaxscore);
#endif
  clear_stocker();
}
