/*
 *
 *  ao_wmm.c
 *
 *      Copyright (C) Benjamin Gerard - March 2007
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

//#define PREPARE_EACH
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <ks.h>
#include <ksmedia.h>

#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <stdio.h>

#ifndef KSDATAFORMAT_SUBTYPE_PCM
#define KSDATAFORMAT_SUBTYPE_PCM (GUID) {0x00000001,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}}
#endif


#include "ao/ao.h"
/* #include "ao/plugin.h" */

#define GALLOC_WVHD_TYPE (GHND)
#define GALLOC_DATA_TYPE (GHND)

static const char * mmerror(MMRESULT mmrError)
{
  static char mmbuffer[1024];
  int len;
  sprintf(mmbuffer,"mm:%d ",(int)mmrError);
  len = (int)strlen(mmbuffer);
  waveOutGetErrorText(mmrError, mmbuffer+len, sizeof(mmbuffer)-len);
  mmbuffer[sizeof(mmbuffer)-1] = 0;
  return mmbuffer;
}

static char * ao_wmm_options[] = {"dev", "id", "matrix","verbose","quiet","debug"};
static ao_info ao_wmm_info =
  {
    /* type             */ AO_TYPE_LIVE,
    /* name             */ "WMM audio driver output ",
    /* short-name       */ "wmm",
    /* author           */ "Benjamin Gerard <benjihan@users.sourceforge.net>",
    /* comment          */ "Outputs audio to the Windows MultiMedia driver.",
    /* prefered format  */ AO_FMT_LITTLE,
    /* priority         */ 20,
    /* options          */ ao_wmm_options,
    /* # of options     */ sizeof(ao_wmm_options)/sizeof(*ao_wmm_options)
  };

typedef struct {
  WAVEHDR wh;          /* waveheader                        */
  char *  data;        /* sample data ptr                   */
  int     idx;         /* index of this header              */
  int     count;       /* current byte count                */
  int     length;      /* size of data                      */
  int     sent;        /* set when header is sent to device */
} myWH_t;

typedef struct ao_wmm_internal {
  UINT  id;             /* device id                       */
  HWAVEOUT hwo;         /* waveout handler                 */
  WAVEOUTCAPS caps;     /* device caps                     */
  WAVEFORMATEXTENSIBLE wavefmt; /* sample format           */

  int opened;           /* device has been opened          */
  int prepared;         /* waveheaders have been prepared  */
  int blocks;           /* number of blocks (wave headers) */
  int splPerBlock;      /* sample per blocks.              */
  int msPerBlock;       /* millisecond per block (approx.) */

  void * bigbuffer;     /* Allocated buffer for waveheaders and sound data */
  myWH_t * wh;          /* Pointer to waveheaders in bigbuffer             */
  BYTE * spl;           /* Pointer to sound data in bigbuffer              */

  int sent_blocks;      /* Number of waveheader sent (not ack).        */
  int full_blocks;      /* Number of waveheader full (ready to send).  */
  int widx;             /* Index to the block being currently filled.  */
  int ridx;             /* Index to the block being sent.              */

} ao_wmm_internal;

int ao_wmm_test(void)
{
  return 1; /* This plugin works in default mode */
}

ao_info *ao_wmm_driver_info(void)
{
  return &ao_wmm_info;
}

