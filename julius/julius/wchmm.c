/**
 * @file   wchmm.c
 * @author Akinobu Lee
 * @date   Mon Sep 19 23:39:15 2005
 * 
 * <JA>
 * @brief  �ڹ�¤��������ۤ���
 *
 * �����Ǥϡ�Ϳ����줿ñ�켭��, HMM�������Ӹ������󤫤��ڹ�¤�������
 * ���ۤ���ؿ����������Ƥ��ޤ����ڹ�¤������ϵ�ư���˹��ۤ��졤
 * ��1�ѥ���ǧ�����Ѥ����ޤ����ڹ�¤������Ͼ���ñ�̤ǹ������졤
 * �ƾ��֤�HMM���ϳ�Ψ���������¾�������õ���Τ�����͡��ʾ����ޤߤޤ���
 *
 * ��ȯ�ηа޾塤��������Ǥ��ڹ�¤������� wchmm (word-conjunction HMM) ��
 * ��ɽ������Ƥ��ޤ���
 * 
 * </JA>
 * 
 * <EN>
 * @brief  Build tree lexicon.
 *
 * Functions to build a tree lexicon (or called word-conjunction HMM here)
 * from word dictionary, HMM and language models are defined here.  The
 * constructed tree lexicon will be used for the recognition of the 1st pass.
 * The lexicon is composed per HMM state unit, and various informations
 * about output probabilities, arcs, language model constraints, and others
 * are assembled in the lexicon.
 *
 * Note that the word "wchmm" in the source code is a synonim of
 * "tree lexicon". 
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

/* wchmm = word conjunction HMM = lexicon tree */

#include <julius.h>


static int dupcount = 0;	///< Number of duplicated nodes (for debug only)

#undef WCHMM_SIZE_CHECK		///< If defined, do wchmm size estimation (for debug only)


/**************************************************************/
/*********** Initialization of tree lexicon *******************/
/**************************************************************/

/** 
 * <JA>
 * �ڹ�¤������򿷤��˳���դ��롥
 * 
 * @return �����˥����˳���դ���줿�ڹ�¤������¤�ΤؤΥݥ��󥿤��֤���
 * </JA>
 * <EN>
 * Allocate a new tree lexicon structure.
 * 
 * @return pointer to the newly allocated tree lexicon structure.
 * </EN>
 */
WCHMM_INFO *
wchmm_new()
{
  WCHMM_INFO *w;
  w = (WCHMM_INFO *)mymalloc(sizeof(WCHMM_INFO));
#ifdef USE_NGRAM
  w->ngram = NULL;
#endif
#ifdef USE_DFA
  w->dfa = NULL;
#endif
  w->winfo = NULL;
  w->malloc_root = NULL;
#ifdef PASS1_IWCD
#ifdef CATEGORY_TREE
  w->lcdset_category_root = NULL;
#endif /* CATEGORY_TREE */
#endif /* PASS1_IWCD */
  return w;
}

/** 
 * <JA>
 * �ڹ�¤����������Ƥ��������롥
 * 
 * @param wchmm [out] �ڹ�¤������ؤΥݥ���
 * </JA>
 * <EN>
 * Initialize content of a lexicon tree.
 * 
 * @param wchmm [out] pointer to the lexicon tree structure
 * </EN>
 */
static void
wchmm_init(WCHMM_INFO *wchmm)
{
  wchmm->maxwcn = MAXWCNSTEP;
  wchmm->state = (WCHMM_STATE *)mymalloc(sizeof(WCHMM_STATE)*wchmm->maxwcn);
#ifdef MULTIPATH_VERSION
  wchmm->wordbegin = (int *)mymalloc(sizeof(int)*wchmm->winfo->num);
#else
  wchmm->ststart = (WORD_ID *)mymalloc(sizeof(WORD_ID)*wchmm->maxwcn);
#endif
  wchmm->stend = (WORD_ID *)mymalloc(sizeof(WORD_ID)*wchmm->maxwcn);
  wchmm->offset = (int **)mymalloc(sizeof(int *)*wchmm->winfo->num);
  wchmm->wordend = (int *)mymalloc(sizeof(int)*wchmm->winfo->num);
#ifdef MULTIPATH_VERSION
  wchmm->maxstartnum = STARTNODE_STEP;
  wchmm->startnode = (int *)mymalloc(sizeof(int)*STARTNODE_STEP);
  wchmm->startnum = 0;
# ifdef CATEGORY_TREE
  wchmm->start2wid = (WORD_ID *)mymalloc(sizeof(WORD_ID)*STARTNODE_STEP);
# endif
#else
  wchmm->startnode = (int *)mymalloc(sizeof(int)*wchmm->winfo->num);
  wchmm->wordend_a = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->winfo->num);
#endif /* MULTIPATH_VERSION */
#ifdef PASS1_IWCD
  wchmm->outstyle = (unsigned char *)mymalloc(sizeof(unsigned char)*wchmm->maxwcn);
#endif
#ifdef UNIGRAM_FACTORING
  wchmm->start2isolate = NULL;
  wchmm->isolatenum = 0;
#endif
#ifndef CATEGORY_TREE
  wchmm->sclist = NULL;
  wchmm->sclist2node = NULL;
#ifdef UNIGRAM_FACTORING
  wchmm->fscore = NULL;
#endif
#endif /* CATEGORY_TREE */
  wchmm->n = 0;
}

/** 
 * <JA>
 * �ڹ�¤������ξ��ֳ�Ǽ�ΰ�� MAXWCNSTEP ʬ������Ĺ���롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Expand state-related area in a tree lexicon by MAXWCNSTEP.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
wchmm_extend(WCHMM_INFO *wchmm)
{
  wchmm->maxwcn += MAXWCNSTEP;
  wchmm->state = (WCHMM_STATE *)myrealloc(wchmm->state, sizeof(WCHMM_STATE)*wchmm->maxwcn);
#ifndef MULTIPATH_VERSION
  wchmm->ststart = (WORD_ID *)myrealloc(wchmm->ststart, sizeof(WORD_ID)*wchmm->maxwcn);
#endif
  wchmm->stend = (WORD_ID *)myrealloc(wchmm->stend, sizeof(WORD_ID)*wchmm->maxwcn);
#ifdef PASS1_IWCD
  wchmm->outstyle = (unsigned char *)myrealloc(wchmm->outstyle, sizeof(unsigned char)*wchmm->maxwcn);
#endif
}

#ifdef MULTIPATH_VERSION
/** 
 * <JA>
 * �ڹ�¤�������ñ����Ƭ�Ρ��ɳ�Ǽ�ΰ�� STARTNODE_STEPʬ������Ĺ���롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Expand word-start nodes area in a tree lexicon by STARTNODE_STEP.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
wchmm_extend_startnode(WCHMM_INFO *wchmm)
{
  wchmm->maxstartnum += STARTNODE_STEP;
  wchmm->startnode = (int *)myrealloc(wchmm->startnode, sizeof(int) * wchmm->maxstartnum);
#ifdef CATEGORY_TREE
  /* wchmm->start2wid = (WORD_ID *)myrealloc(wchmm->startnode, sizeof(WORD_ID) * wchmm->maxstartnum); */
  wchmm->start2wid = (WORD_ID *)myrealloc(wchmm->start2wid, sizeof(WORD_ID) * wchmm->maxstartnum);
#endif
}
#endif /* MULTIPATH_VERSION */

/** 
 * <JA>
 * �ڹ�¤�����񤪤�Ӥ��������γ��ե�������Ʋ������롥
 * 
 * @param w [in] �ڹ�¤������
 * </JA>
 * <EN>
 * Free all data in a tree lexicon.
 * 
 * @param w [in] tree lexicon
 * </EN>
 */
void
wchmm_free(WCHMM_INFO *w)
{
  S_CELL *sc, *sctmp;
  int i;
  /* wchmm->state[i].ac malloced by mybmalloc2() */
  /* wchmm->offset[][] malloced by mybmalloc2() */
#ifdef PASS1_IWCD
  /* LRC_INFO, RC_INFO in wchmm->state[i].outsty malloced by mybmalloc2() */
#endif
  /* they all will be freed by a single mybfree2() call */
  mybfree2(&(w->malloc_root));
#ifndef CATEGORY_TREE
  if (w->sclist != NULL) {
    for(i=1;i<w->scnum;i++) {
      sc = w->sclist[i];
      while(sc) {
	sctmp = sc->next;
	free(sc);
	sc = sctmp;
      }
    }
    free(w->sclist);
  }
  if (w->sclist2node != NULL) free(w->sclist2node);
#ifdef UNIGRAM_FACTORING
  if (w->fscore != NULL) free(w->fscore);
#endif
#endif
#ifdef UNIGRAM_FACTORING
  if (w->start2isolate != NULL) free(w->start2isolate);
#endif
#ifdef PASS1_IWCD
  free(w->outstyle);
#endif
#ifndef MULTIPATH_VERSION
  free(w->wordend_a);
#endif
  free(w->startnode);
#ifdef MULTIPATH_VERSION
# ifdef CATEGORY_TREE
  free(w->start2wid);
# endif
#endif /* MULTIPATH_VERSION */
  free(w->wordend);
  free(w->offset);
  free(w->stend);
#ifdef MULTIPATH_VERSION
  free(w->wordbegin);
#else
  free(w->ststart);
#endif
  free(w->state);
#ifdef PASS1_IWCD
#ifdef CATEGORY_TREE
  lcdset_remove_with_category_all(w);
#endif /* CATEGORY_TREE */
#endif /* PASS1_IWCD */
  free(w);
}


/**************************************************************/
/*********** Word sort functions for tree construction ********/
/**************************************************************/

static WORD_INFO *local_winfo;	///< Temporary work area for sort callbacks

