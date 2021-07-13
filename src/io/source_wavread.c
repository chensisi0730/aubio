/*
  Copyright (C) 2014 Paul Brossier <piem@aubio.org>

  This file is part of aubio.

  aubio is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  aubio is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with aubio.  If not, see <http://www.gnu.org/licenses/>.
 
*/
 
#include "aubio_priv.h"
#include <stdio.h>
#include <string.h>


#define MY_DEBUG 1

#ifdef HAVE_WAVREAD

#include "fvec.h"
#include "fmat.h"
#include "ioutils.h"
#include "source_wavread.h"

#define AUBIO_WAVREAD_BUFSIZE 1024

//#define SHORT_TO_FLOAT(x) (smpl_t)(x * 3.0517578125e-05)
/*
struct _aubio_source_wavread_t {
  uint_t hop_size;
  uint_t samplerate;
  uint_t channels;

  // some data about the file
  char_t *path;
  uint_t input_samplerate;
  uint_t input_channels;

  // internal stuff
  FILE *fid;
  uint_t data_size ;
  uint_t *data_add;

  uint_t read_samples;
  uint_t blockalign;
  uint_t bitspersample;
  uint_t read_index;
  uint_t eof;

  uint_t duration;

  size_t seek_start;

  unsigned char *short_output;
  fmat_t *output;
};
*/



static unsigned int read_little_endian (unsigned char *buf,
    unsigned int length);

static unsigned int read_little_endian (unsigned char *buf,
    unsigned int length)
{
  uint_t i, ret = 0;
  for (i = 0; i < length; i++) {
    ret += buf[i] << (i * 8);
  }
  return ret;
}


aubio_source_wavread_mem_t * new_aubio_source_wavread_mem( unsigned char* pData ,  uint_t   nLen, 
    uint_t samplerate, uint_t hop_size, uint_t channels , uint_t BitsPerSample  ) {
  aubio_source_wavread_mem_t * s = AUBIO_NEW(aubio_source_wavread_mem_t);
  size_t bytes_read = 0, bytes_junk = 0, bytes_expected = 44;
  unsigned char buf[5] = "\0";
  unsigned int format,  sr, byterate, blockalign, duration, bitspersample = 0;//, data_size;
     BitsPerSample=16;
  if ((sint_t)samplerate <= 0) {
    AUBIO_ERR("source_wavread: Can not open  with samplerate %d\n", samplerate);
    goto beach;
  }
  if ((sint_t)hop_size <= 0) {
    AUBIO_ERR("source_wavread: Can not open  with hop_size %d\n", hop_size);
    goto beach;
  }
  
  #if 1
  if ( BitsPerSample != 16 ) {
    AUBIO_ERR("source_wavread: can not process %dbit \n",
        bitspersample);
  }
  #endif

  channels = 2;
  bitspersample = 16;
  s->samplerate = samplerate;
  s->hop_size = hop_size;
  s->input_samplerate = samplerate;
  s->input_channels = channels;
  s->pData= pData;//±£´æÔÚ´Ë
  s->nLen= nLen;


  blockalign = (channels * bitspersample)/8 ;
  s->seek_start = 0;
  s->output = new_fmat(s->input_channels , AUBIO_WAVREAD_BUFSIZE);//  ????
  s->blockalign= blockalign;
  s->bitspersample = BitsPerSample;
  s->duration = duration;
  s->short_output = (unsigned char *)calloc(s->blockalign, AUBIO_WAVREAD_BUFSIZE);// 4 * 1024 
  s->read_index = 0;
  s->read_samples = 0;
  s->eof = 0;
#if MY_DEBUG
    AUBIO_ERR("channels %d\n", channels);
    AUBIO_ERR("sr %d\n", s->samplerate);
    //AUBIO_ERR("byterate %d\n", byterate);
    AUBIO_ERR("blockalign %d\n", blockalign);
    AUBIO_ERR("bitspersample %d\n", bitspersample);
    AUBIO_ERR("s->pData 0x%x\n", s->pData);
    AUBIO_ERR("s->nLen %d\n", s->nLen);
    AUBIO_ERR("s->short_output 0x%x\n", s->short_output);
    AUBIO_ERR("s->output 0x%x\n", s->output);
    
#endif
  return s;
beach:
  AUBIO_ERR("source_wavread: can not read  at samplerate %dHz with a hop_size of %d\n"
      , s->samplerate, s->hop_size);
  del_aubio_source_wavread_mem(s);
  return NULL;
}


