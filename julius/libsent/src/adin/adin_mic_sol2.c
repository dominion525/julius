/**
 * @file   adin_mic_sol2.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 19:06:46 2005
 * 
 * <JA>
 * @brief  マイク入力 (Solaris2.x)
 *
 * Solaris 2.x でマイク入力を使用するための低レベル音声入力関数です．
 * Solaris 2.x のマシンではデフォルトでこのファイルが使用されます．
 *
 * Sun Solaris 2.5.1 および 2.6 で動作確認をしています．
 * ビッグエンディアンを前提としているため，Solaris x86 では動きません．
 *
 * 起動後オーディオ入力はマイクに自動的に切り替わりますが，
 * ボリュームは自動調節されません．gaintoolなどで別途調節してください． 
 *
 * デフォルトのデバイス名は "/dev/audio" です．環境変数 AUDIODEV に
 * デバイス名を指定することで，他のデバイス名を使用できます．
 * </JA>
 * <EN>
 * @brief  Microphone input on Solaris 2.x
 *
 * Low level I/O functions for microphone input on Solaris 2.x machines.
 * This file is used as default on Solaris 2.x machines.
 *
 * Tested on Sun Solaris 2.5.1 and 2.6.  Also works on later versions.
 * Please note that this will not work on Solaris x86, since machine
 * byte order is fixed to big endian.
 *
 * The microphone input device will be automatically selected by Julius
 * on startup.  Please note that the recoding volue will not be
 * altered by Julius, and appropriate value should be set by another tool
 * such as gaintool.
 * 
 * The default device name is "/dev/audio", which can be changed by setting
 * environment variable AUDIODEV.
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
#include <sent/adin.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stropts.h>

/* sound header */
#include <sys/audioio.h>
static int afd;			///< Audio file descriptor
static struct audio_info ainfo;	///< Audio format information

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/audio"

/** 
 * Device initialization: check device capability and open for recording.
 * 
 * @param sfreq [in] required sampling frequency.
 * @param arg [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_standby(int sfreq, void *arg)
{
  char *defaultdev = DEFAULT_DEVICE;
  char *devname;

  /* get device name if specified in $AUDIODEV */
  if ((devname = getenv("AUDIODEV")) == NULL) {
    devname = defaultdev;
  }

  /* open the device */
  if ((afd = open(devname, O_RDONLY)) == -1) {
    perror("adin_mic_standby: open audio device");
    return(FALSE);
  }

#if 0
  {
    /* output hardware info (debug) */
    struct audio_device adev;
    if (ioctl(afd, AUDIO_GETDEV, &adev)== -1) {
      perror("adin_mic_standby: AUDIO_GETDEV");
      return(FALSE);
    }
    j_printf("Hardware name: %s\n",adev.name);
    j_printf("Hardware version: %s\n", adev.version);
    j_printf("Properties: %s\n", adev.config);
  }
#endif

  /* get current setting */
  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    perror("adin_mic_standby: AUDIO_GETINFO");
    return(FALSE);
  }
  /* pause for changing setting */
  ainfo.record.pause = 1;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    perror("adin_mic_standby: AUDIO_SETINFO");
    return(FALSE);
  }
  /* flush current input buffer (in old format) */
  if((ioctl(afd , I_FLUSH , FLUSHR)) == -1) {
    perror("adin_mic_standby: I_FLUSH");
    return(FALSE);
  }
  /* set record setting */
  ainfo.record.sample_rate = sfreq;
  ainfo.record.channels = 1;
  ainfo.record.precision = 16;
  ainfo.record.encoding = AUDIO_ENCODING_LINEAR;
  /* ainfo.record.gain = J_DEF_VOLUME * (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN) / 100 + AUDIO_MIN_GAIN; */
  ainfo.record.port = AUDIO_MICROPHONE;
  /* recording should be paused when initialized */
  ainfo.record.pause = 1;

  /* set audio setting, remain pause */
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    perror("adin_mic_standby: AUDIO_SETINFO");
    return(FALSE);
  }

  return(TRUE);

}

/** 
 * Start recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_start()
{
  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    perror("adin_mic_start: AUDIO_GETINFO");
    return(FALSE);
  }
  ainfo.record.pause = 0;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    perror("adin_mic_start: AUDIO_SETINFO");
    return(FALSE);
  }

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
  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    perror("adin_mic_stop: AUDIO_GETINFO");
    return(FALSE);
  }
  ainfo.record.pause = 1;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    perror("adin_mic_stop: AUDIO_SETINFO");
    return(FALSE);
  }
  return(TRUE);
}

/**
 * @brief  Read samples from device
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until
 * at least some samples are obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if an error occured.
 */
int
adin_mic_read(SP16 *buf, int sampnum)
{
  int cnt;
  cnt = read(afd, buf, sampnum * sizeof(SP16)) / sizeof(SP16);
  if (cnt < 0) {
    perror("adin_mic_read: failed to read sample\n");
    return(-2);
  }
  return(cnt);
}
