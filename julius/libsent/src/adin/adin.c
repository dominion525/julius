/**
 * @file   adin.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 12:02:08 2005
 *
 * <JA>
 * @brief  入力ソースの選択と準備，および録音の開始・読み込み・録音停止
 *
 * ここには，入力ソースの設定およびデータ読み込み，開始・停止を呼び出す
 * ための汎用関数が定義されています．
 *
 * ユーザはまず最初に adin_select() を呼び，入力ソースごとに依存した
 * 関数と設定値をセットします．あとは， adin_standby() でデバイスの初期化，
 * adin_begin() , adin_end() で入力ストリームの開始と終了， adin_pause() と
 * adin_resume() でストリームの中断と再開，adin_read() でサンプルの読み込み
 * を行ないます．それぞれの関数の実体は，選択した入力ソースごとに
 * それぞれ別ファイルで定義されています．この仕組みにより，ユーザは
 * ファイルやマイクなどの入力ソースごとの処理の違いを気にせずにすみます．
 * 
 * 以下に，実際にどのようにこれらの関数が呼ばれているかを
 * 簡単に示します．adin_begin(), adin_end() と adin_resume(), adin_pause() の
 * 違いは，前者はある入力ストリーム全体の開始と終了，後者はその中で
 * 検出した音声区間ごとの入力中断・再開を表します．例えばファイル入力では，
 * 前者をファイルの open と close に対応し，マイク入力では（入力ストリーム
 * は無限に続くので）後者を入力の中断と再開に対応しています．実際の処理では，
 * ストリームの終了や入力エラーにも対応する必要があります．
 * </JA>
 * <EN>
 * @brief  General function to select, initialize, start, stop, capture audio input
 *
 * This file contains a function to select input source and general functions
 * to initialize, start, stop, capture the audio input.
 *
 * User should call adin_select() at first to select input device dependent
 * functions and variables.  After that, user can  call adin_standby() to
 * initialize the selected device (with device specific argument if required),
 * use adin_begin() and adin_end() to begin and end an input stream, use
 * adin_pause() and adin_resume() to suspend and resume the current input
 * stream, and use adin_read() to read the audio input.  The entity of these
 * functions are actually defined in separate files in this directory.
 * This interface allows user to handle audio input consistently.
 *
 * The following describes how these functions should be called.
 * The difference between adin_begin(), adin_end() and adin_resume(),
 * adin_pause() is that, the former corresponds to the start and end of
 * an input stream, and the latter corrsponds to the pause and resume of
 * the current input stream, typically for speech detection.  For examples,
 * in file input, file open and close are assigned to adin_begin() and
 * adin_end(), and in live microphone input, device is opened in adin_standby()
 * and their pause and resume are handled in adin_resume() and adin_pause().
 * </EN>
 *
 * @code
 * adin_go() {
 *   adin_resume();
 *   for(;;)
 *     adin_read();
 *     SOME SPEECH SEGMENT DETECTION HERE
 *     if (no_segmentation || valid_segment) callback();
 *     if (error || end_of_stream || (!no_segmentation && end_of_segment)) break;
 *   }
 *   adin_pause();
 *   return(-1..error 0...end of stream, 1...end of segment);
 * }
 *
 * main() {
 *   adin_standby(freq, arg);
 *   for(;;) {
 *     adin_begin();
 *     for(;;) {
 *       adin_go();
 *       if (error) break;
 *       Process_2nd_Pass;
 *       if (end of segment) break;
 *     }
 *     adin_end();
 *   }
 * }
 * @endcode
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

/* device handling functions out side of the speech detection functions */
/// Pointer to store function for device initialization (call once on startup)
static boolean (*ad_standby)(int, void *);
/// Pointer to store function for begin capturing
static boolean (*ad_begin)();
/// Pointer to store function for end capturing
static boolean (*ad_end)();

/** 
 * Assign specified functions to adin functions and adin_cut().
 * 
 */
