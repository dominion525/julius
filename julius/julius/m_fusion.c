/**
 * @file   m_fusion.c
 * @author Akinobu Lee
 * @date   Thu May 12 13:31:47 2005
 * 
 * <JA>
 * @brief  モデルの読み込みとデータの構築を行ない，認識の準備をする．
 * </JA>
 * 
 * <EN>
 * @brief  Read all models and build data for recognition.
 * </EN>
 * 
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

/** 
 * <JA>
 * 音響HMMをファイルから読み込んでセットアップする．
 * </JA>
 * <EN>
 * Read in an acoustic HMM from file and setup for recognition.
 * </EN>
 */
static void
initialize_HMM()
{
  /* at here, global variable "para" holds values specified by user or
     by user-specified HTK config file */
  
  /* allocate new hmminfo */
  hmminfo = hmminfo_new();
  /* load hmmdefs */
  init_hmminfo(hmminfo, hmmfilename, mapfilename, &para_hmm);
  /* only MFCC is supported for audio input */
  /* MFCC_{0|E}[_D][_A][_Z][_N] is supported */
  /* check parameter type of this acoustic HMM */
  if (speech_input != SP_MFCFILE) {
    /* Decode parameter extraction type according to the training
       parameter type in the header of the given acoustic HMM */
    if ((hmminfo->opt.param_type & F_BASEMASK) != F_MFCC) {
      j_error("Error: for direct speech input, only HMM trained by MFCC is supported\n");
    }
    /* set acoustic analysis parameters from HMM header */
    calc_para_from_header(&para, hmminfo->opt.param_type, hmminfo->opt.vec_size);
  }
  /* check if tied_mixture */
  if (hmminfo->is_tied_mixture && hmminfo->codebooknum <= 0) {
    j_error("%s: this tied-mixture model has no codebook!?\n", EXECNAME);
  }
#ifdef PASS1_IWCD
  /* make state clusters of same context for inter-word triphone approx. */
  if (hmminfo->is_triphone) {
    j_printerr("Making pseudo bi/mono-phone for IW-triphone...");
    if (make_cdset(hmminfo) == FALSE) {
      j_error("\nError: failed to make context-dependent state set\n");
    }
    /* add those `pseudo' biphone and monophone to the logical HMM names */
    /* they points not to the defined HMM, but to the CD_Set structure */
    hmm_add_pseudo_phones(hmminfo);
    j_printerr("done\n");
  }
#endif

  /* find short pause model and set to hmminfo->sp */
  htk_hmm_set_pause_model(hmminfo, spmodel_name);

  /* set flag for context dependent handling (if not specified in command arg)*/
  if (!ccd_flag_force) {
    if (hmminfo->is_triphone) {
      ccd_flag = TRUE;
    } else {
      ccd_flag = FALSE;
    }
  }
  /* set which iwcd1 method to use */
  hmminfo->cdset_method = iwcdmethod;
  hmminfo->cdmax_num = iwcdmaxn;

#ifdef MULTIPATH_VERSION
  /* find short-pause model */
  if (enable_iwsp) {
    if (hmminfo->sp == NULL) {
      j_error("cannot find short pause model \"%s\" in hmmdefs\n", spmodel_name);
    }
    hmminfo->iwsp_penalty = iwsp_penalty;
  }
#endif
  
}

/** 
 * <JA>
 * Gaussian Mixture Selection のための状態選択用モノフォンHMMを読み込む．
 * </JA>
 * <EN>
 * Initialize context-independent HMM for state selection with Gaussian
 * Mixture Selection.
 * </EN>
 */
static void
initialize_GSHMM()
{
  j_printerr("Reading GS HMMs:\n");
  hmm_gs = hmminfo_new();
  undef_para(&para_dummy);
  init_hmminfo(hmm_gs, hmm_gs_filename, NULL, &para_dummy);
}

/* initialize GMM for utterance verification */
/** 
 * <JA>
 * 発話検証・棄却用の1状態 GMM を読み込んで初期化する．
 * 
 * </JA>
 * <EN>
 * Read and initialize an 1-state GMM for utterance verification and
 * rejection.
 * 
 * </EN>
 */
static void
initialize_GMM()
{
  j_printerr("Reading GMM:\n");
  gmm = hmminfo_new();
  undef_para(&para_dummy);
  init_hmminfo(gmm, gmm_filename, NULL, &para_dummy);

  gmm_init(gmm, gmm_gprune_num);
}

