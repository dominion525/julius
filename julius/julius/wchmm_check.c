/**
 * @file   wchmm_check.c
 * @author Akinobu Lee
 * @date   Sat Sep 24 15:45:06 2005
 * 
 * <JA>
 * @brief  �ڹ�¤������Υ桼�������å��⡼��
 *
 * �����Ǥϡ�Ϳ����줿ñ�켭��ȸ����ǥ뤫���������줿�ڹ�¤������ι�¤��
 * ����Ū�˥����å����뤿��δؿ����������Ƥ��ޤ�����ư���� "-check wchmm"
 * �Ȥ��뤳�Ȥǡ��ڹ�¤������ι��۸�˥ץ��ץȤ�ɽ�����졤����ñ�줬
 * �ڹ�¤������Τɤ��˰��֤��뤫�����뤤�Ϥ���Ρ��ɤˤɤΤ褦�ʾ���
 * ��Ϳ����Ƥ��뤫�ʤɤ�Ĵ�٤뤳�Ȥ��Ǥ��ޤ���
 * </JA>
 * 
 * <EN>
 * @brief  User inspection mode of tree lexicon
 *
 * This file defines some functions to browse and check the structure
 * of the tree lexicon at startup time. When invoking with "-check wchmm",
 * it will enter to a prompt mode after tree lexicon is generated, and
 * you can check its structure, e.g. how the specified word is located in the
 * tree lexicon, or what kind of information a node has in it.
 * </EN>
 * 
 * $Revision: 1.4 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

/** 
 * <JA>
 * ñ��μ���������Ϥ���
 * 
 * @param winfo [in] ñ�켭��
 * @param word [in] ���Ϥ���ñ���ID
 * </JA>
 * <EN>
 * Display informations of a word in the dictionary.
 * 
 * @param winfo [in] word dictionary
 * @param word [in] ID of a word to be displayed.
 * </EN>
 */
static void
print_winfo_w(WORD_INFO *winfo, WORD_ID word)
{
  int i;
  if (word >= winfo->num) return;
  j_printf("--winfo\n");
  j_printf("wname   = %s\n",winfo->wname[word]);
  j_printf("woutput = %s\n",winfo->woutput[word]);
  j_printf("\ntransp  = %s\n", (winfo->is_transparent[word]) ? "yes" : "no");
  j_printf("wlen    = %d\n",winfo->wlen[word]);
  j_printf("wseq    =");
  for (i=0;i<winfo->wlen[word];i++) {
    j_printf(" %s",winfo->wseq[word][i]->name);
  }
  j_printf("\nwseq_def=");
  for (i=0;i<winfo->wlen[word];i++) {
    if (winfo->wseq[word][i]->is_pseudo) {
      j_printf(" (%s)", winfo->wseq[word][i]->body.pseudo->name);
    } else {
      j_printf(" %s",winfo->wseq[word][i]->body.defined->name);
    }
  }
  j_printf("\nwton    = %d\n",winfo->wton[word]);
#ifdef CLASS_NGRAM
  j_printf("cprob   = %f(%f)\n", winfo->cprob[word], pow(10.0, winfo->cprob[word]));
#endif
  
}

/** 
 * <JA>
 * �ڹ�¤��������ñ��ΰ��־������Ϥ��롥
 * 
 * @param wchmm [in] �ڹ�¤������
 * @param word [in] ñ��ID
 * </JA>
 * <EN>
 * Display the location of a word in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param word [in] word ID
 * </EN>
 */
static void
print_wchmm_w(WCHMM_INFO *wchmm, WORD_ID word)
{
  int i;
  if (word >= wchmm->winfo->num) return;
  j_printf("--wchmm (word)\n");
  j_printf("offset  =");
  for (i=0;i<wchmm->winfo->wlen[word];i++) {
    j_printf(" %d",wchmm->offset[word][i]);
  }
  j_printf("\n");
#ifdef MULTIPATH_VERSION
  j_printf("wordbegin = %d\n",wchmm->wordbegin[word]);
#endif
  j_printf("wordend = %d\n",wchmm->wordend[word]);
}