//½âÎöWAV¸ñÊ½
aubio_source_wavread_t * new_aubio_source_wavread(const char_t * path, uint_t samplerate, uint_t hop_size) {
  aubio_source_wavread_t * s = AUBIO_NEW(aubio_source_wavread_t);
  size_t bytes_read = 0, bytes_junk = 0, bytes_expected = 44;
  unsigned char buf[5] = "\0";
  unsigned int format, channels, sr, byterate, blockalign, duration, bitspersample;//, data_size;

  if (path == NULL) {
    AUBIO_ERR("source_wavread: Aborted opening null path\n");
    goto beach;
  }
  if ((sint_t)samplerate < 0) {
    AUBIO_ERR("source_wavread: Can not open %s with samplerate %d\n", path, samplerate);
    goto beach;
  }
  if ((sint_t)hop_size <= 0) {
    AUBIO_ERR("source_wavread: Can not open %s with hop_size %d\n", path, hop_size);
    goto beach;
  }

  s->path = AUBIO_ARRAY(char_t, strnlen(path, PATH_MAX) + 1);//±£´æÂ·¾¶
  strncpy(s->path, path, strnlen(path, PATH_MAX) + 1);

  s->samplerate = samplerate;
  s->hop_size = hop_size;

  s->fid = fopen((const char *)path, "rb");
  if (!s->fid) {
    AUBIO_STRERR("source_wavread: Failed opening %s (%s)\n", s->path, errorstr);
    goto beach;
  }

  // ChunkID
  bytes_read += fread(buf, 1, 4, s->fid);
  buf[4] = '\0' ;
  if ( strcmp((const char *)buf, "RIFF") != 0 ) {//  ²»µÈÓÚRIFF¾Í±¨´í
    AUBIO_ERR("source_wavread: Failed opening %s (could not find RIFF header)\n", s->path);
    goto beach;
  }

  // ChunkSize
  bytes_read += fread(buf, 1, 4, s->fid);

  // Format
  bytes_read += fread(buf, 1, 4, s->fid);
  buf[4] = '\0';
  if ( strcmp((const char *)buf, "WAVE") != 0 ) {//  ²»µÈÓÚWAVE¾Í±¨´í
    AUBIO_ERR("source_wavread: Failed opening %s (wrong format in RIFF header)\n", s->path);
    goto beach;
  }

  // Subchunk1ID
  bytes_read += fread(buf, 1, 4, s->fid);
  buf[4] = '\0';

  // check if we have a JUNK Chunk
  if ( strcmp((const char *)buf, "JUNK") == 0 ) {
    bytes_junk = fread(buf, 1, 4, s->fid);
    buf[4] = '\0';
    bytes_junk += read_little_endian(buf, 4);
    if (fseek(s->fid, bytes_read + bytes_junk, SEEK_SET) != 0) {
      AUBIO_STRERR("source_wavread: Failed opening %s (could not seek past JUNK Chunk: %s)\n",
          s->path, errorstr);
      goto beach;
    }
    bytes_read += bytes_junk;
    bytes_expected += bytes_junk + 4;
    // now really read the fmt chunk
    bytes_read += fread(buf, 1, 4, s->fid);
    buf[4] = '\0';
  }

  // get the fmt chunk
  if ( strcmp((const char *)buf, "fmt ") != 0 ) {
    AUBIO_ERR("source_wavread: Failed opening %s (could not find 'fmt ' in RIFF header)\n", s->path);
    goto beach;
  }

  // Subchunk1Size
  bytes_read += fread(buf, 1, 4, s->fid);
  format = read_little_endian(buf, 4);
  if ( format != 16 ) {
    // TODO accept format 18
    AUBIO_ERR("source_wavread: Failed opening %s (not encoded with PCM)\n", s->path);
    goto beach;
  }
  if ( buf[1] || buf[2] | buf[3] ) {
    AUBIO_ERR("source_wavread: Failed opening %s (Subchunk1Size should be 0)\n", s->path);
    goto beach;
  }

  // AudioFormat
  bytes_read += fread(buf, 1, 2, s->fid);
  if ( buf[0] != 1 || buf[1] != 0) {
    AUBIO_ERR("source_wavread: Failed opening %s (AudioFormat should be PCM)\n", s->path);
    goto beach;
  }

  // NumChannels
  bytes_read += fread(buf, 1, 2, s->fid);
  channels = read_little_endian(buf, 2);

  // SampleRate
  bytes_read += fread(buf, 1, 4, s->fid);
  sr = read_little_endian(buf, 4);

  // ByteRate
  bytes_read += fread(buf, 1, 4, s->fid);
  byterate = read_little_endian(buf, 4);

  // BlockAlign  Êý¾Ý¿é¶ÔÆëµ¥Î»(Ã¿¸ö²ÉÑùÐèÒªµÄ×Ö½ÚÊý)  Ò»¸ö²ÉÑù¾ÍÊÇÒ»Ö¡
  bytes_read += fread(buf, 1, 2, s->fid); 
  blockalign = read_little_endian(buf, 2);

  // BitsPerSample
  bytes_read += fread(buf, 1, 2, s->fid);
  bitspersample = read_little_endian(buf, 2);
  
  AUBIO_INF("source_wavread:    channels = %d  sr = %d byterate=%d  blockalign=%d bitspersample=%d   \n", channels ,sr ,byterate ,blockalign ,bitspersample);
//channels = 2  sr = 44100 byterate=176400  blockalign=4 bitspersample=16

  if ( channels == 0 ) {
    AUBIO_ERR("source_wavread: Failed opening %s (number of channels can not be 0)\n", s->path);
    goto beach;
  }

  if ( (sint_t)sr <= 0 ) {
    AUBIO_ERR("source_wavread: Failed opening %s (samplerate can not be <= 0)\n", s->path);
    goto beach;
  }

  if ( byterate == 0 ) {
    AUBIO_ERR("source_wavread: Failed opening %s (byterate can not be 0)\n", s->path);
    goto beach;
  }

  if ( bitspersample == 0 ) {
    AUBIO_ERR("source_wavread: Failed opening %s (bitspersample can not be 0)\n", s->path);
    goto beach;
  }
#if 0
  if ( bitspersample != 16 ) {
    AUBIO_ERR("source_wavread: can not process %dbit file %s\n",
        bitspersample, s->path);
    goto beach;
  }
#endif

  if ( byterate * 8 != sr * channels * bitspersample ) {// ÆäÊµÊÇÑéÖ¤ÏÂBITÂÊ¶Ô²»¶Ô
    AUBIO_ERR("source_wavread: Failed opening %s (wrong byterate)\n", s->path);
    goto beach;
  }

  if ( blockalign * 8 != channels * bitspersample ) {
    AUBIO_ERR("source_wavread: Failed opening %s (wrong blockalign)\n", s->path);
    goto beach;
  }

  s->input_samplerate = sr;
  s->input_channels = channels;

#if MY_DEBUG
  AUBIO_DBG("channels %d\n", channels);
  AUBIO_DBG("sr %d\n", sr);
  AUBIO_DBG("byterate %d\n", byterate);
  AUBIO_DBG("blockalign %d\n", blockalign);
  AUBIO_DBG("bitspersample %d\n", bitspersample);

  AUBIO_DBG("found %d channels in %s\n", s->input_channels, s->path);
  AUBIO_DBG("found %d samplerate in %s\n", s->input_samplerate, s->path);
#endif

  if (samplerate == 0) {
    s->samplerate = s->input_samplerate;
  } else if (samplerate != s->input_samplerate) {// ²»¿ÉÄÜ
    AUBIO_ERR("source_wavread: can not resample %s from %d to %dHz\n",
        s->path, s->input_samplerate, samplerate);
    goto beach;
  }

  // Subchunk2ID
  bytes_read += fread(buf, 1, 4, s->fid);
  buf[4] = '\0';
  while ( strcmp((const char *)buf, "data") != 0 ) {// no find data(id)
    if (feof(s->fid) || ferror(s->fid)) {
      AUBIO_ERR("source_wavread: no data RIFF header found in %s\n", s->path);
      goto beach;
    }
    bytes_junk = fread(buf, 1, 4, s->fid);
    buf[4] = '\0';
    bytes_junk += read_little_endian(buf, 4);
    if (fseek(s->fid, bytes_read + bytes_junk, SEEK_SET) != 0) {
      AUBIO_STRERR("source_wavread: could not seek past unknown chunk in %s (%s)\n",
          s->path, errorstr);
      goto beach;
    }
    bytes_read += bytes_junk;
    bytes_expected += bytes_junk+ 4;
    bytes_read += fread(buf, 1, 4, s->fid);
    buf[4] = '\0';
  }

  // Subchunk2Size
  bytes_read += fread(buf, 1, 4, s->fid);
  duration = read_little_endian(buf, 4) / blockalign;

#if MY_DEBUG
  int data_size =0 ,tem_len=0;
  data_size = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);//Êµ¼ÊÊý¾Ý´óÐ¡,BYTE
  AUBIO_MSG("found %d frames in %s\n", 8 * data_size / bitspersample / channels, s->path);//also =data_size/blockalign
  AUBIO_MSG("duration %d\n", duration);// duration ==frames
