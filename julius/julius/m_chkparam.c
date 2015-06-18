/**
 * @file   m_chkparam.c
 * @author Akinobu LEE
 * @date   Fri Mar 18 16:31:45 2005
 * 
 * <JA>
 * @brief  指定オプションの整合性チェック，およびデフォルト値の設定．
 * </JA>
 * 
 * <EN>
 * @brief  Check option parameters and set default if needed.
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
 * ファイルが存在して読み込み可能かチェックする．
 * 
 * @param filename [in] ファイルパス名
 * </JA>
 * <EN>
 * Check if a file actually exist and is readable.
 * 
 * @param filename [in] file path name
 * </EN>
 */
void
checkpath(char *filename)
{
  if (access(filename, R_OK) == -1) {
    perror("checkpath");
    j_error("%s: cannot access %s\n", EXECNAME, filename);
  }
}

#if !defined(_WIN32) || defined(__CYGWIN32__)
/** 
 * <JA>
 * ディレクトリが存在して書き込み可能かチェックする．
 * 
 * @param dirname [in] ディレクトリパス名
 * </JA>
 * <EN>
 * Check if a directory exists and is writable.
 * 
 * @param dirname [in] directory path name
 * </EN>
 */
static void
checkdir(char *dirname)
{
  if (access(dirname, R_OK | W_OK | X_OK) == -1) {
    perror("checkdir");
    j_error("%s: cannot write to dir %s\n", EXECNAME, dirname);
  }
}
#endif

/** 
 * <JA>
 * @brief  指定されたパラメータをチェックする．
 *
 * ファイルの存在チェックやパラメータ指定の整合性，モデルとの対応
 * などについてチェックを行なう．重要な誤りが見つかった場合エラー終了する．
 * 
 * </JA>
 * <EN>
 * @brief  Check the user-specified parameters.
 *
 * This functions checks whether the specified files actually exist,
 * and also the mutual coherence of the parameters and their correspondence
 * with used model is also checked.  If a serious error is found, it
 * produces error and exits.
 * 
 * </EN>
 */
void
check_specs()
{
  boolean ok_p;

  VERMES("###### check configurations\n");

  /* check if needed files are specified */
  ok_p = TRUE;
  if (hmmfilename == NULL) {
    j_printerr("Error: needs HMM definition file (-h hmmdef_file)\n");
    ok_p = FALSE;
  }
#ifdef USE_NGRAM
  if (dictfilename == NULL) {
    j_printerr("Error: needs dictionary file (-v dict_file)\n");
    ok_p = FALSE;
  }
  /* only LR 2-gram specified .... calculate only 1-pass */
  /* only RL 3-gram specified .... ERROR */
  if (ngram_filename == NULL) {
    if (ngram_filename_lr_arpa == NULL && ngram_filename_rl_arpa == NULL) {
      j_printerr("Error: needs word n-gram file (-d bingram | -nlr ARPA_2gram -nrl ARPA_r3gram)\n");
      ok_p = FALSE;
    } else if (ngram_filename_lr_arpa == NULL) {
      j_printerr("Error: also needs ARPA 2-gram file (-nlr ARPA_2gram_file)\n");
      ok_p = FALSE;
    } else if (ngram_filename_rl_arpa == NULL) {
      compute_only_1pass = TRUE;
    }
  }
#else  /* USE_DFA */
  if (!module_mode) {
    if (gramlist_root == NULL) {
      if (dfa_filename == NULL) {
	j_printerr("Error: needs DFA grammar file (-dfa file)\n");
	j_printerr("Error: or specify grammar prefix (-gram prefix | -gramlist listfile)\n");
	ok_p = FALSE;
      }
      if (dictfilename == NULL) {
	j_printerr("Error: needs dictionary file (-v file)\n");
	j_printerr("Error: or specify grammar prefix (-gram prefix | -gramlist listfile)\n");
	ok_p = FALSE;
      }
    }
  }
#endif

  /* file existence check */
  if (hmmfilename != NULL) checkpath(hmmfilename);
  if (mapfilename != NULL) checkpath(mapfilename);
  if (dictfilename != NULL) checkpath(dictfilename);
#ifdef USE_NGRAM
  if (ngram_filename != NULL) checkpath(ngram_filename);
  if (ngram_filename_lr_arpa != NULL) checkpath(ngram_filename_lr_arpa);
  if (ngram_filename_rl_arpa != NULL) checkpath(ngram_filename_rl_arpa);
#else
  if (dfa_filename != NULL) checkpath(dfa_filename);
#endif
  if (hmm_gs_filename != NULL) checkpath(hmm_gs_filename);
  if (gmm_filename != NULL) checkpath(gmm_filename);
  if (inputlist_filename != NULL) {
    if (speech_input != SP_RAWFILE && speech_input != SP_MFCFILE) {
      j_printerr("Warning: not file input, \"-filelist %s\" ignored\n", inputlist_filename);
    } else {
      checkpath(inputlist_filename);
    }
  }
  /* cmn{save,load}_filename allows missing file (skipped if missing) */
  if (ssload_filename != NULL) checkpath(ssload_filename);

  /* check if record dir exists */
#if !defined(_WIN32) || defined(__CYGWIN32__)
  if (record_dirname != NULL) checkdir(record_dirname);
#endif

  /* set default realtime flag according to input mode */
  if (force_realtime_flag) {
    if (speech_input == SP_MFCFILE) {
      j_printerr("Warning: realtime decoding of mfcfile is not supported yet\n");
      j_printerr("Warning: -realtime turned off\n");
      realtime_flag = FALSE;
    } else {
      realtime_flag = forced_realtime;
    }
  }

  /* check for cmn */
  if (realtime_flag) {
    if (cmn_update == FALSE && cmnload_filename == NULL) {
      j_error("Error: when \"-cmnnoupdate\", initial cepstral mean should be given by \"-cmnload\"\n");
    }
  }

#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
  if (module_mode) {
    j_error("module mode conflicts with \"--enable-cm-multiple-alpha\"!\n");
  }
#endif
#endif /* CONFIDENCE_MEASURE */

  if (!ok_p) {
    j_error("check spec failed\n");            /* exit on error */
  }
}

