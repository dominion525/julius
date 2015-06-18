/*
 * Copyright (c) 2002-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/*
 * mkss --- compute average spectrum of mic input for SS in Julius
 *
 * $Id: mkss.c,v 1.5 2006/12/26 03:17:26 sumomo Exp $
 *
 */

#include <sent/config.h> /* for WORDS_BIGENDIAN */
#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>
#include <sent/mfcc.h>
#include <sys/stat.h>

static int fd = -1;		/* file descriptor for output */

static char *filename = NULL;	/* output file name */
static boolean stout = FALSE;	/* true if output to stdout ("-") */
static int sfreq = 16000;	/* sampling frequency */
static int slen  = 3000;	/* record length in msec */
static int fsize = 400;		/* frame size: 400 = 25ms in 16kHz */
static int fshift = 160;	/* frameshift: 160 = 10ms in 16khz */
static boolean use_zmean = FALSE;
static boolean zmeanframe = FALSE;

/* parameter for SS */
static Value para;
static SP16 *speech;
static int speechnum;
static int samples;

/* print help and exit */
void
usage()
{
  fprintf(stderr, "mkss --- compute averate spectrum of mic input for SS\n");
  fprintf(stderr, "Usage: mkss [options..] filename\n");
  fprintf(stderr, "    [-freq frequency]    sampling freq in Hz   (%d)\n", sfreq);
  fprintf(stderr, "    [-len msec]          record length in msec (%d)\n", slen);
  fprintf(stderr, "    [-fsize samplenum]   window size           (%d)\n", fsize);
  fprintf(stderr, "    [-fshift samplenum]  frame shift           (%d)\n", fshift);
  fprintf(stderr, "    [-zmean]             enable zmean         (off)\n");
  fprintf(stderr, "    [-zmeanframe]        frame-wise zmean     (off)\n");
  fprintf(stderr, "Library configuration: ");
  confout_version(stderr);
  confout_audio(stderr);
  confout_process(stderr);
  fprintf(stderr, "\n");
  exit(1);
}

/* read option argument and set values */
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
      } else if (!strcmp(argv[i], "-len")) {
	i++;
	if (i >= argc) usage();
	slen = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-fsize")) {
	i++;
	if (i >= argc) usage();
	fsize = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-fshift")) {
	i++;
	if (i >= argc) usage();
	fshift = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-zmean")) {
	use_zmean = TRUE;
      } else if (!strcmp(argv[i], "-zmeanframe")) {
	zmeanframe = TRUE;
      } else if (!strcmp(argv[i], "-h")) {
	usage();
      } else if (!strcmp(argv[i], "-help")) {
	usage();
      } else if (!strcmp(argv[i], "--help")) {
	usage();
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
}

static int
adin_callback(SP16 *now, int len)
{
  int num;
  int ret;

  /* store recorded data up to samples */
  if (speechnum + len > samples) {
    num = samples - speechnum;
    /* stop recording */
    ret = 1;
  } else {
    num = len;
    /* continue recording */
    ret = 0;
  }
  memcpy(&(speech[speechnum]), now, num * sizeof(SP16));
  speechnum += num;

  fprintf(stderr, ".");

  return(ret);
}

static int x;
static int sslen;

int
main(int argc, char *argv[])
{
  float *ss;
  int silence_cut = 0;		/* disable silence cutting */
  boolean strip_zero_sample = TRUE; /* strip zero samples */
  int level_thres = 0;		/* set level thres to 0 */
  int zero_cross_num = 20;	/* dummy */
  int margin = 400;		/* dummy */

  /* parse option */
  opt_parse(argc, argv);

  /* allocate speech store buffer */
  samples = sfreq * slen / 1000;
  speech = (SP16 *)mymalloc(sizeof(SP16) * samples);

  /* select microphone input */
  if (adin_select(SP_MIC) == FALSE) {
    j_printerr("Error: microphone input is not supported\n");
    return(1);
  }
  /* ready the microphone (arg is dummy) */
  if (adin_standby(sfreq, NULL) == FALSE) {
    j_printerr("Error: failed to ready input\n");
    return(1);
  }
  /* set device-independent param */
  adin_setup_param(silence_cut, strip_zero_sample, level_thres, zero_cross_num, margin, margin, sfreq, TRUE, use_zmean);

  /* file check */
  if (!stout) {
    if (access(filename, F_OK) == 0) {
      if (access(filename, W_OK) == 0) {
	fprintf(stderr, "Warning: overwriting file \"%s\"\n", filename);
      } else {
	perror("mkss");
	return(1);
      }
    }
  }
  /* record mic input */
  fprintf(stderr, "recording %.2f second", (float)slen /(float)1000);
  speechnum = 0;
  adin_begin();
  adin_go(adin_callback, NULL);
  adin_end();
  fprintf(stderr, "\n%d samples (%d bytes, %.1f sec) recorded\n", samples, samples * sizeof(SP16), (float)samples / (float)sfreq);

  /* compute SS */
  fprintf(stderr, "compute SS:\n");
  fprintf(stderr, "  fsize : %4d samples (%.1f msec)\n", fsize, (float)fsize * 1000.0/ (float)sfreq);
  fprintf(stderr, "  fshift: %4d samples (%.1f msec)\n", fshift, (float)fshift * 1000.0/ (float)sfreq);
  para.framesize  = fsize;
  para.frameshift = fshift;
  para.preEmph    = DEF_PREENPH;
  para.zmeanframe = zmeanframe;
  ss = new_SS_calculate(speech, samples, para, &sslen);
  
  /* open file for recording */
  fprintf(stderr, "writing result to [%s]...", filename);
  if (stout) {
    fd = 1;
  } else {
    if ((fd = creat(filename, 0644)) == -1) {
      perror("mkss");
      return(1);
    }
  }
  x = sslen;
#ifndef WORDS_BIGENDIAN
  swap_bytes((char *)&x, sizeof(int), 1);
#endif
  if (write(fd, &x, sizeof(int)) < sizeof(int)) {
    perror("mkss");
    return(1);
  }
#ifndef WORDS_BIGENDIAN
  swap_bytes((char *)ss, sizeof(float), sslen);
#endif
  if (write(fd, ss, sslen * sizeof(float)) < sslen * sizeof(float)) {
    perror("mkss");
    return(1);
  }
  if (!stout) {
    if (close(fd) < 0) {
      perror("mkss");
      return(1);
    }
  }
  fprintf(stderr, "done\n");
  return 0;
}

