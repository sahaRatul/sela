/*
 *
 *  ao_macosx.c
 *
 *      Original Copyright (C) Timothy J. Wood - Aug 2000
 *       Modifications (C) Michael Guntsche - March 2008
 *                         Monty - Feb 2010
 *
 *  This file is part of libao, a cross-platform library.  See
 *  README for a history of this source code.
 *
 *  libao is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  libao is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ********************************************************************

 last mod: $Id$

 ********************************************************************/
/* The MacOS X CoreAudio framework doesn't mesh as simply as some
   simpler frameworks do.  This is due to the fact that CoreAudio pulls
   audio samples rather than having them pushed at it (which is nice
   when you are wanting to do good buffering of audio).  */

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUComponent.h>
#include <stdio.h>
#include <pthread.h>

#include "ao/ao.h"
#include "ao/plugin.h"

#define DEFAULT_BUFFER_TIME (250);

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define true  1
#define false 0

static char *ao_macosx_options[] = {"matrix","verbose","quiet","debug","buffer_time","dev"};

static ao_info ao_macosx_info =
{
	AO_TYPE_LIVE,
	"MacOS X AUHAL output",
	"macosx",
	"Monty <monty@xiph.org>",
	"",
	AO_FMT_NATIVE,
	30,
	ao_macosx_options,
	sizeof(ao_macosx_options)/sizeof(*ao_macosx_options)
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct ao_macosx_internal
{
  /* Stuff describing the CoreAudio device */
  AudioDeviceID                outputDevice;
  ComponentInstance            outputAudioUnit;
  int                          output_p;

  /* Keep track of whether the output stream has actually been
     started/stopped */
  Boolean                      started;
  Boolean                      isStopping;

  /* Our internal queue of samples waiting to be consumed by
     CoreAudio */
  void                        *buffer;
  unsigned int                 bufferByteCount;
  unsigned int                 firstValidByteOffset;
  unsigned int                 validByteCount;

  unsigned int                 buffer_time;

  /* Need access in the cllbacks for messaging */
  ao_device                   *device;
} ao_macosx_internal;

static OSStatus audioCallback (void *inRefCon, 
			       AudioUnitRenderActionFlags *inActionFlags,
			       const AudioTimeStamp *inTimeStamp, 
			       UInt32 inBusNumber, 
			       UInt32 inNumberFrames,
			       AudioBufferList *ioData)
{
  OSStatus err = noErr;
  ao_macosx_internal *internal = (ao_macosx_internal *)inRefCon;
  unsigned int validByteCount;
  unsigned int totalBytesToCopy;
  ao_device *device = internal->device;

  /* Despite the audio buffer list, playback render can only submit a
     single buffer. */

  if(!ioData){
    aerror("Unexpected number of buffers (NULL)\n");
    return 0;
  }

  if(ioData->mNumberBuffers != 1){
    aerror("Unexpected number of buffers (%d)\n",
	   (int)ioData->mNumberBuffers);
    return 0;
  }

  totalBytesToCopy = ioData->mBuffers[0].mDataByteSize;

  pthread_mutex_lock(&mutex);

  validByteCount = internal->validByteCount;

  if (validByteCount < totalBytesToCopy && !internal->isStopping) {
    /* Not enough data ... let it build up a bit more before we start
       copying stuff over. If we are stopping, of course, we should
       just copy whatever we have. This also happens if an application
       pauses output. */
    *inActionFlags = kAudioUnitRenderAction_OutputIsSilence;
    memset(ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);
    pthread_mutex_unlock(&mutex);
    return 0;
  }

  {
    unsigned char *outBuffer = ioData->mBuffers[0].mData;
    unsigned int outBufSize = ioData->mBuffers[0].mDataByteSize;
    unsigned int bytesToCopy = MIN(outBufSize, validByteCount);
    unsigned int firstFrag = bytesToCopy;
    unsigned char *sample = internal->buffer + internal->firstValidByteOffset;

    /* Check if we have a wrap around in the ring buffer If yes then
       find out how many bytes we have */
    if (internal->firstValidByteOffset + bytesToCopy > internal->bufferByteCount) 
      firstFrag = internal->bufferByteCount - internal->firstValidByteOffset;

    /* If we have a wraparound first copy the remaining bytes off the end
       and then the rest from the beginning of the ringbuffer */
    if (firstFrag < bytesToCopy) {
      memcpy(outBuffer,sample,firstFrag);
      memcpy(outBuffer+firstFrag,internal->buffer,bytesToCopy-firstFrag);
    } else {
      memcpy(outBuffer,sample,bytesToCopy);
    }
    if(bytesToCopy < outBufSize) /* the stopping case */
      memset(outBuffer+bytesToCopy, 0, outBufSize-bytesToCopy);

    internal->validByteCount -= bytesToCopy;
    internal->firstValidByteOffset = (internal->firstValidByteOffset + bytesToCopy) % 
      internal->bufferByteCount;
  }

  pthread_mutex_unlock(&mutex);
  pthread_cond_signal(&cond);

  return err;
}

int ao_plugin_test()
{
  /* This plugin will only build on a 10.4 or later Mac (Darwin 8+);
     if it built, default AUHAL is available. */
  return 1; /* This plugin works in default mode */

}

ao_info *ao_plugin_driver_info(void)
{
  return &ao_macosx_info;
}


int ao_plugin_device_init(ao_device *device)
{
  ao_macosx_internal *internal;

  internal = (ao_macosx_internal *) calloc(1,sizeof(ao_macosx_internal));

  if (internal == NULL)	
    return 0; /* Could not initialize device memory */

  internal->buffer_time = DEFAULT_BUFFER_TIME;
  internal->started = false;
  internal->isStopping = false;

  internal->device = device;
  device->internal = internal;
  device->output_matrix_order = AO_OUTPUT_MATRIX_COLLAPSIBLE;
  device->output_matrix = strdup("L,R,C,LFE,BL,BR,CL,CR,BC,SL,SR");

  return 1; /* Memory alloc successful */
}

static void lowercasestr(char *str)
{
  while (*str) {
    if ('A' <= *str && *str <= 'Z')
      *str += 'a' - 'A';
    ++str;
  }
}

static char *cfstringdupe(CFStringRef cfstr)
{
  CFIndex maxlen;
  char *cstr;

  if (!cfstr)
    return NULL;
  maxlen = 1+CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr),kCFStringEncodingUTF8);
  cstr = (char *) malloc(maxlen);
  if (!cstr)
    return NULL;
  if (!CFStringGetCString(cfstr, cstr, maxlen, kCFStringEncodingUTF8)) {
    free(cstr);
    return NULL;
  }
  return cstr;
}

