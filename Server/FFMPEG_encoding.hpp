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

#include <math.h>

#define __STDC_CONSTANT_MACROS

extern "C" {
	#include <libavutil/opt.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/common.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/samplefmt.h>
};

using namespace boost::asio;
using ip::tcp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

uint8_t endcode[] = { 0, 0, 1, 0xb7 };
class FFMPEG_encoding {
public:
	void load(int width, int height, socket_ptr sock) {
		this->sock = sock;
		c = NULL;
		codec_id = AV_CODEC_ID_H264;
		i=0;

		avcodec_register_all();

		/* find the mpeg1 video encoder */
		codec = avcodec_find_encoder(codec_id);
		if (!codec) {
			fprintf(stderr, "Codec not found\n");
			exit(1);
		}

		c = avcodec_alloc_context3(codec);
		if (!c) {
			fprintf(stderr, "Could not allocate video codec context\n");
			exit(1);
		}

		/* put sample parameters */
		c->bit_rate = 400000;
		/* resolution must be a multiple of two */
		c->width = width;
		c->height = height;
		/* frames per second */
		AVRational r;
		r.den=1;
		r.num=25;
		c->time_base = r;
		/* emit one intra frame every ten frames
		 * check frame pict_type before passing frame
		 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
		 * then gop_size is ignored and the output of encoder
		 * will always be I frame irrespective to gop_size
		 */
		c->gop_size = 10;
		c->max_b_frames = 1;
		c->pix_fmt = AV_PIX_FMT_YUV420P;//AV_PIX_FMT_YUV444P;

		// ultrafast,superfast, veryfast, faster, fast, medium, slow, slower, veryslow
		if (codec_id == AV_CODEC_ID_H264)
			av_opt_set(c->priv_data, "preset", "ultrafast", 0);

		/* open it */
		if (avcodec_open2(c, codec, NULL) < 0) {
			fprintf(stderr, "Could not open codec\n");
			exit(1);
		}

		/*f = fopen(filename.c_str(), "wb");
		if (!f) {
			fprintf(stderr, "Could not open %s\n", filename);
			exit(1);
		}*/

		frame = av_frame_alloc();
		if (!frame) {
			fprintf(stderr, "Could not allocate video frame\n");
			exit(1);
		}
		frame->format = c->pix_fmt;
		frame->width  = c->width;
		frame->height = c->height;

		/* the image can be allocated by any means and av_image_alloc() is
		 * just the most convenient way if av_malloc() is to be used */
		int ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
							 c->pix_fmt, 32);
		if (ret < 0) {
			fprintf(stderr, "Could not allocate raw picture buffer\n");
			exit(1);
		}
	}
	void write(int width, int height, RGBQUAD *pPixels) {
		av_init_packet(&pkt);
		pkt.data = NULL;    // packet data will be allocated by the encoder
		pkt.size = 10000;

		fflush(stdout);
		
		for (int y = 0; y < c->height; y++) {
			for (int x = 0; x < c->width; x++) {
				
				RGBQUAD px = pPixels[y*width+x];

				int Y =  0.299 * px.rgbRed + 0.587 * px.rgbGreen + 0.114 * px.rgbBlue;
				int U  = -0.147 * px.rgbRed - 0.289 * px.rgbGreen + 0.436 *  px.rgbBlue + 128;
				int V  =  0.615 * px.rgbRed - 0.515 * px.rgbGreen - 0.100 * px.rgbBlue + 128;

				frame->data[0][y * frame->linesize[0] + x] = Y;
				//frame->data[1][y * frame->linesize[0] + x] = U;
				//frame->data[2][y * frame->linesize[0] + x] = V;
				
				frame->data[1][(y >> 1) * frame->linesize[1] + (x >> 1)] = U;
				frame->data[2][(y >> 1) * frame->linesize[2] + (x >> 1)] = V;
			}
		}

		frame->pts = i;
		i++;
		/* encode the image */
		int got_output;
		int ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame (size=%5d)\n", pkt.size);
			//fwrite(pkt.data, 1, pkt.size, f);
			boost::asio::write(*sock, buffer((char*)pkt.data, pkt.size));
			av_free_packet(&pkt);
		}
	}
	void close () {
		/* get the delayed frames */
		/*for (got_output = 1; got_output; i++) {
			fflush(stdout);

			int ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
			if (ret < 0) {
				fprintf(stderr, "Error encoding frame\n");
				exit(1);
			}

			if (got_output) {
				printf("Write frame %3d (size=%5d)\n", i, pkt.size);
				fwrite(pkt.data, 1, pkt.size, f);
				av_free_packet(&pkt);
			}
		}*/

		/* add sequence end code to have a real mpeg file */
		//fwrite(endcode, 1, sizeof(endcode), f);
		//fclose(f);

		avcodec_close(c);
		av_free(c);
		av_freep(&frame->data[0]);
		av_frame_free(&frame);
	}
private:
	AVCodecID codec_id;
	AVCodec *codec;
	AVCodecContext *c;
	AVFrame *frame;
	AVPacket pkt;
	socket_ptr sock;
	int i;
};