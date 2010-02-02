/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>

#include <chunk.h>

#define input_desc AVFormatContext
#include "../input.h"
#define STATIC_BUFF_SIZE 1000 * 1024

struct input_desc *input_open(const char *fname)
{
    AVFormatContext *s;
    int res;

  avcodec_register_all();
  av_register_all();

    res = av_open_input_file(&s, fname, NULL, 0, NULL);
    if (res < 0) {
        fprintf(stderr, "Error opening %s: %d\n", fname, res);

        return NULL;
    }

    res = av_find_stream_info(s);
    if (res < 0) {
        fprintf(stderr, "Cannot find codec parameters for %s\n", fname);

        return NULL;
    }

    dump_format(s, 0, fname, 0);

    return s;
}

void input_close(struct input_desc *s)
{
    av_close_input_file(s);
}

int input_get_1(struct input_desc *s, struct chunk *c)
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
    c->timestamp = pkt.dts;
    memcpy(p, pkt.data, pkt.size);
    p += pkt.size;
    while (1) {
        res = av_read_frame(s, &pkt);
        if (res >= 0) {
            st = s->streams[pkt.stream_index];
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

int input_get(struct input_desc *s, struct chunk *c)
{
    AVPacket pkt;
    int res;
    static int cid;

    res = av_read_frame(s, &pkt);
    if (res < 0) {
      fprintf(stderr, "First read failed: %d!!!\n", res);

      return 0;
    }
    
    c->size = pkt.size;
    c->data = malloc(c->size);
    if (c->data == NULL) {
      av_free_packet(&pkt);
      return 0;
    }
    memcpy(c->data, pkt.data, c->size);
    c->attributes_size = 0;
    c->attributes = NULL;
    c->id = cid++; 
    av_free_packet(&pkt);
    return 1;
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