#endif 
  s->data_size = data_size ;
  s->data_addr = (unsigned char*)AUBIO_MALLOC(data_size);
  tem_len = fread(s->data_addr, 1, data_size, s->fid) ;//°ÑÊý¾ÝºÍ´óÐ¡¶¼¶Á³öÀ´
  fseek(s->fid , -data_size, SEEK_CUR );//SEEK_CUR:1 ÎÄ¼þÖ¸Õë»¹ÐèÒªÒÆ¶¯µ½Ô­À´µÄÎ»ÖÃ
  
  //fopen();//save as pcm file chensisi TODO:
  // check the total number of bytes read is correct
  if ( bytes_read != bytes_expected ) {
#ifndef HAVE_WIN_HACKS
    AUBIO_ERR("source_wavread: short read (%zd instead of %zd) in %s\n",
        bytes_read, bytes_expected, s->path);
#else // mingw does not know about %zd...
    AUBIO_ERR("source_wavread: short read (%d instead of %d) in %s\n",
        (int)bytes_read, (int)bytes_expected, s->path);
#endif
    goto beach;
  } 
  s->seek_start = bytes_read;

  s->output = new_fmat(s->input_channels , AUBIO_WAVREAD_BUFSIZE);//  ???? chensisi  2* 1024
  s->blockalign= blockalign;
  s->bitspersample = bitspersample;

  s->duration = duration;

  s->short_output = (unsigned char *)calloc(s->blockalign, AUBIO_WAVREAD_BUFSIZE);// 4 * 1024  TODO: CHENSISI __attribute__((align(4)))
  s->read_index = 0;
  s->read_samples = 0;
  s->eof = 0;
  //AUBIO_ERR("s->short_output 's len =  %d\n", s->blockalign* AUBIO_WAVREAD_BUFSIZE);// duration ==frames

  return s;

beach:
  //AUBIO_ERR("source_wavread: can not read %s at samplerate %dHz with a hop_size of %d\n",
  //    s->path, s->samplerate, s->hop_size);
  del_aubio_source_wavread(s);
  return NULL;
}
 
void aubio_source_wavread_readframe_wav_mem(aubio_source_wavread_wav_mem_t *s, uint_t *wavread_read);
void aubio_source_wavread_readframe_wav_mem(aubio_source_wavread_wav_mem_t *s, uint_t *wavread_read) {
  unsigned char *short_ptr = s->short_output;
  unsigned char * dest;
  size_t  read ;
  //size_t read = fread(short_ptr, s->blockalign, AUBIO_WAVREAD_BUFSIZE, s->fid); //org
    if(s->nLen >= s->blockalign *AUBIO_WAVREAD_BUFSIZE){
      //AUBIO_ERR("file: %s , func :%s , line:%d ,s->short_output =0x%x, s->pData=0x%x  ,s->nLen:%d  ,copy-len=%d\n",__FILE__ , __func__ , __LINE__,
      //                                           s->short_output,        s->pData ,      s->nLen  ,s->blockalign *AUBIO_WAVREAD_BUFSIZE);
      dest = AUBIO_MEMCPY(short_ptr ,s->pData + s->seek_start ,  s->blockalign *AUBIO_WAVREAD_BUFSIZE);
      s->pData = s->pData + s->blockalign *AUBIO_WAVREAD_BUFSIZE;
      s->nLen = s->nLen - s->blockalign *AUBIO_WAVREAD_BUFSIZE;
      //  AUBIO_ERR("file: %s , func :%s , line:%d ,s->short_output =0x%x, s->pData=0x%x  ,s->nLen:%d\n",__FILE__ , __func__ , __LINE__,
      //                                      s->short_output,        s->pData ,      s->nLen);
      read = AUBIO_WAVREAD_BUFSIZE ; //read ï¿½Ä²ï¿½ï¿½ï¿½SIZE ï¿½ï¿½ï¿½Ç¿ï¿½ï¿½ï¿½ï¿½ï¿½
  }
  else{
    s->nLen=0;
    read = 0;
  }
  uint_t i, j, b, bitspersample = s->bitspersample;
  uint_t wrap_at = (1 << ( bitspersample - 1 ) );
  uint_t wrap_with = (1 << bitspersample);
  smpl_t scaler = 1. / wrap_at;
  int signed_val = 0;
  unsigned int unsigned_val = 0;

  for (j = 0; j < read; j++) {
    for (i = 0; i < s->input_channels; i++) {
      unsigned_val = 0;
      for (b = 0; b < bitspersample; b+=8 ) {
        unsigned_val += *(short_ptr) << b;
        short_ptr++;
      }
      signed_val = unsigned_val;
      // FIXME why does 8 bit conversion maps [0;255] to [-128;127]
      // instead of [0;127] to [0;127] and [128;255] to [-128;-1]
      if (bitspersample == 8) signed_val -= wrap_at;
      else if (unsigned_val >= wrap_at) signed_val = unsigned_val - wrap_with;
      s->output->data[i][j] = signed_val * scaler;
      //AUBIO_ERR("aubio_source_wavread_readframe:%d ,%d ,0x%0.5f\n",        i,j ,signed_val * scaler);//ï¿½ï¿½ï¿½ï¿½0
    }
  }

  *wavread_read = read;
  //AUBIO_ERR("source_wavread: can not read  at samplerate %d with a hop_size of %d\n", s->blockalign, read ); // 4 1024
  if (read < AUBIO_WAVREAD_BUFSIZE)
    s->eof = 1;
}
void aubio_source_wavread_readframe_mem(aubio_source_wavread_mem_t *s, uint_t *wavread_read);


