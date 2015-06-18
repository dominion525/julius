/**
 * @file   ngram_access.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 07:46:18 2005
 * 
 * <JA>
 * @brief  単語列・クラス列の N-gram 確率を求める
 *
 * </JA>
 * 
 * <EN>
 * @brief  Get N-gram probability of a word/class sequence.
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

#include <sent/stddefs.h>
#include <sent/ngram2.h>

/** 
 * Search for 2-gram tuple (@a w_l, @a w_r) in the 2-gram part of N-gram.
 * 
 * @param ndata [in] word/class N-gram
 * @param w_l [in] left word/class ID in N-gram
 * @param w_r [in] right word/class ID in N-gram
 * 
 * @return corresponding index to the 2-gram data part if found, or
 * NNID_INVALID if the tuple does not exist in 2-gram.
 */
NNID
search_bigram(NGRAM_INFO *ndata, WORD_ID w_l, WORD_ID w_r)
{
  /* do binary search */
  /* assume that data in (bigrams) are ordered by wid */
  NNID left,right,mid;		/* n2 */

  if ((left = ndata->n2_bgn[w_l]) == NNID_INVALID) /* has no bigram */
    return (NNID_INVALID);
  right = left + ndata->n2_num[w_l] - 1;
  while(left < right) {
    mid = (left + right) / 2;
    if (ndata->n2tonid[mid] < w_r) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (ndata->n2tonid[left] == w_r) {
    return (left);
  } else {
    return (NNID_INVALID);
  }
}

/** 
 * Search for a reverse 3-gram tuple (@a wkey, w1, w2), where the context
 * 2-gram tuple (w1, w2) should exist on 2-gram data part at @a n2.
 *
 * (for v3 struture format)
 * 
 * @param ndata [in] word/class N-gram
 * @param n2 [in] 2-gram data part where context 2-gram data exists.
 * @param wkey [in] left word/class ID in N-gram
 * 
 * @return corresponding index to the 3-gram data part if found, or
 * NNID_INVALID if the 3-gram does not exist.
 */
static NNID
search_trigram_v3(NGRAM_INFO *ndata, NNID n2, WORD_ID wkey)
{
  /* do binary search */
  /* assume that data in (trigrams) are ordered by wid */
  int left,right,mid;

  if ((left = ndata->n3_bgn[n2]) == NNID_INVALID) 	/* has no bigram */
    return (NNID_INVALID);
  right = left + ndata->n3_num[n2] - 1;
  while(left < right) {
    mid = (left + right) / 2;
    if (ndata->n3tonid[mid] < wkey) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (ndata->n3tonid[left] == wkey) {
    return (left);
  } else {
    return (NNID_INVALID);
  }
}

/** 
 * Search for a reverse 3-gram tuple (@a wkey, w1, w2), where the context
 * 2-gram tuple (w1, w2) should exist on 2-gram data part at @a n2.  
 * 
 * (for v4 struture format)
 * 
 * @param ndata [in] word/class N-gram
 * @param n2 [in] 2-gram data part where context 2-gram data exists.
 * @param wkey [in] left word/class ID in N-gram
 * 
 * @return corresponding index to the 3-gram data part if found, or
 * NNID_INVALID if the 3-gram does not exist.
 */
static NNID
search_trigram_v4(NGRAM_INFO *ndata, NNID n2, WORD_ID wkey)
{
  /* do binary search */
  /* assume that data in (trigrams) are ordered by wid */
  NNID left,right,mid,boid;

  if ((boid = ndata->n2bo_upper[n2]) == NNID_INVALID_UPPER) 	/* has no bigram */
     return (NNID_INVALID);
  boid = (boid << 16) + (NNID)(ndata->n2bo_lower[n2]);
  left = ((NNID)(ndata->n3_bgn_upper[boid]) << 16) + (NNID)(ndata->n3_bgn_lower[boid]);
  right = left + ndata->n3_num[boid] - 1;
  while(left < right) {
    mid = (left + right) / 2;
    if (ndata->n3tonid[mid] < wkey) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (ndata->n3tonid[left] == wkey) {
    return (left);
  } else {
    return (NNID_INVALID);
  }
}


/* ---------------------------------------------------------------------- */
/* for 1-gram */

/** 
 * Get 1-gram probability of @f$w@f$ in log10.
 *
 * @param ndata [in] word/class N-gram
 * @param w [in] word/class ID in N-gram
 * 
 * @return log10 probability @f$\log p(w)@f$.
 */
LOGPROB
uni_prob(NGRAM_INFO *ndata, WORD_ID w)
{
  if (w != ndata->unk_id) {
    return(ndata->p[w]);
  } else {
    return(ndata->p[w] - ndata->unk_num_log);
  }
}

/* for 2-gram */
/** 
 * Get LR 2-gram probability of word/class sequence @f$(w_1, w_2)@f$ in
 * log10
 * 
 * @param ndata [in] word/class N-gram
 * @param w1 [in] left word/class ID in N-gram
 * @param w2 [in] right word/class ID in N-gram (to compute probability)
 * 
 * @return log10 probability @f$\log p(w_2|w_1)@f$.
 */
LOGPROB
bi_prob_lr(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  NNID n2;
  LOGPROB prob;

  if ((n2 = search_bigram(ndata, w1, w2)) != NNID_INVALID) {
    /* bigram exist */
    prob = ndata->p_lr[n2];
  } else {
    /* bigram not exist, return back-off prob */
    /* bo_wt_lr(w1) * p(w2) */
    prob = ndata->bo_wt_lr[w1] + ndata->p[w2];
  }
  if (w2 != ndata->unk_id) {
    return(prob);
  } else {
    return(prob - ndata->unk_num_log);
  }
}

/** 
 * Get RL 2-gram probability of word/class sequence @f$(w_1, w_2)@f$ in
 * log10.
 * 
 * @param ndata [in] word/class N-gram
 * @param w1 [in] left word/class ID in N-gram (to compute probability)
 * @param w2 [in] right word/class ID in N-gram
 * 
 * @return log10 probability @f$\log p(w_1|w_2)@f$.
 */
LOGPROB
bi_prob_rl(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  NNID n2;
  LOGPROB prob;

  if ((n2 = search_bigram(ndata, w1, w2)) != NNID_INVALID) {
    /* bigram exist */
    prob = ndata->p_rl[n2];
  } else {
    /* bigram not exist, return back-off prob */
    /* bo_wt_rl(w2) * p(w1) */
    prob = ndata->bo_wt_rl[w2] + ndata->p[w1];
  }
  if (w1 != ndata->unk_id) {
    return(prob);
  } else {
    return(prob - ndata->unk_num_log);
  }
}

/* for 3-gram */
/** 
 * Get RL 3-gram probability of word/class sequence @f$(w_1, w_2, w_3)@f$ in
 * log10.
 * 
 * @param ndata [in] word/class N-gram
 * @param w1 [in] left word/class ID in N-gram (to compute probability)
 * @param w2 [in] middle word/class ID in N-gram
 * @param w3 [in] right word/class ID in N-gram
 * 
 * @return log10 probability @f$\log p(w_1|w_2, w_3)@f$.
 */
LOGPROB
tri_prob_rl(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2, WORD_ID w3)
{
  NNID n2, n3;
  int boid;
  
  if ((n2 = search_bigram(ndata, w2, w3)) != NNID_INVALID) {
    switch(ndata->version) {
    case 4:
      n3 = search_trigram_v4(ndata, n2, w1);
      break;
    case 3:
      n3 = search_trigram_v3(ndata, n2, w1);
      break;
    }
    if (n3 != NNID_INVALID) {
      /* trigram exist */
      if (w1 != ndata->unk_id) {
	return(ndata->p_rrl[n3]);
      } else {
	return(ndata->p_rrl[n3] - ndata->unk_num_log);
      }
    } else {
      /* return back-off prob */
      /* bo_wt_rl(w2,w3) * p(w1|w2) */
      /* unk will be discounted at bi-gram */
      switch(ndata->version) {
      case 4:
	if ((boid = ndata->n2bo_upper[n2]) == NNID_INVALID_UPPER) {	/* has no bigram */
	  return(bi_prob_rl(ndata, w1, w2));
	} else {
	  boid = (boid << 16) + (NNID)(ndata->n2bo_lower[n2]);
	  return(ndata->bo_wt_rrl[boid] + bi_prob_rl(ndata, w1, w2)); 
	}
	break;
      case 3:
	return(ndata->bo_wt_rrl[n2] + bi_prob_rl(ndata, w1, w2));
	break;
      }
    }
  }
  /* context not exist, so return bigram prob */
  return(bi_prob_rl(ndata, w1, w2));
}
