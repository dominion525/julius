/**
 * @file   adin_mic_linux_oss.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * <JA>
 * @brief  �ޥ������� (Linux/OSS)
 *
 * �ޥ������ϤΤ�������٥�ؿ��Ǥ������󥿥ե������Ȥ��� OSS
 * ������ɥɥ饤�Ф���Ѥ����硤���Υե����뤬���Ѥ���ޤ���
 * �����ͥ�ɸ��Υɥ饤�С�OSS/Linux�Υɥ饤�С������ ALSA ��
 * OSS�ߴ��⡼�ɤ��б����Ƥ��ޤ���
 *
 * configure �ǥޥ��������פμ�ưȽ�̤�
 * �Ԥʤ����ʥǥե���ȡˡ�Linux �ǤϤ���OSS�ѥ��󥿥ե����������򤵤�ޤ���
 * ¾�Υ��󥿥ե����� (ALSA, esd, portaudio, spAudio��) ����Ѥ���������
 * configure ���� "--with-mictype=TYPE" ������Ū�˻��ꤷ�Ʋ�������
 *
 * ������ɥ����ɤ� 16bit ��Υ�� ��Ͽ���Ǥ��뤳�Ȥ�ɬ�ܤǤ���
 * ������Linux�Ǥϡ����ƥ쥪Ͽ�������Ǥ��ʤ��ǥХ����ξ�硤
 * �������ͥ�Τߤ����ϤȤ��Ƽ��Ф���ǧ�����뤳�Ȥ�Ǥ��ޤ���
 *
 * Julius��Linux�Ǥϥߥ������ǥХ������������ڹԤ��ޤ���Ͽ���ǥХ�����
 * ����ʥޥ���/�饤��ˤ�Ͽ���ܥ�塼���Ĵ��� xmixer �ʤ�¾�Υġ����
 * �ԤʤäƲ�������
 *
 * �ǥե���ȤΥǥХ���̾�� "/dev/dsp" �Ǥ����Ķ��ѿ� AUDIODEV ��
 * �ǥХ���̾����ꤹ�뤳�Ȥǡ�¾�ΥǥХ���̾����ѤǤ��ޤ���
 * </JA>
 * <EN>
 * @brief  Microphone input on Linux/OSS
 *
 * Low level I/O functions for microphone input on Linux using OSS API.
 * Works on kernel driver, OSS/Linux, ALSA OSS compatible device, and
 * other OSS compatible drivers.  This will work on kernel 2.0.x and later.
 *
 * This OSS API is used by default on Linux machines.  If you want
 * another API (like ALSA native, esd, portaudio, spAudio), please specify
 * "--with-mictype=TYPE" options at compilation time to configure script.
 *
 * Sound card should support 16bit monaural recording.  On Linux, however,
 * if you unfortunately have a device with only stereo recording support,
 * Julius will try to get input from the left channel of the stereo input.
 *
 * Julius does not alter any mixer device setting at all on Linux.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as xmixer.
 *
 * The default device name is "/dev/dsp", which can be changed by setting
 * environment variable AUDIODEV.
 * </EN>
 *
 * $Revision: 1.6 $
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
#include <sys/time.h>
#include <fcntl.h>

/* sound header */
#include <sys/soundcard.h>

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/dsp"

#define FREQALLOWRANGE 200	///< Acceptable width of sampling frequency
#define MAXPOLLINTERVAL 300	///< Read timeout in msec.
/**
 * Maximum Data fragment Length in msec.  Input can be delayed to this time.
 * a maximum of 2^x number smaller than this value will be taken.
 * You can override this value by specifying environment valuable
 * "LATENCY_MSEC".
 * 
 */
#define MAX_FRAGMENT_MSEC 50
/**
 * Minimum fragment length in bytes
 * 
 */
#define MIN_FRAGMENT_SIZE 256