/** 
 * <JA>
 * �ڹ�¤�������Τ���Ρ��ɤξ������Ϥ��롥
 * 
 * @param wchmm [in] �ڹ�¤������
 * @param node [in] �Ρ����ֹ�
 * </JA>
 * <EN>
 * Display informations assigned to a node in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param node [in] node id
 * </EN>
 */
static void
print_wchmm_s(WCHMM_INFO *wchmm, int node)
{
  j_printf("--wchmm (node)\n");
#ifndef MULTIPATH_VERSION
  j_printf("ststart = %d\n",wchmm->ststart[node]);
#endif
  j_printf("stend   = %d\n",wchmm->stend[node]);
#ifdef MULTIPATH_VERSION
  if (wchmm->state[node].out.state == NULL) {
    j_printf("NO OUTPUT\n");
    return;
  }
#endif
#ifdef PASS1_IWCD
  j_printf("outstyle= ");
  switch(wchmm->outstyle[node]) {
  case AS_STATE:
    j_printf("AS_STATE (id=%d)\n", (wchmm->state[node].out.state)->id);
    break;
  case AS_LSET:
    j_printf("AS_LSET  (%d variants)\n", (wchmm->state[node].out.lset)->num);
    break;
  case AS_RSET:
    if ((wchmm->state[node].out.rset)->hmm->is_pseudo) {
      j_printf("AS_RSET  (name=\"%s\", pseudo=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.rset)->hmm->name,
	       (wchmm->state[node].out.rset)->hmm->body.pseudo->name,
	       (wchmm->state[node].out.rset)->state_loc);
    } else {
      j_printf("AS_RSET  (name=\"%s\", defined=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.rset)->hmm->name,
	       (wchmm->state[node].out.rset)->hmm->body.defined->name,
	       (wchmm->state[node].out.rset)->state_loc);
    }
    break;
  case AS_LRSET:
    if ((wchmm->state[node].out.rset)->hmm->is_pseudo) {
      j_printf("AS_LRSET  (name=\"%s\", pseudo=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.lrset)->hmm->name,
	       (wchmm->state[node].out.lrset)->hmm->body.pseudo->name,
	       (wchmm->state[node].out.lrset)->state_loc);
    } else {
      j_printf("AS_LRSET  (name=\"%s\", defined=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.lrset)->hmm->name,
	       (wchmm->state[node].out.lrset)->hmm->body.defined->name,
	       (wchmm->state[node].out.lrset)->state_loc);
    }
    break;
  default:
    j_printf("UNKNOWN???\n");
  }
#endif /* PASS1_IWCD */
}

/** 
 * <JA>
 * �ڹ�¤�������Τ���Ρ��ɤˤĤ��ơ�������Υꥹ�Ȥ���Ϥ��롥
 * 
 * @param wchmm [in] �ڹ�¤������
 * @param node [in] �Ρ����ֹ�
 * </JA>
 * <EN>
 * Display list of transition arcs from a node in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param node [in] node ID
 * </EN>
 */
static void
print_wchmm_s_arc(WCHMM_INFO *wchmm, int node)
{
  A_CELL *ac;
  int i = 0;
  j_printf("arcs:\n");
  for (ac=wchmm->state[node].ac;ac;ac=ac->next) {
    j_printf(" %d %f(%f)\n",ac->arc,ac->a,pow(10.0, ac->a));
    i++;
  }
  j_printf(" total %d arcs\n",i);
}

#ifndef CATEGORY_TREE
/** 
 * <JA>
 * �ڹ�¤�������Τ���Ρ��ɤλ��� factoring �������Ϥ��롥
 * 
 * @param wchmm [in] �ڹ�¤������
 * @param node [in] �Ρ����ֹ�
 * </JA>
 * <EN>
 * Display factoring values on a node in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param node [in] node ID
 * </EN>
 */
static void
print_wchmm_s_successor(WCHMM_INFO *wchmm, int node)
{
  S_CELL *sc;
  int i = 0;
  int scid;

  scid = wchmm->state[node].scid;
  if (scid == 0) {
    j_printf("no successors\n");
  } else if (scid < 0) {
    j_printf("successor id: %d\n", scid);
#ifdef UNIGRAM_FACTORING
    j_printf("1-gram factoring node: score=%f\n",wchmm->fscore[-scid]);
#endif
  } else {
    j_printf("successor id: %d\n", scid);
    for (sc=wchmm->sclist[scid];sc;sc=sc->next) {
      j_printf(" %d\n",sc->word);
      i++;
    }
    j_printf(" total %d successors\n",i);
  }
}
#endif