/** 
 * <JA>
 * ñ����ǤΤʤ�Ӥǥ����Ȥ���qsort�ؿ�
 * 
 * @param widx1 [in] ñ��ID 1 �ؤΥݥ���
 * @param widx2 [in] ñ��ID 2 �ؤΥݥ���
 * 
 * @return ñ��widx2��ñ��widx1�ΰ���������Ǥ���� 1, ñ��widx1��ñ��widx2�ΰ���������Ǥ���� -1, ����Ʊ�������¤ӤǤ���� 0 ���֤���
 * </JA>
 * <EN>
 * qsort function to sort words by their phoneme sequence.
 * 
 * @param widx1 [in] pointer to word id #1
 * @param widx2 [in] pointer to wrod id #2
 * 
 * @return 1 if word[widx2] is part of word[widx1], -1 if word[widx1] is part of word[widx2], or 0 if the two words are equal.
 * </EN>
 */
static int
compare_wseq(WORD_ID *widx1, WORD_ID *widx2)
{
  int len1, len2, n;
  int p=0;
  
  len1 = local_winfo->wlen[*widx1];
  len2 = local_winfo->wlen[*widx2];

  n=0;
  /*  while (n < len1 && n < len2 && (p = (int)winfo->wseq[*widx1][n] - (int)winfo->wseq[*widx2][n]) == 0 ) n++;*/
  while (n < len1 && n < len2 && (p = strcmp((local_winfo->wseq[*widx1][n])->name, (local_winfo->wseq[*widx2][n])->name)) == 0 ) n++;
  if (n < len1) {
    if (n < len2) {
      /* differ */
      return(p);
    } else {
      /* 2 is part of 1 */
      return(1);
    }
  } else {
    if (n < len2) {
      /* 1 is part of 2 */
      return(-1);
    } else {
      /* same */
      return(0);
    }
  }
}

/** 
 * <JA>
 * ñ��ID�ν��� windex[bgn..bgn+len-1] ��ñ��β��Ǥʤ�Ӥǥ����Ȥ��롥
 * 
 * @param winfo [in] ñ�켭��
 * @param windex [i/o] ñ��ID�Υ���ǥå�����������ǥ����Ȥ�����
 * @param bgn [in] @a windex �Υ����ȳ�����
 * @param len [in] @a windex �� @a bgn ����Υ����Ȥ������ǿ�
 * </JA>
 * <EN>
 * Sort word IDs in windex[bgn..bgn+len-1] by their phoneme sequence order.
 * 
 * @param winfo [in] word lexicon
 * @param windex [i/o] index sequence of word IDs, (will be sorted in this function)
 * @param bgn [in] start point to sort in @a windex
 * @param len [in] length of indexes to be sorted from @a bgn
 * </EN>
 */
static void
wchmm_sort_idx_by_wseq(WORD_INFO *winfo, WORD_ID *windex, WORD_ID bgn, WORD_ID len)
{
  local_winfo = winfo;
  qsort(&(windex[bgn]), len, sizeof(WORD_ID), (int (*)(const void *, const void *))compare_wseq);
}

#ifdef CATEGORY_TREE
/** 
 * <JA>
 * ñ��򥫥ƥ���ID�ǥ����Ȥ���qsort�ؿ���
 * 
 * @param widx1 [in] ����1�ؤΥݥ���
 * @param widx2 [in] ����2�ؤΥݥ���
 * 
 * @return 
 * </JA>
 * <EN>
 * qsort function to sort words by their category ID.
 * 
 * @param widx1 [in] pointer to element #1
 * @param widx2 [in] pointer to element #2
 * 
 * @return 
 * </EN>
 */
static int
compare_category(WORD_ID *widx1, WORD_ID *widx2)
{
  int c1,c2;
  c1 = local_winfo->wton[*widx1];
  c2 = local_winfo->wton[*widx2];
  return(c1 - c2);
}

/** 
 * <JA>
 * ñ��ID���� windex[0..len-1] �򥫥ƥ���ID�ǥ����Ȥ��롥
 * 
 * @param winfo [in] ñ�켭��
 * @param windex [i/o] ñ��ID�Υ���ǥå�����������ǥ����Ȥ�����
 * @param len [in] @a windex �����ǿ�
 * </JA>
 * <EN>
 * Sort word IDs in windex[0..len-1] by their category ID.
 * 
 * @param winfo [in] tree lexicon
 * @param windex [i/o] index sequence of word IDs, (will be sorted in this function)
 * @param len [in] number of elements in @a windex
 * </EN>
 */
static void
wchmm_sort_idx_by_category(WORD_INFO *winfo, WORD_ID *windex, WORD_ID len)
{
  local_winfo = winfo;
  qsort(windex, len, sizeof(WORD_ID), (int (*)(const void *, const void *))compare_category);
}
#endif /* CATEGORY_TREE */
  

/**********************************************************************/
/************** Subroutines to link part of words  ********************/
/**********************************************************************/

/* 
   �֤���: ��ͭ���ǿ� */
/* compare 2 words 'i', 'j' from start phoneme, and return the number
   of sharable phonemes. */
/** 
 * <JA>
 * ��ñ��֤ǡ�ñ�����Ƭ����Ʊ��Ƕ�ͭ��ǽ�ʲ��Ǥο���Ĵ�٤롥
 * 
 * @param winfo [in] ñ�켭��
 * @param i [in] ñ�죱
 * @param j [in] ñ�죲
 * 
 * @return ��ͭ��ǽ����Ƭ����β��ǿ����֤���
 * </JA>
 * <EN>
 * Compare two words from word head per phoneme to see how many phones
 * can be shared among the two.
 * 
 * @param winfo [in] word dictionary
 * @param i [in] a word
 * @param j [in] another word
 * 
 * @return the number of phonemes to be shared from the head of the words.
 * </EN>
 */
static int
wchmm_check_match(WORD_INFO *winfo, int i, int j)
{
  int k,tmplen;

  for (tmplen=0,k=0;k<winfo->wlen[i];k++) {
    if (k > winfo->wlen[j]-1)
      break;
    if (! (strmatch(winfo->wseq[i][k]->name, winfo->wseq[j][k]->name)))
      break;
    tmplen++;
  }
  return(tmplen);
}

/** 
 * <JA>
 * �ڹ�¤������Τ���Ρ��ɤˡ��̤ΥΡ��ɤؤ����ܤ��ɲä���
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * @param node [in] �Ρ����ֹ�
 * @param a [in] ���ܳ�Ψ���п���
 * @param arc [in] ������ΥΡ����ֹ�
 * </JA>
 * <EN>
 * Add a transition arc between two nodes on the tree lexicon
 * 
 * @param wchmm [i/o] tree lexicon
 * @param node [in] node number of source node
 * @param a [in] transition probability in log scale
 * @param arc [in] node number of destination node
 * </EN>
 */
static void
add_wacc(WCHMM_INFO *wchmm, int node, LOGPROB a, int arc)
{
  A_CELL *ac;
  ac       = (A_CELL *) mybmalloc2(sizeof(A_CELL), &(wchmm->malloc_root));
  ac->a    = a;
  ac->arc  = arc;
  ac->next = wchmm->state[node].ac;
  wchmm->state[node].ac   = ac;
}

#ifdef MULTIPATH_VERSION

/* MULTIPATH_VERSION: homophone processing is not needed */

/**
 * <JA>
 * ����ñ��Τ�����֤β��Ǥ���ñ����ü�γ��ؽФ����ܤΥꥹ�Ȥ����롥
 * 
 * @param wchmm [in] �ڹ�¤������
 * @param w [in] ñ��ID
 * @param pos [in] ���ǰ���
 * @param node [out] ������Ρ�ñ����ü���ؤ����ܤ���ľ��֤Υꥹ��
 * @param a [out] @a node �γ����Ǥ����ܳ�Ψ
 * @param num [out] @a node �����ǿ���ȯ�����������ä���롥
 * @param maxnum [in] @a node �γ�Ǽ��ǽ�ʺ����
 * @param insert_sp [in] ñ�콪ü�Ǥ� sp ���߹��ߤ��θ����ʤ�TRUE
 * </JA>
 * <EN>
 * Make outgoing transition list for given phone position of a word.
 * 
 * @param wchmm [in] tree lexicon
 * @param w [in] word ID
 * @param pos [in] location of target phone to be inspected in the word @a w
 * @param node [out] list of wchmm states that possibly has outgoing transition
 * @param a [out] transition probabilities of the outgoing transitions in @a node
 * @param num [out] number of elements in @a out (found num will be added)
 * @param maxnum [in] maximum number of elements that can be stored in @a node
 * @param insert_sp [in] TRUE if consider short-pause insertion on word end
 * </EN>
 */
static void
get_outtrans_list(WCHMM_INFO *wchmm, WORD_ID w, int pos, int *node, LOGPROB *a, int *num, int maxnum, boolean insert_sp)
{
  HMM_Logical *ltmp;
  int states;
  int k, ato;
  LOGPROB prob;
  int oldnum;

  if (pos < 0) {
    
    /* set the word-beginning node, and return */
    node[*num] = wchmm->wordbegin[w];
    a[*num] = 0.0;
    (*num)++;
    
  } else {

    ltmp = wchmm->winfo->wseq[w][pos];
    states = hmm_logical_state_num(ltmp);

    /* check initial->final state */
    if ((hmm_logical_trans(ltmp))->a[0][states-1] != LOG_ZERO) {
      /* recursive call for previous phone */
      oldnum = *num;
      get_outtrans_list(wchmm, w, pos-1, node, a, num, maxnum, FALSE); /* previous phone should not be an sp-inserted phone */
      /* add probability of the skip transition to all the previous ones */
      for(k=oldnum;k<*num;k++) {
	a[k] += (hmm_logical_trans(ltmp))->a[0][states-1];
      }
    }
    /* add to list the arcs from output state to final state */
    for (k = 1; k < states - 1; k++) {
      prob = (hmm_logical_trans(ltmp))->a[k][states-1];
      if (prob != LOG_ZERO) {
	if (*num >= maxnum) {
	  j_error("Maximum outtrans list num exceeded! (%d)\n", maxnum);
	}
	node[*num] = wchmm->offset[w][pos] + k - 1;
	a[*num] = prob;
	(*num)++;
      }
    }
    /* for -iwsp, add outgoing arc from the tail sp model
       only if need_sp == TRUE.
       need_sp should be TRUE only when the connecting [pos] phone is also an end phone of the to-be-added word (i.e. homophone word)
     */
    /*  */
    if (enable_iwsp && insert_sp) {
      /* consider sp */
      for (k = 1; k < hmm_logical_state_num(wchmm->hmminfo->sp) - 1; k++) {
	prob = hmm_logical_trans(wchmm->hmminfo->sp)->a[k][hmm_logical_state_num(wchmm->hmminfo->sp)-1];
	if (prob != LOG_ZERO) {
	  if (*num >= maxnum) {
	    j_error("Maximum outtrans list num exceeded! (%d)\n", maxnum);
	  }
	  node[*num] = wchmm->offset[w][pos] + (states - 2) + k - 1;
	  a[*num] = prob;
	  (*num)++;
	}
      }
    }
  }
  /*printf("   %d(%s)-%d:\"%s\", num=%d\n", w, wchmm->winfo->woutput[w], pos,
    (pos < 0) ? "BGN" : wchmm->winfo->wseq[w][pos]->name, *num);*/
  return;
}  

