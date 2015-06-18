/**
 * @file   globalvars.h
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:42:26 2005
 * 
 * <JA>
 * @brief  音響尤度計算用の大域変数
 *
 * 関数呼び出しの余分なオーバヘッドを避けるため，計算中のパラメータや
 * 状態に関する情報は大域変数として定義されています．

 * 
 * <EN>
 * @brief  Global variables for acoustic likelihood computation
 *
 * The information regarding the input parameter and the current %HMM state
 * are defined as global variables here, to minimize the overhead of
 * function calls.
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

#ifdef GLOBAL_VARIABLE_DEFINE
#define GLOBAL ///< Global variables should be defined in the included point
#define GLOBAL_VAL(v) = (v) ///< Default variables are set.
#else
#define GLOBAL extern ///< Global variables should be just a reference
#define GLOBAL_VAL(v) ///< Just a reference.
#endif

/* functions selected in outprob_init.c */
/// Function to compute output probability with/without code book level cache
GLOBAL LOGPROB (*calc_outprob)();
/// Function to compute state output with/without GMS support
GLOBAL LOGPROB (*calc_outprob_state)();
/// Pruning function to compute likelihood of a mixture component
GLOBAL void (*compute_gaussset)(HTK_HMM_Dens **g, int num, int *last_id);
/// Initialization function that corresponds to compute_gaussset.
GLOBAL boolean (*compute_gaussset_init)();
GLOBAL void (*compute_gaussset_free)();

GLOBAL HTK_HMM_INFO *OP_hmminfo; ///< Current %HMM definition data
GLOBAL HTK_HMM_INFO *OP_gshmm; ///< Current GMS %HMM data
GLOBAL int OP_gprune_num; ///< Current number of computed mixtures for pruning

GLOBAL HTK_Param *OP_param;	///< Current parameter
GLOBAL HTK_HMM_State *OP_state;	///< Current state
GLOBAL int OP_state_id;		///< Current state ID
GLOBAL int OP_time;		///< Current time
GLOBAL int OP_last_time;	///< last time
GLOBAL VECT *OP_vec;		///< Current input vector
GLOBAL short OP_veclen;		///< Current vector length

/* work buffer for compute_gsset (used in gprune_*.c and calc_*mix.c) */
GLOBAL int OP_calced_maxnum; ///< Allocated length of below
GLOBAL LOGPROB *OP_calced_score; ///< Scores of computed mixtures
GLOBAL int *OP_calced_id; ///< IDs of computed mixtures
GLOBAL int OP_calced_num; ///< Number of computed mixtures
