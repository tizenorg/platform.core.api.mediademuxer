/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*=======================================================================
|  INCLUDE FILES                                                                        |
========================================================================*/
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <glib.h>
#include <mm_error.h>
#include <mm_debug.h>
#include <mediademuxer_error.h>
#include <mediademuxer.h>
#include <media_format.h>
#include <media_packet.h>
#include <media_codec.h>

/*-----------------------------------------------------------------------
|    GLOBAL CONSTANT DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
#define MAX_STRING_LEN 100
#define PACKAGE "mediademuxer_test"

enum {
	CURRENT_STATUS_MAINMENU,
	CURRENT_STATUS_FILENAME,
	CURRENT_STATUS_SET_DATA
};

/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
mediademuxer_h demuxer = NULL;
media_format_mimetype_e v_mime;
media_format_mimetype_e a_mime;
int g_menu_state = CURRENT_STATUS_MAINMENU;
int num_tracks = 0;
int aud_track = -1;
int vid_track = -1;
int w;
int h;
int channel = 0;
int samplerate = 0;
int bit = 0;
bool is_adts = 0;

/*-----------------------------------------------------------------------
|    DEBUG DEFINITIONS                                                            |
-----------------------------------------------------------------------*/
/*
 * D E B U G   M E S S A G E
 */
#define MMF_DEBUG                       "** (demuxer testsuite) DEBUG: "
#define MMF_ERR                         "** (demuxer testsuite) ERROR: "
#define MMF_INFO                        "** (demuxer testsuite) INFO: "
#define MMF_WARN                        "** (demuxer testsuite) WARNING: "

#define CHECK_MM_ERROR(expr) \
	do {\
		int ret = 0; \
		ret = expr; \
		if (ret != MD_ERROR_NONE) {\
			printf("[%s:%d] error code : %x \n", __func__, __LINE__, ret); \
			return; \
		} \
	} while (0)

