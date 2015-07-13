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
04/23/2015   jyotinder.ps@samsung.com      Modified
*/

/**
 * @file mediademuxer_port_gst.h
 * @brief Handling for GST Port
 */

#ifndef __TIZEN_MEDIADEMUXER_PORT_GST_H__
#define __TIZEN_MEDIADEMUXER_PORT_GST_H__

#include <tizen.h>
#include <gst/gst.h>
#include <media_format.h>
#include <gst/app/gstappsink.h>
#ifdef __cplusplus
extern "C" {
#endif

#define POLLING_INTERVAL 1000
#define MAX_APP_BUFFER 100

#define MEDIADEMUXER_SET_STATE( x_element, x_state, error ) \
	do \
	{ \
		MD_I("setting state [%s:%d] to [%s]\n", #x_state, x_state, GST_ELEMENT_NAME( x_element ) ); \
		if ( GST_STATE_CHANGE_FAILURE == gst_element_set_state ( x_element, x_state) ) \
		{ \
			MD_E("failed to set state %s to %s\n", #x_state, GST_ELEMENT_NAME( x_element )); \
			goto error; \
		} \
	}while(0)

#define MEDIADEMUXER_LINK_PAD( srcpad, sinkpad, error ) \
	do \
	{ \
		if ( GST_PAD_LINK_OK != gst_pad_link(srcpad, sinkpad) ) \
		{ \
			MD_E("failed to linkpad\n"); \
			goto error; \
		} \
	}while(0)

typedef enum {
	_GST_EVENT_TYPE_COMPLETE,
	_GST_EVENT_TYPE_ERROR,
	_GST_EVENT_TYPE_EOS,
	_GST_EVENT_TYPE_NUM
} _gst_event_e;

typedef struct track {
	GstPad *pad;
	GstCaps *caps;
	gchar *name;
	gchar *caps_string;
	GstElement *appsink;
	GstElement *fakesink;
	GstElement *queue;
	struct track *next;
} track;

typedef struct track_info {
	int num_audio_track;
	int num_video_track;
	int num_subtitle_track;
	int num_other_track;
	track *head;
} track_info;

/* GST port Private data */
typedef struct _mdgst_handle_t {
	void *hdemux;	/*< demux handle */
	int state;	/*< demux current state */
	bool is_prepared;
	GstElement *pipeline;
	GstElement *filesrc;
	GstElement *typefind;
	GstElement *demux;
	GstElement *fakesink;
	gulong signal_handoff;
	gint bus_whatch_id;
	bool is_valid_container;
	track_info info;
	int state_change_timeout;
	bool *selected_tracks;
	int total_tracks;
	GMutex *mutex;
	/* for user cb */
	void* user_cb[_GST_EVENT_TYPE_NUM];
	void* user_data[_GST_EVENT_TYPE_NUM];

} mdgst_handle_t;

/**
 * @brief Called when the error has occured.
 * @since_tizen 3.0
 * @details It will be invoked when the error has occured.
 * @param[in] error_code  The error code
 * @param[in] user_data  The user data passed from the callback registration function
 * @pre It will be invoked when the error has occured if user register this callback using mediademuxer_set_error_cb().
 * @see mediademuxer_set_error_cb()
 * @see mediademuxer_unset_error_cb()
 */
typedef void (*gst_error_cb)(mediademuxer_error_e error, void *user_data);

#ifdef __cplusplus
}
#endif
#endif /* __TIZEN_MEDIADEMUXER_PORT_GST_H__ */