/* initialize word dictionary */
/** 
 * <JA>
 * 単語辞書をファイルから読み込んでセットアップする．
 * 
 * </JA>
 * <EN>
 * Read in word dictionary from a file and setup for recognition.
 * 
 * </EN>
 */
static void
initialize_dict()
{
  /* allocate new word dictionary */
  winfo = word_info_new();
  /* read in dictinary from file */
  if ( ! 
#ifdef MONOTREE
      /* leave winfo monophone for 1st pass lexicon tree */
       init_voca(winfo, dictfilename, hmminfo, TRUE, forcedict_flag)
#else 
       init_voca(winfo, dictfilename, hmminfo, FALSE, forcedict_flag)
#endif
       ) {
    j_error("ERROR: failed to read dictionary, terminated\n");
  }

#ifdef USE_NGRAM
  /* if necessary, append a IW-sp word to the dict if "-iwspword" specified */
  if (enable_iwspword) {
    if (
#ifdef MONOTREE
	voca_append_htkdict(iwspentry, winfo, hmminfo, TRUE)
#else 
	voca_append_htkdict(iwspentry, winfo, hmminfo, FALSE)
#endif
	== FALSE) {
      j_error("Error: failed to make IW-sp word entry \"%s\"\n", iwspentry);
    } else {
      j_printerr("1 IW-sp word entry added\n");
    }
  }
  /* set {head,tail}_silwid */
  winfo->head_silwid = voca_lookup_wid(head_silname, winfo);
  if (winfo->head_silwid == WORD_INVALID) { /* not exist */
    j_error("ERROR: head sil word \"%s\" not exist in voca\n", head_silname);
  }
  winfo->tail_silwid = voca_lookup_wid(tail_silname, winfo);
  if (winfo->tail_silwid == WORD_INVALID) { /* not exist */
    j_error("ERROR: tail sil word \"%s\" not exist in voca\n", tail_silname);
  }
#endif
  
#ifdef PASS1_IWCD
  if (triphone_check_flag && hmminfo->is_triphone) {
    /* go into interactive triphone HMM check mode */
    hmm_check(hmminfo, winfo);
  }
#endif


}


#ifdef USE_NGRAM

/** 
 * <JA>
 * 単語N-gramをファイルから読み込んでセットアップする．
 * 
 * </JA>
 * <EN>
 * Read in word N-gram from file and setup for recognition.
 * 
 * </EN>
 */
static void
initialize_ngram()
{
  /* allocate new */
  ngram = ngram_info_new();
  /* load LM */
  if (ngram_filename != NULL) {	/* binary format */
    init_ngram_bin(ngram, ngram_filename);
  } else {			/* ARPA format */
    init_ngram_arpa(ngram, ngram_filename_lr_arpa, ngram_filename_rl_arpa);
  }

  /* map dict item to N-gram entry */
  make_voca_ref(ngram, winfo);
}

#endif /* USE_NGRAM */

/* set params whose default will change by models and not specified in arg */
/** 
 * <JA>
 * @brief モデルに依存したデフォルト値の設定
 * 
 * 言語重みや Gaussian pruning アルゴリズムの選択など，モデルによって
 * デフォルト設定が異なるパラメータをここで決定する．なおユーザによって
 * 明示的に指定されている場合はそちらを優先する．
 * </JA>
 * <EN>
 * @brief Set model-dependent default values.
 *
 * The default values of parameters which depends on the using models,
 * such as language weights, insertion penalty, gaussian pruning
 * methods and so on, are determined at this function.  If values are
 * explicitly defined in jconf file or command argument at run time,
 * they will be used instead.
 * </EN>
 */
static void
configure_param()
{
#ifdef USE_NGRAM
  /* set default lm parameter */
  if (!lmp_specified) set_lm_weight();
  if (!lmp2_specified) set_lm_weight2();
  if (lmp_specified != lmp2_specified) {
    j_printerr("Warning: only -lmp or -lmp2 specified, LM weights may be unbalanced\n");
  }
#endif
  /* select Gaussian pruning function */
  if (gprune_method == GPRUNE_SEL_UNDEF) {/* set default if not specified */
    if (hmminfo->is_tied_mixture) {
      /* enabled by default for tied-mixture models */
#ifdef GPRUNE_DEFAULT_SAFE
      gprune_method = GPRUNE_SEL_SAFE;
#elif GPRUNE_DEFAULT_HEURISTIC
      gprune_method = GPRUNE_SEL_HEURISTIC;
#elif GPRUNE_DEFAULT_BEAM
      gprune_method = GPRUNE_SEL_BEAM;
#endif
    } else {
      /* disabled by default for non tied-mixture model */
      gprune_method = GPRUNE_SEL_NONE;
    }
  }
}

