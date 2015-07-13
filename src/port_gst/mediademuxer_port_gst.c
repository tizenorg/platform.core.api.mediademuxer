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
 * @file mediademuxer_port_gst.c
 * @brief Handling for GStreamer Port, defined function and there implementation
 */

#include <mm_debug.h>
#include <unistd.h>
#include <mediademuxer_error.h>
#include <mediademuxer_private.h>
#include <mediademuxer_port.h>
#include <mediademuxer_port_gst.h>

static int gst_demuxer_init(MMHandleType *pHandle);
static int gst_demuxer_prepare(MMHandleType pHandle, char *uri);
static int gst_demuxer_unprepare(MMHandleType pHandle);
static int gst_demuxer_get_data_count(MMHandleType pHandle, int *count);
static int gst_demuxer_get_track_info(MMHandleType pHandle,
                                      media_format_h format, int index);
static int gst_demuxer_set_track(MMHandleType pHandle, int track);
static int gst_demuxer_unset_track(MMHandleType pHandle, int track);
static int gst_demuxer_get_data(MMHandleType pHandle, char *buffer);
static int gst_demuxer_destroy(MMHandleType pHandle);
static int gst_demuxer_start(MMHandleType pHandle);
static int gst_demuxer_read_sample(MMHandleType pHandle,
                                   media_packet_h outbuf, int track_indx);
static int gst_demuxer_stop(MMHandleType pHandle);
static int _gst_copy_buf_to_media_packet(media_packet_h outbuf,
                                         GstBuffer *buffer);
static int gst_demuxer_seek(MMHandleType pHandle, gint64 pos1);
static int gst_set_error_cb(MMHandleType pHandle,
			gst_error_cb callback, void* user_data);

/*Media Demuxer API common*/
static media_port_demuxer_ops def_demux_ops = {
	.n_size = 0,
	.init = gst_demuxer_init,
	.prepare = gst_demuxer_prepare,
	.set_track = gst_demuxer_set_track,
	.unset_track = gst_demuxer_unset_track,
	.get_track_count = gst_demuxer_get_data_count,
	.get_track_info = gst_demuxer_get_track_info,
	.get_data = gst_demuxer_get_data,
	.destroy = gst_demuxer_destroy,
	.start = gst_demuxer_start,
	.read_sample = gst_demuxer_read_sample,
	.stop = gst_demuxer_stop,
	.unprepare = gst_demuxer_unprepare,
	.seek = gst_demuxer_seek,
	.set_error_cb = gst_set_error_cb,
};

