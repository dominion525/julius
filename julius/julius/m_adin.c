/**
 * @file   m_adin.c
 * @author Akinobu LEE
 * @date   Fri Mar 18 16:17:23 2005
 * 
 * <JA>
 * @brief  音声入力デバイスの初期化
 * </JA>
 * 
 * <EN>
 * @brief  Initialize audio input device
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

#include <julius.h>

/** 
 * <JA>
 * 音声入力デバイスを初期化する．speech_input で選択されているデバイスを
 * 初期化し，サンプリング周波数や音声検出用パラメータをセットする．
 * 
 * </JA>
 * <EN>
 * Initialize audio input device specified by speech_input, and set
 * parameters for sampling rate and speech detection parameters.
 * 
 * </EN>
 */
void
adin_initialize()
{
  char *arg = NULL;
  boolean igsp = TRUE;

  if (speech_input == SP_MFCFILE) return; /* no need to initialize */
  
  VERMES("###### initialize input device\n");

  /* select input device: file, mic, netaudio, etc... */
  if (adin_select(speech_input) == FALSE) {
    j_error("Error: invalid input device\n");
  }

  /* set sampling frequency and device-dependent configuration
     (argument is device-dependent) */
  switch(speech_input) {
  case SP_ADINNET:		/* arg: port number */
    arg = mymalloc(100);
    sprintf(arg, "%d", adinnet_port);
    break;
  case SP_RAWFILE:		/* arg: filename of file list (if any) */
    if (inputlist_filename != NULL) {
      arg = mymalloc(strlen(inputlist_filename)+1);
      strcpy(arg, inputlist_filename);
    } else {
      arg = NULL;
    }
    break;
  case SP_STDIN:
    arg = NULL;
    break;
#ifdef USE_NETAUDIO
  case SP_NETAUDIO:		/* netaudio server/port name */
    arg = mymalloc(strlen(netaudio_devname)+1);
    strcpy(arg, netaudio_devname);
    break;
#endif
  }

  if (speech_input == SP_MIC) {
#ifdef SP_BREAK_CURRENT_FRAME
    igsp = FALSE;		/* does not drop speech while decoding */
#endif
  }
  

  if (use_ds48to16) {
    if (use_ds48to16 && para.smp_freq != 16000) {
      j_error("Error: in 48kHz input mode, target sampling rate should be 16k!\n");
    }
    /* setup for 1/3 down sampling */
    adin_setup_48to16();
    /* set device sampling rate to 48kHz */
    if (adin_standby(48000, arg) == FALSE) { /* fail */
      j_error("Error: failed to ready input device\n");
    }
  } else {
    if (adin_standby(para.smp_freq, arg) == FALSE) { /* fail */
      j_error("Error: failed to ready input device\n");
    }
  }

  /* set parameter for recording/silence detection */
  adin_setup_param(silence_cut,/* silence cutting: 0=off,1=on,2=use device default */
		   strip_zero_sample, /* strip zero sample if TRUE */
		   level_thres,	/* trigger level threshold (0-32767) */
		   zero_cross_num, /* zero-cross num for silence detection */
		   head_margin_msec, /* margin at beginning of input segment */
		   tail_margin_msec, /* margin at end of input segment */
		   para.smp_freq, /* sampling frequency */
		   igsp,	/* TRUE if ignore speech while decoding */
		   use_zmean);	/* TRUE if need zmeansource */

  if (arg != NULL) free(arg);
}
