/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"

static AVFormatContext *format_init(const uint8_t *data)
{
  AVFormatContext *of;
  AVCodecContext *c;
  AVOutputFormat *outfmt;
  int width, height, frame_rate_n, frame_rate_d;

  av_register_all();

  width = data[1] << 8 | data[2];
  height = data[3] << 8 | data[4];
  frame_rate_n = data[5] << 8 | data[6];
  frame_rate_d = data[7] << 8 | data[8];
  dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);

  outfmt = av_guess_format("nut", NULL, NULL);
  of = avformat_alloc_context();
  if (of == NULL) {
    return NULL;
  }
  of->oformat = outfmt;
  av_new_stream(of, 0);
  c = of->streams[0]->codec;
  c->codec_id = CODEC_ID_MPEG4;	// FIXME!!!
  c->codec_type = CODEC_TYPE_VIDEO;
  c->width = width;
  c->height= height;
  c->time_base.den = frame_rate_n;
  c->time_base.num = frame_rate_d;
  of->streams[0]->avg_frame_rate.num = frame_rate_n;
  of->streams[0]->avg_frame_rate.den = frame_rate_d;
  c->pix_fmt = PIX_FMT_YUV420P;

  return of;
}

void chunk_write(int id, const uint8_t *data, int size)
{
  static AVFormatContext *outctx;
  const int header_size = 1 + 2 + 2 + 2 + 2 + 1; // 1 Frame type + 2 width + 2 height + 2 frame rate num + 2 frame rate den + 1 number of frames
  int frames, i;

  if (data[0] != 1) {
    fprintf(stderr, "Error! Non video chunk: %x!!!\n", data[0]);
    return;
  }
  if (outctx == NULL) {
    outctx = format_init(data);
    if (outctx == NULL) {
      fprintf(stderr, "Format init failed\n");

      return;
    }
    av_set_parameters(outctx, NULL);
    snprintf(outctx->filename, sizeof(outctx->filename), "%s", "out.nut");
    dump_format(outctx, 0, "out.nut", 1);
    url_fopen(&outctx->pb, "out.nut", URL_WRONLY);
    av_write_header(outctx);
  }

  frames = data[9];
  for (i = 0; i < frames; i++) {
    AVPacket pkt;

    dprintf("Frame %d has size %d\n", i, data[10 + 2 * i] << 8 | data[11 + 2 * i]);
    av_init_packet(&pkt);
    pkt.stream_index = 0;	// FIXME!
    pkt.pts = AV_NOPTS_VALUE;	// FIXME!
    pkt.data = data + header_size + frames * 2;
    pkt.size = data[10 + 2 * i] << 8 | data[11 + 2 * i];
    av_interleaved_write_frame(outctx, &pkt);
  }
}
