/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdbool.h>

#include "../input-stream.h"
#include "../input.h"		//TODO: for flags. Check if we can do something smarter
#define STATIC_BUFF_SIZE 1000 * 1024
#define HEADER_REFRESH_PERIOD 50

struct input_stream {
  AVFormatContext *s;
  bool loop;	//loop on input file infinitely
  int audio_stream;
  int video_stream;
  int64_t last_ts;
  int64_t base_ts;
  int frames_since_global_headers;
};

#define VIDEO_PAYLOAD_HEADER_SIZE 1 + 2 + 2 + 2 + 2 + 1; // 1 Frame type + 2 width + 2 height + 2 frame rate num + 2 frame rate den + 1 number of frames

static uint8_t codec_type(enum CodecID cid)
{
  switch (cid) {
    case CODEC_ID_MPEG1VIDEO:
    case CODEC_ID_MPEG2VIDEO:
      return 1;
    case CODEC_ID_H261:
      return 2;
    case CODEC_ID_H263P:
    case CODEC_ID_H263:
      return 3;
    case CODEC_ID_MJPEG:
      return 4;
    case CODEC_ID_MPEG4:
      return 5;
    case CODEC_ID_FLV1:
      return 6;
    case CODEC_ID_SVQ3:
      return 7;
    case CODEC_ID_DVVIDEO:
      return 8;
    case CODEC_ID_H264:
      return 9;
    case CODEC_ID_THEORA:
    case CODEC_ID_VP3:
      return 10;
    case CODEC_ID_SNOW:
      return 11;
    case CODEC_ID_VP6:
      return 12;
    case CODEC_ID_DIRAC:
      return 13;
    default:
      fprintf(stderr, "Unknown codec ID %d\n", cid);
      return 0;
  }
}

static void video_header_fill(uint8_t *data, AVStream *st)
{
  int num, den;

  data[0] = codec_type(st->codec->codec_id);
  data[1] = st->codec->width >> 8;
  data[2] = st->codec->width & 0xFF;
  data[3] = st->codec->height >> 8;
  data[4] = st->codec->height & 0xFF;
  num = st->avg_frame_rate.num;
  den = st->avg_frame_rate.den;
//fprintf(stderr, "Rate: %d/%d\n", num, den);
  if (num == 0) {
    num = st->r_frame_rate.num;
    den = st->r_frame_rate.den;
  }
  if (num > (1 << 16)) {
    num /= 1000;
    den /= 1000;
  }
  data[5] = num >> 8;
  data[6] = num & 0xFF;
  data[7] = den >> 8;
  data[8] = den & 0xFF;
} 

struct input_stream *input_stream_open(const char *fname, int *period, uint16_t flags)
{
  struct input_stream *desc;
  int i, res;

  avcodec_register_all();
  av_register_all();

  desc = malloc(sizeof(struct input_stream));
  if (desc == NULL) {
    return NULL;
  }
  res = av_open_input_file(&desc->s, fname, NULL, 0, NULL);
  if (res < 0) {
    fprintf(stderr, "Error opening %s: %d\n", fname, res);

    return NULL;
  }

  res = av_find_stream_info(desc->s);
  if (res < 0) {
    fprintf(stderr, "Cannot find codec parameters for %s\n", fname);

    return NULL;
  }
  desc->video_stream = -1;
  desc->audio_stream = -1;
  desc->last_ts = 0;
  desc->base_ts = 0;
  desc->frames_since_global_headers = 0;
  desc->loop = flags & INPUT_LOOP;
  for (i = 0; i < desc->s->nb_streams; i++) {
    if (desc->video_stream == -1 && desc->s->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
      desc->video_stream = i;
      fprintf(stderr, "Video Frame Rate = %d/%d --- Period: %lld\n",
              desc->s->streams[i]->r_frame_rate.num,
              desc->s->streams[i]->r_frame_rate.den,
              av_rescale(1000000, desc->s->streams[i]->r_frame_rate.den, desc->s->streams[i]->r_frame_rate.num));
      *period = av_rescale(1000000, desc->s->streams[i]->r_frame_rate.den, desc->s->streams[i]->r_frame_rate.num);
    }
    if (desc->audio_stream == -1 && desc->s->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
      desc->audio_stream = i;
    }
  }

  dump_format(desc->s, 0, fname, 0);

  return desc;
}

void input_stream_close(struct input_stream *s)
{
    av_close_input_file(s->s);
    free(s);
}

int input_stream_rewind(struct input_stream *s)
{
    int ret;

    ret = av_seek_frame(s->s,-1,0,0);
    s->base_ts = s->last_ts;

    return ret;
}


