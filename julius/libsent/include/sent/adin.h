/**
 * @file   adin.h
 * @author Akinobu LEE
 * @date   Thu Feb 10 17:22:36 2005
 *
 * <EN>
 * @brief  Definitions for A/D-in processing and speech detection.
 *
 * This file has some definitions relating audio input processing from
 * various devices, and start/end of speech detection.
 * @sa speech.h
 * </EN>
 * <JA>
 * @brief  音声入力および音声区間検出に関する定義
 *
 * このファイルには, さまざまなソースからの音声入力処理と音声区間の検出
 * に関連するいくつかの定義が含まれています．
 * @sa speech.h
 * </JA>
 *
 * $Revision: 1.5 $ 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_ADIN__
#define __SENT_ADIN__

#include <sent/stddefs.h>
#include <sent/speech.h>

/// To select speech input source
enum {
  SP_RAWFILE,			///< Wavefile
  SP_MIC,			///< Live microphone device
  SP_ADINNET,			///< Network client (adintool etc.)
  SP_MFCFILE,			///< HTK parameter file
  SP_NETAUDIO,			///< Live NetAudio/DatLink input
  SP_STDIN			///< Standard input
};

/// Default unit size of speech input segment in bytes
#define DEFAULT_WSTEP 1000

/**
 * @def SUPPORTED_WAVEFILE_FORMAT
 * String describing the list of supported wave file formats.
 * It depends on HAVE_LIBSNDFILE.
 */
#ifdef HAVE_LIBSNDFILE
#define SUPPORTED_WAVEFILE_FORMAT "RAW(BE),WAV,AU,SND,NIST,ADPCM and more"
#else
#define SUPPORTED_WAVEFILE_FORMAT "RAW(BE),WAV"
#endif

/**
 * Number of samples from beggining of input to be used for computing
 * the zero mean of source channel (for microphone/network input).
 * 
 */
#define ZMEANSAMPLES 48000

/* adin/adin.c */
boolean adin_select(int source);
boolean adin_standby(int freq, void *arg);
boolean adin_begin();
boolean adin_end();

/* adin/adin-cut.c */
void
adin_setup_func(int (*cad_read)(SP16 *, int),
		boolean (*cad_pause)(),
		boolean (*cad_resume)(),
		boolean use_cut_def,
		boolean need_thread);
void adin_setup_param(int silence_cut, boolean strip_zero, int cthres, int czc, int margin, int tail_margin, int sample_freq, boolean ignore_speech, boolean need_zeromean);
boolean query_segment_on();
boolean query_thread_on();
void adin_reset_zmean();
int adin_go(int (*ad_process)(SP16 *, int), int (*ad_check)());
void adin_setup_48to16();

/* adin/adin_mic_*.c */
boolean adin_mic_standby(int freq, void *arg);
boolean adin_mic_start();
boolean adin_mic_stop();
int adin_mic_read(SP16 *buf, int sampnum);
/* adin/adin_netaudio.c  and adin/adin_na.c */
boolean adin_netaudio_standby(int freq, void *arg);
boolean adin_netaudio_start();
boolean adin_netaudio_stop();
int adin_netaudio_read(SP16 *buf, int sampnum);
int NA_standby(int, char *);
void NA_start();
void NA_stop();
int NA_read(SP16 *buf, int sampnum);

/* adin/adin_file.c */
char *get_line(char *prompt);
boolean adin_file_standby(int freq, void *arg);
boolean adin_file_begin();
int adin_file_read(SP16 *buf, int sampnum);
boolean adin_file_end();
boolean adin_stdin_standby(int freq, void *arg);
boolean adin_stdin_begin();
int adin_stdin_read(SP16 *buf, int sampnum);

/* adin/adin_sndfile.c */
#ifdef HAVE_LIBSNDFILE
boolean adin_sndfile_standby(int freq, void *arg);
boolean adin_sndfile_begin();
int adin_sndfile_read(SP16 *buf, int sampnum);
boolean adin_sndfile_end();
#endif

/* adin/adin_tcpip.c */
boolean adin_tcpip_standby(int freq, void *arg);
boolean adin_tcpip_begin();
boolean adin_tcpip_end();
int adin_tcpip_read(SP16 *buf, int sampnum);
boolean adin_tcpip_send_pause();
boolean adin_tcpip_send_terminate();
boolean adin_tcpip_send_resume();

/* adin/zc-e.c */
void init_count_zc_e(int c_trigger, int c_length, int c_offset);
void end_count_zc_e();
int count_zc_e(SP16 *buf,int step);
int count_zc_e_level(SP16 *buf,int step,int *levelp);
void zc_copy_buffer(SP16 *newbuf, int *len);

/* adin/zmean.c */
void zmean_reset();
void sub_zmean(SP16 *speech, int samplenum);

/* adin/ds48to16.c */
boolean ds48to16_setup();
int ds48to16(SP16 *dst, SP16 *src, int srclen, int maxdstlen);

#endif /* __SENT_ADIN__ */