void aubio_source_wavread_readframe_mem(aubio_source_wavread_mem_t *s, uint_t *wavread_read) {
  unsigned char *short_ptr = s->short_output; 
  unsigned char * dest;
  size_t  read ;
  //AUBIO_ERR("file: %s , func :%s , line:%d ,s->short_output =0x%x, s->pData=0x%x  ,s->nLen:%d ,s->pData= 0x%x \n",__FILE__ , __func__ , __LINE__,
  //                                          s->short_output,        s->pData ,      s->nLen , *(s->pData));

  //size_t read = fread(short_ptr, s->blockalign, AUBIO_WAVREAD_BUFSIZE, s->fid);//del  file interface
  if(s->nLen >= s->blockalign *AUBIO_WAVREAD_BUFSIZE){
      //AUBIO_ERR("file: %s , func :%s , line:%d ,s->short_output =0x%x, s->pData=0x%x  ,s->nLen:%d  ,copy-len=%d\n",__FILE__ , __func__ , __LINE__,
      //                                           s->short_output,        s->pData ,      s->nLen  ,s->blockalign *AUBIO_WAVREAD_BUFSIZE);
      dest = AUBIO_MEMCPY(short_ptr ,s->pData ,  s->blockalign *AUBIO_WAVREAD_BUFSIZE);
      s->pData = s->pData + s->blockalign *AUBIO_WAVREAD_BUFSIZE;
      s->nLen = s->nLen - s->blockalign *AUBIO_WAVREAD_BUFSIZE;
      //  AUBIO_ERR("file: %s , func :%s , line:%d ,s->short_output =0x%x, s->pData=0x%x  ,s->nLen:%d\n",__FILE__ , __func__ , __LINE__,
      //                                      s->short_output,        s->pData ,      s->nLen);
      read = AUBIO_WAVREAD_BUFSIZE ; //read µÄ²»ÊÇSIZE £¬ÊÇ¿éÊýÁ¿
  }
  else{

    read = 0;
  }
  
  //AUBIO_ERR("file: %s , func :%s , line:%d  s->nLen:%d\n",__FILE__ , __func__ , __LINE__,s->nLen);

  
  uint_t i, j, b, bitspersample = s->bitspersample;
  uint_t wrap_at = (1 << ( bitspersample - 1 ) );
  uint_t wrap_with = (1 << bitspersample);
  smpl_t scaler = 1. / wrap_at;
  int signed_val = 0;
  unsigned int unsigned_val = 0;

  for (j = 0; j < read; j++) {
    for (i = 0; i < s->input_channels; i++) {
      unsigned_val = 0;
      for (b = 0; b < bitspersample; b+=8 ) {
        unsigned_val += *(short_ptr) << b;
        short_ptr++;
      }
      signed_val = unsigned_val;
      // FIXME why does 8 bit conversion maps [0;255] to [-128;127]
      // instead of [0;127] to [0;127] and [128;255] to [-128;-1]
      if (bitspersample == 8) signed_val -= wrap_at;
      else if (unsigned_val >= wrap_at) signed_val = unsigned_val - wrap_with;
      s->output->data[i][j] = signed_val * scaler;
      //AUBIO_ERR("aubio_source_wavread_readframe:%d ,%d ,0x%0.5f\n",        i,j ,signed_val * scaler);//¶¼ÊÇ0
    }
  }

  *wavread_read = read;//read = 4096
  //AUBIO_ERR("source_wavread: can not read  at samplerate %d with a hop_size of %d\n", s->blockalign, read ); // 4 1024
  //AUBIO_ERR("file: %s , func :%s , line:%d  read:%d\n",__FILE__ , __func__ , __LINE__,read);

  //if (read == 0) s->eof = 1;//Èç¹û¶ÁÍê£¬ÉèÖÃ½áÊø±êÖ¾Î»
  if (read < AUBIO_WAVREAD_BUFSIZE) {
    s->eof = 1;
    
    }
}

void aubio_source_wavread_readframe(aubio_source_wavread_t *s, uint_t *wavread_read);

void aubio_source_wavread_readframe(aubio_source_wavread_t *s, uint_t *wavread_read) {
  unsigned char *short_ptr = s->short_output;
  size_t read = fread(short_ptr, s->blockalign, AUBIO_WAVREAD_BUFSIZE, s->fid);
  uint_t i, j, b, bitspersample = s->bitspersample;
  uint_t wrap_at = (1 << ( bitspersample - 1 ) );
  uint_t wrap_with = (1 << bitspersample);
  smpl_t scaler = 1. / wrap_at;
  int signed_val = 0;
  unsigned int unsigned_val = 0;

  for (j = 0; j < read; j++) {
    for (i = 0; i < s->input_channels; i++) {
      unsigned_val = 0;
      for (b = 0; b < bitspersample; b+=8 ) {
        unsigned_val += *(short_ptr) << b;
        short_ptr++;
      }
      signed_val = unsigned_val;
      // FIXME why does 8 bit conversion maps [0;255] to [-128;127]
      // instead of [0;127] to [0;127] and [128;255] to [-128;-1]
      if (bitspersample == 8) signed_val -= wrap_at;
      else if (unsigned_val >= wrap_at) signed_val = unsigned_val - wrap_with;
      s->output->data[i][j] = signed_val * scaler;
      //AUBIO_ERR("aubio_source_wavread_readframe:%d ,%d ,0x%0.5f\n",        i,j ,signed_val * scaler);//¶¼ÊÇ0
    }
  }

  *wavread_read = read;
  //AUBIO_ERR("source_wavread: can not read  at samplerate %d with a hop_size of %d\n", s->blockalign, read ); // 4 1024

  if (read == 0) s->eof = 1;
}

// read_data->length = hop_size(MAINÀïÃæµÄ) , read:¶Á³ö¶àÉÙ×Ö½ÚµÄÊý¾Ý£¬Êä³ö

void aubio_source_wavread_do(aubio_source_wavread_t * s, fvec_t * read_data, uint_t * read){
  uint_t i, j;
  uint_t end = 0;
  uint_t total_wrote = 0;
  uint_t length = aubio_source_validate_input_length("source_wavread", s->path,
      s->hop_size, read_data->length);//PATH  Ã»ÓÐÓÃ
  if (s->fid == NULL) {
    AUBIO_ERR("source_wavread: could not read from %s (file not opened)\n",
        s->path);
    return;
  }
  while (total_wrote < length) {
    end = MIN(s->read_samples - s->read_index, length - total_wrote);
    //AUBIO_ERR("aubio_source_wavread_do : length %d , total_wrote %d\n", length , total_wrote  );
    for (i = 0; i < end; i++) {
      read_data->data[i + total_wrote] = 0;
      for (j = 0; j < s->input_channels; j++ ) {
        read_data->data[i + total_wrote] += s->output->data[j][i + s->read_index];
      }
      read_data->data[i + total_wrote] /= (smpl_t)(s->input_channels);
    }
    total_wrote += end;
    if (total_wrote < length) {
      uint_t wavread_read = 0;
      aubio_source_wavread_readframe(s, &wavread_read);
      s->read_samples = wavread_read;
      s->read_index = 0;
      if (s->eof) {
        break;
      }
    } else {
      s->read_index += end;
    }
  }

  aubio_source_pad_output (read_data, total_wrote);

  *read = total_wrote;
}

void aubio_source_wavread_do_multi(aubio_source_wavread_t * s, fmat_t * read_data, uint_t * read){
  uint_t i,j;
  uint_t end = 0;
  uint_t total_wrote = 0;
  uint_t length = aubio_source_validate_input_length("source_wavread", s->path,
      s->hop_size, read_data->length);
  uint_t channels = aubio_source_validate_input_channels("source_wavread",
      s->path, s->input_channels, read_data->height);
  if (s->fid == NULL) {
    AUBIO_ERR("source_wavread: could not read from %s (file not opened)\n",
        s->path);
    return;
  }
  while (total_wrote < length) {
    end = MIN(s->read_samples - s->read_index, length - total_wrote);
    for (j = 0; j < channels; j++) {
      for (i = 0; i < end; i++) {
        read_data->data[j][i + total_wrote] = s->output->data[j][i];
      }
    }
    total_wrote += end;
    if (total_wrote < length) {
      uint_t wavread_read = 0;
      aubio_source_wavread_readframe(s, &wavread_read);
      s->read_samples = wavread_read;
      s->read_index = 0;
      if (s->eof) {
        break;
      }
    } else {
      s->read_index += end;
    }
  }

  aubio_source_pad_multi_output(read_data, s->input_channels, total_wrote);

  *read = total_wrote;
}

