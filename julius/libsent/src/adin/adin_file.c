/**
 * @file   adin_file.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 13:31:20 2005
 *
 * <JA>
 * @brief  �ե��������ϡ�WAV/RAW�ե����뤪���ɸ�����Ϥ�����ɤ߹���
 *
 * �����ե����뤫������Ϥ�Ԥʤ��ؿ��Ǥ������ݡ��Ȥ��Ƥ���ե����������
 * Microsoft WAVE�����β����ե����롤����ӥإå�̵����RAW�˥ե�����Ǥ���
 * �����ǥ��������ϡ�̵����PCM��16bit����Υ��˸¤��Ƥ��ޤ���
 * RAW�ե�����ξ�硤�ǡ����ΥХ��ȥ��������� big endian �Ǥ��뤳�Ȥ�
 * ����Ȥ��Ƥ��ޤ��Τǡ���դ��Ƥ���������
 * 
 * �ե�����Υ���ץ�󥰥졼�Ȥϥ����ƥ���׵᤹�륵��ץ�󥰥졼��
 * ��adin_standby() �ǻ��ꤵ����͡ˤȰ��פ���ɬ�פ�����ޤ���
 * WAVE�����Υե�����ξ��ϡ�
 * �ե�����Υ���ץ�󥰥졼�Ȥ����λ����ͤȰ��פ��ʤ���Х��顼�Ȥʤ�ޤ���
 * RAW�ե��������Ϥξ��ϡ��ե�����˥إå�����̵��Ͽ������
 * ����ץ�󥰥졼�Ȥ������ʤ��ᡤ�����å�̵���ǥե������
 * ����ץ�󥰥졼�Ȥ� adin_standby() �ǻ��ꤵ�줿�ͤǤ���
 * �Ȳ��ꤷ�ƽ�������ޤ���
 *
 * ���ϥե�����̾�ϡ�ɸ�����Ϥ����ɤ߹��ޤ�ޤ���
 * �ե�����̾����󤷤��ե�����ꥹ�ȥե����뤬���ꤵ�줿��硤
 * ���Υե����뤫�����ϥե�����̾���缡�ɤ߹��ޤ�ޤ���
 *
 * libsndfile ����Ѥ����硤adin_sndfile.c ��δؿ������Ѥ���ޤ���
 * ���ξ�硤���Υե�����δؿ��ϻ��Ѥ���ޤ���
 *
 * ���Υե�������Ǥ� int �� 4byte, short �� 2byte �Ȳ��ꤷ�Ƥ��ޤ���
 * </JA>
 * <EN>
 * @brief  Audio input from file or stdin
 *
 * Functions to get input from wave file or standard input.
 * Two file formats are supported, Microsoft WAVE format and RAW (no header)
 * format.  The audio format should be uncompressed PCM, 16bit, monoral.
 * On RAW file input, the data byte order must be in big endian.
 *
 * The sampling rate of input file must be equal to the system requirement
 * value which is specified by adin_standby() . For WAVE format file,
 * the sampling rate of the input file described in its header is checked
 * against the system value, and rejected if not matched.  But for RAW format
 * file, no check will be applied since it has no header information about
 * the recording sampling rate, so be careful of the sampling rate setting.
 *
 * When file input mode, the file name will be read from standard input.
 * If a filelist file is specified, the file names are read from the file
 * sequencially instead.
 *
 * When compiled with libsndfile support, the functions in adin_sndfile.c
 * is used for file input instead of functions below.
 *
 * In this file, assume sizeof(int)=4, sizeof(short)=2
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


#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>

static FILE *gfp;		///< File pointer of current input file
static boolean wav_p;		///< TRUE if input is WAVE file, FALSE if RAW file
static int maxlen;		///< Number of samples, described in the header of WAVE file
static int nowlen;		///< Current number of read samples
/**
 * When file input, the first 4 bytes of the file are read at first to
 * identify whether it is WAVE file format.  This work area is used to
 * keep back the 4 bytes if the input is actually a RAW format.
 */
static SP16 pre_data[2];
static boolean has_pre;		///< TRUE if pre_data is available

static unsigned int sfreq;	///< Sampling frequency in Hz, specified by adin_standby()

/* read .wav data with endian conversion */
/* (all .wav datas are in little endian) */

