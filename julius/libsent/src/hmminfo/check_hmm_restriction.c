/**
 * @file   check_hmm_restriction.c
 * @author Akinobu LEE
 * @date   Tue Feb 15 19:00:58 2005
 * 
 * <JA>
 * @brief  与えられた %HMM の遷移が使用可能な形式かどうかチェックする
 * </JA>
 * 
 * <EN>
 * @brief  Check if the given %HMM definition file can be used
 * </EN>
 *
 * TRANSITION RESTRICTIONS:
 *
 *   - for HTK and Julius:
 *      - no arc to initial state
 *      - no arc from final state
 * 
 *   - Normal version of Julius:
 *      - should have at least one output state
 *      - allow only one arc from initial state
 *      - allow only one arc to final state
 *        (internal skip/loop is allowed)
 *
 *   - Multipath version of Julius:
 *      - should have at least one output state
 *
 * In multipath version, all the transitions including model-skipping
 * transition is allowed.  However, in normal version, their transition
 * is restricted as above.
 * 
 * If such transition is found, Julius output warning and
 * proceed by modifying transition to suite for the restriction.
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

#define GOOD 0			///< Mark transition as allowed
#define FIXED 1			///< Mark fixed transition 
#define BAD 3			///< Mark not supported transition

/* transition allowance matrix on normal version */
/* G=good,F=fixed,B=bad */
/*  |G|F|B */
/* -+-+-+- */
/* G|G|F|B */
/* F|F|F|B */
/* B|B|B|B */

/* transition probability is not loganized yet */

/** 
 * Scan the transition matrix to test the ristrictions.
 * 
 * @param t [in] a transition matrix to be tested
 * 
 * @return 0 if it conforms, 1 if unacceptable transition was found and
 * modification forced, 3 if totally unsupported transition as included and
 * cannot by handled.
 */
static int
trans_ok_p(HTK_HMM_Trans *t)
{
#ifdef MULTIPATH_VERSION
  int i;
  int tflag;
  int retflag = GOOD;

  /* no arc to initial state */
  tflag = GOOD;
  for (i=0;i<t->statenum;i++) {
    if (t->a[i][0] != (PROB)0.0) {
      tflag = BAD;
      break;
    }
  }
  if (tflag == BAD) {
    j_printerr("Error: transition to initial state not allowed\n");
    retflag = BAD;
  }
  /* no arc from final state */
  tflag = GOOD;
  for (i=0;i<t->statenum;i++) {
    if (t->a[t->statenum-1][i] != (PROB)0.0) {
      tflag = BAD;
      break;
    }
  }
  if (tflag == BAD) {
    j_printerr("Error: transition from final state not allowed\n");
    retflag = BAD;
  }
    
  return(retflag);
  
#else  /* ~MULTIPATH_VERSION */
  
  int i, j;
  int tflag = BAD;
  int retflag = BAD;
  PROB maxprob;
  int maxid = -1;
  
  /* allow only one arc from initial state */
  tflag = BAD;
  for (i=0;i<t->statenum;i++) {
    if (t->a[0][i] != (PROB)0.0) {
      if (tflag == BAD) {
	tflag = GOOD;
      } else {			/* 2nd time */
	j_printerr("Warning: initial state has more than one arc\n");
	tflag = BAD;
	break;
      }
    }
  }
  if (tflag == BAD) {		/* unacceptable transition found */
    if (i >= t->statenum) {	/* no arc */
      j_printerr("Error: initial state has no arc\n");
    } else {
      /* modify the transition: gather them to an arc with best probability */
      maxprob = 0.0; maxid = -1;
      for (j=0;j<t->statenum;j++) {
	if (maxprob < t->a[0][j]) {
	  maxprob = t->a[0][j];
	  maxid = j;
	}
      }
      if (maxid == -1) {
	j_error("Error: trans_ok_p: no transition in a state?\n");
      }
      t->a[0][maxid] = 1.0;
      for (j=0;j<t->statenum;j++) {
	if (j == maxid) continue;
	t->a[0][j] = 0.0;
      }
      tflag = FIXED;
    }
  }

  retflag = tflag;
  
  /* allow only one arc to final state */
  tflag = BAD;
  for (i=0;i<t->statenum;i++) {
    if (t->a[i][t->statenum-1] != (PROB)0.0) {
      if (tflag == BAD) {
	tflag = GOOD;
      } else {			/* 2nd time */
	j_printerr("Warning: more than one arc to end state\n");
	tflag = BAD;
	break;
      }
    }
  }
  if (tflag == BAD) {
    if (i >= t->statenum) {	/* no arc */
      j_printerr("Error: no arc to end state\n");
    } else {
      /* modify the transition: gather them to an arc with best probability */
      maxprob = (PROB)0.0;
      for (j=0;j<t->statenum;j++) {
	if (maxprob < t->a[j][t->statenum-1]) {
	  maxprob = t->a[j][t->statenum-1];
	  maxid = j;
	}
      }
      for (i=0;i<t->statenum;i++) {
	if (t->a[i][t->statenum-1] == (PROB)0.0) continue;
	if (i == maxid) continue;
	for (j=t->statenum-2;j>=0;j--) {
	  if (t->a[i][j] != (PROB)0.0) {
	    t->a[i][j] += t->a[i][t->statenum-1];
	    t->a[i][t->statenum-1] = (PROB)0.0;
	    break;
	  }
	}
      }
      tflag = FIXED;
    }
  }
    
  return(retflag | tflag);

#endif /* MULTIPATH_VERSION */
}

