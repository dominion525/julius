/**
 * @file   global.h
 * @author Akinobu Lee
 * @date   Sun Sep 18 23:53:17 2005
 * 
 * <JA>
 * @brief  大域変数の定義
 *
 * 値はデフォルト値として用いられます．
 *
 * </JA>
 * 
 * <EN>
 * @brief  Global variables
 *
 * Dfeault values are specified in this file.
 * 
 * </EN>
 * 
 * $Revision: 1.13 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_EXTERNAL_DEFINITION__
#define __SENT_EXTERNAL_DEFINITION__

#include <sent/stddefs.h>
#include <sent/hmm.h>
#include <sent/vocabulary.h>
#ifdef USE_NGRAM
#include <sent/ngram2.h>
#else  /* USE_DFA */
#include <sent/dfa.h>
#endif
#include "wchmm.h"
#include "search.h"

/**
 * If GLOBAL_VARIABLE_DEFINE is defined, global variables are actually made.
 * Else, these are external definition.
 * 
 */
#ifdef GLOBAL_VARIABLE_DEFINE
#define GLOBAL /*  */
#define GLOBAL_VAL(v) = (v)
#else
#define GLOBAL extern
#define GLOBAL_VAL(v) /*  */
#endif


/* -------------------------------------------------------------- */
/* ------- User-specified filenames ----------------------------- */
/* -------------------------------------------------------------- */

/********************************************/
/* File names (NULL if not specified) */
GLOBAL char *hmmfilename GLOBAL_VAL(NULL);///< HMM definition file
GLOBAL char *mapfilename GLOBAL_VAL(NULL); ///< HMMlist file
GLOBAL char *hmm_gs_filename GLOBAL_VAL(NULL); ///< HMM definition for GMS
GLOBAL char *dictfilename GLOBAL_VAL(NULL); ///< Word dictionary
#ifdef USE_NGRAM
GLOBAL char *ngram_filename GLOBAL_VAL(NULL); ///< N-gram (binary format)
GLOBAL char *ngram_filename_lr_arpa GLOBAL_VAL(NULL); ///< N-gram in ARPA format(LR 2-gram)
GLOBAL char *ngram_filename_rl_arpa GLOBAL_VAL(NULL); ///< N-gram in ARPA format(RL 3-gram)
#endif
#ifdef USE_DFA
GLOBAL char *dfa_filename GLOBAL_VAL(NULL); ///< Grammar file
GLOBAL GRAMLIST *gramlist_root GLOBAL_VAL(NULL); ///< List of grammars to be read at startup
#endif

GLOBAL char *inputlist_filename GLOBAL_VAL(NULL); ///< Input file list
GLOBAL char *cmnload_filename GLOBAL_VAL(NULL);	///< Load CMN parameter from
GLOBAL char *cmnsave_filename GLOBAL_VAL(NULL);	///< Save CMN parameter to
GLOBAL char *ssload_filename GLOBAL_VAL(NULL); ///< Load SS parameter from
GLOBAL char *record_dirname GLOBAL_VAL(NULL); ///< Record speech data to this dir
GLOBAL char *gmm_filename GLOBAL_VAL(NULL); ///< GMM definition file


/* -------------------------------------------------------------- */
/* ------- User-specified parameters --------- ------------------ */
/* -------------------------------------------------------------- */

/********************************************/
/* Input source */
GLOBAL int speech_input GLOBAL_VAL(SP_MFCFILE); ///< Selected input source
GLOBAL int adinnet_port GLOBAL_VAL(ADINNET_PORT); ///< Port number for adinnet input
#ifdef USE_NETAUDIO
GLOBAL char *netaudio_devname GLOBAL_VAL(NULL); ///< Host/unit name for NetAudio/DatLink input
#endif

/********************************************/
/* Input triggering and silence cutting */
GLOBAL int silence_cut GLOBAL_VAL(2); ///< 0..off, 1..on, 2..use device default
GLOBAL int level_thres GLOBAL_VAL(2000); ///< Level threshold
GLOBAL int zero_cross_num GLOBAL_VAL(60); ///< Zero cross number threshold per a second
GLOBAL int head_margin_msec GLOBAL_VAL(300); ///< Head margin in msec
GLOBAL int tail_margin_msec GLOBAL_VAL(400); ///< Tail margin in msec

