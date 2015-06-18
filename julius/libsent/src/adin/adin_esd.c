/**
 * @file   adin_esd.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * <JA>
 * @brief  ネットワーク入力：Enlightened Sound Daemon (EsounD) からの音声入力
 *
 * 入力ソースとして Enlightened Sound Daemon (EsounD, 以下 esd) を
 * 使用する低レベル関数です．
 * 使用には esd が動作していることが必要です．
 * 使用するには configure 時に "--with-mictype=esd" を指定して下さい．
 * </JA>
 * <EN>
 * @brief  Audio input from Englightened Sound Daemon (EsounD)
 *
 * Low level I/O functions for audio input via the Enlightened Sound
 * Daemon (EsounD, or esd in short).  If you want to use this API,
 * please specify "--with-mictype=esd" options at compilation time
 * to configure script.
 * </EN>
 *
 * $Revision: 1.3 $
 * 
 */
/*
 * Copyright (c) 2004-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/adin.h>

/* sound header */
#include <esd.h>

static int sock;		///< Audio socket
static char name_buf[ESD_NAME_MAX]; ///< Unique identifier of this process that will be passed to EsounD
static int latency = 50;	///< Lantency time in msec

/** 
 * Connection initialization: check connectivity and open for recording.
 * 
 * @param sfreq [in] required sampling frequency
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_standby(int sfreq, void *dummy)
{
  esd_format_t format = ESD_BITS16 | ESD_MONO | ESD_STREAM | ESD_RECORD;

  /* generate uniq ID */
  snprintf(name_buf, ESD_NAME_MAX, "julius%d", getpid());

  /* open stream */
  j_printf("opening socket, format = 0x%08x at %d Hz id=%s\n", format, sfreq, name_buf);
  sock = esd_record_stream_fallback(format, sfreq, NULL, name_buf);
  if (sock <= 0) {
    j_printerr("adin_mic_standby: failed to connect to esd\n");
    return FALSE;
  }

  return TRUE;
}
 
/** 
 * Start recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_start()
{
  return(TRUE);
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_stop()
{
  return TRUE;
}

/**
 * @brief  Read samples from the daemon.
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until
 * at least one sample can be obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if an error occured.
 */
int
adin_mic_read(SP16 *buf, int sampnum)
{
  int size, cnt;

  size = sampnum;
  if (size > ESD_BUF_SIZE) size = ESD_BUF_SIZE;
  size *= sizeof(SP16);

  while((cnt = read(sock, buf, size)) <= 0) {
    if (cnt < 0) {
      perror("adin_mic_read: read error\n");
      return ( -2 );
    }
    usleep(latency * 1000);
  }
  cnt /= sizeof(SP16);
  return(cnt);
}