#else  /* ~MULTIPATH_VERSION */

/** 
 * <JA>
 * ���벻�Ǥ������ξ��֤��顤���벻�Ǥ���Ƭ���֤ؤ����ܤ��ɲä��롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * @param from_node [in] ���벻�Ǥ������ξ���
 * @param to_node [in] ���벻�Ǥ���Ƭ����
 * @param tinfo [in] @a from_node ��°���벻��HMM�����ܳ�Ψ����
 * </JA>
 * <EN>
 * Add a transition from end node of a phone to start node of another phone.
 * 
 * @param wchmm [i/o] tree lexicon
 * @param from_node [in] end node of a phone
 * @param to_node [in] start node of a phone
 * @param tinfo [in] transition prob. matrix of the @a from_node phone.
 * </EN>
 */
static void
wchmm_link_hmm(WCHMM_INFO *wchmm, int from_node, int to_node, HTK_HMM_Trans *tinfo)
{     
  A_CELL *actmp;
  LOGPROB a, atmp;
  int i;
  boolean tflag;

  /* get transition probability to outer state in tinfo */
  for(i = tinfo->statenum - 2; i >= 0; i--) {
    if ((a = tinfo->a[i][tinfo->statenum-1]) != LOG_ZERO) { /* found */
      atmp = a;
      /* check if the arc already exist */
      tflag = FALSE;
      for (actmp = wchmm->state[from_node].ac; actmp; actmp = actmp->next) {
	if (actmp->arc == to_node && actmp->a == atmp) {
	  tflag = TRUE;
	  break;
	}
      }
      if (tflag) break;
      /* add the arc to wchmm */
      add_wacc(wchmm, from_node, atmp, to_node);
      return;			/* exit function here */
    }
  }      
  j_error("Error: No arc to endstate?\n");
}

/** 
 * <JA>
 * �ڹ�¤��������Σ�ñ����Τ��벻�Ǵ֤���³���롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * @param from_word [in] ���ܸ���ñ���ID
 * @param from_seq [in] ���ܸ���ñ�������³���벻�Ǥΰ���
 * @param to_word [in] �������ñ���ID
 * @param to_seq [in] �������ñ�������³���벻�Ǥΰ���
 * </JA>
 * <EN>
 * Connect two phonemes in tree lexicon.
 * 
 * @param wchmm [i/o] tree lexicon
 * @param from_word [in] source word ID
 * @param from_seq [in] index of source phoneme in @a from_word from which the other will be connected
 * @param to_word [in] destination word ID
 * @param to_seq [in] index of destination phoneme in @a to_word to which the other will connect
 * </EN>
 */
static void
wchmm_link_subword(WCHMM_INFO *wchmm, int from_word, int from_seq, int to_word, int to_seq)
{     
  HMM_Logical *last;
  int lastp;

  last = wchmm->winfo->wseq[from_word][from_seq];
  lastp = wchmm->offset[from_word][from_seq] + hmm_logical_state_num(last)-2 -1;
  wchmm_link_hmm(wchmm, lastp, wchmm->offset[to_word][to_seq],
		 hmm_logical_trans(last));
}

/**************************************************************/
/******** homophone processing: duplicating leaf nodes ********/
/**************************************************************/

/**
 * @note
 * <JA>
 * Ʊ�������:
 * �ڹ�¤������ˤ����Ƥ��٤Ƥ�ñ�����Ω�����ǽ����֤����ɬ�פ����뤿�ᡤ
 * Ʊ�������տ�������ɬ�פ����롥���Τ��ᡤ�ǽ���ڹ�¤��������ۤ�����, 
 * �̤�ñ��ȴ����˶�ͭ���줿ñ��(Ʊ����), ���뤤���̤�ñ��ΰ����Ȥ���
 * �����ޤ�Ƥ��ޤäƤ���ñ���ȯ������ȤȤ��, ���κǽ��Ρ��ɤ�
 * ���ԡ����ƿ�����ñ�콪ü�Ρ��ɤ���ɬ�פ����롥
 * </JA>
 * <EN>
 * Homophones:
 * As all words need to have an uniq state as a final state in a lexicon tree,
 * homophones should be handled carefully.  After primal tree has been made,
 * we look through the tree to find the fully shared or embedded words
 * (homophone or part of other word), and duplicate the last leaf node 
 * to have uniq end state.
 * </EN>
 */

/** 
 * <JA>
 * ñ�콪ü���֤���Ω����Ϳ����줿ñ��ν�ü�Ρ��ɤ򥳥ԡ����ơ�
 * �����ˤ���ñ��κǽ����֤Ȥ���������롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * @param node [in] Ʊ����ν�ü�Ρ����ֹ�
 * @param word [in] ��������Ͽ����ñ��
 * </JA>
 * <EN>
 * Isolation of word-end nodes for homophones: duplicate the word-end state,
 * link as the same as original, and make it the new word-end node of the
 * given new word.
 * 
 * @param wchmm [i/o] tree lexicon
 * @param node [in] the word end node of the already existing homophone
 * @param word [in] word ID to be added to the tree
 * </EN>
 */
static void
wchmm_duplicate_state(WCHMM_INFO *wchmm, int node, int word) /* source node, new word */
{
  int n;
  int n_src, n_prev;
  A_CELL	*ac;
  HMM_Logical *lastphone;

  /* 1 state will newly created: expand tree if needed */
  if (wchmm->n + 1 >= wchmm->maxwcn) {
    wchmm_extend(wchmm);
  }
  /* n: the target new node to which 'node' is copied */
  n = wchmm->n;

  n_src = node;

  /* copy output probability info */
#ifdef PASS1_IWCD
  {
    RC_INFO *rcnew;
    LRC_INFO *lrcnew;
    wchmm->outstyle[n] = wchmm->outstyle[n_src];
    if (wchmm->outstyle[n] == AS_RSET) {
      /* duplicate RC_INFO because it has its own cache */
      rcnew = (RC_INFO *)mybmalloc2(sizeof(RC_INFO), &(wchmm->malloc_root));
      memcpy(rcnew, wchmm->state[n_src].out.rset, sizeof(RC_INFO));
      wchmm->state[n].out.rset = rcnew;
    } else if (wchmm->outstyle[n] == AS_LRSET) {
      /* duplicate LRC_INFO because it has its own cache */
      lrcnew = (LRC_INFO *)mybmalloc2(sizeof(LRC_INFO), &(wchmm->malloc_root));
      memcpy(lrcnew, wchmm->state[n_src].out.lrset, sizeof(LRC_INFO));
      wchmm->state[n].out.lrset = lrcnew;
    } else {
      /* share same info, simply copy the pointer */
      memcpy(&(wchmm->state[n].out), &(wchmm->state[n_src].out), sizeof(ACOUSTIC_SPEC));
    }
  }
#else  /* ~PASS1_IWCD */
  memcpy(&(wchmm->state[n].out), &(wchmm->state[n_src].out), sizeof(HTK_HMM_State *));
#endif

  lastphone = wchmm->winfo->wseq[word][wchmm->winfo->wlen[word]-1];
  wchmm->state[n].ac = NULL;

  /* add self transition arc */
  for(ac=wchmm->state[n_src].ac; ac; ac=ac->next) {
    if (ac->arc == n_src) {
      add_wacc(wchmm, n, ac->a, n);
    }
  }
  
  /* copy transition arcs whose destination is the source node to new node */
  if (hmm_logical_state_num(lastphone) == 3) { /* = 1 state */
    /* phone with only 1 state should be treated carefully */
    if (wchmm->winfo->wlen[word] == 1) { /* word consists of only this phone */
      /* no arcs need to be copied: this is also a start node of a word */
      wchmm->ststart[n] = word;
      wchmm->offset[word][0] = n;
    } else {
      /* copy arcs from the last state of the previous phone */
      n_prev = wchmm->offset[word][wchmm->winfo->wlen[word]-2]
	+ hmm_logical_state_num(wchmm->winfo->wseq[word][wchmm->winfo->wlen[word]-2]) - 3;
      for (ac=wchmm->state[n_prev].ac; ac; ac=ac->next) {
	if (ac->arc == n_src) {
	  add_wacc(wchmm, n_prev, ac->a, n);
	}
      }
      /* also update the last offset (== wordend in this case) */
      wchmm->offset[word][wchmm->winfo->wlen[word]-1] = n;
      /* the new node should not be a start node of a word */
      wchmm->ststart[n] = WORD_INVALID;
    }
  } else {			/* phone with more than 2 states */
    /* copy arcs from/to the source node to new node */
    for (n_prev = wchmm->offset[word][wchmm->winfo->wlen[word]-1]; n_prev < n_src; n_prev++) {
      for (ac=wchmm->state[n_prev].ac; ac; ac=ac->next) {
	if (ac->arc == n_src) {
	  add_wacc(wchmm, n_prev, ac->a, n);
	}
      }
      for (ac=wchmm->state[n_src].ac; ac; ac=ac->next) {
	if (ac->arc == n_prev) {
	  add_wacc(wchmm, n, ac->a, n_prev);
	}
      }
    }
    /* the new node should not be a start node of a word */
    wchmm->ststart[n] = WORD_INVALID;
  }

  /* map word <-> node */
  wchmm->stend[n]   = word;	/* 'n' is an end node of word 'word' */
  wchmm->wordend[word] = n;	/* the word end node of 'word' is 'n' */

  /* new state has been created: increment the size */
  wchmm->n++;
  
}

