/**
 * @file   beam.h
 * @author Akinobu LEE
 * @date   Mon Mar  7 15:12:29 2005
 * 
 * <JA>
 * @brief  第１パスのフレーム同期ビーム探索用のトークン等の定義
 * </JA>
 * 
 * <EN>
 * @brief  Definitions for frame-synchronous beam search on 1st pass.
 * </EN>
 * 
 * $Revision: 1.3 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __J_BEAM_H__
#define __J_BEAM_H__

/// token id for the 1st pass
typedef int TOKENID;

/// id for undefined token
#define TOKENID_UNDEFINED -1

/// Token to hold viterbi pass history
typedef struct {
  TRELLIS_ATOM *last_tre;	///< Previous word candidate in word trellis
#ifdef USE_NGRAM
  WORD_ID last_cword;		///< Previous context-aware (not transparent) word
  LOGPROB last_lscore;		///< Currently assigned word-internal LM score for factoring
#endif
  LOGPROB score;		///< Current accumulated score (AM+LM)
  int node;			///< Lexicon node ID to which this token is assigned
#ifdef WPAIR
  TOKENID next;			///< ID pointer to next token at same node, for word-pair approx.
#endif
} TOKEN2;

#define FILLWIDTH 70		///< Word-wrap character length for progressive output

#endif /* __J_BEAM_H__ */
