/**
 * @file   adin_portaudio.c
 * @author Akinobu LEE
 * @date   Mon Feb 14 12:03:48 2005
 * 
 * <JA>
 * @brief  �ޥ������� (portaudio�饤�֥��)
 *
 * portaudioo�饤�֥�����Ѥ����ޥ������ϤΤ�������٥�ؿ��Ǥ���
 * ���Ѥ���ˤ� configure ���� "--with-mictype=portaudio" ����ꤷ�Ʋ�������
 * Linux ����� Win32 �ǻ��Ѳ�ǽ�Ǥ���Win32�⡼�ɤǤϤ��줬
 * �ǥե���ȤȤʤ�ޤ���
 *
 * Julius�ϥߥ������ǥХ������������ڹԤ��ޤ���Ͽ���ǥХ�����
 * ����ʥޥ���/�饤��ˤ�Ͽ���ܥ�塼���Ĵ���Windows��
 * �֥ܥ�塼�ॳ��ȥ���� �� Linux �� xmixer �ʤɡ�¾�Υġ����
 * �ԤʤäƲ�������
 *
 * portaudio �ϥե꡼�ǥ����ץ�åȥۡ���Υ����ǥ��������ϥ饤�֥��
 * �Ǥ����������� libsent/src/adin/pa/ �˴ޤޤ�Ƥ��ޤ������Υץ����Ǥ�
 * ����åɤ����Ѥ���callback �����Ѥ������ϲ������󥰥Хåե��˼������
 * ���ޤ���
 *
 * @sa http://www.portaudio.com/
 * </JA>
 * <EN>
 * @brief  Microphone input using portaudio library
 *
 * Low level I/O functions for microphone input using portaudio library.
 * To use, please specify "--with-mictype=portaudio" options
 * to configure script.  This function is currently available for Linux and
 * Win32.  On Windows, this is default.
 *
 * Julius does not alter any mixer device setting at all.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as xmixer on Linux, or
 * 'Volume Control' on Windows.
 *
 * Portaudio is a free, cross platform, open-source audio I/O library.
 * The sources are included at libsent/src/adin/pa/.  This program uses
 * ring buffer to store captured samples in callback functions with threading.
 *
 * @sa http://www.portaudio.com/
 * </EN>
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 2004-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>

/* sound header */
#include "pa/portaudio.h"


#undef DDEBUG

/**
 * Maximum Data fragment Length in msec.  Input can be delayed to this time.
 * You can override this value by specifying environment valuable
 * "LATENCY_MSEC".
 * 
 */
#define MAX_FRAGMENT_MSEC 128

/* temporal buffer */
static SP16 *speech;		///< cycle buffer for incoming speech data
static int current;		///< writing point
static int processed;		///< reading point
static boolean buffer_overflowed = FALSE; ///< TRUE if buffer overflowed
static int cycle_buffer_len;	///< length of cycle buffer based on INPUT_DELAY_SEC

/** 
 * PortAudio callback to store the incoming speech data into the cycle
 * buffer.
 * 
 * @param inbuf [in] portaudio input buffer 
 * @param outbuf [in] portaudio output buffer (not used)
 * @param len [in] length of above
 * @param outTime [in] output time (not used)
 * @param userdata [in] user defined data (not used)
 * 
 * @return 0 when no error, or 1 to terminate recording.
 */
static int
Callback(void *inbuf, void *outbuf, unsigned long len, PaTimestamp outTime, void *userdata)
{
  SP16 *now;
  int avail;
  int processed_local;
  int written;

  now = inbuf;

  processed_local = processed;

#ifdef DDEBUG
  printf("callback-1: processed=%d, current=%d: recordlen=%d\n", processed_local, current, len);
#endif

  /* check overflow */
  if (processed_local > current) {
    avail = processed_local - current;
  } else {
    avail = cycle_buffer_len + processed_local - current;
  }
  if (len > avail) {
#ifdef DDEBUG
    printf("callback-*: buffer overflow!\n");
#endif
    buffer_overflowed = TRUE;
    len = avail;
  }

  /* store to buffer */
  if (current + len <= cycle_buffer_len) {
    memcpy(&(speech[current]), now, len * sizeof(SP16));
#ifdef DDEBUG
    printf("callback-2: [%d..%d] %d samples written\n", current, current+len, len);
#endif
  } else {
    written = cycle_buffer_len - current;
    memcpy(&(speech[current]), now, written * sizeof(SP16));
#ifdef DDEBUG
    printf("callback-2-1: [%d..%d] %d samples written\n", current, current+written, written);
#endif
    memcpy(&(speech[0]), &(now[written]), (len - written) * sizeof(SP16));
#ifdef DDEBUG
    printf("callback-2-2: ->[%d..%d] %d samples written (total %d samples)\n", 0, len-written, len-written, len);
#endif
  }
  current += len;
  if (current >= cycle_buffer_len) current -= cycle_buffer_len;
#ifdef DDEBUG
  printf("callback-3: new current: %d\n", current);
#endif

  return(0);
}

