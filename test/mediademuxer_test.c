/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
/*
 * Contact: Satheesan E N <satheesan.en@samsung.com>,
 EDIT HISTORY FOR MODULE
    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order.
when            who                                              what, where, why
---------   ------------------------   ----------------------------------------------------------
0423/2015   jyotinder.ps@samsung.com      created
0315/2015   jk7704.seo@samsung.com        Modified
 */

/*=======================================================================
|  INCLUDE FILES                                                                        |
========================================================================*/
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <mm_error.h>
#include <mm_debug.h>
#include <mediademuxer_error.h>
#include <mediademuxer.h>
#include <media_format.h>
#include <media_packet.h>

/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
#define MAX_STRING_LEN 100
#define PACKAGE "mediademuxer_test"

/*-----------------------------------------------------------------------
|    GLOBAL CONSTANT DEFINITIONS:                                       |
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    IMPORTED VARIABLE DECLARATIONS:                                    |
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    IMPORTED FUNCTION DECLARATIONS:                                    |
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    LOCAL #defines:                                                                   |
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
		}\
	} while(0)

#define debug_msg_t(fmt,arg...)\
	do { \
		fprintf(stderr, MMF_DEBUG"[%s:%05d]  " fmt "\n",__func__, __LINE__, ##arg); \
	} while(0)

#define err_msg_t(fmt,arg...)\
	do { \
		fprintf(stderr, MMF_ERR"[%s:%05d]  " fmt "\n",__func__, __LINE__, ##arg); \
	} while(0)

#define info_msg_t(fmt,arg...)\
	do { \
		fprintf(stderr, MMF_INFO"[%s:%05d]  " fmt "\n",__func__, __LINE__, ##arg); \
	} while(0)

#define warn_msg_t(fmt,arg...)\
	do { \
		fprintf(stderr, MMF_WARN"[%s:%05d]  " fmt "\n",__func__, __LINE__, ##arg); \
	} while(0)

/*-----------------------------------------------------------------------
|    LOCAL CONSTANT DEFINITIONS:                                        |
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    LOCAL DATA TYPE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:                                        |
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:                                        |
-----------------------------------------------------------------------*/
#if 0
int test_mediademuxer_create();
int test_mediademuxer_destroy();
int test_mediademuxer_prepare();
int test_mediademuxer_unprepare();
int test_mediademuxer_set_data_source();
int test_mediademuxer_get_track_count();
int test_mediademuxer_get_track_info();
int test_mediademuxer_select_track();
int test_mediademuxer_unselect_track();
int test_mediademuxer_seek_to();
int test_mediademuxer_read_sample();
int test_mediademuxer_get_sample_track_index();
int test_mediademuxer_get_sample_track_time();
int test_mediademuxer_advance();
int test_mediademuxer_is_key_frame();
int test_mediademuxer_is_encrypted();
int test_mediademuxe_audio_only();
int test_mediademuxe_video_only();
int test_mediademuxe_audio_video_only();
#endif
int aud_track=-1;
int vid_track=-1;
mediademuxer_h demuxer = NULL;
int num_tracks = 0;
media_format_h *g_media_format = NULL;

