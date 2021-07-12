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
#include "aubio_priv.h"

#include <string.h>
    
#include "fvec.h"
#include "fmat.h"
#include "ioutils.h"

*/

#include <stdio.h>




#ifndef AUBIO_SOURCE_WAVREAD_H
#define AUBIO_SOURCE_WAVREAD_H

/** \file

  Read from file using custom wav reading routines.

  Avoid including this file directly! Prefer using ::aubio_source_t instead to
  make your code portable.

  To write to file, use ::aubio_sink_t.

  References:

    - http://netghost.narod.ru/gff/graphics/summary/micriff.htm
    - https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

  \example io/test-source_wavread.c

*/

#ifdef __cplusplus
extern "C" {
#endif
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
  unsigned char *data_addr;

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

struct _aubio_source_wavread_mem_t {
  uint_t hop_size;
  uint_t samplerate;
  uint_t channels;

  // some data about the file
  unsigned char  *pData;//±£¥Ê‘Ÿ¥Œ
  uint_t  nLen;
  uint_t input_samplerate;
  uint_t input_channels;

  // internal stuff
  

  uint_t read_samples;
  uint_t blockalign;
  uint_t bitspersample;
  uint_t read_index;
  uint_t eof;

  uint_t duration;

  size_t seek_start;

  unsigned char *short_output;//??
  fmat_t *output;//???
};

/** wavread media source object */
typedef struct _aubio_source_wavread_t aubio_source_wavread_t;
typedef struct _aubio_source_wavread_mem_t aubio_source_wavread_mem_t;


/**

  create new ::aubio_source_wavread_t

  \param uri the file path or uri to read from
  \param samplerate sampling rate to view the fie at
  \param hop_size the size of the blocks to read from

  Creates a new source object. If `0` is passed as `samplerate`, the sample
  rate of the original file is used.

  The samplerate of newly created source can be obtained using
  ::aubio_source_wavread_get_samplerate.

*/
aubio_source_wavread_t * new_aubio_source_wavread(const char_t * uri, uint_t samplerate, uint_t hop_size);
aubio_source_wavread_mem_t * new_aubio_source_wavread_mem( unsigned char* pData ,  uint_t nLen, 
        uint_t samplerate, uint_t hop_size ,  uint_t BitsPerSample);

/**

  read monophonic vector of length hop_size from source object

  \param s source object, created with ::new_aubio_source_wavread
  \param read_to ::fvec_t of data to read to
  \param[out] read upon returns, equals to number of frames actually read

  Upon returns, `read` contains the number of frames actually read from the
  source. `hop_size` if enough frames could be read, less otherwise.

*/
void aubio_source_wavread_do(aubio_source_wavread_t * s, fvec_t * read_to, uint_t * read);
void aubio_source_wavread_do_mem(aubio_source_wavread_mem_t * s, fvec_t * read_to, uint_t * read);

/**

  read polyphonic vector of length hop_size from source object

  \param s source object, created with ::new_aubio_source_wavread
  \param read_to ::fmat_t of data to read to
  \param read upon returns, equals to number of frames actually read

  Upon returns, `read` contains the number of frames actually read from the
  source. `hop_size` if enough frames could be read, less otherwise.

*/
void aubio_source_wavread_do_multi(aubio_source_wavread_t * s, fmat_t * read_to, uint_t * read);
void aubio_source_wavread_do_multi_mem(aubio_source_wavread_mem_t * s, fmat_t * read_to, uint_t * read);

/**

  get samplerate of source object

  \param s source object, created with ::new_aubio_source_wavread
  \return samplerate, in Hz

*/
uint_t aubio_source_wavread_get_samplerate(aubio_source_wavread_t * s);
uint_t aubio_source_wavread_get_samplerate_mem(aubio_source_wavread_mem_t * s);

/**

  get number of channels of source object

  \param s source object, created with ::new_aubio_source_wavread
  \return number of channels

*/
uint_t aubio_source_wavread_get_channels (aubio_source_wavread_t * s);
uint_t aubio_source_wavread_get_channels_mem (aubio_source_wavread_mem_t * s);

/**

  seek source object

  \param s source object, created with ::new_aubio_source_wavread
  \param pos position to seek to, in frames

  \return 0 if sucessful, non-zero on failure

*/
uint_t aubio_source_wavread_seek (aubio_source_wavread_t *s, uint_t pos);
uint_t aubio_source_wavread_seek_mem (aubio_source_wavread_mem_t *s, uint_t pos);

/**

  get the duration of source object, in frames

  \param s source object, created with ::new_aubio_source_sndfile
  \return number of frames in file

*/
uint_t aubio_source_wavread_get_duration (const aubio_source_wavread_t *s);
uint_t aubio_source_wavread_get_duration_mem (const aubio_source_wavread_mem_t *s);

/**

  close source

  \param s source object, created with ::new_aubio_source_wavread

  \return 0 if sucessful, non-zero on failure

*/
uint_t aubio_source_wavread_close (aubio_source_wavread_t *s);
uint_t aubio_source_wavread_close_mem (aubio_source_wavread_mem_t *s);

/**

  close source and cleanup memory

  \param s source object, created with ::new_aubio_source_wavread

*/
void del_aubio_source_wavread(aubio_source_wavread_t * s);
void del_aubio_source_wavread_mem(aubio_source_wavread_mem_t * s);

#ifdef __cplusplus
}
#endif

#endif /* AUBIO_SOURCE_WAVREAD_H */
