/**
 * @file   outprob.c
 * @author Akinobu LEE
 * @date   Fri Feb 18 18:45:21 2005
 * 
 * <JA>
 * @brief  音響尤度計算の実行および状態レベルキャッシュ
 *
 * %HMM の状態の出力確率（対数尤度）を計算します．状態の型（単語末端の
 * pseudo %HMM set かどうか）にしたがっていくつか定義されていますが，
 * 全て下位の outprob_state() を呼びます．outprob_state() は
 * 必要な情報を OP_ で始まる大域変数に格納し，calc_outprob_state() を呼び
 * 出します．calc_outprob_state() は関数のポインタであり，実体は tied-mixture
 * モデルの場合 calc_tied_mix(), それ以外の場合は calc_mix() となります．
 * （GMS を使用する場合は gms_state()）になります．
 *
 * 状態レベルの音響尤度キャッシュが行なわれます．キャッシュは 状態 x
 * 入力フレームで格納され，必要な長さにしたがって伸長されます．このキャッシュは
 * 第2パスの計算でも用いるため，全時間に渡って記録されています．
 *
 * なお tied-mixture の場合はコードブックレベルでのキャッシュも同時に
 * 行なわれます．これについては calc_tied_mix.c をご覧下さい．
 * </JA>
 * 
 * <EN>
 * @brief  Computation of acoustic likelihood in %HMM state, with state-level cache
 *
 * This file defines functions to compute output log probability of 
 * %HMM state.  Several functions are defined for each state type (whether
 * it is on word edge and a part of pseudo HMM), and all of them calls
 * outprob_state() to get the log probability of a %HMM state.  The
 * outprob_state() will set the needed values to the global variables
 * that begins with "OP_", and call calc_outprob_state().  The
 * calc_outprob_state() is actually a function pointer, and the entity is
 * either calc_tied_mix() for tied-mixture model and calc_mix() for others.
 * (If you use GMS, the entity will be gms_state() instead.)
 *
 * The state scores will be cached here.
 * The 2-dimension cache array of state and
 * input frame are used to store the computed scores.  They will be expanded
 * when needed.  Thus the scores will be cached for all input frame because
 * they will also be used in the 2nd pass of recognition process.
 *
 * When using a tied-mixture model, codebook-level cache will be also done
 * in addition to this state-level cache.  See calc_tied_mix.c for details.
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
#include <sent/speech.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>
#include "globalvars.h"


/* cache */
static int statenum;		///< Local work area that holds total number of HMM states in the %HMM definition data
static LOGPROB **outprob_cache = NULL; ///< State-level cache [t][stateid]
static int allocframenum;	///< Allocated frames of the cache
static int allocblock;		///< Allocation block size per allocateion of the cache
static BMALLOC_BASE *croot;	///< Root alloc pointer to state outprob cache
static LOGPROB *last_cache;	///< Local work are to hold cache list of current time
#define LOG_UNDEF (LOG_ZERO - 1) ///< Value to be used as the initial cache value

/** 
 * Initialize the cache data, should be called once on startup.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_cache_init()
{
  statenum = OP_hmminfo->totalstatenum;
  outprob_cache = NULL;
  allocframenum = 0;
  allocblock = OUTPROB_CACHE_PERIOD;
  OP_time = -1;
  croot = NULL;
  return TRUE;
}

/** 
 * Prepare cache for the next input, by clearing the existing cache.
 * 
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_cache_prepare()
{
  int s,t;

  /* clear already allocated area */
  for (t = 0; t < allocframenum; t++) {
    for (s = 0; s < statenum; s++) {
      outprob_cache[t][s] = LOG_UNDEF;
    }
  }
  
  return TRUE;
}

/** 
 * Expand the cache to time axis if needed.
 * 
 * @param reqframe [in] required frame length
 */
static void
outprob_cache_extend(int reqframe)
{
  int newnum;
  int size;
  int t, s;
  LOGPROB *tmpp;

  /* if enough length are already allocated, return immediately */
  if (reqframe < allocframenum) return;

  /* allocate per certain period */
  newnum = reqframe + 1;
  if (newnum < allocframenum + allocblock) newnum = allocframenum + allocblock;
  size = (newnum - allocframenum) * statenum;
  
  /* allocate */
  if (outprob_cache == NULL) {
    outprob_cache = (LOGPROB **)mymalloc(sizeof(LOGPROB *) * newnum);
  } else {
    outprob_cache = (LOGPROB **)myrealloc(outprob_cache, sizeof(LOGPROB *) * newnum);
  }
  tmpp = (LOGPROB *)mybmalloc2(sizeof(LOGPROB) * size, &croot);
  /* clear the new part */
  for(t = allocframenum; t < newnum; t++) {
    outprob_cache[t] = &(tmpp[(t - allocframenum) * statenum]);
    for (s = 0; s < statenum; s++) {
      outprob_cache[t][s] = LOG_UNDEF;
    }
  }

  /*j_printf("outprob cache: %d->%d\n", allocframenum, newnum);*/
  allocframenum = newnum;
}

/**
 * Free work area for cache.
 * 
 */
void
outprob_cache_free()
{
  if (croot != NULL) mybfree2(&croot);
  if (outprob_cache != NULL) free(outprob_cache);
}


