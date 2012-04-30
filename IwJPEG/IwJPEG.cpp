/*
 * This file is part of the Marmalade SDK Code Samples.
 *
 * Copyright (C) 2001-2011 Ideaworks3D Ltd.
 * All Rights Reserved.
 *
 * This source code is intended only as a supplement to Ideaworks Labs
 * Development Tools and/or on-line documentation.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

// Includes
#include "IwJPEG.h"
#include "IwTexture.h"

extern "C"
{
#include "jpeglib.h"
}

//-----------------------------------------------------------------------------

struct buf_source_mgr
{
    jpeg_source_mgr pub;
    char*           buf;
    char            buf_term[2];
    long            buf_size;
    long            pos;
    bool            read_started;
};

void init_source_from_buf(j_decompress_ptr cinfo){
  buf_source_mgr* src = (buf_source_mgr*) cinfo->src;
  src->read_started = true;
}

void skip_input_data_from_buf(j_decompress_ptr cinfo, long nbytes){
  buf_source_mgr* src = (buf_source_mgr*) cinfo->src;
  if (nbytes > 0) {
    src->pub.next_input_byte += (size_t) nbytes;
    src->pub.bytes_in_buffer -= (size_t) nbytes;
  }
}

boolean fill_input_buffer_from_buf(j_decompress_ptr cinfo){
  buf_source_mgr* src = (buf_source_mgr*) cinfo->src;

  if (src->pos == src->buf_size){
    src->buf_term[0] = (JOCTET) 0xFF;
    src->buf_term[1] = (JOCTET) JPEG_EOI;
    src->pub.next_input_byte = (JOCTET*)src->buf_term;
    src->pub.bytes_in_buffer = 2;
    src->read_started = false;
    return TRUE;
  }

  src->pub.next_input_byte = (JOCTET*)src->buf;
  src->pub.bytes_in_buffer = src->buf_size;
  src->pos = src->buf_size;
  src->read_started = false;

  return TRUE;
}

void term_source_from_buf(j_decompress_ptr cinfo){
}

void jpeg_buf_src (j_decompress_ptr cinfo, char* buf,long size){
  buf_source_mgr* src = (buf_source_mgr*) cinfo->src;
  if (cinfo->src == NULL) {
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                 sizeof(buf_source_mgr));
    src = (buf_source_mgr*) cinfo->src;
  }

  src = (buf_source_mgr*) cinfo->src;
  src->pub.init_source = init_source_from_buf;
  src->pub.fill_input_buffer = fill_input_buffer_from_buf;
  src->pub.skip_input_data = skip_input_data_from_buf;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source = term_source_from_buf;
  src->pub.bytes_in_buffer = 0;
  src->pub.next_input_byte = (JOCTET*)NULL;

  src->buf = buf;
  src->read_started = false;
  src->buf_size = size;
  src->pos = 0;
}

bool IsJPEG(const char * buf, int len)
{
    const char pJPEGSignature[] =
        { 0xFF, 0xD8, 0xFF, 0xE0, 0, 0, 0x4A, 0x46, 0x49, 0x46, 0x00 };

    if (len > (int)sizeof(pJPEGSignature))
    {
        if (!memcmp(buf, pJPEGSignature, 4) &&
            !memcmp(buf+6, pJPEGSignature+6, 5))
        {
            return true;
        }
    }

    return false;
}

void JPEGTexture(const char * buf, int len, CIwTexture & ImageTex)
{
    /* Init jpeg */
    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress  (&cinfo);
    //jpeg_stdio_src        (&cinfo, fp);
    jpeg_buf_src(&cinfo,(char*)buf,len);
    //Realloc crash?
    jpeg_read_header    (&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress   (&cinfo);

    int newlen = cinfo.image_width * cinfo.image_height * 3;

    unsigned char * data = (unsigned char*)s3eMalloc(newlen);
    unsigned char * linha = data;

    while (cinfo.output_scanline < cinfo.output_height)
    {
        linha = data + 3 * cinfo.image_width * cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo,&linha,1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    ImageTex.CopyFromBuffer(cinfo.output_width, cinfo.output_height, CIwImage::BGR_888,
        cinfo.output_components*cinfo.output_width, data, 0);

    s3eFree(data);
}