static int isAudioOutputDevice(AudioDeviceID aid)
{
  if (aid != kAudioObjectUnknown) {
    AudioObjectPropertyAddress theAddress = {
      kAudioDevicePropertyDeviceCanBeDefaultDevice,
      kAudioDevicePropertyScopeOutput,
      kAudioObjectPropertyElementMaster
    };
    UInt32 val;
    UInt32 size = sizeof(val);
    OSStatus err;

    err = AudioObjectGetPropertyData(aid, &theAddress, 0, NULL, &size, &val);
    if (!err && val)
      return 1;
  }
  return 0;
}

static AudioDeviceID findAudioOutputDevice(const char *name)
{
  OSStatus err;
  AudioDeviceID aid;
  AudioObjectPropertyAddress propertyAddress;
  propertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
  propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAddress.mElement = kAudioObjectPropertyElementMaster;

  /* First see if it's a valid device UID */
  {
    CFStringRef namestr;
    AudioValueTranslation avt = {&namestr, sizeof(namestr), &aid, sizeof(aid)};
    UInt32 size = sizeof(avt);

    namestr = CFStringCreateWithCStringNoCopy(NULL, name, kCFStringEncodingUTF8,
                                              kCFAllocatorNull);
    if (!namestr)
      return kAudioObjectUnknown;
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, &avt);
    CFRelease(namestr);
    if (!err && aid != kAudioObjectUnknown)
      return isAudioOutputDevice(aid) ? aid : kAudioObjectUnknown;
  }

  /* otherwise fall back to a device name search */
  {
    AudioDeviceID *devices;
    UInt32 size;
    size_t count, i, match, matches;
    char *lcname = strdup(name);

    if (!lcname)
      return kAudioObjectUnknown; /* no memory */
    lowercasestr(lcname);
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, devices);
    if (err) {
      free(lcname);
      return kAudioObjectUnknown;
    }
    devices = (AudioDeviceID *) malloc(size);
    if (!devices) {
      free(lcname);
      return kAudioObjectUnknown; /* no memory */
    }
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, devices);
    if (err) {
      free(lcname);
      free(devices);
      return kAudioObjectUnknown; /* no device list */
    }
    count = size / sizeof(AudioDeviceID);
    match = 0;
    matches = 0;
    for (i = 0; i < count; ++i) {
      AudioObjectPropertyAddress theAddress = {
        kAudioObjectPropertyName,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
      };
      CFStringRef devstr;
      UInt32 srcnum;
      UInt32 size = sizeof(devstr);
      char *devname;
      char *srcname = NULL;

      if (!isAudioOutputDevice(devices[i]))
        continue;
      err = AudioObjectGetPropertyData(devices[i], &theAddress, 0, NULL, &size, &devstr);
      if (err || devstr == NULL)
        continue;
      devname = cfstringdupe(devstr);
      CFRelease(devstr);
      if (!devname)
        continue;
      lowercasestr(devname);
      if (!strcmp(lcname,devname)) {
        /* exact match always wins */
        match = i;
        matches = 1;
        free(devname);
        break;
      }
      /* Check the source name too */
      size = sizeof(srcnum);
      AudioObjectPropertyAddress address = { kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeInput, 0 };
      err = AudioObjectGetPropertyData(devices[i], &address, 0, NULL, &size, &srcnum);
      if (!err) {
        CFStringRef srcstr;
        AudioValueTranslation avt = {&srcnum, sizeof(srcnum), &srcstr, sizeof(srcstr)};
        size = sizeof(avt);
        err = AudioObjectGetPropertyData(devices[i], &address, 0, NULL, &size, &avt);
        if (!err && srcstr) {
          srcname = cfstringdupe(srcstr);
          CFRelease(srcstr);
          if (srcname)
            lowercasestr(srcname);
        }
      }
      if (srcname && !strcmp(lcname,srcname)) {
        /* exact match always wins */
        match = i;
        matches = 1;
        free(srcname);
        free(devname);
        break;
      }
      /* tally partial matches */
      if (strstr(devname,lcname) || (srcname && strstr(srcname,lcname))) {
        match = i;
        ++matches;
      }
      free(devname);
      if (srcname)
        free(srcname);
    }
    if (matches == 1)
      aid = devices[match];
    else
      aid = kAudioObjectUnknown;
    free(lcname);
    free(devices);
    return aid;
  }
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
  {
  ao_macosx_internal *internal = (ao_macosx_internal *) device->internal;
  int buffer;

  if (!strcmp(key,"buffer_time")) {
    buffer = atoi(value);
    if (buffer < 100) {
      awarn("Buffer time clipped to 100ms\n");
      buffer = 100;
    }
    internal->buffer_time = buffer;
  } else if (!strcmp(key,"dev")) {
    if (!value || !value[0]) {
      /* permit switching back to default device with empty string */
      internal->outputDevice = kAudioObjectUnknown;
    } else {
      internal->outputDevice = findAudioOutputDevice(value);
      if (internal->outputDevice == kAudioObjectUnknown)
        return 0;
    }
  }

  return 1;
}

