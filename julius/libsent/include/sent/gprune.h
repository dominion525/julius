/**
 * @file   gprune.h
 * @author Akinobu LEE
 * @date   Thu Feb 10 18:55:11 2005
 *
 * <EN>
 * @brief  Definitions for Gaussian pruning
 *
 * This file contains definitions for Gaussian pruning, an efficient method
 * of calculating acoustic likelihoods of a state in mixture-component %HMM.
 * </EN>
 * <JA>
 * @brief  Gaussian pruning に関する定義
 *
 * このファイルには, 混合ガウス出力分布を持つ%HMMにおける音響尤度計算の
 * 高速計算手法の一つである Gaussian pruning に関する定義が含まれています．
 * </JA>
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

#ifndef __SENT_GAUSSIAN_PRUNE__
#define __SENT_GAUSSIAN_PRUNE__

/**
 * @brief Symbols to specify which Gaussian pruning algorithm to use.
 *
 *   - GPRUNE_SEL_UNDEF: unspecified by user
 *   - GPRUNE_SEL_NONE: no pruning
 *   - GPRUNE_SEL_SAFE: safe pruning
 *   - GPRUNE_SEL_HEURISTIC: heuristic pruning
 *   - GPRUNE_SEL_BEAM: beam pruning
 * 
 */
enum{GPRUNE_SEL_UNDEF, GPRUNE_SEL_NONE, GPRUNE_SEL_SAFE, GPRUNE_SEL_HEURISTIC, GPRUNE_SEL_BEAM};

/// A component of per-codebook probability cache while search
typedef struct {
  LOGPROB score;		///< Cached probability of below
  unsigned short id;		///< ID of the cached Gaussian in the codebook
} MIXCACHE;

/**
 * @brief Score beam offset for GPRUNE_SEL_BEAM.
 *
 * Larger value may ease pruning error, but processing may become slower.
 * Smaller value can speed up the acoustic computation, but may cause error.
 * 
 */
#define TMBEAMWIDTH 5.0

/* gprune_common.c */
int cache_push(int id, LOGPROB score, int len);
/* gprune_none.c */
LOGPROB compute_g_base(HTK_HMM_Dens *binfo);
boolean gprune_none_init();
void gprune_none_free();
void gprune_none(HTK_HMM_Dens **g, int num, int *last_id);
/* gprune_safe.c */
LOGPROB compute_g_safe(HTK_HMM_Dens *binfo, LOGPROB thres);
boolean gprune_safe_init();
void gprune_safe_free();
void gprune_safe(HTK_HMM_Dens **g, int gnum, int *last_id);
/* gprune_heu.c */
boolean gprune_heu_init();
void gprune_heu_free();
void gprune_heu(HTK_HMM_Dens **g, int gnum, int *last_id);
/* gprune_beam.c */
boolean gprune_beam_init();
void gprune_beam_free();
void gprune_beam(HTK_HMM_Dens **g, int gnum, int *last_id);


#endif /*__SENT_GAUSSIAN_PRUNE__*/
