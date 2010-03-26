/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"
#include "payload.h"

static const char *output_format = "nut";
static const char *output_file = "/dev/stdout";

static int64_t prev_pts, prev_dts;

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
  uint8_t codec;

  av_register_all();

  payload_header_parse(data, &codec, &width, &height, &frame_rate_n, &frame_rate_d);
  dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);
  outfmt = av_guess_format(output_format, NULL, NULL);
  of = avformat_alloc_context();
  if (of == NULL) {
    return NULL;
  }
  of->oformat = outfmt;
  av_new_stream(of, 0);
  c = of->streams[0]->codec;
  c->codec_id = libav_codec_id(codec);
  c->codec_type = CODEC_TYPE_VIDEO;
  c->width = width;
  c->height= height;
  c->time_base.den = frame_rate_n;
  c->time_base.num = frame_rate_d;
  of->streams[0]->avg_frame_rate.num = frame_rate_n;
  of->streams[0]->avg_frame_rate.den = frame_rate_d;
  c->pix_fmt = PIX_FMT_YUV420P;

  prev_pts = 0;
  prev_dts = 0;

  return of;
}

void chunk_write(int id, const uint8_t *data, int size)
{
  static AVFormatContext *outctx;
  const int header_size = VIDEO_PAYLOAD_HEADER_SIZE; 
  int frames, i;
  const uint8_t *p;

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
    snprintf(outctx->filename, sizeof(outctx->filename), "%s", output_file);
    dump_format(outctx, 0, output_file, 1);
    url_fopen(&outctx->pb, output_file, URL_WRONLY);
    av_write_header(outctx);
  }

  frames = data[header_size - 1];
  p = data + header_size + FRAME_HEADER_SIZE * frames;
  for (i = 0; i < frames; i++) {
    AVPacket pkt;
    int32_t pts, dts;
    int frame_size;

    frame_header_parse(data + header_size + FRAME_HEADER_SIZE * i,
                       &frame_size, &pts, &dts);
    dprintf("Frame %d PTS1: %d\n", i, pts);
    pts += (pts < prev_pts - (1 << 15)) ? ((prev_pts >> 16) + 1) << 16 : (prev_pts >> 16) << 16;
    dprintf(" PTS2: %d\n", pts);
    prev_pts = pts;
    dts += (dts < prev_dts - (1 << 15)) ? ((prev_dts >> 16) + 1) << 16 : (prev_dts >> 16) << 16;
    prev_dts = dts;
    dprintf("Frame %d has size %d --- PTS: %lld DTS: %lld\n", i, frame_size,
                                             av_rescale_q(pts, outctx->streams[0]->codec->time_base, AV_TIME_BASE_Q),
                                             av_rescale_q(dts, outctx->streams[0]->codec->time_base, AV_TIME_BASE_Q));
    av_init_packet(&pkt);
    pkt.stream_index = 0;	// FIXME!
    pkt.pts = av_rescale_q(pts, outctx->streams[0]->codec->time_base, outctx->streams[0]->time_base);
    pkt.dts = av_rescale_q(dts, outctx->streams[0]->codec->time_base, outctx->streams[0]->time_base);
    pkt.data = p;
    p += frame_size;
    pkt.size = frame_size;
    av_interleaved_write_frame(outctx, &pkt);
  }
}