int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
  ao_macosx_internal *internal = (ao_macosx_internal *) device->internal;
  OSStatus result = noErr;
  AudioComponent comp;
  AudioComponentDescription desc;
  AudioStreamBasicDescription requestedDesc;
  AURenderCallbackStruct      input;
  UInt32 i_param_size, requestedEndian;

  /* Locate the default output audio unit */
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_HALOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  comp = AudioComponentFindNext (NULL, &desc);
  if (comp == NULL) {
    aerror("Failed to start CoreAudio: AudioComponentFindNext returned NULL");
    return 0;
  }

  /* Open & initialize the default output audio unit */
  result = AudioComponentInstanceNew (comp, &internal->outputAudioUnit);
  if (result) {
    aerror("AudioComponentInstanceNew() error => %d\n",(int)result);
    return 0;
  }
  /* Set the desired output device if not default */
  if (internal->outputDevice != kAudioObjectUnknown) {
    result = AudioUnitSetProperty (internal->outputAudioUnit,
                                   kAudioOutputUnitProperty_CurrentDevice,
                                   kAudioUnitScope_Global,
                                   0,
                                   &internal->outputDevice,
                                   sizeof(internal->outputDevice));
    if (result) {
      aerror("AudioComponentSetDevice() error => %d\n",(int)result);
      AudioComponentInstanceDispose(internal->outputAudioUnit);
      return 0;
    }
  }
  internal->output_p=1;

  /* Request desired format of the audio unit.  Let HAL do all
     conversion since it will probably be doing some internal
     conversion anyway. */

  device->driver_byte_format = format->byte_format;
  requestedDesc.mFormatID = kAudioFormatLinearPCM;
  requestedDesc.mFormatFlags = kAudioFormatFlagIsPacked;
  switch(format->byte_format){
  case AO_FMT_BIG:
    requestedDesc.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    break;
  case AO_FMT_NATIVE:
    if(ao_is_big_endian())
      requestedDesc.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    break;
  }
  requestedEndian = requestedDesc.mFormatFlags & kAudioFormatFlagIsBigEndian;
  if (format->bits > 8)
    requestedDesc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;

  requestedDesc.mChannelsPerFrame = device->output_channels;
  requestedDesc.mSampleRate = format->rate;
  requestedDesc.mBitsPerChannel = format->bits;
  requestedDesc.mFramesPerPacket = 1;
  requestedDesc.mBytesPerFrame = requestedDesc.mBitsPerChannel * requestedDesc.mChannelsPerFrame / 8;
  requestedDesc.mBytesPerPacket = requestedDesc.mBytesPerFrame * requestedDesc.mFramesPerPacket;

  result = AudioUnitSetProperty (internal->outputAudioUnit,
				 kAudioUnitProperty_StreamFormat,
				 kAudioUnitScope_Input,
				 0,
				 &requestedDesc,
				 sizeof(requestedDesc));

  if (result) {
    aerror("AudioUnitSetProperty error => %d\n",(int)result);
    return 0;
  }

  /* what format did we actually get? */
  i_param_size = sizeof(requestedDesc);
  result = AudioUnitGetProperty(internal->outputAudioUnit,
			     kAudioUnitProperty_StreamFormat,
			     kAudioUnitScope_Input,
			     0,
			     &requestedDesc,
			     &i_param_size );
  if (result) {
    aerror("Failed to query modified device hardware settings => %d\n",(int)result);
    return 0;
  }

  /* If any major settings differ, abort */
  if(fabs(requestedDesc.mSampleRate-format->rate) > format->rate*.05){
    aerror("Unable to set output sample rate\n");
    return 0;
  }
  if(requestedDesc.mChannelsPerFrame != device->output_channels){
    aerror("Could not configure %d channel output\n",device->output_channels);
    return 0;
  }
  if(requestedDesc.mBitsPerChannel != format->bits){
    aerror("Could not configure %d bit output\n",format->bits);
    return 0;
  }
  if(requestedDesc.mBitsPerChannel != format->bits){
    aerror("Could not configure %d bit output\n",format->bits);
    return 0;
  }
  if(requestedDesc.mFormatFlags & kAudioFormatFlagIsFloat){
    aerror("Could not configure integer sample output\n");
    return 0;
  }
  if((requestedDesc.mFormatFlags & kAudioFormatFlagsNativeEndian) !=
     requestedEndian){
    aerror("Could not configure output endianness\n");
    return 0;
  }

  if (format->bits > 8){
    if(!(requestedDesc.mFormatFlags & kAudioFormatFlagIsSignedInteger)){
      aerror("Could not configure signed output\n");
      return 0;
    }
  }else{
    if((requestedDesc.mFormatFlags & kAudioFormatFlagIsSignedInteger)){
      aerror("Could not configure unsigned output\n");
      return 0;
    }
  }
  if(requestedDesc.mSampleRate != format->rate){
    awarn("Could not set sample rate to exactly %d; using %g instead.\n",
	  format->rate,(double)requestedDesc.mSampleRate);
  }

  /* set the channel mapping.  MacOSX AUHAL is capable of mapping any
     channel format currently representable in the libao matrix. */

  if(device->output_mask){
    AudioChannelLayout layout;
    memset(&layout,0,sizeof(layout));

    layout.mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelBitmap;
    layout.mChannelBitmap = device->output_mask;

    result = AudioUnitSetProperty(internal->outputAudioUnit,
				  kAudioUnitProperty_AudioChannelLayout,
				  kAudioUnitScope_Input, 0, &layout, sizeof(layout));
    if (result) {
      aerror("Failed to set audio channel layout => %d\n", (int)result);
    }
  }

  /* Set the audio callback */
  input.inputProc = (AURenderCallback) audioCallback;
  input.inputProcRefCon = internal;

  result = AudioUnitSetProperty( internal->outputAudioUnit,
				 kAudioUnitProperty_SetRenderCallback,
				 kAudioUnitScope_Input,
				 0, &input, sizeof( input ));


  if (result) {
    aerror("Callback set error => %d\n",(int)result);
    return 0;
  }

  result = AudioUnitInitialize (internal->outputAudioUnit);
  if (result) {
    aerror("AudioUnitInitialize() error => %d\n",(int)result);
    return 0;
  }

  /* Since we don't know how big to make the buffer until we open the device
     we allocate the buffer here instead of ao_plugin_device_init() */

  internal->bufferByteCount =  (internal->buffer_time * format->rate / 1000) *
    (device->output_channels * format->bits / 8);

  internal->firstValidByteOffset = 0;
  internal->validByteCount = 0;
  internal->buffer = malloc(internal->bufferByteCount);
  if (!internal->buffer) {
    aerror("Unable to allocate queue buffer.\n");
    return 0;
  }
  memset(internal->buffer, 0, internal->bufferByteCount);

  /* limited to stereo for now */
  //if(!device->output_matrix)
  //device->output_matrix=strdup("L,R");

  return 1;
}