int test_mediademuxer_get_track_info()
{
	int ret = 0;
	int track = 0;
	int w;
	int h;
	int channel;
	int samplerate;
	media_format_mimetype_e mime;
	g_print("test_mediademuxer_get_track_info\n");
	g_media_format =
	    (media_format_h *) g_malloc(sizeof(media_format_h) * num_tracks);
	g_print("allocated %p memory for g_media_format\n", g_media_format);
	if (g_media_format) {
		for (; track < num_tracks; track++) {
			ret = media_format_create(&g_media_format[track]);
			if (ret == 0) {
				g_print
				("g_media_format[%d] is created successfully! \n",
				 track);
				ret =
				    mediademuxer_get_track_info(demuxer, track,
				                                g_media_format[track]);
				if (ret == 0) {
					if (media_format_get_video_info
					    (g_media_format[track], &mime, &w,
					     &h, NULL,
					     NULL) == MEDIA_FORMAT_ERROR_NONE) {
						g_print
						("media_format_get_video_info is sucess!\n");
						g_print
						("\t\t[media_format_get_video]mime:%x, width :%d, height :%d\n",
						 mime, w, h);
						vid_track = track;
					} else if (media_format_get_audio_info
					           (g_media_format[track], &mime,
					            &channel, &samplerate, NULL,
					            NULL) ==
					           MEDIA_FORMAT_ERROR_NONE) {
						g_print
						("media_format_get_audio_info is sucess!\n");
						g_print
						("\t\t[media_format_get_audio]mime:%x, channel :%d, samplerate :%d\n",
						 mime, channel, samplerate);
						aud_track = track;
					} else {
						g_print("Not Supported YET");
					}
				} else {
					g_print
					("Error while getting mediademuxer_get_track_info\n");
				}
			} else {
				g_print
				("Error while creating media_format_create\n");
			}
		}
	} else {
		g_print("Error while allocating memory\n");
	}

	return ret;
}

int test_mediademuxer_create()
{
	int ret = 0;
	g_print("test_mediademuxer_create\n");
	ret = mediademuxer_create(&demuxer);
	return ret;
}

int test_mediademuxer_destroy()
{
	int ret = 0;
	g_print("test_mediademuxer_destroy\n");
	ret = mediademuxer_destroy(demuxer);
	demuxer = NULL;
	return ret;
}

int test_mediademuxer_prepare()
{
	int ret = 0;
	g_print("test_mediademuxer_prepare\n");
	ret = mediademuxer_prepare(demuxer);
	return ret;
}

int test_mediademuxer_start()
{
	int ret = 0;
	g_print("test_mediademuxer_start\n");
	ret = mediademuxer_start(demuxer);
	return ret;
}

