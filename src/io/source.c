/*
  Copyright (C) 2012 Paul Brossier <piem@aubio.org>

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
#include "fvec.h"
#include "fmat.h"
#include "io/source.h"
#ifdef HAVE_LIBAV
#include "io/source_avcodec.h"
#endif /* HAVE_LIBAV */
#ifdef HAVE_SOURCE_APPLE_AUDIO
#include "io/source_apple_audio.h"
#endif /* HAVE_SOURCE_APPLE_AUDIO */
#ifdef HAVE_SNDFILE
#include "io/source_sndfile.h"
#endif /* HAVE_SNDFILE */
#ifdef HAVE_WAVREAD
#include "io/source_wavread.h"
#endif /* HAVE_WAVREAD */





aubio_source_mem_t* new_aubio_source_pcm_mem(unsigned char  * pData ,  uint_t  nLen , uint_t channels , uint_t BitsPerSample, uint_t samplerate, 
    uint_t hop_size )
{
  aubio_source_mem_t * s = AUBIO_NEW(aubio_source_mem_t);
  AUBIO_ERROR("file: %s , func :%s , line:%d ,pData = %x ,nLen=%d\n",__FILE__ , __func__ , __LINE__ ,pData, nLen);//9922500 frames, p->data_size=39690000,4倍的关系才是对的

#ifdef HAVE_WAVREAD
  s->source_mem = (void *)new_aubio_source_wavread_mem( pData , nLen , samplerate, hop_size , channels, BitsPerSample);
  if (s->source_mem) {
    s->s_do_mem = (aubio_source_do_mem_t)(aubio_source_wavread_do_mem);
    s->s_do_multi_mem = (aubio_source_do_multi_mem_t)(aubio_source_wavread_do_multi_mem);
    s->s_get_channels_mem = (aubio_source_get_channels_mem_t)(aubio_source_wavread_get_channels_mem);
    s->s_get_samplerate_mem = (aubio_source_get_samplerate_mem_t)(aubio_source_wavread_get_samplerate_mem);
    s->s_get_duration_mem = (aubio_source_get_duration_mem_t)(aubio_source_wavread_get_duration_mem);
    s->s_seek_mem = (aubio_source_seek_mem_t)(aubio_source_wavread_seek_mem);
    s->s_close_mem = (aubio_source_close_mem_t)(aubio_source_wavread_close_mem);
    s->s_del_mem = (del_aubio_source_mem_t)(del_aubio_source_wavread_mem);
    return s;
  }
  
#endif /* HAVE_WAVREAD */
#if !defined(HAVE_WAVREAD) && \
  !defined(HAVE_LIBAV) && \
  !defined(HAVE_SOURCE_APPLE_AUDIO) && \
  !defined(HAVE_SNDFILE)
  AUBIO_ERROR("source: failed creating with  at %dHz with hop size %d"
     " (no source built-in)\n",  samplerate, hop_size);
#endif
  del_aubio_source_mem(s);
  return NULL;
}


aubio_source_t * new_aubio_source(const char_t * uri, uint_t samplerate, uint_t hop_size) {
  aubio_source_t * s = AUBIO_NEW(aubio_source_t);

#ifdef HAVE_WAVREAD
  s->source = (void *)new_aubio_source_wavread(uri, samplerate, hop_size);
  if (s->source) {
    s->s_do = (aubio_source_do_t)(aubio_source_wavread_do);
    s->s_do_multi = (aubio_source_do_multi_t)(aubio_source_wavread_do_multi);
    s->s_get_channels = (aubio_source_get_channels_t)(aubio_source_wavread_get_channels);
    s->s_get_samplerate = (aubio_source_get_samplerate_t)(aubio_source_wavread_get_samplerate);
    s->s_get_duration = (aubio_source_get_duration_t)(aubio_source_wavread_get_duration);
    s->s_seek = (aubio_source_seek_t)(aubio_source_wavread_seek);
    s->s_close = (aubio_source_close_t)(aubio_source_wavread_close);
    s->s_del = (del_aubio_source_t)(del_aubio_source_wavread);
    return s;
  }
#endif /* HAVE_WAVREAD */
#if !defined(HAVE_WAVREAD) && \
  !defined(HAVE_LIBAV) && \
  !defined(HAVE_SOURCE_APPLE_AUDIO) && \
  !defined(HAVE_SNDFILE)
  AUBIO_ERROR("source: failed creating with %s at %dHz with hop size %d"
     " (no source built-in)\n", uri, samplerate, hop_size);
#endif
  del_aubio_source(s);
  return NULL;
}

void aubio_source_do(aubio_source_t * s, fvec_t * data, uint_t * read) {
  s->s_do((void *)s->source, data, read);
}

void aubio_source_do_multi(aubio_source_t * s, fmat_t * data, uint_t * read) {
  s->s_do_multi((void *)s->source, data, read);
}

uint_t aubio_source_close(aubio_source_t * s) {
  return s->s_close((void *)s->source);
}

void del_aubio_source(aubio_source_t * s) {
  //AUBIO_ASSERT(s);
  if (s && s->s_del && s->source)
    s->s_del((void *)s->source);
  AUBIO_FREE(s);
}

uint_t aubio_source_get_samplerate(aubio_source_t * s) {
  return s->s_get_samplerate((void *)s->source);
}

uint_t aubio_source_get_channels(aubio_source_t * s) {
  return s->s_get_channels((void *)s->source);
}