uint_t aubio_source_wavread_get_samplerate(aubio_source_wavread_t * s) {
  return s->samplerate;
}

uint_t aubio_source_wavread_get_channels(aubio_source_wavread_t * s) {
  return s->input_channels;
}

uint_t aubio_source_wavread_seek (aubio_source_wavread_t * s, uint_t pos) {
  uint_t ret = 0;
  if (s->fid == NULL) {
    AUBIO_ERR("source_wavread: could not seek %s (file not opened?)\n", s->path, pos);
    return AUBIO_FAIL;
  }
  if ((sint_t)pos < 0) {
    AUBIO_ERR("source_wavread: could not seek %s at %d (seeking position should be >= 0)\n", s->path, pos);
    return AUBIO_FAIL;
  }
  ret = fseek(s->fid, s->seek_start + pos * s->blockalign, SEEK_SET);
  if (ret != 0) {
    AUBIO_STRERR("source_wavread: could not seek %s at %d (%s)\n", s->path, pos, errorstr);
    return AUBIO_FAIL;
  }
  // reset some values
  s->eof = 0;
  s->read_index = 0;
  return AUBIO_OK;
}

uint_t aubio_source_wavread_get_duration (const aubio_source_wavread_t * s) {
  if (s && s->duration) {
    return s->duration;
  }
  return 0;
}

uint_t aubio_source_wavread_close (aubio_source_wavread_t * s) {
  if (s->fid == NULL) {
    return AUBIO_OK;
  }
  if (fclose(s->fid)) {
    AUBIO_STRERR("source_wavread: could not close %s (%s)\n", s->path, errorstr);
    return AUBIO_FAIL;
  }
  s->fid = NULL;
  return AUBIO_OK;
}

void del_aubio_source_wavread(aubio_source_wavread_t * s) {
  AUBIO_ASSERT(s);
  aubio_source_wavread_close(s);
  if (s->short_output) AUBIO_FREE(s->short_output);
  if (s->output) del_fmat(s->output);
  if (s->path) AUBIO_FREE(s->path);
  AUBIO_FREE(s);
}

//##############################  add ####


