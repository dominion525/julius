/**
 * @file   adin_mic_darwin_coreaudio.c
 * @author Masatomo Hashimoto
 * @date   Wed Oct 12 11:31:27 2005
 * 
 * <JA>
 * @brief  adin microphone library for CoreAudio API
 *
 * by Masatomo Hashimoto <m.hashimoto@aist.go.jp>
 *
 * Tested on Mac OS X v10.3.9 and v10.4.1
 *
 * このプログラムは，
 * 独立行政法人 産業技術総合研究所 情報技術研究部門
 * ユビキタスソフトウェアグループ
 * より提供されました．
 * </JA>
 * 
 * <EN>
 * @brief  adin microphone library for CoreAudio API
 *
 * by Masatomo Hashimoto <m.hashimoto@aist.go.jp>
 *
 * Tested on Mac OS X v10.3.9 and v10.4.1
 *
 * This file has been contributed from the Ubiquitous Software Group,
 * Information Technology Research Institute, AIST.
 * 
 * </EN>
 * 
 * $Revision: 1.1.1.1 $
 * 
 */

/*
 * adin_mic_darwin_coreaudio.c
 *
 * adin microphone library for CoreAudio API
 *
 * by Masatomo Hashimoto <m.hashimoto@aist.go.jp>
 *
 * Tested on Mac OS X v10.3.9 and v10.4.1
 *
 */

/* $Id: adin_mic_darwin_coreaudio.c,v 1.1.1.1 2005/11/17 11:11:49 sumomo Exp $ */

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioOutputUnit.h>
#include <AudioToolbox/AudioConverter.h>
#include <pthread.h>
#include <stdio.h>

#define DEVICE_NAME_LEN 128
#define BUF_SAMPLES 4096

static UInt32 ConvQuality = kAudioConverterQuality_Medium;

typedef SInt16 Sample;
static UInt32 BytesPerSample = sizeof(Sample);

#define BITS_PER_BYTE 8

static AudioDeviceID InputDeviceID;
static AudioUnit InputUnit;
static AudioConverterRef Converter;

static pthread_mutex_t MutexInput;
static pthread_cond_t CondInput;

static bool CoreAudioRecordStarted = FALSE;
static bool CoreAudioHasInputDevice = FALSE;
static bool CoreAudioInit = FALSE;

static UInt32 NumSamplesAvailable = 0;

static UInt32 InputDeviceBufferSamples = 0;
static UInt32 InputBytesPerPacket = 0;
static UInt32 InputFramesPerPacket = 0;
static UInt32 InputSamplesPerPacket = 0;
static UInt32 OutputBitsPerChannel = 0;
static UInt32 OutputBytesPerPacket = 0;
static UInt32 OutputSamplesPerPacket = 0;

static AudioBufferList* BufList;
static AudioBufferList BufListBackup;
static AudioBufferList* BufListConverted;


static void printStreamInfo(AudioStreamBasicDescription* desc) {
  j_printf("----- details of stream -----\n");
  j_printf("sample rate: %f\n", desc->mSampleRate);
  j_printf("format flags: %s%s%s%s%s%s%s\n", 
	   desc->mFormatFlags & kAudioFormatFlagIsFloat ? 
	   "[float]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsBigEndian ? 
	   "[big endian]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsSignedInteger ? 
	   "[signed integer]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsPacked ? 
	   "[packed]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsAlignedHigh ? 
	   "[aligned high]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsNonInterleaved ? 
	   "[non interleaved]" : "",
	   desc->mFormatFlags & kAudioFormatFlagsAreAllClear ? 
	   "[all clear]" : ""
	   );
  j_printf("bytes per packet: %d\n", desc->mBytesPerPacket);
  j_printf("frames per packet: %d\n", desc->mFramesPerPacket);
  j_printf("bytes per frame: %d\n", desc->mBytesPerFrame);
  j_printf("channels per frame: %d\n", desc->mChannelsPerFrame);
  j_printf("bits per channel: %d\n", desc->mBitsPerChannel);
  j_printf("-----------------------------------\n");
}

static void printAudioBuffer(AudioBuffer* buf) {
  int sz = buf->mDataByteSize / BytesPerSample;
  int i;
  Sample* p = (Sample*)(buf->mData);
  for (i = 0; i < sz; i++) {
    j_printf("%d ", p[i]);
  }
}