/** 
 * <JA>
 * �ڹ�¤���������Τ��������ơ����٤Ƥ�Ʊ����ˤĤ���ñ�콪ü���֤���Ω��
 * ��Ԥ���
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Scan the whole lexicon tree to find already registered homophones, and
 * make word-end nodes of the found homophones isolated from others.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
wchmm_duplicate_leafnode(WCHMM_INFO *wchmm)
{
  int w, nlast, n, narc, narc_model;
  boolean *dupw;		/* node marker */
  A_CELL *actmp;

  nlast = wchmm->n;
  dupw = (boolean *)mymalloc(sizeof(boolean) * nlast);
  for(n=0;n<nlast;n++) dupw[n] = FALSE;	/* initialize all marker */

  for (w=0;w<wchmm->winfo->num;w++) {
    n = wchmm->wordend[w];
    if (dupw[n]) {		/* if already marked (2nd time or later */
      wchmm_duplicate_state(wchmm, n, w); dupcount++; /* duplicate */
    } else {			/* if not marked yet (1st time) */
      /* try to find an arc outside the word */
      {
	/* count number of model-internal arc from the last state */
	HMM_Logical *lastphone;
	HTK_HMM_Trans *tinfo;
	int laststate, i;
	lastphone = wchmm->winfo->wseq[w][wchmm->winfo->wlen[w]-1];
	laststate = hmm_logical_state_num(lastphone) - 2;
	tinfo = hmm_logical_trans(lastphone);
	narc_model=0;
	for(i=1;i<hmm_logical_state_num(lastphone)-1;i++) {
	  if (tinfo->a[laststate][i] != LOG_ZERO) narc_model++;
	}
	/* count number of actual arc from the last state in the tree */
	narc = 0;
	for(actmp=wchmm->state[n].ac;actmp;actmp=actmp->next) narc++;
      }
      /* if both number does not match, it means it is not a single word tail */
      if (narc_model != narc) {
	/* word 'w' is embedded as part of other words at this node 'n' */
	/* duplicate this node now */
	wchmm_duplicate_state(wchmm, n, w); dupcount++;
	/* as new node has been assigned as word end node of word 'w',
	   reset this source node as it is not the word end node */
	wchmm->stend[n] = WORD_INVALID;
      } else {
	/* no arc to other node found, it means it is a single word tail */
	/* as this is first time, only make sure that this node is word end of [w] */
	wchmm->stend[n] = w;
      }
      /* mark node 'n' */
      dupw[n] = TRUE;
    }
  }
  free(dupw);
}

#endif /* ~MULTIPATH_VERSION */

/**************************************************************/
/*************** add a word to wchmm lexicon tree *************/
/**************************************************************/

/** 
 * <JA>
 * �ڹ�¤������˿�����ñ����ɲä��롥�ɲþ��ξ���Ȥ��ơ����ߤ��ڹ�¤��
 * ������ǺǤ⤽��ñ�����Ƭ�����ɤ��ޥå�����ñ�졤����Ӥ��Υޥå�����Ĺ��
 * ����ꤹ�롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * @param word [in] �ɲä��뼭��ñ���ID
 * @param matchlen [in] @a word �� @a matchword ����Ƭ����ޥå����벻��Ĺ
 * @param matchword [in] ��¸���ڹ�¤��������� @a word �ȺǤ�ޥå�����ñ��
 * </JA>
 * <EN>
 * Add a new word to the lexicon tree.  The longest matched word in the current
 * lexicon tree and the length of the matched phoneme from the word head should
 * be specified to tell where to insert the new word to the tree.
 * 
 * @param wchmm [i/o] tree lexicon
 * @param word [in] word id to be added to the lexicon
 * @param matchlen [in] phoneme match length between @a word and @a matchword.
 * @param matchword [in] the longest matched word with @a word in the current lexicon tree
 * </EN>
 */
static void
wchmm_add_word(WCHMM_INFO *wchmm, int word, int matchlen, int matchword)
{
  int   j,k,n;
  int   add_head, add_tail, add_to;
  int   word_len, matchword_len;
  HMM_Logical *ltmp;
  int ato;
  LOGPROB prob;
  int ntmp;
  int ltmp_state_num;
#ifdef PASS1_IWCD
  CD_Set *lcd = NULL;
#endif
  
#ifdef MULTIPATH_VERSION
  int *out_from, *out_from_next;
  LOGPROB *out_a, *out_a_next;
  int out_num_prev, out_num_next;
  int kkk;
#endif
  
/* 
 *   if (matchlen > 0) {
 *     printf("--\n");
 *     put_voca(wchmm->winfo, word);
 *     put_voca(wchmm->winfo, matchword);
 *     printf("matchlen=%d\n", matchlen);
 *   }
 */
  
  /* variable abbreviations */
  n = wchmm->n;
  word_len      = wchmm->winfo->wlen[word];
  matchword_len = wchmm->winfo->wlen[matchword];

  /* malloc phone offset area */
  if((wchmm->offset[word]=(int *)mybmalloc2(sizeof(int)*word_len, &(wchmm->malloc_root)))==NULL){ 
    j_error("malloc failed at wchmm_add_word()\n");
  }
  
  /* allocate unshared (new) part */
  add_head = matchlen;
  add_tail = word_len - 1;
  add_to   = matchlen - 1;

#ifdef MULTIPATH_VERSION
  /* make word-beginning node if needed */
  if (matchlen == 0) {
    /* create word-beginning node */
    wchmm->wordbegin[word] = n;
    wchmm->stend[n] = WORD_INVALID;
    wchmm->state[n].ac = NULL;
    wchmm->state[n].out.state = NULL;
    /* index the new word-beginning node as startnode (old ststart) */
    wchmm->startnode[wchmm->startnum] = n;
#ifdef CATEGORY_TREE
    wchmm->start2wid[wchmm->startnum] = word;
#endif
    /* expand data area if necessary */
    if (++wchmm->startnum >= wchmm->maxstartnum) wchmm_extend_startnode(wchmm);
    if (++n >= wchmm->maxwcn) wchmm_extend(wchmm);
  } else {
    wchmm->wordbegin[word] = wchmm->wordbegin[matchword];
  }

  /* now n is at beginning of output state */

  /* prepare work area for skip arcs */
  out_from = (int *)mymalloc(sizeof(int) * wchmm->winfo->maxwn);
  out_from_next = (int *)mymalloc(sizeof(int) * wchmm->winfo->maxwn);
  out_a = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wchmm->winfo->maxwn);
  out_a_next = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wchmm->winfo->maxwn);

  /* store the initial outgoing arcs to out_from[] and out_a[] */
  out_num_prev = 0;
  if (matchlen == 0) {
    /* set the word-beginning node */
    out_from[0] = wchmm->wordbegin[word];
    out_a[0] = 0.0;
    out_num_prev = 1;
  } else {
    /*printf("%d(%s)\n", word, wchmm->winfo->woutput[word]);*/
    /* on -iwsp, trailing sp is needed only when no phone will be created */
    get_outtrans_list(wchmm, matchword, add_to, out_from, out_a, &out_num_prev, wchmm->winfo->maxwn, (add_tail - add_head + 1 > 0) ? FALSE : TRUE);
    /*printf("NUM=%d\n", out_num_prev);*/
  }
  
