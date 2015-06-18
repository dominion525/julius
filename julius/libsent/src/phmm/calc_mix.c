/**
 * @file   calc_mix.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:18:52 2005
 * 
 * <JA>
 * @brief  混合ガウス分布の重みつき和の計算：非 tied-mixture 用，キャッシュ無し
 * </JA>
 * 
 * <EN>
 * @brief Compute weighed sum of Gaussian mixture for non tied-mixture model (no cache)
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

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>
#include "globalvars.h"

/** 
 * @brief  Compute the output probability of current state OP_State.
 *
 * No codebook-level cache is done.  
 * 
 * @return the output probability of the state OP_State in log10
 */
LOGPROB
calc_mix()
{
  int i;
  LOGPROB logprob = LOG_ZERO;

  /* compute Gaussian set */
  compute_gaussset(OP_state->b, OP_state->mix_num, NULL);
  /* computed Gaussians will be set in:
     score ... OP_calced_score[0..OP_calced_num]
     id    ... OP_calced_id[0..OP_calced_num] */
  
  /* sum */
  for(i=0;i<OP_calced_num;i++) {
    OP_calced_score[i] += OP_state->bweight[OP_calced_id[i]];
  }
  logprob = addlog_array(OP_calced_score, OP_calced_num);
  if (logprob <= LOG_ZERO) return LOG_ZERO;
  return (logprob * INV_LOG_TEN);
}