/** 
 * Read a header value from WAVE file with endian conversion.
 * If required number of data has not been read, it produces error.
 * 
 * @param buf [out] Pointer to store read data
 * @param unitbyte [in] Unit size
 * @param unitnum [in] Required number of units to read
 * @param fp [in] File pointer
 * 
 * @return TRUE on success, FALSE if required number of data has not been read.
 */
static boolean
myread(void *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  int tmp;
  if ((tmp = myfread(buf, unitbyte, unitnum, fp)) < unitnum) {
    return(FALSE);
  }
#ifdef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}
/// Abbreviation for header reading
#define MYREAD(A,B,C,D)  if (!myread(A, B, C, D)) {j_printerr("adin_file: file is corrupted\n"); return -1;}

/** 
 * @brief  Parse header part of a WAVE file to prepare for data reading
 *
 * The audio format will be checked here, and data length is also read from
 * the header.  Then the pointer is moved to the start point of data part.
 *
 * When called, the file pointer should be located just after 
 * the first 4 bytes, "RIFF".  It also sets @a maxlen and @a nowlen .
 * 
 * @param fp [in] File pointer
 * 
 * @return TRUE if check successfully passed, FALSE if an error occured.
 */
static boolean
setup_wav(FILE *fp)
{
  char dummy[9];
  unsigned int i, len;
  unsigned short s;

  /* 4 byte: byte num of rest ( = filesize - 8) */
  /* --- just skip them */
  MYREAD(dummy, 1, 4, fp);
  /* first part: WAVE format specifications */
  /* 4 byte: "WAVE" */
  MYREAD(dummy, 1, 4, fp);
  if (dummy[0] != 'W' ||
      dummy[1] != 'A' ||
      dummy[2] != 'V' ||
      dummy[3] != 'E') {
    j_printerr("adin_file: WAVE header not found, file corrupted?\n");
    fclose_readfile(fp); return FALSE;
  }
  /* format chunk: "fmt " */
  MYREAD(dummy, 1, 4, fp);
  if (dummy[0] != 'f' ||
      dummy[1] != 'm' ||
      dummy[2] != 't' ||
      dummy[3] != ' ') {
    j_printerr("adin_file: fmt chunk not found, file corrupted?\n");
    fclose_readfile(fp); return FALSE;
  }
  /* 4byte: byte size of this part */
  MYREAD(&len, 4, 1, fp);

  /* 2byte: data format */
  MYREAD(&s, 2, 1, fp);
  if (s != 1) {
    j_printerr("adin_file: data format != PCM (id=%d)\n", s);
    fclose_readfile(fp); return FALSE;
  }
  /* 2byte: channel num */
  MYREAD(&s, 2, 1, fp);
  if (s >= 2) {
    j_printerr("adin_file: channel num != 1 (%d)\n", s);
    fclose_readfile(fp); return FALSE;
  }
  /* 4byte: sampling rate */
  MYREAD(&i, 4, 1, fp);
  if (i != sfreq) {
    j_printerr("adin_file: sampling rate != %d (%d)\n", sfreq, i);
    fclose_readfile(fp); return FALSE;
  }
  /* 4byte: bytes per second */
  MYREAD(&i, 4, 1, fp);
  if (i != sfreq * sizeof(SP16)) {
    j_printerr("adin_file: bytes per second != %d (%d)\n", sfreq * sizeof(SP16), i);
    fclose_readfile(fp); return FALSE;
  }
  /* 2bytes: bytes per frame ( = (bytes per sample) x channel ) */
  MYREAD(&s, 2, 1, fp);
  if (s != 2) {
    j_printerr("adin_file: (bytes per sample) x channel != 2 (%d)\n", s);
    fclose_readfile(fp); return FALSE;
  }
  /* 2bytes: bits per sample */
  MYREAD(&s, 2, 1, fp);
  if (s != 16) {
    j_printerr("adin_file: bits per sample != 16 (%d)\n", s);
    fclose_readfile(fp); return FALSE;
  }
  /* skip rest */
  if (len > 16) {
    len -= 16;
    while (len > 0) {
      if (len > 8) {
	MYREAD(dummy, 1, 8, fp);
	len -= 8;
      } else {
	MYREAD(dummy, 1, len, fp);
	len = 0;
      }
    }
  }
  /* end of fmt part */

  /* seek for 'data' part */
  while (myread(dummy, 1, 4, fp)) {
    MYREAD(&len, 4, 1, fp);
    if (dummy[0] == 'd' &&
	dummy[1] == 'a' &&
	dummy[2] == 't' &&
	dummy[3] == 'a') {
      break;
    }
    for (i=0;i<len;i++) myread(dummy, 1, 1, fp);
  }
  /* ready to read in "data" part --- this is speech data */
  maxlen = len / sizeof(SP16);
  nowlen = 0;
  return TRUE;
}

/** 
 * @brief  Open input file
 *
 * Open the file, check the file format, and set the file pointer to @a gfp .
 * 
 * @param filename [in] file name, or NULL for standard input
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_file_open(char *filename)	/* NULL for standard input */
{
  FILE *fp;
  char dummy[4];

  if (filename != NULL) {
    if ((fp = fopen_readfile(filename)) == NULL) {
      j_printerr("failed to open %s\n",filename);
      return(FALSE);
    }
  } else {
    fp = stdin;
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
    if (_setmode( _fileno( stdin ), _O_BINARY ) == -1) {
      j_printerr("Error: _setmode() failed\n");
    }
#endif
  }
    
  /* check first 4 byte to detect Microsoft WAVE format */
  if (myfread(dummy, 1, 4, fp) < 4) {
    j_printerr("Error: size less than 4 bytes?\n",filename);
    fclose_readfile(fp);
    return(FALSE);
  }
  if (dummy[0] == 'R' &&
      dummy[1] == 'I' &&
      dummy[2] == 'F' &&
      dummy[3] == 'F') {
    /* it's a WAVE file */
    wav_p = TRUE;
    has_pre = FALSE;
    if (setup_wav(fp) == FALSE) {
      j_printerr("Error: failed to read %s as a wav file\n",filename);
      fclose_readfile(fp);
      return(FALSE);
    }
  } else {
    /* read as raw format file */
    wav_p = FALSE;
    memcpy(pre_data, dummy, 4);    /* already read (4/sizeof(SP)) samples */
    has_pre = TRUE;
  }

  gfp = fp;

  return(TRUE);
}

/** 
 * Close the input file
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_file_close()
{
  FILE *fp;

  fp = gfp;
  if (fclose_readfile(fp) != 0) {
    perror("adin_file_close: fclose_readfile");
    return FALSE;
  }
 return TRUE; 
}



/* get 1 line input from stdin with prompt */
/* return value: newly allocated buffer */
/* repeat if no input, and */
/* returns NULL on EOF */
/** 
 * Get one file name from stdin with a prompt .  Blank line is omitted.
 * 
 * @param prompt [in] prompt string
 * 
 * @return newly allocated buffer filled with the input.
 */
char *
get_line(char *prompt)
{
  char *buf = NULL;
  char *p;

  buf = (char *)mymalloc(500);
  do {
    j_printerr("%s",prompt);
    if (fgets(buf, 500, stdin) == NULL) {
      free(buf);
      return(NULL);
    }
  } while (!buf[0]);		/* loop till some input */
  /* chop last newline */
  p = buf + strlen(buf) - 1;
  if (p >= buf && *p == '\n') {
    *(p --) = '\0';
  }
  if (p >= buf && *p == '\r') {
    *(p --) = '\0';
  }
  /* chop last space */
  while(p >= buf && *p == ' ') {
    *(p --) = '\0';
  }

  return(buf);
}

static boolean from_file;	///< TRUE if list file is used to read input filename
static FILE *fp_list;		///< File pointer used for the listfile

/** 
 * Initialization: if listfile is specified, open it here.
 * 
 * @param freq [in] required sampling frequency.
 * @param arg [in] file name of listfile, or NULL if not use
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_file_standby(int freq, void *arg)
{
  char *fname = arg;
  if (fname != NULL) {
    /* read input filename from file */
    if ((fp_list = fopen(fname, "r")) == NULL) {
      j_printerr("failed to open %s\n", fname);
      return(FALSE);
    }
    from_file = TRUE;
  } else {
    /* read filename from stdin */
    from_file = FALSE;
  }
  /* store sampling frequency */
  sfreq = freq;
  
  return(TRUE);
}

