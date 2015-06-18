/**
 * @file   m_info.c
 * @author Akinobu Lee
 * @date   Thu May 12 14:14:01 2005
 * 
 * <JA>
 * @brief  起動時に認識システムの全情報を出力する．
 * </JA>
 * 
 * <EN>
 * @brief  Output all information of recognition system to standard out.
 * </EN>
 * 
 * $Revision: 1.12 $
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
 * 読み込んだモデルファイル名を出力する．
 * 
 * </JA>
 * <EN>
 * Output all file names of the models.
 * 
 * </EN>
 */
void
print_setting()
{
#ifdef USE_NETAUDIO
  char *p;
#endif
#ifdef USE_DFA
  GRAMLIST *g;
  int n;
#endif
  
  j_printf("    hmmfilename=%s\n",hmmfilename);
  if (mapfilename != NULL) {
    j_printf("    hmmmapfilename=%s\n",mapfilename);
  }
#ifdef USE_NGRAM
  j_printf("    vocabulary filename=%s\n",dictfilename);
  if (ngram_filename != NULL) {
    j_printf("    n-gram  filename=%s (binary format)\n",ngram_filename);
  } else {
    j_printf("    LR 2-gram filename=%s\n",ngram_filename_lr_arpa);
    if (ngram_filename_rl_arpa != NULL) {
      j_printf("    RL 3-gram filename=%s\n",ngram_filename_rl_arpa);
    }
  }
#else  /* USE_DFA */
  n = 1;
  for(g = gramlist_root; g; g = g->next) {
    j_printf("    grammar #%d:\n", n++);
    j_printf("        dfa  = %s\n", g->dfafile);
    j_printf("        dict = %s\n", g->dictfile);
  }
#endif
  if (hmm_gs_filename != NULL) {
    j_printf("    hmmfile for Gaussian Selection: %s\n", hmm_gs_filename);
  }
  if (gmm_filename != NULL) {
    j_printf("    GMM file for utterance verification: %s\n", gmm_filename);
  }
}

/** 
 * <JA>
 * 全てのシステム情報を出力する．
 * 
 * </JA>
 * <EN>
 * Output full system information.
 * 
 * </EN>
 */
