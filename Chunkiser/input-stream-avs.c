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
    int header_out;

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
    header_out = (pkt.flags & PKT_FLAG_KEY) != 0;
    if (header_out == 0) {
      s->frames_since_global_headers++;
      if (s->frames_since_global_headers == HEADER_REFRESH_PERIOD) {
        s->frames_since_global_headers = 0;
        header_out = 1;
      }
    }
    *size = pkt.size + s->s->streams[pkt.stream_index]->codec->extradata_size * header_out;
    data = malloc(*size);
    if (data == NULL) {
      *size = -1;
      av_free_packet(&pkt);

      return NULL;
    }
    if (header_out && s->s->streams[pkt.stream_index]->codec->extradata_size) {
      memcpy(data, s->s->streams[pkt.stream_index]->codec->extradata, s->s->streams[pkt.stream_index]->codec->extradata_size);
      memcpy(data + s->s->streams[pkt.stream_index]->codec->extradata_size, pkt.data, pkt.size);
    } else {
      memcpy(data, pkt.data, pkt.size);
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