static PortAudioStream *stream;		///< Stream information

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
  PaError err;
  int frames_per_buffer, num_buffer;
  int latency;
  char *p;

  /* set cycle buffer length */
  cycle_buffer_len = INPUT_DELAY_SEC * sfreq;
  j_printerr("Audio cycle buffer length: %d bytes\n", cycle_buffer_len * sizeof(SP16));

  /* for safety... */
  if (sizeof(SP16) != paInt16) {
    j_error("SP16 != paInt16\n");
  }

  /* allocate and init */
  current = processed = 0;
  speech = (SP16 *)mymalloc(sizeof(SP16) * cycle_buffer_len);
  buffer_overflowed = FALSE;

  /* set buffer parameter*/
  if ((p = getenv("LATENCY_MSEC")) != NULL) {
    j_printerr("adin_mic_standby: get LATENCY_MSEC=%s\n", p);
    latency = atoi(p);
  } else {
    latency = MAX_FRAGMENT_MSEC;
  }
  frames_per_buffer = 256;
  num_buffer = sfreq * latency / (frames_per_buffer * 1000);
  j_printf("framesPerBuffer=%d, NumBuffers(guess)=%d (%d)\n",
	   frames_per_buffer, num_buffer, 
	   Pa_GetMinNumBuffers(frames_per_buffer, sfreq));
  j_printf("Audio I/O Latency = %d msec (data fragment = %d frames)\n",
	   (frames_per_buffer * num_buffer) * 1000 / sfreq, 
	   (frames_per_buffer * num_buffer));

  /* initialize device and open stream */
  err = Pa_Initialize();
  if (err != paNoError) {
    j_printerr("Error: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }

  err = Pa_OpenDefaultStream(&stream, 1, 0, paInt16, sfreq, 
			     frames_per_buffer, num_buffer, 
			     Callback, NULL);
  if (err != paNoError) {
    j_printerr("Error: %s\n", Pa_GetErrorText(err));
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
  PaError err;

  /* start stream */
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    j_printerr("Error: %s\n", Pa_GetErrorText(err));
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
  PaError err;

  /* stop stream */
  err = Pa_StopStream(stream);
  if (err != paNoError) {
    j_printerr("Error: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }
  
  return TRUE;
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
  int current_local;
  int avail;
  int len;

  if (buffer_overflowed) {
    j_printerr("Error: input buffer OVERFLOW, increase INPUT_DELAY_SEC in sent/speech.h\n");
    buffer_overflowed = FALSE;
  }

  while (current == processed) {
#ifdef DDEBUG
    printf("process  : current == processed: %d: wait\n", current);
#endif
    Pa_Sleep(MAX_FRAGMENT_MSEC); /* wait till some input comes */
  }

  current_local = current;

#ifdef DDEBUG
  printf("process-1: processed=%d, current=%d\n", processed, current_local);
#endif

  if (processed < current_local) {
    avail = current_local - processed;
    if (avail > sampnum) avail = sampnum;
    memcpy(buf, &(speech[processed]), avail * sizeof(SP16));
#ifdef DDEBUG
    printf("process-2: [%d..%d] %d samples read\n", processed, processed+avail, avail);
#endif
    len = avail;
    processed += avail;
  } else {
    avail = cycle_buffer_len - processed;
    if (avail > sampnum) avail = sampnum;
    memcpy(buf, &(speech[processed]), avail * sizeof(SP16));
#ifdef DDEBUG
    printf("process-2-1: [%d..%d] %d samples read\n", processed, processed+avail, avail);
#endif
    len = avail;
    processed += avail;
    if (processed >= cycle_buffer_len) processed -= cycle_buffer_len;
    if (sampnum - avail > 0) {
      if (sampnum - avail < current_local) {
	avail = sampnum - avail;
      } else {
	avail = current_local;
      }
      if (avail > 0) {
	memcpy(&(buf[len]), &(speech[0]), avail * sizeof(SP16));
#ifdef DDEBUG
	printf("process-2-2: [%d..%d] %d samples read (total %d)\n", 0, avail, avail, len + avail);
#endif
	len += avail;
	processed += avail;
	if (processed >= cycle_buffer_len) processed -= cycle_buffer_len;
      }
    }
  }
#ifdef DDEBUG
  printf("process-3: new processed: %d\n", processed);
#endif
  return len;
}