static void
adin_register_func(boolean (*cad_standby)(int, void *),///< [in] Function of device initialization
		   boolean (*cad_begin)(),///< [in] Function to begin input stream for capturing
		   int (*cad_read)(SP16 *, int),///< [in] Function to read captured samples
		   boolean (*cad_end)(), ///< [in] Function to end input stream
		   boolean (*cad_resume)(),///< [in] Function to resume the paused stream
		   boolean (*cad_pause)(),///< [in] Function to pause the stream
		   boolean segmentation_default,///< [in] TRUE if speech detection and segmentation is preferrable on this device by default
		   boolean need_threaded) ///< [in] TRUE if this device is live input and needs threading of A/D-in process
{
  ad_standby = cad_standby;
  ad_begin = cad_begin;
  ad_end = cad_end;
  adin_setup_func(cad_read, cad_pause, cad_resume,
		  segmentation_default, 
		  need_threaded);
}


/** 
 * Select input source and setup device-specific functions.
 * 
 * @param source [In] selection ID of input source @sa adin.h
 * 
 * @return TRUE on success, FALSE if @a source is not available.
 */
boolean
adin_select(int source)
{
  switch(source) {
  case SP_RAWFILE:
#ifdef HAVE_LIBSNDFILE
    /* libsndfile interface */
    adin_register_func(adin_sndfile_standby, /* standby (called once on startup) */
		       adin_sndfile_begin, /* start (called for each stream) */
		       adin_sndfile_read, /* read (read sample) */
		       adin_sndfile_end, /* stop (called for each stream) */
		       NULL,	/* pause (called between each segment) */
		       NULL,	/* resume (called between each segment) */
		       FALSE,	/* default of pause segmentation */
		       FALSE);	/* TRUE if need threaded */
#else  /* ~HAVE_LIBSNDFILE */
    /* built-in RAW/WAV reader */
    adin_register_func(adin_file_standby, /* standby (called once on startup) */
		       adin_file_begin, /* start (called for each stream) */
		       adin_file_read, /* read (read sample) */
		       adin_file_end, /* stop (called for each stream) */
		       NULL,	/* pause (called between each segment) */
		       NULL,	/* resume (called between each segment) */
		       FALSE,	/* default of pause segmentation */
		       FALSE);	/* TRUE if need threaded */
#endif
    break;
#ifdef USE_MIC
  case SP_MIC:
    /* microphone input */
    adin_register_func(adin_mic_standby,
		       NULL,
		       adin_mic_read,
		       NULL,
		       adin_mic_start,
		       adin_mic_stop,
		       TRUE,
		       TRUE);
    break;
#endif
#ifdef USE_NETAUDIO
  case SP_NETAUDIO:
    /* DatLink/NetAudio input */
    adin_register_func(adin_netaudio_standby,
		       NULL,
		       adin_netaudio_read,
		       NULL,
		       adin_netaudio_start,
		       adin_netaudio_stop,
		       TRUE,
		       TRUE);
    break;
#endif
  case SP_ADINNET:
    /* adinnet network input */
    adin_register_func(adin_tcpip_standby,
		       adin_tcpip_begin,
		       adin_tcpip_read,
		       adin_tcpip_end,
		       NULL,
		       NULL,
		       FALSE,
		       FALSE);

    break;
  case SP_STDIN:
    /* standard input */
    adin_register_func(adin_stdin_standby,
		       adin_stdin_begin,
		       adin_stdin_read,
		       NULL,
		       NULL,
		       NULL,
		       FALSE,
		       FALSE);
    break;
  case SP_MFCFILE:
    /* MFC_FILE is not waveform, so special handling on main routine should be done */
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

/** 
 * Call device-specific initialization.
 * 
 * @param freq [in] sampling frequency
 * @param arg [in] device-dependent extra argument
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_standby(int freq, void *arg)
{
  adin_reset_zmean();		/* reset zmean at beginning of stream */
  if (ad_standby != NULL) return(ad_standby(freq, arg));
  else return TRUE;
}
/** 
 * Call device-specific function to begin capturing of the audio stream.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_begin()
{
  adin_reset_zmean();		/* reset zmean at beginning of stream */
  if (ad_begin != NULL) return(ad_begin());
  else return TRUE;
}
/** 
 * Call device-specific function to end capturing of the audio stream.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_end()
{
  if (ad_end != NULL) return(ad_end());
  else return TRUE;
}