GLOBAL boolean strip_zero_sample GLOBAL_VAL(TRUE); ///< If TRUE, strip off zero samples
GLOBAL boolean use_zmean GLOBAL_VAL(FALSE); ///< If TRUE, remove DC offset by zero mean

/********************************************/
/* Speech analysis parameters */
GLOBAL Value para;		///< MFCC computation parameters
GLOBAL Value para_hmm;		///< MFCC computation parameters from HMM header
GLOBAL Value para_htk;	///< MFCC parameters read from HTK Config
GLOBAL Value para_default;    ///< default MFCC computation parameters
GLOBAL Value para_dummy;    ///< dummy entry for reading binary GMM/GMS HMMs

/********************************************/
/* Spectral subtraction */
GLOBAL boolean sscalc GLOBAL_VAL(FALSE); ///< If TRUE, compute SS using head silence for file input
GLOBAL int sscalc_len GLOBAL_VAL(300); ///< Length of speech at input head to compute SS in msec
/* ssfloor, ssalpha moved into Value */

/********************************************/
/* LM parameters (default will be set in m_bootup.c) */
#ifdef USE_NGRAM
GLOBAL LOGPROB lm_weight;	///< Language model weight
GLOBAL LOGPROB lm_penalty;	///< Word insertion penalty
GLOBAL LOGPROB lm_weight2;	///< Language model weight for 2nd pass
GLOBAL LOGPROB lm_penalty2;	///< Word insertion penalty for 2nd pass
GLOBAL LOGPROB lm_penalty_trans GLOBAL_VAL(0.0); ///< Additional insertion penalty for transparent words
#endif /* USE_NGRAM */
#ifdef USE_DFA
GLOBAL LOGPROB penalty1 GLOBAL_VAL(0.0); ///< Insertion penalty on 1st pass of Julian 
GLOBAL LOGPROB penalty2 GLOBAL_VAL(0.0); ///< Insertion penalty on 2nd pass of Julian
#endif

#ifdef CONFIDENCE_MEASURE
/********************************************/
/* Confidence measure */
#ifdef CM_MULTIPLE_ALPHA	/* Test multiple alpha coef. for confidence scoring */
GLOBAL LOGPROB cm_alpha_bgn GLOBAL_VAL(0.03); ///< Begin value of alpha
GLOBAL LOGPROB cm_alpha_end GLOBAL_VAL(0.15); ///< End value of alpha
GLOBAL LOGPROB cm_alpha_step GLOBAL_VAL(0.03); ///< Step value of alpha
GLOBAL int cm_alpha_num GLOBAL_VAL(5); ///< Number of test values (will be set from above values)
#else  /* single value (default) */
GLOBAL LOGPROB cm_alpha GLOBAL_VAL(0.05); ///< Scaling factor for confidence scoring
#endif
#ifdef CM_SEARCH_LIMIT
GLOBAL LOGPROB cm_cut_thres GLOBAL_VAL(0.03); ///< Cut-off threshold for generated hypo. for confidence decoding
#endif
#ifdef CM_SEARCH_LIMIT_POP
GLOBAL LOGPROB cm_cut_thres_pop GLOBAL_VAL(0.1); ///< Cut-off threshold for popped hypo. for confidence decoding
#endif
#endif /* CONFIDENCE_MEASURE */

/********************************************/
/* Pause (silence) model related */
#ifdef USE_NGRAM
GLOBAL char *head_silname GLOBAL_VAL(NULL); ///< Silence model name at beginning of search
GLOBAL char *tail_silname GLOBAL_VAL(NULL); ///< Silence model name at end of search
/* Short pause word, that will be added to dictionary when "-iwspword" is spcified */
GLOBAL boolean enable_iwspword GLOBAL_VAL(FALSE); ///< Enable addition of short pause word to dictionary
GLOBAL char *iwspentry GLOBAL_VAL(NULL); ///< Dictionary entry to be added as short-pause word
#endif
GLOBAL char *spmodel_name GLOBAL_VAL(NULL); ///< Logical HMM name of short pause model

