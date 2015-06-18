/**
 * @file   ngram_decode.c
 * @author Akinobu Lee
 * @date   Fri Jul  8 14:57:51 2005
 * 
 * <JA>
 * @brief  N-gram��Ψ�ȥȥ�ꥹ���ñ�줫�鼡ñ���ͽ¬����(��2�ѥ�)��
 *
 * Julius ��N-gram���Ѥ��������å��ǥ����ǥ���(��2�ѥ�)�ˤ����ơ�
 * ������³������ñ��ν������ꤹ�롥
 * 
 * Ϳ����줿Ÿ��������λ�ü�ե졼���ͽ¬����ñ��ȥ�ꥹ���
 * ����ͽ¬�ե졼����դ˽�ü��¸�ߤ���ñ��ν����
 * ����N-gram�и���Ψ�ȤȤ���֤���
 *
 * Julius �Ǥ� ngram_firstwords(), ngram_nextwords(), ngram_acceptable() ��
 * ���줾����2�ѥ��Υᥤ��ؿ� wchmm_fbs() ����ƤӽФ���롥�ʤ���
 * Julian �ǤϤ����δؿ�������� dfa_decode.c �δؿ����Ѥ����롥
 * </JA>
 * 
 * <EN>
 * @brief  Word prediction based on N-gram probability and word trellis index
 * for 2nd pass of Julius.
 *
 * These functions returns next word candidates in the 2nd recognition
 * pass of Julius, i.e. N-gram based stack decoding.
 *
 * Given a partial sentence hypothesis, it first estimate the beginning frame
 * of the hypothesis based on the word trellis.  Then the words in the word
 * trellis around the estimated frame are extracted from the word trellis.
 * They will be returned with their N-gram probabilities.
 *
 * In Julius, ngram_firstwords(), ngram_nextwords() and ngram_acceptable()
 * are called from main search function wchmm_fbs().  In Julian,
 * corresponding functions in dfa_decode.c will be used instead.
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

#ifdef USE_NGRAM

/** 
 * <JA>
 * ��ñ������ñ��ID�Ǿ��祽���Ȥ��뤿��� qsort ������Хå��ؿ���
 * 
 * @param a [in] ����1
 * @param b [in] ����2
 * 
 * @return a��ñ��ID > b��ñ��ID �ʤ�1, �դʤ� -1, Ʊ���ʤ� 0 ���֤���
 * </JA>
 * <EN>
 * qsort callback function to sort next word candidates by their word ID.
 * 
 * @param a [in] element 1
 * @param b [in] element 2
 * 
 * @return 1 if word id of a > that of b, -1 if negative, 0 if equal.
 * </EN>
 */
static int
compare_nw(NEXTWORD **a, NEXTWORD **b)
{
  if ((*a)->id > (*b)->id) return 1;
  if ((*a)->id < (*b)->id) return -1;
  return 0;
}

/** 
 * <JA>
 * ���ꤵ�줿ñ���ñ�����ꥹ���⤫�鸡�����롥
 * 
 * @param nw [in] ��ñ�����ꥹ��
 * @param w [in] ��������ñ���ID
 * @param num [in] ��ñ�����ꥹ�Ȥ�Ĺ��
 * 
 * @return ���Ĥ��ä���礽�μ�ñ����乽¤�ΤؤΥݥ��󥿡����Ĥ���ʤ����
 * NULL ���֤���
 * </JA>
 * <EN>
 * Find a word from list of next word candidates.
 * 
 * @param nw [in] list of next word candidates
 * @param w [in] word id to search for
 * @param num [in] length of @a nw
 * 
 * @return the pointer to the NEXTWORD data if found, or NULL if not found.
 * </EN>
 */
