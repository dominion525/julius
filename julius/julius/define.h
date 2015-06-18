/**
 * @file   define.h
 * @author Akinobu LEE
 * @date   Mon Mar  7 15:17:26 2005
 * 
 * <JA>
 * @brief  内部処理選択のためのシンボル定義
 *
 * configure スクリプトは，システム/ユーザごとの設定を config.h に書き出し
 * ます．このファイルでは，その configure で設定された config.h 内の定義を
 * 元に，Julius/Julian のための内部シンボルの定義を行います．
 * これらは実験用のコードの切り替えや古いオプションとの互換性のために
 * 定義されているものがほとんどです．通常のユーザはこの定義を書き換える
 * 必要はありません．
 * </JA>
 * 
 * <EN>
 * @brief  Internal symbol definitions
 *
 * The "configure" script will output the system- and user-dependent
 * configuration in "config.h".  This file defines some symboles
 * according to the generated config.h, to switch internal functions.
 * Most of the definitions here are for disabling experimental or debug
 * code for development, or to keep compatibility with old Julius.  These
 * definitions are highly internal, and normal users should not alter
 * these definitions without knowning what to do.
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

/*****************************************************************************/
/** DO NOT MODIFY MANUALLY DEFINES BELOW UNLESS YOU KNOW WHAT YOU ARE DOING **/
/*****************************************************************************/

/* switch N-gram mode (julius) <-> grammar mode (julian) */
#ifdef USE_DFA
#undef  USE_NGRAM
#else
#define USE_NGRAM
#endif

/* delete incoherent option */
#ifdef USE_DFA
#ifdef UNIGRAM_FACTORING
#undef UNIGRAM_FACTORING
#endif
#define CATEGORY_TREE
#else  /* USE_NGRAM */
#ifdef CATEGORY_TREE
#undef CATEGORY_TREE
#endif
#endif /* USE_DFA */

/* abbreviations for verbose message output */
#define VERMES if (verbose_flag) j_printerr

/* define this to report memory usage on exit (Linux only) */
#undef REPORT_MEMORY_USAGE

#ifdef USE_NGRAM
/*** tree construction ***/
/* With 1-best approximation, Constructing a single tree from all words
   causes much error by factoring.  Listing each word flatly with no
   tree-organization will not cause this error, but the network becomes
   much larger and, especially, the inter-word LM handling becomes much more
   complex (O(n^2)).  The cost may be eased by LM caching, but it needs much
   memory. */
/* This is a trade-off of accuracy and cost */
#define SHORT_WORD_LEN 2
#ifdef LOWMEM
/* don't separate, construct a single tree from all words */
/* root nodes are about 50 in monophone, cache size will be 5MB on max */
#define NO_SEPARATE_SHORT_WORD
#else
#ifdef LOWMEM2
/* experimental: separate words frequently appears in corpus (1-gram) */
/* root nodes will be "-sepnum num" + 50, cache size will be 10MB or so */
#define NO_SEPARATE_SHORT_WORD
#define SEPARATE_BY_UNIGRAM
#define DEFAULT_SEPARATE_WNUM 150
#else
/* separate all short words (<= 2 phonemes) */
/* root nodes are about 1100 in 20k (proportional to vocabulary),
   cache size will be about 100MB on max */
#endif /* LOWMEM2 */
#endif /* LOWMEM */

/*#define HASH_CACHE_IW*/
/* "./configure --enable-lowmem" defines NO_SEPARATE_SHORT_WORD instead */

#endif /* USE_NGRAM */

#ifdef USE_NGRAM
/* default language model weight and insertion penalty for pass1 and pass2 */
/* these values come from the best parameters in IPA evaluation result */
#define DEFAULT_LM_WEIGHT_MONO_PASS1   5.0
#define DEFAULT_LM_PENALTY_MONO_PASS1 -1.0
#define DEFAULT_LM_WEIGHT_MONO_PASS2   6.0
#define DEFAULT_LM_PENALTY_MONO_PASS2  0.0
#ifdef PASS1_IWCD
#define DEFAULT_LM_WEIGHT_TRI_PASS1   8.0
#define DEFAULT_LM_PENALTY_TRI_PASS1 -2.0
#define DEFAULT_LM_WEIGHT_TRI_PASS2   8.0
#define DEFAULT_LM_PENALTY_TRI_PASS2 -2.0
#else
#define DEFAULT_LM_WEIGHT_TRI_PASS1   9.0
#define DEFAULT_LM_PENALTY_TRI_PASS1  8.0
#define DEFAULT_LM_WEIGHT_TRI_PASS2  11.0
#define DEFAULT_LM_PENALTY_TRI_PASS2 -2.0
#endif /* PASS1_IWCD */
#endif /* USE_NGRAM */

/* Switch head/tail word insertion penalty to be inserted */
#undef FIX_PENALTY

/* some definitions for short-pause segmentation */
#ifdef SP_BREAK_CURRENT_FRAME
#undef SP_BREAK_EVAL		/* output messages for evaluation */
#undef SP_BREAK_DEBUG		/* output messages for debug */
#undef SP_BREAK_RESUME_WORD_BEGIN /* resume word = maxword at beginning of sp area */
#endif

