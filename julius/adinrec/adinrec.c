/**
 * @file   adinrec.c
 * @author Akinobu LEE
 * @date   Wed Mar 23 20:33:01 2005
 * 
 * <JA>
 * @brief  マイクから一発話をファイルへ記録する
 * </JA>
 * 
 * <EN>
 * @brief  Record a speech segment from microphone to a file
 * </EN>
 * 
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2001-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* libsent headers */
#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>
#include <sent/tcpip.h>
#include <sys/stat.h>
#include <signal.h>


static int speechlen;		///< Total length of recorded sample
static int fd = -1;		///< File descriptor for output
static FILE *fp = NULL;		///< File pointer for WAV output
static int  size;		///< Output file size

static char *filename = NULL;	///< Output file name
static boolean stout = FALSE;	///< True if output to stdout
static int sfreq = 16000;	///< Sampling frequency in Hz
static int level_thres = 2000;	///< Level thresholds
static int zero_cross_num = 60; ///< Zerocross num threshold
static int margin = 400;	///< Head/tail margin in msec
static boolean strip_zero_sample = TRUE; ///< Strip zero sample if TRUE
static boolean use_raw = FALSE;	///< Output in RAW format if TRUE
static boolean use_zmean = FALSE; ///< Enable zero mean source if TRUE
static boolean use_48to16 = FALSE; ///< TRUE if use 48kHz sampling with down sampling to 16kHz

/** 
 * <JA>ヘルプを表示して終了する</JA>
 * <EN>Print help and exit</EN>
 */
void
usage()
{
  fprintf(stderr, "adinrec --- record one sentence input to a file\n");
  fprintf(stderr, "Usage: adinrec [options..] filename\n");
  fprintf(stderr, "    [-freq frequency]     sampling frequency in Hz        (%d)\n", sfreq);
  fprintf(stderr, "    [-48]                 48000Hz recording with down sampling (16kHz only)\n");
  fprintf(stderr, "    [-lv unsignedshort]   silence cut level threshold (%d)\n", level_thres);
  fprintf(stderr, "    [-zc zerocrossnum]    silence cut zerocross num   (%d)\n", zero_cross_num);
  fprintf(stderr, "    [-margin msec]        head/tail margin length         (%d)\n", margin);
  fprintf(stderr, "    [-nostrip]            not strip off zero samples\n");
  fprintf(stderr, "    [-zmean]              remove DC by zero mean\n");
  fprintf(stderr, "    [-raw]                output in RAW format\n");
  fprintf(stderr, "\nLibrary configuration: ");
  confout_version(stderr);
  confout_audio(stderr);
  confout_process(stderr);
  fprintf(stderr, "\n");
  exit(1);
}

/**
 * <EN>
 * Read option argument and set values
 *
 * @param argc [in] number of argument from command line
 * @param argv [in] array of arguments
 * </EN>
 * <JA>
 * オプション引数を読んで値をセットする
 *
 * @param argc [in] コマンドラインからの引数の数
 * @param argv [in] 引数の配列
 * </JA>
 */
void
opt_parse(int argc, char *argv[])
{
  int i;
  /* option parsing */
  if (argc <= 1) usage();
  for (i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i], "-freq")) {
	i++;
	if (i >= argc) usage();
	sfreq = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-lv")) {
	i++;
	if (i >= argc) usage();
	level_thres = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-zc")) {
	i++;
	if (i >= argc) usage();
	zero_cross_num = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-margin")) {
	i++;
	if (i >= argc) usage();
	margin = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-nostrip")) {
	strip_zero_sample = FALSE;
      } else if (!strcmp(argv[i], "-h")) {
	usage();
      } else if (!strcmp(argv[i], "-help")) {
	usage();
      } else if (!strcmp(argv[i], "--help")) {
	usage();
      } else if (!strcmp(argv[i], "-raw")) {
	use_raw = TRUE;
      } else if (!strcmp(argv[i], "-zmean")) {
	use_zmean = TRUE;
      } else if (!strcmp(argv[i], "-48")) {
	use_48to16 = TRUE;
      } else if (!strcmp(argv[i], "-")) {
	stout = TRUE;
      } else {
	fprintf(stderr,"Unknown option: \"%s\"\n\n", argv[i]);
	usage();
      }
    } else {
      filename = argv[i];
    }
  }
  if (stout) use_raw = TRUE;	/* force RAW output in stdout */

}


/** 
 * <JA>
 * 録音されたサンプル列を処理するコールバック関数
 * 
 * @param now [in] 録音されたサンプル列
 * @param len [in] 長さ（サンプル数）
 * 
 * @return エラー時 -1，処理成功時 0，処理成功＋区間終端検出時 1 を返す．
 * </JA>
 * <EN>
 * Callback handler of recorded sample fragments
 * 
 * @param now [in] recorded fragments of speech sample
 * @param len [in] length of above in samples
 * 
 * @return -1 on device error (require caller to exit and terminate input),
 * 0 on success (allow caller to continue),
 * 1 on succeeded but segmentation detected (require caller to exit but
 * input will continue in the next call.
 * </EN>
 */