static AudioBufferList* 
allocateAudioBufferList(UInt32 data_bytes, UInt32 nsamples, UInt32 nchan) {

  AudioBufferList* bufl;

#ifdef DEBUG
  j_printf("allocateAudioBufferList: data_bytes:%d nsamples:%d nchan:%d\n",
	   data_bytes, nsamples, nchan);
#endif

  bufl = (AudioBufferList*)(malloc(sizeof(AudioBufferList)));

  if(bufl == NULL) {
    j_printerr("allocateAudioBufferList: failed\n");
    exit(1);
  }

  bufl->mNumberBuffers = nchan;

  int i;
  for (i = 0; i < nchan; i++) {
    bufl->mBuffers[i].mNumberChannels = nchan;
    bufl->mBuffers[i].mDataByteSize = data_bytes * nsamples;
    bufl->mBuffers[i].mData = malloc(data_bytes * nsamples);
    
    if(bufl->mBuffers[i].mData == NULL) {
      j_printerr("allocateAudioBufferList: malloc for mBuffers[%d] failed\n", i);
      exit(1);
    }
  }
  return bufl;
}

/* gives input data for Converter */
static OSStatus 
ConvInputProc(AudioConverterRef inConv,
	      UInt32* ioNumDataPackets,
	      AudioBufferList* ioData, // to be filled
	      AudioStreamPacketDescription** outDataPacketDesc,
	      void* inUserData)
{
  int i;
  UInt32 nPacketsRequired = *ioNumDataPackets;
  UInt32 nBytesProvided = 0;
  UInt32 nBytesRequired;
  UInt32 n;
  
  pthread_mutex_lock(&MutexInput);

#ifdef DEBUG
  j_printf("ConvInputProc: required %d packets\n", nPacketsRequired);
#endif

  while(NumSamplesAvailable == 0){
    pthread_cond_wait(&CondInput, &MutexInput);
  }

  for(i = 0; i < BufList->mNumberBuffers; i++) {
    n = BufList->mBuffers[i].mDataByteSize;
    if (nBytesProvided != 0 && nBytesProvided != n) {
      j_printerr("WARNING: buffer size mismatch\n");
    }
    nBytesProvided = n;
  }

#ifdef DEBUG
  j_printf("ConvInputProc: %d bytes in buffer\n", nBytesProvided);
#endif

  for(i = 0; i < BufList->mNumberBuffers; i++) {
    ioData->mBuffers[i].mNumberChannels = 
      BufList->mBuffers[i].mNumberChannels;

    nBytesRequired = nPacketsRequired * InputBytesPerPacket;

    if(nBytesRequired < nBytesProvided) {
      ioData->mBuffers[i].mData = BufList->mBuffers[i].mData;
      ioData->mBuffers[i].mDataByteSize = nBytesRequired;
      BufList->mBuffers[i].mData += nBytesRequired;
      BufList->mBuffers[i].mDataByteSize = nBytesProvided - nBytesRequired;
    } else {
      ioData->mBuffers[i].mData = BufList->mBuffers[i].mData;
      ioData->mBuffers[i].mDataByteSize = nBytesProvided;
      
      BufList->mBuffers[i].mData = BufListBackup.mBuffers[i].mData;

    }

  }

  *ioNumDataPackets = ioData->mBuffers[0].mDataByteSize / InputBytesPerPacket;

#ifdef DEBUG
  j_printf("ConvInputProc: provided %d packets\n", *ioNumDataPackets);
#endif

  NumSamplesAvailable = 
    nBytesProvided / BytesPerSample - *ioNumDataPackets * InputSamplesPerPacket;

#ifdef DEBUG
  j_printf("ConvInputProc: %d samples available\n", NumSamplesAvailable);
#endif

  pthread_mutex_unlock(&MutexInput);

  return noErr;
}


