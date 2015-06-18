/**
 * @file   adin_mic_linux_alsa.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * <JA>
 * @brief  マイク入力 (Linux/ALSA)
 *
 * ALSA API を使用する，マイク入力のための低レベル関数です．
 * 使用には ALSA サウンドドライバーがインストールされていることが必要です．
 * 使用するには configure 時に "--with-mictype=alsa" を指定して下さい．
 *
 * サウンドカードが 16bit モノラル で録音できることが必須です．
 *
 * JuliusはLinuxではミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節は alsamixer など他のツールで
 * 行なって下さい．
 *
 * 複数サウンドカードはサポートされていません．複数のサウンドカードが
 * インストールされている場合，最初の１つが用いられます．
 * </JA>
 * <EN>
 * @brief  Microphone input on Linux/ALSA
 *
 * Low level I/O functions for microphone input on Linux using
 * Advanced Linux Sound Architechture (ALSA) API, developed on version 0.9.x.
 * If you want to use this API, please specify
 * "--with-mictype=alsa" options at compilation time to configure script.
 *
 * Julius does not alter any mixer device setting at all on Linux.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as alsamixer.
 *
 * Note that sound card should support 16bit monaural recording, and multiple
 * cards are not supported (in that case the first one will be used).
 * </EN>
 *
 * @sa http://www.alsa-project.org/
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
#include <sent/adin.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <alsa/asoundlib.h>

static snd_pcm_t *handle;	///< Audio handler
static snd_pcm_hw_params_t *hwparams; ///< Pointer to device hardware parameters
static char *pcm_name = "hw:0,0"; ///< Name of the PCM device

static boolean need_swap;	///< Whether samples need byte swap
static int latency = 32;	///< Lantency time in msec.  You can override this value by specifying environment valuable "LATENCY_MSEC".

static struct pollfd *ufds;	///< Poll descriptor
static int count;		///< Poll descriptor count


/** 
 * Device initialization: check device capability and open for recording.
 * 
 * @param sfreq [in] required sampling frequency.
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_standby(int sfreq, void *dummy)
{
  int err;
#if (SND_LIB_MAJOR == 0)
  int actual_rate;		/* sample rate returned by hardware */
#else
  unsigned int actual_rate;		/* sample rate returned by hardware */
#endif
  int dir;			/* comparison result of exact rate and given rate */

  /* allocate hwparam structure */
  snd_pcm_hw_params_alloca(&hwparams);

  /* open device (for resource test, open in non-block mode) */
  if ((err = snd_pcm_open(&handle, pcm_name, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
    j_printerr("Error: cannot open PCM device %s (%s)\n", pcm_name, snd_strerror(err));
    return(FALSE);
  }
  
  /* set device to non-block mode */
  if ((err = snd_pcm_nonblock(handle, 0)) < 0) {
    j_printerr("Error: cannot set PCM device to block mode\n");
    return(FALSE);
  }

  /* initialize hwparam structure */
  if ((err = snd_pcm_hw_params_any(handle, hwparams)) < 0) {
    j_printerr("Error: cannot initialize PCM device parameter structure (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  /* set interleaved read/write format */
  if ((err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    j_printerr("Error: cannot set PCM device access mode (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  /* set sample format */
#ifdef WORDS_BIGENDIAN
  /* try big endian, then little endian with byte swap */
  if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_BE)) >= 0) {
    need_swap = FALSE;
  } else if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE)) >= 0) {
    need_swap = TRUE;
  } else {
    j_printerr("Error: cannot set PCM device format to 16bit-signed (%s)\n", snd_strerror(err));
    return(FALSE);
  }
#else  /* LITTLE ENDIAN */
  /* try little endian, then big endian with byte swap */
  if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE)) >= 0) {
    need_swap = FALSE;
  } else if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_BE)) >= 0) {
    need_swap = TRUE;
  } else {
    j_printerr("Error: cannot set PCM device format to 16bit-signed (%s)\n", snd_strerror(err));
    return(FALSE);
  }
#endif
  
  /* set sample rate (if the exact rate is not supported by the hardware, use nearest possible rate */