#endif /* MULTIPATH_VERSION */
  
  if (add_tail - add_head + 1 > 0) { /* there are new phones to be created */
      ntmp = n;
      for (j=add_head; j <= add_tail; j++) { /* for each new phones */
	ltmp = wchmm->winfo->wseq[word][j];
	ltmp_state_num = hmm_logical_state_num(ltmp);
#ifdef PASS1_IWCD
	if (ccd_flag) {
	  /* in the triphone lexicon tree, the last phone of a word has
	     left-context cdset */
	  if (wchmm->winfo->wlen[word] > 1 && j == wchmm->winfo->wlen[word] - 1) {
#ifdef CATEGORY_TREE
	    if (! old_iwcd_flag) {
	      lcd = lcdset_lookup_with_category(wchmm, ltmp, wchmm->winfo->wton[word]);
	      if (lcd == NULL) {
		/* no category-aware cdset found.  This is case when no word
		   can follow this word grammatically.
		   so fallback to normal state */
		j_printerr("Warning: no lcdset found for [%s::%d], fallback to [%s]\n", ltmp->name, wchmm->winfo->wton[word], ltmp->name);
		lcd = lcdset_lookup_by_hmmname(wchmm->hmminfo, ltmp->name);
	      }
	    } else {
	      lcd = lcdset_lookup_by_hmmname(wchmm->hmminfo, ltmp->name);
	    }
#else
	    lcd = lcdset_lookup_by_hmmname(wchmm->hmminfo, ltmp->name);
#endif
	    if (lcd == NULL) {
	      j_error("Error: no lcdset found for [%s]\n",ltmp->name);
	    }
	  }
	}
#endif /* PASS1_IWCD */
	for (k = 1; k < ltmp_state_num - 1; k++) { /* for each state in the phone */
	  /* set state output prob info */
#ifdef PASS1_IWCD
	  if (ccd_flag) {
	    /* output info of triphones needs special handling */
	    if (wchmm->winfo->wlen[word] == 1) { /* word with only 1 phone */
	      wchmm->outstyle[ntmp] = AS_LRSET;
	      wchmm->state[ntmp].out.lrset = (LRC_INFO *)mybmalloc2(sizeof(LRC_INFO), &(wchmm->malloc_root));
	      (wchmm->state[ntmp].out.lrset)->hmm       = ltmp;
	      (wchmm->state[ntmp].out.lrset)->state_loc = k;
#ifdef CATEGORY_TREE
	      (wchmm->state[ntmp].out.lrset)->category  = wchmm->winfo->wton[word];
#endif
	    } else if (j == 0) {	/* head phone of a word */
	      wchmm->outstyle[ntmp] = AS_RSET;
	      wchmm->state[ntmp].out.rset = (RC_INFO *)mybmalloc2(sizeof(RC_INFO), &(wchmm->malloc_root));
	      (wchmm->state[ntmp].out.rset)->hmm       = ltmp;
	      (wchmm->state[ntmp].out.rset)->state_loc = k;
	    } else if (j == wchmm->winfo->wlen[word] - 1) { /* last phone of a word */
	      wchmm->outstyle[ntmp] = AS_LSET;
	      wchmm->state[ntmp].out.lset = &(lcd->stateset[k]);
	    } else {
	      wchmm->outstyle[ntmp] = AS_STATE;
	      if (ltmp->is_pseudo) {
		j_printerr("Warning: word-internal phone should not be pseudo\n");
		put_voca(wchmm->winfo, word);
	      }
	      wchmm->state[ntmp].out.state = ltmp->body.defined->s[k];
	    }
	  } else {
	    /* monophone */
	    if (ltmp->is_pseudo) {
	      j_printerr("InternalError: CDSET phoneme exist in monophone?\n");
	      put_voca(wchmm->winfo, word);
	    }
	    wchmm->outstyle[ntmp] = AS_STATE;
	    wchmm->state[ntmp].out.state = ltmp->body.defined->s[k];
	  }
#else  /* ~PASS1_IWCD */
	  if (ltmp->is_pseudo) {
	    j_printerr("InternalError: CDSET phone exist in monophone?\n");
	    put_voca(wchmm->winfo, word);
	  }
	  wchmm->state[ntmp].out = ltmp->body.defined->s[k];
#endif /* PASS1_IWCD */
	  
 	  /* initialize other info */
  	  wchmm->state[ntmp].ac = NULL;
	  wchmm->stend[ntmp] = WORD_INVALID;
#ifndef MULTIPATH_VERSION
	  wchmm->ststart[ntmp] = WORD_INVALID;
	  /* make transition arc from HMM transition info */
	  for (ato = 1; ato < ltmp_state_num; ato++) {
	    prob = (hmm_logical_trans(ltmp))->a[k][ato];
	    if
	      (prob != LOG_ZERO)
	      {
	      if (j == add_tail && k == ltmp_state_num - 2 && ato == ltmp_state_num - 1) {
		/* arc outside new part will be handled later */
	      } else {
		add_wacc(wchmm, ntmp, prob, ntmp + ato - k);
	      }
	    }
	  }
#endif
	  
	  ntmp++;
	  /* expand wchmm if neccesary */
	  if (ntmp >= wchmm->maxwcn) wchmm_extend(wchmm);
        } /* end of state loop */
      }	/* end of phone loop */

#ifdef MULTIPATH_VERSION
	  
      /* On multipath version, the skip transition should be handled! */
      
      /* make transition arc from HMM transition info */
      ntmp = n;
      for (j = add_head; j <= add_tail; j++) {
	ltmp = wchmm->winfo->wseq[word][j];
	ltmp_state_num = hmm_logical_state_num(ltmp);
	out_num_next = 0;
	/* arc from initial state ... need arc expansion from precious phone */
	for (ato = 1; ato < ltmp_state_num; ato++) {
	  prob = (hmm_logical_trans(ltmp))->a[0][ato];
	  if (prob != LOG_ZERO) {
	    /* expand arc from previous HMM */
	    if (ato == ltmp_state_num - 1) {
	      /* to final state ... just register states for next expansion */
	      for(kkk=0; kkk<out_num_prev; kkk++) {
		out_from_next[out_num_next] = out_from[kkk];
		out_a_next[out_num_next] = out_a[kkk] + prob;
		out_num_next++;
	      }
	    } else {
	      for(kkk=0; kkk<out_num_prev; kkk++) {
		add_wacc(wchmm, out_from[kkk], out_a[kkk] + prob, ntmp + ato - 1);
	      }
	    }
	  }
	} /* end of state loop */
	/* from outprob state */
	for(k = 1; k < ltmp_state_num - 1; k++) {
	  for (ato = 1; ato < ltmp_state_num; ato++) {
	    prob = (hmm_logical_trans(ltmp))->a[k][ato];
	    if (prob != LOG_ZERO) {
	      if (ato == ltmp_state_num - 1) {
		/* to final state ... register states for next expansion */
		out_from_next[out_num_next] = ntmp;
		out_a_next[out_num_next] = prob;
		out_num_next++;
	      } else {
		add_wacc(wchmm, ntmp, prob, ntmp + ato - k);
	      }
	    }
	  }
	  ntmp++;
        } /* end of state loop */
	/* swap out list for next phone */
	for(kkk=0;kkk<out_num_next;kkk++) {
	  out_from[kkk] = out_from_next[kkk];
	  out_a[kkk] = out_a_next[kkk];
	}
	out_num_prev = out_num_next;
      }	/* end of phone loop */

#endif /* MULTIPATH_VERSION */
      
  } /* new phone node creation loop for this word */


#ifdef MULTIPATH_VERSION
  /*************************/
  /* Short Pause appending */
  /*************************/
  
  /* if -iwsp, add noise model to the end of word at ntmp */
  if (enable_iwsp && add_tail - add_head + 1 > 0) { /* there are new phones to be created */
    int ntmp_bak;
    
    /* set short pause state info */
    ntmp_bak = ntmp;
    if (wchmm->hmminfo->sp->is_pseudo) {
      for(k = 1;k < hmm_logical_state_num(wchmm->hmminfo->sp) - 1; k++) {
	wchmm->outstyle[ntmp] = AS_LSET;
	wchmm->state[ntmp].out.lset = &(wchmm->hmminfo->sp->body.pseudo->stateset[k]);
	wchmm->state[ntmp].ac = NULL;
	wchmm->stend[ntmp] = WORD_INVALID;
	ntmp++;
	if (ntmp >= wchmm->maxwcn) wchmm_extend(wchmm);
      }
    } else {
      for(k = 1;k < hmm_logical_state_num(wchmm->hmminfo->sp) - 1; k++) {
	wchmm->outstyle[ntmp] = AS_STATE;
	wchmm->state[ntmp].out.state = wchmm->hmminfo->sp->body.defined->s[k];
	wchmm->state[ntmp].ac = NULL;
	wchmm->stend[ntmp] = WORD_INVALID;
	ntmp++;
	if (ntmp >= wchmm->maxwcn) wchmm_extend(wchmm);
      }
    }
    ntmp = ntmp_bak;
    /* connect incoming arcs from previous phone */
    out_num_next = 0;
    for (ato = 1; ato < hmm_logical_state_num(wchmm->hmminfo->sp); ato++) {
      prob = hmm_logical_trans(wchmm->hmminfo->sp)->a[0][ato];
      if (prob != LOG_ZERO) {
	/* to control short pause insertion, transition probability toward
	 the word-end short pause will be given a penalty */
	prob += wchmm->hmminfo->iwsp_penalty;
	if (ato == hmm_logical_state_num(wchmm->hmminfo->sp) - 1) {
	  /* model has a model skip transition, just inherit them to next */
	  for(kkk=0; kkk<out_num_prev; kkk++) {
	    out_from_next[out_num_next] = out_from[kkk];
	    out_a_next[out_num_next] = out_a[kkk] + prob;
	    out_num_next++;
	  }
	} else {
	  /* connect incoming arcs from previous phone to this phone */
	  for(kkk=0; kkk<out_num_prev; kkk++) {
	    add_wacc(wchmm, out_from[kkk], out_a[kkk] + prob, ntmp + ato - 1);
	  }
	}
      }
    }
    /* if short pause model doesn't have a model skip transition, also add it */
    if (hmm_logical_trans(wchmm->hmminfo->sp)->a[0][hmm_logical_state_num(wchmm->hmminfo->sp)-1] == LOG_ZERO) {
      /* to make insertion sp model to have no effect on the original path,
	 the skip transition probability should be 0.0 (=100%) */
      prob = 0.0;
      for(kkk=0; kkk<out_num_prev; kkk++) {
	out_from_next[out_num_next] = out_from[kkk];
	out_a_next[out_num_next] = out_a[kkk] + prob;
	out_num_next++;
      }
    }
    /* connect arcs within model, and store new outgoing arcs for wordend node */
    for (k = 1; k < hmm_logical_state_num(wchmm->hmminfo->sp) - 1; k++) {
      for (ato = 1; ato < hmm_logical_state_num(wchmm->hmminfo->sp); ato++) {
	prob = hmm_logical_trans(wchmm->hmminfo->sp)->a[k][ato];
	if (prob != LOG_ZERO) {
	  if (ato == hmm_logical_state_num(wchmm->hmminfo->sp) - 1) {
	    out_from_next[out_num_next] = ntmp;
	    out_a_next[out_num_next] = prob;
	    out_num_next++;
	  } else {
	    add_wacc(wchmm, ntmp, prob, ntmp + ato - k);
	  }
	}
      }
      ntmp++;
    }
    /* swap work area for next */
    for(kkk=0;kkk<out_num_next;kkk++) {
      out_from[kkk] = out_from_next[kkk];
      out_a[kkk] = out_a_next[kkk];
    }
    out_num_prev = out_num_next;
  }

