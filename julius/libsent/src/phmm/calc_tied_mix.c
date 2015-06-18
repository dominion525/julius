/**
 * @file   calc_tied_mix.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:22:44 2005
 * 
 * <JA>
 * @brief  混合ガウス分布の重みつき和の計算：tied-mixture用，キャッシュ有り
 *
 * Tied-mixture 用のガウス混合分布計算ではキャッシュが考慮されます．
 * 計算された混合分布の音響尤度はコードブック単位でフレームごとに
 * キャッシュされ，同じコードブックが同じ時間でアクセスされた場合は
 * そのキャッシュから値を返します．
 * </JA>
 * 
 * <EN>
 * @brief  Compute weighed sum of Gaussian mixture for tied-mixture model (cache enabled)
 *
 * In tied-mixture computation, the computed output probability of each
 * Gaussian component will be cache per codebook, for each input frame.
 * If the same codebook of the same time is accessed later, the cached
 * value will be returned.
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
#include "globalvars.h"


/* book cache */
static MIXCACHE ***mixture_cache = NULL;///< Codebook cache: [time][book_id][0..computed_mixture_num]
static MIXCACHE **tcache; ///< Local work area that holds pointer to the cache array [bookid][0..computed_mixture_num] of the current time
static MIXCACHE *ttcache; ///< Local work area that holds pointer to the cache list [0..computed_mixture_num] of the current time and the current codebook
static int *last_id;		///< List of computed mixture id on the previous input frame
static int allocframenum;	///< Allocated frame length of codebook cache
static MIXCACHE **last_tcache; ///< Pointer to the cache array of previous frame, to pass to compute_gaussset()
static MIXCACHE *last_ttcache; ///< Pointer to the cache list of current codebook on previous frame

/** 
 * Initialize codebook cache area.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
calc_tied_mix_init()
{
  mixture_cache = NULL;
  allocframenum = -1;
  last_id = (int *)mymalloc(sizeof(int) * OP_hmminfo->maxmixturenum);
  return TRUE;
}

/** 
 * Setup codebook cache for the next incoming input.
 * 
 * @param framenum [in] length of the next input.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
calc_tied_mix_prepare(int framenum)
{
  int bid, t, size;

  /* (re)-allocate */
  if (allocframenum < framenum) {
    if (mixture_cache != NULL) {
      for(t=0;t<allocframenum;t++) {
	free(mixture_cache[t][0]);
	free(mixture_cache[t]);
      }
      free(mixture_cache);
    }
    size = OP_gprune_num * OP_hmminfo->codebooknum;
  
    mixture_cache = (MIXCACHE ***)mymalloc(sizeof(MIXCACHE **) * framenum);
    for(t=0;t<framenum;t++) {
      mixture_cache[t] = (MIXCACHE **)mymalloc(sizeof(MIXCACHE *) * OP_hmminfo->codebooknum);
      mixture_cache[t][0] = (MIXCACHE *)mymalloc(sizeof(MIXCACHE) * size);
      for(bid=1;bid<OP_hmminfo->codebooknum;bid++) {
	mixture_cache[t][bid] = &(mixture_cache[t][0][OP_gprune_num * bid]);
      }
    }
    allocframenum = framenum;
  }
  /* clear */
  for(t=0;t<framenum;t++) {
    for(bid=0;bid<OP_hmminfo->codebooknum;bid++) {
      mixture_cache[t][bid][0].score = LOG_ZERO;
    }
  }

  return TRUE;
}

/** 
 * Free work area for tied-mixture calculation.
 * 
 */
void
calc_tied_mix_free()
{
  int t;
  if (mixture_cache != NULL) {
    for(t=0;t<allocframenum;t++) {
      free(mixture_cache[t][0]);
      free(mixture_cache[t]);
    }
    free(mixture_cache);
  }
  free(last_id);
}

/** 
 * @brief  Compute the output probability of current state OP_State on
 * tied-mixture model
 * 
 * This function assumes that the OP_state is assigned to a tied-mixture
 * codebook.  Here the output probability of Gaussian mixture component
 * referred by OP_state is consulted to the book level cache, and if not
 * computed yet on that input frame time, it will be computed here.
 *
 * @return the computed output probability in log10.
 */
LOGPROB
calc_tied_mix()
{
  GCODEBOOK *book = (GCODEBOOK *)(OP_state->b);
  LOGPROB logprob;
  int i;

  if (OP_last_time != OP_time) { /* different frame */
    tcache = mixture_cache[OP_time];
    if (OP_time >= 1) {
      last_tcache = mixture_cache[OP_time-1];
    } else {
      last_tcache = NULL;
    }
  }
  ttcache = tcache[book->id];
  if (tcache[book->id][0].score != LOG_ZERO) { /* already calced */
    /* calculate using cache and weight */
    for (i=0;i<OP_calced_num;i++) {
      OP_calced_score[i] = ttcache[i].score + OP_state->bweight[ttcache[i].id];
    }
  } else { /* not calced yet */
    /* compute Gaussian set */
    if (OP_time >= 1) {
      last_ttcache = last_tcache[book->id];
      if (last_ttcache[0].score != LOG_ZERO) {
	for(i=0;i<OP_gprune_num;i++) last_id[i] = last_ttcache[i].id;
	/* tell last calced best */
	compute_gaussset(book->d, book->num, last_id);
      } else {
	compute_gaussset(book->d, book->num, NULL);
      }
    } else {
      compute_gaussset(book->d, book->num, NULL);
    }
    /* computed Gaussians will be set in:
       score ... OP_calced_score[0..OP_calced_num]
       id    ... OP_calced_id[0..OP_calced_num] */
    /* OP_gprune_num = required, OP_calced_num = actually calced */
    /* store to cache */
    for (i=0;i<OP_calced_num;i++) {
      ttcache[i].id = OP_calced_id[i];
      ttcache[i].score = OP_calced_score[i];
      /* now OP_calced_{id|score} can be used for work area */
      OP_calced_score[i] += OP_state->bweight[OP_calced_id[i]];
    }
  }
  logprob = addlog_array(OP_calced_score, OP_calced_num);
  if (logprob <= LOG_ZERO) return LOG_ZERO;
  return (logprob * INV_LOG_TEN);
}  