#if (SND_LIB_MAJOR == 0)
  actual_rate = snd_pcm_hw_params_set_rate_near(handle, hwparams, sfreq, &dir);
  if (actual_rate < 0) {
    j_printerr("Error: cannot set PCM device sample rate to %d (%s)\n", sfreq, snd_strerror(actual_rate));
    return(FALSE);
  }
#else
  actual_rate = sfreq;
  err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &actual_rate, &dir);
  if (err < 0) {
    j_printerr("Error: cannot set PCM device sample rate to %d (%s)\n", sfreq, snd_strerror(err));
    return(FALSE);
  }
#endif
  if (actual_rate != sfreq) {
    j_printerr("Warning: the rate %d Hz is not supported by your PCM hardware.\n         ==> Using %d Hz instead.\n", sfreq, actual_rate);
  }

  /* set number of channels */
  {
    int minchannels;
    snd_pcm_hw_params_get_channels_min(hwparams, &minchannels);
    if (minchannels > 1) {
      j_printerr("Error: monoral recording not supported on this device/driver\n");
      return(FALSE);
    }
  }
  
  if ((err = snd_pcm_hw_params_set_channels(handle, hwparams, 1)) < 0) {
    j_printerr("Error: cannot set PCM channel to %d (%s)\n", 1, snd_strerror(err));
    return(FALSE);
  }

  /* set period size */
  {
#if (SND_LIB_MAJOR == 0)
    int periodsize;		/* period size (bytes) */
    int actual_size;
    int maxsize, minsize;
#else
    snd_pcm_uframes_t periodsize;		/* period size (bytes) */
    snd_pcm_uframes_t actual_size;
    snd_pcm_uframes_t maxsize, minsize;
#endif
    char *p;
    
    /* get hardware max/min size */
    dir = 0;
#if (SND_LIB_MAJOR == 0)
    if ((maxsize = snd_pcm_hw_params_get_period_size_max(hwparams, &dir)) < 0) {
      j_printerr("Error: cannot get maximum period size\n");
      return(FALSE);
    }
    if ((minsize = snd_pcm_hw_params_get_period_size_min(hwparams, &dir)) < 0) {
      j_printerr("Error: cannot get minimum period size\n");
      return(FALSE);
    }
#else    
    if ((err = snd_pcm_hw_params_get_period_size_max(hwparams, &maxsize, &dir)) < 0) {
      j_printerr("Error: cannot get maximum period size\n");
      return(FALSE);
    }
    if ((err = snd_pcm_hw_params_get_period_size_min(hwparams, &minsize, &dir)) < 0) {
      j_printerr("Error: cannot get minimum period size\n");
      return(FALSE);
    }
#endif

    /* set apropriate period size */
    if ((p = getenv("LATENCY_MSEC")) != NULL) {
      j_printerr("adin_mic_standby: get LATENCY_MSEC=%s\n", p);
      latency = atoi(p);
    }

    periodsize = actual_rate * latency / 1000 * sizeof(SP16);
    if (periodsize < minsize) {
      j_printerr("Warning: PCM latency of %d ms (%d bytes) too small, use device minimum %d bytes\n", latency, periodsize, minsize);
      periodsize = minsize;
    } else if (periodsize > maxsize) {
      j_printerr("Warning: PCM latency of %d ms (%d bytes) too large, use device maximum %d bytes\n", latency, periodsize, maxsize);
      periodsize = maxsize;
    }
    
    /* set size (near value will be used) */
#if (SND_LIB_MAJOR == 0)
    actual_size = snd_pcm_hw_params_set_period_size_near(handle, hwparams, periodsize, &dir);
    if (actual_size < 0) {
      j_printerr("Error: cannot set PCM record period size to %d (%s)\n", periodsize, snd_strerror(actual_size));
      return(FALSE);
    }
#else
    actual_size = periodsize;
    err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &actual_size, &dir);
    if (err < 0) {
      j_printerr("Error: cannot set PCM record period size to %d (%s)\n", periodsize, snd_strerror(err));
      return(FALSE);
    }
