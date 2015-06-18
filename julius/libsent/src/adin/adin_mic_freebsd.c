/**
 * @file   adin_mic_freebsd.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * <JA>
 * @brief  マイク入力 (FreeBSD)
 *
 * マイク入力のための低レベル関数です．FreeBSDでこのファイルが使用されます．
 *
 * サウンドカードが 16bit モノラル で録音できることが必須です．
 * 
 * JuliusはFreeBSDでミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節は他のツールで
 * 行なって下さい．
 *
 * デフォルトのデバイス名は "/dev/dsp" です．環境変数 AUDIODEV に
 * デバイス名を指定することで，他のデバイス名を使用できます．
 *
 * 動作確認はFreeBSD 3.2-RELEASE で行なわれました．サウンドドライバは
 * snd を使用しています．
 * </JA>
 * <EN>
 * @brief  Microphone input on FreeBSD
 *
 * Low level I/O functions for microphone input on FreeBSD.
 *
 * To use microphone input in FreeBSD, sound card and sound driver must
 * support 16bit monaural recording.
 *
 * Julius does not alter any mixer device setting at all.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool.
 *
 * The default device name is "/dev/dsp", which can be changed by setting
 * environment variable AUDIODEV.
 *
 * Tested on FreeBSD 3.2-RELEASE with snd driver.
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

/* Thanks to Kentaro Nagatomo for information */
/* All functions are the same as OSS version, except the header filename */

#include <sent/stddefs.h>
#include <sent/adin.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

/* sound header */
#include <machine/soundcard.h>

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/dsp"

static int audio_fd;		///< Audio descriptor
static boolean need_swap;	///< Whether input samples need byte-swapping
struct pollfd fds[1];		///< Workarea for polling device

#define FREQALLOWRANGE 200	///< Acceptable width of sampling frequency
#define POLLINTERVAL 200	///< Polling interval in miliseconds

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
  int fmt, fmt_can, fmt1, fmt2, rfmt; /* sampling format */
  int samplerate;		/* actual sampling rate */
  int stereo;		/* mono */
  char *defaultdev = DEFAULT_DEVICE; /* default device */
  char *devname;

  /* set device name */
  if ((devname = getenv("AUDIODEV")) == NULL) {
    devname = defaultdev;
  }
  
  /* open device */
  if ((audio_fd = open(devname, O_RDONLY)) == -1) {
    perror("adin_mic_standby: open");
    return(FALSE);
  }

  /* check whether soundcard can record 16bit data */
  /* and set fmt */
  if (ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &fmt_can) == -1) {
    perror("adin_mic_standby: sndctl_dsp_getfmts");
    return(FALSE);
  }
#ifdef WORDS_BIGENDIAN
  fmt1 = AFMT_S16_BE;
  fmt2 = AFMT_S16_LE;
#else
  fmt1 = AFMT_S16_LE;               /* 16bit signed (little endian) */
  fmt2 = AFMT_S16_BE;               /* (big endian) */
#endif /* WORDS_BIGENDIAN */
  /* fmt2 needs byte swap */
  if (fmt_can & fmt1) {
    fmt = fmt1;
    need_swap = FALSE;
  } else if (fmt_can & fmt2) {
    fmt = fmt2;
    need_swap = TRUE;
  } else {
    fprintf(stderr, "adin_mic_standby: 16bit recording not supported\n");
    return FALSE;
  }
#ifdef DEBUG
  if (need_swap) {
    j_printf("samples need swap\n");
  } else {
    j_printf("samples need not swap\n");
  }
#endif
  
  if (close(audio_fd) != 0) return FALSE;

  /* re-open for recording */
  /* open device */
  if ((audio_fd = open(devname, O_RDONLY)) == -1) {
    perror("adin_mic_standby: open");
    return(FALSE);
  }
  /* set format, samplerate, channels */
  rfmt = fmt;
  if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &rfmt) == -1) {
    perror("adin_mic_standby: sndctl_dsp_setfmt");
    return(FALSE);
  }
  if (rfmt != fmt) {
    fprintf(stderr, "adin_mic_standby: 16bit recording not supported\n");
    return FALSE;
  }

  stereo = 0;			/* mono */
  if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
    perror("adin_mic_standby: sndctl_dsp_stereo (mono)");
    return(FALSE);
  }
  if (stereo != 0) {
    fprintf(stderr,"adin_mic_standby: monoral recording not supported\n");
    return FALSE;
  }

  samplerate = sfreq;
  if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &samplerate) == -1) {
    fprintf(stderr, "adin_mic_standby: sndctl_dsp_speed (%dHz)\n", sfreq);
    return(FALSE);
  }
  if (samplerate < sfreq - FREQALLOWRANGE || samplerate > sfreq + FREQALLOWRANGE) {
    fprintf(stderr,"adin_mic_standby: couldn't set sampling rate to near %dHz. (%d)\n", sfreq, samplerate);
    return FALSE;
  }
  if (samplerate != sfreq) {
    fprintf(stderr,"adin_mic_standby: set sampling rate to %dHz\n", samplerate);
  }

  /* set polling status */
  fds[0].fd = audio_fd;
  fds[0].events = POLLIN;
  
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
  char buf[2];

  /* Read 1 sample (and ignore it) to tell the audio device start recording.
     (If you knows better way, teach me...) */
  read(audio_fd, buf, 2);
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
  /*
   * Not reset device on each end of speech, just let the buffer overrun...
   * Resetting and restarting of recording device sometimes causes
   * hawling noises at the next recording.
   * I don't now why, so take the easy way... :-(
   */
  return TRUE;
}

/**
 * @brief  Read samples from device
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until at least one
 * sample can be obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if error.
 */
int
adin_mic_read(SP16 *buf, int sampnum)
{
  int size,cnt;
  audio_buf_info info;

  /* wait till at least one sample can be read */
  poll(fds, 1, POLLINTERVAL);
  /* get actual sample num in the device buffer */
  if (ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info) == -1) {
    perror("adin_mic_read: sndctl_dsp_getispace");
    return(-2);
  }
  
  /* get them as much as possible */
  size = sampnum * sizeof(SP16);
  if (size > info.bytes) size = info.bytes;
  cnt = read(audio_fd, buf, size);
  if ( cnt < 0 ) {
    perror("adin_mic_read: read error\n");
    return ( -2 );
  }
  cnt /= sizeof(short);
  if (need_swap) swap_sample_bytes(buf, cnt);
  return(cnt);
}