/* called when input data are available (an AURenderCallback) */
static OSStatus 
InputProc(void* inRefCon,
	  AudioUnitRenderActionFlags* ioActionFlags,
	  const AudioTimeStamp* inTimeStamp,
	  UInt32 inBusNumber,
	  UInt32 inNumberFrames,
	  AudioBufferList* ioData // null
	  )
{
  OSStatus status = noErr;
  int i;

  pthread_mutex_lock(&MutexInput);

  if (NumSamplesAvailable == 0) {

    status = AudioUnitRender(InputUnit,
			     ioActionFlags,
			     inTimeStamp,
			     inBusNumber,
			     inNumberFrames,
			     BufList);
    NumSamplesAvailable = 
      BufList->mBuffers[0].mDataByteSize / InputBytesPerPacket;

#ifdef DEBUG
    printAudioBuffer(BufList->mBuffers);
#endif
  }

  pthread_mutex_unlock(&MutexInput);
  
  pthread_cond_signal(&CondInput);

  /*
  j_printf("InputProc: %d bytes filled (BufList)\n", 
	  BufList->mBuffers[0].mDataByteSize);
  */

  return status;
}


/* initialize default sound device */
bool adin_mic_standby(int sfreq, void* dummy) {
  OSStatus status;
  UInt32 propertySize;
  char deviceName[DEVICE_NAME_LEN];
  struct AudioStreamBasicDescription inDesc;
  int err;

  j_printf("adin_mic_standby: sample rate = %d\n", sfreq);

  if (CoreAudioInit) 
    return TRUE;

  Component halout;
  ComponentDescription haloutDesc;

  haloutDesc.componentType = kAudioUnitType_Output;
  haloutDesc.componentSubType = kAudioUnitSubType_HALOutput;
  haloutDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
  haloutDesc.componentFlags = 0;
  haloutDesc.componentFlagsMask = 0;
  halout = FindNextComponent(NULL, &haloutDesc);

  if(halout == NULL) {
    j_printerr("no HALOutput component found\n");
    return FALSE;
  }

  OpenAComponent(halout, &InputUnit);

  UInt32 enableIO;
  
  enableIO = 1;
  status = AudioUnitSetProperty(InputUnit, 
				kAudioOutputUnitProperty_EnableIO,
				kAudioUnitScope_Input,
				1,
				&enableIO,
				sizeof(enableIO));
    if (status != noErr) {
      j_printerr("cannot set InputUnit's EnableIO(Input)\n");
      return FALSE;
    }

  enableIO = 0;
  status = AudioUnitSetProperty(InputUnit, 
				kAudioOutputUnitProperty_EnableIO,
				kAudioUnitScope_Output,
				0,
				&enableIO,
				sizeof(enableIO));
    if (status != noErr) {
      j_printerr("cannot set InputUnit's EnableIO(Output)\n");
      return FALSE;
    }


  /* get default input device */
  propertySize = sizeof(InputDeviceID);
  status = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,
	                            &propertySize,
				    &InputDeviceID);
  if (status != noErr) {
    j_printerr("cannot get default input device\n");
    return FALSE;
  }

  if (InputDeviceID == kAudioDeviceUnknown) {
    j_printerr("no available input device found\n");
    return FALSE;

  } else {

    CoreAudioHasInputDevice = TRUE;

    /* get input device name */
    propertySize = sizeof(char) * DEVICE_NAME_LEN;
    status = AudioDeviceGetProperty(InputDeviceID,
				    1,
				    1,
				    kAudioDevicePropertyDeviceName,
				    &propertySize,
				    deviceName);
    if (status != noErr) {
      j_printerr("cannot get device name\n");
      return FALSE;
    }

    status = AudioUnitSetProperty(InputUnit,
				  kAudioOutputUnitProperty_CurrentDevice,
				  kAudioUnitScope_Global,
				  0,
				  &InputDeviceID,
				  sizeof(InputDeviceID));

    if (status != noErr) {
      j_printerr("cannot bind default input device to AudioUnit\n");
      return FALSE;
    }

    /* get input device's format */
    propertySize = sizeof(inDesc);
    status = AudioDeviceGetProperty(InputDeviceID,
				    1,
				    1,
				    kAudioDevicePropertyStreamFormat,
				    &propertySize,
				    &inDesc);
    if (status != noErr) {
      j_printerr("cannot get input device's stream format\n");
      return FALSE;
    }

    /* get input device's buffer frame size */
    UInt32 bufferFrameSize;
    propertySize = sizeof(bufferFrameSize);
    status = AudioDeviceGetProperty(InputDeviceID,
				    1,
				    1,
				    kAudioDevicePropertyBufferFrameSize,
				    &propertySize,
				    &bufferFrameSize);
    if (status != noErr) {
      j_printerr("cannot get input device's buffer frame size\n");
      return FALSE;
    }

    j_printf("using device \"%s\" for input\n", deviceName);
    j_printf("\tsample rate %f\n\t%ld channels\n\t%ld-bit sample\n",
	    inDesc.mSampleRate,
	    inDesc.mChannelsPerFrame,
	    inDesc.mBitsPerChannel);

    j_printf("\t%d buffer frames\n", bufferFrameSize);


    printStreamInfo(&inDesc);

    UInt32 formatFlagEndian = 
      inDesc.mFormatFlags & kAudioFormatFlagIsBigEndian;

    inDesc.mFormatFlags = 
      kAudioFormatFlagIsSignedInteger | 
      kAudioFormatFlagIsPacked | 
      formatFlagEndian;

    inDesc.mBytesPerPacket = BytesPerSample;
    inDesc.mFramesPerPacket = 1;
    inDesc.mBytesPerFrame = BytesPerSample;
    inDesc.mChannelsPerFrame = 1;
    inDesc.mBitsPerChannel = BytesPerSample * BITS_PER_BYTE;

    printStreamInfo(&inDesc);

    propertySize = sizeof(inDesc);
    status = AudioUnitSetProperty(InputUnit, 
				  kAudioUnitProperty_StreamFormat,
				  kAudioUnitScope_Output,
				  1,
				  &inDesc,
				  propertySize
				  );
    if (status != noErr) {
      j_printerr("cannot set InputUnit's stream format\n");
      return FALSE;
    }

    InputBytesPerPacket = inDesc.mBytesPerPacket;
    InputFramesPerPacket = inDesc.mFramesPerPacket;
    InputSamplesPerPacket = InputBytesPerPacket / BytesPerSample;

    InputDeviceBufferSamples = 
      bufferFrameSize * InputSamplesPerPacket * InputFramesPerPacket;

    j_printf("input device's buffer size (# of samples): %d\n", 
	     InputDeviceBufferSamples);

    AudioStreamBasicDescription outDesc;
    outDesc.mSampleRate = sfreq;
    outDesc.mFormatID = kAudioFormatLinearPCM;
    outDesc.mFormatFlags = 
      kAudioFormatFlagIsSignedInteger | 
      kAudioFormatFlagIsPacked | 
      formatFlagEndian;
    outDesc.mBytesPerPacket = BytesPerSample;
    outDesc.mFramesPerPacket = 1;
    outDesc.mBytesPerFrame = BytesPerSample;
    outDesc.mChannelsPerFrame = 1;
    outDesc.mBitsPerChannel = BytesPerSample * BITS_PER_BYTE;

    printStreamInfo(&outDesc);

    OutputBitsPerChannel = outDesc.mBitsPerChannel;
    OutputBytesPerPacket = outDesc.mBytesPerPacket;

    OutputSamplesPerPacket = (OutputBitsPerChannel / BITS_PER_BYTE) / OutputBytesPerPacket;

    status = AudioConverterNew(&inDesc, &outDesc, &Converter);
    if (status != noErr){
      j_printerr("cannot create audio converter\n");
      exit(1);
    }

    /*
    UInt32 nChan = inDesc.mChannelsPerFrame;
    int i;

    if (inDesc.mFormatFlags & kAudioFormatFlagIsNonInterleaved && nChan > 1) {
      UInt32 chmap[nChan];
      for (i = 0; i < nChan; i++)
	chmap[i] = 0;

      status = AudioConverterSetProperty(Converter, 
					 kAudioConverterChannelMap,
					 sizeof(chmap), chmap);
      if (status != noErr){
	j_printerr("cannot set audio converter's channel map\n");
	exit(1);
      }
    }
    */

    status = 
      AudioConverterSetProperty(Converter, 
				kAudioConverterSampleRateConverterQuality,
				sizeof(ConvQuality), &ConvQuality);
    if (status != noErr){
      j_printerr("cannot set audio converter quality\n");
      exit(1);
    }


    //j_printf("audio converter generated\n");

    /* allocate buffers */
    BufList = allocateAudioBufferList(inDesc.mBitsPerChannel / BITS_PER_BYTE, 
				      InputDeviceBufferSamples, 1);

    BufListBackup.mNumberBuffers = BufList->mNumberBuffers;

    BufListBackup.mBuffers[0].mNumberChannels = 1;
    BufListBackup.mBuffers[0].mDataByteSize = 
      BufList->mBuffers[0].mDataByteSize;
    BufListBackup.mBuffers[0].mData = BufList->mBuffers[0].mData;

    BufListConverted = allocateAudioBufferList(BytesPerSample, BUF_SAMPLES, 1);
    //j_printf("buffers allocated\n");

    err = pthread_mutex_init(&MutexInput, NULL);
    if (err) {
      j_printerr("cannot init mutex\n");
      return FALSE;
    }
    err = pthread_cond_init(&CondInput, NULL);
    if (err) {
      j_printerr("cannot init condition variable\n");
      return FALSE;
    }

    /* register the callback */
    AURenderCallbackStruct input;
    input.inputProc = InputProc; // an AURenderCallback
    input.inputProcRefCon = 0;
    AudioUnitSetProperty(InputUnit,
			 kAudioOutputUnitProperty_SetInputCallback,
			 kAudioUnitScope_Global,
			 0,
			 &input,
			 sizeof(input));

    status = AudioUnitInitialize(InputUnit);
    if (status != noErr){
      j_printerr("InputUnit initialize failed\n");
      exit(1);
    }

  }

  CoreAudioInit = TRUE;

  j_printf("CoreAudio: initialized\n");

  return TRUE;
}