void
print_info()
{
  j_printf("------------- System Info begin -------------\n");
  put_header(stdout);
  if (verbose_flag) {
    put_compile_defs(stdout);
    j_printf("\n");
#ifdef USE_NGRAM
    j_printf("Large Vocabulary Continuous Speech Recognition Based on N-gram\n\n");
#else  /* USE_DFA */
    j_printf("Continuous Speech Recognition Parser based on automaton grammar\n\n");
#endif
  }
  
  /* print current argument setting to stdout */
  j_printf("Files:\n");
  print_setting();
  j_printf("\n");

  /* for backward compatibility with scoring tool (IPA99)... :-( */
  if (speech_input == SP_RAWFILE) {
    j_printf("Speech input source: file\n\n");
  } else if (speech_input == SP_MFCFILE) {
    j_printf("Speech input source: MFCC parameter file (HTK format)\n\n");
  }

  if (speech_input != SP_MFCFILE) {

    put_para(&para);

    j_printf("\t base setup from =");
    if (para_htk.loaded == 1 || para_hmm.loaded == 1) {
      if (para_hmm.loaded == 1) {
	j_printf(" binhmm-embedded");
	if (para_htk.loaded == 1) {
	  j_printf(", then overridden by HTK Config and defaults");
	}
      } else {
	if (para_htk.loaded == 1) {
	  j_printf(" HTK Config (and HTK defaults)");
	}
      }
    } else {
      j_printf(" Julius defaults");
    }
    j_printf("\n");


    j_printf("\n");
    j_printf("    spectral subtraction = ");
    if (ssload_filename || sscalc) {
      if (sscalc) {
	j_printf("use head silence of each input\n");
	if (speech_input != SP_RAWFILE) {
	  j_error("Error: ss calculation with head silence only for rawfile input\n");
	}
	j_printf("\t head sil length = %d msec\n", sscalc_len);
      } else {			/* ssload_filename != NULL */
	j_printf("use a constant value from file\n");
	j_printf("     noise spectrum file = \"%s\"\n", ssload_filename);
      }
      j_printf("\t     alpha coef. = %f\n", para.ss_alpha);
      j_printf("\t  spectral floor = %f\n", para.ss_floor);
    } else {
      j_printf("off\n");
    }
    j_printf("\n");
  }
    
  print_hmmdef_info(hmminfo); j_printf("\n");
  if (hmm_gs_filename != NULL) {
    j_printf("GS ");
    print_hmmdef_info(hmm_gs); j_printf("\n");
  }
  if (winfo != NULL) {
    print_voca_info(winfo); j_printf("\n");
  }
  if (wchmm != NULL) {
    print_wchmm_info(wchmm); j_printf("\n");
  }
#ifdef USE_NGRAM
  print_ngram_info(ngram);
#else  /* USE_DFA */
  if (dfa != NULL) {
    print_dfa_info(dfa);
    if (debug2_flag) print_dfa_cp(dfa);
  }
#endif /* USE_NGRAM */

#ifdef USE_NGRAM
  j_printf("    inter-word N-gram cache: \n");
  {
    int num, len;
#ifdef UNIGRAM_FACTORING
    len = wchmm->isolatenum;
    j_printf("\t   root node to be cached = %d / %d (isolated only)\n",
	     len, wchmm->startnum);
#else
    len = wchmm->startnum;
    j_printf("\t   root node to be cached = %d (all)\n", len);
#endif
#ifdef HASH_CACHE_IW
    num = (iw_cache_rate * ngram->max_word_num) / 100;
    j_printf("\tword end num to be cached = %d / %d\n", num, ngram->max_word_num);
#else
    num = ngram->max_word_num;
    j_printf("\tword end num to be cached = %d (all)\n", num);
#endif
    j_printf("\t  maximum allocation size = %dMB\n", num * len / 1000 * sizeof(LOGPROB) / 1000);
  }

#endif /* USE_NGRAM */

  j_printf("\nWeights and words: \n");
#ifdef USE_NGRAM
  j_printf("\t(-lmp)  pass1 LM weight = %2.1f  ins. penalty = %+2.1f\n", lm_weight, lm_penalty);
  j_printf("\t(-lmp2) pass2 LM weight = %2.1f  ins. penalty = %+2.1f\n", lm_weight2, lm_penalty2);
  j_printf("\t(-transp)trans. penalty = %+2.1f per word\n", lm_penalty_trans);
  j_printf("\t(-silhead)head sil word = ");
  put_voca(winfo, winfo->head_silwid);
  j_printf("\t(-siltail)tail sil word = ");
  put_voca(winfo, winfo->tail_silwid);
#else
  j_printf("\t(-penalty1) IW penalty1 = %+2.1f\n", penalty1);
  j_printf("\t(-penalty2) IW penalty2 = %+2.1f\n", penalty2);
#endif /* USE_NGRAM */

#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
  j_printf("\t(-cmalpha)CM alpha coef = from %f to %f by step of %f (%d outputs)\n", cm_alpha_bgn, cm_alpha_end, cm_alpha_step, cm_alpha_num);
#else
  j_printf("\t(-cmalpha)CM alpha coef = %f\n", cm_alpha);
#endif
#ifdef CM_SEARCH_LIMIT
  j_printf("\t(-cmthres) CM cut thres = %f for hypo generation\n", cm_cut_thres);
#endif
#ifdef CM_SEARCH_LIMIT_POP
  j_printf("\t(-cmthres2)CM cut thres = %f for popped hypo\n", cm_cut_thres_pop);
#endif
#endif /* CONFIDENCE_MEASURE */
  j_printf("\t(-sp)shortpause HMM name= \"%s\" specified", spmodel_name);
  if (hmminfo->sp != NULL) {
    j_printf(", \"%s\" applied", hmminfo->sp->name);
    if (hmminfo->sp->is_pseudo) {
      j_printf(" (pseudo)");
    } else {
      j_printf(" (physical)");
    }
  }
  j_printf("\n");
#ifdef USE_DFA
  if (dfa != NULL) {
    int i;
    j_printf("\t  found sp category IDs =");
    for(i=0;i<dfa->term_num;i++) {
      if (dfa->is_sp[i]) {
	j_printf(" %d", i);
      }
    }
    j_printf("\n");
  }
#endif
#ifdef MULTIPATH_VERSION
  if (enable_iwsp) {
    j_printf("\t inter-word short pause = on (append \"%s\" for each word tail)\n", hmminfo->sp->name);
    j_printf("\t  sp transition penalty = %+2.1f\n", iwsp_penalty);
  }
#endif
#ifdef USE_NGRAM
  if (enable_iwspword) {
    j_printf("\tIW-sp word added to dict= \"%s\"\n", iwspentry);
  }
#endif
  
  if (gmm != NULL) {
    j_printf("\nUtterance verification by GMM\n");
    j_printf("           GMM defs file = %s\n", gmm_filename);
    j_printf("          GMM gprune num = %d\n", gmm_gprune_num);
    if (gmm_reject_cmn_string != NULL) {
      j_printf("   GMM names to reject = %s\n", gmm_reject_cmn_string);
    }
    j_printf("    ");
   print_hmmdef_info(gmm);
  }

  if (realtime_flag && para.cmn) {
    j_printf("\nMAP-CMN on realtime input: \n");
    if (cmnload_filename) {
      if (cmn_loaded) {
	j_printf("\t      initial CMN param = from \"%s\"\n", cmnload_filename);
      } else {
	j_printf("\t      initial CMN param = from \"%s\" (failed, ignored)\n", cmnload_filename);
      }
    } else {
      j_printf("\t      initial CMN param = not specified\n");
    }
    j_printf("\t    initial mean weight = %6.2f\n", cmn_map_weight);
    if (cmn_update) {
      j_printf("\t       CMN param update = yes, update from last inputs\n");
    } else {
      j_printf("\t       CMN param update = no, keep initial\n");
    }
    if (cmnsave_filename) {
      if (realtime_flag) {
	j_printf("\t      save CMN param to = %s\n", cmnsave_filename);
      } else {
	j_printf("\t      save CMN param to = %s (not realtime CMN, ignored)\n", cmnsave_filename);
      }
    }
  }

  j_printf("\nSearch parameters: \n");
    
  j_printf("\t      1st pass decoding = ");
  if (force_realtime_flag) j_printf("(forced) ");
  if (realtime_flag) {
    j_printf("on-the-fly");
    if (speech_input != SP_MFCFILE && para.cmn) j_printf(" with MAP-CMN");
    j_printf("\n");
  } else {
    j_printf("batch");
    if (speech_input != SP_MFCFILE && para.cmn) j_printf(" with sentence CMN");
    j_printf("\n");
  }
  j_printf("\t        1st pass method = ");
#ifdef WPAIR
# ifdef WPAIR_KEEP_NLIMIT
  j_printf("word-pair approx., keeping only N tokens ");
# else
  j_printf("word-pair approx. ");
# endif
#else
  j_printf("1-best approx. ");
#endif
#ifdef WORD_GRAPH
  j_printf("generating word_graph\n");
#else
  j_printf("generating indexed trellis\n");
#endif

  j_printf("\t(-b) trellis beam width = %d", trellis_beam_width);
  if (specified_trellis_beam_width == -1) {
    j_printf(" (-1 or not specified - guessed)\n");
  } else if (specified_trellis_beam_width == 0) {
    j_printf(" (0 - full)\n");
  } else {
    j_printf("\n");
  }
  j_printf("\t(-n)search candidate num= %d\n", nbest);
  j_printf("\t(-s)  search stack size = %d\n", stack_size);
  j_printf("\t(-m)    search overflow = after %d hypothesis poped\n", hypo_overflow);
  j_printf("\t        2nd pass method = ");
#ifdef GRAPHOUT
#ifdef GRAPHOUT_DYNAMIC
#ifdef GRAPHOUT_SEARCH
  j_printf("searching graph, generating dynamic graph\n");
#else
  j_printf("searching sentence, generating dynamic graph\n");
#endif /* GRAPHOUT_SEARCH */
#else  /* ~GRAPHOUT_DYNAMIC */
  j_printf("searching sentence, generating static graph from N-best\n");
#endif
#else  /* ~GRAPHOUT */
  j_printf("searching sentence, generating N-best\n");
#endif
  if (enveloped_bestfirst_width >= 0) {
    j_printf("\t(-b2)  pass2 beam width = %d\n", enveloped_bestfirst_width);
  }
  j_printf("\t(-lookuprange)lookup range= %d  (tm-%d <= t <tm+%d)\n",lookup_range,lookup_range,lookup_range);
#ifdef SCAN_BEAM
  j_printf("\t(-sb)2nd scan beamthres = %.1f (in logscore)\n",scan_beam_thres);
#endif
  j_printf("\t(-gprune)Gauss. pruning = ");
  switch(gprune_method){
  case GPRUNE_SEL_NONE: j_printf("none (full computation)\n"); break;
  case GPRUNE_SEL_BEAM: j_printf("beam\n"); break;
  case GPRUNE_SEL_HEURISTIC: j_printf("heuristic\n"); break;
  case GPRUNE_SEL_SAFE: j_printf("safe\n"); break;
  }
  if (gprune_method != GPRUNE_SEL_NONE) {
    j_printf("\t(-tmix)   mixture thres = %d / %d\n", mixnum_thres, hmminfo->maxcodebooksize);
  }
  if (hmm_gs_filename != NULL) {
    j_printf("\t(-gsnum)   GS state num = %d / %d selected\n", gs_statenum, hmm_gs->totalstatenum);
  }

  j_printf("\t(-n)        search till = %d candidates found\n", nbest);
  j_printf("\t(-output)    and output = %d candidates out of above\n", output_hypo_maxnum);
  if (ccd_flag) {
    j_printf("\t IWCD handling:\n");
#ifdef PASS1_IWCD
    j_printf("\t   1st pass: approximation ");
    switch(hmminfo->cdset_method) {
    case IWCD_AVG:
      j_printf("(use average prob. of same LC)\n");
      break;
    case IWCD_MAX:
      j_printf("(use max. prob. of same LC)\n");
      break;
    case IWCD_NBEST:
      j_printf("(use %d-best of same LC)\n", hmminfo->cdmax_num);
      break;
    }
#else
    j_printf("\t   1st pass: ignored\n");
#endif
#ifdef PASS2_STRICT_IWCD
    j_printf("\t   2nd pass: strict (apply when expanding hypo. )\n");
#else
    j_printf("\t   2nd pass: loose (apply when hypo. is popped and scanned)\n");
#endif
  }
  
#ifdef USE_NGRAM
  j_printf("\t factoring score: ");
#ifdef UNIGRAM_FACTORING
  j_printf("1-gram prob. (statically assigned beforehand)\n");
#else
  j_printf("2-gram prob. (dynamically computed while search)\n");
#endif
#endif /* USE_NGRAM */

  if (align_result_word_flag) {
    j_printf("\t output word alignments\n");
  }
  if (align_result_phoneme_flag) {
    j_printf("\t output phoneme alignments\n");
  }
  if (align_result_state_flag) {
    j_printf("\t output state alignments\n");
  }
#ifdef USE_DFA
  if (looktrellis_flag) {
    j_printf("\t only words in backtrellis will be expanded in 2nd pass\n");
  } else {
    j_printf("\t all possible words will be expanded in 2nd pass\n");
  }
#endif
#ifdef CATEGORY_TREE
  if (old_tree_function_flag) {
    j_printf("\t build_wchmm() used\n");
  } else {
    j_printf("\t build_wchmm2() used\n");
  }
#ifdef PASS1_IWCD
  if (old_iwcd_flag) {
    j_printf("\t full lcdset used\n");
  } else {
    j_printf("\t lcdset limited by word-pair constraint\n");
  }
#endif
#endif
  if (progout_flag) j_printf("\tprogressive output on 1st pass\n");
  /* if (param_kind != NULL) {
    j_printf("Selectively use input parameter vector as: %s\n", param_kind);
  } */
  if (compute_only_1pass) {
    j_printf("\tCompute only 1-pass\n");
  }
#ifdef CONFIDENCE_MEASURE
  j_printf("\t output word confidence measure ");
#ifdef CM_NBEST
  j_printf("based on N-best candidates\n");
#endif
#ifdef CM_SEARCH
  j_printf("based on search-time scores\n");
#endif
#endif /* CONFIDENCE_MEASURE */
  
#ifdef GRAPHOUT
  j_printf("\nGraph output:\n");
  j_printf("\t(-graphrange)    margin = %d frames", graph_merge_neighbor_range);
  if (graph_merge_neighbor_range < 0) {
    j_printf(" (all post-marging disabled)\n");
  } else if (graph_merge_neighbor_range == 0) {
    j_printf(" (merge same word with the same boundary)\n");
  } else {
    j_printf(" (merge same words around this margin)\n");
  }
#ifdef GRAPHOUT_DEPTHCUT
  j_printf("\t(-graphcut)cutoff depth = ");
  if (graphout_cut_depth < 0) {
    j_printf("disabled (-1)\n");
  } else {
    j_printf("%d words\n",graphout_cut_depth);
  }
#endif
#ifdef GRAPHOUT_LIMIT_BOUNDARY_LOOP
  j_printf("\t(-graphboundloop)loopmax= %d for boundary adjustment\n",graphout_limit_boundary_loop_num);
#endif
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
  j_printf("\tInhibit graph search termination before 1st sentence found = ");
  if (graphout_search_delay) {
    j_printf("enabled\n");
  } else {
    j_printf("disabled\n");
  }
#endif
#endif /* GRAPHOUT */
  
  j_printf("\nSystem I/O configuration:\n");
  j_printf("\t    speech input source = ");
  if (speech_input == SP_RAWFILE) {
    j_printf("speech file\n");
    j_printf("\t         input filelist = ");
    if (inputlist_filename == NULL) {
      j_printf("(none, enter filenames from stdin)\n");
    } else {
      j_printf("%s\n", inputlist_filename);
    }
  } else if (speech_input == SP_MFCFILE) {
    j_printf("MFCC parameter file (HTK format)\n");
    j_printf("\t               filelist = ");
    if (inputlist_filename == NULL) {
      j_printf("(none, enter filenames from stdin)\n");
    } else {
      j_printf("%s\n", inputlist_filename);
    }
  } else if (speech_input == SP_STDIN) {
    j_printf("standard input\n");
  } else if (speech_input == SP_ADINNET) {
    j_printf("adinnet client\n");
#ifdef USE_NETAUDIO
  } else if (speech_input == SP_NETAUDIO) {
    char *p;
    j_printf("NetAudio server on ");
    if (netaudio_devname != NULL) {
      j_printf("%s\n", netaudio_devname);
    } else if ((p = getenv("AUDIO_DEVICE")) != NULL) {
      j_printf("%s\n", p);
    } else {
      j_printf("local port\n");
    }
#endif
  } else if (speech_input == SP_MIC) {
    j_printf("microphone\n");
  }
  if (speech_input != SP_MFCFILE) {
    if (speech_input == SP_RAWFILE || speech_input == SP_STDIN || speech_input == SP_ADINNET) {
      if (use_ds48to16) {
	j_printf("\t         sampling freq. = assume 48000Hz, then down to %dHz\n", para.smp_freq);
      } else {
	j_printf("\t         sampling freq. = %d Hz required\n", para.smp_freq);
      }
    } else {
      if (use_ds48to16) {
	j_printf("\t         sampling freq. = 48000Hz, then down to %d Hz\n", para.smp_freq);
      } else {
 	j_printf("\t         sampling freq. = %d Hz\n", para.smp_freq);
      }
    }
  }
  if (speech_input != SP_MFCFILE) {
    j_printf("\t        threaded A/D-in = ");
#ifdef HAVE_PTHREAD
    if (query_thread_on()) {
      j_printf("supported, on\n");
    } else {
      j_printf("supported, off\n");
    }
#else
    j_printf("not supported (live input may be dropped)\n");
#endif
  }
  if (strip_zero_sample) {
    j_printf("\t  zero frames stripping = on\n");
  } else {
    j_printf("\t  zero frames stripping = off\n");
  }
  if (speech_input != SP_MFCFILE) {
    if (query_segment_on()) {
      j_printf("\t        silence cutting = on\n");
      j_printf("\t            level thres = %d / 32767\n", level_thres);
      j_printf("\t        zerocross thres = %d / sec.\n", zero_cross_num);
      j_printf("\t            head margin = %d msec.\n", head_margin_msec);
      j_printf("\t            tail margin = %d msec.\n", tail_margin_msec);
    } else {
      j_printf("\t        silence cutting = off\n");
    }
    if (use_zmean || para.zmeanframe) {
      j_printf("\t       remove DC offset = on");
      if (para.zmeanframe) {
	j_printf(" (frame-wise)\n");
      }
      if (speech_input == SP_RAWFILE) {
	j_printf(" (will compute for each file)\n");
      } else {
	j_printf(" (will compute from first %.1f sec)\n",
		 (float)ZMEANSAMPLES / (float)para.smp_freq);
      }
    } else {
      j_printf("\t       remove DC offset = off\n");
    }
  }
  j_printf("\t     reject short input = ");
  if (rejectshortlen > 0) {
    j_printf("< %d msec\n", rejectshortlen);
  } else {
    j_printf("off\n");
  }
#ifdef SP_BREAK_CURRENT_FRAME
  j_printf("\tshort pause segmentation= on\n");
  j_printf("\t     sp duration length = %d frames\n", sp_frame_duration);
#else
  j_printf("\tshort pause segmentation= off\n");
#endif
  j_printf("\t       result output to = ");
  switch(result_output) {
  case SP_RESULT_TTY:
    j_printf("tty (standard out)\n"); break;
  case SP_RESULT_MSOCK:
    j_printf("msock\n"); break;
  }
  if (progout_flag) {
    j_printf("\t       progout interval = %d msec\n", progout_interval);
  }
  if (speech_input != SP_MFCFILE) {
    if (record_dirname != NULL) {
      j_printf("\tspeech data stored to = %s/\n", record_dirname);
    }
  }
  j_printf("\t   output charset conv. = ");
#ifdef CHARACTER_CONVERSION
  if (to_code == NULL) {
    j_printf("disabled\n");
  } else {
    if (from_code != NULL) {
      j_printf("from \"%s\" ", from_code);
    }
    j_printf("to \"%s\"\n", to_code);
  }
#else
  j_printf("not supported\n");
#endif /* CHARACTER_CONVERSION */
  j_printf("\n------------- System Info end -------------\n");

#ifdef USE_MIC
  if (realtime_flag) {
    if (para.cmn) {
      if (cmn_loaded) {
	j_printf("\ninitial CMN parameter loaded from file\n");
      } else {
	j_printf("\n");
	j_printf("\t*************************************************************\n");
	j_printf("\t* NOTICE: The first input may not be recognized, since      *\n");
	j_printf("\t*         no initial CMN parameter is available on startup. *\n");
	j_printf("\t*************************************************************\n");
      }
    }
    if (para.energy && para.enormal) {
      j_printf("\t*************************************************************\n");
      j_printf("\t* NOTICE: Energy normalization is activated on live input:  *\n");
      j_printf("\t*         maximum energy of LAST INPUT will be used for it. *\n");
      j_printf("\t*         So, the first input will not be recognized.       *\n");
      j_printf("\t*************************************************************\n");
    }
  }
#endif
}