int test_mediademuxer_get_state()
{
	g_print("test_mediademuxer_get_state\n");
	mediademuxer_state state;
	if(mediademuxer_get_state(demuxer, &state) == MEDIADEMUXER_ERROR_NONE) {
		g_print("\nMediademuxer_state=%d",state);
	}
	else {
		g_print("\nMediademuxer_state call failed\n");
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

int test_mediademuxer_set_data_source(mediademuxer_h demuxer, const char *path)
{
	int ret = 0;
	g_print("test_mediademuxer_set_data_source\n");
	ret = mediademuxer_set_data_source(demuxer, path);
	return ret;
}

int test_mediademuxer_get_track_count()
{
	g_print("test_mediademuxer_get_track_count\n");
	mediademuxer_get_track_count(demuxer, &num_tracks);
	g_print("Number of tracks [%d]", num_tracks);
	return 0;
}

int test_mediademuxer_select_track()
{
	g_print("test_mediademuxer_select_track\n");
	if (mediademuxer_select_track(demuxer, 0)) {
		g_print("mediademuxer_select track 0 failed\n");
		return -1;
	}
	if (mediademuxer_select_track(demuxer, 1)) {
		g_print("mediademuxer_select track 1 failed\n");
		return -1;
	}

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

int test_mediademuxer_seek_to()
{
	g_print("test_mediademuxer_seek_to\n");
	int64_t pos = 1;
	mediademuxer_seek(demuxer, pos);
	g_print("Number of tracks [%d]", num_tracks);
	return 0;
}

void app_err_cb(mediademuxer_error_e error, void *user_data)
{
	printf("Got Error %d from Mediademuxer\n",error);
}

int test_mediademuxer_set_error_cb()
{
	int ret = 0;
	g_print("test_mediademuxer_set_error_cb\n");
	ret = mediademuxer_set_error_cb(demuxer,app_err_cb,demuxer);
	return ret;
}

void *_fetch_audio_data(void *ptr)
{
	int *status = (int *)g_malloc(sizeof(int) * 1);
	*status = -1;
	g_print("Audio Data function\n");
	media_packet_h audbuf;
	media_format_h audfmt;
	int count = 0;
	if (media_format_create(&audfmt)) {
		g_print("media_format_create failed\n");
		return (void *)status;
	}
	if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AAC)) {
		g_print("media_format_set_audio_mime failed\n");
		return (void *)status;
	}
	if (media_packet_create_alloc(audfmt, NULL, NULL, &audbuf)) {
		g_print("media_packet_create_alloc failed\n");
		return (void *)status;
	}

	while (1) {
		int EOS = mediademuxer_read_sample(demuxer, aud_track, audbuf);
		if (EOS == MD_EOS || EOS != MD_ERROR_NONE)
			pthread_exit(NULL);
		count++;
		g_print("Read::[%d] audio sample\n", count);
		media_packet_destroy(audbuf);
		if (media_packet_create_alloc(audfmt, NULL, NULL, &audbuf)) {
			g_print("media_packet_create_alloc failed\n");
			break;
		}
	}

	*status = 0;

	return (void *)status;
}

void *_fetch_video_data(void *ptr)
{
	int *status = (int *)g_malloc(sizeof(int) * 1);
	*status = -1;
	g_print("Video Data function\n");
	int count = 0;
	media_packet_h vidbuf;
	media_format_h vidfmt;
	if (media_format_create(&vidfmt)) {
		g_print("media_format_create failed\n");
		return (void *)status;
	}
	if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_H264_SP)) {
		g_print("media_format_set_video_mime failed\n");
		return (void *)status;
	}
	if (media_format_set_video_width(vidfmt, 760)) {
		g_print("media_format_set_video_width failed\n");
		return (void *)status;
	}
	if (media_format_set_video_height(vidfmt, 480)) {
		g_print("media_format_set_video_height failed\n");
		return (void *)status;
	}

	if (media_packet_create_alloc(vidfmt, NULL, NULL, &vidbuf)) {
		g_print("media_packet_create_alloc failed\n");

	}

	while (1) {
		int EOS = mediademuxer_read_sample(demuxer, vid_track, vidbuf);
		if (EOS == MD_EOS || EOS != MD_ERROR_NONE)
			pthread_exit(NULL);
		count++;
		g_print("Read::[%d] video sample\n", count);
		media_packet_destroy(vidbuf);
		if (media_packet_create_alloc(vidfmt, NULL, NULL, &vidbuf)) {
			g_print("media_packet_create_alloc failed\n");
			break;
		}
	}
	*status = 0;
	return (void *)status;
}

int test_mediademuxer_read_sample()
{
	pthread_t thread[2];
	pthread_attr_t attr;
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(vid_track != -1){
		g_print("In main: creating thread  for video\n");
		pthread_create(&thread[0], &attr, _fetch_video_data, NULL);
	}
	if(aud_track != -1){
		g_print("In main: creating thread  for audio\n");
		pthread_create(&thread[1], &attr, _fetch_audio_data, NULL);
	}
	pthread_attr_destroy(&attr);
	return 0;
}

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

enum {
	CURRENT_STATUS_MAINMENU,
	CURRENT_STATUS_FILENAME,
	CURRENT_STATUS_SET_DATA
};

int g_menu_state = CURRENT_STATUS_MAINMENU;

static void display_sub_basic();

void reset_menu_state()
{
	g_menu_state = CURRENT_STATUS_MAINMENU;
	return;
}

