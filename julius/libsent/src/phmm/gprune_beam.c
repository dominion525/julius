/**
 * @file   gprune_beam.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 03:27:53 2005
 * 
 * <JA>
 * @brief  ���祬����ʬ�۷׻�: Gaussian pruning (beam algorithm)
 *
 * gprune_beam()�Ϻ��祬����ʬ�۽���η׻��롼����ΰ�ĤǤ���
 * beam pruning ��Ȥäƾ�̤Υ�����ʬ�ۤν��ϳ�Ψ�Τߤ��®�˵��ޤ���
 * Tied-mixture %HMM ���ѻ��� Julius ��GPRUNE_DEFAULT_BEAM ���������Ƥ��뤫��
 * ���뤤��Julius�Υ��ץ���� "-gprune beam" ����ꤹ�뤳�ȤǤ��δؿ���
 * ���Ѥ���ޤ���
 *
 * beam pruning �ϺǤ��Ѷ�Ū�˻޴����Ԥʤ��ޤ����׻��ϺǤ��®�Ǥ�����
 * ��̤Υ�����ʬ�ۤ�����������줺���ϳ�Ψ�θ�꤬�礭���ʤ��ǽ��������ޤ���
 * 
 * gprune_beam() �� outprob_init() �ˤ�äƤ��δؿ��ؤΥݥ��󥿤�
 * compute_gaussset �˥��åȤ���뤳�Ȥǻ��Ѥ���ޤ������Υݥ��󥿤�
 * calc_tied_mix() �ޤ��� calc_mix() ����ƤӽФ���ޤ���
 * </JA>
 * 
 * <EN>
 * @brief  Calculate probability of a set of Gaussian densities by
 * Gaussian pruning: beam algorithm
 *
 * gprune_beam() is one of the functions to compute output probability of
 * a set of Gaussian densities.  This function does beam pruning, trying
 * to compute only the best ones for faster computation.  If a tied-mixture
 * %HMM model with GPRUNE_DEFAULT_BEAM defined in Julius, or explicitly
 * specified by "-gprune beam" option, this function will be used.
 *
 * The beam pruning is the most aggressive pruning method.  This is the fastest
 * method, but they may miss the N-best Gaussian to be found, which may
 * result in some likelihood error.
 *
 * gprune_beam() will be used by calling outprob_init() to set its pointer
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

/*

  best_mixtures_on_last_frame[]
  
  dim:  0 1 2 3 4 .... veclen-1    -> sum up
 ================================
  thres
 --------------------------------
  mix1  | |
  mix2  | |
  mix3  v v
  ...
  mixN  
 ================================
         \_\_ vecprob[0],vecprob[1]

  algorithm 1:
	 
  foreach dim {
     foreach all_mixtures in best_mixtures_on_last_frame {
        compute score
     }
     threshold = the current lowest score + beam_width?
     foreach rest_mixtures {
        if (already marked as pruned at previous dim) {
	   skip
	}
	compute score
        if (score < threshold) {
	   mark as pruned
	   skip
	}
	if (score > threshold) {
	   update threshold
	}
     }
  }

  algorithm 2:

  foreach all_mixtures in best_mixtures_on_last_frame {
     foreach dim {
       compute score
       if (threshold[dim] < score) update
     }
     threshold[dim] += beam_width
  }
  foreach rest_mixtures {
     foreach dim {
        compute score
	if (score < threshold[dim]) skip this mixture
	update thres
     }
  }
     
*/

/* pruning threshold */
static LOGPROB *dimthres;	///< Threshold for each dimension (inversed)
static int dimthres_num;	///< Length of above

static boolean *mixcalced;	///< Mark which Gaussian has been computed

/** 
 * Clear per-dimension thresholds.
 * 
 */
static void
clear_dimthres()
{
  int i;
  for(i=0;i<dimthres_num;i++) dimthres[i] = 0.0;
}

/** 
 * Set beam thresholds by adding TMBEAMWIDTH to the maximum values
 * of each dimension.
 * 
 */
