/**
 * @file   record.c
 * @author Akinobu Lee
 * @date   Tue Sep 06 14:13:54 2005
 * 
 * <JA>
 * @brief  認識した入力音声をファイルに保存する．
 *
 * 入力された音声データを1つずつファイルに保存する．
 * ファイル名は録音時のタイムスタンプから "YYYY.MMDD.HHMMSS.wav" となる．
 * ファイル形式は Microsoft WAVE format, 16bit, PCM (無圧縮) である．
 *
 * 録音はいったんメモリに確保されずに、入力と平行してファイルに直接
 * 書き込まれる。最初は一時ファイルに記録され、録音終了後（＝第1パス
 * 終了後）に上記の形式のファイル名に変更される。
 * </JA>
 * 
 * <EN>
 * @brief  Record speech inputs into successive files.
 *
 * These functions record each input data to the corresponding file with
 * file name of their time stamp in a format of "YYYY.MMDD.HHMMSS.wav".
 * The audio format is Microsoft WAVE, 16bit, PCM (no compression).
 *
 * The recording will not be stored in memory, instead it will be directly
 * recorded to a temporary file on the fly.  After an end-of-sentence found
 * and the first pass ends, the temporary file will be moved to the
 * final filename descrived above.
 * </EN>
 * 
 * $Revision: 1.4 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>
#include <time.h>

#define TSTRLEN 18		///< String length of a time format

static char tstr[TSTRLEN];	///< Work area to hold time string
static char recfilename[MAXLINELEN]; ///< Temporary file name for storing the incoming data
static char finalfilename[MAXLINELEN]; ///< After recording is done, the file will be renames to this
static FILE *fp = NULL;		///< File pointer of current file.
static int totalnum;		///< Current number of recorded samples.

/** 
 * <JA>
 * システム時間からベースファイル名を作成する．
 * 
 * @param t [out] 結果を格納する文字列バッファ
 * @param maxlen [in] @a t の最大長
 * </JA>
 * <EN>
 * Make base filename string from current system time.
 * 
 * @param t [out] string buffer to hold the result string.
 * @param maxlen [in] the length of @a t.
 * </EN>
 */
static void
timestring(char *t, int maxlen)
{
  time_t timep;
  struct tm *lmtm;

  time(&timep);
  lmtm = localtime(&timep);

  snprintf ( t, maxlen,"%04d.%02d%02d.%02d%02d%02d", 1900+lmtm->tm_year, 1+lmtm->tm_mon, lmtm->tm_mday, lmtm->tm_hour, lmtm->tm_min, lmtm->tm_sec);
}

/** 
 * <JA>
 * ベースファイル名から実際のパス名を作成する．ディレクトリは大域変数
 * record_dirname であらかじめ指定されている．
 * 
 * @param buf [out] 結果のパス名を格納するバッファへのポインタ
 * @param buflen [in] @a buf の最大長
 * @param filename [in] ベースファイル名
 * </JA>
 * <EN>
 * Make actual file path name from base file name.  The recording directory
 * should be specified by the global variable "record_dirname".
 * 
 * @param buf [out] buffer to hold the result string of this function
 * @param buflen [in] maximum length of @a buf.
 * @param filename [in] base filename.
 * </EN>
 */
static void
make_record_filename(char *buf, int buflen, char *filename)
{
  if (record_dirname == NULL) {
    j_error("no record directory specified??\n");
  }
  snprintf(buf, buflen,
#if defined(_WIN32) && !defined(__CYGWIN32__)
	   "%s\\%s.wav"
#else
	   "%s/%s.wav"
#endif
	   , record_dirname, filename);
}

/** 
 * <JA>
 * 一時ファイル名を作成する．
 * 
 * @param buf [out] 結果のファイル名を格納するポインタ
 * @param buflen [in] @a buf の最大長
 * </JA>
 * <EN>
 * Make temporary filename to store the incoming data while recording.
 * 
 * @param buf [out] pointer of buffer to hold the resulting file name.
 * @param buflen [in] maximum length of @a buf.
 * </EN>
 */
static void
make_tmp_filename(char *buf, int buflen)
{
#if defined(_WIN32) && !defined(__CYGWIN32__)
  snprintf(buf, buflen, "%s\\tmprecord.000", record_dirname);
#else
  snprintf(buf, buflen, "%s/tmprecord.%d", record_dirname, getpid());
#endif
}  

/** 
 * <JA>
 * 録音のために一時ファイルをオープンする．
 * 
 * </JA>
 * <EN>
 * Open temporary file for starting recording.
 * 
 * </EN>
 */
void
record_sample_open()
{
  if (fp != NULL) {
    j_error("Error: record_sample_open: re-opened before closed!\n");
  }
  make_tmp_filename(recfilename, MAXLINELEN);
  if ((fp = wrwav_open(recfilename, para.smp_freq)) == NULL) {
    perror("Error: record_sample_open");
    j_error("failed to open \"%s\" (temporary record file)\n", recfilename);
  }

  totalnum = 0;
}

/** 
 * <JA>
 * 入力音声断片をファイルに追加記録する．
 * 
 * @param speech [in] 音声データのバッファ
 * @param samplenum [in] 音声データの長さ（サンプル数）
 * </JA>
 * <EN>
 * Append speech segment to file previously opened by record_sample_open().
 * 
 * @param speech [in] speech buffer 
 * @param samplenum [in] length of above in samples
 * </EN>
 */
void
record_sample_write(SP16 *speech, int samplenum)
{
  if (fp == NULL) {
    j_error("Error: record_sample_write; file not opened yet, cannot write!\n");
  }

  if (wrwav_data(fp, speech, samplenum) == FALSE) {
    perror("Error: record_sample_write");
    j_error("failed to write samples to \"%s\"\n", recfilename);
  }

  /* make timestamp of system time when an input begins */
  /* the current temporal recording file will be renamed to this time-stamp filename */
  if (totalnum == 0) {		/* beginning of recording */
    timestring(tstr, TSTRLEN);
  }
  
  totalnum += samplenum;
}

/** 
 * <JA>
 * 録音を終了する．録音用の一時ファイルをクローズし、本来の名前にrenameする。
 * 
 * </JA>
 * <EN>
 * End recording.  Close the current temporary recording file, and
 * rename the temporary file to the final time-stamp file name.
 * 
 * </EN>
 */
void
record_sample_close()
{
  if (fp == NULL) {
    j_printerr("Warning: record_sample_close; file not opened yet!?\n");
    return;
  }

  if (wrwav_close(fp) == FALSE) {
    perror("Error: record_sample_close");
  }
  fp = NULL;

  if (totalnum == 0) {
    unlink(recfilename);
    if (verbose_flag) {
      j_printerr("No input, not recorded\n");
    }
    return;
  }

  /* now rename the temporary file to time-stamp filename */
  make_record_filename(finalfilename, MAXLINELEN, tstr);
  if (rename(recfilename, finalfilename) < 0) {
    perror("Error: record_sample_close");
    j_error("failed to move %s to %s\n", recfilename, finalfilename);
  }
  if (verbose_flag) {
    j_printerr("recorded to \"%s\" (%d bytes, %.2f sec.)\n", finalfilename, totalnum * sizeof(SP16), (float)totalnum / (float) para.smp_freq);
  }
}