uint_t aubio_source_get_duration(aubio_source_t *s) {
  return s->s_get_duration((void *)s->source);
}

uint_t aubio_source_seek (aubio_source_t * s, uint_t seek ) {
  return s->s_seek((void *)s->source, seek);
}


//#####################add 

aubio_source_wav_mem_t * new_aubio_source_wav_mem( unsigned char  * pData ,  uint_t  nLen , uint_t samplerate, uint_t hop_size) {
  aubio_source_wav_mem_t * s = AUBIO_NEW(aubio_source_wav_mem_t);

#ifdef HAVE_WAVREAD
  s->source_wav_mem = (void *)new_aubio_source_wavread_wav_mem(pData, nLen , samplerate, hop_size);
  if (s->source_wav_mem) {
    s->s_do_wav_mem = (aubio_source_do_wav_mem_t)(aubio_source_wavread_do_wav_mem);
    s->s_do_multi_wav_mem = (aubio_source_do_multi_wav_mem_t)(aubio_source_wavread_do_multi_wav_mem);
    s->s_get_channels_wav_mem = (aubio_source_get_channels_wav_mem_t)(aubio_source_wavread_get_channels_wav_mem);
    s->s_get_samplerate_wav_mem = (aubio_source_get_samplerate_wav_mem_t)(aubio_source_wavread_get_samplerate_wav_mem);
    s->s_get_duration_wav_mem = (aubio_source_get_duration_wav_mem_t)(aubio_source_wavread_get_duration_wav_mem);
    s->s_seek_wav_mem = (aubio_source_seek_wav_mem_t)(aubio_source_wavread_seek_wav_mem);
    s->s_close_wav_mem = (aubio_source_close_wav_mem_t)(aubio_source_wavread_close_wav_mem);
    s->s_del_wav_mem = (del_aubio_source_wav_mem_t)(del_aubio_source_wavread_wav_mem);
    return s;
  }
#endif /* HAVE_WAVREAD */
#if !defined(HAVE_WAVREAD) && \
  !defined(HAVE_LIBAV) && \
  !defined(HAVE_SOURCE_APPLE_AUDIO) && \
  !defined(HAVE_SNDFILE)
  AUBIO_ERROR("source: failed creating with %s at %dHz with hop size %d"
     " (no source built-in)\n", uri, samplerate, hop_size);
#endif
  del_aubio_source_wav_mem(s);
  return NULL;
}

void aubio_source_do_wav_mem(aubio_source_wav_mem_t * s, fvec_t * data, uint_t * read) {
  s->s_do_wav_mem((void *)s->source_wav_mem, data, read);
}

void aubio_source_do_multi_wav_mem(aubio_source_wav_mem_t * s, fmat_t * data, uint_t * read) {
  s->s_do_multi_wav_mem((void *)s->source_wav_mem, data, read);
}

uint_t aubio_source_close_wav_mem(aubio_source_wav_mem_t * s) {
  return s->s_close_wav_mem((void *)s->source_wav_mem);
}

void del_aubio_source_wav_mem(aubio_source_wav_mem_t * s) {
  //AUBIO_ASSERT(s);
  if (s && s->s_del_wav_mem && s->source_wav_mem)
    s->s_del_wav_mem((void *)s->source_wav_mem);
  AUBIO_FREE(s);
}

uint_t aubio_source_get_samplerate_wav_mem(aubio_source_wav_mem_t * s) {
  return s->s_get_samplerate_wav_mem((void *)s->source_wav_mem);
}

uint_t aubio_source_get_channels_wav_mem(aubio_source_wav_mem_t * s) {
  return s->s_get_channels_wav_mem((void *)s->source_wav_mem);
}

uint_t aubio_source_get_duration_wav_mem(aubio_source_wav_mem_t *s) {
  return s->s_get_duration_wav_mem((void *)s->source_wav_mem);
}

uint_t aubio_source_seek_wav_mem (aubio_source_wav_mem_t * s, uint_t seek ) {
  return s->s_seek_wav_mem((void *)s->source_wav_mem, seek);
}


void aubio_source_do_mem(aubio_source_mem_t * s, fvec_t * data, uint_t * read) {
  s->s_do_mem((void *)s->source_mem , data , read );
}

void aubio_source_do_multi_mem(aubio_source_mem_t * s, fmat_t * data, uint_t * read) {
  s->s_do_multi_mem((void *)s->source_mem, data, read);
}


uint_t aubio_source_close_mem(aubio_source_mem_t * s) {
  return s->s_close_mem((void *)s->source_mem);
}

void del_aubio_source_mem(aubio_source_mem_t * s) {
  AUBIO_ASSERT(s);
  if (s && s->s_del_mem && s->source_mem)
    s->s_del_mem((void *)s->source_mem);
  AUBIO_FREE(s);
}

uint_t aubio_source_get_samplerate_mem(aubio_source_mem_t * s) {
  return s->s_get_samplerate_mem((void *)s->source_mem);
}

uint_t aubio_source_get_channels_mem(aubio_source_mem_t * s) {
  return s->s_get_channels_mem((void *)s->source_mem);
}

uint_t aubio_source_get_duration_mem(aubio_source_mem_t *s) {
  return s->s_get_duration_mem((void *)s->source_mem);
}

uint_t aubio_source_seek_mem (aubio_source_mem_t * s, uint_t seek ) {
  return s->s_seek_mem((void *)s->source_mem, seek);
}

