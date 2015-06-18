/**
 * @file   gprune_safe.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 05:28:12 2005
 * 
 * <JA>
 * @brief  混合ガウス分布計算: Gaussian pruning (safe algorithm)
 *
 * gprune_safe()は混合ガウス分布集合の計算ルーチンの一つです．
 * safe pruning を使って上位のガウス分布の出力確率のみを高速に求めます．
 * Tied-mixture %HMM 使用時に Julius でGPRUNE_DEFAULT_SAFE が定義されているか，
 * あるいはJuliusのオプション "-gprune safe" を指定することでこの関数が
 * 使用されます．
 *
 * safe pruning は最も安全な枝刈り法です．上位N個のガウス分布が確実に
 * 得られますが，高速化の効果は他の手法に比べて小さいです．
 * 
 * gprune_safe() は outprob_init() によってその関数へのポインタが
 * compute_gaussset にセットされることで使用されます．このポインタが
 * calc_tied_mix() または calc_mix() から呼び出されます．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate probability of a set of Gaussian densities by
 * Gaussian pruning: safe algorithm
 *
 * gprune_safe() is one of the functions to compute output probability of
 * a set of Gaussian densities.  This function does safe pruning, trying
 * to compute only the best ones for faster computation.  If a tied-mixture
 * %HMM model with GPRUNE_DEFAULT_SAFE defined in Julius, or explicitly
 * specified by "-gprune safe" option, this function will be used.
 *
 * The safe pruning is the most safe method that can find the exact N-best
 * Gaussians, but the efficiency is smaller.
 *
 * gprune_safe() will be used by calling outprob_init() to set its pointer
 * to the global variable @a compute_gaussset.  Then it will be called from
 * calc_tied_mix() or calc_mix().
 * </EN>
 * 
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* gprune_safe.c --- calculate probability of Gaussian densities */
/*                   with Gaussian pruning (safe) */

/* $Id: gprune_safe.c,v 1.6 2006/12/14 08:18:36 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>
#include "globalvars.h"

static boolean *mixcalced;	///< Mark which Gaussian has been computed

/** 
 * @brief  Calculate probability with safe pruning.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * performing pruning using the scholar threshold.
 * 
 * @param binfo [in] Gaussian density
 * @param thres [in] threshold
 * 
 * @return the output log probability.
 */
LOGPROB
compute_g_safe(HTK_HMM_Dens *binfo, LOGPROB thres)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec = OP_vec;
  short veclen = OP_veclen;
  VECT fthres = thres * (-2.0);

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  tmp = binfo->gconst;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if (tmp > fthres)  return LOG_ZERO;
  }
  return(tmp * -0.5);
}



/** 
 * Initialize and setup work area for Gaussian pruning by safe algorithm.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gprune_safe_init()
{
  int i;
  /* maximum Gaussian set size = maximum mixture size */
  OP_calced_maxnum = OP_hmminfo->maxmixturenum;
  OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * OP_gprune_num);
  OP_calced_id = (int *)mymalloc(sizeof(int) * OP_gprune_num);
  mixcalced = (boolean *)mymalloc(sizeof(int) * OP_calced_maxnum);
  for(i=0;i<OP_calced_maxnum;i++) mixcalced[i] = FALSE;
  return TRUE;
}

/**
 * Free gprune_safe related work area.
 * 
 */
void
gprune_safe_free()
{
  free(OP_calced_score);
  free(OP_calced_id);
  free(mixcalced);
}

/** 
 * @brief  Compute a set of Gaussians with safe pruning.
 *
 * If the N-best mixtures in the previous frame is specified in @a last_id,
 * They are first computed to set the initial threshold.
 * After that, the rest of the Gaussians will be computed with the thresholds
 * to drop unpromising Gaussians from computation at early stage
 * of likelihood computation.  If the computation of a Gaussian reached to
 * the end, the threshold will be updated to always hold the likelihood of
 * current N-best score.
 *
 * The calculated scores will be stored to OP_calced_score, with its
 * corresponding mixture id to OP_calced_id.  These are done by calling
 * cache_push().
 * The number of calculated mixtures is also stored in OP_calced_num.
 * 
 * This can be called from calc_tied_mix() or calc_mix().
 * 
 * @param g [in] set of Gaussian densities to compute the output probability
 * @param gnum [in] length of above
 * @param last_id [in] ID list of N-best mixture in previous input frame,
 * or NULL if not exist
 */
void
gprune_safe(HTK_HMM_Dens **g, int gnum, int *last_id)
{
  int i, j, num = 0;
  LOGPROB score, thres;

  if (last_id != NULL) {	/* compute them first to form threshold */
    /* 1. calculate first $OP_gprune_num and set initial threshold */
    for (j=0; j<OP_gprune_num; j++) {
      i = last_id[j];
      score = compute_g_base(g[i]);
      num = cache_push(i, score, num);
      mixcalced[i] = TRUE;      /* mark them as calculated */
    }
    thres = OP_calced_score[num-1];
    /* 2. calculate the rest with pruning*/
    for (i = 0; i < gnum; i++) {
      /* skip calced ones in 1. */
      if (mixcalced[i]) {
        mixcalced[i] = FALSE;
        continue;
      }
      /* compute with safe pruning */
      score = compute_g_safe(g[i], thres);
      if (score <= thres) continue;
      num = cache_push(i, score, num);
      thres = OP_calced_score[num-1];
    }
  } else {			/* in case the last_id not available */
    /* not tied-mixture, or at the first 0 frame */
    thres = LOG_ZERO;
    for (i = 0; i < gnum; i++) {
      if (num < OP_gprune_num) {
	score = compute_g_base(g[i]);
      } else {
	score = compute_g_safe(g[i], thres);
	if (score <= thres) continue;
      }
      num = cache_push(i, score, num);
      thres = OP_calced_score[num-1];
    }
  }
  OP_calced_num = num;
}