#if 0
int input_get_1(struct input_stream *s, struct chunk *c)
{
    static AVPacket pkt;
    static int inited;
    AVStream *st;
    int res;
    static uint8_t static_buff[STATIC_BUFF_SIZE];
    static int cid;
    uint8_t *p;

    p = static_buff;
    if (inited == 0) {
        inited = 1;
        res = av_read_frame(s->s, &pkt);
        if (res < 0) {
            fprintf(stderr, "First read failed: %d!!!\n", res);

            return 0;
        }
        if ((pkt.flags & PKT_FLAG_KEY) == 0) {
            fprintf(stderr, "First frame is not key frame!!!\n");

            return 0;
        }
    }
    c->timestamp = pkt.dts;
    memcpy(p, pkt.data, pkt.size);
    p += pkt.size;
    while (1) {
        res = av_read_frame(s->s, &pkt);
        if (res >= 0) {
            st = s->s->streams[pkt.stream_index];
            if (pkt.flags & PKT_FLAG_KEY) {
                c->size = p - static_buff;
                c->data = malloc(c->size);
                if (c->data == NULL) {
                  return 0;
                }
                memcpy(c->data, static_buff, c->size);
                c->attributes_size = 0;
                c->attributes = NULL;
                c->id = cid++; 
                return 1;
            }
            memcpy(p, pkt.data, pkt.size);
            p += pkt.size;
        } else {
            if (p - static_buff > 0) {
                c->size = p - static_buff;
                c->data = malloc(c->size);
                if (c->data == NULL) {
                  return 0;
                }
                memcpy(c->data, static_buff, c->size);
                c->attributes_size = 0;
                c->attributes = NULL;
                c->id = cid++; 
                return 1;
            }
            return 0;
        }
    }

    return 0;
}
#endif

uint8_t *chunkise(struct input_stream *s, int id, int *size, uint64_t *ts)
{
    AVPacket pkt;
    int res;
    uint8_t *data;
    int header_out, header_size;

    res = av_read_frame(s->s, &pkt);
    if (res < 0) {
      if (s->loop) {
        if (input_stream_rewind(s) >= 0) {
          *size = 0;
          *ts = s->last_ts;

          return NULL;
        }
      }
      fprintf(stderr, "AVPacket read failed: %d!!!\n", res);
      *size = -1;

      return NULL;
    }
    if (pkt.stream_index != s->video_stream) {
      *size = 0;
      *ts = s->last_ts;
      av_free_packet(&pkt);

      return NULL;
    }

    if (s->s->streams[pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) {
      header_size = VIDEO_PAYLOAD_HEADER_SIZE;
    }
    header_out = (pkt.flags & PKT_FLAG_KEY) != 0;
    if (header_out == 0) {
      s->frames_since_global_headers++;
      if (s->frames_since_global_headers == HEADER_REFRESH_PERIOD) {
        s->frames_since_global_headers = 0;
        header_out = 1;
      }
    }
    *size = pkt.size + s->s->streams[pkt.stream_index]->codec->extradata_size * header_out + header_size + 2;
    data = malloc(*size);
    if (data == NULL) {
      *size = -1;
      av_free_packet(&pkt);

      return NULL;
    }
    if (s->s->streams[pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) {
      video_header_fill(data, s->s->streams[pkt.stream_index]);
    }
    data[9] = 1;
    data[10] = (*size - header_size - 2) >> 8;
    data[11] = (*size - header_size - 2) & 0xFF;

    if (header_out && s->s->streams[pkt.stream_index]->codec->extradata_size) {
      memcpy(data + header_size + 2, s->s->streams[pkt.stream_index]->codec->extradata, s->s->streams[pkt.stream_index]->codec->extradata_size);
      memcpy(data + header_size + 2 + s->s->streams[pkt.stream_index]->codec->extradata_size, pkt.data, pkt.size);
    } else {
      memcpy(data + header_size + 2, pkt.data, pkt.size);
    }
    *ts = av_rescale_q(pkt.dts, s->s->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q);
    *ts += s->base_ts;
    s->last_ts = *ts;
    av_free_packet(&pkt);

    return data;
}

#if 0
int chunk_read_avs1(void *s_h, struct chunk *c)
{
    AVFormatContext *s = s_h;
    static AVPacket pkt;
    static int inited;
    AVStream *st;
    int res;
    int cnt;
    static uint8_t static_buff[STATIC_BUFF_SIZE];
    uint8_t *p, *pcurr;
    static uint8_t *p1;
    static struct chunk c2;
    int f1;
    static int f2;

    if (p1) {
        c2.id = c->id;
        *c = c2;
        p1 = NULL;

        return f2;
    }

    p = static_buff;
    p1 = static_buff + STATIC_BUFF_SIZE / 2;
    if (inited == 0) {
        inited = 1;
        res = av_read_frame(s, &pkt);
        if (res < 0) {
            fprintf(stderr, "First read failed: %d!!!\n", res);

            return 0;
        }
        if ((pkt.flags & PKT_FLAG_KEY) == 0) {
            fprintf(stderr, "First frame is not key frame!!!\n");

            return 0;
        }
    }
    cnt = 0; f1 = 0; f2 = 0;
    c->stride_size = 2;
    c2.stride_size = 2;
    pcurr = p1;
    if (pkt.size > 0) {
        memcpy(p, pkt.data, pkt.size);
        c->frame[0] = p;
        c->frame_len[0] = pkt.size;
        f1++;
        p += pkt.size;
    }
    while (1) {
        res = av_read_frame(s, &pkt);
        if (res >= 0) {
            st = s->streams[pkt.stream_index];
            if (pkt.flags & PKT_FLAG_KEY) {
                cnt++;
                if (cnt == 2) {
                    return f1;
                }
            }
            memcpy(pcurr, pkt.data, pkt.size);
            if (pcurr == p) {
                c->frame[f1] = pcurr;
                c->frame_len[f1] = pkt.size;
                p += pkt.size;
                pcurr = p1;
                f1++;
            } else {
                c2.frame[f2] = pcurr;
                c2.frame_len[f2] = pkt.size;
                p1 += pkt.size;
                pcurr = p;
                f2++;
            }
        } else {
            pkt.size = 0;

            return f1;
        }
    }

    return 0;
}
#endif