/** 
 * <JA>
 * ���ꤵ�줿����̾��HMM�򸡺��������ξ������Ϥ��롥
 * 
 * @param name [in] ����HMM��̾��
 * </JA>
 * <EN>
 * Lookup an HMM of given name, and display specs of it.
 * 
 * @param name [in] HMM logical name
 * </EN>
 */
static void
print_hmminfo(char *name)
{
  HMM_Logical *l;

  l = htk_hmmdata_lookup_logical(hmminfo, name);
  if (l == NULL) {
    j_printf("no HMM named \"%s\"\n", name);
  } else {
    put_logical_hmm(l);
  }
}

#ifdef USE_NGRAM
/** 
 * <JA>
 * ñ��N-gram�Τ���ñ��ξ������Ϥ��롥
 * 
 * @param ngram [in] ñ��N-gram
 * @param nid [in] N-gramñ���ID
 * </JA>
 * <EN>
 * Display specs of a word in the word N-gram
 * 
 * @param ngram [in] word N-gram
 * @param nid [in] N-gram word ID
 * </EN>
 */
static void
print_ngraminfo(NGRAM_INFO *ngram, int nid)
{
  j_printf("-- N-gram entry --\n");
  j_printf("nid  = %d\n", nid);
  j_printf("name = %s\n", ngram->wname[nid]);
}
#endif


/** 
 * <JA>
 * �ڹ�¤������ι�¤��ư��������Ū�˥����å�����ݤΥ��ޥ�ɥ롼��
 * 
 * @param wchmm [in] �ڹ�¤������
 * </JA>
 * <EN>
 * Command loop to browse and check the structure of the constructed tree
 * lexicon on startup.
 * 
 * @param wchmm [in] tree lexicon
 * </EN>
 */
void
wchmm_check_interactive(WCHMM_INFO *wchmm) /* interactive check */
{
  static const int len = 24;
  char buf[len], *name;
  int arg, newline;
  WORD_ID argw;
  boolean endflag;

  j_printf("\n\n");
  j_printf("********************************************\n");
  j_printf("********  LM & LEXICON CHECK MODE  *********\n");
  j_printf("********************************************\n");
  j_printf("\n");

  for (endflag = FALSE; endflag == FALSE;) {
    j_printf("===== syntax: command arg (\"H\" for help) > ");
    if (fgets(buf, len, stdin) == NULL) break;
    name = "";
    arg = 0;
    if (isalpha(buf[0]) != 0 && buf[1] == ' ') {
      newline = strlen(buf)-1;
      if (buf[newline] == '\n') {
	buf[newline] = '\0';
      }
      if (buf[2] != '\0') {
	name = buf + 2;
	arg = atoi(name);
      }
    }
    switch(buf[0]) {
    case 'w':			/* word info */
      argw = arg;
      print_winfo_w(wchmm->winfo, argw);
      print_wchmm_w(wchmm, argw);
      break;
    case 'n':			/* node info */
      print_wchmm_s(wchmm, arg);
      break;
    case 'a':			/* arc list */
      print_wchmm_s_arc(wchmm, arg);
      break;
#if 0
    case 'r':			/* reverse arc list */
      print_wchmm_r_arc(arg);
      break;
#endif
#ifndef CATEGORY_TREE
    case 's':			/* successor word list */
      print_wchmm_s_successor(wchmm, arg);
      break;
#endif
    case 't':			/* node total info of above */
      print_wchmm_s(wchmm, arg);
      print_wchmm_s_arc(wchmm, arg);
#if 0
      print_wchmm_r_arc(arg);
#endif
#ifndef CATEGORY_TREE
      print_wchmm_s_successor(wchmm, arg);
#endif
      break;
    case 'h':			/* hmm state info */
      print_hmminfo(name);
      break;
#ifdef USE_NGRAM
    case 'l':			/* N-gram language model info */
      print_ngraminfo(wchmm->ngram, arg);
      break;
#endif
    case 'q':			/* quit */
      endflag = TRUE;
      break;
    default:			/* help */
      j_printf("syntax: [command_character] [number(#)]\n");
      j_printf("  w [word_id] ... show word info\n");
      j_printf("  n [state]   ... show wchmm state info\n");
      j_printf("  a [state]   ... show arcs from the state\n");
#if 0
      j_printf("  r [state]   ... show arcs  to  the state\n");
#endif
      j_printf("  s [state]   ... show successor list of the state\n");
      j_printf("  h [hmmname] ... show HMM info of the name\n");
#ifdef USE_NGRAM
      j_printf("  l [nwid]    ... N-gram entry info\n");
#endif
      j_printf("  H           ... print this help\n");
      j_printf("  q           ... quit\n");
      break;
    }
  }
  j_printf("\n");
  j_printf("********************************************\n");
  j_printf("*****  END OF LM & LEXICON CHECK MODE  *****\n");
  j_printf("********************************************\n");
  j_printf("\n");
}


