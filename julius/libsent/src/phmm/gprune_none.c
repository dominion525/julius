/**
 * @file   gprune_none.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 05:09:46 2005
 * 
 * <JA>
 * @brief  混合ガウス分布計算: Gaussian pruning 無し
 *
 * gprune_none()は混合ガウス分布集合の計算ルーチンの一つです．
 * pruningなどを行なわずに全てのGauss分布について出力確率を求めます．
 * tied-mixtureでないモデルではこの関数がデフォルトで用いられます．
 * また "-gprune none" を指定することでも選択することができます．
 *
 * outprob_init() によって gprune_none() が関数ポインタ @a compute_gaussset に
 * セットされ，それが calc_tied_mix() またはcalc_mix() から呼び出されます．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate probability of a set of Gaussian densities (no pruning)
 *
 * gprune_none() is one of the functions to compute output probability of
 * a set of Gaussian densities.  This function does no pruning, just computing
 * each Gaussian density one by one.  For non tied-mixture model, this function
 * is used as default.  Specifying "-gprune none" at runtime can also select
 * this function.
 *
 * gprune_none() will be used by calling outprob_init() to set its pointer
 * to the global variable @a compute_gaussset.  Then it will be called from
 * calc_tied_mix() or calc_mix().
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

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>
#include "globalvars.h"

/** 
 * Calculate probability of a Gaussian density against input
 * vector on OP_vec.
 * 
 * @param binfo [in] a Gaussian density
 * 
 * @return the output log probability.
 */
LOGPROB
compute_g_base(HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec = OP_vec;
  short veclen = OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  tmp = binfo->gconst;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
  }
  return(tmp * -0.5);
}

/** 
 * Initialize and setup work area for Gaussian computation
 * 
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gprune_none_init()
{
  /* maximum Gaussian set size = maximum mixture size */
  OP_calced_maxnum = OP_hmminfo->maxmixturenum;
  OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * OP_calced_maxnum);
  OP_calced_id = (int *)mymalloc(sizeof(int) * OP_calced_maxnum);
  /* force gprune_num to the max number */
  OP_gprune_num = OP_calced_maxnum;
  return TRUE;
}

/**
 * Free gprune_none related work area.
 * 
 */
void
gprune_none_free()
{
  free(OP_calced_score);
  free(OP_calced_id);
}

/** 
 * @brief  Compute a set of Gaussians with no pruning
 *
 * The calculated scores will be stored to OP_calced_score, with its
 * corresponding mixture id to OP_calced_id. 
 * The number of calculated mixtures is also stored in OP_calced_num.
 * 
 * This can be called from calc_tied_mix() or calc_mix().
 * 
 * @param g [in] set of Gaussian densities to compute the output probability.
 * @param num [in] length of above
 * @param last_id [in] ID list of N-best mixture in previous input frame,
 * or NULL if not exist
 */
void
gprune_none(HTK_HMM_Dens **g, int num, int *last_id)
{
  int i;
  HTK_HMM_Dens *dens;
  LOGPROB *prob = OP_calced_score;
  int *id = OP_calced_id;
  OP_calced_num = 0;
  for(i=0; i<num; i++) {
    dens = *(g++);
    *(prob++) = compute_g_base(dens);
    *(id++) = i;
    OP_calced_num++;
  }
}
