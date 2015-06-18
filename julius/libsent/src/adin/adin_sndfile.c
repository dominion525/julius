/**
 * @file   adin_sndfile.c
 * @author Akinobu LEE
 * @date   Mon Feb 14 12:13:27 2005
 * 
 * <JA>
 * @brief  ファイル入力：libsndfile を用いた音声ファイル読み込み
 *
 * libsndfile を用いて音声ファイルからの入力を行なう関数です．
 * Microsoft WAVE形式の音声ファイル，およびヘッダ無し（RAW）ファイルの他に,
 * AU, AND, NIST, ADPCM など様々な形式のファイルを読み込むことができます．
 * なお，チャンネル数は１(モノラル)に限られます．またRAWファイルの場合，
 * データのバイトオーダーは big endian である必要があります．
 * 
 * ファイルのサンプリングレートはシステムの要求するサンプリングレート
 * （adin_standby() で指定される値）と一致する必要があります．
 * ファイルのサンプリングレートがこの指定値と一致しなければエラーとなります．
 * RAWファイル入力の場合は，ファイルにヘッダ情報が無く録音時の
 * サンプリングレートが不明なため，チェック無しでファイルの
 * サンプリングレートが adin_standby() で指定された値である
 * と仮定して処理されます．
 *
 * 入力ファイル名は，標準入力から読み込まれます．
 * ファイル名を列挙したファイルリストファイルが指定された場合，
 * そのファイルから入力ファイル名が順次読み込まれます．
 *
 * libsndfile はconfigure 時に自動検出されます．検出に失敗した場合，
 * ファイル入力には adin_file.c 内の関数が使用されます．
 *
 * Libsndfile のバージョンは 1.0.x に対応しています．
 *
 * @sa http://www.mega-nerd.com/libsndfile/
 * </JA>
 * <EN>
 * @brief  Audio input from file using libsndfile library.
 *
 * Functions to get input from wave file using libsndfile library.
 * Many file formats are supported, including Microsoft WAVE format,
 * and RAW (no header) format, AU, SND, NIST and so on.  The channel number
 * should be 1 (monaural).
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
 * libsndfile should be installed before compilation.  The library and header
 * will be automatically detected by configure script.  When failed to detect,
 * Julius uses adin_file.c instead for file input.
 *
 * This file will work on libsndfile version 1.0.x.
 *
 * @sa http://www.mega-nerd.com/libsndfile/
 * </EN>
 *
 * $Revision: 1.3 $
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

#ifdef HAVE_LIBSNDFILE

/* sound header */
#include <sndfile.h>

static int sfreq;		///< Required sampling frequency in Hz
static SF_INFO sinfo;		///< Wavefile information
static SNDFILE *sp;		///< File handler
static boolean from_file;	///< TRUE if reading filename from listfile
static FILE *fp_list;		///< File pointer used for the listfile

