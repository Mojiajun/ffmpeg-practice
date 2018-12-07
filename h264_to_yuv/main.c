/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * video decoding with libavcodec API example
 * 在官方的例子和雷神的纯净版本例子上面的修改.
 * @example decode_video.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

#define INBUF_SIZE 4096

static int first_time = 1;

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, FILE *fp_out)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        //Y, U, V
        for(int i = 0 ; i < frame->height ; i++) {
            fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->width, fp_out);
        }
        for(int i = 0 ; i < frame->height/2 ; i++) {
            fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->width/2, fp_out);
        }
        for(int i = 0 ; i < frame->height/2 ; i++) {
            fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->width/2, fp_out);
        }

        printf("frame width is %d, frame height is %d", frame->width, frame->height);
        printf("Succeed to decode 1 frame!\n");
    }
}

int main(int argc, char **argv)
{
    const char *infilename, *outfilename;
    const AVCodec *codec;
    AVCodecContext *avcodecctx= NULL;
    AVCodecParserContext *parser;

    FILE *fp_in, *fp_out;
    AVFrame *frame;
    AVPacket *pkt;

    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;
    int ret;

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file, **.h264> <output file, **.yuv>\n", argv[0]);
        exit(0);
    }
    infilename  = argv[1];
    outfilename = argv[2];

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    /* find the MPEG-1 video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    avcodecctx = avcodec_alloc_context3(codec);
    if (!avcodecctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(avcodecctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    fp_in = fopen(infilename, "rb");
    if (!fp_in) {
        fprintf(stderr, "Could not open input file %s\n", infilename);
        exit(1);
    }

    fp_out = fopen(outfilename, "wb");
    if (!fp_out) {
        printf("Could not open output YUV file %s\n", outfilename);
        return -1;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    while (!feof(fp_in)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, fp_in);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, avcodecctx, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;
            if (first_time) {
                printf("\nCodec Full Name:%s\n", avcodecctx->codec->long_name);
                printf("width:%d\nheight:%d\n\n", avcodecctx->width, avcodecctx->height);
                first_time=0;
            }
            if (pkt->size) { //if result of av_parser_parse2() is a complete packet.
                //Some Info from AVCodecParserContext
                printf("[Packet]Size:%6d\t",pkt->size);
                switch(parser->pict_type){
                    case AV_PICTURE_TYPE_I: printf("Type:I\t");break;
                    case AV_PICTURE_TYPE_P: printf("Type:P\t");break; case AV_PICTURE_TYPE_B: printf("Type:B\t");break;
                    default: printf("Type:Other\t");break;
                }
                printf("Number:%4d\n",parser->output_picture_number);
                decode(avcodecctx, frame, pkt, fp_out);
            }
        }
    }

    /* flush the decoder */
    decode(avcodecctx, frame, NULL, fp_out);

    fclose(fp_in);
    fclose(fp_out);

    av_parser_close(parser);
    avcodec_free_context(&avcodecctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}