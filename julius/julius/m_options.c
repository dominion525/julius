/**
 * @file   m_options.c
 * @author Akinobu Lee
 * @date   Thu May 12 18:52:07 2005
 * 
 * <JA>
 * @brief  jconfやコマンドからのオプション設定を処理する．
 * </JA>
 * 
 * <EN>
 * @brief  Process options and configurations from jconf or command argument.
 * </EN>
 * 
 * $Revision: 1.16 $
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
 * @brief  相対パスをフルパスに変換する．
 * 
 * ファイルのパス名が相対パスであれば，カレントディレクトリをつけた
 * フルパスに変換して返す．絶対パスであれば，そのまま返す．
 * 
 * @param filename [in] ファイルのパス名
 * @param dirname [in] カレントディレクトリのパス名
 * 
 * @return 絶対パス名の入った，新たに割り付けられたバッファ
 * </JA>
 * <EN>
 * @brief  Change relative path to full path.
 *
 * If the file path is given as relative, prepend the dirname to it.
 * If the file path is full, just copy it to new buffer and return.
 * 
 * @param filename [in] file path name
 * @param dirname [in] full path of current directory
 * 
 * @return newly malloced buffer holding the full path name.
 * </EN>
 */
char *
filepath(char *filename, char *dirname)
{
  char *p;
  if (dirname != NULL && filename[0] != '/'
#if defined(_WIN32)
      && filename[0] != '\\' && !(strlen(filename) >= 3 && filename[1] == ':')
#endif
      ) {
    p = (char *)mymalloc(strlen(filename) + strlen(dirname) + 1);
    strcpy(p, dirname);
    strcat(p, filename);
  } else {
    p = strcpy((char *)mymalloc(strlen(filename)+1), filename);
  }
  return p;
}

/** 
 * <JA>
 * 必要な引数が与えられなかった場合にエラー終了する．
 * 
 * @param opt [in] オプションの文字列
 * 
 * @return NULL.
 * </JA>
 * <EN>
 * Error exit when required argument was not specified.
 * 
 * @param opt [in] option string
 * 
 * @return NULL.
 * </EN>
 */
static char *
args_needed_exit(char *opt)
{
  j_printerr("%s: option requires argument -- %s\n", EXECNAME, opt);
  opt_terminate();
  return(NULL);
}

/** 
 * <JA>
 * @brief メモリ領域を解放し NULL で埋める．
 * @param p [i/o] メモリ領域の先頭を指すポインタ変数へのポインタ
 * @note @a p が NULL の場合は何も起こらない。
 * </JA>
 */
#define FREE_MEMORY(p) \
  {if (*(p)) {free(*(p)); *(p) = NULL;}}

/**
 * <JA>
 * @brief  オプションを解析して対応する値をセットする．
 *
 * @param argc [in] @a argv に含まれる引数の数
 * @param argv [in] 引数を表す文字列の配列
 * @param cwd [in] カレントディレクトリ文字列
 * </JA>
 * <EN>
 * Parse options and set values.
 * 
 * @param argc [in] number of elements in @a argv
 * @param argv [in] array of argument strings
 * @param cwd [in] current directory string
 * </EN>
 */