int ao_wmm_set_option(ao_device *device,
                      const char *key, const char *value)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  int res = 0;

  if (!strcmp(key, "dev")) {
    if (!strcmp(value,"default")) {
      key = "id";
      value = "0";
    } else {
      WAVEOUTCAPS caps;
      int i, max = waveOutGetNumDevs();

      adebug("searching for device %s among %d\n", value, max);
      for (i=0; i<max; ++i) {
        MMRESULT mmres = waveOutGetDevCaps(i, &caps, sizeof(caps));
        if (mmres == MMSYSERR_NOERROR) {
          res = !strcmp(value, caps.szPname);
          adebug("checking id=%d, name='%s', ver=%d.%d  => [%s]\n",
                i,caps.szPname,caps.vDriverVersion>>8,caps.vDriverVersion&255,res?"YES":"no");
          if (res) {
            internal->id   = i;
            internal->caps = caps;
            break;
          }
        } else {
          aerror("waveOutGetDevCaps(%d) => %s",i,mmerror(mmres));
        }
      }
      goto finish;
    }
  }

  if (!strcmp(key,"id")) {
    MMRESULT mmres;
    WAVEOUTCAPS caps;

    int id  = strtol(value,0,0);
    int max = waveOutGetNumDevs();

    if (id >= 0 &&  id <= max) {
      if (id-- == 0) {
        adebug("set default wavemapper\n");
        id = WAVE_MAPPER;
      }
      mmres = waveOutGetDevCaps(id, &caps, sizeof(caps));

      if (mmres == MMSYSERR_NOERROR) {
        res = 1;
        adebug("checking id=%d, name='%s', ver=%d.%d  => [YES]\n",
              id,caps.szPname,caps.vDriverVersion>>8,caps.vDriverVersion&255);
        internal->id   = id;
        internal->caps = caps;
      } else {
        aerror("waveOutGetDevCaps(%d) => %s",id,mmerror(mmres));
      }
    }
  }

 finish:
  return res;
}


int ao_wmm_device_init(ao_device *device)
{
  ao_wmm_internal *internal;
  int res;

  internal = (ao_wmm_internal *) malloc(sizeof(ao_wmm_internal));
  device->internal = internal;
  if (internal != NULL) {
    memset(internal,0,sizeof(ao_wmm_internal));
    internal->id          = WAVE_MAPPER;
    internal->blocks      = 32;
    internal->splPerBlock = 512;
    /* set default device */
    ao_wmm_set_option(device,"id","0");
  }

  res = internal != NULL;

  device->output_matrix = strdup("L,R,C,LFE,BL,BR,CL,CR,BC,SL,SR");
  device->output_matrix_order = AO_OUTPUT_MATRIX_COLLAPSIBLE;

  return res;
}

static int _ao_open_device(ao_device *device)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  int res;
  MMRESULT mmres;

  mmres =
    waveOutOpen(&internal->hwo,
		internal->id,
		&internal->wavefmt.Format,
		(DWORD_PTR)0/* waveOutProc */,
		(DWORD_PTR)device,
		CALLBACK_NULL/* |WAVE_FORMAT_DIRECT */|WAVE_ALLOWSYNC);

  if(mmres == MMSYSERR_NOERROR){
    adebug("waveOutOpen id=%d, channels=%d, bits=%d, rate %d => SUCCESS\n",
          internal->id,
          internal->wavefmt.Format.nChannels,
          internal->wavefmt.Format.wBitsPerSample,
          internal->wavefmt.Format.nSamplesPerSec);
  }else{
    aerror("waveOutOpen id=%d, channels=%d, bits=%d, rate %d => FAILED\n",
          internal->id,
          internal->wavefmt.Format.nChannels,
          internal->wavefmt.Format.wBitsPerSample,
          internal->wavefmt.Format.nSamplesPerSec);
  }

  if (mmres == MMSYSERR_NOERROR) {
    UINT id;
    if (MMSYSERR_NOERROR == waveOutGetID(internal->hwo,&id)) {
      internal->id = id;
    }
  }

  res = (mmres == MMSYSERR_NOERROR);
  return res;
}

static int _ao_close_device(ao_device *device)
{
  ao_wmm_internal * internal = (ao_wmm_internal *) device->internal;
  int res;
  MMRESULT mmres;

  mmres = waveOutClose(internal->hwo);
  if(mmres == MMSYSERR_NOERROR) {
    adebug("waveOutClose(%d)\n => %s\n", internal->id, mmerror(mmres));
  }else{
    aerror("waveOutClose(%d)\n => %s\n", internal->id, mmerror(mmres));
  }
  res = (mmres == MMSYSERR_NOERROR);

  return res;
}