/** 
 * @brief  Compute output probability of a state.
 *
 * Set the needed values to the global variables
 * that begins with "OP_", and call calc_outprob_state().  The
 * calc_outprob_state() is actually a function pointer, and the entity is
 * either calc_tied_mix() for tied-mixture model and calc_mix() for others.
 * (If you use GMS, the entity will be gms_state() instead.)
 *
 * The state-level cache is also consulted here.
 * 
 * @return output log probability.
 */
LOGPROB
outprob_state(
     int t,			/* time frame */
     HTK_HMM_State *stateinfo,	/* state info */
     HTK_Param *param)		/* parameter */
{
  LOGPROB outp;
  
  /* set global values for outprob functions to access them */
  OP_state = stateinfo;
  OP_state_id = stateinfo->id;
  OP_param = param;
  if (OP_time != t) {
    OP_last_time = OP_time;
    OP_time = t;
    OP_vec = param->parvec[t];
    OP_veclen = param->veclen;

    outprob_cache_extend(t);	/* extend cache if needed */
    last_cache = outprob_cache[t]; /* reduce 2-d array access */
  }
  
  /* consult cache */
  if ((outp = last_cache[OP_state_id]) == LOG_UNDEF) {
    outp = last_cache[OP_state_id] = calc_outprob_state();
  }
  return(outp);
}

static LOGPROB *maxprobs;	///< Work area that holds N-best state scores for pseudo state set
static int maxn;		///< Allocated length of above

/** 
 * Initialize work area for outprob_cd_nbest().
 * 
 * @param num [in] number of top states to be calculated.
 */
void
outprob_cd_nbest_init(int num)
{
  maxprobs = (LOGPROB *)mymalloc(sizeof(LOGPROB) * num);
  maxn = num;
}

/**
 * Free work area for outprob_cd_nbest().
 * 
 */
void
outprob_cd_nbest_free()
{
  free(maxprobs);
}
  
/** 
 * Return average of N-beat outprob for pseudo state set.
 * 
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return outprob log probability, average of top N states in @a lset.
 */
static LOGPROB
outprob_cd_nbest(int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB prob;
  int i, k, n;

  n = 0;
  for(i=0;i<lset->num;i++) {
    prob = outprob_state(t, lset->s[i], param);
    /*printf("\t\t%d:%f\n", i, prob);*/
    if (prob <= LOG_ZERO) continue;
    if (n == 0 || prob <= maxprobs[n-1]) {
      if (n == maxn) continue;
      maxprobs[n] = prob;
      n++;
    } else {
      for(k=0; k<n; k++) {
	if (prob > maxprobs[k]) {
	  memmove(&(maxprobs[k+1]), &(maxprobs[k]),
		  sizeof(LOGPROB) * (n - k - ( (n == maxn) ? 1 : 0)));
	  maxprobs[k] = prob;
	  break;
	}
      }
      if (n < maxn) n++;
    }
  }
  prob = 0.0;
  for(i=0;i<n;i++) {
    /*printf("\t\t\t- %d: %f\n", i, maxprobs[i]);*/
    prob += maxprobs[i];
  }
  return(prob/(float)n);
}
  
/** 
 * Return maximum outprob of the pseudo state set.
 * 
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return maximum output log probability among states in @a lset.
 */
static LOGPROB
outprob_cd_max(int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB maxprob, prob;
  int i;
  maxprob = LOG_ZERO;
  for(i=0;i<lset->num;i++) {
    prob = outprob_state(t, lset->s[i], param);
    if (maxprob < prob) maxprob = prob;
  }
  return(maxprob);
}

/** 
 * Return average outprob of the pseudo state set.
 * 
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return average output log probability of states in @a lset.
 */
static LOGPROB
outprob_cd_avg(int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB sum, p;
  int i,j;
  sum = 0.0;
  j = 0;
  for(i=0;i<lset->num;i++) {
    p = outprob_state(t, lset->s[i], param);
    if (p > LOG_ZERO) {
      sum += p;
      j++;
    }
  }
  return(sum/(float)j);
}

/** 
 * Compute the log output probability of a pseudo state set.
 * 
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return the computed log output probability.
 */
LOGPROB
outprob_cd(int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB ret;

  /* select computation method */
  switch(OP_hmminfo->cdset_method) {
  case IWCD_AVG:
    ret = outprob_cd_avg(t, lset, param);
    break;
  case IWCD_MAX:
    ret = outprob_cd_max(t, lset, param);
    break;
  case IWCD_NBEST:
    ret = outprob_cd_nbest(t, lset, param);
    break;
  default:
    j_error("unknown cdhmm method!\n");
    ret = 0;
    break;
  }
  return(ret);
}
  

/** 
 * Top function to compute the output probability of a HMM state.
 * 
 * @param t [in] input frame
 * @param hmmstate [in] HMM state
 * @param param [in] input parameter data
 * 
 * @return the computed log output probability.
 */
LOGPROB
outprob(int t, HMM_STATE *hmmstate, HTK_Param *param)
{
  if (hmmstate->is_pseudo_state) {
    return(outprob_cd(t, hmmstate->out.cdset, param));
  } else {
    return(outprob_state(t, hmmstate->out.state, param));
  }
}
