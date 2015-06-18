/**
 * @file   gms.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:52:18 2005
 * 
 * <JA>
 * @brief  Gaussian Mixture Selection による状態尤度計算
 *
 * 実装方法についてはソース内のコメントをご覧ください．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate state probability with Gaussian Mixture Selection
 *
 * See the comments in the source for details about implementation.
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

/*
  Implementation of Gaussian Mixture Selection (old doc...)
  
  It is called from gs_calc_selected_mixture_and_cache_{safe,heu,beam} in
  the first pass for each frame.  It calculates all GS HMM outprob for
  given input frame and get the N-best GS HMM states. Then,
       for the selected (N-best) states:
           calculate the corresponding codebook,
           and set fallback_score[t][book] to LOG_ZERO.
       else:
           set fallback_score[t][book] to the GS HMM outprob.
  Later, when calculating state outprobs, the fallback_score[t][book]
  is consulted and,
       if fallback_score[t][book] == LOG_ZERO:
           it means it has been selected, so calculate the outprob with
           the corresponding codebook and its weights.
       else:
           it means it was pruned, so use the fallback_score[t][book]
           as its outprob.

           
  For triphone, GS HMMs should be assigned to each state.
  So the fallback_score[][] is kept according to the GS state ID,
  and corresponding GS HMM state id for each triphone state id should be
  kept beforehand.
  GS HMM Calculation:
       for the selected (N-best) GS HMM states:
           set fallback_score[t][gs_stateid] to LOG_ZERO.
       else:
           set fallback_score[t][gs_stateid] to the GS HMM outprob.
  triphone HMM probabilities are assigned as:
       if fallback_score[t][state2gs[tri_stateid]] == LOG_ZERO:
           it has been selected, so calculate the original outprob.
       else:
           as it was pruned, re-use the fallback_score[t][stateid]
           as its outprob.
*/


#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>
#include "globalvars.h"

#undef NORMALIZE_GS_SCORE	/* normalize score (ad-hoc) */

  /* GS HMMs must be defined at STATE level using "~s NAME" macro,
     where NAMES are like "i:4m", "s2m", etc. */

/* variables for GMS */
static int my_nbest;		///< Number of states to be selected
static int allocframenum;	///< Allocated number of frame for storing fallback scores per frame

/* GMS info */
static GS_SET *gsset;		///< Set of GS states
static int gsset_num;		///< Num of above
static int *state2gs; ///< Mapping from triphone state id to gs id

/* results */
static boolean *is_selected;	///< TRUE if the frame is already selected
static LOGPROB **fallback_score = NULL; ///< [t][gssetid], LOG_ZERO if selected

/* for calculation */
static int *gsindex;		///< Index buffer
static LOGPROB *t_fs;		///< Current fallback_score


/** 
 * Register all state defs in GS HMM to GS_SET.
 * 
 */
static void
build_gsset()
{
  HTK_HMM_State *st;

  /* allocate */
  gsset = (GS_SET *)mymalloc(sizeof(GS_SET) * OP_gshmm->totalstatenum);
  gsset_num = OP_gshmm->totalstatenum;
  /* make ID */
  for(st = OP_gshmm->ststart; st; st=st->next) {
    gsset[st->id].state = st;
  }
}

/**
 * Free gsset.
 * 
 */
static void
free_gsset()
{
  free(gsset);
}

#define MAXHMMNAMELEN 40	///< Assumed maximum length of %HMM name

/** 
 * Build the correspondence from GS states to triphone states.
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
build_state2gs()
{
  HTK_HMM_Data *dt;
  HTK_HMM_State *st, *cr;
  int i;
  char gstr[MAXHMMNAMELEN], cbuf[MAXHMMNAMELEN];
  boolean ok_p = TRUE;

  /* initialize */
  state2gs = (int *)mymalloc(sizeof(int) * OP_hmminfo->totalstatenum);
  for(i=0;i<OP_hmminfo->totalstatenum;i++) state2gs[i] = -1;

  /* parse through all HMM macro to register their state */
  for(dt = OP_hmminfo->start; dt; dt=dt->next) {
    if (strlen(dt->name) >= MAXHMMNAMELEN - 2) {
      j_printerr("Error: too long hmm name (>%d): \"%s\"\n",
		 MAXHMMNAMELEN-3, dt->name);
      ok_p = FALSE;
      continue;
    }
    for(i=1;i<dt->state_num-1;i++) { /* for all state */
      st = dt->s[i];
      /* skip if already assigned */
      if (state2gs[st->id] != -1) continue;
      /* set corresponding gshmm name */
      sprintf(gstr, "%s%dm", center_name(dt->name, cbuf), i + 1);
      /* look up the state in OP_gshmm */
      if ((cr = state_lookup(OP_gshmm, gstr)) == NULL) {
	j_printerr("Error: GS HMM \"%s\" not defined\n", gstr);
	ok_p = FALSE;
	continue;
      }
      /* store its ID */
      state2gs[st->id] = cr->id;
    }
  }
#ifdef PARANOIA
  {
    HTK_HMM_State *st;
    for(st=OP_hmminfo->ststart; st; st=st->next) {
      printf("%s -> %s\n", (st->name == NULL) ? "(NULL)" : st->name,
	     (gsset[state2gs[st->id]].state)->name);
    }
  }
#endif
  return ok_p;
}

/**
 * free state2gs.
 * 
 */
static void
free_state2gs()
{
  free(state2gs);
}


/* sort to find N-best states */
#define SD(A) gsindex[A-1]	///< Index macro for heap sort
#define SCOPY(D,S) D = S	///< Element copy macro for heap sort
#define SVAL(A) (t_fs[gsindex[A-1]]) ///< Element evaluation macro for heap sort
#define STVAL (t_fs[s]) ///< Element current value macro for heap sort