#define debug_msg_t(fmt, arg...)\
	do { \
		fprintf(stderr, MMF_DEBUG"[%s:%05d]  " fmt "\n", __func__, __LINE__, ##arg); \
	} while (0)

#define err_msg_t(fmt, arg...)\
	do { \
		fprintf(stderr, MMF_ERR"[%s:%05d]  " fmt "\n", __func__, __LINE__, ##arg); \
	} while (0)

#define info_msg_t(fmt, arg...)\
	do { \
		fprintf(stderr, MMF_INFO"[%s:%05d]  " fmt "\n", __func__, __LINE__, ##arg); \
	} while (0)

#define warn_msg_t(fmt, arg...)\
	do { \
		fprintf(stderr, MMF_WARN"[%s:%05d]  " fmt "\n", __func__, __LINE__, ##arg); \
	} while (0)


/*-----------------------------------------------------------------------
|    TEST VARIABLE DEFINITIONS:                                        |
-----------------------------------------------------------------------*/
#define DEMUXER_OUTPUT_DUMP         1

#if DEMUXER_OUTPUT_DUMP
FILE *fp_audio_out = NULL;
FILE *fp_video_out = NULL;
bool validate_dump = false;

#define ADTS_HEADER_SIZE            7
unsigned char buf_adts[ADTS_HEADER_SIZE];

#define AMR_NB_MIME_HDR_SIZE          6
#define AMR_WB_MIME_HDR_SIZE          9
static const char AMRNB_HDR[] = "#!AMR\n";
static const char AMRWB_HDR[] = "#!AMR-WB\n";
int write_amrnb_header = 0;	/* write  magic number for AMR-NB Header at one time */
int write_amrwb_header = 0;	/* write  magic number for AMR-WB Header at one time */
#endif

bool validate_with_codec = false;
mediacodec_h g_media_codec = NULL;
FILE *fp_out_codec_audio = NULL;
mediacodec_h g_media_codec_1 = NULL;
FILE *fp_out_codec_video = NULL;

/*-----------------------------------------------------------------------
|    HELPER  FUNCTION                                                                 |
-----------------------------------------------------------------------*/

#if DEMUXER_OUTPUT_DUMP
/**
 *  Add ADTS header at the beginning of each and every AAC packet.
 *  This is needed as MediaCodec encoder generates a packet of raw AAC data.
 *  Note the packetLen must count in the ADTS header itself.
 **/
void generate_header_aac_adts(unsigned char *buffer, int packetLen)
{
	int profile = 2;	/* AAC LC (0x01) */
	int freqIdx = 4;	/* 44KHz (0x04) */
	int chanCfg = 1;	/* CPE (0x01) */

	if (samplerate == 96000) freqIdx = 0;
	else if (samplerate == 88200) freqIdx = 1;
	else if (samplerate == 64000) freqIdx = 2;
	else if (samplerate == 48000) freqIdx = 3;
	else if (samplerate == 44100) freqIdx = 4;
	else if (samplerate == 32000) freqIdx = 5;
	else if (samplerate == 24000) freqIdx = 6;
	else if (samplerate == 22050) freqIdx = 7;
	else if (samplerate == 16000) freqIdx = 8;
	else if (samplerate == 12000) freqIdx = 9;
	else if (samplerate == 11025) freqIdx = 10;
	else if (samplerate == 8000) freqIdx = 11;

	if ((channel == 1) || (channel == 2))
		chanCfg = channel;

	/* Make ADTS header */
	buffer[0] = (char)0xFF;
	buffer[1] = (char)0xF1;
	buffer[2] = (char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
	buffer[3] = (char)(((chanCfg&3)<<6) + (packetLen>>11));
	buffer[4] = (char)((packetLen&0x7FF) >> 3);
	buffer[5] = (char)(((packetLen&7)<<5) + 0x1F);
	buffer[6] = (char)0xFC;
}
#endif



/*-----------------------------------------------------------------------
|    LOCAL FUNCTION                                                                 |
-----------------------------------------------------------------------*/

int test_mediademuxer_create()
{
	int ret = 0;
	g_print("test_mediademuxer_create\n");
	ret = mediademuxer_create(&demuxer);
	return ret;
}

int test_mediademuxer_set_data_source(mediademuxer_h demuxer, const char *path)
{
	int ret = 0;
	g_print("test_mediademuxer_set_data_source\n");

#if DEMUXER_OUTPUT_DUMP
	fp_audio_out = fopen("/opt/usr/dump_audio.out", "wb");
	if (fp_audio_out != NULL) {
		validate_dump = true;
		fp_video_out = fopen("/opt/usr/dump_video.out", "wb");
	} else
		g_print("Error - Cannot open file for file dump, Please chek root\n");
#endif

	ret = mediademuxer_set_data_source(demuxer, path);
	return ret;
}

int test_mediademuxer_prepare()
{
	int ret = 0;
	g_print("test_mediademuxer_prepare\n");
	ret = mediademuxer_prepare(demuxer);
	return ret;
}

int test_mediademuxer_get_track_count()
{
	g_print("test_mediademuxer_get_track_count\n");
	mediademuxer_get_track_count(demuxer, &num_tracks);
	g_print("Number of total tracks [%d]\n", num_tracks);
	return 0;
}

int test_mediademuxer_select_track()
{
	int track = 0;
	g_print("test_mediademuxer_select_track\n");
	for (track = 0; track < num_tracks; track++) {
		if (mediademuxer_select_track(demuxer, track)) {
			g_print("mediademuxer_select_track index [%d] failed\n", track);
			return -1;
		}
		g_print("select track index is [%d] of the total track [%d]\n", track, num_tracks);
	}
	return 0;
}

int test_mediademuxer_start()
{
	int ret = 0;
	g_print("test_mediademuxer_start\n");
	ret = mediademuxer_start(demuxer);
	return ret;
}

int test_mediademuxer_get_track_info()
{
	int ret = 0;
	int track = 0;

	g_print("test_mediademuxer_get_track_info\n");
	for (; track < num_tracks; track++) {
		media_format_h g_media_format;
		ret = mediademuxer_get_track_info(demuxer, track, &g_media_format);
		if (ret == 0) {
			if (media_format_get_video_info(g_media_format, &v_mime,
					&w, &h, NULL, NULL) == MEDIA_FORMAT_ERROR_NONE) {
				g_print("media_format_get_video_info is sucess!\n");
				g_print("\t\t[media_format_get_video]mime:%x, width :%d, height :%d\n",
							v_mime, w, h);
				vid_track = track;
			} else if (media_format_get_audio_info(g_media_format, &a_mime,
							&channel, &samplerate, &bit, NULL) == MEDIA_FORMAT_ERROR_NONE) {
				g_print("media_format_get_audio_info is sucess!\n");
				g_print("\t\t[media_format_get_audio]mime:%x, channel :%d, samplerate :%d, bit :%d\n",
							a_mime, channel, samplerate, bit);
				if (a_mime == MEDIA_FORMAT_AAC_LC)
					media_format_get_audio_aac_type(g_media_format, &is_adts);
				aud_track = track;
			} else {
					g_print("Not Supported YET\n");
			}
			media_format_unref(g_media_format);
			g_media_format = NULL;
		} else {
			g_print("Error while getting mediademuxer_get_track_info\n");
		}
	}

#if DEMUXER_OUTPUT_DUMP
	if ((a_mime == MEDIA_FORMAT_AAC_LC) && (is_adts == 0)) {
		g_print("MIME : MEDIA_FORMAT_AAC_LC ------Need to add header for dump test \n");
	} else if (a_mime == MEDIA_FORMAT_AMR_NB) {
		g_print("MIME : MEDIA_FORMAT_AMR_NB ------Need to add header for dump test \n");
		write_amrnb_header = 1;
	} else if (a_mime == MEDIA_FORMAT_AMR_WB) {
		g_print("MIME : MEDIA_FORMAT_AMR_WB ------Need to add header for dump test \n");
		write_amrwb_header = 1;
	} else
		g_print("--------------------------- Don't Need to add header for dump test\n");
#endif

	return ret;
}

static void mediacodec_finish(mediacodec_h handle, FILE *fp)
{
	int err = 0;
	fclose(fp);
	mediacodec_unset_output_buffer_available_cb(handle);
	err = mediacodec_unprepare(handle);
	if (err != MEDIACODEC_ERROR_NONE) {
		g_print("mediacodec_unprepare failed error = %d \n", err);
		return;
	}
	err = mediacodec_destroy(handle);
	if (err != MEDIACODEC_ERROR_NONE)
		g_print("mediacodec_destory failed error = %d \n", err);
	return;
}

static void _mediacodec_fill_audio_buffer_cb(media_packet_h pkt, void *user_data)
{
	int err = 0;
	uint64_t buf_size = 0;
	void *data = NULL;
	media_packet_h output_buf;

	if (pkt != NULL) {
		err = mediacodec_get_output(g_media_codec, &output_buf, 0);
		if (err == MEDIACODEC_ERROR_NONE) {
			media_packet_get_buffer_size(output_buf, &buf_size);
			media_packet_get_buffer_data_ptr(output_buf, &data);
			if (data != NULL)
				fwrite(data, 1, buf_size, fp_out_codec_audio);
			else
				g_print("Data is null inside _mediacodec_fill_audio_buffer_cb\n");

			media_packet_destroy(output_buf);
		} else {
			g_print("mediacodec_get_output failed inside _mediacodec_fill_audio_buffer_cb err = %d\n", err);
			return;
		}
	} else {
		g_print("audio pkt from mediacodec is null\n");
	}
	return;
}

static void mediacodec_init_audio(int codecid, int flag, int samplerate, int channel, int bit)
{
	/* This file will be used to dump the audio data coming out from mediacodec */
	fp_out_codec_audio = fopen("/opt/usr/codec_dump_audio.out", "wb");
	g_print("Create dumped file as codec_dump_audio.out\n");

	if (g_media_codec != NULL) {
		mediacodec_unprepare(g_media_codec);
		mediacodec_destroy(g_media_codec);
		g_media_codec = NULL;
	}
	if (mediacodec_create(&g_media_codec) != MEDIACODEC_ERROR_NONE) {
		g_print("mediacodec_create is failed\n");
		return;
	}
	/* Now set the code info */
	if ((mediacodec_set_codec(g_media_codec, (mediacodec_codec_type_e)codecid,
		(mediacodec_support_type_e)flag) != MEDIACODEC_ERROR_NONE)) {
		g_print("mediacodec_set_codec is failed\n");
		return;
	}
	/* set the audio dec info */
	if ((mediacodec_set_adec_info(g_media_codec, samplerate, channel, bit)) != MEDIACODEC_ERROR_NONE) {
		g_print("mediacodec_set_adec is failed\n");
		return;
	}
	/* Set the callback for output data, which will be used to write the data to file */
	mediacodec_set_output_buffer_available_cb(g_media_codec,
											_mediacodec_fill_audio_buffer_cb,
											g_media_codec);

	if (MEDIACODEC_ERROR_NONE !=  mediacodec_prepare(g_media_codec)) {
		g_print("mediacodec prepare is failed\n");
		return;
	}
}

static void mediacodec_process_audio_pkt(media_packet_h in_buf)
{
	if (g_media_codec != NULL) {
		/* process the media packet */
		if (MEDIACODEC_ERROR_NONE != mediacodec_process_input(g_media_codec, in_buf, 0)) {
			g_print("mediacodec_process_input is failed inside mediacodec_process_audio_pkt\n");
			return;
		}
	}
}

void *_fetch_audio_data(void *ptr)
{
	int ret = MD_ERROR_NONE;
	int *status = (int *)g_malloc(sizeof(int) * 1);
	media_packet_h audbuf;
	int count = 0;
	uint64_t buf_size = 0;
	void *data = NULL;

	*status = -1;
	g_print("Audio Data function\n");

	if (validate_with_codec) {
		int flag = 0;
		if (a_mime == MEDIA_FORMAT_AAC_LC || a_mime == MEDIA_FORMAT_AAC_HE ||
			a_mime == MEDIA_FORMAT_AAC_HE_PS) {
			flag = 10;
			g_print("mediacodec_init_audio() for MEDIACODEC_AAC\n");
			mediacodec_init_audio(MEDIACODEC_AAC, flag, samplerate, channel, bit);
		} else if (a_mime == MEDIA_FORMAT_MP3) {
			flag = 10;
			g_print("mediacodec_init_audio() for MEDIACODEC_MP3\n");
			mediacodec_init_audio(MEDIACODEC_MP3, flag, samplerate, channel, bit);
		} else if (a_mime == MEDIA_FORMAT_AMR_NB) {
			flag = 10;
			g_print("mediacodec_init_audio() for MEDIACODEC_AMR_NB\n");
			mediacodec_init_audio(MEDIACODEC_AMR_NB, flag, samplerate, channel, bit);
		} else if (a_mime == MEDIA_FORMAT_AMR_WB) {
			flag = 10;
			g_print("mediacodec_init_audio() for MEDIACODEC_AMR_WB\n");
			mediacodec_init_audio(MEDIACODEC_AMR_WB, flag, samplerate, channel, bit);
		} else if (a_mime == MEDIA_FORMAT_FLAC) {
			flag = 10;
			g_print("mediacodec_init_audio() for MEDIACODEC_FLAC\n");
			mediacodec_init_audio(MEDIACODEC_FLAC, flag, samplerate, channel, bit);
		} else if (a_mime == MEDIA_FORMAT_VORBIS) {
			flag = 10;
			g_print("mediacodec_init_audio() for MEDIACODEC_VORBIS\n");
			mediacodec_init_audio(MEDIACODEC_VORBIS, flag, samplerate, channel, bit);
		} else {
			g_print("Not Supported YET- Need to add mime for validating with audio codec\n");
			return (void *)status;
		}
	}

	while (1) {
		ret = mediademuxer_read_sample(demuxer, aud_track, &audbuf);
		if (ret == MD_EOS) {
			g_print("EOS return of mediademuxer_read_sample()\n");
			pthread_exit(NULL);
		} else if (ret != MD_ERROR_NONE) {
			g_print("Error (%d) return of mediademuxer_read_sample()\n", ret);
			pthread_exit(NULL);
		}
		count++;
		media_packet_get_buffer_size(audbuf, &buf_size);
		media_packet_get_buffer_data_ptr(audbuf, &data);
		g_print("Audio Read Count::[%4d] frame - get_buffer_size = %"PRIu64"\n", count, buf_size);

#if DEMUXER_OUTPUT_DUMP
		if (validate_dump) {
			if ((a_mime == MEDIA_FORMAT_AAC_LC) && (is_adts == 0)) {
				/* This is used only AAC raw case for adding each ADTS frame header */
				generate_header_aac_adts(buf_adts, (buf_size+ADTS_HEADER_SIZE));
				fwrite(&buf_adts[0], 1, ADTS_HEADER_SIZE, fp_audio_out);
			} else if ((a_mime == MEDIA_FORMAT_AMR_NB) && (write_amrnb_header == 1)) {
				/* This is used only AMR-NB case for adding magic header in only first frame */
				g_print("%s - AMRNB_HDR write in first frame\n", __func__);
				fwrite(&AMRNB_HDR[0], 1, sizeof(AMRNB_HDR)  - 1, fp_audio_out);
				write_amrnb_header = 0;
			} else if ((a_mime == MEDIA_FORMAT_AMR_WB) && (write_amrwb_header == 1)) {
				/* This is used only AMR-WB case for adding magic header in only first frame */
				g_print("%s - AMRWB_HDR write in first frame\n", __func__);
				fwrite(&AMRWB_HDR[0], 1, sizeof(AMRWB_HDR)  - 1, fp_audio_out);
				write_amrwb_header = 0;
			}

			if (data != NULL)
				fwrite(data, 1, buf_size, fp_audio_out);
			else
				g_print("DUMP : write(audio data) fail for NULL\n");
		}
#endif

		if (validate_with_codec)
			mediacodec_process_audio_pkt(audbuf);
		else
			media_packet_destroy(audbuf);
	}

	*status = 0;
	if (validate_with_codec)
		mediacodec_finish(g_media_codec, fp_out_codec_audio);
	return (void *)status;
}

static void _mediacodec_fill_video_buffer_cb(media_packet_h pkt, void *user_data)
{
	int err = 0;
	uint64_t buf_size = 0;
	void *data = NULL;
	media_packet_h output_buf;

	if (pkt != NULL) {
		err = mediacodec_get_output(g_media_codec_1, &output_buf, 0);
		if (err == MEDIACODEC_ERROR_NONE) {
			media_packet_get_buffer_size(output_buf, &buf_size);
			/* g_print("%s - output_buf size = %lld\n", __func__, buf_size); */
			media_packet_get_buffer_data_ptr(output_buf, &data);
			if (data != NULL)
				fwrite(data, 1, buf_size, fp_out_codec_video);
			else
				g_print("Data is null inside _mediacodec_fill_video_buffer_cb\n");
			media_packet_destroy(output_buf);
		} else {
			g_print("mediacodec_get_output failed inside _mediacodec_fill_video_buffer_cb lerr = %d\n", err);
			return;
		}
	} else {
		g_print("video pkt from mediacodec is null\n");
	}
	return;
}

static void mediacodec_init_video(int codecid, int flag, int width, int height)
{
	/* This file  will be used to dump the data */
	fp_out_codec_video = fopen("/opt/usr/codec_dump_video.out", "wb");
	g_print("Create dumped file as codec_dump_video.out\n");

	if (g_media_codec_1 != NULL) {
		mediacodec_unprepare(g_media_codec_1);
		mediacodec_destroy(g_media_codec_1);
		g_media_codec_1 = NULL;
	}
	if (mediacodec_create(&g_media_codec_1) != MEDIACODEC_ERROR_NONE) {
		g_print("mediacodec_create is failed\n");
		return;
	}
	/* Now set the code info */
	if ((mediacodec_set_codec(g_media_codec_1, (mediacodec_codec_type_e)codecid,
		(mediacodec_support_type_e)flag) != MEDIACODEC_ERROR_NONE)) {
		g_print("mediacodec_set_codec is failed\n");
		return;
	}
	/* set the video dec info */
	if ((mediacodec_set_vdec_info(g_media_codec_1, width, height)) != MEDIACODEC_ERROR_NONE) {
		g_print("mediacodec_set_vdec is failed\n");
		return;
	}
	/* Set the callback for output data, which will be used to write the data to file */
	mediacodec_set_output_buffer_available_cb(g_media_codec_1,
											_mediacodec_fill_video_buffer_cb,
											g_media_codec_1);

	if (MEDIACODEC_ERROR_NONE !=  mediacodec_prepare(g_media_codec_1)) {
		g_print("mediacodec_prepare is failed\n");
		return;
	}
}

static void mediacodec_process_video_pkt(media_packet_h in_buf)
{
	if (g_media_codec_1 != NULL) {
		/* process the media packet */
		if (MEDIACODEC_ERROR_NONE != mediacodec_process_input(g_media_codec_1, in_buf, 0)) {
			g_print("mediacodec process input is failed inside mediacodec_process_video_pkt\n");
			return;
		}
	} else {
		g_print("mediacodec handle is invalid inside mediacodec_process_video_pkt()\n");
	}
}

#if 0
static void _local_media_packet_get_codec_data(media_packet_h pkt)
{
	unsigned char* get_codec_data;
	unsigned int get_codec_data_size;

	if (media_packet_get_codec_data(pkt, (void**) &get_codec_data, &get_codec_data_size) == MEDIA_PACKET_ERROR_NONE) {
		g_print("media_packet_get_codec_data is sucess ... !\n");
		g_print("codec_data_size = %u\n", get_codec_data_size);
		get_codec_data[get_codec_data_size] = '\0';
		if (get_codec_data_size == 0)
			return;
		g_print("media packet codec_data is [%s] \n", get_codec_data);
	} else {
		g_print("media_packet_get_codec_data is failed...\n");
	}
}
#endif

void *_fetch_video_data(void *ptr)
{
	int ret = MD_ERROR_NONE;
	int *status = (int *)g_malloc(sizeof(int) * 1);
	media_packet_h vidbuf;
	int count = 0;
	uint64_t buf_size = 0;
	void *data = NULL;

	*status = -1;
	g_print("Video Data function\n");

	if (validate_with_codec) {
		int flag = 0;
		if (v_mime == MEDIA_FORMAT_H264_SP || v_mime == MEDIA_FORMAT_H264_MP ||
			v_mime == MEDIA_FORMAT_H264_HP) {
			flag = 10;
			g_print("mediacodec_init_video() for MEDIACODEC_H264\n");
			mediacodec_init_video(MEDIACODEC_H264, flag, w, h);
		} else if (v_mime == MEDIA_FORMAT_H263) {
			g_print("mediacodec_init_video() for MEDIACODEC_H263\n");
			flag = 10;
			mediacodec_init_video(MEDIACODEC_H263, flag, w, h);
		} else {
			g_print("Not Supported YET- Need to add mime for validating with video codec\n");
			return (void *)status;
		}
	}
	while (1) {
		ret = mediademuxer_read_sample(demuxer, vid_track, &vidbuf);
		if (ret == MD_EOS) {
			g_print("EOS return of mediademuxer_read_sample()\n");
			pthread_exit(NULL);
		} else if (ret != MD_ERROR_NONE) {
			g_print("Error (%d) return of mediademuxer_read_sample()\n", ret);
			pthread_exit(NULL);
		}
		count++;
		media_packet_get_buffer_size(vidbuf, &buf_size);
		media_packet_get_buffer_data_ptr(vidbuf, &data);
		g_print("Video Read Count::[%4d] frame - get_buffer_size = %"PRIu64"\n", count, buf_size);
#if 0
		/* This is used for debugging purpose */
		_local_media_packet_get_codec_data(vidbuf);
#endif
#if DEMUXER_OUTPUT_DUMP
		if (validate_dump) {
			if (data != NULL)
				fwrite(data, 1, buf_size, fp_video_out);
			else
				g_print("DUMP : write(video data) fail for NULL\n");
		}
#endif

		if (validate_with_codec)
			mediacodec_process_video_pkt(vidbuf);
		else
			media_packet_destroy(vidbuf);
	}
	*status = 0;
	if (validate_with_codec)
		mediacodec_finish(g_media_codec_1, fp_out_codec_video);

	return (void *)status;
}

int test_mediademuxer_read_sample()
{
	pthread_t thread[2];
	pthread_attr_t attr;
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (vid_track != -1) {
		g_print("In main: creating thread  for video\n");
		pthread_create(&thread[0], &attr, _fetch_video_data, NULL);
	}
	if (aud_track != -1) {
		g_print("In main: creating thread  for audio\n");
		pthread_create(&thread[1], &attr, _fetch_audio_data, NULL);
	}
	pthread_attr_destroy(&attr);
	return 0;
}

int test_mediademuxer_seek_to()
{
	g_print("test_mediademuxer_seek_to\n");
	int64_t pos = 1;
	mediademuxer_seek(demuxer, pos);
	g_print("Number of tracks [%d]\n", num_tracks);
	return 0;
}

int test_mediademuxer_unselect_track()
{
	g_print("test_mediademuxer_select_track\n");
	if (mediademuxer_unselect_track(demuxer, 0)) {
		g_print("mediademuxer_select track 0 failed\n");
		return -1;
	}
	return 0;
}

int test_mediademuxer_stop()
{
	int ret = 0;
	g_print("test_mediademuxer_stop\n");
	ret = mediademuxer_stop(demuxer);
	return ret;
}

int test_mediademuxer_unprepare()
{
	int ret = 0;
	g_print("test_mediademuxer_unprepare\n");
	ret = mediademuxer_unprepare(demuxer);
	return ret;
}

int test_mediademuxer_destroy()
{
	int ret = 0;
	g_print("test_mediademuxer_destroy\n");
	ret = mediademuxer_destroy(demuxer);
	demuxer = NULL;

#if DEMUXER_OUTPUT_DUMP
	if (fp_audio_out)
		fclose(fp_audio_out);
	if (fp_video_out)
		fclose(fp_video_out);
#endif
	return ret;
}

int test_mediademuxer_get_state()
{
	g_print("test_mediademuxer_get_state\n");
	mediademuxer_state state;
	if (mediademuxer_get_state(demuxer, &state) == MEDIADEMUXER_ERROR_NONE) {
		if (state == MEDIADEMUXER_NONE)
			g_print("Mediademuxer_state = NONE\n");
		else if (state == MEDIADEMUXER_IDLE)
			g_print("Mediademuxer_state = IDLE\n");
		else if (state == MEDIADEMUXER_READY)
			g_print("Mediademuxer_state = READY\n");
		else if (state == MEDIADEMUXER_DEMUXING)
			g_print("Mediademuxer_state = DEMUXING\n");
		else
			g_print("Mediademuxer_state = NOT SUPPORT STATE\n");
	} else
		g_print("Mediademuxer_state call failed\n");
	return 0;
}

void app_err_cb(mediademuxer_error_e error, void *user_data)
{
	printf("Got Error %d from Mediademuxer\n", error);
}

int test_mediademuxer_set_error_cb()
{
	int ret = 0;
	g_print("test_mediademuxer_set_error_cb\n");
	ret = mediademuxer_set_error_cb(demuxer, app_err_cb, demuxer);
	return ret;
}


/*-----------------------------------------------------------------------
|    EXTRA FUNCTION                                                                 |
-----------------------------------------------------------------------*/
int test_mediademuxer_get_sample_track_index()
{
	g_print("test_mediademuxer_get_sample_track_index\n");
	return 0;
}

int test_mediademuxer_get_sample_track_time()
{
	g_print("test_mediademuxer_get_sample_track_time\n");
	return 0;
}

int test_mediademuxer_advance()
{
	g_print("test_mediademuxer_advance\n");
	return 0;
}

int test_mediademuxer_is_key_frame()
{
	g_print("test_mediademuxer_is_key_frame\n");
	return 0;
}

int test_mediademuxer_is_encrypted()
{
	g_print("test_mediademuxer_is_encrypted\n");
	return 0;
}

int test_mediademuxer_audio_only()
{
	g_print("AUDIO ONLY\n");
	return 0;
}

int test_mediademuxer_video_only()
{
	g_print("VIDEO ONLY\n");
	return 0;
}

int test_mediademuxer_audio_video_only()
{
	g_print("AUDIO & VIDEO ONLY\n");
	return 0;
}

/*-----------------------------------------------------------------------
|    TEST  FUNCTION                                                                 |
-----------------------------------------------------------------------*/
static void display_sub_basic()
{
	g_print("\n");
	g_print("===========================================================================\n");
	g_print("                     media demuxer test\n");
	g_print(" SELECT : c -> (s) -> p -> (goto submenu) -> d -> q \n");
	g_print("---------------------------------------------------------------------------\n");
	g_print("c. Create \t");
	g_print("s. Set callback \t");
	g_print("p. Path \t");
	g_print("d. Destroy \t");
	g_print("q. Quit \n");
	g_print("---------------------------------------------------------------------------\n");
	if (validate_with_codec)
		g_print("[Validation with Media codec]\n");
	else
		g_print("[validation as stand alone. To validate with media codec, run mediademuxertest with -c option]\n");
}

void _interpret_main_menu(char *cmd)
{
	int len = strlen(cmd);
	if (len == 1) {
		if (strncmp(cmd, "c", 1) == 0) {
			test_mediademuxer_create();
		} else if (strncmp(cmd, "s", 1) == 0) {
			test_mediademuxer_set_error_cb();
		} else if (strncmp(cmd, "p", 1) == 0) {
			g_menu_state = CURRENT_STATUS_FILENAME;
		} else if (strncmp(cmd, "d", 1) == 0) {
			test_mediademuxer_unprepare();
			test_mediademuxer_destroy();
		} else if (strncmp(cmd, "q", 1) == 0) {
			exit(0);
		} else {
			g_print("unknown menu \n");
		}
	}
	return;
}

static void displaymenu(void)
{
	if (g_menu_state == CURRENT_STATUS_MAINMENU) {
		display_sub_basic();
	} else if (g_menu_state == CURRENT_STATUS_FILENAME) {
		g_print("*** input media path.\n");
	} else if (g_menu_state == CURRENT_STATUS_SET_DATA) {
		g_print("\n");
		g_print("=============================================================\n");
		g_print("                     media demuxer submenu\n");
		g_print(" SELECT from 1 to 9, others are option \n");
		g_print("-------------------------------------------------------------\n");
		g_print(" 1. Get Track\t");
		g_print(" 2. Select Track\n");
		g_print(" 3. Start\t");
		g_print(" 4. Get Info\n");
		g_print(" 5. Read Sample\n");
		g_print(" 6. Stop\t");
		g_print(" 7. Unprepare\t");
		g_print(" 8. Get state\n");
		g_print(" 9. Go Back to main menu\n");
		g_print(" a. Seek\t");
		g_print(" b. Uselect Track\n");
		g_print(" c. Get Sample Track Index\t");
		g_print(" d. Get Sample Track Time\n");
		g_print(" e. Advance\t");
		g_print(" f. Is Key Frame\t");
		g_print(" g. Is Key encrypted\n");
		g_print("-------------------------------------------------------------\n");
	} else {
		g_print("*** unknown status.\n");
		/*  exit(0); */
	}
	g_print(" >>> ");
}

void reset_menu_state()
{
	g_menu_state = CURRENT_STATUS_MAINMENU;
	return;
}

gboolean timeout_menu_display(void *data)
{
	displaymenu();
	return FALSE;
}

static void interpret(char *cmd)
{
	switch (g_menu_state) {
	case CURRENT_STATUS_MAINMENU: {
			_interpret_main_menu(cmd);
			break;
		}
	case CURRENT_STATUS_FILENAME: {
			int ret = 0;
			ret = test_mediademuxer_set_data_source(demuxer, cmd);
			if (ret != MD_ERROR_INVALID_ARGUMENT) {
				ret = test_mediademuxer_prepare();
				if (ret != MD_ERROR_INVALID_ARGUMENT) {
					g_menu_state = CURRENT_STATUS_SET_DATA;
				} else {
					g_print("test_mediademuxer_prepare failed \n");
					g_menu_state = CURRENT_STATUS_FILENAME;
				}
			} else {
				g_menu_state = CURRENT_STATUS_FILENAME;
			}
			break;
		}
	case CURRENT_STATUS_SET_DATA: {
			int len = strlen(cmd);
			if (len == 1) {
				if (strncmp(cmd, "1", len) == 0)
					test_mediademuxer_get_track_count();
				else if (strncmp(cmd, "2", len) == 0)
					test_mediademuxer_select_track();
				else if (strncmp(cmd, "3", len) == 0)
					test_mediademuxer_start();
				else if (strncmp(cmd, "4", len) == 0)
					test_mediademuxer_get_track_info();
				else if (strncmp(cmd, "5", len) == 0)
					test_mediademuxer_read_sample();
				else if (strncmp(cmd, "6", len) == 0)
					test_mediademuxer_stop();
				else if (strncmp(cmd, "7", len) == 0)
					test_mediademuxer_unprepare();
				else if (strncmp(cmd, "8", len) == 0)
					test_mediademuxer_get_state();
				else if (strncmp(cmd, "9", len) == 0)
					reset_menu_state();
				else if (strncmp(cmd, "a", len) == 0)
					test_mediademuxer_seek_to();
				else if (strncmp(cmd, "b", len) == 0)
					test_mediademuxer_unselect_track();
				else if (strncmp(cmd, "c", len) == 0)
					test_mediademuxer_get_sample_track_index();
				else if (strncmp(cmd, "d", len) == 0)
					test_mediademuxer_get_sample_track_time();
				else if (strncmp(cmd, "e", len) == 0)
					test_mediademuxer_advance();
				else if (strncmp(cmd, "f", len) == 0)
					test_mediademuxer_is_key_frame();
				else if (strncmp(cmd, "g", len) == 0)
					test_mediademuxer_is_encrypted();
				else
					g_print("UNKNOW COMMAND\n");
			} else if (len == 2) {
				if (strncmp(cmd, "10", len) == 0)
					g_print("UNKNOW COMMAND\n");
				else
					g_print("UNKNOW COMMAND\n");
			} else
				g_print("UNKNOW COMMAND\n");
			break;
		}
	default:
			break;
	}
	g_timeout_add(100, timeout_menu_display, 0);
}

/**
 * This function is to execute command.
 *
 * @param	channel [in]    1st parameter
 *
 * @return	This function returns TRUE/FALSE
 * @remark
 * @see
 */
gboolean input(GIOChannel *channel)
{
	gchar buf[MAX_STRING_LEN];
	gsize read;
	GError *error = NULL;
	g_io_channel_read_chars(channel, buf, MAX_STRING_LEN, &read, &error);
	buf[read] = '\0';
	g_strstrip(buf);
	interpret(buf);
	return TRUE;
}

/**
 * This function is the example main function for mediademuxer API.
 *
 * @param
 *
 * @return      This function returns 0.
 * @remark
 * @see         other functions
 */
int main(int argc, char *argv[])
{
	GIOChannel *stdin_channel;
	GMainLoop *loop = g_main_loop_new(NULL, 0);
	stdin_channel = g_io_channel_unix_new(0);
	g_io_channel_set_flags(stdin_channel, G_IO_FLAG_NONBLOCK, NULL);
	g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc) input, NULL);

	if (argc > 1) {
		/* Check whether validation with media codec is required */
		if (argv[1][0] == '-' && argv[1][1] == 'c')
			validate_with_codec = true;
	}
	displaymenu();

	g_print("RUN main loop\n");
	g_main_loop_run(loop);
	g_print("STOP main loop\n");

	g_main_loop_unref(loop);
	return 0;
}