/******* set default params suitable for the models and setting *******/

/** 
 * <JA>
 * @brief  あらかじめ定められた第1パスのデフォルトビーム幅を返す．
 *
 * デフォルトのビーム幅は，認識エンジンのコンパイル時設定や
 * 使用する音響モデルに従って選択される．これらの値は，20k の
 * IPA 評価セットで得られた最適値（精度を保ちつつ最大速度が得られる値）
 * である．
 * 
 * @return 実行時の条件によって選択されたビーム幅
 * </JA>
 * <EN>
 * @brief  Returns the pre-defined default beam width on 1st pass of
 * beam search.
 * 
 * The default beam width will be selected from the pre-defined values
 * according to the compilation-time engine setting and the type of
 * acoustic model.  The pre-defined values were determined from the
 * development experiments on IPA evaluation testset of Japanese 20k-word
 * dictation task.
 * 
 * @return the selected default beam width.
 * </EN>
 */
static int
default_width()
{
  if (strmatch(SETUP, "fast")) { /* for fast setup */
    if (hmminfo->is_triphone) {
      if (hmminfo->is_tied_mixture) {
	/* tied-mixture triphones (PTM etc.) */
	return(600);
      } else {
	/* shared-state triphone */
#ifdef PASS1_IWCD
	return(800);
#else
	/* v2.1 compliant (no IWCD on 1st pass) */
	return(1000);		
#endif
      }
    } else {
      /* monophone */
      return(400);
    }
  } else {			/* for standard / v2.1 setup */
    if (hmminfo->is_triphone) {
      if (hmminfo->is_tied_mixture) {
	/* tied-mixture triphones (PTM etc.) */
	return(800);
      } else {
	/* shared-state triphone */
#ifdef PASS1_IWCD
	return(1500);
#else
	return(1500);		/* v2.1 compliant (no IWCD on 1st pass) */
#endif
      }
    } else {
      /* monophone */
      return(700);
    }
  }
}

/** 
 * <JA>
 * @brief  第1パスのビーム幅を決定する．
 *
 * ユーザが "-b" オプションでビーム幅を指定しなかった場合は，
 * 下記のうち小さい方がビーム幅として採用される．
 *   - default_width() の値
 *   - sqrt(語彙数) * 15
 * 
 * @param wchmm [in] 木構造化辞書
 * @param specified [in] ユーザ指定ビーム幅(0: 全探索 -1: 未指定)
 * 
 * @return 採用されたビーム幅．
 * </JA>
 * <EN>
 * @brief  Determine beam width on the 1st pass.
 * 
 * @param wchmm [in] tree lexicon data
 * @param specified [in] user-specified beam width (0: full search,
 * -1: not specified)
 * 
 * @return the final beam width to be used.
 * </EN>
 */
int
set_beam_width(WCHMM_INFO *wchmm, int specified)
{
  int width;
  int standard_width;
  
  if (specified == 0) { /* full search */
    VERMES("doing full search\n");
    VERMES("Warning: this can be extremely slow\n");
    width = wchmm->n;
  } else if (specified == -1) { /* not specified */
    standard_width = default_width(); /* system default */
    width = (int)(sqrt(wchmm->winfo->num) * 15.0); /* heuristic value!! */
    if (width > standard_width) width = standard_width;
    /* 2007/1/20 bgn */
    if (width < MINIMAL_BEAM_WIDTH) {
      width = MINIMAL_BEAM_WIDTH;
    }
    /* 2007/1/20 end */
  } else {			/* actual value has been specified */
    width = specified;
  }
  if (width > wchmm->n) width = wchmm->n;

  return(width);
}

#ifdef USE_NGRAM
/** 
 * <JA>
 * 第1パスの言語モデルの重みと単語挿入ペナルティのデフォルト値を，
 * 音響モデルの型に従ってセットする．
 * 
 * </JA>
 * <EN>
 * Set default values of LM weight and word insertion penalty on the 1st pass
 * depending on the acoustic model type.
 * 
 * </EN>
 */
void
set_lm_weight()
{
  if (hmminfo->is_triphone) {
    lm_weight = DEFAULT_LM_WEIGHT_TRI_PASS1;
    lm_penalty = DEFAULT_LM_PENALTY_TRI_PASS1;
  } else {
    lm_weight = DEFAULT_LM_WEIGHT_MONO_PASS1;
    lm_penalty = DEFAULT_LM_PENALTY_MONO_PASS1;
  }
}

/** 
 * <JA>
 * 第2パスの言語モデルの重みと単語挿入ペナルティのデフォルト値を，
 * 音響モデルの型に従ってセットする．
 * 
 * </JA>
 * <EN>
 * Set default values of LM weight and word insertion penalty on the 2nd pass
 * depending on the acoustic model type.
 * 
 * </EN>
 */
void
set_lm_weight2()
{
  if (hmminfo->is_triphone) {
    lm_weight2 = DEFAULT_LM_WEIGHT_TRI_PASS2;
    lm_penalty2 = DEFAULT_LM_PENALTY_TRI_PASS2;
  } else {
    lm_weight2 = DEFAULT_LM_WEIGHT_MONO_PASS2;
    lm_penalty2 = DEFAULT_LM_PENALTY_MONO_PASS2;
  }
}
    
#endif /* USE_NGRAM */
