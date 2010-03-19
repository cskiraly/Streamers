/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"

static enum CodecID libav_codec_id(uint8_t mytype)
{
  switch (mytype) {
    case 1:
      return CODEC_ID_MPEG2VIDEO;
    case 2:
      return CODEC_ID_H261;
    case 3:
      return CODEC_ID_H263P;
    case 4:
      return CODEC_ID_MJPEG;
    case 5:
      return CODEC_ID_MPEG4;
    case 6:
      return CODEC_ID_FLV1;
    case 7:
      return CODEC_ID_SVQ3;
    case 8:
      return CODEC_ID_DVVIDEO;
    case 9:
      return CODEC_ID_H264;
    case 10:
      return CODEC_ID_THEORA;
    case 11:
      return CODEC_ID_SNOW;
    case 12:
      return CODEC_ID_VP6;
    case 13:
      return CODEC_ID_DIRAC;
    default:
      fprintf(stderr, "Unknown codec %d\n", mytype);
      return 0;
  }
}

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
  c->codec_id = libav_codec_id(data[0]);
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

  if (data[0] > 127) {
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
    int32_t pts, dts;
    int frame_size;

    frame_size = data[10 + (2 + 2 + 2) * i] << 8 | data[11 + (2 + 2 + 2) * i];
    pts = data[12 + (2 + 2 + 2) * i] << 8 | data[13 + (2 + 2 + 2) * i];
    dts = data[14 + (2 + 2 + 2) * i] << 8 | data[15 + (2 + 2 + 2) * i];
    dprintf("Frame %d has size %d --- PTS: %lld DTS: %lld\n", i, frame_size,
                                             av_rescale_q(pts, outctx->streams[0]->codec->time_base, AV_TIME_BASE_Q),
                                             av_rescale_q(dts, outctx->streams[0]->codec->time_base, AV_TIME_BASE_Q));
    av_init_packet(&pkt);
    pkt.stream_index = 0;	// FIXME!
    pkt.pts = av_rescale_q(pts, outctx->streams[0]->codec->time_base, outctx->streams[0]->time_base);
    pkt.dts = av_rescale_q(dts, outctx->streams[0]->codec->time_base, outctx->streams[0]->time_base);
    pkt.data = data + header_size + (i + 1) * (2 + 2 + 2);
    pkt.size = frame_size;
    av_interleaved_write_frame(outctx, &pkt);
  }
}