/* '01/10/18 by ri: enable fix for trellis lookup order */
#define PREFER_CENTER_ON_TRELLIS_LOOKUP

#ifdef MULTIPATH_VERSION
/* '01/11/28 by ri: malloc step for startnode */
# define STARTNODE_STEP 300
/* default value of iwsp penalty */
# define IWSP_PENALTY_DEFAULT -1.0
#endif

/* default dict entry for IW-sp word that will be added to dict with -iwspword */
#ifdef USE_NGRAM
#define IWSPENTRY_DEFAULT "<UNK> [sp] sp sp"
#endif

/* confidence scoring method */
#ifdef CONFIDENCE_MEASURE
# ifndef CM_NBEST	/* use conventional N-best CM, will be defined if "--enable-cm-nbest" specified */
#  define CM_SEARCH	/* otherwise, use on-the-fly CM scoring */
# endif
#endif

/* dynamic word graph generation */
#ifdef GRAPHOUT	       /* output result in word-graph format */

#undef GRAPHOUT_SEARCH_CONSIDER_RIGHT /* if defined, only hypothesis whose
					 left/right contexts is already
					 included in popped hypo will be merged.
					 EXPERIMENTAL, should not be defined.
				       */
#ifdef CM_SEARCH_LIMIT
#undef CM_SEARCH_LIMIT_AFTER	/* enable above only after 1 sentence found */
#undef CM_SEARCH_LIMIT_POP	/* terminate hypo of low CM on pop */
#endif

/* compute exact boundary instead of using 1st pass result */
/* also propagate exact time boundary to the right context after generation */
/* this may produce precise word boundary, but cause bigger word graph output */
#define GRAPHOUT_PRECISE_BOUNDARY

#undef GDEBUG			/* enable debug message in graphout.c */

#endif /* GRAPHOUT */

/* some decoding fix candidates */
#undef FIX_35_PASS2_STRICT_SCORE /* fix hypothesis scores by enabling
				      bt_discount_pescore() in standard mode
				      with PASS2_STRICT_IWCD, 
				   */
#define FIX_35_INHIBIT_SAME_WORD_EXPANSION /* privent connecting the same trellis word in 2nd pass */


/* below are new since 3.5.2 */
#ifdef GRAPHOUT

/** 
 * Allow overwriting existing graph word if score is higher.
 * By default, while search, Julius merges the same graph words appeared at the
 * same location as previously stored word, and terminate search.  This
 * option make Julius to continue search in that case if fscore_head of
 * current hypo. is greater than the already existing one.  In that case,
 * the score of existing one will be overridden by the new higher one.
 * (from 3.5.2)
 * 
 */
#define GRAPHOUT_OVERWRITE

/* with GRAPHOUT_OVERWRITE, use gscore_head instead of fscore_head */
/**
 * (EXPERIMENTAL) With GRAPHOUT_OVERWRITE, use gscore_head for the
 * comparison instead of fscore_head.
 * 
 */
#undef GRAPHOUT_OVERWRITE_GSCORE

/**
 * At post-processing of graph words, this option limits the number of
 * "fit boundary" loop up to this value.  This option is made to avoid
 * long loop by the "boundary oscillation" of short words. (from 3.5.2)
 * 
 */
#define GRAPHOUT_LIMIT_BOUNDARY_LOOP

/**
 * The maximum number of loop for GRAPHOUT_LIMIT_BOUNDARY_LOOP.
 * This is a system default value and can be modified by runtime option
 * "-graphboundloop"
 * 
 */
#define GRAPHOUT_LIMIT_BOUNDARY_LOOP_NUM_DEFAULT 20

/**
 * This option enables "-graphsearchdelay" and "-nographsearchdelay" option.
 * When "-graphsearchdelay" option is set, Julius modifies its alogrithm of
 * graph generation on the 2nd pass not to apply search termination by graph
 * merging until the first sentence candidate is found.
 *
 * This option may result in slight improvement of graph accuracy only
 * when you are going to generate a huge word graph by setting broad search.
 * Namely, it may result in better graph accuracy when you set wide beams on
 * both 1st pass "-b" and 2nd pass "-b2", and large number for "-n".
 * 
 */
#define GRAPHOUT_SEARCH_DELAY_TERMINATION

/**
 * This option enables word graph cutting by word depth at post-processing.
 * This option will erase many short words to explode at a wide beam width.
 * 
 */
#define GRAPHOUT_DEPTHCUT

/**
 * Default value of word graph depth cutting for GRAPHOUT_DEPTHCUT.
 * Setting larger value is safe for all condition.
 * 
 */
#define GRAPHOUT_DEPTHCUT_DEFAULT 80

#endif /* GRAPHOUT */

/**
 * Mimimal beam width that will be auto-determined for the 1st pass.
 * See set_beam_width() and default_width() for details.
 *
 */
#define MINIMAL_BEAM_WIDTH 200