/* find next word candiate whose id 'w' */
static NEXTWORD *
search_nw(NEXTWORD **nw, WORD_ID w, int num)
{
  int left,right,mid;
  NEXTWORD *tmp;

  if (num == 0) return NULL;
  left = 0;
  right = num - 1;
  while (left < right) {
    mid = (left + right) / 2;
    if ((nw[mid])->id < w) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  tmp = nw[left];
  if (tmp->id == w) {
    return tmp;
  } else {
    return NULL;
  }
}

/*********** transparent words handling on 2nd pass **********/
static WORD_ID cnword[2];	///< Last two non-transparent words
static int cnnum;		///< Num of found non-transparent words (<=2)
static int last_trans;		///< Num of skipped transparent words
/** 
 * <JA>
 * ñ���󤫤�Ʃ��ñ��Ǥʤ��Ǹ�Σ�ñ�����Ф���cnword �˥��åȤ��롥
 * 
 * @param cseq [in] ñ����
 * @param n [in] @a cseq ��Ĺ��
 * @param winfo [in] ñ�����¤��
 * </JA>
 * <EN>
 * Set last two non-transparent words in the given word sequence and
 * set them to cnword.
 * 
 * @param cseq [in] word sequence
 * @param n [in] length of @a cseq
 * @param winfo [in] word dictionary information
 * </EN>
 */
static void
set_word_context(WORD_ID *cseq, int n, WORD_INFO *winfo)
{
  int i;

  cnnum = 0;
  last_trans = 0;
  for(i=n-1;i>=0;i--) {
    if (! winfo->is_transparent[cseq[i]]) {
      cnword[cnnum++] = winfo->wton[cseq[i]];
      if (cnnum >= 2) break;
    } else {
      last_trans++;
    }
  }
}

/*************************************************************/

/* lookup survived words on specified frame in backtrellis, 
   compute their N-gram probabilities, and add them to NEXTWORD data */
/** 
 * <JA>
 * @brief  ñ��ȥ�ꥹ��μ�ñ�����Ф���
 *
 * ñ��ȥ�ꥹ��λ��ꤷ���ե졼���˽�ü��¸�ߤ���ȥ�ꥹñ��
 * �Υꥹ�Ȥ���Ф��������μ�ñ��Ȥ��Ƥ� N-gram ��³��Ψ��׻����롥
 * ���Υꥹ�Ȥ�ñ�����¤�Τ��ɲä����֤���
 * 
 * @param bt [in] ñ��ȥ�ꥹ��¤��
 * @param winfo [in] ñ�켭��¤��
 * @param ngram [in] N-gram��¤��
 * @param nw [i/o] ��ñ�����ꥹ�ȡ���з�̤� @a oldnum �ʹߤ��ɲä�����
 * @param oldnum [in] @a nw �ˤ��Ǥ˳�Ǽ����Ƥ��뼡ñ��ο�
 * @param hypo [in] Ÿ������ʸ����
 * @param t [in] ����ե졼��
 * 
 * @return ��Хꥹ�Ȥ��ɲä������Ȥ� @a nw �˴ޤޤ�뼡ñ��������
 * </JA>
 * <EN>
 * @brief  Extract next word candidates from word trellis.
 *
 * This function extracts the list of trellis words whose word end
 * has survived in the word trellis at the specified frame.
 * The N-gram probabilities of them are then computed and added to
 * the current next word candidates data.
 * 
 * @param bt [in] word trellis structure
 * @param winfo [in] word dictionary structure
 * @param ngram [in] N-gram data structure
 * @param nw [in] list of next word candidates (new words will be appended at @a oldnum)
 * @param oldnum [in] number of words already stored in @a nw
 * @param hypo [in] the source sentence hypothesis
 * @param t [in] specified frame
 * 
 * @return the total number of words currently stored in the @a nw.
 * </EN>
 */
static int
pick_backtrellis_words(BACKTRELLIS *bt, WORD_INFO *winfo, NGRAM_INFO *ngram, NEXTWORD **nw, int oldnum, NODE *hypo, short t)
{
  int i;
  WORD_ID w;
  LOGPROB rawscore;
#ifdef WPAIR
  int w_old = WORD_INVALID;
#endif
  int num;

  num = oldnum;
  /* set word context to cnword[0], cnword[1] */
  set_word_context(hypo->seq, hypo->seqnum, winfo);
  /* lookup survived words in backtrellis on time frame 't' */
  for (i=0;i<bt->num[t];i++) {
    w = (bt->rw[t][i])->wid;
#ifdef WORD_GRAPH
    /* only words on the word graphs are expanded */
    if (!(bt->rw[t][i])->within_wordgraph) continue;
#endif /* not WORD_GRAPH */
#ifdef WPAIR
    /* some word have same word ID with different previous word, so
       only one will be opened (best word will be selected later
       by next_word() */
    if (w == w_old) continue;	/* backtrellis is sorted by word ID */
    else w_old = w;
#endif /* WPAIR */
    /* skip if already exist */
    if (search_nw(nw, w, oldnum) != NULL) continue;
    switch(cnnum) {		/* length of valid context */
    case 0:			/* unigram */
      rawscore = uni_prob(ngram, winfo->wton[w]);
      break;
    case 1:			/* bigram */
      rawscore = bi_prob_rl(ngram, winfo->wton[w], cnword[0]);
      break;
    default:			/* trigram */
      rawscore = tri_prob_rl(ngram, winfo->wton[w], cnword[0], cnword[1]);
      break;
    }
#ifdef CLASS_NGRAM
    rawscore += winfo->cprob[w];
#endif
    nw[num]->tre   = bt->rw[t][i];
    nw[num]->id    = w;
    nw[num]->lscore = lm_weight2 * rawscore + lm_penalty2;
    if (winfo->is_transparent[w]) {
      /*nw[num]->lscore -= (LOGPROB)last_trans * TRANS_RENZOKU_PENALTY;*/
      if (winfo->is_transparent[hypo->seq[hypo->seqnum-1]]) {
	nw[num]->lscore += lm_penalty_trans;
      }
    }
    
    /* j_printf("%d: %s added\n", num, winfo->wname[nw[num]->id]); */
    num++;
  }
  return num;
}

/* Look for survived backtrellis words near the specified frame, and
   make NEXTWORD list.
   Words in frame [tm-lookup_range..tm+lookup_range-1] will be picked up.
   If a word exist in several frames, only one near the center frame
   will be taken: the true connection point will be determined later at
   next_word() */
/** 
 * <JA>
 * @brief  ����ե졼����դ�ñ��ȥ�ꥹ���鼡ñ�콸�����ꤹ�롥
 *
 * ����ե졼������� lookup_range ʬ�˽�ü������ȥ�ꥹ���ñ��򽸤ᡤ
 * ��ñ�칽¤�Τ��ۤ��롥Ʊ��ñ�줬�嵭���ϰ����ʣ�������硤
 * ����ե졼��ˤ�äȤ�ᤤ�ȥ�ꥹ���ñ�줬���򤵤�롥
 * 
 * @param bt [in] ñ��ȥ�ꥹ��¤��
 * @param winfo [in] ñ�켭��¤��
 * @param ngram [in] ñ��N-gram��¤��
 * @param nw [out] ��ñ�콸����Ǽ���빽¤�ΤؤΥݥ���
 * @param hypo [in] Ÿ��������ʬʸ����
 * @param tm [in] ñ���õ���濴�Ȥʤ����ե졼��
 * @param t_end [in] ñ���õ���ե졼��α�ü
 * 
 * @return @a nw �˳�Ǽ���줿��ñ�����ο����֤���
 * </JA>
 * <EN>
 * @brief  Look for the next word candidates on the word trellis near the
 * specified time frame.
 *
 * This function builds a list of next word candidates by looking up
 * the word trellis at specified frame, with lookup_range frame margin.
 * If the same words exists in the near frames, only the one nearest to the
 * specified frame will be chosen.
 * 
 * @param bt [in] word trellis structure
 * @param winfo [in] word dictionary structure
 * @param ngram [in] word N-gram structure
 * @param nw [out] pointer to hold the extracted words as list of next word candidates
 * @param hypo [in] partial sentence hypothesis from which the words will be expanded
 * @param tm [in] center time frame to look up the words
 * @param t_end [in] right frame boundary for the lookup.
 * 
 * @return the number of next words candidates stored in @a nw.
 * </EN>
 */
int
get_backtrellis_words(BACKTRELLIS *bt, WORD_INFO *winfo, NGRAM_INFO *ngram, NEXTWORD **nw, NODE *hypo, short tm, short t_end)
{
  int num = 0;
  int t, t_step;
  int oldnum=0;

  if (tm < 0) return(0);

#ifdef PREFER_CENTER_ON_TRELLIS_LOOKUP
  /* fix for 3.2 (01/10/18 by ri) */
  /* before and after (one near center frame has high priority) */
  for (t_step = 0; t_step < lookup_range; t_step++) {
    /* before or center */
    t = tm - t_step;
    if (t < 0 || t > bt->framelen - 1 || t >= t_end) continue;
    num = pick_backtrellis_words(bt, winfo, ngram, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
    if (t_step == 0) continue;	/* center */
    /* after */
    t = tm + t_step;
    if (t < 0 || t > bt->framelen - 1 || t >= t_end) continue;
    num = pick_backtrellis_words(bt, winfo, ngram, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
  }

#else

  /* before the center frame */
  for(t = tm; t >= tm - lookup_range; t--) {
    if (t < 0) break;
    num = pick_backtrellis_words(bt, winfo, ngram, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
  }
  /* after the center frame */
  for(t = tm + 1; t < tm + lookup_range; t++) {
    if (t > bt->framelen - 1) break;
    if (t >= t_end) break;
    num = pick_backtrellis_words(bt, winfo, ngram, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
  }
#endif

  return num;
}

/** 
 * <JA>
 * ����ˤ��Ÿ���оݤȤʤ�ʤ�ñ���ꥹ�Ȥ���õ�롥
 * 
 * @param nw [i/o] ��ñ�콸��ʽ������Ÿ���Ǥ��ʤ�ñ�줬�õ����
 * @param hypo [in] Ÿ��������ʬʸ����
 * @param num [in] @a nw �˸��߳�Ǽ����Ƥ���ñ���
 * 
 * @return ������ nw �˴ޤޤ�뼡ñ���
 * </JA>
 * <EN>
 * Remove words in the nextword list which should not be expanded.
 * 
 * @param nw [i/o] list of next word candidates (will be shrinked by removing some words)
 * @param hypo [in] partial sentence hypothesis from which the words will be expanded
 * @param num [in] current number of next words in @a nw
 * 
 * @return the new number of words in @a nw
 * </EN>
 */
int
limit_nw(NEXTWORD **nw, NODE *hypo, int num)
{
  int src,dst;
  int newnum;

  /* <s>����ϲ���Ÿ�����ʤ� */
  /* no hypothesis will be generated after "<s>" */
  if (hypo->seq[hypo->seqnum-1] == winfo->head_silwid) {
    return(0);
  }

  dst = 0;
  for (src=0; src<num; src++) {
    if (nw[src]->id == winfo->tail_silwid) {
      /* </s> ��Ÿ�����ʤ� */
      /* do not expand </s> (it only appears at start) */
      continue;
    }
#ifdef FIX_35_INHIBIT_SAME_WORD_EXPANSION
    /* ľ��ñ���Ʊ���ȥ�ꥹñ���Ÿ�����ʤ� */
    /* inhibit expanding the exactly the same trellis word twice */
    if (nw[src]->tre == hypo->tre) continue;
#endif
    
    if (src != dst) memcpy(nw[dst], nw[src], sizeof(NEXTWORD));
    dst++;
  }
  newnum = dst;

  return newnum;
}
	


/* �ǽ��ñ�췲���֤����֤���: ñ��� (-1 on error) */
/* return initial word set.  return value: num of words (-1 on error) */

/** 
 * <JA>
 * @brief  ���ñ�첾�⽸����֤���
 *
 * N-gram�١�����õ���Ǥϡ���������ñ��������̵��ñ��˸��ꤵ��Ƥ��롥
 * �����������硼�ȥݡ����������ơ��������ϡ���1�ѥ��Ǻǽ��ե졼��˽�ü��
 * �Ĥä�ñ���������ٺ����ñ��Ȥʤ롥
 * 
 * @param nw [out] ��ñ�����ꥹ�ȡ�����줿���ñ�첾����Ǽ�����
 * @param peseqlen [in] ���ϥե졼��Ĺ
 * @param maxnw [in] @a nw �˳�Ǽ�Ǥ���ñ��κ����
 * @param winfo [in] ñ�����¤��
 * @param bt [in] ñ��ȥ�ꥹ��¤��
 * 
 * @return @a nw �˳�Ǽ���줿ñ���������֤���
 * </JA>
 * <EN>
 * @brief  Return the set of initial word hypotheses at the beginning.
 *
 * on N-gram based recogntion, the initial hypothesis is fixed to the tail
 * silence word.  Exception is that, in short-pause segmentation mode, the
 * initial hypothesis will be chosen from survived words on the last input
 * frame in the first pass.
 * 
 * @param nw [out] pointer to hold the initial word candidates
 * @param peseqlen [in] input frame length
 * @param maxnw [in] maximum number of words that can be stored in @a nw
 * @param winfo [in] word dictionary information
 * @param bt [in] word trellis structure
 * 
 * @return the number of words extracted and stored to @a nw.
 * </EN>
 */
int
ngram_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, WORD_INFO *winfo, BACKTRELLIS *bt)
{
#ifdef SP_BREAK_CURRENT_FRAME
  if (rest_param != NULL) {
    /* �������� �ǽ��ե졼��˻Ĥä�ñ��ȥ�ꥹ��κ���ñ�� */
    /* the initial hypothesis is the best word survived on the last frame of
       the segment */
    nw[0]->id = sp_break_2_begin_word;
  } else {
    /* �ǽ���������: �������� ñ���������̵��ñ��(=winfo->tail_silwid) */
    /* we are in the last of sentence: initial hypothesis is word-end silence word */
    nw[0]->id = winfo->tail_silwid;
  }
#else
  /* initial hypothesis is word-end silence word */
  nw[0]->id = winfo->tail_silwid;
#endif
#ifdef FIX_PENALTY
  nw[0]->lscore = 0.0;
#else
  nw[0]->lscore = lm_penalty2;
#endif

  return 1;			/* number of words = 1 */
}

/* ���벾��μ�����³ñ�췲���֤���������: ñ��� (-1 on error) */
/* return next word set from the hypothesis.  return value:
   num of words (-1 on error) */
/** 
 * <JA>
 * @brief ��ñ�첾�⽸����֤���
 *
 * Ϳ����줿��ʬʸ���⤫�顤������³������ñ��ν�����֤����ºݤˤϡ�
 * ��1�ѥ��η�̤Ǥ���ȥ�ꥹñ�콸�� bt ��ǡ�Ÿ��������ʬʸ����κǽ�ñ���
 * �ʿ��ꤵ�줿�˻�ü�ե졼�� hypo->estimated_next_t �������¸�ߤ���
 * ñ�콸����Ф��������� N-gram ��³��Ψ��׻������֤���
 * ���Ф��줿��ñ�첾��ϡ����餫���� maxnm ��Ĺ������
 * �ΰ褬���ݤ���Ƥ��� nw �˳�Ǽ����롥
 * 
 * @param hypo [in] Ÿ������ʸ����
 * @param nw [out] ��ñ�����ꥹ�Ȥ��Ǽ�����ΰ�ؤΥݥ���
 * @param maxnw [in] @a nw �κ���Ĺ
 * @param ngram [in] N-gram����¤��
 * @param winfo [in] �������¤��
 * @param bt [in] ñ��ȥ�ꥹ��¤��
 * 
 * @return ��Ф��� nw �˳�Ǽ���줿��ñ�첾��ο����֤���
 * </JA>
 * <EN>
 * @brief  Return the list of next word candidate.
 *
 * Given a partial sentence hypothesis "hypo", it returns the list of
 * next word candidates.  Actually, it extracts from word trellis the
 * list of words whose word-end node has survived near the estimated
 * beginning-of-word frame of last word "hypo->estimated_next_t", and store
 * them to "nw" with their N-gram probabilities. 
 * 
 * @param hypo [in] source partial sentence hypothesis
 * @param nw [out] pointer to store the list of next word candidates (should be already allocated)
 * @param maxnw [in] maximum number of words that can be stored to @a nw
 * @param ngram [in] word N-gram
 * @param winfo [in] word dictionary
 * @param bt [in] word trellis structure
 * 
 * @return the number of extracted next word candidates in @a nw.
 * </EN>
 */
int
ngram_nextwords(
		NODE *hypo,
		NEXTWORD **nw,
		int maxnw,	/* hypo: source */
		NGRAM_INFO *ngram, /* N-gram info */
		WORD_INFO *winfo, /* word dictionary */
		BACKTRELLIS *bt) /* backtrellis info */
{
  int num, num2;

  if (hypo->seqnum == 0) {
    j_error("gs_get_next_words: hypo contains no word\n");
  }

  /* ����ο��꽪ü����ˤ����� backtrellis��˻ĤäƤ���ñ������� */
  /* get survived words on backtrellis at the estimated end frame */
  num = get_backtrellis_words(bt, winfo, ngram, nw, hypo, hypo->estimated_next_t, hypo->bestt);

  if (debug2_flag) j_printf("%d",num);

  /* Ÿ���Ǥ��ʤ�ñ�������å����Ƴ��� */
  /* exclude unallowed words */
  num2 = limit_nw(nw, hypo, num);

  if (debug2_flag) j_printf("-%d=%d unfolded\n",num-num2,num2);

  return(num2);
}

/* return if the hypothesis is "acceptable" */
/** 
 * <JA>
 * Ϳ����줿��ʬʸ���⤬��ʸ�ʤ��ʤ��õ����λ�ˤȤ���
 * ������ǽ�Ǥ��뤫�ɤ������֤���N-gram �Ǥ�ʸƬ���б�����̵��ñ��
 * (silhead) �Ǥ���м������롥
 * 
 * @param hypo [in] ��ʬʸ����
 * @param winfo [in] ñ�켭�����
 * 
 * @return ʸ�Ȥ��Ƽ�����ǽ�Ǥ���� TRUE���Բ�ǽ�ʤ� FALSE ���֤���
 * </JA>
 * <EN>
 * Return whether the given partial hypothesis is acceptable as a sentence
 * and can be treated as a final search candidate.  In N-gram mode, it checks
 * whether the last word is the beginning-of-sentence silence (silhead).
 * 
 * @param hypo [in] partial sentence hypothesis to be examined
 * @param winfo [in] word dictionary
 * 
 * @return TRUE if acceptable as a sentence, or FALSE if not.
 * </EN>
 */
boolean
ngram_acceptable(NODE *hypo, WORD_INFO *winfo)
{
  if (
#ifdef SP_BREAK_CURRENT_FRAME
      /* �Ǹ�β��⤬�裱�ѥ����ಾ��κǽ��ñ��Ȱ��פ��ʤ���Фʤ�ʤ� */
      /* the last word should be equal to the first word on the best hypothesis on 1st pass */
      hypo->seq[hypo->seqnum-1] == sp_break_2_end_word
#else
      /* �Ǹ�β��⤬ʸƬ̵��ñ��Ǥʤ���Фʤ�ʤ� */
      /* the last word should be head silence word */
      hypo->seq[hypo->seqnum-1] == winfo->head_silwid
#endif
      ) {
    return TRUE;
  } else {
    return FALSE;
  }
}

#endif /* USE_NGRAM */