/** 
 * Heap sort of @a gsindex to determine which model gets N best likelihoods.
 * 
 * @param neednum [in] needed number N
 * @param totalnum [in] total number of data
 */
static void
sort_gsindex_upward(int neednum, int totalnum)
{
  int n,root,child,parent;
  int s;
  for (root = totalnum/2; root >= 1; root--) {
    SCOPY(s, SD(root));
    parent = root;
    while ((child = parent * 2) <= totalnum) {
      if (child < totalnum && SVAL(child) < SVAL(child+1)) {
	child++;
      }
      if (STVAL >= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
  n = totalnum;
  while ( n > totalnum - neednum) {
    SCOPY(s, SD(n));
    SCOPY(SD(n), SD(1));
    n--;
    parent = 1;
    while ((child = parent * 2) <= n) {
      if (child < n && SVAL(child) < SVAL(child+1)) {
	child++;
      }
      if (STVAL >= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
}

/** 
 * Calculate all GS state scores and select the best ones.
 * 
 */
static void
do_gms()
{
  int i;
  
  /* compute all gshmm scores (in gs_score.c) */
  compute_gs_scores(gsset, gsset_num, t_fs);
  /* sort and select */
  sort_gsindex_upward(my_nbest, gsset_num);
  for(i=gsset_num - my_nbest;i<gsset_num;i++) {
    /* set scores of selected states to LOG_ZERO */
    t_fs[gsindex[i]] = LOG_ZERO;
  }

  /* power e -> 10 */
#ifdef NORMALIZE_GS_SCORE
  /* normalize other fallback scores (rate of max) */
  for(i=0;i<gsset_num;i++) {
    if (t_fs[i] != LOG_ZERO) {
      t_fs[i] = t_fs[i] * 0.975;
    }
  }
#endif
}  


/** 
 * Initialize the GMS related functions and data.
 * 
 * @param nbest [in] N-best state to compute the precise triphone.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gms_init(int nbest)
{
  int i;
  
  /* Check gshmm type */
  if (OP_gshmm->is_triphone) {
    j_printerr("Error: GS HMM should be a monophone model\n");
    return FALSE;
  }
  if (OP_gshmm->is_tied_mixture) {
    j_printerr("Error: GS HMM should not be a tied mixture model\n");
    return FALSE;
  }

  /* store as local info */
  my_nbest = nbest;

  /* Register all GS HMM states in GS_SET */
  build_gsset();
  /* Make correspondence of all triphone states to GS HMM states */
  j_printerr("Mapping HMM states to GS HMM...");
  if (build_state2gs() == FALSE) {
    j_printerr("Error: failed in assigning GS HMM state for each state\n");
    return FALSE;
  }
  j_printerr("done\n");

  /* prepare index buffer for heap sort */
  gsindex = (int *)mymalloc(sizeof(int) * gsset_num);
  for(i=0;i<gsset_num;i++) gsindex[i] = i;

  /* init cache status */
  fallback_score = NULL;
  is_selected = NULL;
  allocframenum = -1;

  /* initialize gms_gprune functions */
  gms_gprune_init(OP_hmminfo, gsset_num);
  
  return TRUE;
}

/** 
 * Setup GMS parameters for next input.
 * 
 * @param framenum [in] length of next input in frames
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gms_prepare(int framenum)
{
  LOGPROB *tmp;
  int t;

  /* allocate cache */
  if (allocframenum < framenum) {
    if (fallback_score != NULL) {
      free(fallback_score[0]);
      free(fallback_score);
      free(is_selected);
    }
    fallback_score = (LOGPROB **)mymalloc(sizeof(LOGPROB *) * framenum);
    tmp = (LOGPROB *)mymalloc(sizeof(LOGPROB) * gsset_num * framenum);
    for(t=0;t<framenum;t++) {
      fallback_score[t] = &(tmp[gsset_num * t]);
    }
    is_selected = (boolean *)mymalloc(sizeof(boolean) * framenum);
    allocframenum = framenum;
  }
  /* clear */
  for(t=0;t<framenum;t++) is_selected[t] = FALSE;

  /* prepare gms_gprune functions */
  gms_gprune_prepare();
  
  return TRUE;
}

/**
 * Free GMS related work areas.
 * 
 */
void
gms_free()
{
  free_gsset();
  free_state2gs();
  free(gsindex);
  if (fallback_score != NULL) {
    free(fallback_score[0]);
    free(fallback_score);
    free(is_selected);
  }
  gms_gprune_free();
}



/** 
 * Get %HMM State probability of current state with Gaussiam Mixture Selection.
 *
 * If the GMS %HMM score of the corresponding basephone is below the
 * N-best, the triphone score will not be computed, and the score of
 * the GMS %HMM will be returned instead as a fallback score.
 * Else, the precise triphone will be computed and returned.
 * 
 * @return the state output probability score in log10.
 */
LOGPROB
gms_state()
{
  LOGPROB gsprob;
  if (OP_last_time != OP_time) { /* different frame */
    /* set current buffer */
    t_fs = fallback_score[OP_time];
    /* select state if not yet */
    if (!is_selected[OP_time]) {
      do_gms();
      is_selected[OP_time] = TRUE;
    }
  }
  if ((gsprob = t_fs[state2gs[OP_state_id]]) != LOG_ZERO) {
    /* un-selected: return the fallback value */
    return(gsprob);
  }
  /* selected: calculate the real outprob of the state */
  return(calc_outprob());
}