/** 
 * Check if the transition matrix conforms the ristrictions of Julius.
 * 
 * @param dt [in] HTK %HMM model to check.
 * 
 * @return TRUE on success, FALSE if the check failed.
 */
boolean
check_hmm_limit(HTK_HMM_Data *dt)
{
  boolean return_flag = TRUE;
  int tflag;

  tflag = trans_ok_p(dt->tr);
  if (tflag == BAD) {
    return_flag = FALSE;
    j_printerr("Limit: HMM \"%s\" has unsupported arc.\n", dt->name);
    put_htk_trans(dt->tr);
  } else if (tflag == FIXED) {
    j_printerr("Warning: HMM \"%s\" has unsupported arc.\n", dt->name);
    j_printerr("SERIOUS WARNING: Transition arc has been modified as below\n");
    j_printerr("SERIOUS WARNING: This may cause unintended recognition result\n");
    put_htk_trans(dt->tr);
  }
#ifdef MULTIPATH_VERSION
  if (dt->tr->statenum < 3) {
    return_flag = FALSE;
    j_printerr("Limit: HMM \"%s\" has no output state (statenum=%d)\n", dt->name, dt->tr->statenum);
  }
#endif
  return(return_flag);
}

/** 
 * Check all the %HMM definitions in a HTK %HMM definition data.
 * 
 * @param hmminfo [in] HTK %HMM data to check.
 * 
 * @return TRUE if there was no bad models, FALSE if at least one model is bad.
 */
boolean
check_all_hmm_limit(HTK_HMM_INFO *hmminfo)
{
  HTK_HMM_Data *dt;
  boolean return_flag = TRUE;

  for (dt = hmminfo->start; dt; dt = dt->next) {
    if (check_hmm_limit(dt) == FALSE) {
      return_flag = FALSE;
    }
  }
  return(return_flag);
}


#ifdef MULTIPATH_VERSION
/** 
 * <JA>
 * モデルが，出力状態を経由せずに入力状態から出力状態へ直接遷移するような
 * 遷移を持つかどうかをチェックする．
 * 
 * @param d [in] 論理HMM
 * 
 * @return 入力から出力への直接遷移を持つ場合 TRUE, 持たない場合 FALSE を返す．
 * </JA>
 * <EN>
 * Check if the model has direct transition from initial state to final state,
 * skipping all the output state.
 * 
 * @param d [in] logical HMM
 * 
 * @return TRUE if it has direct transition from initial state to final state,
 * that is, this is a "skippable" model.  Otherwise, return FALSE.
 * </EN>
 */
boolean
is_skippable_model(HTK_HMM_Data *d)
{
  if (d->tr->a[0][d->tr->statenum-1] != LOG_ZERO) {
    return TRUE;
  }
  return FALSE;
}
#endif