#endif /* MULTIPATH_VERSION */
	  
  /* make mapping: word <-> node on wchmm */
  for (j=0;j<word_len;j++) {
    if (j < add_head) {	/* shared part */
      wchmm->offset[word][j] = wchmm->offset[matchword][j];
    } else if (add_tail < j) { /* shared tail part (should not happen..) */
      wchmm->offset[word][j] = wchmm->offset[matchword][j+(matchword_len-word_len)];
    } else {			/* newly created part */
      wchmm->offset[word][j] = n;
      n += hmm_logical_state_num(wchmm->winfo->wseq[word][j]) - 2;
    }
  }
#ifdef MULTIPATH_VERSION

  /* paranoia check if the short-pause addition has been done well */
  if (enable_iwsp && add_tail - add_head + 1 > 0) {
    n += hmm_logical_state_num(wchmm->hmminfo->sp) - 2;
    if (n != ntmp) j_error("Algorithm Error! cannot match\n");
  }
  
  /* create word-end node */
  wchmm->wordend[word] = n;	/* tail node of 'word' is 'n' */
  wchmm->stend[n] = word;	/* node 'k' is a tail node of 'word' */
  wchmm->state[n].ac = NULL;
  wchmm->state[n].out.state = NULL;
  
  /* connect the final outgoing arcs in out_from[] to the word end node */
  for(k = 0; k < out_num_prev; k++) {
    add_wacc(wchmm, out_from[k], out_a[k], n);
  }
  n++;
  if (n >= wchmm->maxwcn) wchmm_extend(wchmm);

  if (matchlen == 0) {
    /* check if the new word has whole word-skipping transition */
    /* (use out_from and out_num_prev temporary) */
    out_num_prev = 0;
    get_outtrans_list(wchmm, word, word_len-1, out_from, out_a, &out_num_prev, wchmm->winfo->maxwn, TRUE);
    for(k=0;k<out_num_prev;k++) {
      if (out_from[k] == wchmm->wordbegin[word]) {
	j_printerr("\n*** ERROR: WORD SKIPPING TRANSITION NOT ALLOWED ***\n");
	j_printerr("  Word id=%d (%s[%s]) has \"word skipping transition\".\n", word, wchmm->winfo->wname[word], wchmm->winfo->woutput[word]);
	j_printerr("  All HMMs in the word:\n    ");
	for(kkk=0;kkk<wchmm->winfo->wlen[word];kkk++) {
	  j_printerr("%s ", wchmm->winfo->wseq[word][kkk]->name);
	}
	j_printerr("\n  has transitions from initial state to final state.\n");
	j_error("  This type of word skipping is not supported.\n");
      }
    }
  }
  
  wchmm->n = n;
  free(out_from);
  free(out_from_next);
  free(out_a);
  free(out_a_next);
  
#else  /* ~MULTIPATH_VERSION */
  
  wchmm->n = n;
  wchmm->ststart[wchmm->offset[word][0]] = word; /* word head */
  k = wchmm->offset[word][word_len-1] + hmm_logical_state_num(wchmm->winfo->wseq[word][word_len-1])-2 -1;
  wchmm->wordend[word] = k;	/* tail node of 'word' is 'k' */
  wchmm->stend[k] = word;	/* node 'k' is a tail node of 'word' */
  
  if (matchlen != 0 && add_tail - add_head + 1 > 0) {
    /* new part has been created in the above procedure: */
    /* now make link from shared part to the new part */
    wchmm_link_subword(wchmm, matchword,add_to,word,add_head);	
  }
  
#endif /* ~MULTIPATH_VERSION */
  
}

/**************************************************************/
/**** calculate overall info (after wchmm has been built) *****/
/**************************************************************/

#ifndef MULTIPATH_VERSION
/** 
 * <JA>
 * �ڹ�¤���������������ñ������ܷ׻��Τ����ñ�����Ƭ���֤�
 * ����ǥå������������롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Inspect the whole lexicon tree to generate list of word head states
 * for inter-word transition computation.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
wchmm_index_ststart(WCHMM_INFO *wchmm)
{
  int n;
  int id;

  id = 0;
  for (n=0;n<wchmm->n;n++) {
    if (wchmm->ststart[n] != WORD_INVALID) {
#ifdef USE_NGRAM
      /* ��Ƭñ��λ�ü�ؤ����ܤ����ʤ��Τǡ�����ǥå����˴ޤ�ʤ� */
      /* exclude silence model on beginning of a sentence from the index:
	 It cannot come after other words */
      if (wchmm->ststart[n] == wchmm->winfo->head_silwid) continue;
#endif
      wchmm->startnode[id] = n;
      id++;
      if (id > wchmm->winfo->num) {
	j_printerr("Error: start node num exceeded %d\n", wchmm->winfo->num);
      }
    }
  }
  wchmm->startnum = id;		/* total num */
}

/** 
 * <JA>
 * �ڹ�¤���������������ñ��ν�ü���֤��鳰�ؤμ����ܳ�Ψ�Υꥹ�Ȥ�������롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Scan the lexicon tree to make list of emission probability from the word end
 * state.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
wchmm_calc_wordend_arc(WCHMM_INFO *wchmm)
{
  WORD_ID w;
  HTK_HMM_Trans *tr;
  LOGPROB a;

  for (w=0;w<wchmm->winfo->num;w++) {
    tr = hmm_logical_trans(wchmm->winfo->wseq[w][wchmm->winfo->wlen[w]-1]);
    a = tr->a[tr->statenum-2][tr->statenum-1];
    wchmm->wordend_a[w] = a;
  }
}

#endif /* ~MULTIPATH_VERSION */

/********************************************************************/
/****** for separation (linearization) of high-frequent words *******/
/********************************************************************/
#ifdef USE_NGRAM
#ifdef SEPARATE_BY_UNIGRAM

/** 
 * <JA>
 * unigram��Ψ�ǥ����Ȥ��뤿��� qsort ������Хå��ؿ���
 * 
 * @param a [in] ����1
 * @param b [in] ����2
 * 
 * @return �黻�η�̤������֤���
 * </JA>
 * <EN>
 * qsort callback function to sort unigram values.
 * 
 * @param a [in] element #1
 * @param b [in] element #2
 * 
 * @return the result of comparison.
 * </EN>
 */
static int
compare_prob(LOGPROB *a, LOGPROB *b)
{
  if (*a < *b)  return (1);
  if (*a > *b)  return (-1);
  return(0);
}

/** 
 * <JA>
 * 1-gram�������ξ�� N ���ܤ��ͤ���롥
 * 
 * @param winfo [in] ñ�켭��
 * @param n [in] ������
 * 
 * @return ��� N ���ܤ� uni-gram ��Ψ���ͤ��֤���
 * </JA>
 * <EN>
 * Get the Nth-best unigram probability from all words.
 * 
 * @param winfo [in] word dictionary
 * @param n [in] required rank
 * 
 * @return the Nth-best unigram probability.
 * </EN>
 */
static LOGPROB
get_nbest_uniprob(WORD_INFO *winfo, int n)
{
  LOGPROB *u_p;
  WORD_ID w;
  LOGPROB x;

  if (n < 1) n = 1;
  if (n > winfo->num) n = winfo->num;

  /* store all unigram probability to u_p[] */
  u_p = (LOGPROB *)mymalloc(sizeof(LOGPROB) * winfo->num);
  for(w=0;w<winfo->num;w++) u_p[w] =
			      uni_prob(ngram, winfo->wton[w])
#ifdef CLASS_NGRAM
			      + winfo->cprob[w]
#endif
			      ;
  /* sort them downward */
  qsort(u_p, winfo->num, sizeof(LOGPROB),
	(int (*)(const void *,const void *))compare_prob);

  /* return the Nth value */
  x = u_p[n-1];
  free(u_p);
  return(x);
}

#endif
#endif /* USE_NGRAM */


/**********************************************************/
/****** MAKE WCHMM (LEXICON TREE) --- main function *******/
/**********************************************************/
static int separated_word_count; ///< Number of words actually separated (linearlized) from the tree

#ifdef CATEGORY_TREE

#define COUNT_STEP 500         ///< Word count step for debug progress output

/** 
 * <JA>
 * Ϳ����줿ñ�켭��ȸ����ǥ뤫���ڹ�¤��������ۤ��롥���δؿ���
 * �������٤���Julian��"-oldtree"���ץ���������Τ߻��Ѥ���ޤ������ץ����
 * �����������Julius�Ǥ������ build_wchmm2() ���Ѥ����ޤ���
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Build a tree lexicon from given word dictionary and language model.
 * This function is slow and only used when "-oldtree" option is specified
 * in Julian.  Julian without that option and Julius uses build_wchmm2()
 * instead of this.
 * 
 * @param wchmm [i/o] lexicon tree
 * </EN>
 */