static int _ao_alloc_wave_headers(ao_device *device)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  int bytesPerBlock = internal->wavefmt.Format.nBlockAlign * internal->splPerBlock;
  /*   int bytes = internal->blocks * (sizeof(WAVEHDR) + bytesPerBlock); */
  int bytes = internal->blocks * (sizeof(*internal->wh) + bytesPerBlock);
  int res;
  MMRESULT mmres;

  adebug("_ao_alloc_wave_headers blocks=%d, bytes/blocks=%d, total=%d\n",
         internal->blocks,bytesPerBlock,bytes);

  internal->bigbuffer = malloc(bytes);
  if (internal->bigbuffer != NULL) {
    int i;
    BYTE * b;

    memset(internal->bigbuffer,0,bytes);
    internal->wh = internal->bigbuffer;
    internal->spl = (LPBYTE) (internal->wh+internal->blocks);
    for (i=0, b=internal->spl; i<internal->blocks; ++i, b+=bytesPerBlock) {
      internal->wh[i].data = b;
      internal->wh[i].wh.lpData = internal->wh[i].data;
      internal->wh[i].length = bytesPerBlock;
      internal->wh[i].wh.dwBufferLength = internal->wh[i].length;
      internal->wh[i].wh.dwUser = (DWORD_PTR)device;
      mmres = waveOutPrepareHeader(internal->hwo,
				   &internal->wh[i].wh,sizeof(WAVEHDR));
      if (MMSYSERR_NOERROR != mmres) {
        aerror("waveOutPrepareHeader(%d) => %s\n",i, mmerror(mmres));
        break;
      }
    }
    if (i<internal->blocks) {
      while (--i >= 0) {
        waveOutUnprepareHeader(internal->hwo,
			       &internal->wh[i].wh,sizeof(WAVEHDR));
      }
      free(internal->bigbuffer);
      internal->wh        = 0;
      internal->spl       = 0;
      internal->bigbuffer = 0;
    } else {
      /* all ok ! */
    }
  } else {
    adebug("malloc() => FAILED\n");
  }

  res = (internal->bigbuffer != NULL);
  if(!res){
    aerror("_ao_alloc_wave_headers() => FAILED\n");
  }else{
    adebug("_ao_alloc_wave_headers() => success\n");
  }
  return res;
}

static int _ao_get_free_block(ao_device * device);
static int _ao_wait_wave_headers(ao_device *device, int wait_all)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  int res = 1;

  adebug("wait for %d blocks (%swait all)\n",
         internal->sent_blocks,wait_all?"":"not ");

  while (internal->sent_blocks > 0) {
    int n;
    _ao_get_free_block(device);
    n = internal->sent_blocks;
    if (n > 0) {
      unsigned int ms = (internal->msPerBlock>>1)+1;
      if (wait_all) ms *= n;
      adebug("sleep for %ums wait on %d blocks\n",ms, internal->sent_blocks);
      Sleep(ms);
    }
  }

  res &= !internal->sent_blocks;
  if(!res){
    aerror("_ao_wait_wave_headers => FAILED\n");
  }else{
    adebug("_ao_wait_wave_headers => success\n");
  }
  return res;
}

static int _ao_free_wave_headers(ao_device *device)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  MMRESULT mmres;
  int res = 1;

  if (internal->wh) {
    int i;

    /* Reset so we dont need to wait ... Just a satefy net
     * since _ao_wait_wave_headers() has been called once before.
     */
    mmres = waveOutReset(internal->hwo);
    adebug("waveOutReset(%d) => %s\n", internal->id, mmerror(mmres));
    /* Wait again to be sure reseted waveheaders has been released. */
    _ao_wait_wave_headers(device,0);

    for (i=internal->blocks; --i>=0; ) {
      mmres = waveOutUnprepareHeader(internal->hwo,
				     &internal->wh[i].wh,sizeof(WAVEHDR));
      if (mmres != MMSYSERR_NOERROR)
        aerror("waveOutUnprepareHeader(%d) => %s\n", i, mmerror(mmres));

      res &= mmres == MMSYSERR_NOERROR;
    }
    internal->wh  = 0;
    internal->spl = 0;
  }

  if(!res){
    aerror("_ao_alloc_wave_headers() => FAILED\n");
  }else{
    adebug("_ao_alloc_wave_headers() => success\n");
  }
  return res;
}