#ifdef MULTIPATH_VERSION
/* 1) in DFA mode, a word with only "spmodel_name" model as a pronunciation will be specially handled as "short-pause word" */
/* 2) if "-iwsp" enabled, the "spmodel_name" model will be attached to every word end within the dictionary */
/* short pause special handling */
GLOBAL boolean enable_iwsp GLOBAL_VAL(FALSE); /* enable inter-word short pause handling */
GLOBAL LOGPROB iwsp_penalty GLOBAL_VAL(IWSP_PENALTY_DEFAULT); /* transition penalty of inter-word short pause */
#endif

/**********************************************/
/* Search parameters for acoustic computation */
GLOBAL int gprune_method GLOBAL_VAL(GPRUNE_SEL_UNDEF); ///< Gaussian pruning method (default: use default of engine configuration)
GLOBAL int mixnum_thres GLOBAL_VAL(2);	///< Gaussian pruning: number of Gaussian to select per mixture
GLOBAL int gs_statenum GLOBAL_VAL(24);	///< GMS: number of mixture PDF to select

/********************************************/
/* Input rejection by GMM and input length */
GLOBAL int gmm_gprune_num GLOBAL_VAL(10); ///< Number of Gaussians to be computed for GMM
GLOBAL char *gmm_reject_cmn_string GLOBAL_VAL(NULL); ///< Comma-separated list of GMM to be rejected
GLOBAL int rejectshortlen GLOBAL_VAL(0); ///< Length threshold to reject input

/********************************************/
/* Search parameters for 1st pass */
GLOBAL int specified_trellis_beam_width GLOBAL_VAL(-1); ///< User-specified beam width of 1st pass (-1: not specified, 0: guess from vocabulary size)
GLOBAL int trellis_beam_width GLOBAL_VAL(-1); ///< Actual beam width of 1st pass (will be set on startup)
#ifdef SEPARATE_BY_UNIGRAM
GLOBAL int separate_wnum GLOBAL_VAL(DEFAULT_SEPARATE_WNUM); ///< Number of best frequency words to be separated (linearized) from lexicon tree
#endif
#ifdef WPAIR
# ifdef WPAIR_KEEP_NLIMIT
GLOBAL int wpair_keep_nlimit GLOBAL_VAL(3); ///< Keeps only N token on word-pair approximation
# endif
#endif
#ifdef HASH_CACHE_IW
GLOBAL int iw_cache_rate GLOBAL_VAL(10); ///< Inter-word LM cache size rate
#endif

/********************************************/
/* Search parameters for 2nd pass */
GLOBAL int enveloped_bestfirst_width GLOBAL_VAL(30); ///< Word beam width of 2nd pass (word envelope) (-1 for no beaming)
#ifdef SCAN_BEAM
GLOBAL LOGPROB scan_beam_thres GLOBAL_VAL(80.0); ///< Score beam threshold of 2nd pass (score envelope)
#endif
GLOBAL int hypo_overflow GLOBAL_VAL(2000); ///< Hypo. overflow threshold at 2nd pass
GLOBAL int stack_size GLOBAL_VAL(500); ///< Hypo. stack size of 2nd pass
GLOBAL int lookup_range GLOBAL_VAL(5); ///< Trellis lookup frame range for word expansion in 2nd pass
GLOBAL int nbest;		///< Search until n sentence are found
GLOBAL int output_hypo_maxnum GLOBAL_VAL(1); ///< Number of sentence to output