void _interpret_main_menu(char *cmd)
{
	int len = strlen(cmd);
	if (len == 1) {
		if (strncmp(cmd, "a", 1) == 0) {
			test_mediademuxer_create();
		} else if (strncmp(cmd, "d", 1) == 0) {
			test_mediademuxer_unprepare();
			test_mediademuxer_destroy();
		} else if (strncmp(cmd, "s", 1) == 0) {
			g_menu_state = CURRENT_STATUS_FILENAME;
		} else if (strncmp(cmd, "q", 1) == 0) {
			exit(0);
		} else if (strncmp(cmd, "c", 1) == 0) {
			test_mediademuxer_set_error_cb();
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
		g_print("*** input mediapath.\n");
	} else if (g_menu_state == CURRENT_STATUS_SET_DATA) {
		g_print("\n");
		g_print
		("=========================================================================================\n");
		g_print
		("                                    media demuxer submenu\n");
		g_print
		("-----------------------------------------------------------------------------------------\n");
		g_print("1.Get Track\t");
		g_print("2.Get Info\t");
		g_print("3.Select Track\t");
		g_print("4.Uselect Track\n");
		g_print("5.Seek\t");
		g_print("6.Read Sample\t");
		g_print("7.Get Sample Track Index\t");
		g_print("8.Get Sample Track Time\n");
		g_print("9.Advance\t");
		g_print("10.Is Key Frame\t");
		g_print("11.Is Key encrypted\t");
		g_print("a. start\n");
		g_print("b. stop\n");
		g_print("u. unprepare\n");
		g_print("s. get state\n");
		g_print("12.Go Back to main menu\n");
		g_print
		("-----------------------------------------------------------------------------------------\n");
	} else {
		g_print("*** unknown status.\n");
		/*  exit(0); */
	}
	g_print(" >>> ");
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
						g_print
						("test_mediademuxer_prepare failed \n");
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
					if (strncmp(cmd, "1", len) == 0) {
						test_mediademuxer_get_track_count();
					} else if (strncmp(cmd, "2", len) == 0) {
						test_mediademuxer_get_track_info();
					} else if (strncmp(cmd, "3", len) == 0) {
						test_mediademuxer_select_track();
					} else if (strncmp(cmd, "4", len) == 0) {
						test_mediademuxer_unselect_track();
					} else if (strncmp(cmd, "5", len) == 0) {
						test_mediademuxer_seek_to();
					} else if (strncmp(cmd, "6", len) == 0) {
						test_mediademuxer_read_sample();
					} else if (strncmp(cmd, "7", len) == 0) {
						test_mediademuxer_get_sample_track_index
						();
					} else if (strncmp(cmd, "8", len) == 0) {
						test_mediademuxer_get_sample_track_time
						();
					} else if (strncmp(cmd, "9", len) == 0) {
						test_mediademuxer_advance();
					} else if (strncmp(cmd, "a", len) == 0) {
						test_mediademuxer_start();
					} else if (strncmp(cmd, "b", len) == 0) {
						test_mediademuxer_stop();
					} else if (strncmp(cmd, "u", len) == 0) {
						test_mediademuxer_unprepare();
					} else if (strncmp(cmd, "s", len) == 0) {
						test_mediademuxer_get_state();
					}
				} else if (len == 2) {
					if (strncmp(cmd, "10", len) == 0) {
						test_mediademuxer_is_key_frame();
					} else if (strncmp(cmd, "11", len) == 0) {
						test_mediademuxer_is_encrypted();
					} else if (strncmp(cmd, "12", len) == 0) {
						reset_menu_state();
					} else {
						g_print("UNKNOW COMMAND\n");
					}
				} else {
					g_print("UNKNOW COMMAND\n");
				}
				break;
			}
		default:
			break;
	}
	g_timeout_add(100, timeout_menu_display, 0);
}

static void display_sub_basic()
{
	g_print("\n");
	g_print
	("=========================================================================================\n");
	g_print("                                    media demuxer test\n");
	g_print
	("-----------------------------------------------------------------------------------------\n");
	g_print("a. Create \t");
	g_print("c. Set error_call_back\t");
	g_print("s. Setdata \t\t");
	g_print("d. Destroy \t\t");
	g_print("q. exit \t\t");
	g_print("\n");
	g_print
	("=========================================================================================\n");
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

	displaymenu();

	g_print("RUN main loop\n");
	g_main_loop_run(loop);
	g_print("STOP main loop\n");

	g_main_loop_unref(loop);
	return 0;
}
