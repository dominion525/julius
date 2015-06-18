/**
 * @file   outprob_init.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 13:35:37 2005
 * 
 * <JA>
 * @brief  音響尤度計算ルーチンの初期化とセットアップ
 *
 * 音響モデルのタイプにあわせた計算ルーチンの選択や，計算用の各種
 * パラメータの初期化を行います．これらの初期化関数は音響尤度計算を始める前に
 * 呼び出される必要があります．
 * 
 * 音響尤度計算関数の使用方法:
 *    -# 最初に outprob_init() を呼んで初期化とセットアップを行います．
 *    -# 各入力に対して，以下を行います．
 *      -# outprob_prepare() で必要な尤度キャッシュを確保します．
 *      -# outprob(t, hmmstate, param) で各状態に対する音響尤度を計算して
 *         返します．
 * </JA>
 * 
 * <EN>
 * @brief  Initialize and setup the acoustic computation routines
 *
 * These functions switch computation function suitable for the given %HMM
 * types (tied-mixture or shared-state, use GMS or not, and so on).  It also
 * sets various parameters and global pointers for the likelihood computation.
 * These functions should be called at first.
 *
 * How to usage these acoustic computation routines:
 *    -# First call outprob_init() on startup to configure and setup functions
 *    -# For each input, 
 *      -# call outprob_prepare() to prepare cache
 *      -# use outprob(t, hmmstate, param) to get output probability of a state
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
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>

#define GLOBAL_VARIABLE_DEFINE	///< Global variables is actually defined here
#include "globalvars.h"


/** 
 * Initialize and setup acoustic computation functions.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_init(HTK_HMM_INFO *hmminfo,
	     HTK_HMM_INFO *gshmm, int gms_num,
	     int gprune_method, int gprune_mixnum
	     )
{
  /* check if variances are inversed */
  if (!hmminfo->variance_inversed) {
    /* here, inverse all variance values for faster computation */
    htk_hmm_inverse_variances(hmminfo);
    hmminfo->variance_inversed = TRUE;
  }
  /* check if variances are inversed */
  if (gshmm) {
    if (!gshmm->variance_inversed) {
      /* here, inverse all variance values for faster computation */
      htk_hmm_inverse_variances(gshmm);
      gshmm->variance_inversed = TRUE;
    }
  }

  /** select functions **/
  /* select pruning function to compute likelihood of a mixture component
     and set the pointer to global */
  switch(gprune_method) {
  case GPRUNE_SEL_NONE:
    compute_gaussset = gprune_none;
    compute_gaussset_init = gprune_none_init;
    compute_gaussset_free = gprune_none_free;
    break;
  case GPRUNE_SEL_SAFE:
    compute_gaussset = gprune_safe;
    compute_gaussset_init = gprune_safe_init;
    compute_gaussset_free = gprune_safe_free;
    break;
  case GPRUNE_SEL_HEURISTIC:
    compute_gaussset = gprune_heu;
    compute_gaussset_init = gprune_heu_init;
    compute_gaussset_free = gprune_heu_free;
    break;
  case GPRUNE_SEL_BEAM:
    compute_gaussset = gprune_beam;
    compute_gaussset_init = gprune_beam_init;
    compute_gaussset_free = gprune_beam_free;
    break;
  }
  /* select caching function to compute output probability of a mixture */
  if (hmminfo->is_tied_mixture) {
    calc_outprob = calc_tied_mix; /* enable book-level cache, typically for a tied-mixture model */
  } else {
    calc_outprob = calc_mix; /* no mixture-level cache, for a shared-state, non tied-mixture model */
  }
  
  /* select back-off functon for state probability calculation */
  if (gshmm != NULL) {
    calc_outprob_state = gms_state; /* enable GMS */
  } else {
    calc_outprob_state = calc_outprob; /* call mixture outprob directly */
  }

  /* store common variable to global */
  OP_hmminfo = hmminfo;
  OP_gshmm = gshmm;		/* NULL if GMS not used */
  OP_gprune_num = gprune_mixnum;

  /* generate addlog table */
  make_log_tbl();
  
  /* initialize work area for mixture component pruning function */
  if (compute_gaussset_init() == FALSE) return FALSE; /* OP_gprune may change */
  /* initialize work area for book level cache on tied-mixture model */
  if (hmminfo->is_tied_mixture) {
    if (calc_tied_mix_init() == FALSE) return FALSE;
  }
  /* initialize work area for GMS */
  if (OP_gshmm != NULL) {
    if (gms_init(gms_num) == FALSE) return FALSE;
  }
  /* initialize cache for all output probabilities */
  if (outprob_cache_init() == FALSE)  return FALSE;

  /* initialize word area for computation of pseudo HMM set when N-max is specified */
  if (hmminfo->cdset_method == IWCD_NBEST) {
    outprob_cd_nbest_init(hmminfo->cdmax_num);
  }

  return TRUE;
}

/** 
 * Prepare for the next input of given frame length.
 * 
 * @param framenum [in] input length in frame.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_prepare(int framenum)
{
  if (outprob_cache_prepare() == FALSE) return FALSE;
  if (OP_gshmm != NULL) {
    if (gms_prepare(framenum) == FALSE) return FALSE;
  }
  if (OP_hmminfo->is_tied_mixture) {
    if (calc_tied_mix_prepare(framenum) == FALSE) return FALSE;
  }
  /* reset last time */
  OP_last_time = OP_time = -1;
  return TRUE;
}

/**
 * Free all work area for outprob computation.
 * 
 */
void
outprob_free()
{
  compute_gaussset_free();
  if (OP_hmminfo->is_tied_mixture) {
    calc_tied_mix_free();
  }
  if (OP_gshmm != NULL) {
    gms_free();
  }
  outprob_cache_free();
  if (OP_hmminfo->cdset_method == IWCD_NBEST) {
    outprob_cd_nbest_free();
  }

}