static int
adin_callback_file(SP16 *now, int len)
{
  int count;
  /* it may be safe to limit the maximum record len for disk space */
  /*if (speechlen + len > MAXSPEECHLEN) {
    fprintf(stderr, "Error: too long input (> %d samples)\n", MAXSPEECHLEN);
    return(FALSE);
    }*/

  /* erase "<<<please speak>>>" text on tty at trigger up */
  /* (moved from adin-cut.c) */
  if (speechlen == 0) {
    j_printerr("\r                    \r");
  }

  /* open output file for the first time */
  if (use_raw) {
    if (fd == -1) {
      if (stout) {
	fd = 1;
      } else {
	if ((fd = creat(filename, 0644)) == -1) {
	  perror("adinrec");
	  return -1;
	}
      }
    }
  } else {
    if (fp == NULL) {
      if ((fp = wrwav_open(filename, sfreq)) == NULL) {
	perror("adinrec");
	return -1;
      }
    }
  }
  /* write recorded sample to file */
  if (use_raw) {
    count = wrsamp(fd, now, len);
    if (count < 0) perror("adinrec: cannot write");
    if (count < len * sizeof(SP16)) fprintf(stderr, "adinrec: cannot write more %d bytes\ncurrent length = %d\n", count, speechlen * sizeof(SP16));
  } else {
    if (wrwav_data(fp, now, len) == FALSE) {
      fprintf(stderr, "adinrec: cannot write\n");
    }
  }
  
  speechlen += len;
  
  /* progress bar in dots */
  fprintf(stderr, ".");		
  return(0);
}

/* close file */
static void
close_file()
{
  size = sizeof(SP16) * speechlen;
  if (use_raw) {
    if (fd >= 0) {
      if (close(fd) != 0) {
	perror("adinrec");
      }
    }
  } else {
    if (fp != NULL) {
      if (wrwav_close(fp) == FALSE) {
	j_printerr("adinrec: failed to close file\n");
      }
    }
  }
  fprintf(stderr, "\n%d samples (%d bytes, %.2f sec.) recorded\n", speechlen, size, (float)speechlen / (float)sfreq);
}  

/* Interrupt signal handling */
static void
interrupt_record(int signum)
{
  fprintf(stderr, "[Interrupt]");
  /* close files */
  close_file();
  /* terminate program */
  exit(1);
}


/** 
 * <JA>
 * メイン関数
 * 
 * @param argc [in] 引数列の長さ
 * @param argv [in] 引数列
 * 
 * @return 
 * </JA>エラー時 1，通常終了時 0 を返す．
 * <EN>
 * Main function.
 * 
 * @param argc [in] number of argument.
 * @param argv [in] array of arguments.
 * 
 * @return 1 on error, 0 on success.
 * </EN>
 */
int
main(int argc, char *argv[])
{
  int silence_cut = 1;	/* 0=disabled, 1=enabled, 2=undef */

  /* parse option */
  opt_parse(argc, argv);

  /* select microphone input */
  if (adin_select(SP_MIC) == FALSE) {
    j_printerr("Error: microphone input is not supported\n");
    return(1);
  }
  
  /* ready the microphone (arg is dummy) */
  if (use_48to16) {
    if (sfreq != 16000) {
      j_printerr("Error: in 48kHz mode, required rate should be 16kHz!\n");
      return(1);
    }
    /* setup for down sampling */
    adin_setup_48to16();
    /* set device sampling rate to 48kHz */
    if (adin_standby(48000, NULL) == FALSE) { /* fail */
      j_printerr("Error: failed to ready input device to 48kHz\n");
      return(1);
    }
  } else {
    if (adin_standby(sfreq, NULL) == FALSE) {
      j_printerr("Error: failed to standby input\n");
      return(1);
    }
  }
  /* set device-independent param */
  adin_setup_param(silence_cut, strip_zero_sample, level_thres, zero_cross_num, margin, margin, sfreq, TRUE, use_zmean);/* use same margin for head and tail */

  /* file check */
  if (!stout) {
    if (access(filename, F_OK) == 0) {
      if (access(filename, W_OK) == 0) {
	fprintf(stderr, "Warning: overwriting file \"%s\"\n", filename);
      } else {
	perror("adinrec");
	return(1);
      }
    }
  } /* output file will be opened when triggered (not here) */

  /* set interrupt signal handler to properly close output file */
  if (signal(SIGINT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal intterupt may collapse output\n");
  }
  
  /* do recoding */
  speechlen = 0;
  
  adin_begin();
  j_printerr("<<< please speak >>>"); /* moved from adin-cut.c */
  adin_go(adin_callback_file, NULL);
  adin_end();

  /* close file and output status */
  close_file();

  return 1;
}
