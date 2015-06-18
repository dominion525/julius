/**
 * @file   m_usage.c
 * @author Akinobu Lee
 * @date   Fri May 13 15:04:34 2005
 * 
 * <JA>
 * @brief  ヘルプを表示する
 * </JA>
 * 
 * <EN>
 * @brief  Print help.
 * </EN>
 * 
 * $Revision: 1.14 $
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
 * メッセージを出力してエラー終了（オプション指定エラー時用）
 * 
 * </JA>
 * <EN>
 * Output error message and error exit (for option parse error)
 * 
 * </EN>
 */
void
opt_terminate()
{
  j_error("Try `-help' for more information.\n");
}

/** 
 * <JA>
 * バージョンとメッセージを出力して終了（オプション無し実行時）
 * 
 * </JA>
 * <EN>
 * Output version messages and exit (for exec with no option)
 * 
 * </EN>
 */
void
usage()
{
  put_version(stderr);
  j_printerr("Try '-setting' for built-in engine configuration.\n");
  j_printerr("Try '-help' for run time options.\n");
  j_exit();
}

/** 
 * <JA>
 * バージョン情報，エンジン設定および詳細なオプションのヘルプを表示して終了
 * 
 * </JA>
 * <EN>
 * Output version info, engine setting and detailed help for options, and exit.
 * 
 * </EN>
 */