bool adin_mic_start(){ return TRUE; }
bool adin_mic_stop(){ return TRUE; }

int adin_mic_read(void *buffer, int nsamples) {
  OSStatus status;

#ifdef DEBUG
  j_printf("adin_mic_read: %d samples required\n", nsamples);
#endif

  if (!CoreAudioHasInputDevice) 
    return -1;

  if (!CoreAudioRecordStarted) {
    status = AudioOutputUnitStart(InputUnit);
    CoreAudioRecordStarted = TRUE;
  }
  
  UInt32 capacity = BUF_SAMPLES * OutputSamplesPerPacket;
  UInt32 npackets = nsamples * OutputSamplesPerPacket;

  UInt32 numDataPacketsNeeded;

  Sample* inputDataBuf = (Sample*)(BufListConverted->mBuffers[0].mData);

  numDataPacketsNeeded = npackets < capacity ? npackets : capacity;

#ifdef DEBUG
  j_printf("adin_mic_read: numDataPacketsNeeded=%d\n", numDataPacketsNeeded);
#endif

  status = AudioConverterFillComplexBuffer(Converter, 
					   ConvInputProc, 
					   NULL, // user data
					   &numDataPacketsNeeded, 
					   BufListConverted, 
					   NULL // packet description
					   );
  if (status != noErr) {
    j_printerr("AudioConverterFillComplexBuffer: failed\n");
    return -1;
  }

#ifdef DEBUG
  j_printf("adin_mic_read: %d bytes filled (BufListConverted)\n", 
	   BufListConverted->mBuffers[0].mDataByteSize);
#endif

  int providedSamples = numDataPacketsNeeded / OutputSamplesPerPacket;

  pthread_mutex_lock(&MutexInput);

#ifdef DEBUG
  j_printf("adin_mic_read: provided samples: %d\n", providedSamples);
#endif

  Sample* dst_data = (Sample*)buffer;

  int i;

  int count = 0;

  for (i = 0; i < providedSamples; i++) {
    dst_data[i] = inputDataBuf[i];
    if (dst_data[i] == 0) count++;
  }

  //j_printf("%d zero samples\n", count);


  pthread_mutex_unlock(&MutexInput);

#ifdef DEBUG
  j_printf("adin_mic_read: EXIT: %d samples provided\n", providedSamples);
#endif

  return providedSamples;
}

void adin_mic_pause() {
  OSStatus status;

  if (CoreAudioHasInputDevice && CoreAudioRecordStarted) {
    status = AudioOutputUnitStop(InputUnit);
    CoreAudioRecordStarted = FALSE;
  }
  return;
}