int ao_plugin_play(ao_device *device, const char *output_samples,
		uint_32 num_bytes)
{
  ao_macosx_internal *internal = (ao_macosx_internal *) device->internal;
  int err;
  unsigned int bytesToCopy;
  unsigned int firstEmptyByteOffset, emptyByteCount;

  while (num_bytes) {

    // Get a consistent set of data about the available space in the queue,
    // figure out the maximum number of bytes we can copy in this chunk,
    // and claim that amount of space
    pthread_mutex_lock(&mutex);

    // Wait until there is some empty space in the queue
    emptyByteCount = internal->bufferByteCount - internal->validByteCount;
    while (emptyByteCount == 0) {
      if(!internal->started){
	err = AudioOutputUnitStart(internal->outputAudioUnit);
	adebug("Starting audio output unit\n");
	if(err){
	  pthread_mutex_unlock(&mutex);
	  aerror("Failed to start audio output => %d\n",(int)err);
	  return 0;
	}
	internal->started = true;
      }

      err = pthread_cond_wait(&cond, &mutex);
      if (err)
        adebug("pthread_cond_wait() => %d\n",err);
      emptyByteCount = internal->bufferByteCount - internal->validByteCount;
    }

    // Compute the offset to the first empty byte and the maximum number of
    // bytes we can copy given the fact that the empty space might wrap
    // around the end of the queue.
    firstEmptyByteOffset = (internal->firstValidByteOffset + internal->validByteCount) % internal->bufferByteCount;
    if (firstEmptyByteOffset + emptyByteCount > internal->bufferByteCount)
      bytesToCopy = MIN(num_bytes, internal->bufferByteCount - firstEmptyByteOffset);
    else
      bytesToCopy = MIN(num_bytes, emptyByteCount);

    // Copy the bytes and get ready for the next chunk, if any
    memcpy(internal->buffer + firstEmptyByteOffset, output_samples, bytesToCopy);

    num_bytes -= bytesToCopy;
    output_samples += bytesToCopy;
    internal->validByteCount += bytesToCopy;

    pthread_mutex_unlock(&mutex);

  }

  return 1;
}


