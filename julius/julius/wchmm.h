/**
 * @file   wchmm.h
 * @author Akinobu Lee
 * @date   Sun Sep 18 21:31:32 2005
 * 
 * <JA>
 * @brief  木構造化辞書および単語トレリスの構造体定義．
 *
 * このファイルでは，第1パスで用いられる木構造化辞書（あるいは単語連結
 * HMM (wchmm) とも呼ばれる）の構造体を定義しています．起動時に，単語辞書の
 * 前単語が並列に並べられ，ツリー上に結合されて木構造化辞書が構築されます．
 * HMMの状態単位で構築され，各状態は，対応するHMM出力確率，ツリー内での遷移先
 * のリスト，および探索のための様々な情報（言語スコアファクタリングのための
 * successor word list や uni-gram 最大値，単語始終端マーカー，音素開始
 * マーカーなど）を含みます．
 * </JA>
 * 
 * <EN>
 * @brief  Structure Definition of tree lexicon and word trellis.
 *
 * This file defines structure for word-conjunction HMM, aka tree lexicon
 * for recognition of 1st pass.  Words in the dictionary are gathered to
 * build a tree lexicon.  The lexicon is built per HMM state basis,
 * with their HMM output probabilities, transition arcs, and other
 * informations for search such as successor word lists and maximum
 * uni-gram scores for LM factoring, word head/tail marker, phoneme
 * start marker, and so on.
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

#ifndef __SENT_WORD_CONJ_HMM__
#define __SENT_WORD_CONJ_HMM__

#define		MAXWCNSTEP  40000 ///< Number of states to be allocated at once

/**
 * Element of successor word list for LM factoring computation.
 * 
 */
typedef struct s_cell {
  WORD_ID word;			///< N-gram word ID
  struct s_cell *next;		///< Pointer to next element, or NULL if terminated
} S_CELL;


#ifdef PASS1_IWCD

/* Cross-word triphone handling */

/**
 * State output probability data for head phone of a word.  The phoneme HMM
 * should change while search according to the last context word.
 * 
 */
typedef struct {
  HMM_Logical  *hmm;		///< Original HMM state on the dictionary
  short		state_loc;	///< State location within the phoneme (1-)
  /* Context cache */
  boolean	last_is_lset;	///< TRUE if last assigned model was context-dependent state set
  union {
    HTK_HMM_State *state;	///< Last assigned state (last_is_lset = FALSE)
    CD_State_Set  *lset;	///< Last assigned lset (last_is_lset = TRUE)
  } cache;
  WORD_ID	lastwid_cache;	///< Last context word ID
} RC_INFO;

/**
 * State output probability data for 1-phone word.  The phoneme HMM should
 * change according to the last context word.
 * 
 */
typedef struct {
  HMM_Logical  *hmm;		///< Original HMM state on the dictionary
  short		state_loc;	///< State location within the phoneme (1-)
  /* Context cache */
  boolean	last_is_lset;	///< TRUE if last assigned model was context-dependent state set
#ifdef CATEGORY_TREE
  WORD_ID	category;	///< Last context word's category ID
#endif
  union {
    HTK_HMM_State *state;	///< Last assigned state
    CD_State_Set  *lset;	///< Last assigned lset
  } cache;
  WORD_ID	lastwid_cache;	///< Last context word ID
} LRC_INFO;

/* For word tail phoneme, pseudo phone on the dictionary will be directly
   used as context-dependent state set */

/**
 * State output probability container on lexicon tree.  Each state
 * should have one of them.
 * 
 */
typedef union {
  HTK_HMM_State *state;		///< For AS_STATE (word-internal phone)
  CD_State_Set  *lset;		///< For AS_LSET (word tail phone)
  RC_INFO	*rset;		///< For AS_RSET (word head phone)
  LRC_INFO	*lrset;		///< For AS_LRSET (phone in 1-phoneme word)
} ACOUSTIC_SPEC;

/**
 * ID to indicate which data is in the ACOUSTIC_SPEC container.
 * 
 */
typedef enum {
  AS_STATE,			///< This state is in word-internal phone
  AS_LSET,			///< This state is in word tail phone
  AS_RSET,			///< This state is in word head phone
  AS_LRSET			///< This state is in 1-phone word
} AS_Style;
#endif
  
/*************************************************************************/

/**
 * HMM state on tree lexicon.
 * 
 */
typedef struct wchmm_state {
  A_CELL	*ac;		///< Transition arcs from this node
#ifdef PASS1_IWCD
  ACOUSTIC_SPEC out;		///< State output probability container
  /* below has been moved to WCHMM (04/06/22 by ri) */
  /*unsigned char	outstyle;	output type (one of AS_Style) */
#else  /* ~PASS1_IWCD */
  HTK_HMM_State *out;		///< HMM State
#endif /* ~PASS1_IWCD */
#ifndef CATEGORY_TREE
  /**
   * LM factoring parameter:
   * If scid > 0, it will points to the successor list index.
   * If scid = 0, the node is not on branch.
   * If scid < 0, it will points to the unigram factoring value index.
   */
  int scid;
#endif
} WCHMM_STATE;