void
detailed_help()
{
  put_header(stdout);
  put_compile_defs(stdout);
  j_printf("\nOptions:\n");

  j_printf("\n Speech Input:\n");
  j_printf("    (Only MFCC acoustic model can recognize waveform directly)\n");
  j_printf("    [-input devname]    input from:  (default = htkparam)\n");
  j_printf("         htkparam/mfcfile  HTK parameter file\n");
  j_printf("         file/rawfile      waveform file (16bit,mono,linear)\n");
  j_printf("                           supported file types:\n");
  j_printf("                            %s\n", SUPPORTED_WAVEFILE_FORMAT);
#ifdef USE_MIC
  j_printf("         mic           microphone device\n");
#endif
#ifdef USE_NETAUDIO
  j_printf("         netaudio      DatLink/NetAudio server\n");
#endif
  j_printf("         adinnet     adinnet client (TCP/IP)\n");
  j_printf("         stdin       standard input\n");
  j_printf("    [-filelist file]    input file list\n");
#ifdef USE_NETAUDIO
  j_printf("    [-NA host:unit]     netaudio server address\n");
#endif
  j_printf("    [-adport portnum]   adinnet portnum to listen             (%d)\n", adinnet_port);
  j_printf("    [-smpFreq freq]     sample period (Hz)                    (%d)\n", para_default.smp_freq);
  j_printf("    [-smpPeriod period] sample period (100ns unit)            (%d)\n", para_default.smp_period);
  j_printf("    [-48]               48kHz input with down sampling        (OFF)\n");
  j_printf("    [-zmean/-nozmean]   enable/disable DC offset removal      (OFF)\n");
  j_printf("    [-zmeanframe/-nozmeanframe] frame-wise DC offset removal  (OFF)\n");
  j_printf("    [-nostrip]          not strip off zero samples\n");
  j_printf("    [-record dir]       save recognized speech data to dir\n");
  j_printf("    [-rejectshort msec] reject short input\n");
  
  j_printf("\n Speech Detection: (default: on=mic/net off=files)\n");
  /*j_printf("    [-pausesegment]     turn on (force) pause detection\n");*/
  /*j_printf("    [-nopausesegment]   turn off (force) pause detection\n");*/
  j_printf("    [-cutsilence]       turn on (force) long silence cutting\n");
  j_printf("    [-nocutsilence]     turn off (force) long silence cutting\n");
  j_printf("    [-lv unsignedshort] level threshold (0-32767)             (%d)\n", level_thres);
  j_printf("    [-zc zerocrossnum]  zerocross num threshold (per sec.)    (%d)\n", zero_cross_num);
  j_printf("    [-headmargin msec]  header margin length in msec.         (%d)\n", head_margin_msec);
  j_printf("    [-tailmargin msec]  tail margin length in msec.           (%d)\n", tail_margin_msec);

  j_printf("\n Acoustic analysis:\n");
  j_printf("    [-htkconf file]     read parameters from HTK Config file\n");
  j_printf("                        (specifying options below will override it)\n");
  j_printf("    [-smpFreq freq]     sample period (Hz)                    (%d)\n", para_default.smp_freq);
  j_printf("    [-smpPeriod period] sample period (100ns)                    (%d)\n", para_default.smp_period);
  j_printf("    [-fsize sample]     window size (sample)                  (%d)\n", para_default.framesize);
  j_printf("    [-fshift sample]    frame shift (sample)                  (%d)\n", para_default.frameshift);
  j_printf("    [-preemph]          pre-emphasis coef.                    (%.2f)\n", para_default.preEmph);
  j_printf("    [-fbank]            number of filterbank channels         (%d)\n", para_default.fbank_num);
  j_printf("    [-ceplif]           cepstral liftering coef.              (%d)\n", para_default.lifter);
  j_printf("    [-rawe] [-norawe]    toggle using raw energy              (not use)\n");
  j_printf("    [-enormal] [-noenormal]  toggle normalizing log energy    (not normalize)\n");
  j_printf("                        (cannot set -enormal when live input)\n");
  j_printf("    [-escale]           scaling log energy for enormal        (%.1f)\n", para_default.escale);
  j_printf("    [-silfloor]         energy silence floor in dB            (%.1f)\n", para_default.silFloor);
  j_printf("    [-delwin frame]     delta windows length (frame)          (%d)\n", para_default.delWin);
  j_printf("    [-accwin frame]     accel windows length (frame)          (%d)\n", para_default.accWin);
  j_printf("    [-hifreq freq]      freq. of upper band limit, off if <0  (%d)\n", para_default.hipass);
  j_printf("    [-lofreq freq]      freq. of lower band limit, off if <0  (%d)\n", para_default.lopass);
  j_printf("    [-sscalc]           do spectral subtraction (file input only)\n");
  j_printf("    [-sscalclen msec]   length of head silence for SS (msec)  (%d)\n", sscalc_len);
  j_printf("    [-ssload filename]  load constant noise spectrum from file for SS\n");
  j_printf("    [-ssalpha value]    alpha coef. for SS                    (%f)\n", para_default.ss_alpha);
  j_printf("    [-ssfloor value]    spectral floor for SS                 (%f)\n", para_default.ss_floor);

  j_printf("\n GMM utterance verification:\n");
  j_printf("    -gmm filename       GMM definition file\n");
  j_printf("    -gmmnum num         GMM Gaussian pruning num              (%d)\n", gmm_gprune_num);
  j_printf("    -gmmreject string   list of GMMs for rejection (cancel pass2, keep CM)\n");

#ifdef USE_NGRAM
  j_printf("\n N-gram Language model (\"-d\" or \"-nlr -nrl\" needed):\n");
  j_printf("    -d file.bingram     n-gram file name in Julius binary format\n");
  j_printf("    -nlr file.arpa      2-gram file name in ARPA format\n");
  j_printf("    -nrl file.arpa      reverse 3-gram file name in ARPA format\n");
  j_printf("    [-lmp float float]  weight and penalty (tri: %.1f %.1f mono: %.1f %1.f)\n", DEFAULT_LM_WEIGHT_TRI_PASS1, DEFAULT_LM_PENALTY_TRI_PASS1, DEFAULT_LM_WEIGHT_MONO_PASS1, DEFAULT_LM_PENALTY_MONO_PASS1);
  j_printf("    [-lmp2 float float]       for 2nd pass (tri: %.1f %.1f mono: %.1f %1.f)\n", DEFAULT_LM_WEIGHT_TRI_PASS2, DEFAULT_LM_PENALTY_TRI_PASS2, DEFAULT_LM_WEIGHT_MONO_PASS2, DEFAULT_LM_PENALTY_MONO_PASS2);
  j_printf("    [-transp float]     penalty for transparent word (%+2.1f)\n", lm_penalty_trans);
#else  /* USE_DFA */
  j_printf("\n Grammar:\n");
  j_printf("    -dfa file.dfa       DFA grammar file\n");
  j_printf("    -gram file[,file2...] (list of) grammar prefix\n");
  j_printf("    -gramlist filename  filename of grammar list\n");
  j_printf("    [-penalty1 float]   word insertion penalty (1st pass)     (%.1f)\n", penalty1);
  j_printf("    [-penalty2 float]   word insertion penalty (2nd pass)     (%.1f)\n", penalty1);
#endif

  j_printf("\n Word Dictionary:\n");
  j_printf("    -v dictfile         dictionary file name\n");
#ifdef USE_NGRAM
  j_printf("    [-silhead wordname] specify beginning-of-sentence word    (%s)\n", BEGIN_WORD_DEFAULT);
  j_printf("    [-siltail wordname] specify end-of-sentence word          (%s)\n", END_WORD_DEFAULT);
#endif
  j_printf("    [-forcedict]        not terminate on error words, just ignore\n");
#ifdef USE_NGRAM
  j_printf("    [-iwspword]         add an sp-word to the vocabulary for inter-word CD sp\n");
  j_printf("    [-iwspentry dictentry] specify content of the iwspword    (%s)\n", IWSPENTRY_DEFAULT);
#endif
  
  j_printf("\n Acoustic Model:\n");
  j_printf("    -h hmmdefsfile      HMM definition file name\n");
  j_printf("    [-hlist HMMlistfile] HMMlist filename (must for triphone model)\n");
  j_printf("    [-iwcd1 methodname] switch IWCD triphone handling on 1st pass\n");
#ifdef USE_NGRAM
  j_printf("             best N     use N best score (default, N=%d)\n", iwcdmaxn);
  j_printf("             max        use maximum score\n");
  j_printf("             avg        use average score\n");
#else
  j_printf("             best N     use N best score (N=%d by default)\n", iwcdmaxn);
  j_printf("             max        use maximum score\n");
  j_printf("             avg        use average score (default)\n");
#endif
  j_printf("    [-force_ccd]        force to handle IWCD\n");
  j_printf("    [-no_ccd]           don't handle IWCD\n");
  j_printf("    [-notypecheck]      don't check input parameter type\n");
  j_printf("    [-spmodel HMMname]  name of short pause model             (\"%s\")\n", SPMODEL_NAME_DEFAULT);

  j_printf("\n Acoustic Computation Method:\n");
  j_printf("    [-gprune methodname] select Gaussian pruning method:\n");
#ifdef GPRUNE_DEFAULT_SAFE
  j_printf("             safe          safe pruning (default for TM/PTM)\n");
#else
  j_printf("             safe          safe pruning\n");
#endif
#if GPRUNE_DEFAULT_HEURISTIC
  j_printf("             heuristic     heuristic pruning (default for TM/PTM)\n");
#else
  j_printf("             heuristic     heuristic pruning\n");
#endif
#if GPRUNE_DEFAULT_BEAM
  j_printf("             beam          beam pruning (default for TM/PTM)\n");
#else
  j_printf("             beam          beam pruning\n");
#endif
  j_printf("             none          no pruning (default for non tmix models)\n");
  j_printf("    [-tmix gaussnum]    Gaussian num threshold per mixture for pruning (%d)\n", mixnum_thres);
  j_printf("    [-gshmm hmmdefs]    monophone hmmdefs for GS\n");
  j_printf("    [-gsnum N]          N-best state will be selected        (%d)\n", gs_statenum);

#ifdef SP_BREAK_CURRENT_FRAME
  j_printf("\n Short-pause Segmentation:\n");
  j_printf("    [-spdur]            length threshold of sp frames         (%d)\n", sp_frame_duration);
#endif

  j_printf("\n Search Parameters (First Pass):\n");
  j_printf("    [-b beamwidth]      beam width (by state num)             (guessed)\n");
  j_printf("                        (0: full search, -1: force guess)\n");
#ifdef WPAIR
# ifdef WPAIR_KEEP_NLIMIT
  j_printf("    [-nlimit N]         keeps only N tokens on each state     (%d)\n", wpair_keep_nlimit);
# endif
#endif
#ifdef USE_NGRAM
#ifdef SEPARATE_BY_UNIGRAM
  j_printf("    [-sepnum wordnum]   # of hi-freq word isolated from tree  (%d)\n", separate_wnum);
#endif
#ifdef HASH_CACHE_IW
  j_printf("    [-iwcache percent]  amount of inter-word LM cache         (%3d)\n", iw_cache_rate);
#endif
#endif /* USE_NGRAM */
  j_printf("    [-1pass]            do 1st pass only, omit 2nd pass\n");

  j_printf("\n Search Parameters (Second Pass):\n");
  j_printf("    [-b2 hyponum]       word envelope beam width (by hypo num) (%d)\n",enveloped_bestfirst_width);
  j_printf("    [-n N]              # of sentence to find                 (%d)\n", nbest);
  j_printf("    [-output N]         # of sentence to output               (%d)\n",output_hypo_maxnum);
#ifdef SCAN_BEAM
  j_printf("    [-sb score]         score beam threshold (by score)       (%.1f)\n", scan_beam_thres);
#endif
  j_printf("    [-s hyponum]        global stack size of hypotheses       (%d)\n", stack_size);
  j_printf("    [-m hyponum]        hypotheses overflow threshold num     (%d)\n", hypo_overflow);

  j_printf("    [-lookuprange N]    frame lookup range in word expansion  (%d)\n", lookup_range);
#ifdef USE_DFA
  j_printf("    [-looktrellis]      expand only backtrellis words\n");
  j_printf("    [-[no]multigramout] output per-grammar results\n");
#endif
#ifdef CATEGORY_TREE
  j_printf("    [-oldtree]          use old build_wchmm()\n");
#ifdef PASS1_IWCD
  j_printf("    [-oldiwcd]          use full lcdset\n");
#endif
#endif
#ifdef MULTIPATH_VERSION
  j_printf("    [-iwsp]             turn on inter-word short pause handling (off)\n");
  j_printf("    [-iwsppenalty]      trans. penalty for inter-word sp      (%.1f)\n", iwsp_penalty);
#endif 

#ifdef GRAPHOUT
  j_printf("\n Graph Output:\n");
  j_printf("    [-graphrange N]     merge same words in graph (%d)\n", graph_merge_neighbor_range);
  j_printf("                        -1: not merge, leave same loc. with diff. score\n");
  j_printf("                         0: merge same words at same location\n");
  j_printf("                        >0: merge same words around the margin\n");
#ifdef GRAPHOUT_DEPTHCUT
  j_printf("    [-graphcut num]     graph cut depth at postprocess (-1: disable)(%d)\n", graphout_cut_depth);
#endif
#ifdef GRAPHOUT_LIMIT_BOUNDARY_LOOP
  j_printf("    [-graphboundloop num] max. num of boundary adjustment loop (%d)\n", graphout_limit_boundary_loop_num);
#endif
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
  j_printf("    [-graphsearchdelay] inhibit search termination until 1st sent. found\n");
  j_printf("    [-nographsearchdelay] disable it (default)\n");
#endif
#endif /* GRAPHOUT */

  j_printf("\n On-the-fly Decoding: (default: on=mic/net off=files)\n");
  j_printf("    [-realtime]         turn on, input streamed with MAP-CMN\n");
  j_printf("    [-norealtime]       turn off, input buffered with sentence CMN\n");
  j_printf("    [-cmnload file]     load initial CMN param from file on startup\n");
  j_printf("    [-cmnsave file]     save CMN param to file after each input\n");
  j_printf("    [-cmnnoupdate]      not update CMN param while recog. (use with -cmnload)\n");
  j_printf("    [-cmnmapweight]     weight value of initial cm for MAP-CMN (%6.2f)\n", cmn_map_weight);

  j_printf("\n Forced Alignment:\n");
  j_printf("    [-walign]           optionally output word alignments\n");
  j_printf("    [-palign]           optionally output phoneme alignments\n");
  j_printf("    [-salign]           optionally output state alignments\n");
#ifdef CONFIDENCE_MEASURE
  j_printf("\n Confidence Score:\n");
#ifdef CM_MULTIPLE_ALPHA
  j_printf("    [-cmalpha f t s]    CM smoothing factor        (from, to, step)\n");
#else
  j_printf("    [-cmalpha value]    CM smoothing factor                    (%f)\n", cm_alpha);
#endif
#ifdef CM_SEARCH_LIMIT
  j_printf("    [-cmthres value]    CM threshold to cut hypo on 2nd pass   (%f)\n", cm_cut_thres);
#endif
#endif /* CONFIDENCE_MEASURE */
  j_printf("\n Output:\n");
  j_printf("    [-result devname]   output recognition result to:         (tty)\n");
  j_printf("             tty        plain text to standard out\n");
  j_printf("             msock      module socket\n");
  j_printf("\n Server Module Mode:\n");
  j_printf("    [-module [port]]    enable server module mode             (%d)\n", module_port);
  j_printf("                        (implies \"-result msock\")\n");
  j_printf("    [-fork] [-nofork]   fork / not fork on module connection  (%s)\n", fork_on_module_connection ? "fork" : "nofork");
#ifdef CONFIDENCE_MEASURE
  j_printf("    [-outcode WLPSCwlps] select info to output to the module\n");
  j_printf("                        (Word,Lang,Phone,Score,Confidence low=1st pass)\n");
#else
  j_printf("    [-outcode WLPSwlps] select info to output to the module\n");
  j_printf("                        (Word,Lang,Phone,Score, lower == 1st pass)\n");
#endif
  j_printf("\n Message Output:\n");
#ifdef USE_NGRAM
  j_printf("    [-separatescore]    output LM and AM score independently\n");
#endif
  j_printf("    [-quiet]            reduce output to only word string\n");
  j_printf("    [-progout]          progressive output in 1st pass\n");
  j_printf("    [-proginterval]     interval of progout in msec           (%d)\n", progout_interval);
  j_printf("    [-demo]             equal to \"-quiet -progout\"\n");
#ifdef CHARACTER_CONVERSION
  j_printf("    [-charconv from to] convert output character set from [from] to [to]\n");
#ifdef USE_LIBJCODE
  j_printf("                        (support only Japanese sets: euc,sjis,jis)\n");
#endif
  j_printf("    [-nocharconv]       disable output code conversion (default)\n");
#endif
  j_printf("\n Others:\n");
  j_printf("    [-C jconffile]      load options from jconf file\n");
  j_printf("    [-debug]            (for debug) dump numerous log\n");
  j_printf("    [-check (wchmm|trellis)] (for debug) check internal structure\n");
  j_printf("    [-check triphone]   triphone mapping check\n");
  j_printf("    [-setting]          print engine configuration and exit\n");
  j_printf("    [-help]             print this message and exit\n");
  j_exit();
}