/*
 * open the audio device for writing to
 */
int ao_wmm_open(ao_device * device, ao_sample_format * format)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  int res = 0;
  WAVEFORMATEXTENSIBLE wavefmt;

  adebug("open() channels=%d, bits=%d, rate=%d, format %d(%s)\n",
         device->output_channels,format->bits,format->rate,format->byte_format,
         format->byte_format==AO_FMT_LITTLE
         ?"little"
         :(format->byte_format==AO_FMT_NATIVE
           ?"native"
           :(format->byte_format==AO_FMT_BIG?"big":"unknown")));

  if(internal->opened) {
    aerror("open() => already opened\n");
    goto error_no_close;
  }

  /* Force LITTLE as specified by WIN32 API */
  format->byte_format = AO_FMT_LITTLE;
  device->driver_byte_format = AO_FMT_LITTLE;

  /* $$$ WMM 8 bit samples are unsigned... Not sure for ao ... */
  /* Yes, ao 8 bit PCM is unsigned -- Monty */

  /* Make sample format */
  memset(&wavefmt,0,sizeof(wavefmt));
  wavefmt.Format.wFormatTag          = WAVE_FORMAT_EXTENSIBLE;
  wavefmt.Format.nChannels           = device->output_channels;
  wavefmt.Format.wBitsPerSample      = (((format->bits+7)>>3)<<3);
  wavefmt.Format.nSamplesPerSec      = format->rate;
  wavefmt.Format.nBlockAlign         = (wavefmt.Format.wBitsPerSample>>3)*wavefmt.Format.nChannels;
  wavefmt.Format.nAvgBytesPerSec     = wavefmt.Format.nSamplesPerSec*wavefmt.Format.nBlockAlign;
  wavefmt.Format.cbSize              = 22;
  wavefmt.Samples.wValidBitsPerSample = format->bits;
  wavefmt.SubFormat           = KSDATAFORMAT_SUBTYPE_PCM;
  wavefmt.dwChannelMask       = device->output_mask;

  internal->wavefmt       = wavefmt;

  /* $$$ later this should be optionnal parms */
  internal->blocks      = 64;
  internal->splPerBlock = 512;
  internal->msPerBlock  =
    (internal->splPerBlock * 1000 + format->rate - 1) / format->rate;

  /* Open device */
  if(!_ao_open_device(device))
    goto error;
  internal->opened = 1;

  /* Allocate buffers */
  if (!_ao_alloc_wave_headers(device))
    goto error;
  internal->prepared = 1;

  res = 1;
 error:
  if (!res) {
    if (internal->prepared) {
      _ao_free_wave_headers(device);
      internal->prepared = 0;
    }
    if (internal->opened) {
      _ao_close_device(device);
      internal->opened = 0;
    }
  }

 error_no_close:
  if(res){
    adebug("open() => success\n");
  }else{
    aerror("open() => FAILED\n");
  }
  return res;
}



/* Send a block to audio hardware */
static int _ao_send_block(ao_device *device, const int idx)
{
  ao_wmm_internal * internal = (ao_wmm_internal *) device->internal;
  MMRESULT mmres;

  /* Satanity checks */
  if (internal->wh[idx].sent) {
    adebug("block %d marked SENT\n",idx);
    return 0;
  }
  if (!!(internal->wh[idx].wh.dwFlags & WHDR_DONE)) {
    adebug("block %d marked DONE\n",idx);
    return 0;
  }

  /* count <= 0, just pretend it's been sent */
  if (internal->wh[idx].count <= 0) {
    internal->wh[idx].sent = 2; /* set with 2 so we can track these special cases */
    internal->wh[idx].wh.dwFlags |= WHDR_DONE;
    ++internal->sent_blocks;
    return 1;
  }

  internal->wh[idx].wh.dwBufferLength = internal->wh[idx].count;
  internal->wh[idx].count = 0;
  mmres = waveOutWrite(internal->hwo,
		       &internal->wh[idx].wh, sizeof(WAVEHDR));
  internal->wh[idx].sent = (mmres == MMSYSERR_NOERROR);
  /*&& !(internal->wh[idx].wh.dwFlags & WHDR_DONE);*/
  internal->sent_blocks += internal->wh[idx].sent;
  if (mmres != MMSYSERR_NOERROR) {
    adebug("waveOutWrite(%d) => %s\n",idx,mmerror(mmres));
  }
  return mmres == MMSYSERR_NOERROR;
}