#endif
    if (actual_size != periodsize) {
      j_printerr("Warning: PCM period size: %d bytes (%d ms) -> %d bytes\n", periodsize, latency, actual_size);
    }
    j_printf("Audio I/O Latency = %d msec (data fragment = %d frames)\n", actual_size * 1000 / (actual_rate * sizeof(SP16)), actual_size / sizeof(SP16));
      
    /* set number of periods ( = 2) */
    if ((err = snd_pcm_hw_params_set_periods(handle, hwparams, sizeof(SP16), 0)) < 0) {
      j_printerr("Error: cannot set PCM number of periods to %d (%s)\n", sizeof(SP16), snd_strerror(err));
      return(FALSE);
    }
  }

  /* apply the configuration to the PCM device */
  if ((err = snd_pcm_hw_params(handle, hwparams)) < 0) {
    j_printerr("Error: cannot set PCM hardware parameters (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  /* prepare for recording */
  if ((err = snd_pcm_prepare(handle)) < 0) {
    j_printerr("Error: cannot prepare audio interface (%s)\n", snd_strerror(err));
  }

  /* prepare for polling */
  count = snd_pcm_poll_descriptors_count(handle);
  if (count <= 0) {
    j_printerr("Error: invalid PCM poll descriptors count\n");
    return(FALSE);
  }
  ufds = mymalloc(sizeof(struct pollfd) * count);

  if ((err = snd_pcm_poll_descriptors(handle, ufds, count)) < 0) {
    j_printerr("Error: unable to obtain poll descriptors for PCM recording (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  return(TRUE);
}

/** 
 * Error recovery when PCM buffer underrun or suspend.
 * 
 * @param handle [in] audio handler
 * @param err [in] error code
 * 
 * @return 0 on success, otherwise the given errno.
 */
static int
xrun_recovery(snd_pcm_t *handle, int err)
{
  if (err == -EPIPE) {    /* under-run */
    err = snd_pcm_prepare(handle);
    if (err < 0)
      j_printerr("Can't recovery from PCM buffer underrun, prepare failed: %s\n", snd_strerror(err));
    return 0;
  } else if (err == -ESTRPIPE) {
    while ((err = snd_pcm_resume(handle)) == -EAGAIN)
      sleep(1);       /* wait until the suspend flag is released */
    if (err < 0) {
      err = snd_pcm_prepare(handle);
      if (err < 0)
	j_printerr("Can't recovery from PCM buffer suspend, prepare failed: %s\n", snd_strerror(err));
    }
    return 0;
  }
  return err;
}

/** 
 * Start recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_start()
{
  int err;
  snd_pcm_state_t status;

  /* check hardware status */
  while(1) {			/* wait till prepared */
    status = snd_pcm_state(handle);
    switch(status) {
    case SND_PCM_STATE_PREPARED: /* prepared for operation */
      if ((err = snd_pcm_start(handle)) < 0) {
	j_printerr("Error: cannot start PCM (%s)\n", snd_strerror(err));
	return (FALSE);
      }
      return(TRUE);
      break;
    case SND_PCM_STATE_RUNNING:	/* capturing the samples of other application */
      if ((err = snd_pcm_drop(handle)) < 0) { /* discard the existing samples */
	j_printerr("Error: cannot drop PCM (%s)\n", snd_strerror(err));
	return (FALSE);
      }
      break;
    case SND_PCM_STATE_XRUN:	/* buffer overrun */
      if ((err = xrun_recovery(handle, -EPIPE)) < 0) {
	j_printerr("Error: PCM XRUN recovery failed (%s)\n", snd_strerror(err));
	return(FALSE);
      }
      break;
    case SND_PCM_STATE_SUSPENDED:	/* suspended by power management system */
      if ((err = xrun_recovery(handle, -ESTRPIPE)) < 0) {
	j_printerr("Error: PCM XRUN recovery failed (%s)\n", snd_strerror(err));
	return(FALSE);
      }
      break;
    }
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
  return(TRUE);
}

/**
 * @brief  Read samples from device
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
  int cnt;
  snd_pcm_sframes_t avail;

  while ((avail = snd_pcm_avail_update(handle)) <= 0) {
    usleep(latency * 1000);
  }
  if (avail < sampnum) {
    cnt = snd_pcm_readi(handle, buf, avail);
  } else {
    cnt = snd_pcm_readi(handle, buf, sampnum);
  }

  if (cnt < 0) {
    j_printerr("Error: PCM read failed (%s)\n", snd_strerror(cnt));
    return(-2);
  }

  if (need_swap) {
    swap_sample_bytes(buf, cnt);
  }
  return(cnt);
}