void
build_wchmm(WCHMM_INFO *wchmm)
{
  int i,j;
  int matchword=0, sharelen=0, maxsharelen=0;
  int counter = COUNT_STEP;
#ifdef SEPARATE_BY_UNIGRAM
  LOGPROB separate_thres;
#endif

  /* lingustic infos must be set before build_wchmm() is called */
  /* check if necessary lingustic info is already assigned (for debug) */
  if (wchmm->winfo == NULL
#ifdef USE_NGRAM
      || wchmm->ngram == NULL
#endif
#ifdef USE_DFA
      || wchmm->dfa == NULL
#endif
      ) {
    j_error("InternalError: build_wchmm: lingustic info not available!!\n");
  }
  
#ifdef SEPARATE_BY_UNIGRAM
  /* ���[separate_wnum]���ܤ�1-gram����������� */
  /* 1-gram�������������Ͱʾ�Τ�Τ��ڤ���ʬ���� */
  separate_thres = get_nbest_uniprob(wchmm->winfo, separate_wnum);
#endif

#ifdef PASS1_IWCD
#ifdef CATEGORY_TREE
  if (ccd_flag && !old_iwcd_flag) {
    /* ���ƤΥ��ƥ���ID�դ� lcd_set ����� */
    lcdset_register_with_category_all(wchmm, wchmm->hmminfo, winfo, dfa);
  }
#endif /* CATEGORY_TREE */
#endif /* PASS1_IWCD */
  

  /* wchmm������ */
  wchmm_init(wchmm);

  /* �����󥿥ꥻ�å� */
  separated_word_count=0;

  j_printerr("Building HMM lexicon tree (left-to-right)...\n");
  for (i=0;i<wchmm->winfo->num;i++) {
    if (verbose_flag) {
      if (i >= counter) {
       j_printerr("\r %5d words proceeded (%6d nodes)",i,wchmm->n);
       counter += COUNT_STEP;
      }
    }
#ifdef USE_NGRAM
    if (i == wchmm->winfo->head_silwid || i == wchmm->winfo->tail_silwid) {
      /* ��Ƭ/������̵����ǥ���ڹ�¤��������
       * ��Ƭ��̵��ñ�����Ƭ�ؤ����ܡ�����ñ���������������ܤϺ��ʤ�*/
      wchmm_add_word(wchmm, i,0,0); /* sharelen=0�Ǥ��Τޤ� */
      continue;
    }
#ifndef NO_SEPARATE_SHORT_WORD
    if (wchmm->winfo->wlen[i] <= SHORT_WORD_LEN) {
      /* Ĺ����û��ñ����ڹ�¤�����ʤ�(�����Ǥ�1����) */
      wchmm_add_word(wchmm, i,0,0); /* sharelen=0�Ǥ��Τޤ� */
      separated_word_count++;
      continue;
    }
#endif
#ifdef SEPARATE_BY_UNIGRAM
    if (
       uni_prob(wchmm->ngram, wchmm->winfo->wton[i])
#ifdef CLASS_NGRAM
       + wchmm->winfo->cprob[i]
#endif
       >= separate_thres && separated_word_count < separate_wnum) {
      /* ���٤ι⤤ñ����ڹ�¤�����ʤ� */
      /* separate_thres �Ͼ��separate_wnum���ܤΥ����� */
      wchmm_add_word(wchmm, i,0,0);
      separated_word_count++;
      continue;
    }
#endif
#endif /* USE_NGRAM */
    /* �Ǥ�Ĺ�����Ǥ�ͭ�����ñ���õ�� */
    maxsharelen=0;
    for (j=0;j<i;j++) {
#ifdef CATEGORY_TREE
      if (wchmm->winfo->wton[i] != wchmm->winfo->wton[j]) continue;
#endif
      sharelen = wchmm_check_match(wchmm->winfo, i, j);
      if (sharelen == wchmm->winfo->wlen[i] && sharelen == wchmm->winfo->wlen[j]) {
       /* word ��Ʊ���줬¸�ߤ��� */
       /* ɬ�������Ĺ���Ǥ��ꡤ��ʣ������Ȥ��򤱤뤿�ᤳ����ȴ���� */
       maxsharelen = sharelen;
       matchword = j;
       break;
      }
      if (sharelen > maxsharelen) {
       matchword = j;
       maxsharelen = sharelen;
      }
    }
    wchmm_add_word(wchmm, i,maxsharelen,matchword);
  }
  j_printerr("\r %5d words ended     (%6d nodes)\n",i,wchmm->n);

#if 0
  /* �ڹ�¤����ʤ� */
  for (i=0;i<wchmm->winfo->num;i++) {
    if (verbose_flag) {
      if (i >= counter) {
       j_printerr("  %5d words proceeded (%6d nodes)\n",i,wchmm->n);
       counter += COUNT_STEP;
      }
    }
    wchmm_add_word(wchmm, i,0,0); /* sharelen=0�Ǥ��Τޤ� */
  }
  j_printerr("  %5d words ended     (%6d nodes)\n",i,wchmm->n);
#endif  

#ifndef MULTIPATH_VERSION
  /* Ʊ�첻�Ƿ�������ñ��Ʊ�Τ� leaf node ��2�Ų����ƶ��̤��� */
  wchmm_duplicate_leafnode(wchmm);
  VERMES("  %d leaf nodes are made unshared\n",dupcount);
  
  /* ñ��ν�ü���鳰�ؤ����ܳ�Ψ����Ƥ��� */
  wchmm_calc_wordend_arc(wchmm);
#endif

  /* wchmm��������������å����� */
  check_wchmm(wchmm);

#ifndef MULTIPATH_VERSION
  /* ñ�����Ƭ�Ρ��� ststart ���ֹ�� �̤˳�Ǽ */
  wchmm_index_ststart(wchmm);
#endif

  /* factoring�Ѥ˳ƾ��֤˸�³ñ��Υꥹ�Ȥ��ղä��� */
#ifndef CATEGORY_TREE
  make_successor_list(wchmm);
#ifdef USE_NGRAM
#ifdef UNIGRAM_FACTORING
  /* ����ä�factoring�ͤ�׻� */
  /* ��ü�ʳ���sc��ɬ�פʤ��Τǥե꡼���� */
  calc_all_unigram_factoring_values(wchmm);
  j_printerr("  1-gram factoring values has been pre-computed\n");
#endif /* UNIGRAM_FACTORING */
#endif /* USE_NGRAM */
#ifdef MULTIPATH_VERSION
  /* ���ۤ��줿 factoring ����򥹥��å����ܤ����ʸƬʸˡ�Ρ��ɤ˥��ԡ� */
  adjust_sc_index(wchmm);
#endif
#ifdef USE_NGRAM
#ifdef UNIGRAM_FACTORING
  /* ñ���LM����å��夬ɬ�פʥΡ��ɤΥꥹ�Ȥ��� */
  make_iwcache_index(wchmm);
#endif /* UNIGRAM_FACTORING */
#endif /* USE_NGRAM */
#endif /* CATEGORY_TREE */

  j_printerr("done\n");

  /* ��ư�� -check �ǥ����å��⡼�ɤ� */
  if (wchmm_check_flag) {
    wchmm_check_interactive(wchmm);
  }
}

#endif /* CATEGORY_TREE */

/** 
 * <JA>
 * Ϳ����줿ñ�켭��ȸ����ǥ뤫���ڹ�¤��������ۤ��롥
 * ���δؿ��� bulid_wchmm() ��Ʊ��������Ԥ��ޤ�����
 * �ǽ��ñ�������ǥ����Ȥ��Ʋ�����λ������ñ����¤٤뤿�ᡤ
 * ����®���ڹ�¤����Ԥ����Ȥ��Ǥ��롥�Ȥ��˥��ץ�������򤷤ʤ�
 * �¤ꡤJulius/Julian�ǤϤ����餬�Ѥ����롥
 * 
 * @param wchmm [i/o] �ڹ�¤������
 * </JA>
 * <EN>
 * Build a tree lexicon from given word dictionary and language model.
 * This function does the same job as build_wchmm(), but it is much
 * faster because finding of the longest matched word to an adding word
 * is done by first sorting all the words in the dictoinary by their phoneme
 * sequence order.  This function will be used instead of build_wchmm()
 * by default.
 * 
 * @param wchmm [i/o] lexicon tree
 * </EN>
 */  