static void
set_dimthres()
{
  int i;
  for(i=0;i<dimthres_num;i++) dimthres[i] += TMBEAMWIDTH;
}

/**
 * @brief  Calculate probability with threshold update.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * while storing the maximum values of each dimension to @a dimthres.
 * for future pruning.  The pruning itself is not performed here.
 * This function will be used to compute the first N Gaussians.
 * 
 * @param binfo [in] Gaussian density
 * 
 * @return the output log probability.
 */
static LOGPROB
compute_g_beam_updating(HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *th = dimthres;
  VECT *vec = OP_vec;
  short veclen = OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;

  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if ( *th < tmp) *th = tmp;
    th++;
  }
  return((tmp + binfo->gconst) * -0.5);
}

/** 
 * @brief  Calculate probability with pruning.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * performing pruning using the dimension thresholds
 * that has been set by compute_g_beam_updating() and
 * set_dimthres().
 * 
 * @param binfo [in] Gaussian density
 * 
 * @return the output log probability.
 */
static LOGPROB
compute_g_beam_pruning(HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *th = dimthres;
  VECT *vec = OP_vec;
  short veclen = OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;

  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if ( tmp > *(th++)) {
      return LOG_ZERO;
    }
  }
  return((tmp + binfo->gconst) * -0.5);
}


/** 
 * Initialize and setup work area for Gaussian pruning by beam algorithm.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gprune_beam_init()
{
  int i;
  /* maximum Gaussian set size = maximum mixture size */
  OP_calced_maxnum = OP_hmminfo->maxmixturenum;
  OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * OP_gprune_num);
  OP_calced_id = (int *)mymalloc(sizeof(int) * OP_gprune_num);
  mixcalced = (boolean *)mymalloc(sizeof(int) * OP_calced_maxnum);
  for(i=0;i<OP_calced_maxnum;i++) mixcalced[i] = FALSE;
  dimthres_num = OP_hmminfo->opt.vec_size;
  dimthres = (LOGPROB *)mymalloc(sizeof(LOGPROB) * dimthres_num);

  return TRUE;
}

/**
 * Free gprune_beam related work area.
 * 
 */
void
gprune_beam_free()
{
  free(OP_calced_score);
  free(OP_calced_id);
  free(mixcalced);
  free(dimthres);
}

/** 
 * @brief  Compute a set of Gaussians with beam pruning.
 *
 * If the N-best mixtures in the previous frame is specified in @a last_id,
 * They are first computed to set the thresholds for each dimension.
 * After that, the rest of the Gaussians will be computed with those dimension
 * thresholds to drop unpromising Gaussians from computation at early stage
 * of likelihood computation.  If the @a last_id is not specified (typically
 * at the first frame of the input), a safe pruning as same as one in
 * gprune_safe.c will be applied.
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
gprune_beam(HTK_HMM_Dens **g, int gnum, int *last_id)
{
  int i, j, num = 0;
  LOGPROB score, thres;

  if (last_id != NULL) {	/* compute them first to form thresholds */
    /* 1. clear dimthres */
    clear_dimthres();
    /* 2. calculate first $OP_gprune_num and set initial thresholds */
    for (j=0; j<OP_gprune_num; j++) {
      i = last_id[j];
      score = compute_g_beam_updating(g[i]);
      num = cache_push(i, score, num);
      mixcalced[i] = TRUE;      /* mark them as calculated */
    }
    /* 3. set pruning thresholds for each dimension */
    set_dimthres();

    /* 4. calculate the rest with pruning*/
    for (i = 0; i < gnum; i++) {
      /* skip calced ones in 1. */
      if (mixcalced[i]) {
        mixcalced[i] = FALSE;
        continue;
      }
      /* compute with safe pruning */
      score = compute_g_beam_pruning(g[i]);
      if (score > LOG_ZERO) {
	num = cache_push(i, score, num);
      }
    }
  } else {			/* in case the last_id not available */
    /* at the first 0 frame */
    /* calculate with safe pruning */
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