/* Get idx of next free block. */
static int _ao_get_free_block(ao_device * device)
{
  ao_wmm_internal * internal = (ao_wmm_internal *) device->internal;
  const int idx = internal->widx;
  int ridx = internal->ridx;

  while (internal->wh[ridx].sent && !!(internal->wh[ridx].wh.dwFlags & WHDR_DONE)) {
    /* block successfully sent to hardware, release it */
    /*debug("_ao_get_free_block: release block %d\n",ridx);*/
    internal->wh[ridx].sent = 0;
    internal->wh[ridx].wh.dwFlags &= ~WHDR_DONE;

    --internal->full_blocks;
    if (internal->full_blocks<0) {
      adebug("internal error with full block counter\n");
      internal->full_blocks = 0;
    }

    --internal->sent_blocks;
    if (internal->sent_blocks<0) {
      adebug("internal error with sent block counter\n");
      internal->sent_blocks = 0;
    }
    if (++ridx >= internal->blocks) ridx = 0;
  }
  internal->ridx = ridx;

  return internal->wh[idx].sent
    ? -1
    : idx;
}

/*
 * play the sample to the already opened file descriptor
 */
int ao_wmm_play(ao_device *device,
                const char *output_samples, uint_32 num_bytes)
{
  int ret = 1;
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;

  while(ret && num_bytes > 0) {
    int n;
    const int idx = _ao_get_free_block(device);

    if (idx == -1) {
      Sleep(internal->msPerBlock);
      continue;
    }

    /* Get free bytes in the block */
    n = internal->wh[idx].wh.dwBufferLength
      - internal->wh[idx].count;

    /* Get amount to copy */
    if (n > (int)num_bytes) {
      n = num_bytes;
    }

    /* Do copy */
    CopyMemory((char*)internal->wh[idx].wh.lpData
	       + internal->wh[idx].count,
	       output_samples, n);

    /* Updates pointers and counters */
    output_samples += n;
    num_bytes -= n;
    internal->wh[idx].count += n;

    /* Is this block full ? */
    if (internal->wh[idx].count
	== internal->wh[idx].wh.dwBufferLength) {
      ++internal->full_blocks;
      if (++internal->widx == internal->blocks) {
	internal->widx = 0;
      }
      ret = _ao_send_block(device,idx);
    }
  }

  adebug("ao_wmm_play => %d rem => [%s]\n",num_bytes,ret?"success":"error");
  return ret;

}

int ao_wmm_close(ao_device *device)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;
  int ret = 0;

  if (internal->opened && internal->prepared) {
    _ao_wait_wave_headers(device, 1);
  }

  if (internal->prepared) {
    ret = _ao_free_wave_headers(device);
    internal->prepared = 0;
  }

  if (internal->opened) {
    ret = _ao_close_device(device);
    internal->opened = 0;
  }

  return ret;
}

void ao_wmm_device_clear(ao_device *device)
{
  ao_wmm_internal *internal = (ao_wmm_internal *) device->internal;

  if (internal->bigbuffer) {
    free(internal->bigbuffer); internal->bigbuffer = NULL;
  }
  free(internal);
  device->internal=NULL;
}

ao_functions ao_wmm = {
  ao_wmm_test,
  ao_wmm_driver_info,
  ao_wmm_device_init,
  ao_wmm_set_option,
  ao_wmm_open,
  ao_wmm_play,
  ao_wmm_close,
  ao_wmm_device_clear
};