int gst_port_register(media_port_demuxer_ops *pOps)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pOps);
	def_demux_ops.n_size = sizeof(def_demux_ops);

	memcpy((char *)pOps + sizeof(pOps->n_size),
	       (char *)&def_demux_ops + sizeof(def_demux_ops.n_size),
	       pOps->n_size - sizeof(pOps->n_size));

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_init(MMHandleType *pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	mdgst_handle_t *new_mediademuxer = NULL;
	new_mediademuxer = (mdgst_handle_t *) g_malloc(sizeof(mdgst_handle_t));
	MD_I("gst_demuxer_init allocating new_mediademuxer:%p\n",
	     new_mediademuxer);
	if (!new_mediademuxer) {
		MD_E("Cannot allocate memory for demuxer\n");
		ret = MD_ERROR;
		goto ERROR;
	}
	memset(new_mediademuxer, 0, sizeof(mdgst_handle_t));
	new_mediademuxer->is_prepared = false;
	(new_mediademuxer->info).num_audio_track = 0;
	(new_mediademuxer->info).num_video_track = 0;
	(new_mediademuxer->info).num_subtitle_track = 0;
	(new_mediademuxer->info).num_other_track = 0;
	(new_mediademuxer->info).head = NULL;
	*pHandle = (MMHandleType) new_mediademuxer;
	new_mediademuxer->total_tracks = 0;
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_destroy(MMHandleType pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;
	MD_I("gst_demuxer_destroy deallocating new_mediademuxer:%p\n",
	     new_mediademuxer);
	g_free(new_mediademuxer);
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

gboolean __gst_bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_EOS: {
				MD_I("EOS Reached");
			}
			break;
		case GST_MESSAGE_ERROR: {
				GError *error = NULL;
				gst_message_parse_error(msg, &error, NULL);
				if (!error) {
					MD_I("GST error message parsing failed");
					break;
				}
				MD_E("Error: %s\n", error->message);
				g_error_free(error);
			}
			break;
		default:
			break;
	}
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static void __gst_no_more_pad(GstElement *element, gpointer data)
{
	MEDIADEMUXER_FENTER();
	mdgst_handle_t *core = (mdgst_handle_t *) data;
	int loop_track;
	track_info *head_track = &(core->info);
	track *head = head_track->head;
	core->selected_tracks =
	    (bool *) g_malloc(sizeof(bool) * (core->total_tracks));
	MD_I("Allocating %p to core->selected_tracks \n", core->selected_tracks);
	if (!core->selected_tracks) {
		MD_E("[%s]Memory allocation failed\n", __FUNCTION__);
		return;
	} else {
		for (loop_track = 0; loop_track < core->total_tracks;
		     loop_track++)
			core->selected_tracks[loop_track] = false;
	}
	MD_I("Number of video tracks are %d\n", head_track->num_video_track);
	MD_I("Number of audio tracks are %d\n", head_track->num_audio_track);
	MD_I("Number of subtitle tracks are %d\n",
	     head_track->num_subtitle_track);
	MD_I("Number of other tracks are %d\n", head_track->num_other_track);
	while (head) {
		MD_I("track caps[%s]\n", head->name);
		head = head->next;
	}
	core->is_prepared = true;
	MD_I("core->is_prepared: ");
	core->is_prepared ? MD_I("true\n") : MD_I("false\n");
	MEDIADEMUXER_FLEAVE();
}

void __gst_free_stuct(track **head)
{
	MEDIADEMUXER_FENTER();
	track *temp = NULL;
	temp = *head;
	while (temp) {
		/*
		if (temp->pad) {
		MD_I("deallocate GST_PAD %p\n", temp->pad);
		gst_object_unref(temp->pad);
		} */
		if (temp->name) {
			MD_I("deallocate GST_PAD name  %p\n", temp->name);
			g_free(temp->name);
		}
		if (temp->caps_string) {
			MD_I("deallocate GST_PAD caps_string  %p\n",
			     temp->caps_string);
			g_free(temp->caps_string);
		}
		if (temp->caps) {
			MD_I("deallocate GST_PAD caps_  %p\n", temp->caps);
			gst_caps_unref(temp->caps);
		}

		if (temp->next) {
			track *next = temp->next;
			MD_I("deallocate memory %p\n", temp);
			free(temp);
			temp = next;
		} else {
			MD_I("deallocate memory %p\n", temp);
			free(temp);
			temp = NULL;
			*head = NULL;
		}
	}
	MEDIADEMUXER_FLEAVE();
}

int __gst_add_track_info(GstPad *pad, gchar *name, track **head,
                         GstElement *pipeline)
{
	MEDIADEMUXER_FENTER();
	GstPad *apppad = NULL;
	track *temp = NULL;
	temp = (track *)(g_malloc(sizeof(track)));
	if (!temp) {
		MD_E("Not able to allocate memory");
		return MD_ERROR;
	} else {
		MD_I("allocate memory %p", temp);
	}
	temp->pad = pad;
	temp->caps = gst_pad_get_current_caps(pad);
	temp->name = name;
	temp->caps_string = gst_caps_to_string(temp->caps);
	temp->next = NULL;
	temp->appsink = gst_element_factory_make("appsink", NULL);
	if (!temp->appsink) {
		MD_E("factory not able to make appsink");
		__gst_free_stuct(head);
		return MD_ERROR;
	}
	gst_bin_add_many(GST_BIN(pipeline), temp->appsink, NULL);
	gst_app_sink_set_max_buffers((GstAppSink *) temp->appsink,
	                             MAX_APP_BUFFER);
	MEDIADEMUXER_SET_STATE(temp->appsink, GST_STATE_PAUSED, ERROR);
	apppad = gst_element_get_static_pad(temp->appsink, "sink");
	if (!apppad) {
		MD_E("sink pad of appsink not avaible");
		__gst_free_stuct(head);
		return MD_ERROR;
	}
	MEDIADEMUXER_LINK_PAD(pad, apppad, ERROR);
	/*gst_pad_link(pad, fpad); */
	if (*head == NULL) {
		*head = temp;
	} else {
		track *prev = *head;
		while (prev->next) {
			prev = prev->next;
		}
		prev->next = temp;
	}
	gst_object_unref(apppad);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	if (apppad)
		gst_object_unref(apppad);
	__gst_free_stuct(head);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static void __gst_on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
	MEDIADEMUXER_FENTER();
	MD_I("Dynamic pad created, linking demuxer/decoder\n");
	mdgst_handle_t *core = (mdgst_handle_t *) data;
	track_info *head_track = &(core->info);
	gchar *name = gst_pad_get_name(pad);
	core->total_tracks++;
	if (__gst_add_track_info(pad, name, &(head_track->head), core->pipeline)
	    != MD_ERROR_NONE) {
		MD_E("unable to added track info");
		head_track->num_audio_track = 0;
		head_track->num_video_track = 0;
		head_track->num_subtitle_track = 0;
		head_track->num_other_track = 0;
		__gst_free_stuct(&(head_track->head));
		return;
	}
	if (name[0] == 'v') {
		MD_I("found Video Pad\n");
		(head_track->num_video_track)++;
	} else if (name[0] == 'a') {
		MD_I("found Audio Pad\n");
		(head_track->num_audio_track)++;
	} else if (name[0] == 's') {
		MD_I("found subtitle Pad\n");
		(head_track->num_subtitle_track)++;
	} else {
		MD_I("found Pad %s\n", name);
		(head_track->num_other_track)++;
	}
	MEDIADEMUXER_FLEAVE();
}

static int __gst_create_audio_only_pipeline(gpointer data,  GstCaps *caps)
{
	MEDIADEMUXER_FENTER();
	GstPad *pad = NULL;
	GstPad *aud_pad = NULL;
	GstPad *aud_srcpad = NULL;
	GstPad *fake_pad = NULL;
	GstElement *id3tag = NULL;
	mdgst_handle_t *core = (mdgst_handle_t *) data;
	gchar *name;
	gchar *type;
	track_info *head_track = &(core->info);
	track *trck;
	core->is_valid_container = true;
	type = gst_caps_to_string(caps);
	if (strstr(type, "adts")) {
		core->demux = gst_element_factory_make("aacparse", NULL);
	} else if (strstr(type, "audio/mpeg")) {
		core->demux = gst_element_factory_make("mpegaudioparse", NULL);
	} else if (strstr(type, "application/x-id3")) {
		id3tag = gst_element_factory_make("id3demux", NULL);
		core->demux = gst_element_factory_make("mpegaudioparse", NULL);
	} else if (strstr(type, "audio/x-amr-nb-sh")
		   || strstr(type, "audio/x-amr-wb-sh")) {
		core->demux = gst_element_factory_make("amrparse", NULL);
	}
	g_free(type);
	if (!core->demux) {
		core->is_valid_container = false;
		MD_E("factory not able to create audio parse element\n");
		goto ERROR;
	} else {
		gst_bin_add_many(GST_BIN(core->pipeline),
				 core->demux, id3tag, NULL);
		pad = gst_element_get_static_pad(core->typefind, "src");
		if (!pad) {
			MD_E("fail to get typefind src pad.\n");
			goto ERROR;
		}
		if (!id3tag) {
			aud_pad =
			    gst_element_get_static_pad(core->demux, "sink");
		} else {
			aud_pad = gst_element_get_static_pad(id3tag, "sink");
		}
		if (!aud_pad) {
			MD_E("fail to get audio parse sink pad.\n");
			goto ERROR;
		}
		fake_pad = gst_element_get_static_pad(core->fakesink, "sink");
		if (!fake_pad) {
			MD_E("fail to get fakesink sink pad.\n");
			goto ERROR;
		}
		gst_pad_unlink(pad, fake_pad);
		MEDIADEMUXER_LINK_PAD(pad, aud_pad, ERROR);
		if (!id3tag) {
			MEDIADEMUXER_SET_STATE(core->demux,
					       GST_STATE_PAUSED, ERROR);
		} else {
			MEDIADEMUXER_SET_STATE(id3tag, GST_STATE_PAUSED, ERROR);
			MEDIADEMUXER_SET_STATE(core->demux,
					       GST_STATE_PAUSED, ERROR);
			gst_element_link(id3tag, core->demux);
		}
		if (pad)
			gst_object_unref(pad);
		if (aud_pad)
			gst_object_unref(aud_pad);
		if (fake_pad)
			gst_object_unref(fake_pad);
	}

	/* calling "on_pad_added" function to set the caps*/
	aud_srcpad = gst_element_get_static_pad(core->demux, "src");
	if (!aud_srcpad) {
		MD_E("fail to get audioparse source pad.\n");
		goto ERROR;
	}
	core->total_tracks++;
	name = gst_pad_get_name(aud_srcpad);
	if (__gst_add_track_info(aud_srcpad, name, &(head_track->head), core->pipeline)
	    != MD_ERROR_NONE) {
		MD_E("unable to added track info");
		head_track->num_audio_track = 0;
		head_track->num_video_track = 0;
		head_track->num_subtitle_track = 0;
		head_track->num_other_track = 0;
		__gst_free_stuct(&(head_track->head));
		goto ERROR;
	}
	if (aud_srcpad)
		gst_object_unref(aud_srcpad);

	trck = head_track->head;
	while (aud_srcpad != trck->pad && trck != NULL) {
		trck = trck->next;
	}
	if (trck != NULL) {
		trck->caps = caps;
		trck->caps_string = gst_caps_to_string(trck->caps);
		MD_I("caps set to %s\n",trck->caps_string);
		g_strlcpy(name,"audio",strlen(name));
		trck->name = name;
	}
	(head_track->num_audio_track)++;
	__gst_no_more_pad(core->demux, data);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	core->is_valid_container = false;
	if (core->demux)
		gst_object_unref(core->demux);
	if (pad)
		gst_object_unref(pad);
	if (aud_pad)
		gst_object_unref(aud_pad);
	if (fake_pad)
		gst_object_unref(fake_pad);
	if (aud_srcpad)
		gst_object_unref(aud_srcpad);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static void __gst_cb_typefind(GstElement *tf, guint probability,
                  GstCaps *caps, gpointer data)
{
	GstPad *pad = NULL;
	GstPad *qt_pad = NULL;
	GstPad *fake_pad = NULL;
	mdgst_handle_t *core = (mdgst_handle_t *) data;
	MEDIADEMUXER_FENTER();
	gchar *type;
	type = gst_caps_to_string(caps);
	if (type) {
		MD_I("Media type %s found, probability %d%%\n", type,
		     probability);
		if (strstr(type, "quicktime")) {
			core->is_valid_container = true;
			core->demux = gst_element_factory_make("qtdemux", NULL);
			if (!core->demux) {
				core->is_valid_container = false;
				MD_E("factory not able to create qtdemux\n");
				goto ERROR;
			} else {
				g_signal_connect(core->demux, "pad-added",
				                 G_CALLBACK(__gst_on_pad_added),
				                 core);
				g_signal_connect(core->demux, "no-more-pads",
				                 G_CALLBACK(__gst_no_more_pad),
				                 core);
				gst_bin_add_many(GST_BIN(core->pipeline),
				                 core->demux, NULL);
				pad = gst_element_get_static_pad(core->typefind, "src");
				if (!pad) {
					MD_E("fail to get typefind src pad.\n");
					goto ERROR;
				}
				qt_pad = gst_element_get_static_pad(core->demux, "sink");
				if (!qt_pad) {
					MD_E("fail to get qtdemuc sink pad.\n");
					goto ERROR;
				}
				fake_pad = gst_element_get_static_pad(core->fakesink, "sink");
				if (!fake_pad) {
					MD_E("fail to get fakesink sink pad.\n");
					goto ERROR;
				}
				gst_pad_unlink(pad, fake_pad);
				MEDIADEMUXER_LINK_PAD(pad, qt_pad, ERROR);
				MEDIADEMUXER_SET_STATE(core->demux,
				                       GST_STATE_PAUSED, ERROR);
				if (pad)
					gst_object_unref(pad);
				if (qt_pad)
					gst_object_unref(qt_pad);
				if (fake_pad)
					gst_object_unref(fake_pad);
			}
		} else if ((strstr(type, "adts"))
			   || (strstr(type, "audio/mpeg"))
			   || strstr(type, "audio/x-amr-nb-sh")
			   || strstr(type, "application/x-id3")
			   || (strstr(type, "audio/x-amr-wb-sh"))) {
			MD_I("Audio only format is found\n");
			__gst_create_audio_only_pipeline(data, caps);
		} else {
			core->is_valid_container = false;
			MD_E("Not supported container %s\n", type);
		}
		g_free(type);
	}
	MEDIADEMUXER_FLEAVE();
	return;
ERROR:
	core->is_valid_container = false;
	if (core->demux)
		gst_object_unref(core->demux);
	if (type)
		g_free(type);
	if (pad)
		gst_object_unref(pad);
	if (qt_pad)
		gst_object_unref(qt_pad);
	if (fake_pad)
		gst_object_unref(fake_pad);
	MEDIADEMUXER_FLEAVE();
	return;
}

#if 0
int __gst_set_playing(mdgst_handle_t *data)
{
	MEDIADEMUXER_FENTER();
	mdgst_handle_t *core = (mdgst_handle_t *) data;
	track_info *head_track = &(core->info);
	track *head = head_track->head;
	while (head) {
		gst_element_set_state(head->fakesink, GST_STATE_PLAYING);
		head = head->next;
	}
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
}
#endif

static int _gst_create_pipeline(mdgst_handle_t *core, char *uri)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	GstBus *bus = NULL;
	gst_init(NULL, NULL);
	core->pipeline = gst_pipeline_new(NULL);
	if (!core->pipeline) {
		MD_E("pipeline create fail");
		ret = MD_ERROR;
		goto ERROR;
	}
	core->filesrc = gst_element_factory_make("filesrc", NULL);
	if (!core->filesrc) {
		MD_E("filesrc creation failed");
		ret = MD_ERROR;
		goto ERROR;
	}
	g_object_set(G_OBJECT(core->filesrc), "location", uri + 7, NULL);
	core->typefind = gst_element_factory_make("typefind", NULL);
	if (!core->typefind) {
		MD_E("typefind creation failed");
		ret = MD_ERROR;
		goto ERROR;
	}
	g_signal_connect(core->typefind, "have-type",
	                 G_CALLBACK(__gst_cb_typefind), core);
	core->fakesink = gst_element_factory_make("fakesink", NULL);
	if (!core->fakesink) {
		MD_E("fakesink creation failed");
		ret = MD_ERROR;
		goto ERROR;
	}

	gst_bin_add_many(GST_BIN(core->pipeline), core->filesrc, core->typefind,
	                 core->fakesink, NULL);
	gst_element_link_many(core->filesrc, core->typefind, core->fakesink,
	                      NULL);

	bus = gst_pipeline_get_bus(GST_PIPELINE(core->pipeline));
	core->bus_whatch_id = gst_bus_add_watch(bus, __gst_bus_call, core);
	gst_object_unref(bus);

	MEDIADEMUXER_SET_STATE(core->pipeline, GST_STATE_PAUSED, ERROR);

	int count = 0;
	while (core->is_prepared != true) {
		count++;
		usleep(POLLING_INTERVAL);
		MD_I("Inside while loop\n");
		if (count > POLLING_INTERVAL) {
			MD_E("Error occure\n");
			ret = MD_ERROR;
		}
	}

	/*    __gst_set_playing(core);*/
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_prepare(MMHandleType pHandle, char *uri)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	MD_I("gst_demuxer_prepare Creating pipeline %p", new_mediademuxer);
	ret = _gst_create_pipeline(new_mediademuxer, uri);
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_get_data_count(MMHandleType pHandle, int *count)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	*count = (new_mediademuxer->info).num_video_track +
	         (new_mediademuxer->info).num_audio_track +
	         (new_mediademuxer->info).num_subtitle_track +
	         (new_mediademuxer->info).num_other_track;
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

int _set_mime_video(media_format_h format, track *head)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	GstStructure *struc = NULL;
	int src_width;
	int src_height;
	struc = gst_caps_get_structure(head->caps, 0);
	if (!struc) {
		MD_E("cannot get structure from caps.\n");
		goto ERROR;
	}
	if (gst_structure_has_name(struc, "video/x-h264")) {
		const gchar *version =
		    gst_structure_get_string(struc, "stream-format");
		if (strncmp(version, "avc", 3) == 0) {
			gst_structure_get_int(struc, "width", &src_width);
			gst_structure_get_int(struc, "height", &src_height);
			if (media_format_set_video_mime
			    (format,
			     MEDIA_FORMAT_H264_SP) != MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if (media_format_set_video_width(format, src_width) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if (media_format_set_video_height(format, src_height) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if (media_format_set_video_avg_bps(format, 0) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if (media_format_set_video_max_bps(format, 0) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
		}

	} else {
		MD_I("Not Supported YET\n");
	}
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

int _set_mime_audio(media_format_h format, track *head)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	GstStructure *struc = NULL;
	int rate = 0;
	int channels = 0;
	int id3_flag = 0;
	struc = gst_caps_get_structure(head->caps, 0);
	if (!struc) {
		MD_E("cannot get structure from caps.\n");
		goto ERROR;
	}

	if (gst_structure_has_name(struc, "application/x-id3")) {
		id3_flag = 1;
	}
	if (gst_structure_has_name(struc, "audio/mpeg") || id3_flag) {
		gint mpegversion;
		int layer;
		gst_structure_get_int(struc, "mpegversion", &mpegversion);
		if (mpegversion == 4 || mpegversion == 2 ) {
			gst_structure_get_int(struc, "channels", &channels);
			gst_structure_get_int(struc, "rate", &rate);
			if (media_format_set_audio_mime
			    (format,
			     MEDIA_FORMAT_AAC_LC) != MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if(channels == 0)
				channels = 2; /* default */
			if (media_format_set_audio_channel(format, channels) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if(rate == 0)
				rate = 44100; /* default */
			if (media_format_set_audio_samplerate(format, rate) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if (media_format_set_audio_avg_bps(format, 0) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
			if (media_format_set_audio_bit(format, 0) !=
			    MEDIA_FORMAT_ERROR_NONE)
				goto ERROR;
		}
		if (mpegversion == 1 || id3_flag ) {
			gst_structure_get_int(struc, "layer", &layer);
			if((layer == 3) || (id3_flag == 1)) {
				gst_structure_get_int(struc, "channels", &channels);
				gst_structure_get_int(struc, "rate", &rate);
				if (media_format_set_audio_mime
				    (format,
				     MEDIA_FORMAT_MP3) != MEDIA_FORMAT_ERROR_NONE)
					goto ERROR;
				if(channels == 0)
					channels = 2; /* default */
				if (media_format_set_audio_channel(format, channels) !=
				    MEDIA_FORMAT_ERROR_NONE)
					goto ERROR;
				if(rate == 0)
					rate = 44100; /* default */
				if (media_format_set_audio_samplerate(format, rate) !=
				    MEDIA_FORMAT_ERROR_NONE)
					goto ERROR;
				if (media_format_set_audio_avg_bps(format, 0) !=
				    MEDIA_FORMAT_ERROR_NONE)
					goto ERROR;
				if (media_format_set_audio_bit(format, 0) !=
				    MEDIA_FORMAT_ERROR_NONE)
					goto ERROR;
			}
			else {
				MD_I("No Support for MPEG%d Layer %d media\n",mpegversion,layer);
				goto ERROR;
			}
		}
	}else if (gst_structure_has_name(struc, "audio/x-amr-nb-sh") ||
		gst_structure_has_name(struc, "audio/x-amr-wb-sh")) {
		media_format_mimetype_e mime_type;
		gst_structure_get_int(struc, "channels", &channels);
		gst_structure_get_int(struc, "rate", &rate);
		if (gst_structure_has_name(struc, "audio/x-amr-nb-sh")) {
			mime_type= MEDIA_FORMAT_AMR_NB;
			rate = 8000;
		}
		else {
			mime_type = MEDIA_FORMAT_AMR_WB;
			rate = 16000;
		}
		if (media_format_set_audio_mime
		    (format,mime_type) != MEDIA_FORMAT_ERROR_NONE)
			goto ERROR;
		if(channels == 0)
			channels = 1; /* default */
		if (media_format_set_audio_channel(format, channels) !=
		    MEDIA_FORMAT_ERROR_NONE)
			goto ERROR;
		if (media_format_set_audio_samplerate(format, rate) !=
		    MEDIA_FORMAT_ERROR_NONE)
			goto ERROR;
		if (media_format_set_audio_avg_bps(format, 0) !=
		    MEDIA_FORMAT_ERROR_NONE)
			goto ERROR;
		if (media_format_set_audio_bit(format, 0) !=
		    MEDIA_FORMAT_ERROR_NONE)
			goto ERROR;
	}
	else {
		MD_I("Not Supported YET\n");
	}

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static int gst_demuxer_get_track_info(MMHandleType pHandle,
                                      media_format_h format, int index)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	track *temp = NULL;
	int loop;
	int count = 0;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	temp = (new_mediademuxer->info).head;
	loop = (new_mediademuxer->info).num_video_track +
	       (new_mediademuxer->info).num_audio_track +
	       (new_mediademuxer->info).num_subtitle_track +
	       (new_mediademuxer->info).num_other_track;
	if (index >= loop && index < 0)
		goto ERROR;

	while (count != index) {
		temp = temp->next;
		count++;
	}

	MD_I("CAPS for selected track [%d] is [%s]\n", index,
	     temp->caps_string);

	MD_I("format[%p]\n", format);
	if (temp->name[0] == 'a') {
		MD_I("Setting for Audio \n");
		_set_mime_audio(format, temp);
	} else if (temp->name[0] == 'v') {
		MD_I("Setting for Video \n");
		_set_mime_video(format, temp);
	} else {
		MD_I("Not supported so far\n");
	}
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

int _gst_set_appsink(track *temp, int index, int loop)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	int count = 0;
	if (index >= loop && index < 0)
		goto ERROR;
	while (count != index) {
		temp = temp->next;
		count++;
	}
	gst_app_sink_set_max_buffers((GstAppSink *)(temp->appsink),
	                             (guint) MAX_APP_BUFFER);
	gst_app_sink_set_drop((GstAppSink *)(temp->appsink), false);

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

int _gst_unset_appsink(track *temp, int index, int loop)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	int count = 0;
	if (index >= loop && index < 0)
		goto ERROR;
	while (count != index) {
		temp = temp->next;
		count++;
	}
	gst_app_sink_set_max_buffers((GstAppSink *)(temp->appsink), (guint) 0);
	gst_app_sink_set_drop((GstAppSink *)(temp->appsink), true);

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

#if 0
int __gst_remove_appsink(track *temp, int index, int loop,
                         GstElement *pipeline)
{
	int ret = MD_ERROR_NONE;
	int count = 0;
	GstPad *fake_pad = NULL;
	GstPad *app_pad = NULL;
	MEDIADEMUXER_FENTER();
	if (index >= loop && index < 0)
		goto ERROR;
	while (count != index) {
		temp = temp->next;
		count++;
	}
	app_pad = gst_element_get_static_pad(temp->appsink, "sink");
	gst_pad_unlink(temp->pad, app_pad);
	temp->fakesink = gst_element_factory_make("fakesink", NULL);
	MEDIADEMUXER_SET_STATE(temp->appsink, GST_STATE_PAUSED, ERROR);
	gst_bin_add_many(GST_BIN(pipeline), temp->fakesink, NULL);
	if (!temp->fakesink) {
		MD_E("factory not able to create fakesink\n");
		goto ERROR;
	} else {
		fake_pad = gst_element_get_static_pad(temp->fakesink, "sink");
		MEDIADEMUXER_LINK_PAD(temp->pad, fake_pad, ERROR);
	}
	MEDIADEMUXER_SET_STATE(temp->fakesink, GST_STATE_PLAYING, ERROR);
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static int _gst_demuxer_unset(MMHandleType pHandle, int track)
{
	int ret = MD_ERROR_NONE;
	int loop;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;
	track_info *head_track = &(new_mediademuxer->info);
	loop = (new_mediademuxer->info).num_video_track +
	       (new_mediademuxer->info).num_audio_track +
	       (new_mediademuxer->info).num_subtitle_track +
	       (new_mediademuxer->info).num_other_track;
	if (__gst_remove_appsink
	    ((head_track->head), track, loop, new_mediademuxer->pipeline) != 0)
		goto ERROR;
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}
#endif

static int gst_demuxer_set_track(MMHandleType pHandle, int track)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_INVALID_ARG;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	if (track >= new_mediademuxer->total_tracks || track < 0) {
		goto ERROR;
	}
	new_mediademuxer->selected_tracks[track] = true;
	_gst_set_appsink((((mdgst_handle_t *) pHandle)->info).head, track,
	                 new_mediademuxer->total_tracks);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_unset_track(MMHandleType pHandle, int track)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_INVALID_ARG;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	if (track >= new_mediademuxer->total_tracks || track < 0) {
		goto ERROR;
	}
	new_mediademuxer->selected_tracks[track] = false;
	_gst_unset_appsink((((mdgst_handle_t *) pHandle)->info).head, track,
	                   new_mediademuxer->total_tracks);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_get_data(MMHandleType pHandle, char *buffer)
{
	MEDIADEMUXER_FENTER();
	MD_E("%s: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return 0;
}

static int gst_demuxer_seek(MMHandleType pHandle, gint64 pos1)
{
	MEDIADEMUXER_FENTER();
	gint64 pos, len;
	gdouble rate = 1;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	if (gst_element_query_position
	    (gst_handle->pipeline, GST_FORMAT_TIME, &pos)
	    && gst_element_query_duration(gst_handle->pipeline, GST_FORMAT_TIME,
	                                  &len)) {
		MD_I("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
		     GST_TIME_ARGS(pos), GST_TIME_ARGS(len));
	}
	pos1 = pos + (pos1 * GST_SECOND);

	MD_I("\n\n");

	MD_I("NEW Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
	     GST_TIME_ARGS(pos1), GST_TIME_ARGS(len));

	track_info *head_track = &(gst_handle->info);
	track *temp = head_track->head;

	int indx = 0;
	while (temp) {
		MD_I("Got one element %p\n", temp->appsink);
		if (gst_handle->selected_tracks[indx] == true) {
			if (!gst_element_seek
			    (temp->appsink, rate, GST_FORMAT_TIME,
			     GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, pos1,
			     GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
				g_print("Seek failed!\n");
			} else {
				MD_I("SUCCESS\n");
			}
		}
		indx++;
		if (temp->next) {
			track *next = temp->next;
			temp = next;
		} else {
			temp = NULL;
		}
	}

	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static int gst_demuxer_start(MMHandleType pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	int indx;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	for (indx = 0; indx < gst_handle->total_tracks; indx++) {
		MD_I("track[%d] is marked as [%d]. (0- not selected, 1= selected)\n",
		     indx, gst_handle->selected_tracks[indx]);
		/*
		if (gst_handle->selected_tracks[indx] ==  false)
			_gst_demuxer_unset(pHandle, indx);
		*/
	}

	track_info *head_track = &(gst_handle->info);
	MD_I("Total Audio trk=%d, Total Video trk=%d, total text trk=%d\n",
	     head_track->num_audio_track, head_track->num_video_track,
	     head_track->num_other_track);

	track *temp = head_track->head;
	indx = 0;
	while (temp) {
		MD_I("Got one element %p\n", temp->appsink);
		/*
		if( gst_handle->selected_tracks[indx] ==  true) {
			MD_I("track %d is selected. Set it to PLAYING\n",indx);
		*/
		if (gst_element_set_state(temp->appsink, GST_STATE_PLAYING) ==
		    GST_STATE_CHANGE_FAILURE) {
			MD_E("Failed to set into PLAYING state");
			ret = MD_ERROR_UNKNOWN;
		}
		MD_I("set the state to playing\n");
		/*
		}
		*/
		indx++;
		if (temp->next) {
			track *next = temp->next;
			temp = next;
		} else {
			temp = NULL;
		}
	}

	MD_I("gst_demuxer_start pipeine %p", gst_handle);
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int _gst_copy_buf_to_media_packet(media_packet_h out_pkt,
                                         GstBuffer *buffer)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	void *pkt_data;
	uint64_t size;
	GstMapInfo map;
	MEDIADEMUXER_CHECK_NULL(out_pkt);

	if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
		MD_E("gst_buffer_map failed\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	/* copy data*/
	media_packet_get_buffer_size(out_pkt, &size);
	MD_I("Media packet Buffer capacity: %llu GST Buffer size = %d\n", size, map.size);
	if (size < (uint64_t)map.size) {
		MD_W("Media packet Buffer capacity[%llu] is \
			less than the GST Buffer size[%d]. Resizing...\n", size, map.size);
		ret = media_packet_set_buffer_size(out_pkt, (uint64_t)map.size);
		media_packet_get_buffer_size(out_pkt, &size);
		MD_I("Media packet Buffer NEW  capacity: %llu \n", size);
	}
	if (media_packet_get_buffer_data_ptr(out_pkt, &pkt_data)) {
		MD_E("unable to get the buffer pointer from media_packet_get_buffer_data_ptr\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	memcpy((char *)pkt_data, map.data, map.size);
	if (media_packet_set_pts(out_pkt, GST_BUFFER_PTS(buffer))) {
		MD_E("unable to set the pts\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	if (media_packet_set_dts(out_pkt, GST_BUFFER_DTS(buffer))) {
		MD_E("unable to set the dts\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	if (media_packet_set_duration(out_pkt, GST_BUFFER_DURATION(buffer))) {
		MD_E("unable to set the duration\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	if (media_packet_set_buffer_size(out_pkt,  gst_buffer_get_size(buffer))) {
		MD_E("unable to set the buffer size\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	if (media_packet_set_flags(out_pkt,  GST_BUFFER_FLAGS(buffer))) {
		MD_E("unable to set the buffer size\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
ERROR:
	gst_buffer_unmap(buffer, &map);
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_read_sample(MMHandleType pHandle, media_packet_h outbuf,
                                   int track_indx)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	int indx = 0;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *demuxer = (mdgst_handle_t *) pHandle;

	track *atrack = demuxer->info.head;
	if ((demuxer->selected_tracks)[track_indx] == false) {
		MD_E("Track Not selected\n");
		ret = MD_ERROR;
		goto ERROR;
	}
	while (atrack) {
		if (indx == track_indx)  /*Got the requird track details*/
			break;
		if (atrack->next) {
			track *next = atrack->next;
			atrack = next;
		} else {
			MD_E("Invalid track Index\n");
			ret = MD_ERROR_INVALID_ARGUMENT;
			goto ERROR;
		}
		indx++;
	}
	if (indx != track_indx) {
		MD_E("Invalid track Index\n");
		ret = MD_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}
	GstElement *sink = atrack->appsink;
	GstSample *sample = NULL;
	if (gst_app_sink_is_eos((GstAppSink *) sink)) {
		MD_W("End of stream reached\n");
		ret = MD_EOS;
		goto ERROR;
	}

	sample = gst_app_sink_pull_sample((GstAppSink *) sink);
	if (sample == NULL) {
		MD_E("gst_demuxer_read_sample failed\n");
		ret = MD_ERROR_UNKNOWN;
	}

	GstBuffer *buffer = gst_sample_get_buffer(sample);
	if (buffer == NULL) {
		MD_E("gst_sample_get_buffer returned NULL pointer\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}

	/*Fill the media_packet with proper information*/
	ret = _gst_copy_buf_to_media_packet(outbuf, buffer);
	gst_sample_unref(sample);

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_stop(MMHandleType pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;
	MD_I("gst_demuxer_stop pipeine %p", gst_handle);

	if (gst_element_set_state(gst_handle->pipeline, GST_STATE_PAUSED) ==
	    GST_STATE_CHANGE_FAILURE) {
		MD_E("Failed to set into PAUSE state");
		ret = MD_ERROR_UNKNOWN;
	}

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

void  _gst_clear_struct(mdgst_handle_t *gst_handle)
{
	MEDIADEMUXER_FENTER();
	if (gst_handle->selected_tracks) {
		MD_I("Deallocating gst_handle->selected_tracks %p\n",
		     gst_handle->selected_tracks);
		g_free(gst_handle->selected_tracks);
		gst_handle->selected_tracks = NULL;
	}
	if ((gst_handle->info).head) {
		__gst_free_stuct(&(gst_handle->info).head);
	}
	MEDIADEMUXER_FLEAVE();
}

int _md_gst_destroy_pipeline(GstElement *pipeline)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	if (pipeline)
		MEDIADEMUXER_SET_STATE(pipeline, GST_STATE_NULL, ERROR);
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	if (pipeline)
		gst_object_unref(GST_OBJECT(pipeline));
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static int gst_demuxer_unprepare(MMHandleType pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	_gst_clear_struct(gst_handle);
	MD_I("gst_demuxer_stop pipeine %p\n", gst_handle->pipeline);
	if (_md_gst_destroy_pipeline(gst_handle->pipeline) != MD_ERROR_NONE) {
		ret = MD_ERROR;
		goto ERROR;
	}
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

int gst_set_error_cb(MMHandleType pHandle,
			gst_error_cb callback, void* user_data)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	if (!gst_handle) {
		MD_E("fail invaild param\n");
		ret = MD_INVALID_ARG;
		goto ERROR;
	}

	if(gst_handle->user_cb[_GST_EVENT_TYPE_ERROR]) {
		MD_E("Already set mediademuxer_error_cb\n");
		ret = MD_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}
	else {
		if (!callback) {
			ret = MD_ERROR_INVALID_ARGUMENT;
			goto ERROR;
		}
	}

	MD_I("Set event handler callback(cb = %p, data = %p)\n", callback, user_data);

	gst_handle->user_cb[_GST_EVENT_TYPE_ERROR] = (gst_error_cb) callback;
	gst_handle->user_data[_GST_EVENT_TYPE_ERROR] = user_data;
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;

}