/** 
 * <JA>
 * ログや認識結果の出力先をセットアップする．
 * 
 * </JA>
 * <EN>
 * Setup functions to output recognition results and logs.
 * 
 * </EN>
 */
void
select_result_output()
{
  switch(result_output) {
  case SP_RESULT_TTY: setup_result_tty(); break; /* in result_tty.c */
  case SP_RESULT_MSOCK: setup_result_msock(); break; /* in result_msock.c */
  default:
    j_printerr("Internal Error: no such result output device: id = %d\n", result_output);
    break;
  }

}



/**********************************************************************/
/** 
 * <JA>
 * @brief  全てのモデルを読み込み，認識の準備を行なう．
 *
 * 認識で用いる全てのモデルを読み込んだあと，モデル同士の関連づけや
 * 木構造化辞書の構築，
 * キャッシュの確保を行なう．またいくつかのデフォルトパラメータの決定
 * や，結果の出力先の設定もおこなう．
 * 
 * </JA>
 * <EN>
 * @brief  Read in all models and prepare data for recognition.
 *
 * This function reads in all the models needed for recognition,
 * inspects the models and their relations, builds a tree lexicon, and
 * allocate cache.  Some default parameters will also be set here.
 * 
 * </EN>
 */
void
final_fusion()
{
  VERMES("###### build up system\n");

  /* stage 1: load models */
  initialize_HMM();
  if (hmm_gs_filename != NULL) initialize_GSHMM();
  if (gmm_filename != NULL) initialize_GMM();
#ifdef USE_NGRAM
  initialize_dict();
  initialize_ngram();
#endif

  /* stage 2: fixate params */
  /* set params whose default will change by models and not specified in arg */
  configure_param();
  /* 
     gather all the MFCC configuration parameters to form final config.
       preference: Julian option > HTK config > HMM > Julian default
     With HTK config, the default values are overridden to HTK values.
  */
  if (para_htk.loaded == 1) apply_para(&para, &para_htk);
  if (para_hmm.loaded == 1) apply_para(&para, &para_hmm);
  apply_para(&para, &para_default);

  /* stage 3: build lexicon tree */
#ifdef USE_DFA

  /* read and setup all the initial grammars */
  if (dfa_filename != NULL && dictfilename != NULL) {
    multigram_add_gramlist(dfa_filename, dictfilename);
  }
  multigram_read_all_gramlist();
  
  /* execute generation of global grammar and (re)building of wchmm */
  multigram_exec();

#else  /* ~USE_DFA */

  wchmm = wchmm_new();
  wchmm->ngram = ngram;
  wchmm->winfo = winfo;
  wchmm->hmminfo = hmminfo;
#ifdef CATEGORY_TREE
  if (old_tree_function_flag) {
    build_wchmm(wchmm);
  } else {
    build_wchmm2(wchmm);
  }
#else
  build_wchmm2(wchmm);
#endif /* CATEGORY_TREE */
  /* set actual beam width */
  /* guess beam width from models, when not specified */
  trellis_beam_width = set_beam_width(wchmm, specified_trellis_beam_width);

#endif			/* USE_DFA */

#ifdef MONOTREE
  /* after building tree lexocon, */
  /* convert monophone to triphone in winfo for 2nd pass */
  if (hmminfo->is_triphone) {
    j_printerr("convert monophone dictionary to word-internal triphone...");
    if (voca_mono2tri(winfo, hmminfo) == FALSE) {
      j_error("failed\n");
    }
    j_printerr("done\n");
  }
#endif
  
  /* stage 4: setup output function */
  if (hmm_gs_filename != NULL) {/* with GMS */
    outprob_init(hmminfo, hmm_gs, gs_statenum, gprune_method, mixnum_thres);
  } else {
    outprob_init(hmminfo, NULL, 0, gprune_method, mixnum_thres);
  }

  /* stage 5: initialize work area and misc. */
  bt_init(&backtrellis);	/* backtrellis initialization */
#ifdef USE_NGRAM
  max_successor_cache_init(wchmm);	/* initialize cache for factoring */
#endif
  if (realtime_flag) {
    RealTimeInit();		/* prepare for 1st pass pipeline processing */
  }
  /* setup result output function */
  select_result_output();

  /* finished! */
  VERMES("All init successfully done\n\n");
}