// read_data->length = hop_size(MAINÀïÃæµÄ) , read:¶Á³ö¶àÉÙ×Ö½ÚµÄÊý¾Ý£¬Êä³ö=hod_size
aubio_source_wavread_wav_mem_t * new_aubio_source_wavread_wav_mem(unsigned char  * pData ,  uint_t  nLen, uint_t samplerate, uint_t hop_size) {
  aubio_source_wavread_wav_mem_t * s = AUBIO_NEW(aubio_source_wavread_wav_mem_t);
  size_t bytes_read = 0, bytes_junk = 0, bytes_expected = 44;
  unsigned char  * pData_bak=  pData;
  unsigned char buf[5] = "\0";
  unsigned int format, channels, sr, byterate, blockalign, duration, bitspersample;//, data_size;

  if (pData == NULL) {
    AUBIO_ERR("source_wavread: Aborted opening null path\n");
    goto beach;
  }
  if ((sint_t)samplerate < 0) {
    AUBIO_ERR("source_wavread: Can not open %s with samplerate %d\n", samplerate);
    goto beach;
  }
  if ((sint_t)hop_size <= 0) {
    AUBIO_ERR("source_wavread: Can not open %s with hop_size %d\n",  hop_size);
    goto beach;
  }

  //s->path = AUBIO_ARRAY(char_t, strnlen(path, PATH_MAX) + 1);//ï¿½ï¿½ï¿½ï¿½Â·ï¿½ï¿½
  //strncpy(s->path, path, strnlen(path, PATH_MAX) + 1);

  s->samplerate = samplerate;
  s->hop_size = hop_size;



  //// ChunkID
  //bytes_read += fread(buf, 1, 4, s->fid);
  //buf[4] = '\0' ;
  //if ( strcmp((const char *)buf, "RIFF") != 0 ) {//  ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½RIFFï¿½Í±ï¿½ï¿½ï¿½
  //  AUBIO_ERR("source_wavread: Failed opening %s (could not find RIFF header)\n", s->path);
  //  goto beach;
  //}

  // ChunkID
  if(nLen>=4){
      memcpy(buf,pData, 1*4);
      buf[4] = '\0' ;
      if ( strcmp((const char *)buf, "RIFF") != 0 ) {//  ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½RIFFï¿½Í±ï¿½ï¿½ï¿½
        AUBIO_ERR("source_wavread: Failed opening  (could not find RIFF header)\n");
        goto beach;
      }
      nLen -= 4;
      bytes_read += 4;
      pData += 4;
  }
  

    

  // ChunkSize
  //bytes_read += fread(buf, 1, 4, s->fid);
  
  if(nLen>=4){
      memcpy(buf,pData, 1*4);
      buf[4] = '\0' ;
      
      nLen -= 4;
      bytes_read += 4;
      pData += 4;
  }


  // Format
  //bytes_read += fread(buf, 1, 4, s->fid);
  //buf[4] = '\0';
  //if ( strcmp((const char *)buf, "WAVE") != 0 ) {//  ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½WAVEï¿½Í±ï¿½ï¿½ï¿½
  //  AUBIO_ERR("source_wavread: Failed opening %s (wrong format in RIFF header)\n", s->path);
  //  goto beach;
  //}
  
  if(nLen>=4){
      memcpy(buf,pData, 1*4);
      buf[4] = '\0' ;
      if ( strcmp((const char *)buf, "WAVE") != 0 ) {//  ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½RIFFï¿½Í±ï¿½ï¿½ï¿½
        AUBIO_ERR("source_wavread: Failed opening  (could not find RIFF header)\n");
        goto beach;
      }
      nLen -= 4;
      bytes_read += 4;
      pData += 4;
  }  

  //// Subchunk1ID
  //bytes_read += fread(buf, 1, 4, s->fid);
  //buf[4] = '\0';

  //// check if we have a JUNK Chunk
  //if ( strcmp((const char *)buf, "JUNK") == 0 ) {
  //  bytes_junk = fread(buf, 1, 4, s->fid);
  //  buf[4] = '\0';
  //  bytes_junk += read_little_endian(buf, 4);
  //  if (fseek(s->fid, bytes_read + bytes_junk, SEEK_SET) != 0) {
  //    AUBIO_STRERR("source_wavread: Failed opening %s (could not seek past JUNK Chunk: %s)\n",
  //        s->path, errorstr);
  //    goto beach;
  //  }
  //  bytes_read += bytes_junk;
  //  bytes_expected += bytes_junk + 4;
  //  // now really read the fmt chunk
  //  bytes_read += fread(buf, 1, 4, s->fid);
  //  buf[4] = '\0';
  //}

        // Subchunk1ID
      memcpy(buf,pData, 1*4);
      nLen -= 4;
      bytes_read += 4;
      pData += 4;
      buf[4] = '\0' ;
      
      if ( strcmp((const char *)buf, "JUNK") == 0 ) {
            memcpy(buf,pData, 1*4);
            bytes_junk = 4;
            buf[4] = '\0';
            nLen -= 4;
            pData += 4;        
            bytes_junk += read_little_endian(buf, 4);

            pData = pData_bak + bytes_read + bytes_junk;

            bytes_read += bytes_junk;
            bytes_expected += bytes_junk + 4;
            // now really read the fmt chunk

            memcpy(buf,pData, 1*4);
            nLen -= 4;
            bytes_read += 4;
            pData += 4;
            buf[4] = '\0';
      }


    

  // get the fmt chunk
  if ( strcmp((const char *)buf, "fmt ") != 0 ) {
    AUBIO_ERR("source_wavread: Failed opening  (could not find 'fmt ' in RIFF header)\n");
    goto beach;
  }

  //// Subchunk1Size
  //bytes_read += fread(buf, 1, 4, s->fid);
  //format = read_little_endian(buf, 4);
  //if ( format != 16 ) {
  //  // TODO accept format 18
  //  AUBIO_ERR("source_wavread: Failed opening %s (not encoded with PCM)\n", s->path);
  //  goto beach;
  //}
  //if ( buf[1] || buf[2] | buf[3] ) {
  //  AUBIO_ERR("source_wavread: Failed opening %s (Subchunk1Size should be 0)\n", s->path);
  //  goto beach;
  //}

    // Subchunk1Size
   memcpy(buf,pData, 1*4);nLen -= 4;bytes_read += 4;pData += 4;
  format = read_little_endian(buf, 4);
  if ( format != 16 ) {
    // TODO accept format 18
    AUBIO_ERR("source_wavread: Failed opening  (not encoded with PCM)\n");
    goto beach;
  }
  if ( buf[1] || buf[2] | buf[3] ) {
    AUBIO_ERR("source_wavread: Failed opening (Subchunk1Size should be 0)\n");
    goto beach;
  }


  

  //// AudioFormat
  //bytes_read += fread(buf, 1, 2, s->fid);
  //if ( buf[0] != 1 || buf[1] != 0) {
  //  AUBIO_ERR("source_wavread: Failed opening %s (AudioFormat should be PCM)\n", s->path);
  //  goto beach;
  //}
  
  // AudioFormat
  memcpy(buf,pData, 1*2);nLen -= 2;bytes_read += 2;pData += 2;
  if ( buf[0] != 1 || buf[1] != 0) {
    AUBIO_ERR("source_wavread: Failed opening  (AudioFormat should be PCM)\n");
    goto beach;
  }
  

  //// NumChannels
  //bytes_read += fread(buf, 1, 2, s->fid);
  //channels = read_little_endian(buf, 2);
  // NumChannels
  memcpy(buf,pData, 1*2);nLen -= 2;bytes_read += 2;pData += 2;
  channels = read_little_endian(buf, 2);


  //// SampleRate
  //bytes_read += fread(buf, 1, 4, s->fid);
  //sr = read_little_endian(buf, 4);
  // SampleRate
  memcpy(buf,pData, 4);nLen -= 4;bytes_read += 4;pData += 4;
  sr = read_little_endian(buf, 4);



  //// ByteRate
  //bytes_read += fread(buf, 1, 4, s->fid);
  //byterate = read_little_endian(buf, 4);
  // ByteRate
  memcpy(buf,pData, 4);nLen -= 4;bytes_read += 4;pData += 4;
  byterate = read_little_endian(buf, 4);  

  //// BlockAlign  ï¿½ï¿½ï¿½Ý¿ï¿½ï¿½ï¿½ëµ¥Î?Ã¿ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½ï¿½ï¿½Ö½ï¿½ï¿½ï¿½)  Ò»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò»Ö¡
  //bytes_read += fread(buf, 1, 2, s->fid); 
  //blockalign = read_little_endian(buf, 2);
  // BlockAlign  ï¿½ï¿½ï¿½Ý¿ï¿½ï¿½ï¿½ëµ¥Î?Ã¿ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½ï¿½ï¿½Ö½ï¿½ï¿½ï¿½)  Ò»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò»Ö¡
  memcpy(buf,pData, 1*2);nLen -= 2;bytes_read += 2;pData += 2;
  blockalign = read_little_endian(buf, 2);  

  //// BitsPerSample
  //bytes_read += fread(buf, 1, 2, s->fid);
  //bitspersample = read_little_endian(buf, 2);

    // BitsPerSample
  memcpy(buf,pData, 1*2);nLen -= 2;bytes_read += 2;pData += 2;
  bitspersample = read_little_endian(buf, 2);
  
  
  AUBIO_INF("source_wavread:    channels = %d  sr = %d byterate=%d  blockalign=%d bitspersample=%d   \n", channels ,sr ,byterate ,blockalign ,bitspersample);
//channels = 2  sr = 44100 byterate=176400  blockalign=4 bitspersample=16

  if ( channels == 0 ) {
    AUBIO_ERR("source_wavread: Failed opening  (number of channels can not be 0)\n");
    goto beach;
  }

  if ( (sint_t)sr <= 0 ) {
    AUBIO_ERR("source_wavread: Failed opening(samplerate can not be <= 0)\n");
    goto beach;
  }

  if ( byterate == 0 ) {
    AUBIO_ERR("source_wavread: Failed opening  (byterate can not be 0)\n");
    goto beach;
  }

  if ( bitspersample == 0 ) {
    AUBIO_ERR("source_wavread: Failed opening  (bitspersample can not be 0)\n");
    goto beach;
  }
#if 0
  if ( bitspersample != 16 ) {
    AUBIO_ERR("source_wavread: can not process %dbit file %s\n",
        bitspersample, s->path);
    goto beach;
  }
#endif

  if ( byterate * 8 != sr * channels * bitspersample ) {// ï¿½ï¿½Êµï¿½ï¿½ï¿½ï¿½Ö¤ï¿½ï¿½BITï¿½Ê¶Ô²ï¿½ï¿½ï¿½
    AUBIO_ERR("source_wavread: Failed opening (wrong byterate)\n");
    goto beach;
  }

  if ( blockalign * 8 != channels * bitspersample ) {
    AUBIO_ERR("source_wavread: Failed opening (wrong blockalign)\n");
    goto beach;
  }

  s->input_samplerate = sr;
  s->input_channels = channels;

#if MY_DEBUG
  AUBIO_DBG("channels %d\n", channels);
  AUBIO_DBG("sr %d\n", sr);
  AUBIO_DBG("byterate %d\n", byterate);
  AUBIO_DBG("blockalign %d\n", blockalign);
  AUBIO_DBG("bitspersample %d\n", bitspersample);

  AUBIO_DBG("found %d channels in \n", s->input_channels);
  AUBIO_DBG("found %d samplerate in \n", s->input_samplerate);
#endif

  if (samplerate == 0) {
    s->samplerate = s->input_samplerate;
  } else if (samplerate != s->input_samplerate) {// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
    AUBIO_ERR("source_wavread: can not resample from %d to %dHz\n",
         s->input_samplerate, samplerate);
    goto beach;
  }

  //// Subchunk2ID
  //bytes_read += fread(buf, 1, 4, s->fid);
  //buf[4] = '\0';
  //while ( strcmp((const char *)buf, "data") != 0 ) {// no find data(id)
  //  if (feof(s->fid) || ferror(s->fid)) {
  //    AUBIO_ERR("source_wavread: no data RIFF header found in %s\n", s->path);
  //    goto beach;
  //  }
  //  bytes_junk = fread(buf, 1, 4, s->fid);
  //  buf[4] = '\0';
  //  bytes_junk += read_little_endian(buf, 4);
  //  if (fseek(s->fid, bytes_read + bytes_junk, SEEK_SET) != 0) {
  //    AUBIO_STRERR("source_wavread: could not seek past unknown chunk in %s (%s)\n",
  //        s->path, errorstr);
  //    goto beach;
  //  }
  //  bytes_read += bytes_junk;
  //  bytes_expected += bytes_junk+ 4;
  //  bytes_read += fread(buf, 1, 4, s->fid);
  //  buf[4] = '\0';
  //}
  
  // Subchunk2ID
  memcpy(buf,pData, 4);nLen -= 4;bytes_read += 4;pData += 4;
  buf[4] = '\0';
  if ( strcmp((const char *)buf, "data") != 0 )   {
      AUBIO_MSG("duration !!!!!!!!\n");// duration ==frames

    }



  // Subchunk2Size
 // bytes_read += fread(buf, 1, 4, s->fid);
 // duration = read_little_endian(buf, 4) / blockalign;

    // Subchunk2Size
    memcpy(buf,pData, 4);nLen -= 4;bytes_read += 4;pData += 4;
  duration = read_little_endian(buf, 4) / blockalign;

#if MY_DEBUG
  int data_size =0 ,tem_len=0;
  data_size = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);//Êµï¿½ï¿½ï¿½ï¿½ï¿½Ý´ï¿½Ð¡,BYTE
  AUBIO_MSG("found %d frames in %s\n", 8 * data_size / bitspersample / channels, s->path);//also =data_size/blockalign
  AUBIO_MSG("duration %d\n", duration);// duration ==frames