void
opt_parse(int argc, char *argv[], char *cwd)
{
  char *tmparg;
  int i;

#define NEXTARG (++i >= argc) ? (char *)args_needed_exit(argv[i-1]) : argv[i] ///< Points to next argument, or error if not exist

  if (argc == 1) {		/* no argument */
    usage();
  }
  
  for (i=1;i<argc;i++) {
    if (strmatch(argv[i],"-C")) { /* include jconf file  */
      tmparg = filepath(NEXTARG, cwd);
      config_file_parse(tmparg);
      free(tmparg);
      continue;
    } else if (strmatch(argv[i],"-input")) { /* speech input */
      tmparg = NEXTARG;
      if (strmatch(tmparg,"file")) {
	speech_input = SP_RAWFILE;
	realtime_flag = FALSE;
      } else if (strmatch(tmparg,"rawfile")) {
	speech_input = SP_RAWFILE;
	realtime_flag = FALSE;
      } else if (strmatch(tmparg,"htkparam")) {
	speech_input = SP_MFCFILE;
	realtime_flag = FALSE;
      } else if (strmatch(tmparg,"mfcfile")) {
	speech_input = SP_MFCFILE;
	realtime_flag = FALSE;
      } else if (strmatch(tmparg,"stdin")) {
	speech_input = SP_STDIN;
	realtime_flag = FALSE;
      } else if (strmatch(tmparg,"adinnet")) {
	speech_input = SP_ADINNET;
	realtime_flag = TRUE;
#ifdef USE_NETAUDIO
      } else if (strmatch(tmparg,"netaudio")) {
	speech_input = SP_NETAUDIO;
	realtime_flag = TRUE;
#endif
#ifdef USE_MIC
      } else if (strmatch(tmparg,"mic")) {
	speech_input = SP_MIC;
	realtime_flag = TRUE;
#endif
      } else if (strmatch(tmparg,"file")) { /* for 1.1 compat */
	speech_input = SP_RAWFILE;
	realtime_flag = FALSE;
      } else if (strmatch(tmparg,"mfc")) { /* for 1.1 compat */
	speech_input = SP_MFCFILE;
	realtime_flag = FALSE;
      } else {
	j_printerr("%s: no such speech input source \"%s\" supported\n", argv[0], tmparg);
	opt_terminate();
      }
      continue;
    } else if (strmatch(argv[i],"-filelist")) {	/* input file list */
      FREE_MEMORY(&inputlist_filename);
      tmparg = NEXTARG;
      inputlist_filename = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
    } else if (strmatch(argv[i],"-record")) {	/* record speech data*/
      FREE_MEMORY(&record_dirname);
      tmparg = NEXTARG;
      record_dirname = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
    } else if (strmatch(argv[i],"-rejectshort")) { /* short input rejection */
      rejectshortlen = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-module")) { /* enable module mode */
      module_mode = TRUE;
      if (i+1 < argc) {
	if (argv[i+1][0] >= '0' && argv[i+1][0] <= '9') {
	  module_port = atoi(NEXTARG);
	}
      }
      /* this option implicitly includes "-result msock" */
      result_output = SP_RESULT_MSOCK;
      continue;
    } else if (strmatch(argv[i],"-fork")) { /* force forking on module mode */
      fork_on_module_connection = TRUE;
      continue;
    } else if (strmatch(argv[i],"-nofork")) { /* disable forking on module mode */
      fork_on_module_connection = FALSE;
      continue;
    } else if (strmatch(argv[i],"-result")) { /* result output */
      tmparg = NEXTARG;
      if (strmatch(tmparg,"tty")) {
	result_output = SP_RESULT_TTY;
      } else if (strmatch(tmparg,"msock")) {
	result_output = SP_RESULT_MSOCK;
      } else {
	j_printerr("%s: no such result output \"%s\"\n", argv[0], tmparg);
	opt_terminate();
      }
      continue;
    } else if (strmatch(argv[i],"-outcode")) {
      decode_output_selection(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-force_realtime")) { /* force realtime */
      tmparg = NEXTARG;
      if (strmatch(tmparg, "on")) {
	forced_realtime = TRUE;
      } else if (strmatch(tmparg, "off")) {
	forced_realtime = FALSE;
      } else {
	j_printerr("%s: \"-force_realtime\" allows \"on\" or \"off\" only\n", EXECNAME);
	opt_terminate();
      }
      force_realtime_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-realtime")) {	/* equal to "-force_realtime on" */
      forced_realtime = TRUE;
      force_realtime_flag = TRUE;
      continue;
    } else if (strmatch(argv[i], "-norealtime")) { /* equal to "-force_realtime off" */
      forced_realtime = FALSE;
      force_realtime_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-forcedict")) { /* skip dict error */
      forcedict_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-check")) { /* interactive model check mode */
      tmparg = NEXTARG;
      if (strmatch(tmparg, "wchmm")) {
	wchmm_check_flag = TRUE;
      } else if (strmatch(tmparg, "trellis")) {
	trellis_check_flag = TRUE;
      } else if (strmatch(tmparg, "triphone")) {
	triphone_check_flag = TRUE;
      } else {
	j_printerr("%s: invalid check style: %s\n", argv[0], tmparg);
	opt_terminate();
      }
      continue;
    } else if (strmatch(argv[i],"-notypecheck")) { /* don't check param type */
      paramtype_check_flag = FALSE;
      continue;
    } else if (strmatch(argv[i],"-separatescore")) { /* output LM/AM score */
#ifdef USE_NGRAM
      separate_score_flag = TRUE;
#else  /* USE_DFA */
      j_printerr("Warning: option \"-separatescore\" ignored\n");
#endif
      continue;
    } else if (strmatch(argv[i],"-nlimit")) { /* limit N token in a node */
#ifdef WPAIR_KEEP_NLIMIT
      wpair_keep_nlimit = atoi(NEXTARG);
#else
      j_printerr("Warning: option \"-nlimit\" ignored\n");
#endif
      continue;
    } else if (strmatch(argv[i],"-lookuprange")) { /* trellis neighbor range */
      lookup_range = atoi(NEXTARG);
      continue;
#ifdef GRAPHOUT
    } else if (strmatch(argv[i],"-graphrange")) { /* neighbor merge range frame */
      graph_merge_neighbor_range = atoi(NEXTARG);
      continue;
#ifdef GRAPHOUT_DEPTHCUT
    } else if (strmatch(argv[i],"-graphcut")) { /* cut graph word by depth */
      graphout_cut_depth = atoi(NEXTARG);
      continue;
#endif
#ifdef GRAPHOUT_LIMIT_BOUNDARY_LOOP
    } else if (strmatch(argv[i],"-graphboundloop")) { /* neighbor merge range frame */
      graphout_limit_boundary_loop_num = atoi(NEXTARG);
      continue;
#endif
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
    } else if (strmatch(argv[i],"-graphsearchdelay")) { /* not do graph search termination before the 1st sentence is found */
      graphout_search_delay = TRUE;
      continue;
    } else if (strmatch(argv[i],"-nographsearchdelay")) { /* not do graph search termination before the 1st sentence is found */
      graphout_search_delay = FALSE;
      continue;
#endif
#endif /* GRAPHOUT */
#ifdef USE_DFA
    } else if (strmatch(argv[i],"-looktrellis")) { /* activate loopuprange */
      looktrellis_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-multigramout")) { /* enable per-grammar decoding on 2nd pass */
      multigramout_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-nomultigramout")) { /* disable per-grammar decoding on 2nd pass */
      multigramout_flag = FALSE;
      continue;
#endif
#ifdef CATEGORY_TREE
    } else if (strmatch(argv[i],"-oldtree")) { /* use old tree function */
      old_tree_function_flag = TRUE;
      continue;
#ifdef PASS1_IWCD
    } else if (strmatch(argv[i],"-oldiwcd")) { /* use full lcd ignoring category constraint */
      old_iwcd_flag = TRUE;
      continue;
#endif
#endif
    } else if (strmatch(argv[i],"-sb")) { /* score envelope width in 2nd pass */
#ifdef SCAN_BEAM
      scan_beam_thres = atof(NEXTARG);
#else
      j_printerr("Warning: option \"-sb\" ignored\n");
#endif
      continue;
    } else if (strmatch(argv[i],"-discount")) {	/* (bogus) */
      j_printerr("Warning: option \"-discount\" ignored\n");
      continue;
    } else if (strmatch(argv[i],"-cutsilence")) { /* force (long) silence detection on */
      silence_cut = 1;
      continue;
    } else if (strmatch(argv[i],"-nocutsilence")) { /* force (long) silence detection off */
      silence_cut = 0;
      continue;
    } else if (strmatch(argv[i],"-pausesegment")) { /* force (long) silence detection on (for backward compatibility) */
      silence_cut = 1;
      continue;
    } else if (strmatch(argv[i],"-nopausesegment")) { /* force (long) silence detection off (for backward comatibility) */
      silence_cut = 0;
      continue;
    } else if (strmatch(argv[i],"-lv")) { /* silence detection threshold level */
      level_thres = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-zc")) { /* silence detection zero cross num */
      zero_cross_num = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-headmargin")) { /* head silence length */
      head_margin_msec = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-tailmargin")) { /* tail silence length */
      tail_margin_msec = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-hipass")||strmatch(argv[i],"-hifreq")) { /* frequency of upper band limit */
      para.hipass = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-lopass")||strmatch(argv[i],"-lofreq")) { /* frequency of lower band limit */
      para.lopass = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-smpPeriod")) { /* sample period (ns) */
      para.smp_period = atoi(NEXTARG);
      para.smp_freq = period2freq(para.smp_period);
      continue;
    } else if (strmatch(argv[i],"-smpFreq")) { /* sample frequency (Hz) */
      para.smp_freq = atoi(NEXTARG);
      para.smp_period = freq2period(para.smp_freq);
      continue;
    } else if (strmatch(argv[i],"-fsize")) { /* Window size */
      para.framesize = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-fshift")) { /* Frame shiht */
      para.frameshift = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-preemph")) {
      para.preEmph = atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-fbank")) {
      para.fbank_num = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-ceplif")) {
      para.lifter = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-rawe")) {
      para.raw_e = TRUE;
      continue;
    } else if (strmatch(argv[i],"-norawe")) {
      para.raw_e = FALSE;
      continue;
    } else if (strmatch(argv[i],"-enormal")) {
      para.enormal = TRUE;
      continue;
    } else if (strmatch(argv[i],"-noenormal")) {
      para.enormal = FALSE;
      continue;
    } else if (strmatch(argv[i],"-escale")) {
      para.escale = atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-silfloor")) {
      para.silFloor = atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-delwin")) { /* Delta window length */
      para.delWin = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-accwin")) { /* Acceleration window length */
      para.accWin = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-ssalpha")) { /* alpha coef. for SS */
      para.ss_alpha = atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-ssfloor")) { /* spectral floor for SS */
      para.ss_floor = atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-48")) { /* use 48kHz input and down to 16kHz */
      use_ds48to16 = TRUE;
      continue;
    } else if (strmatch(argv[i],"-version") || strmatch(argv[i], "--version") || strmatch(argv[i], "-setting") || strmatch(argv[i], "--setting")) { /* print version and exit */
      put_header(stderr);
      put_compile_defs(stderr);
      j_printerr("\n");
      put_library_defs(stderr);
      j_exit();
    } else if (strmatch(argv[i],"-quiet")) { /* minimum output */
      debug2_flag = verbose_flag = FALSE;
      continue;
    } else if (strmatch(argv[i],"-debug")) { /* debug mode: output huge log */
      debug2_flag = verbose_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-progout")) { /* enable progressive output */
      progout_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-proginterval")) { /* interval for -progout */
      progout_interval = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-nocharconv")) { /* disable character set conversion */
#ifdef CHARACTER_CONVERSION
      FREE_MEMORY(&from_code);
      FREE_MEMORY(&to_code);
#else
      j_printerr("Warning: character set conversion disabled, option \"-nocharconv\" ignored\n");
#endif
      continue;
    } else if (strmatch(argv[i],"-charconv")) {	/* enable character set conversion */
#ifdef CHARACTER_CONVERSION
      FREE_MEMORY(&from_code);
      FREE_MEMORY(&to_code);
      tmparg = NEXTARG;
      from_code = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      tmparg = NEXTARG;
      to_code = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
#else
      j_printerr("Warning: character set conversion disabled, option \"-charconv\" ignored\n");
      i+=2;
#endif
      continue;
    } else if (strmatch(argv[i],"-kanji")) {
#ifdef CHARACTER_CONVERSION
      FREE_MEMORY(&from_code);
      FREE_MEMORY(&to_code);
      tmparg = NEXTARG;
      if (!strmatch(tmparg, "noconv")) {
	to_code = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      }
#else
      j_printerr("Warning: character set conversion disabled, option \"-kanji\" ignored\n");
      i++;
#endif
      continue;
    } else if (strmatch(argv[i],"-demo")) { /* quiet + progout */
      debug2_flag = verbose_flag = FALSE;
      progout_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-walign")) { /* do forced alignment by word */
      align_result_word_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-palign")) { /* do forced alignment by phoneme */
      align_result_phoneme_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-salign")) { /* do forced alignment by state */
      align_result_state_flag = TRUE;
      continue;
    } else if (strmatch(argv[i],"-output")) { /* output up to N candidate */
      output_hypo_maxnum = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-1pass")) { /* do only 1st pass */
      compute_only_1pass = TRUE;
      continue;
    } else if (strmatch(argv[i],"-hlist")) { /* HMM list file */
      FREE_MEMORY(&mapfilename);
      mapfilename = filepath(NEXTARG, cwd);
      continue;
#ifdef USE_NGRAM
    } else if (strmatch(argv[i],"-nlr")) { /* word LR 2-gram (ARPA) */
      FREE_MEMORY(&ngram_filename_lr_arpa);
      ngram_filename_lr_arpa = filepath(NEXTARG, cwd);
      FREE_MEMORY(&ngram_filename);
      continue;
    } else if (strmatch(argv[i],"-nrl")) { /* word RL 3-gram (ARPA) */
      FREE_MEMORY(&ngram_filename_rl_arpa);
      ngram_filename_rl_arpa = filepath(NEXTARG, cwd);
      FREE_MEMORY(&ngram_filename);
      continue;
    } else if (strmatch(argv[i],"-lmp")) { /* LM weight and penalty (pass1) */
      lm_weight = (LOGPROB)atof(NEXTARG);
      lm_penalty = (LOGPROB)atof(NEXTARG);
      lmp_specified = TRUE;
      continue;
    } else if (strmatch(argv[i],"-lmp2")) { /* LM weight and penalty (pass2) */
      lm_weight2 = (LOGPROB)atof(NEXTARG);
      lm_penalty2 = (LOGPROB)atof(NEXTARG);
      lmp2_specified = TRUE;
      continue;
    } else if (strmatch(argv[i],"-transp")) { /* penalty for transparent word */
      lm_penalty_trans = (LOGPROB)atof(NEXTARG);
      continue;
#else  /* USE_DFA */
    } else if (strmatch(argv[i],"-gram")) { /* comma-separatedlist of grammar prefix */
      multigram_add_prefix_list(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-gramlist")) { /* file of grammar prefix list */
      tmparg = filepath(NEXTARG, cwd);
      multigram_add_prefix_filelist(tmparg);
      free(tmparg);
      continue;
    } else if (strmatch(argv[i],"-nogram")) { /* remove grammar list */
      multigram_remove_gramlist();
      FREE_MEMORY(&dfa_filename);
      FREE_MEMORY(&dictfilename);
      continue;
    } else if (strmatch(argv[i],"-dfa")) { /* DFA filename */
      FREE_MEMORY(&dfa_filename);
      dfa_filename = filepath(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-penalty1")) {	/* word insertion penalty (pass1) */
      penalty1 = (LOGPROB)atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-penalty2")) {	/* word insertion penalty (pass2) */
      penalty2 = (LOGPROB)atof(NEXTARG);
      continue;
#endif
    } else if (strmatch(argv[i],"-spmodel") || strmatch(argv[i], "-sp")) { /* name of short pause word */
      FREE_MEMORY(&spmodel_name);
      tmparg = NEXTARG;
      spmodel_name = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
#ifdef MULTIPATH_VERSION
    } else if (strmatch(argv[i],"-iwsp")) { /* enable inter-word short pause handing */
      enable_iwsp = TRUE;
      continue;
    } else if (strmatch(argv[i],"-iwsppenalty")) { /* set inter-word short pause transition penalty */
      iwsp_penalty = atof(NEXTARG);
      continue;
#endif
#ifdef USE_NGRAM
    } else if (strmatch(argv[i],"-silhead")) { /* head silence word name */
      FREE_MEMORY(&head_silname);
      tmparg = NEXTARG;
      head_silname = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
    } else if (strmatch(argv[i],"-siltail")) { /* tail silence word name */
      FREE_MEMORY(&tail_silname);
      tmparg = NEXTARG;
      tail_silname = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
    } else if (strmatch(argv[i],"-iwspword")) { /* add short pause word */
      enable_iwspword = TRUE;
      continue;
    } else if (strmatch(argv[i],"-iwspentry")) { /* content of the iwspword */
      FREE_MEMORY(&iwspentry);
      tmparg = NEXTARG;
      iwspentry = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
    } else if (strmatch(argv[i],"-iwcache")) { /* control cross-word LM cache */
#ifdef HASH_CACHE_IW
      iw_cache_rate = atof(NEXTARG);
      if (iw_cache_rate > 100) iw_cache_rate = 100;
      if (iw_cache_rate < 1) iw_cache_rate = 1;
#else
      j_printerr("Warning: option \"-iwcache\" ignored\n");
#endif
      continue;
    } else if (strmatch(argv[i],"-sepnum")) { /* N-best frequent word will be separated from tree */
#ifdef SEPARATE_BY_UNIGRAM
      separate_wnum = atoi(NEXTARG);
#else
      j_printerr("Warning: option \"-sepnum\" ignored\n");
      i++;
#endif
      continue;
#endif /* USE_NGRAM */
#ifdef USE_NETAUDIO
    } else if (strmatch(argv[i],"-NA")) { /* netautio device name */
      FREE_MEMORY(&netaudio_devname);
      tmparg = NEXTARG;
      netaudio_devname = strcpy((char*)mymalloc(strlen(tmparg)+1),tmparg);
      continue;
#endif
    } else if (strmatch(argv[i],"-adport")) { /* adinnet port num */
      adinnet_port = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-nostrip")) { /* do not strip zero samples */
      strip_zero_sample = FALSE;
      continue;
    } else if (strmatch(argv[i],"-zmean")) { /* enable DC offset by zero mean */
      use_zmean = TRUE;
      continue;
    } else if (strmatch(argv[i],"-nozmean")) { /* disable DC offset by zero mean */
      use_zmean = FALSE;
      continue;
    } else if (strmatch(argv[i],"-zmeanframe")) { /* enable frame-wise DC offset by zero mean */
      para.zmeanframe = TRUE;
      continue;
    } else if (strmatch(argv[i],"-nozmeanframe")) { /* disable frame-wise DC offset by zero mean */
      para.zmeanframe = FALSE;
      continue;
#ifdef SP_BREAK_CURRENT_FRAME
    } else if (strmatch(argv[i],"-spdur")) { /* short-pause duration threshold */
      sp_frame_duration = atoi(NEXTARG);
      continue;
#endif
    } else if (strmatch(argv[i],"-gprune")) { /* select Gaussian pruning method */
      tmparg = NEXTARG;
      if (strmatch(tmparg,"safe")) { /* safest, slowest */
	gprune_method = GPRUNE_SEL_SAFE;
      } else if (strmatch(tmparg,"heuristic")) {
	gprune_method = GPRUNE_SEL_HEURISTIC;
      } else if (strmatch(tmparg,"beam")) { /* fastest */
	gprune_method = GPRUNE_SEL_BEAM;
      } else if (strmatch(tmparg,"none")) { /* no prune: compute all Gaussian */
	gprune_method = GPRUNE_SEL_NONE;
      } else if (strmatch(tmparg,"default")) {
	gprune_method = GPRUNE_SEL_UNDEF;
      } else {
	j_printerr("%s: no such pruning method \"%s\"\n", argv[0], tmparg);
	opt_terminate();
      }
      continue;
/* 
 *     } else if (strmatch(argv[i],"-reorder")) {
 *	 result_reorder_flag = TRUE;
 *	 continue;
 */
    } else if (strmatch(argv[i],"-no_ccd")) { /* force triphone handling = OFF */
      ccd_flag = FALSE;
      ccd_flag_force = TRUE;
      continue;
    } else if (strmatch(argv[i],"-force_ccd")) { /* force triphone handling = ON */
      ccd_flag = TRUE;
      ccd_flag_force = TRUE;
      continue;
    } else if (strmatch(argv[i],"-iwcd1")) { /* select cross-word triphone computation method */
      tmparg = NEXTARG;
      if (strmatch(tmparg, "max")) { /* use maximum score in triphone variants */
	iwcdmethod = IWCD_MAX;
      } else if (strmatch(tmparg, "avg")) { /* use average in variants */
	iwcdmethod = IWCD_AVG;
      } else if (strmatch(tmparg, "best")) { /* use average in variants */
	iwcdmethod = IWCD_NBEST;
	tmparg = NEXTARG;
	iwcdmaxn = atoi(tmparg);
      } else {
	j_printerr("%s: -iwcd1: wrong argument (max|avg|best N): %s\n", argv[0], tmparg);
	opt_terminate();
      }
      continue;
    } else if (strmatch(argv[i],"-tmix")) { /* num of mixture to select */
      if (i + 1 < argc && isdigit(argv[i+1][0])) {
	mixnum_thres = atoi(argv[++i]);
      }
      continue;
    } else if (strmatch(argv[i],"-b2") || strmatch(argv[i],"-bw") || strmatch(argv[i],"-wb")) {	/* word beam width in 2nd pass */
      enveloped_bestfirst_width = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-hgs")) { /* Gaussian selection model file */
      FREE_MEMORY(&hmm_gs_filename);
      hmm_gs_filename = filepath(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-booknum")) { /* num of state to select in GS */
      gs_statenum = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-gshmm")) { /* same as "-hgs" */
      FREE_MEMORY(&hmm_gs_filename);
      hmm_gs_filename = filepath(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-gsnum")) { /* same as "-booknum" */
      gs_statenum = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-cmnload")) { /* load CMN parameter from file */
      FREE_MEMORY(&cmnload_filename);
      cmnload_filename = filepath(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-cmnsave")) { /* save CMN parameter to file */
      FREE_MEMORY(&cmnsave_filename);
      cmnsave_filename = filepath(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-cmnupdate")) { /* update CMN parameter */
      cmn_update = TRUE;
      continue;
    } else if (strmatch(argv[i],"-cmnnoupdate")) { /* not update CMN parameter */
      cmn_update = FALSE;
      continue;
    } else if (strmatch(argv[i],"-cmnmapweight")) { /* CMN weight for MAP */
      cmn_map_weight = (float)atof(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-sscalc")) { /* do spectral subtraction (SS) for raw file input */
      sscalc = TRUE;
      FREE_MEMORY(&ssload_filename);
      continue;
    } else if (strmatch(argv[i],"-sscalclen")) { /* head silence length used to compute SS (in msec) */
      sscalc_len = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-ssload")) { /* load SS parameter from file */
      FREE_MEMORY(&ssload_filename);
      ssload_filename = filepath(NEXTARG, cwd);
      sscalc = FALSE;
      continue;
#ifdef CONFIDENCE_MEASURE
    } else if (strmatch(argv[i],"-cmalpha")) { /* CM log score scaling factor */
#ifdef CM_MULTIPLE_ALPHA
      cm_alpha_bgn = (LOGPROB)atof(NEXTARG);
      cm_alpha_end = (LOGPROB)atof(NEXTARG);
      cm_alpha_step = (LOGPROB)atof(NEXTARG);
      cm_alpha_num = (int)((cm_alpha_end - cm_alpha_bgn) / cm_alpha_step) + 1;
      if (cm_alpha_num > 100) j_error("too small step or wide range! outputnum > 100\n");
#else
      cm_alpha = (LOGPROB)atof(NEXTARG);
#endif
      continue;
#ifdef CM_SEARCH_LIMIT
    } else if (strmatch(argv[i],"-cmthres")) { /* CM cut threshold for CM decoding */
      cm_cut_thres = (LOGPROB)atof(NEXTARG);
      continue;
#endif
#ifdef CM_SEARCH_LIMIT_POP
    } else if (strmatch(argv[i],"-cmthres2")) { /* CM cut threshold for CM decoding */
      cm_cut_thres_pop = (LOGPROB)atof(NEXTARG);
      continue;
#endif
#endif /* CONFIDENCE_MEASURE */
    } else if (strmatch(argv[i],"-gmm")) { /* load SS parameter from file */
      FREE_MEMORY(&gmm_filename);
      gmm_filename = filepath(NEXTARG, cwd);
      continue;
    } else if (strmatch(argv[i],"-gmmnum")) { /* num of Gaussian pruning for GMM */
      gmm_gprune_num = atoi(NEXTARG);
      continue;
    } else if (strmatch(argv[i],"-gmmreject")) {
      tmparg = NEXTARG;
      gmm_reject_cmn_string = strcpy((char *)mymalloc(strlen(tmparg)+1), tmparg);
      continue;
    } else if (strmatch(argv[i],"-htkconf")) {
      tmparg = NEXTARG;
      if (htk_config_file_parse(tmparg, &para_htk) == FALSE) {
	j_error("Error: failed to read %s\n", tmparg);
      }
      continue;
    } else if (strmatch(argv[i],"-help")) { /* output version and option */
      detailed_help();
    } else if (strmatch(argv[i],"--help")) {
      detailed_help();
    }
    if (argv[i][0] == '-' && strlen(argv[i]) == 2) {
      /* 1-letter options */
      switch(argv[i][1]) {
      case 'h':			/* hmmdefs */
	FREE_MEMORY(&hmmfilename);
	hmmfilename = filepath(NEXTARG, cwd);
	break;
      case 'v':			/* dictionary */
	FREE_MEMORY(&dictfilename);
	dictfilename = filepath(NEXTARG, cwd);
	break;
#ifdef USE_NGRAM
      case 'd':			/* binary N-gram */
	FREE_MEMORY(&ngram_filename);
	FREE_MEMORY(&ngram_filename_lr_arpa);
	FREE_MEMORY(&ngram_filename_rl_arpa);
	ngram_filename = filepath(NEXTARG, cwd);
	break;
#endif
      case 'b':			/* beam width in 1st pass */
	specified_trellis_beam_width = atoi(NEXTARG);
	break;
      case 's':			/* stack size in 2nd pass */
	stack_size = atoi(NEXTARG);
	break;
      case 'n':			/* N-best search */
	nbest = atoi(NEXTARG);
	break;
      case 'm':			/* upper limit of hypothesis generation */
	hypo_overflow = atoi(NEXTARG);
	break;
      default:
	j_printerr("%s: wrong argument: %s\n", argv[0], argv[i]);
	opt_terminate();
      }
    } else {			/* error */
      j_printerr("%s: wrong argument: %s\n", argv[0], argv[i]);
      opt_terminate();
    }
  }

  /* set default values if not specified yet */
  if (!spmodel_name) {
    spmodel_name = strcpy((char*)mymalloc(strlen(SPMODEL_NAME_DEFAULT)+1),
			  SPMODEL_NAME_DEFAULT);
  }
#ifdef USE_NGRAM
  if (!head_silname) {
    head_silname = strcpy((char*)mymalloc(strlen(BEGIN_WORD_DEFAULT)+1),
			  BEGIN_WORD_DEFAULT);
  }
  if (!tail_silname) {
    tail_silname = strcpy((char*)mymalloc(strlen(END_WORD_DEFAULT)+1),
			  END_WORD_DEFAULT);
  }
  if (!iwspentry) {
    iwspentry = strcpy((char*)mymalloc(strlen(IWSPENTRY_DEFAULT)+1),
		       IWSPENTRY_DEFAULT);
  }
#endif	/* USE_NGRAM */
#ifdef USE_NETAUDIO
  if (!netaudio_devname) {
    netaudio_devname = strcpy((char*)mymalloc(strlen(NETAUDIO_DEVNAME)+1),
			      NETAUDIO_DEVNAME);
  }
#endif	/* USE_NETAUDIO */
}

/** 
 * <JA>
 * オプション関連のグローバル変数のメモリ領域を解放する．
 * </JA>
 * <EN>
 * Free memories of global variables allocated by option arguments.
 * </EN>
 */
void
opt_release()
{
  FREE_MEMORY(&inputlist_filename);
  FREE_MEMORY(&record_dirname);
#ifdef CHARACTER_CONVERSION
  FREE_MEMORY(&from_code);
  FREE_MEMORY(&to_code);
#endif	/* CHARACTER_CONVERSION */
  FREE_MEMORY(&mapfilename);
#ifdef USE_NGRAM
  FREE_MEMORY(&ngram_filename);
  FREE_MEMORY(&ngram_filename_lr_arpa);
  FREE_MEMORY(&ngram_filename_rl_arpa);
#endif	/* USE_NGRAM */
#ifdef USE_DFA
  FREE_MEMORY(&dfa_filename);
#endif	/* USE_DFA */
  FREE_MEMORY(&spmodel_name);
#ifdef USE_NGRAM
  FREE_MEMORY(&head_silname);
  FREE_MEMORY(&tail_silname);
  FREE_MEMORY(&iwspentry);
#endif	/* USE_NGRAM */
#ifdef USE_NETAUDIO
  FREE_MEMORY(&netaudio_devname);
#endif	/* USE_NETAUDIO */
  FREE_MEMORY(&hmm_gs_filename);
  FREE_MEMORY(&cmnload_filename);
  FREE_MEMORY(&cmnsave_filename);
  FREE_MEMORY(&ssload_filename);
  FREE_MEMORY(&gmm_filename);
  FREE_MEMORY(&gmm_reject_cmn_string);
  FREE_MEMORY(&hmmfilename);
  FREE_MEMORY(&dictfilename);
}