#ifdef GRAPHOUT
/********************************************/
/* Word graph output */
GLOBAL int graph_merge_neighbor_range GLOBAL_VAL(0); ///< Allowed margin for post-merging on word graph generation
GLOBAL int graph_totalwordnum GLOBAL_VAL(0); ///< Total number of words in the generated graph
#ifdef GRAPHOUT_DEPTHCUT
GLOBAL int graphout_cut_depth GLOBAL_VAL(GRAPHOUT_DEPTHCUT_DEFAULT); ///< density threshold to cut word graph at post-processing
#endif
#ifdef GRAPHOUT_LIMIT_BOUNDARY_LOOP
GLOBAL int graphout_limit_boundary_loop_num GLOBAL_VAL(GRAPHOUT_LIMIT_BOUNDARY_LOOP_NUM_DEFAULT); ///< Maximum loop iteration for word boundary adjustment
#endif
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
GLOBAL boolean graphout_search_delay GLOBAL_VAL(FALSE); ///< If TRUE, inhibit termination by graph word merging before the first sentence candidate is found.
#endif
#endif /* GRAPHOUT */

#ifdef SP_BREAK_CURRENT_FRAME
/********************************************/
/* Short-pause segmentation */
GLOBAL int sp_frame_duration GLOBAL_VAL(10);///< Default length of short-pause frames to be segmented
#endif /* SP_BREAK_CURRENT_FRAME */

/********************************************/
/* Output parameters */
GLOBAL int result_output GLOBAL_VAL(SP_RESULT_TTY);///< Selected output device
GLOBAL int progout_interval GLOBAL_VAL(300); ///< Progressive output interval on 1st pass in msec
#ifdef CHARACTER_CONVERSION
GLOBAL char *from_code GLOBAL_VAL(NULL); ///< Input character set name
GLOBAL char *to_code GLOBAL_VAL(NULL); ///< Output character set name
#endif

/********************************************/
/* Module mode */
GLOBAL int module_port GLOBAL_VAL(DEFAULT_MODULEPORT); ///< Port number to listen

/********************************************/
/* MAP-CMN */
GLOBAL float cmn_map_weight GLOBAL_VAL(100.0); ///< Weight for initial cepstral mean on MAP-CMN

/* -------------------------------------------------------------- */
/* -------- User-specified switches ----------------------------- */
/* -------------------------------------------------------------- */

/********************************************/
/* Enable/disable extra check mode for debugging */
GLOBAL boolean wchmm_check_flag GLOBAL_VAL(FALSE); ///< Enter lexicon structure consulting mode after bootup if TRUE
GLOBAL boolean trellis_check_flag GLOBAL_VAL(FALSE); ///< Enter trellis interactive check routine after bootup if TRUE
GLOBAL boolean triphone_check_flag GLOBAL_VAL(FALSE); ///< Enter triphone existence check routine after bootup if TRUE

/********************************************/
/* Dictionary */
GLOBAL boolean forcedict_flag GLOBAL_VAL(FALSE); ///< TRUE if ignore error words in dictionary and does not stop on error

/********************************************/
/* Search */
GLOBAL boolean compute_only_1pass GLOBAL_VAL(FALSE); ///< TRUE if compute only 1pass
GLOBAL boolean forced_realtime GLOBAL_VAL(FALSE); ///< TRUE if do on-the-fly decoding on 1st pass
GLOBAL boolean force_realtime_flag GLOBAL_VAL(FALSE); ///< TRUE if force on-the-fly decoding, or FALSE if use device-specific default
#ifdef CATEGORY_TREE
GLOBAL boolean old_tree_function_flag GLOBAL_VAL(FALSE); ///< TRUE if use old build_wchmm() instead of build_wchmm2() for lexicon construction (for debug only)
#ifdef PASS1_IWCD
GLOBAL boolean old_iwcd_flag GLOBAL_VAL(FALSE); ///< TRUE if use old full lcdset instead of category-pair-aware lcdset on Julian
#endif
#endif
#ifdef USE_NGRAM
GLOBAL short iwcdmethod GLOBAL_VAL(IWCD_NBEST); ///< Calculation method for outprob score of a lcdset on cross-word triphone (default: use average of N-best)
#else
GLOBAL short iwcdmethod GLOBAL_VAL(IWCD_AVG); ///< Calculation method for outprob score of a lcdset on cross-word triphone (default: use average of all)
#endif
GLOBAL short iwcdmaxn GLOBAL_VAL(3); ///< Number of states to be computed on top if IWCD_NBEST is specified in @a iwcdmethod
#ifdef USE_DFA
GLOBAL boolean looktrellis_flag GLOBAL_VAL(FALSE); ///< TRUE of limit expansion words for trellis words on neighbor frames at 2nd pass of Julian
GLOBAL boolean multigramout_flag GLOBAL_VAL(FALSE); ///< TRUE if perform per-grammar sequencial decoding on 2nd pass
#endif
GLOBAL boolean result_reorder_flag GLOBAL_VAL(TRUE); ///< Re-order n-best result by score on 2nd pass if TRUE