void
build_wchmm2(WCHMM_INFO *wchmm)
{
  int i,j, last_i;
  int count_step, counter;
  WORD_ID *windex;
#ifdef USE_NGRAM
#ifdef SEPARATE_BY_UNIGRAM
  LOGPROB separate_thres;
#endif
#endif

  /* lingustic infos must be set before build_wchmm() is called */
  /* check if necessary lingustic info is already assigned (for debug) */
  if (wchmm->winfo == NULL
#ifdef USE_NGRAM
      || wchmm->ngram == NULL
#endif
#ifdef USE_DFA
      || wchmm->dfa == NULL
#endif
      ) {
    j_error("InternalError: build_wchmm: lingustic info not available!!\n");
  }
  
  separated_word_count = 0;
  count_step = wchmm->winfo->num / 10;
  counter = count_step;
  
  j_printerr("Building HMM lexicon tree");
  
#ifdef USE_NGRAM
#ifdef SEPARATE_BY_UNIGRAM
  /* compute score threshold beforehand to separate words from tree */
  /* here we will separate best [separate_wnum] words from tree */
  separate_thres = get_nbest_uniprob(wchmm->winfo, separate_wnum);
#endif
#endif

#ifdef PASS1_IWCD
#ifdef CATEGORY_TREE
  if (ccd_flag && !old_iwcd_flag) {
    /* when Julian mode (category-tree) and triphone is used,
       make all category-indexed context-dependent phone set (cdset) here */
    /* these will be assigned on the last phone of each word on tree */
    lcdset_register_with_category_all(wchmm, wchmm->hmminfo, winfo, dfa);
  }
#endif /* CATEGORY_TREE */
#endif /* PASS1_IWCD */
  
  /* initialize wchmm */
  wchmm_init(wchmm);

  /* make sorted word index ordered by phone sequence */
  windex = (WORD_ID *)mymalloc(sizeof(WORD_ID) * wchmm->winfo->num);
  for(i=0;i<wchmm->winfo->num;i++) windex[i] = i;
#ifdef CATEGORY_TREE
  /* sort by category -> sort by word ID in each category */
  wchmm_sort_idx_by_category(wchmm->winfo, windex, wchmm->winfo->num);
  {
    int last_cate;
    last_i = 0;
    last_cate = wchmm->winfo->wton[windex[0]];
    for(i = 1;i<wchmm->winfo->num;i++) {
      if (wchmm->winfo->wton[windex[i]] != last_cate) {
	wchmm_sort_idx_by_wseq(wchmm->winfo, windex, last_i, i - last_i);
	last_cate = wchmm->winfo->wton[windex[i]];
	last_i = i;
      }
    }
    wchmm_sort_idx_by_wseq(wchmm->winfo, windex, last_i, wchmm->winfo->num - last_i);
  }
#else
  /* sort by word ID for whole vocabulary */
  wchmm_sort_idx_by_wseq(wchmm->winfo, windex, 0, wchmm->winfo->num);
#endif

/* 
 *   {
 *     int i,w;
 *     for(i=0;i<wchmm->winfo->num;i++) {
 *	 w = windex[i];
 *	 printf("%d: cate=%4d wid=%4d %s\n",i, wchmm->winfo->wton[w], w, wchmm->winfo->woutput[w]);
 *     }
 *   }
 */

  /* incrementaly add words to lexicon tree */
  /* now for each word, the previous word (last_i) is always the most matched one */
  last_i = WORD_INVALID;
  for (j=0;j<wchmm->winfo->num;j++) {
    i = windex[j];
    if (j >= counter) {
      /*j_printerr("\r %5d words proceeded (%6d nodes)",j, wchmm->n);*/
      j_printerr(".");
      counter += count_step;
    }
#ifdef USE_NGRAM
    /* start/end silence word should not be shared */
    if (i == wchmm->winfo->head_silwid || i == wchmm->winfo->tail_silwid) {
      wchmm_add_word(wchmm, i,0,0); /* add whole word as new (sharelen=0) */
      continue;
    }
#ifndef NO_SEPARATE_SHORT_WORD
    /* separate short words from tree */
    if (wchmm->winfo->wlen[i] <= SHORT_WORD_LEN) {
      wchmm_add_word(wchmm, i,0,0);
      separated_word_count++;
      continue;
    }
#endif
#ifdef SEPARATE_BY_UNIGRAM
    /* separate high-frequent words from tree (threshold = separate_thres) */
    if (
	uni_prob(wchmm->ngram, wchmm->winfo->wton[i])
#ifdef CLASS_NGRAM
	+ wchmm->winfo->cprob[i]
#endif
	>= separate_thres && separated_word_count < separate_wnum) {
      wchmm_add_word(wchmm, i,0,0);
      separated_word_count++;
      continue;
    }
#endif
#endif /* USE_NGRAM */
    if (last_i == WORD_INVALID) { /* first word */
      wchmm_add_word(wchmm, i,0,0);
    } else {
      /* the previous word (last_i) is always the most matched one */
#ifdef CATEGORY_TREE
      if (wchmm->winfo->wton[i] != wchmm->winfo->wton[last_i]) {
	wchmm_add_word(wchmm, i,0,0);
      } else {
	wchmm_add_word(wchmm, i, wchmm_check_match(wchmm->winfo, i, last_i), last_i);
      }
#else
      wchmm_add_word(wchmm, i, wchmm_check_match(wchmm->winfo, i, last_i), last_i);
#endif
    }
    last_i = i;
    
  } /* end of add word loop */
  
  /*j_printerr("\r %5d words ended     (%6d nodes)\n",j,wchmm->n);*/

  /* free work area */
  free(windex);

#ifdef MULTIPATH_VERSION
  j_printerr("%d nodes\n", wchmm->n);
#else
  /* duplicate leaf nodes of homophone/embedded words */
  j_printerr("%d", wchmm->n);
  wchmm_duplicate_leafnode(wchmm);
  j_printerr("+%d=%d nodes\n",dupcount, wchmm->n);
#endif

#ifndef MULTIPATH_VERSION
  /* calculate transition probability of word end node to outside */
  wchmm_calc_wordend_arc(wchmm);
#endif

  /* check wchmm coherence (internal debug) */
  check_wchmm(wchmm);

#ifndef MULTIPATH_VERSION
  /* make index of word-beginning nodes (for inter-word transition) */
  wchmm_index_ststart(wchmm);
#endif

  /* make successor list for all branch nodes for N-gram factoring */
#ifndef CATEGORY_TREE
  make_successor_list(wchmm);
#ifdef UNIGRAM_FACTORING
  /* for 1-gram factoring, we can compute the values before search */
  calc_all_unigram_factoring_values(wchmm);
  j_printerr("  1-gram factoring values has been pre-computed\n");
#endif /* UNIGRAM_FACTORING */
#ifdef MULTIPATH_VERSION
  /* Copy the factoring data according to the skip transitions and startword nodes */
  adjust_sc_index(wchmm);
#endif
#ifdef UNIGRAM_FACTORING
  /* make list of start nodes that needs inter-word LM cache */
  make_iwcache_index(wchmm);
#endif /* UNIGRAM_FACTORING */
#endif

  j_printerr("done\n");

  /* go into interactive check mode ("-check" on start) */
  if (wchmm_check_flag) {
    wchmm_check_interactive(wchmm);
  }

#ifdef WCHMM_SIZE_CHECK
  /* detailed check of lexicon tree size (inaccurate!) */
  j_printf("--- estimated size of word lexicon ---\n");
  j_printf("wchmm: %d words, %d nodes\n", wchmm->winfo->num, wchmm->n);
  j_printf("%9d bytes: wchmm->state[node] (exclude ac, sc)\n", sizeof(WCHMM_STATE) * wchmm->n);
#ifndef MULTIPATH_VERSION
  j_printf("%9d bytes: wchmm->ststart[node]\n", sizeof(WORD_ID) * wchmm->n);
#endif
  printf("%9d bytes: wchmm->stend[node]\n", sizeof(WORD_ID) * wchmm->n);
  {
    int w,count;
    count = 0;
    for(w=0;w<wchmm->winfo->num;w++) {
      count += wchmm->winfo->wlen[w] * sizeof(int) + sizeof(int *);
    }
    printf("%9d bytes: wchmm->offset[w][]\n", count);
  }
#ifdef MULTIPATH_VERSION
  j_printf("%9d bytes: wchmm->wordbegin[w]\n", wchmm->winfo->num * sizeof(int));
#endif
  j_printf("%9d bytes: wchmm->wordend[w]\n", wchmm->winfo->num * sizeof(int));
  j_printf("%9d bytes: wchmm->startnode[]\n", wchmm->startnum * sizeof(int));
#ifdef MULTIPATH_VERSION
#ifdef CATEGORY_TREE
  j_printf("%9d bytes: wchmm->start2wid[]\n", wchmm->startnum * sizeof(WORD_ID));
#endif
#endif
#ifdef UNIGRAM_FACTORING
  printf("%9d bytes: wchmm->start2isolate[]\n", wchmm->isolatenum * sizeof(int));
#endif
  printf("%9d bytes: wchmm->wordend_a[]\n", wchmm->winfo->num * sizeof(LOGPROB));
  printf("%9d bytes: wchmm->outstyle[]\n", wchmm->n * sizeof(unsigned char));
#ifndef CATEGORY_TREE
  printf("%9d bytes: wchmm->sclist[]\n", wchmm->scnum * sizeof(S_CELL *));
  printf("%9d bytes: wchmm->sclist2node[]\n", wchmm->scnum * sizeof(int));
#ifdef UNIGRAM_FACTORING
  printf("%9d bytes: wchmm->fscore[]\n", wchmm->fsnum * sizeof(LOGPROB));
#endif  
#endif
  
  printf("under state[]:\n");
  {
    A_CELL *ac;
    int count,n;
    count = 0;
    for(n=0;n<wchmm->n;n++) {
      for(ac=wchmm->state[n].ac;ac;ac=ac->next) {
	count += sizeof(A_CELL);
      }
    }
    printf("\t%9d: ac\n", count);
  }
#ifndef CATEGORY_TREE
  {
    S_CELL *sc;
    int count,n;
    count = 0;
    for(n=0;n<wchmm->n;n++) {
      for(sc=wchmm->state[n].sc;sc;sc=sc->next) {
	count += sizeof(S_CELL);
      }
    }
    printf("\t%9d: sc\n", count);
  }
#endif
#endif /* WCHMM_SIZE_CHECK */
}


/** 
 * <JA>
 * �ڹ�¤������Υ������ʤɤξ����ɸ����Ϥ˽��Ϥ��롥
 * 
 * @param wchmm [in] �ڹ�¤������
 * </JA>
 * <EN>
 * Output some specifications of the tree lexicon (size etc.) to stdout.
 * 
 * @param wchmm [in] tree lexicon already built
 * </EN>
 */
void
print_wchmm_info(WCHMM_INFO *wchmm)
{
  int n,i, rootnum;

#ifdef MULTIPATH_VERSION
  rootnum = wchmm->startnum;
#else
#ifdef USE_NGRAM
  rootnum = wchmm->startnum + 1;	/* including winfo->head_silwid */
#else
  rootnum = wchmm->startnum;
#endif /* USE_NGRAM */
#endif /* MULTIPATH_VERSION */
  
  j_printf("Lexicon tree info:\n");
  j_printf("\t total node num = %6d\n", wchmm->n);
#ifdef USE_NGRAM
  j_printf("\t  root node num = %6d\n", rootnum);
#ifdef NO_SEPARATE_SHORT_WORD
#ifdef SEPARATE_BY_UNIGRAM
  j_printf("\t(%d hi-freq. words are separated from tree lexicon)\n", separated_word_count);
#else
  j_printf(" (no words are separated from tree)\n");
#endif /* SEPARATE_BY_UNIGRAM */
#else
  j_printf(" (%d short words (<= %d phonemes) are separated from tree)\n", separated_word_count, SHORT_WORD_LEN);
#endif /* NO_SEPARATE_SHORT_WORD */
#else /* USE_NGRAM */
  j_printf("\t  root node num = %6d\n", rootnum);
#endif /* USE_NGRAM */
  for(n=0,i=0;i<wchmm->n;i++) {
    if (wchmm->stend[i] != WORD_INVALID) n++;
  }
  j_printf("\t  leaf node num = %6d\n", n);
#ifndef CATEGORY_TREE
  j_printf("\t fact. node num = %6d\n", wchmm->scnum - 1);
#endif /* CATEGORY_TREE */
}