/// Check if the file format is 16bit, monoral.
static boolean
check_format(SF_INFO *s)
{
  if ((s->format & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW) {
    if (s->samplerate != sfreq) {
      j_printerr("adin_sndfile: sample rate != %d, it's %d Hz data\n", sfreq, s->samplerate);
      return FALSE;
    }
  }
  if (s->channels != 1) {
    j_printerr("adin_sndfile: channel num != 1, it has %d channels\n", s->channels);
    return FALSE;
  }
#ifdef HAVE_LIBSNDFILE_VER1
  if ((s->format & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
    j_printerr("adin_sndfile1: not 16-bit data\n");
    return FALSE;
  }
#else
  if (s->pcmbitwidth != 16) {
    j_printerr("adin_sndfile: not 16-bit data, it's %d bit\n", s->pcmbitwidth);
    return FALSE;
  }
#endif
  return TRUE;
}

/// Output format information to stdout (compliant to libsnd-0.0.23)
static void
print_format(SF_INFO *s)
{
  printf("file format: ");
  switch(s->format & SF_FORMAT_TYPEMASK) {
  case SF_FORMAT_WAV:    j_printf("Microsoft WAV"); break;
  case SF_FORMAT_AIFF:   j_printf("Apple/SGI AIFF"); break;
  case SF_FORMAT_AU:     j_printf("Sun/NeXT AU"); break;
#ifndef HAVE_LIBSNDFILE_VER1
  case SF_FORMAT_AULE:   j_printf("DEC AU"); break;
#endif
  case SF_FORMAT_RAW:    j_printf("RAW"); break;
  case SF_FORMAT_PAF:    j_printf("Ensoniq PARIS"); break;
  case SF_FORMAT_SVX:    j_printf("Amiga IFF / SVX8 / SV16"); break;
  case SF_FORMAT_NIST:   j_printf("Sphere NIST"); break;
#ifdef HAVE_LIBSNDFILE_VER1
  case SF_FORMAT_VOC:	 j_printf("VOC file"); break;
  case SF_FORMAT_IRCAM:  j_printf("Berkeley/IRCAM/CARL"); break;
  case SF_FORMAT_W64:	 j_printf("Sonic Foundry's 64bit RIFF/WAV"); break;
  case SF_FORMAT_MAT4:   j_printf("Matlab (tm) V4.2 / GNU Octave 2.0"); break;
  case SF_FORMAT_MAT5:   j_printf("Matlab (tm) V5.0 / GNU Octave 2.1"); break;
#endif
  default: j_printf("UNKNOWN TYPE"); break;
  }
  switch(s->format & SF_FORMAT_SUBMASK) {
#ifdef HAVE_LIBSNDFILE_VER1
  case SF_FORMAT_PCM_U8:    j_printf(", Unsigned 8 bit PCM"); break;
  case SF_FORMAT_PCM_S8:    j_printf(", Signed 8 bit PCM"); break;
  case SF_FORMAT_PCM_16:    j_printf(", Signed 16 bit PCM"); break;
  case SF_FORMAT_PCM_24:    j_printf(", Signed 24 bit PCM"); break;
  case SF_FORMAT_PCM_32:    j_printf(", Signed 32 bit PCM"); break;
  case SF_FORMAT_FLOAT:     j_printf(", 32bit float"); break;
  case SF_FORMAT_DOUBLE:    j_printf(", 64bit float"); break;
  case SF_FORMAT_ULAW:      j_printf(", U-Law"); break;
  case SF_FORMAT_ALAW:      j_printf(", A-Law"); break;
  case SF_FORMAT_IMA_ADPCM: j_printf(", IMA ADPCM"); break;
  case SF_FORMAT_MS_ADPCM:  j_printf(", Microsoft ADPCM"); break;
  case SF_FORMAT_GSM610:    j_printf(", GSM 6.10, "); break;
  case SF_FORMAT_G721_32:   j_printf(", 32kbs G721 ADPCM"); break;
  case SF_FORMAT_G723_24:   j_printf(", 24kbs G723 ADPCM"); break;
  case SF_FORMAT_G723_40:   j_printf(", 40kbs G723 ADPCM"); break;
#else
  case SF_FORMAT_PCM:       j_printf(", PCM"); break;
  case SF_FORMAT_FLOAT:     j_printf(", floats"); break;
  case SF_FORMAT_ULAW:      j_printf(", U-Law"); break;
  case SF_FORMAT_ALAW:      j_printf(", A-Law"); break;
  case SF_FORMAT_IMA_ADPCM: j_printf(", IMA ADPCM"); break;
  case SF_FORMAT_MS_ADPCM:  j_printf(", Microsoft ADPCM"); break;
  case SF_FORMAT_PCM_BE:    j_printf(", Big endian PCM"); break;
  case SF_FORMAT_PCM_LE:    j_printf(", Little endian PCM"); break;
  case SF_FORMAT_PCM_S8:    j_printf(", Signed 8 bit PCM"); break;
  case SF_FORMAT_PCM_U8:    j_printf(", Unsigned 8 bit PCM"); break;
  case SF_FORMAT_SVX_FIB:   j_printf(", SVX Fibonacci Delta"); break;
  case SF_FORMAT_SVX_EXP:   j_printf(", SVX Exponential Delta"); break;
  case SF_FORMAT_GSM610:    j_printf(", GSM 6.10, "); break;
  case SF_FORMAT_G721_32:   j_printf(", 32kbs G721 ADPCM"); break;
  case SF_FORMAT_G723_24:   j_printf(", 24kbs G723 ADPCM"); break;
#endif
  default: j_printf(", UNKNOWN SUBTYPE"); break;
  }

#ifdef HAVE_LIBSNDFILE_VER1
  switch(s->format & SF_FORMAT_ENDMASK) {
  case SF_ENDIAN_FILE:      j_printf(", file native endian"); break;
  case SF_ENDIAN_LITTLE:    j_printf(", forced little endian"); break;
  case SF_ENDIAN_BIG:       j_printf(", forced big endian"); break;
  case SF_ENDIAN_CPU:       j_printf(", forced CPU native endian"); break;
  }
  j_printf(", %d Hz, %d channels\n", s->samplerate, s->channels);
#else
  j_printf(", %d bit, %d Hz, %d channels\n", s->pcmbitwidth, s->samplerate, s->channels);
#endif
}


/** 
 * Initialization: if listfile is specified, open it here. Else, just store
 * the required frequency.
 * 
 * @param freq [in] required sampling frequency
 * @param arg [in] file name of listfile, or NULL if not use
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_sndfile_standby(int freq, void *arg)
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
 * If listfile was specified in adin_sndfile_standby(), the next filename
 * will be read from the listfile.  Otherwise, the
 * filename will be obtained from stdin.  Then the file will be opened here.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_sndfile_begin()
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
#ifndef HAVE_LIBSNDFILE_VER1
    sinfo.samplerate = sfreq;
    sinfo.pcmbitwidth = 16;
    sinfo.channels = 1;
#endif
    sinfo.format = 0x0;
    if ((sp = 
#ifdef HAVE_LIBSNDFILE_VER1
	 sf_open(speechfilename, SFM_READ, &sinfo)
#else
	 sf_open_read(speechfilename, &sinfo)
#endif
	 ) == NULL) {
      /* retry assuming raw format */
      sinfo.samplerate = sfreq;
      sinfo.channels = 1;
#ifdef HAVE_LIBSNDFILE_VER1
      sinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_BIG;
#else
      sinfo.pcmbitwidth = 16;
      sinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_BE;
#endif
      if ((sp =
#ifdef HAVE_LIBSNDFILE_VER1
	   sf_open(speechfilename, SFM_READ, &sinfo)
#else
	   sf_open_read(speechfilename, &sinfo)
#endif
	   ) == NULL) {
	sf_perror(sp);
	j_printerr("Error in opening speech data: \"%s\"\n",speechfilename);
      }
    }
    if (sp != NULL) {		/* open success */
      if (! check_format(&sinfo)) {
	j_printerr("Error: invalid format: \"%s\"\n",speechfilename);
	print_format(&sinfo);
      } else {
	j_printf("\ninput speechfile: %s\n",speechfilename);
	print_format(&sinfo);
	readp = TRUE;
      }
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
adin_sndfile_read(SP16 *buf, int sampnum)
{
  int cnt;

  cnt = sf_read_short(sp, buf, sampnum);
  if (cnt == 0) {		/* EOF */
    return -1;
  } else if (cnt < 0) {		/* error */
    sf_perror(sp);
    sf_close(sp);
    return -2;		/* error */
  }
  return cnt;
}

/** 
 * End recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_sndfile_end()
{
  /* close files */
  if (sf_close(sp) != 0) {
    sf_perror(sp);
    j_printerr("adin_sndfile: failed to close\n");
    return FALSE;
  }
  return TRUE;
}

#endif /* ~HAVE_LIBSNDFILE */