static int audio_fd;		///< Audio descriptor
static boolean need_swap;	///< Whether samples need byte swap
static int frag_size;		///< Actual data fragment size
static boolean stereo_rec;	///< TRUE if stereo recording (use left only)

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
  int fmt, fmt_can, fmt1, fmt2, rfmt; /* sampling format */
  int samplerate;	/* 16kHz */
  char *defaultdev = DEFAULT_DEVICE; /* default device */
  char *devname;
  int frag;

  /* set device name */
  if ((devname = getenv("AUDIODEV")) == NULL) {
    devname = defaultdev;
  }

  /* open device */
  if ((audio_fd = open(devname, O_RDONLY|O_NONBLOCK)) == -1) {
    j_printerr("adin_mic_standby: device=%s\n", devname);
    perror("adin_mic_standby: open device");
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
    j_printerr("adin_mic_standby: device=%s\n", devname);
    perror("adin_mic_standby: open device");
    return(FALSE);
  }

  /* try to set a small fragment size to minimize delay, */
  /* although many devices use static fragment size... */
  /* (and smaller fragment causes busy buffering) */
  {
    int arg;
    int frag_msec;
    int f, f2;
    char *p;

    /* if environment variable "LATENCY_MSEC" is defined, try to set it
       as a minimum latency in msec (will be rouneded to 2^x). */
    if ((p = getenv("LATENCY_MSEC")) == NULL) {
      frag_msec = MAX_FRAGMENT_MSEC;
    } else {
      j_printerr("adin_mic_standby: get LATENCY_MSEC=%s\n", p);
      frag_msec = atoi(p);
    }
      
    /* get fragment size from MAX_FRAGMENT_MSEC and MIN_FRAGMENT_SIZE */
    f = 0;
    f2 = 1;
    while (f2 * 1000 / (sfreq * sizeof(SP16)) <= frag_msec
	   || f2 < MIN_FRAGMENT_SIZE) {
      f++;
      f2 *= 2;
    }
    frag = f - 1;

    /* set to device */
    arg = 0x7fff0000 | frag;
    if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &arg)) {
      fprintf(stderr, "adin_mic_standby: set fragment size to 2^%d=%d bytes (%d msec)\n", frag, 2 << (frag-1), (2 << (frag-1)) * 1000 / (sfreq * sizeof(SP16)));
    }
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

  {
    /* try SNDCTL_DSP_STEREO, SNDCTL_DSP_CHANNELS, monaural, stereo */
    int channels;
    int stereo;
    boolean ok_p = FALSE;

    stereo = 0;			/* mono */
    if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
      /* failed: SNDCTL_DSP_STEREO not supported */
      perror("adin_mic_standby: sndctl_dsp_stereo (mono)");
    } else {
      if (stereo != 0) {
	/* failed to set monaural recording by SNDCTL_DSP_STEREO */
	fprintf(stderr, "adin_mic_standby: failed to set monaural recording by SNDCTL_DSP_STEREO\n");
      } else {
	/* succeeded to set monaural recording by SNDCTL_DSP_STEREO */
	stereo_rec = FALSE;
	ok_p = TRUE;
      }
    }
    if (! ok_p) {		/* not setup yet */
      /* try using sndctl_dsp_channels */
      channels = 1;
      if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
	/* failed: SNDCTL_DSP_CHANNELS not supported */
	perror("adin_mic_standby: sndctl_dsp_channels (channels = 1)");
      } else {
	if (channels != 1) {
	  /* failed to set monaural recording by SNDCTL_DSP_CHANNELS */
	  fprintf(stderr, "adin_mic_standby: failed to set monaural recording by SNDCTL_DSP_CHANNELS\n");
	} else {
	  /* succeeded to set monaural recording by SNDCTL_DSP_CHANNELS */
	  fprintf(stderr, "adin_mic_standby: SNDCTL_DSP_CHANNELS used for monaural record setting\n");
	  stereo_rec = FALSE;
	  ok_p = TRUE;
	}
      }
    }
    if (! ok_p) {
      /* try using stereo input */
      fprintf(stderr, "adin_mic_standby: monaural recording failed, use stereo left channel\n");
      stereo = 1;			/* stereo */
      if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
	/* failed: SNDCTL_DSP_STEREO not supported */
	perror("adin_mic_standby: sndctl_dsp_stereo (stereo)");
      } else {
	if (stereo != 1) {
	  /* failed to set stereo recording by SNDCTL_DSP_STEREO */
	  fprintf(stderr, "adin_mic_standby: failed to set stereo recording by SNDCTL_DSP_STEREO\n");
	} else {
	  /* succeeded to set stereo recording by SNDCTL_DSP_STEREO */
	  stereo_rec = TRUE;
	  ok_p = TRUE;
	}
      }
    }
    if (! ok_p) {		/* not setup yet */
      /* try using stereo input with sndctl_dsp_channels */
      channels = 2;
      if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
	/* failed: SNDCTL_DSP_CHANNELS not supported */
	perror("adin_mic_standby: sndctl_dsp_channels (channels = 2)");
      } else {
	if (channels != 2) {
	  /* failed to set stereo recording by SNDCTL_DSP_CHANNELS */
	  fprintf(stderr, "adin_mic_standby: failed to set stereo recording by SNDCTL_DSP_CHANNELS\n");
	} else {
	  /* succeeded to set stereo recording by SNDCTL_DSP_CHANNELS */
	  fprintf(stderr, "adin_mic_standby: SNDCTL_DSP_CHANNELS used for stereo record setting\n");
	  stereo_rec = TRUE;
	  ok_p = TRUE;
	}
      }
    }
    if (! ok_p) {		/* all failed */
      fprintf(stderr,"adin_mic_standby: give up setting record channels\n");
      return FALSE;
    }
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

  /* get actual fragment size */
  if (ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) == -1) {
    fprintf(stderr, "adin_mic_standby: failed to get fragment size\n");
    return(FALSE);
  }
  j_printf("Audio I/O Latency = %d msec (data fragment = %d frames)", frag_size * 1000/ (sfreq * sizeof(SP16)), frag_size / sizeof(SP16));
  if (frag_size !=  2 << (frag-1)) {
    j_printf(" (tried = %d bytes)", 2 << (frag-1));
  }
  j_printf("\n");

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
  char buf[4];
  /* Read 1 sample (and ignore it) to tell the audio device start recording.
     (If you knows better way, teach me...) */
  if (stereo_rec) {
    read(audio_fd, buf, 4);
  } else {
    read(audio_fd, buf, 2);
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
 * samples currently available.  This function will block at most
 * MAXPOLLINTERVAL msec, until at least one sample can be obtained.
 * If no data has been obtained after waiting for MAXPOLLINTERVAL msec,
 * returns 0.
 * 
 * When stereo input, only left channel will be used.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, 0 of no sample has been captured in
 * MAXPOLLINTERVAL msec, -2 if error.
 */
int
adin_mic_read(SP16 *buf, int sampnum)
{
  int size,cnt,i;
  audio_buf_info info;
  fd_set rfds;
  struct timeval tv;
  int status;

  /* check for incoming samples in device buffer */
  /* if there is at least one sample fragment, go next */
  /* if not exist, wait for the data to come for at most MAXPOLLINTERVAL msec */
  /* if no sample fragment has come in the MAXPOLLINTERVAL period, go next */
  FD_ZERO(&rfds);
  FD_SET(audio_fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = MAXPOLLINTERVAL * 1000;
  status = select(audio_fd+1, &rfds, NULL, NULL, &tv);
  if (status < 0) {
    /* select() failed */
    j_printerr("adin_mic_read: device polling failed\n");
    return(-2);			/* error */
  }
  if (FD_ISSET(audio_fd, &rfds)) { /* has some data */
    /* get sample num that can be read without blocking */
    if (ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info) == -1) {
      perror("adin_mic_read: sndctl_dsp_getispace");
      return(-2);
    }
    /* get them as much as possible */
    size = sampnum * sizeof(SP16);
    if (size > info.bytes) size = info.bytes;
    if (size < frag_size) size = frag_size;
    size &= ~ 1;		/* Force 16bit alignment */
    cnt = read(audio_fd, buf, size);
    if ( cnt < 0 ) {
      perror("adin_mic_read: read error\n");
      return ( -2 );
    }
    cnt /= sizeof(short);

    if (stereo_rec) {
      /* remove R channel */
      for(i=1;i<cnt;i+=2) buf[(i-1)/2]=buf[i];
      cnt/=2;
    }
    
    if (need_swap) swap_sample_bytes(buf, cnt);
  } else {			/* no data after waiting */
    j_printerr("adin_mic_read: no data fragment after %d msec?\n", MAXPOLLINTERVAL);
    cnt = 0;
  }

  return(cnt);
}