/********************************************/
/* Forced alignment */
GLOBAL boolean align_result_word_flag GLOBAL_VAL(FALSE); ///< If TRUE, do forced alignment per word
GLOBAL boolean align_result_phoneme_flag GLOBAL_VAL(FALSE); ///< If TRUE, do forced alignment per phoneme
GLOBAL boolean align_result_state_flag GLOBAL_VAL(FALSE); ///< If TRUE, do forced alignment per state

/********************************************/
/* Module mode */
GLOBAL boolean module_mode GLOBAL_VAL(FALSE);///< TRUE if started as module
GLOBAL boolean fork_on_module_connection GLOBAL_VAL(DEFAULT_FORK_ON_MODULE); ///< If TRUE, process will fork on the client connection on module mode

/********************************************/
/* Output switches */
GLOBAL boolean verbose_flag GLOBAL_VAL(TRUE); ///< Verbose output
GLOBAL boolean debug2_flag GLOBAL_VAL(FALSE); ///< Debug output
GLOBAL boolean paramtype_check_flag GLOBAL_VAL(TRUE); ///< if TRUE, check input parameter type with header of the hmmdefs
GLOBAL boolean progout_flag GLOBAL_VAL(FALSE); ///< Progressive output
#ifdef USE_NGRAM
GLOBAL boolean separate_score_flag GLOBAL_VAL(FALSE); ///< Output both AM and LM score separately
#endif

/********************************************/
/* CMN switches */
GLOBAL boolean cmn_update GLOBAL_VAL(TRUE); ///< update CMN while recognition

/* -------------------------------------------------------------- */
/* -------- Inner status holders -------------------------------- */
/* -------------------------------------------------------------- */
/* for LM */
GLOBAL boolean lmp_specified GLOBAL_VAL(FALSE); ///< True if -lmp specified
GLOBAL boolean lmp2_specified GLOBAL_VAL(FALSE); ///< True if -lmp2 specified
/* for search */
GLOBAL boolean realtime_flag GLOBAL_VAL(FALSE);	///< Do on-the-fly decoding if TRUE
GLOBAL boolean ccd_flag GLOBAL_VAL(TRUE); ///< Handle hmmdefs as context-dependent HMM if TRUE (default determined from hmmdefs macro name)
GLOBAL boolean ccd_flag_force GLOBAL_VAL(FALSE); ///< If TRUE, user-specified ccd_flag will override hmmdefs defaults

/* -------------------------------------------------------------- */
/* -------- Work area for search -------------------------------- */
/* -------------------------------------------------------------- */

/* Models */
GLOBAL HTK_HMM_INFO *hmminfo GLOBAL_VAL(NULL);///< Main phoneme HMM 
GLOBAL HTK_HMM_INFO *hmm_gs GLOBAL_VAL(NULL); ///< HMM for Gaussian Selection
GLOBAL WORD_INFO *winfo GLOBAL_VAL(NULL); ///< Main Word dictionary
#ifdef USE_NGRAM
GLOBAL NGRAM_INFO *ngram GLOBAL_VAL(NULL); ///< Main N-gram language model
#else
GLOBAL MULTIGRAM *gramlist GLOBAL_VAL(NULL); ///< List of grammars
GLOBAL DFA_INFO *dfa GLOBAL_VAL(NULL); ///< Global DFA grammar, generated from @a gramlist.
#endif
GLOBAL HTK_HMM_INFO *gmm GLOBAL_VAL(NULL); ///< GMM for utterance verification