#endif 
    if(data_size < nLen)
    AUBIO_MSG("error@@#@\n");// duration ==frames

  //fopen();//save as pcm file chensisi TODO:
  // check the total number of bytes read is correct
  if ( bytes_read != bytes_expected ) {
#ifndef HAVE_WIN_HACKS
    AUBIO_ERR("source_wavread: short read (%zd instead of %zd) in %s\n",
        bytes_read, bytes_expected, s->path);
#else // mingw does not know about %zd...
    AUBIO_ERR("source_wavread: short read (%d instead of %d) in %s\n",
        (int)bytes_read, (int)bytes_expected, s->path);
#endif
    goto beach;
  } 
  s->seek_start = bytes_read;

  s->output = new_fmat(s->input_channels , AUBIO_WAVREAD_BUFSIZE);//  ???? chensisi  2* 1024
  s->blockalign= blockalign;
  s->bitspersample = bitspersample;

  s->duration = duration;
  s->pData = pData ;
  s->nLen = nLen;
  s->short_output = (unsigned char *)calloc(s->blockalign, AUBIO_WAVREAD_BUFSIZE);// 4 * 1024  TODO: CHENSISI __attribute__((align(4)))
  s->read_index = 0;
  s->read_samples = 0;
  s->eof = 0;
  //AUBIO_ERR("s->short_output 's len =  %d\n", s->blockalign* AUBIO_WAVREAD_BUFSIZE);// duration ==frames

  return s;

beach:
  //AUBIO_ERR("source_wavread: can not read %s at samplerate %dHz with a hop_size of %d\n",
  //    s->path, s->samplerate, s->hop_size);
  del_aubio_source_wavread_wav_mem(s);
  return NULL;
}


void aubio_source_wavread_do_wav_mem(aubio_source_wavread_wav_mem_t * s, fvec_t * read_data, uint_t * read){ 
  uint_t i, j;
  uint_t end = 0;
  uint_t total_wrote = 0;
  uint_t length = aubio_source_validate_input_length("source_wavread", s->path, s->hop_size, read_data->length);

  while (total_wrote < length) {
    end = MIN(s->read_samples - s->read_index, length - total_wrote);
    //AUBIO_ERR("aubio_source_wavread_do : length %d , total_wrote %d\n", length , total_wrote  );
    for (i = 0; i < end; i++) {
      read_data->data[i + total_wrote] = 0;
      for (j = 0; j < s->input_channels; j++ ) {
        read_data->data[i + total_wrote] += s->output->data[j][i + s->read_index];
      }
      read_data->data[i + total_wrote] /= (smpl_t)(s->input_channels);
    }
    total_wrote += end;
    if (total_wrote < length) {
      uint_t wavread_read = 0;
      aubio_source_wavread_readframe_wav_mem(s, &wavread_read);
      s->read_samples = wavread_read;
      s->read_index = 0;
      if (s->eof) {
        break;
      }
    } else {
      s->read_index += end;
    }
  }

  aubio_source_pad_output (read_data, total_wrote);

  *read = total_wrote;
}

void aubio_source_wavread_do_multi_wav_mem(aubio_source_wavread_wav_mem_t * s, fmat_t * read_data, uint_t * read){
  uint_t i,j;
  uint_t end = 0;
  uint_t total_wrote = 0;
  uint_t length = aubio_source_validate_input_length("source_wavread", s->path,
      s->hop_size, read_data->length);
  uint_t channels = aubio_source_validate_input_channels("source_wavread",
      s->path, s->input_channels, read_data->height);

  while (total_wrote < length) {
    end = MIN(s->read_samples - s->read_index, length - total_wrote);
    for (j = 0; j < channels; j++) {
      for (i = 0; i < end; i++) {
        read_data->data[j][i + total_wrote] = s->output->data[j][i];
      }
    }
    total_wrote += end;
    if (total_wrote < length) {
      uint_t wavread_read = 0;
      aubio_source_wavread_readframe(s, &wavread_read);
      s->read_samples = wavread_read;
      s->read_index = 0;
      if (s->eof) {
        break;
      }
    } else {
      s->read_index += end;
    }
  }

  aubio_source_pad_multi_output(read_data, s->input_channels, total_wrote);

  *read = total_wrote;
}

uint_t aubio_source_wavread_get_samplerate_wav_mem(aubio_source_wavread_wav_mem_t * s) {
  return s->samplerate;
}

uint_t aubio_source_wavread_get_channels_wav_mem(aubio_source_wavread_wav_mem_t * s) {
  return s->input_channels;
}