/** 
 * @brief  Begin reading audio data from a file.
 *
 * If listfile was specified in adin_file_standby(), the next filename
 * will be read from the listfile.  Otherwise, the
 * filename will be obtained from stdin.  Then the file will be opened here.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_file_begin()
{
  char *speechfilename;
  boolean readp;

  /* ready to read next input */
  readp = FALSE;
  while(readp == FALSE) {
    if (from_file) {
      /* read file name from listfile */
      speechfilename = (char *)mymalloc(500);
      do {
	if (getl_fp(speechfilename, 500, fp_list) == NULL) { /* end of input */
	  free(speechfilename);
	  fclose(fp_list);
	  return(FALSE); /* end of input */
	}
      } while (speechfilename[0] == '#'); /* skip comment */
    } else {
      /* read file name from stdin */
      speechfilename = get_line("enter filename->");
      if (speechfilename == NULL) return (FALSE);	/* end of input */
    }
    /* open input file */
    if (adin_file_open(speechfilename) == FALSE) {
      j_printerr("Error in reading speech data: \"%s\"\n",speechfilename);
    } else {
      j_printf("\ninput speechfile: %s\n",speechfilename);
      readp = TRUE;
    }
    free(speechfilename);
  }
  return TRUE;
}

/** 
 * Try to read @a sampnum samples and returns actual sample num recorded.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -1 if EOF, -2 if error.
 */