int ao_plugin_close(ao_device *device)
{
  ao_macosx_internal *internal = (ao_macosx_internal *) device->internal;
  OSStatus status;
  UInt32 sizeof_running,running;

  // If we never got started and there's data waiting to be played,
  // start now.

  pthread_mutex_lock(&mutex);

  if(internal->output_p){
    internal->output_p=0;
    internal->isStopping = true;

    if(!internal->started && internal->validByteCount){
      status = AudioOutputUnitStart(internal->outputAudioUnit);
      adebug("Starting audio output unit\n");
      if(status){
        pthread_mutex_unlock(&mutex);
        aerror("Failed to start audio output => %d\n",(int)status);
        return 0;
      }
      internal->started = true;
    }

    // For some rare cases (using atexit in your program) Coreaudio tears
    // down the HAL itself, so we do not need to do that here.
    // We wouldn't get an error if we did, but AO would hang waiting for the 
    // AU to flush the buffer
    sizeof_running = sizeof(UInt32);
    AudioUnitGetProperty(internal->outputAudioUnit, 
                         kAudioDevicePropertyDeviceIsRunning,
                         kAudioUnitScope_Input,
                         0,
                         &running, 
                         &sizeof_running);

    if (!running) {
      pthread_mutex_unlock(&mutex);
      return 1;
    }

    // Only stop if we ever got started
    if (internal->started) {

      // Wait for any pending data to get flushed
      while (internal->validByteCount)
        pthread_cond_wait(&cond, &mutex);

      pthread_mutex_unlock(&mutex);
    
      status = AudioOutputUnitStop(internal->outputAudioUnit);
      if (status) {
        awarn("AudioOutputUnitStop returned %d\n", (int)status);
        return 0;
      }

      status = AudioComponentInstanceDispose(internal->outputAudioUnit);
      if (status) {
        awarn("AudioComponentInstanceDispose returned %d\n", (int)status);
        return 0;
      }
    }else
      pthread_mutex_unlock(&mutex);
  }else
    pthread_mutex_unlock(&mutex);

  return 1;
}


void ao_plugin_device_clear(ao_device *device)
{
  ao_macosx_internal *internal = (ao_macosx_internal *) device->internal;

  if(internal->buffer)
    free(internal->buffer);
  free(internal);
  device->internal=NULL;
}