uint_t aubio_source_wavread_seek_wav_mem (aubio_source_wavread_wav_mem_t * s, uint_t pos) {
  uint_t ret = 0;
  /*
  if (s->fid == NULL) {
    AUBIO_ERR("source_wavread: could not seek %s (file not opened?)\n", s->path, pos);
    return AUBIO_FAIL;
  }
  if ((sint_t)pos < 0) {
    AUBIO_ERR("source_wavread: could not seek %s at %d (seeking position should be >= 0)\n", s->path, pos);
    return AUBIO_FAIL;
  }
  ret = fseek(s->fid, s->seek_start + pos * s->blockalign, SEEK_SET);
  if (ret != 0) {
    AUBIO_STRERR("source_wavread: could not seek %s at %d (%s)\n", s->path, pos, errorstr);
    return AUBIO_FAIL;
  }
  */
  // reset some values
  s->eof = 0;
  s->read_index = 0;
  return AUBIO_OK;
}

uint_t aubio_source_wavread_get_duration_wav_mem (const aubio_source_wavread_wav_mem_t * s) {
  if (s && s->duration) {
    return s->duration;
  }
  return 0;
}

uint_t aubio_source_wavread_close_wav_mem (aubio_source_wavread_wav_mem_t * s) {
  if (s->fid == NULL) {
    return AUBIO_OK;
  }
  if (fclose(s->fid)) {
    AUBIO_STRERR("source_wavread: could not close %s (%s)\n", s->path, errorstr);
    return AUBIO_FAIL;
  }
  s->fid = NULL;
  return AUBIO_OK;
}

void del_aubio_source_wavread_wav_mem(aubio_source_wavread_wav_mem_t * s) {
  AUBIO_ASSERT(s);
  aubio_source_wavread_close_wav_mem(s);
  if (s->short_output) AUBIO_FREE(s->short_output);
  if (s->output) del_fmat(s->output);
  
  AUBIO_FREE(s);
}
void aubio_source_wavread_do_mem(aubio_source_wavread_mem_t * s, fvec_t * read_data, uint_t * read){ 
  uint_t i, j;
  uint_t end = 0;
  uint_t total_wrote = 0;
  
  //AUBIO_ERR("file: %s , func :%s , line:%d  read_data->length:%d\n",__FILE__ , __func__ , __LINE__,read_data->length);
  uint_t length = aubio_source_validate_input_length_mem("source_wavread",s->hop_size, read_data->length);//PATH  Ã»ÓÐÓÃ  

  while (total_wrote < length) {
    end = MIN(s->read_samples - s->read_index , length - total_wrote);
    //AUBIO_ERR("aubio_source_wavread_do : length %d , total_wrote %d\n", length , total_wrote  );
    for (i = 0; i < end; i++) {
      read_data->data[i + total_wrote] = 0;
      for (j = 0; j < s->input_channels; j++ ) {
        read_data->data[i + total_wrote] += s->output->data[j][i + s->read_index];
      }
      read_data->data[i + total_wrote] /= (smpl_t)(s->input_channels);
    }
    total_wrote += end;
    if (total_wrote < length) {
      uint_t wavread_read = 0;
      
      //AUBIO_ERR("file: %s , func :%s , line:%d  read_data->length:%d\n",__FILE__ , __func__ , __LINE__,read_data->length);
      aubio_source_wavread_readframe_mem(s, &wavread_read);//wavread_read  numb of read 
      s->read_samples = wavread_read;
      s->read_index = 0;

      if (s->eof) {
        break;
      }
    } else {
      s->read_index += end;
    }
  }
  //AUBIO_ERR("file: %s , func :%s , line:%d  read_data->length:%d\n",__FILE__ , __func__ , __LINE__,read_data->length);

  aubio_source_pad_output (read_data, total_wrote);// total_wrote=32

  *read = total_wrote;
}

void aubio_source_wavread_do_multi_mem(aubio_source_wavread_mem_t * s, fmat_t * read_data, uint_t * read){
  uint_t i,j;
  uint_t end = 0;
  uint_t total_wrote = 0;
  uint_t length = aubio_source_validate_input_length_mem("source_wavread",
      s->hop_size, read_data->length);
  uint_t channels = aubio_source_validate_input_channels_mem("source_wavread",
      s->input_channels, read_data->height);
  if (s->pData== NULL) {
    AUBIO_ERR("source_wavread: could not read from  (file not opened)\n");
    return;
  }
  while (total_wrote < length) {
    end = MIN(s->read_samples - s->read_index, length - total_wrote);
    for (j = 0; j < channels; j++) {
      for (i = 0; i < end; i++) {
        read_data->data[j][i + total_wrote] = s->output->data[j][i];
      }
    }
    total_wrote += end;
    if (total_wrote < length) {
      uint_t wavread_read = 0;
      aubio_source_wavread_readframe_mem(s, &wavread_read);
      s->read_samples = wavread_read;
      s->read_index = 0;
      if (s->eof) {
        break;
      }
    } else {
      s->read_index += end;
    }
  }

  aubio_source_pad_multi_output(read_data, s->input_channels, total_wrote);

  *read = total_wrote;
}

uint_t aubio_source_wavread_get_samplerate_mem(aubio_source_wavread_mem_t * s) {
  return s->samplerate;
}

uint_t aubio_source_wavread_get_channels_mem(aubio_source_wavread_mem_t * s) {
  return s->input_channels;
}

uint_t aubio_source_wavread_seek_mem(aubio_source_wavread_mem_t * s, uint_t pos) {
  uint_t ret = 0;
  if (s->pData== NULL) {
    AUBIO_ERR("source_wavread: could not seek (file not opened?)\n");
    return AUBIO_FAIL;
  }
  if ((sint_t)pos < 0) {
    AUBIO_ERR("source_wavread: could not seek  at %d (seeking position should be >= 0)\n", pos);
    return AUBIO_FAIL;
  }
/*
  ret = fseek(s->fid, s->seek_start + pos * s->blockalign, SEEK_SET);
  if (ret != 0) {
    AUBIO_STRERR("source_wavread: could not seek %s at %d (%s)\n", s->path, pos, errorstr);
    return AUBIO_FAIL;
  }
  */
  // reset some values
  s->eof = 0;
  s->read_index = 0;
  return AUBIO_OK;
}

uint_t aubio_source_wavread_get_duration_mem (const aubio_source_wavread_mem_t * s) {
  if (s && s->duration) {
    return s->duration;
  }
  return 0;
}

uint_t aubio_source_wavread_close_mem (aubio_source_wavread_mem_t * s) {
  if (s->pData== NULL) {
    return AUBIO_OK;
  }

  return AUBIO_OK;
}

void del_aubio_source_wavread_mem(aubio_source_wavread_mem_t * s) {
  AUBIO_ASSERT(s);
  aubio_source_wavread_close_mem(s);
  if (s->short_output) AUBIO_FREE(s->short_output);
  if (s->output) del_fmat(s->output);
  
  AUBIO_FREE(s);
}


#endif /* HAVE_WAVREAD */
