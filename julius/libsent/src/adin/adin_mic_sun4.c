/**
 * @file   adin_mic_sun4.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 18:56:13 2005
 * 
 * <JA>
 * @brief  �ޥ������� (Sun4)
 *
 * SunOS 4.x �ǥޥ������Ϥ���Ѥ��뤿������٥벻�����ϴؿ��Ǥ���
 * SunOS 4.x �Υޥ���Ǥϥǥե���ȤǤ��Υե����뤬���Ѥ���ޤ���
 *
 * Sun SunOS 4.1.3 ��ư���ǧ�򤷤Ƥ��ޤ���Solaris2.x �ˤĤ��Ƥ�
 * adin_mic_sol2.c �������������
 *
 * ��ư�奪���ǥ������Ϥϥޥ����˼�ưŪ���ڤ��ؤ�ꡤ�ܥ�塼���
 * J_DEF_VOLUME ���ͤ����ꤵ��ޤ���
 *
 * �ǥե���ȤΥǥХ���̾�� "/dev/audio" �Ǥ����Ķ��ѿ� AUDIODEV ��
 * �ǥХ���̾����ꤹ�뤳�Ȥǡ�¾�ΥǥХ���̾����ѤǤ��ޤ���
 * </JA>
 * <EN>
 * @brief  Microphone input on Sun4
 *
 * Low level I/O functions for microphone input on SunOS 4.x machines.
 * This file is used as default on SunOS 4.x machines.
 *
 * Tested on SunOS 4.1.3.
 *
 * The microphone input device will be automatically selected by Julius
 * on startup, and volume will be set to J_DEF_VOLUME.
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

#define J_DEF_VOLUME 20		///< Recording volume (range=0-99)

#include <sent/stddefs.h>
#include <sent/adin.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stropts.h>
#include <poll.h>

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/audio"
/// Default volume
static int volume = J_DEF_VOLUME;

/* sound header */
#include <multimedia/libaudio.h>/* see man audio_device(3) */
#include <multimedia/audio_device.h>
static int afd;			///< Audio file descriptor
static struct pollfd pfd;	///< File descriptor for polling
static audio_info_t ainfo;	///< Audio info

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
  char *defaultdev = DEFAULT_DEVICE;
  char *devname;
  Audio_hdr Dev_hdr, old_hdr;
  double vol;

  /* get device name if specified in $AUDIODEV */
  if ((devname = getenv("AUDIODEV")) == NULL) {
    devname = defaultdev;
  }

  /* open the device */
  if ((afd = open(devname, O_RDONLY)) == -1) {
    if (errno == EBUSY) {
      j_printerr("audio device %s is busy\n", devname);
      return(FALSE);
    } else {
      j_printerr("unable to open %s\n",devname);
      return(FALSE);
    }
  }

  /* set recording port to microphone */
  AUDIO_INITINFO(&ainfo);
  ainfo.record.port = AUDIO_MICROPHONE;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    perror("Audio_set_info");
    return(FALSE);
  }

  /* set recording parameters */
  if (audio_get_record_config(afd, &Dev_hdr) != AUDIO_SUCCESS) {
    j_printerr("get_config error\n"); return(FALSE);
  }
  Dev_hdr.sample_rate = sfreq;
  Dev_hdr.samples_per_unit = 1; /* ? I don't know this param. ? */
  Dev_hdr.bytes_per_unit = 2;
  Dev_hdr.channels = 1;
  Dev_hdr.encoding = AUDIO_ENCODING_LINEAR;
  if (audio_set_record_config(afd, &Dev_hdr) != AUDIO_SUCCESS) {
    j_printerr("set_config error\n"); return(FALSE);
  }

  /* set volume */
  vol = (float)volume / (float)100;
  if (audio_set_record_gain(afd, &vol) != AUDIO_SUCCESS) {
    j_printerr("cannot set record volume\n");
    return(FALSE);
  }

  /* flush buffer */
  if((ioctl(afd , I_FLUSH , FLUSHRW)) == -1) {
    j_printerr("cannot flush input buffer\n");
    return(FALSE);
  }
  
  /* setup polling */
  pfd.fd = afd;
  pfd.events = POLLIN;

  /* pause transfer */
  if (audio_pause_record(afd) == AUDIO_ERR_NOEFFECT) {
    j_printerr("cannot pause audio\n");
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
  /* resume input */
  if (audio_resume_record(afd) == AUDIO_ERR_NOEFFECT) {
    j_printerr("cannot resume audio\n");
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
  /* pause input */
  if (audio_pause_record(afd) == AUDIO_ERR_NOEFFECT) {
    j_printerr("cannot pause audio\n");
    return(FALSE);
  }
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
  int bytes;
  int len;

  /* SunOS4.x needs special dealing when no samples are found */
  len = sampnum * sizeof(SP16);
  bytes = 0;
  while(bytes < len) {
    bytes = read(afd, buf, len);
    if (bytes < 0) {
      if (errno != EAGAIN) {	/* error */
	perror("adin_mic_read: failed to read sample");
	return(-2);
      } else {			/* retry */
	poll(&pfd, 1L, -1);
      }
    }
  }
  if (bytes < 0) {
    perror("adin_mic_read: failed to read sample");
    return(-2);
  }
  return(bytes / sizeof(SP16)); /* success */
}