/**
 * Whole lexicon tree structure holding all information.
 * 
 */
typedef struct wchmm_info {
  HTK_HMM_INFO *hmminfo;	///< HMM definitions used to construct this lexicon
#ifdef USE_NGRAM
  NGRAM_INFO *ngram;		///< N-gram used to construct this lexicon
#endif
#ifdef USE_DFA
  DFA_INFO *dfa;		///< Grammar used to construct this lexicon
#endif
  WORD_INFO *winfo;		///< Word dictionary used to construct this lexicon
  int	maxwcn;			///< Memory assigned maximum number of nodes
  int	n;			///< Num of nodes in this lexicon
  WCHMM_STATE	*state;		///< HMM state on tree lexicon [nodeID]
#ifndef MULTIPATH_VERSION
  WORD_ID	*ststart;	///< Word ID that begins at the state [nodeID]
#endif
  WORD_ID	*stend;		///< Word ID that ends at the state [nodeID]
  int	**offset;		///< Node ID of a phone [wordID][0..phonelen-1]
  int	*wordend;		///< Node ID of word-end state [wordID]
  int	startnum;		///< Number of root nodes
  int	*startnode;		///< Root node index [0..startnum-1] -> node ID
#ifdef MULTIPATH_VERSION
  int	*wordbegin;		///< Node ID of word-beginning state [wordID]
  int	maxstartnum;		///< Allocated number of startnodes
# ifdef CATEGORY_TREE
  WORD_ID *start2wid;		///< Root node index [0..startnum-1] -> word ID
# endif
#endif /* MULTIPATH_VERSION */
#ifdef UNIGRAM_FACTORING
  int	*start2isolate;		///< Root node index -> isolated root node ID
  int	isolatenum;		///< Number of isolated root nodes
#endif
#ifndef MULTIPATH_VERSION
  LOGPROB	*wordend_a;	///< Transition prob. outside word [wordID]
#endif
#ifdef PASS1_IWCD
  unsigned char *outstyle;	///< ID to indicate type of output probability container (one of AS_Style)
#endif
#ifndef CATEGORY_TREE
  /* Successor lists on the tree are stored on sequencial list at @a sclist,
     and each node has index to the list */
  S_CELL **sclist;		///< List of successor list [scid]
  int *sclist2node;		///< Mapping successor list [scid] to node
  int   scnum;			///< Number of factoring nodes that has successor list
#ifdef UNIGRAM_FACTORING
  LOGPROB *fscore;		///< List of 1-gram factoring score [-scid]
  int fsnum;			///< Number of @a fscore
#endif
#endif
  BMALLOC_BASE *malloc_root;	///< Pointer for block memory allocation
#ifdef PASS1_IWCD
#ifdef CATEGORY_TREE
  APATNODE *lcdset_category_root; ///< Index of lexicon-dependent category-aware pseudo phone set when used on Julian
#endif /* CATEGORY_TREE */
#endif /* PASS1_IWCD */
} WCHMM_INFO;

/*************************************************************************/
/* word trellis index (result of 1st pass) */

/**
 * Word trellis element that holds survived word ends at each frame
 * on the 1st pass.
 * 
 */
typedef struct __trellis_atom__ {
  LOGPROB backscore;		///< Accumulated score from start
#ifdef USE_NGRAM
  LOGPROB lscore;		///< N-gram score of this word
#endif
  WORD_ID wid;			///< Word ID
  short begintime;		///< Beginning frame
  short endtime;		///< End frame
#ifdef WORD_GRAPH
  boolean within_wordgraph;	///< TRUE if within word graph
  boolean within_context;	///< TRUE if any of its following word was once survived in beam while search
#endif
  struct __trellis_atom__ *last_tre; ///< Pointer to previous context trellis word
  struct __trellis_atom__ *next; ///< Temporary link to store generated trellis word on 1st pass
} TRELLIS_ATOM;

/**
 * Whole word trellis (aka backtrellis) generated as a result of 1st pass.
 * 
 */
typedef struct __backtrellis__ {
  int framelen;			///< Frame length
  int *num;			///< Number of trellis words at frame [t]
  TRELLIS_ATOM ***rw;		///< List to trellis words at frame [t]: rw[t][0..num[t]]
  TRELLIS_ATOM *list;		///< Temporary storage point used in 1st pass
  BMALLOC_BASE *root;		///< memory allocation base for mybmalloc2()
} BACKTRELLIS;

#endif /* __SENT_WORD_CONJ_HMM__ */