int
adin_file_read(SP16 *buf, int sampnum)
{
  FILE *fp;
  int cnt;

  fp = gfp;
  
  if (wav_p) {
    cnt = myfread(buf, sizeof(SP16), sampnum, fp);
    if (nowlen + cnt > maxlen) {
      cnt = maxlen - nowlen;
    }
    nowlen += cnt;
  } else {
    if (has_pre) {
      buf[0] = pre_data[0]; buf[1] = pre_data[1];
      has_pre = FALSE;
      cnt = myfread(&(buf[2]), sizeof(SP16), sampnum - 2, fp);
      if (cnt > 0) cnt += 2;
    } else {
      cnt = myfread(buf, sizeof(SP16), sampnum, fp);
    }
  }
  if (cnt == 0) {		/* error or EOF */
    if (myfeof(fp) == 1) {		/* EOF */
      return -1;
    }
    perror("adin_file_read");
    adin_file_close();
    return -2;		/* error */
  }
  /* all .wav data are in little endian */
  /* assume .raw data are in big endian */
#ifdef WORDS_BIGENDIAN
  if (wav_p) swap_sample_bytes(buf, cnt);
#else
  if (!wav_p) swap_sample_bytes(buf, cnt);
#endif
  return cnt;
}

/** 
 * End recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_file_end()
{
  /* nothing needed */
  adin_file_close();
  return TRUE;
}

/** 
 * Initialization for speech input via stdin.
 * 
 * @param freq [in] required sampling frequency.
 * @param arg dummy, ignored
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_stdin_standby(int freq, void *arg)
{
  /* store sampling frequency */
  sfreq = freq;
  return(TRUE);
}

/** 
 * @brief  Begin reading audio data from stdin
 *
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_stdin_begin()
{
  if (feof(stdin)) {		/* already reached the end of input stream */
    return FALSE;		/* terminate search here */
  } else {
    /* open input stream */
    if (adin_file_open(NULL) == FALSE) {
      j_printerr("Error in reading speech data from stdin\n");
    }
    j_printf("Reading wavedata from stdin...\n");
  }
  return TRUE;
}

/** 
 * Try to read @a sampnum samples and returns actual sample num recorded.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -1 if EOF, -2 if error.
 */
int
adin_stdin_read(SP16 *buf, int sampnum)
{
  int cnt;

  if (wav_p) {
    cnt = myfread(buf, sizeof(SP16), sampnum, stdin);
  } else {
    if (has_pre) {
      buf[0] = pre_data[0]; buf[1] = pre_data[1];
      has_pre = FALSE;
      cnt = fread(&(buf[2]), sizeof(SP16), sampnum - 2, stdin);
      if (cnt > 0) cnt += 2;
    } else {
      cnt = fread(buf, sizeof(SP16), sampnum, stdin);
    }
  }
  if (cnt == 0) {     
    if (ferror(stdin)) {		/* error */
      perror("adin_stdin_read");
      return -2;		/* error */
    }
    return -1;			/* EOF */
  }
  /* all .wav data are in little endian */
  /* assume .raw data are in big endian */
#ifdef WORDS_BIGENDIAN
  if (wav_p) swap_sample_bytes(buf, cnt);
#else
  if (!wav_p) swap_sample_bytes(buf, cnt);
#endif
  return cnt;
}