/* Tree lexicon HMM */
GLOBAL WCHMM_INFO *wchmm GLOBAL_VAL(NULL);///< Word-conjunction HMM as tree lexicon
GLOBAL BACKTRELLIS backtrellis; ///< Word trellis index generated at the 1st pass
GLOBAL LOGPROB backmax;		///< Maximum score of best hypothesis at 1st pass

/* Misc. for search */
GLOBAL int peseqlen;		///< Input length in frames

/* Score envelope beaming */
#ifdef SCAN_BEAM
GLOBAL LOGPROB *framemaxscore;///< Maximum score of each frame on 2nd pass
#endif

/* Input speech related */
GLOBAL SP16 speech[MAXSPEECHLEN]; ///< Input speech data
GLOBAL int speechlen GLOBAL_VAL(0);		///< Length of @a speech
GLOBAL boolean cmn_loaded GLOBAL_VAL(FALSE); ///< TRUE if CMN parameter loaded from file at boot up
GLOBAL boolean use_ds48to16 GLOBAL_VAL(FALSE); ///< TRUE if use 48kHz input and perform down sampling to 16kHz

/* Spectral subtraction */
GLOBAL float *ssbuf GLOBAL_VAL(NULL);///< Estimated noise spectrum
GLOBAL int sslen;		///< Length of @a ssbuf

#ifdef SP_BREAK_CURRENT_FRAME
/* Short-pause segmentation */
GLOBAL HTK_Param *rest_param GLOBAL_VAL(NULL);///< Rest parameter for next segment
GLOBAL WORD_ID sp_break_last_word GLOBAL_VAL(WORD_INVALID); ///< Last maximum word hypothesis on the begin point
GLOBAL WORD_ID sp_break_last_nword GLOBAL_VAL(WORD_INVALID); ///< Last (not transparent) context word for LM
GLOBAL boolean sp_break_last_nword_allow_override GLOBAL_VAL(TRUE); ///< Allow override of last context word from result of 2nd pass
GLOBAL WORD_ID sp_break_2_begin_word GLOBAL_VAL(WORD_INVALID); ///< Search start word on 2nd pass
GLOBAL WORD_ID sp_break_2_end_word GLOBAL_VAL(WORD_INVALID); ///< Search end word on 2nd pass
#endif

/* Module */
GLOBAL int module_sd GLOBAL_VAL(-1);	///< Socket to the connected client

/* Output */
GLOBAL WORD_ID pass1_wseq[MAXSEQNUM]; ///< Word sequence of best hypothesis on 1st pass
GLOBAL int pass1_wnum; ///< Number of words in @a pass1_wseq
GLOBAL LOGPROB pass1_score;	///< Score of @a pass1_wseq

/* Pointer to text output functions (will be set by 'result_output') */
GLOBAL void (*status_process_online)();
GLOBAL void (*status_process_offline)();
GLOBAL void (*status_recready)();
GLOBAL void (*status_recstart)();
GLOBAL void (*status_recend)();
GLOBAL void (*status_param)(HTK_Param *p);
GLOBAL void (*result_pass1_begin)();
GLOBAL void (*result_pass1_current)(int t, WORD_ID *seq, int num, LOGPROB score, LOGPROB LMscore, WORD_INFO *winfo);
GLOBAL void (*result_pass1_final)(WORD_ID *seq, int num, LOGPROB score, LOGPROB LMscore, WORD_INFO *winfo);
GLOBAL void (*result_pass1_end)();
GLOBAL void (*result_pass2_begin)();
GLOBAL void (*result_pass2)(NODE *hypo, int rank, WORD_INFO *winfo);
GLOBAL void (*result_pass2_end)();
GLOBAL void (*result_pass2_failed)(WORD_INFO *winfo);
GLOBAL void (*result_rejected)(const char *);
GLOBAL void (*result_gmm)();
#ifdef GRAPHOUT
GLOBAL void (*result_graph)(WordGraph *root, WORD_INFO *winfo);
#endif

#endif /* __SENT_EXTERNAL_DEFINITION__ */