/** 
 * <JA>
 * �ڹ�¤��������Υ�󥯾���ΰ����������å�����������ǥХå��ѡ�
 * 
 * @param wchmm [in] �ڹ�¤������
 * </JA>
 * <EN>
 * Check coherence of tree lexicon (for internal debug only!)
 * 
 * @param wchmm [in] tree lexicon
 * </EN>
 */
void
check_wchmm(WCHMM_INFO *wchmm)
{
  int i,n;
  boolean ok_flag;
  A_CELL *ac;
#ifdef MULTIPATH_VERSION
  int node;
  WORD_ID w;
#endif

  ok_flag = TRUE;

#ifdef MULTIPATH_VERSION
  
  /* check word-beginning nodes */
  for(i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    if (wchmm->state[node].out.state != NULL) {
      j_printf("Error: word-beginning node %d has output function!\n, node");
      ok_flag = FALSE;
    }
  }
  /* examine if word->state and state->word mapping is correct */
  for(w=0;w<wchmm->winfo->num;w++) {
    if (wchmm->stend[wchmm->wordend[w]] != w) {
      j_printf("Error: no match of word end for word %d!!\n", w);
      ok_flag = FALSE;
    }
  }
  
#else
  
  /* examine if word->state and state->word mapping is correct */
  for (i=0;i<winfo->num;i++) {
    if (wchmm->stend[wchmm->wordend[i]]!=i) {
      j_printf("end ga awanai!!!: word=%d, node=%d, value=%d\n",
	       i, wchmm->wordend[i], wchmm->stend[wchmm->wordend[i]]);
      ok_flag = FALSE;
    }
    if (wchmm->ststart[wchmm->offset[i][0]] == WORD_INVALID) {
      j_printf("word start node is WORD_INVALID:word=%d, node=%d, value=%d\n",
	       i, wchmm->offset[i][0], wchmm->ststart[wchmm->offset[i][0]]);
      ok_flag = FALSE;
    }
  }

#endif /* MULTIPATH_VERSION */

  /* check if the last state is unique and has only one output arc */
  i = 0;
  for (n=0;n<wchmm->n;n++) {
    if (wchmm->stend[n] != WORD_INVALID) {
      i++;
      for (ac=wchmm->state[n].ac; ac; ac=ac->next) {
	if (ac->arc == n) continue;
#ifndef MULTIPATH_VERSION
	if (wchmm->ststart[ac->arc] != WORD_INVALID) continue;
#endif
	break;
      }
      if (ac != NULL) {
	j_printf("node %d is shared?\n",n);
	ok_flag = FALSE;
      }
    }
  }
  if (i != wchmm->winfo->num ) {
    j_printf("num of heads of words in wchmm not match word num!!\n");
    j_printf("from wchmm->stend:%d != from winfo:%d ?\n",i,wchmm->winfo->num);
    ok_flag = FALSE;
  }

  /* if check failed, go into interactive mode */
  if (!ok_flag) {
    wchmm_check_interactive(wchmm);
  }

  VERMES("  coordination check passed\n");
}

