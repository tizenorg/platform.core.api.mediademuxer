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
#include <media_packet_internal.h>

static int gst_demuxer_init(MMHandleType *pHandle);
static int gst_demuxer_prepare(MMHandleType pHandle, char *uri);
static int gst_demuxer_get_data_count(MMHandleType pHandle, int *count);
static int gst_demuxer_set_track(MMHandleType pHandle, int track);
static int gst_demuxer_start(MMHandleType pHandle);
static int gst_demuxer_read_sample(MMHandleType pHandle,
			media_packet_h *outbuf, int track_indx);
static int gst_demuxer_get_track_info(MMHandleType pHandle,
			media_format_h *format, int index);
static int gst_demuxer_seek(MMHandleType pHandle, gint64 pos1);
static int gst_demuxer_unset_track(MMHandleType pHandle, int track);
static int gst_demuxer_stop(MMHandleType pHandle);
static int gst_demuxer_unprepare(MMHandleType pHandle);
static int gst_demuxer_destroy(MMHandleType pHandle);
static int gst_set_error_cb(MMHandleType pHandle,
			gst_error_cb callback, void* user_data);
static int gst_set_eos_cb(MMHandleType pHandle,
			gst_eos_cb callback, void *user_data);
static int __gst_eos_callback(int track_num, void* user_data);

/* Media Demuxer API common */
static media_port_demuxer_ops def_demux_ops = {
	.n_size = 0,
	.init = gst_demuxer_init,
	.prepare = gst_demuxer_prepare,
	.get_track_count = gst_demuxer_get_data_count,
	.set_track = gst_demuxer_set_track,
	.start = gst_demuxer_start,
	.get_track_info = gst_demuxer_get_track_info,
	.read_sample = gst_demuxer_read_sample,
	.seek = gst_demuxer_seek,
	.unset_track = gst_demuxer_unset_track,
	.stop = gst_demuxer_stop,
	.unprepare = gst_demuxer_unprepare,
	.destroy = gst_demuxer_destroy,
	.set_error_cb = gst_set_error_cb,
	.set_eos_cb = gst_set_eos_cb,
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
}

static int gst_demuxer_init(MMHandleType *pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	mdgst_handle_t *new_mediademuxer = NULL;
	new_mediademuxer = (mdgst_handle_t *) g_malloc(sizeof(mdgst_handle_t));
	MD_I("gst_demuxer_init allocating new_mediademuxer:%p\n", new_mediademuxer);
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
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) data;
	int loop_track;
	track_info *head_track = &(gst_handle->info);
	track *head = head_track->head;
	gst_handle->selected_tracks =
	    (bool *) g_malloc(sizeof(bool) * (gst_handle->total_tracks));
	MD_I("Allocating %p to core->selected_tracks \n", gst_handle->selected_tracks);
	if (!gst_handle->selected_tracks) {
		MD_E("[%s]Memory allocation failed\n", __FUNCTION__);
		return;
	} else {
		for (loop_track = 0; loop_track < gst_handle->total_tracks;
		     loop_track++)
			gst_handle->selected_tracks[loop_track] = false;
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
	gst_handle->is_prepared = true;
	MD_I("core->is_prepared: ");
	gst_handle->is_prepared ? MD_I("true\n") : MD_I("false\n");
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
		if (temp->caps) {
			MD_I("deallocate GST_PAD caps_  %p\n", temp->caps);
			gst_caps_unref(temp->caps);
		}
		if (temp->name) {
			MD_I("deallocate GST_PAD name  %p\n", temp->name);
			g_free(temp->name);
		}
		if (temp->caps_string) {
			MD_I("deallocate GST_PAD caps_string  %p\n",
			     temp->caps_string);
			g_free(temp->caps_string);
		}
		if (temp->format) {
			MD_I("unref media_format  %p\n", temp->format);
			media_format_unref(temp->format);
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
	GstCaps *outcaps = NULL;
	GstPad *parse_sink_pad = NULL;
	GstElement *parse_element = NULL;
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
	temp->format = NULL;
	temp->appsink = gst_element_factory_make("appsink", NULL);
	if (!temp->appsink) {
		MD_E("factory not able to make appsink");
		__gst_free_stuct(head);
		return MD_ERROR;
	}
	gst_bin_add_many(GST_BIN(pipeline), temp->appsink, NULL);
	gst_app_sink_set_max_buffers((GstAppSink *) temp->appsink, (guint) 0);
	gst_app_sink_set_drop((GstAppSink *) temp->appsink, true);
	MEDIADEMUXER_SET_STATE(temp->appsink, GST_STATE_PAUSED, ERROR);
	apppad = gst_element_get_static_pad(temp->appsink, "sink");
	if (!apppad) {
		MD_E("sink pad of appsink not avaible");
		__gst_free_stuct(head);
		return MD_ERROR;
	}
	/* Check for type video and it should be h264 */
	if (strstr(name, "video") && strstr(temp->caps_string, "h264")) {
		parse_element = gst_element_factory_make("h264parse", NULL);
		if (!parse_element) {
			MD_E("factory not able to make h264parse");
			__gst_free_stuct(head);
			gst_object_unref(apppad);
			return MD_ERROR;
		}
		gst_bin_add_many(GST_BIN(pipeline), parse_element, NULL);
		MEDIADEMUXER_SET_STATE(parse_element, GST_STATE_PAUSED, ERROR);

		parse_sink_pad = gst_element_get_static_pad(parse_element, "sink");
		if (!parse_sink_pad) {
			MD_E("sink pad of h264parse not available");
			__gst_free_stuct(head);
			gst_object_unref(apppad);
			return MD_ERROR;
		}

		/* Link demuxer pad with sink pad of parse element */
		MEDIADEMUXER_LINK_PAD(pad, parse_sink_pad, ERROR);

		outcaps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "byte-stream", NULL);
		gst_element_link_filtered(parse_element, temp->appsink, outcaps);
		gst_caps_unref(outcaps);
	} else {
		MEDIADEMUXER_LINK_PAD(pad, apppad, ERROR);
	}
	/* gst_pad_link(pad, fpad) */
	if (*head == NULL) {
		*head = temp;
	} else {
		track *prev = *head;
		while (prev->next)
			prev = prev->next;
		prev->next = temp;
	}
	if (apppad)
		gst_object_unref(apppad);
	if (parse_sink_pad)
		gst_object_unref(parse_sink_pad);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	if (apppad)
		gst_object_unref(apppad);
	if (parse_sink_pad)
		gst_object_unref(parse_sink_pad);
	__gst_free_stuct(head);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static void __gst_on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
	MEDIADEMUXER_FENTER();
	MD_I("Dynamic pad created, linking demuxer/decoder\n");
	track *tmp = NULL;
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) data;
	track_info *head_track = &(gst_handle->info);
	gchar *name = gst_pad_get_name(pad);
	gst_handle->total_tracks++;
	if (__gst_add_track_info(pad, name, &(head_track->head), gst_handle->pipeline)
	    != MD_ERROR_NONE) {
		MD_E("unable to added track info");
		head_track->num_audio_track = 0;
		head_track->num_video_track = 0;
		head_track->num_subtitle_track = 0;
		head_track->num_other_track = 0;
		__gst_free_stuct(&(head_track->head));
		return;
	}
	tmp = head_track->head;
	while (tmp->next)
		tmp = tmp->next;
	if (tmp->caps_string[0] == 'v') {
		MD_I("found Video Pad\n");
		(head_track->num_video_track)++;
	} else if (tmp->caps_string[0] == 'a') {
		MD_I("found Audio Pad\n");
		(head_track->num_audio_track)++;
	} else if (tmp->caps_string[0] == 's') {
		MD_I("found subtitle(or Text) Pad\n");
		(head_track->num_subtitle_track)++;
	} else {
		MD_W("found Pad %s\n", name);
		(head_track->num_other_track)++;
	}
	MEDIADEMUXER_FLEAVE();
}

static int __gst_create_audio_only_pipeline(gpointer data,  GstCaps *caps)
{
	MEDIADEMUXER_FENTER();
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) data;
	GstPad *pad = NULL;
	GstPad *aud_pad = NULL;
	GstPad *queue_srcpad = NULL;
	GstPad *queue_sinkpad = NULL;
	GstPad *aud_srcpad = NULL;
	GstPad *fake_pad = NULL;
	GstElement *id3tag = NULL;
	GstElement *adif_queue = NULL;
	gchar *name;
	gchar *type;
	track_info *head_track = &(gst_handle->info);
	track *trck;
	gst_handle->is_valid_container = true;
	type = gst_caps_to_string(caps);
	if (strstr(type, "adts") || strstr(type, "adif")) {
		gst_handle->demux = gst_element_factory_make("aacparse", NULL);
	} else if (strstr(type, "audio/mpeg")) {
		gst_handle->demux = gst_element_factory_make("mpegaudioparse", NULL);
	} else if (strstr(type, "application/x-id3")) {
		id3tag = gst_element_factory_make("id3demux", NULL);
		gst_handle->demux = gst_element_factory_make("mpegaudioparse", NULL);
	} else if (strstr(type, "audio/x-amr-nb-sh")
		   || strstr(type, "audio/x-amr-wb-sh")) {
		gst_handle->demux = gst_element_factory_make("amrparse", NULL);
	} else if (strstr(type, "audio/x-wav")) {
		gst_handle->demux = gst_element_factory_make("wavparse", NULL);
	} else if (strstr(type, "audio/x-flac")) {
		gst_handle->demux = gst_element_factory_make("flacparse", NULL);
	}

	if (!gst_handle->demux) {
		gst_handle->is_valid_container = false;
		MD_E("factory not able to create audio parse element\n");
		goto ERROR;
	} else {
		gst_bin_add_many(GST_BIN(gst_handle->pipeline),
				 gst_handle->demux, id3tag, NULL);
		pad = gst_element_get_static_pad(gst_handle->typefind, "src");
		if (!pad) {
			MD_E("fail to get typefind src pad.\n");
			goto ERROR;
		}
		if (!id3tag)
			aud_pad = gst_element_get_static_pad(gst_handle->demux, "sink");
		else
			aud_pad = gst_element_get_static_pad(id3tag, "sink");
		if (!aud_pad) {
			MD_E("fail to get audio parse sink pad.\n");
			goto ERROR;
		}
		fake_pad = gst_element_get_static_pad(gst_handle->fakesink, "sink");
		if (!fake_pad) {
			MD_E("fail to get fakesink sink pad.\n");
			goto ERROR;
		}
		gst_pad_unlink(pad, fake_pad);
		if (!id3tag) {
			MEDIADEMUXER_SET_STATE(gst_handle->demux,
					       GST_STATE_PAUSED, ERROR);
		} else {
			MEDIADEMUXER_SET_STATE(id3tag, GST_STATE_PAUSED, ERROR);
			MEDIADEMUXER_SET_STATE(gst_handle->demux,
					       GST_STATE_PAUSED, ERROR);
			gst_element_link(id3tag, gst_handle->demux);
		}
	}

	/* calling "on_pad_added" function to set the caps */
	aud_srcpad = gst_element_get_static_pad(gst_handle->demux, "src");
	if (!aud_srcpad) {
		MD_E("fail to get audioparse source pad.\n");
		goto ERROR;
	}
	gst_handle->total_tracks++;
	name = gst_pad_get_name(aud_srcpad);
	if (__gst_add_track_info(aud_srcpad, name, &(head_track->head), gst_handle->pipeline)
	    != MD_ERROR_NONE) {
		MD_E("unable to added track info");
		head_track->num_audio_track = 0;
		head_track->num_video_track = 0;
		head_track->num_subtitle_track = 0;
		head_track->num_other_track = 0;
		__gst_free_stuct(&(head_track->head));
		goto ERROR;
	}
	if (strstr(type, "adif")) {
		adif_queue = gst_element_factory_make("queue", NULL);
		if (!adif_queue) {
			MD_E("factory not able to make queue in case of adif aac\n");
			goto ERROR;
		}
		/* Add this queue to the pipeline */
		gst_bin_add_many(GST_BIN(gst_handle->pipeline), adif_queue, NULL);
		queue_srcpad = gst_element_get_static_pad(adif_queue, "src");
		if (!queue_srcpad) {
			MD_E("fail to get queue src pad for adif aac.\n");
			goto ERROR;
		}
		queue_sinkpad = gst_element_get_static_pad(adif_queue, "sink");
		if (!queue_sinkpad) {
			MD_E("fail to get queue sink pad for adif aac.\n");
			goto ERROR;
		}
		/* link typefind with queue */
		MEDIADEMUXER_LINK_PAD(pad, queue_sinkpad, ERROR);
		/* link queue with aacparse */
		MEDIADEMUXER_LINK_PAD(queue_srcpad, aud_pad, ERROR);
	} else {
		MEDIADEMUXER_LINK_PAD(pad, aud_pad, ERROR);
	}
	if (adif_queue)
		MEDIADEMUXER_SET_STATE(adif_queue, GST_STATE_PAUSED, ERROR);

	trck = head_track->head;
	while (trck != NULL && aud_srcpad != trck->pad)
		trck = trck->next;

	if (trck != NULL) {
		if (trck->caps)
			gst_caps_unref(trck->caps);
		trck->caps = caps;
		if (trck->caps_string)
			g_free(trck->caps_string);
		trck->caps_string = gst_caps_to_string(trck->caps);
		MD_I("caps set to %s\n", trck->caps_string);
		if (trck->name)
			g_free(trck->name);
		g_strlcpy(name, "audio", strlen(name));
		trck->name = name;
	}
	(head_track->num_audio_track)++;

	/* unref pads */
	if (pad)
		gst_object_unref(pad);
	if (aud_pad)
		gst_object_unref(aud_pad);
	if (fake_pad)
		gst_object_unref(fake_pad);
	if (queue_sinkpad)
		gst_object_unref(queue_sinkpad);
	if (queue_srcpad)
		gst_object_unref(queue_srcpad);
	if (aud_srcpad)
		gst_object_unref(aud_srcpad);

	__gst_no_more_pad(gst_handle->demux, data);
	g_free(type);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	gst_handle->is_valid_container = false;
	if (gst_handle->demux)
		gst_object_unref(gst_handle->demux);
	if (pad)
		gst_object_unref(pad);
	if (aud_pad)
		gst_object_unref(aud_pad);
	if (fake_pad)
		gst_object_unref(fake_pad);
	if (aud_srcpad)
		gst_object_unref(aud_srcpad);
	if (queue_sinkpad)
		gst_object_unref(queue_sinkpad);
	if (queue_srcpad)
		gst_object_unref(queue_srcpad);
	g_free(type);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static void __gst_cb_typefind(GstElement *tf, guint probability,
					GstCaps *caps, gpointer data)
{
	MEDIADEMUXER_FENTER();
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) data;
	GstPad *pad = NULL;
	GstPad *demuxer_pad = NULL;
	GstPad *fake_pad = NULL;
	gchar *type;
	type = gst_caps_to_string(caps);
	if (type) {
		MD_I("Media type %s found, probability %d%%\n", type, probability);
		if (strstr(type, "quicktime") || (strstr(type, "audio/x-m4a")) || strstr(type, "x-3gp")
				|| strstr(type, "ogg") || strstr(type, "flv")) {
			gst_handle->is_valid_container = true;
			if (strstr(type, "ogg"))
				gst_handle->demux = gst_element_factory_make("oggdemux", NULL);
			else if (strstr(type, "flv"))
				gst_handle->demux = gst_element_factory_make("flvdemux", NULL);
			else
				gst_handle->demux = gst_element_factory_make("qtdemux", NULL);

			if (!gst_handle->demux) {
				gst_handle->is_valid_container = false;
				MD_E("factory not able to create qtdemux\n");
				goto ERROR;
			} else {
				g_signal_connect(gst_handle->demux, "pad-added",
								G_CALLBACK(__gst_on_pad_added), gst_handle);
				g_signal_connect(gst_handle->demux, "no-more-pads",
								G_CALLBACK(__gst_no_more_pad), gst_handle);
				gst_bin_add_many(GST_BIN(gst_handle->pipeline),
								gst_handle->demux, NULL);
				pad = gst_element_get_static_pad(gst_handle->typefind, "src");
				if (!pad) {
					MD_E("fail to get typefind src pad.\n");
					goto ERROR;
				}
				demuxer_pad = gst_element_get_static_pad(gst_handle->demux, "sink");
				if (!demuxer_pad) {
					MD_E("fail to get qtdemuc sink pad.\n");
					goto ERROR;
				}
				fake_pad = gst_element_get_static_pad(gst_handle->fakesink, "sink");
				if (!fake_pad) {
					MD_E("fail to get fakesink sink pad.\n");
					goto ERROR;
				}
				gst_pad_unlink(pad, fake_pad);
				MEDIADEMUXER_LINK_PAD(pad, demuxer_pad, ERROR);
				MEDIADEMUXER_SET_STATE(gst_handle->demux,
									GST_STATE_PAUSED, ERROR);
				if (pad)
					gst_object_unref(pad);
				if (demuxer_pad)
					gst_object_unref(demuxer_pad);
				if (fake_pad)
					gst_object_unref(fake_pad);
			}
		} else if ((strstr(type, "adts"))
			   || (strstr(type, "audio/mpeg"))
			   || (strstr(type, "audio/x-wav"))
			   || (strstr(type, "audio/x-flac"))
			   || (strstr(type, "application/x-id3"))
			   || (strstr(type, "audio/x-amr-nb-sh"))
			   || (strstr(type, "audio/x-amr-wb-sh"))) {
			MD_I("Audio only format is found\n");
			__gst_create_audio_only_pipeline(data, caps);
		} else {
			gst_handle->is_valid_container = false;
			MD_E("Not supported container %s\n", type);
		}
		g_free(type);
	}
	MEDIADEMUXER_FLEAVE();
	return;
ERROR:
	gst_handle->is_valid_container = false;
	if (gst_handle->demux)
		gst_object_unref(gst_handle->demux);
	if (type)
		g_free(type);
	if (pad)
		gst_object_unref(pad);
	if (demuxer_pad)
		gst_object_unref(demuxer_pad);
	if (fake_pad)
		gst_object_unref(fake_pad);
	MEDIADEMUXER_FLEAVE();
	return;
}

static int _gst_create_pipeline(mdgst_handle_t *gst_handle, char *uri)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	GstBus *bus = NULL;
	char *path = NULL;
	int remote_streaming = 0;
	/* Initialize GStreamer */
	/* Note: Replace the arguments of gst_init to pass the command line args to GStreamer. */
	gst_init(NULL, NULL);

	/* Create the empty pipeline */
	gst_handle->pipeline = gst_pipeline_new("Demuxer Gst pipeline");
	if (!gst_handle->pipeline) {
		MD_E("pipeline create fail");
		ret = MD_ERROR;
		goto ERROR;
	}

	/* Create the elements */
	if ((path = strstr(uri, "http://"))) {
		gst_handle->filesrc  = gst_element_factory_make("souphttpsrc", NULL);
		remote_streaming = 1;
		MD_I("Source is http stream. \n");
	} else {
		gst_handle->filesrc = gst_element_factory_make("filesrc", NULL);
		MD_I("Source is file stream \n");
	}
	if (!gst_handle->filesrc) {
		MD_E("filesrc creation failed");
		ret = MD_ERROR;
		goto ERROR;
	}

	/* Modify the source's properties */
	if (remote_streaming == 1)
		g_object_set(G_OBJECT(gst_handle->filesrc), "location", uri, NULL);
	else
		g_object_set(G_OBJECT(gst_handle->filesrc), "location", uri + 7, NULL);
	gst_handle->typefind = gst_element_factory_make("typefind", NULL);
	if (!gst_handle->typefind) {
		MD_E("typefind creation failed");
		ret = MD_ERROR;
		goto ERROR;
	}
	g_signal_connect(gst_handle->typefind, "have-type",
					G_CALLBACK(__gst_cb_typefind), gst_handle);
	gst_handle->fakesink = gst_element_factory_make("fakesink", NULL);
	if (!gst_handle->fakesink) {
		MD_E("fakesink creation failed");
		ret = MD_ERROR;
		goto ERROR;
	}

	/* Build the pipeline */
	gst_bin_add_many(GST_BIN(gst_handle->pipeline),
						gst_handle->filesrc,
						gst_handle->typefind,
						gst_handle->fakesink,
						NULL);
	gst_element_link_many(gst_handle->filesrc,
							gst_handle->typefind,
							gst_handle->fakesink,
							NULL);

	/* connect signals, bus watcher */
	bus = gst_pipeline_get_bus(GST_PIPELINE(gst_handle->pipeline));
	gst_handle->bus_watch_id = gst_bus_add_watch(bus, __gst_bus_call, gst_handle);
	gst_handle->thread_default = g_main_context_get_thread_default();
	gst_object_unref(GST_OBJECT(bus));

	/* set pipeline state to PAUSED */
	MEDIADEMUXER_SET_STATE(gst_handle->pipeline, GST_STATE_PAUSED, ERROR);

	int count = 0;
	while (gst_handle->is_prepared != true) {
		count++;
		usleep(POLLING_INTERVAL);
		MD_I("Inside while loop\n");
		if (count > POLLING_INTERVAL) {
			MD_E("Error occure\n");
			ret = MD_ERROR;
			break;
		}
	}

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
	if (ret != MD_ERROR_NONE) {
		MD_E("_gst_create_pipeline() failed. returned %d\n", ret);
		goto ERROR;
	}
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
}

int _gst_set_appsink(track *temp, int index, int loop)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	int count = 0;

	while (count != index) {
		temp = temp->next;
		count++;
	}
	gst_app_sink_set_max_buffers((GstAppSink *)(temp->appsink), (guint) MAX_APP_BUFFER);
	gst_app_sink_set_drop((GstAppSink *)(temp->appsink), false);
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_set_track(MMHandleType pHandle, int track)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	MD_I("total_tracks (%d) :: selected  track (%d)", new_mediademuxer->total_tracks, track);
	if (track >= new_mediademuxer->total_tracks || track < 0) {
		MD_E("total_tracks is less then selected track, So not support this track");
		ret = MD_ERROR_INVALID_ARGUMENT;
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

static int gst_demuxer_start(MMHandleType pHandle)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	int indx;
	for (indx = 0; indx < gst_handle->total_tracks; indx++) {
		MD_I("track indx[%d] is marked as [%d]. (0- not selected, 1= selected)\n",
			indx, gst_handle->selected_tracks[indx]);
		/*
		if (gst_handle->selected_tracks[indx] ==  false)
			_gst_demuxer_unset(pHandle, indx);
		*/
	}

	track_info *head_track = &(gst_handle->info);
	MD_I("Total Audio track=%d, Video track=%d, text track=%d\n",
			head_track->num_audio_track, head_track->num_video_track,
			head_track->num_subtitle_track);

	track *temp = head_track->head;
	indx = 0;
	while (temp) {
		MD_I("Got one element %p\n", temp->appsink);
		if (gst_element_set_state(temp->appsink, GST_STATE_PLAYING) ==
			GST_STATE_CHANGE_FAILURE) {
			MD_E("Failed to set into PLAYING state");
			ret = MD_INTERNAL_ERROR;
			goto ERROR;
		}
		MD_I("set the state to playing\n");
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

int _set_mime_video(media_format_h format, track *head)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	GstStructure *struc = NULL;
	int src_width;
	int src_height;
	int frame_rate_numerator = 0;
	int frame_rate_denominator = 0;
	media_format_mimetype_e mime_type = MEDIA_FORMAT_MAX;
	struc = gst_caps_get_structure(head->caps, 0);
	if (!struc) {
		MD_E("cannot get structure from caps.\n");
		goto ERROR;
	}
	if (gst_structure_has_name(struc, "video/x-h264")) {
		const gchar *version = gst_structure_get_string(struc, "stream-format");
		if (strncmp(version, "avc", 3) == 0)
			mime_type = MEDIA_FORMAT_H264_SP;
		else {
			MD_W("Video mime (%s) not supported so far\n", gst_structure_get_name(struc));
			goto ERROR;
		}
	} else if (gst_structure_has_name(struc, "video/x-h263")) {
		mime_type = MEDIA_FORMAT_H263;
	} else {
		MD_W("Video mime (%s) not supported so far\n", gst_structure_get_name(struc));
		goto ERROR;
	}
	if (media_format_set_video_mime(format, mime_type)) {
		MD_E("Unable to set video mime type (%x)\n", mime_type);
		goto ERROR;
	}
	gst_structure_get_int(struc, "width", &src_width);
	gst_structure_get_int(struc, "height", &src_height);
	if (media_format_set_video_width(format, src_width))
		goto ERROR;
	if (media_format_set_video_height(format, src_height))
			goto ERROR;
	gst_structure_get_fraction(struc, "framerate",  &frame_rate_numerator, &frame_rate_denominator);
	if (media_format_set_video_frame_rate(format, frame_rate_numerator)) {
		MD_E("Unable to set video frame rate\n");
		goto ERROR;
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
	int bit = 0;
	int channels = 0;
	int id3_flag = 0;
	const gchar *stream_format;
	media_format_mimetype_e mime_type = MEDIA_FORMAT_MAX;

	struc = gst_caps_get_structure(head->caps, 0);
	if (!struc) {
		MD_E("cannot get structure from caps.\n");
		goto ERROR;
	}

	if (gst_structure_has_name(struc, "application/x-id3"))
		id3_flag = 1;
	if (gst_structure_has_name(struc, "audio/mpeg") || id3_flag) {
		gint mpegversion;
		int layer;
		gst_structure_get_int(struc, "mpegversion", &mpegversion);
		if (mpegversion == 4 || mpegversion == 2) {
			mime_type = MEDIA_FORMAT_AAC_LC;
			stream_format = gst_structure_get_string(struc, "stream-format");
			if (strncmp(stream_format, "adts", 4) == 0)
				media_format_set_audio_aac_type(format, 1);
			else
				media_format_set_audio_aac_type(format, 0);
		} else if (mpegversion == 1 || id3_flag) {
			gst_structure_get_int(struc, "layer", &layer);
			if ((layer == 3) || (id3_flag == 1)) {
				mime_type = MEDIA_FORMAT_MP3;
			} else {
				MD_I("No Support for MPEG%d Layer %d media\n", mpegversion, layer);
				goto ERROR;
			}
		}
	} else if (gst_structure_has_name(struc, "audio/x-amr-nb-sh") ||
		gst_structure_has_name(struc, "audio/x-amr-wb-sh")) {
		if (gst_structure_has_name(struc, "audio/x-amr-nb-sh")) {
			mime_type = MEDIA_FORMAT_AMR_NB;
			rate = 8000;
		} else {
			mime_type = MEDIA_FORMAT_AMR_WB;
			rate = 16000;
		}
	} else if (gst_structure_has_name(struc, "audio/AMR")) {
		mime_type = MEDIA_FORMAT_AMR_NB;
	} else if (gst_structure_has_name(struc, "audio/AMR-WB")) {
		mime_type = MEDIA_FORMAT_AMR_WB;
	} else if (gst_structure_has_name(struc, "audio/x-wav")) {
		mime_type = MEDIA_FORMAT_PCM;
	} else if (gst_structure_has_name(struc, "audio/x-flac")) {
		mime_type = MEDIA_FORMAT_FLAC;
	} else if (gst_structure_has_name(struc, "audio/x-vorbis")) {
		mime_type = MEDIA_FORMAT_VORBIS;
	} else {
		MD_W("Audio mime (%s) not supported so far\n", gst_structure_get_name(struc));
		goto ERROR;
	}
	if (media_format_set_audio_mime(format, mime_type))
		goto ERROR;
	gst_structure_get_int(struc, "channels", &channels);
	if (channels == 0) {	/* default */
		if (mime_type == MEDIA_FORMAT_AMR_NB || mime_type == MEDIA_FORMAT_AMR_WB)
			channels = 1;
		else
			channels = 2;
	}
	if (media_format_set_audio_channel(format, channels))
		goto ERROR;
	if (rate == 0)
		gst_structure_get_int(struc, "rate", &rate);
	if (rate == 0)
		rate = 44100;	/* default */
	if (media_format_set_audio_samplerate(format, rate))
		goto ERROR;
	gst_structure_get_int(struc, "bit", &bit);
	if (bit == 0)
		bit = 16;	/* default */
	if (media_format_set_audio_bit(format, bit))
		goto ERROR;
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR;
}

static int gst_demuxer_get_track_info(MMHandleType pHandle,
							media_format_h *format, int index)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	track *temp = NULL;
	int loop;
	int count = 0;
	temp = (new_mediademuxer->info).head;
	loop = (new_mediademuxer->info).num_video_track +
			(new_mediademuxer->info).num_audio_track +
			(new_mediademuxer->info).num_subtitle_track +
			(new_mediademuxer->info).num_other_track;
	if (index >= loop || index < 0) {
		MD_E("total tracks(loop) is less then selected track(index), So not support this track");
		ret = MD_ERROR;
		goto ERROR;
	}

	while (count != index) {
		temp = temp->next;
		count++;
	}
	if (temp->format != NULL) {
		ret = media_format_ref(temp->format);
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			MD_E("Mediaformat reference count increment failed. returned %d\n", ret);
			ret = MD_INTERNAL_ERROR;
			goto ERROR;
		}
		ret = media_format_make_writable(temp->format, format);	/* copy the content */
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			MD_E("Mediaformat create copy failed. returned %d\n", ret);
			media_format_unref(temp->format);
			ret = MD_INTERNAL_ERROR;
			goto ERROR;
		}
		MEDIADEMUXER_FLEAVE();
		return ret;
	}
	ret = media_format_create(&(temp->format));
	if (ret != MEDIA_FORMAT_ERROR_NONE) {
		MD_E("Mediaformat creation failed. returned %d\n", ret);
		ret = MD_INTERNAL_ERROR;
		goto ERROR;
	}

	MD_I("CAPS for selected track [%d] is [%s]\n", index, temp->caps_string);
	MD_I("format ptr[%p]\n", temp->format);
	if (temp->caps_string[0] == 'a') {
		MD_I("Setting for Audio \n");
		_set_mime_audio(temp->format, temp);
	} else if (temp->caps_string[0] == 'v') {
		MD_I("Setting for Video \n");
		_set_mime_video(temp->format, temp);
	} else
		MD_W("Not supported so far (except audio and video)\n");

	ret = media_format_ref(temp->format);	/* increment the ref to retain the original content */
	if (ret != MEDIA_FORMAT_ERROR_NONE) {
		MD_E("Mediaformat reference count increment failed. returned %d\n", ret);
		ret = MD_INTERNAL_ERROR;
		goto ERROR;
	}
	ret = media_format_make_writable(temp->format, format);	/* copy the content */
	if (ret != MEDIA_FORMAT_ERROR_NONE) {
		MD_E("Mediaformat create copy failed. returned %d\n", ret);
		media_format_unref(temp->format);
		ret = MD_INTERNAL_ERROR;
		goto ERROR;
	}
	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int _gst_copy_buf_to_media_packet(media_packet_h out_pkt,
							GstBuffer *buffer, char *codec_data)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(out_pkt);
	void *pkt_data;
	uint64_t size;
	GstMapInfo map;

	if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
		MD_E("gst_buffer_map failed\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	/* copy data */
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
	if (media_packet_set_buffer_size(out_pkt, gst_buffer_get_size(buffer))) {
		MD_E("unable to set the buffer size\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	if (media_packet_set_flags(out_pkt, GST_BUFFER_FLAGS(buffer))) {
		MD_E("unable to set the buffer size\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}
	if (codec_data) {
		if (media_packet_set_codec_data(out_pkt, (void*) codec_data, strlen(codec_data))) {
			MD_E("unable to set the codec data\n");
			ret = MD_ERROR_UNKNOWN;
			goto ERROR;
		}
	}
ERROR:
	gst_buffer_unmap(buffer, &map);
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_read_sample(MMHandleType pHandle,
						media_packet_h *outbuf, int track_indx)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *demuxer = (mdgst_handle_t *) pHandle;

	media_packet_h mediabuf = NULL;
	int indx = 0;
	char *codec_data = NULL;
	char *temp_codec_data = NULL;
	int index = 0;

	track *atrack = demuxer->info.head;
	if ((demuxer->selected_tracks)[track_indx] == false) {
		MD_E("Track Not selected\n");
		ret = MD_ERROR;
		goto ERROR;
	}
	while (atrack) {
		if (indx == track_indx)  /* Got the requird track details */
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

	if (!atrack) {
		MD_E("atrack is NULL\n");
		goto ERROR;
	}

	if (media_packet_create_alloc(atrack->format, NULL, NULL, &mediabuf)) {
		MD_E("media_packet_create_alloc failed\n");
		ret = MD_ERROR;
		goto ERROR;
	}

	if (indx != track_indx) {
		MD_E("Invalid track Index\n");
		ret = MD_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}
	GstElement *sink = atrack->appsink;
	GstSample *sample = NULL;

	sample = gst_app_sink_pull_sample((GstAppSink *) sink);
	if (sample == NULL) {
		if (gst_app_sink_is_eos((GstAppSink *) sink)) {
			MD_W("End of stream (EOS) reached, triggering the eos callback\n");
			ret = MD_ERROR_NONE;
			__gst_eos_callback(track_indx, demuxer);
			return ret;
		} else {
			MD_E("gst_demuxer_read_sample failed\n");
			ret = MD_ERROR_UNKNOWN;
		}
	}

	GstBuffer *buffer = gst_sample_get_buffer(sample);
	if (buffer == NULL) {
		MD_E("gst_sample_get_buffer returned NULL pointer\n");
		ret = MD_ERROR_UNKNOWN;
		goto ERROR;
	}

	/* Create the codec data and pass to _gst_copy_buf_to_media_packet() to add into the media packet */
	temp_codec_data = strstr(atrack->caps_string, "codec_data");
	if (temp_codec_data != NULL) {
		while (*temp_codec_data != ')')
			temp_codec_data++;
		temp_codec_data++; /* to esacpe ')' */
		codec_data = (char*) malloc(sizeof(char)*strlen(temp_codec_data));
		if (codec_data != NULL) {
			while (*temp_codec_data != ',') {
				codec_data[index++] = *temp_codec_data;
				temp_codec_data++;
			}
			codec_data[index] = '\0';
		}
	}

	/* Fill the media_packet with proper information */
	ret = _gst_copy_buf_to_media_packet(mediabuf, buffer, codec_data);
	gst_sample_unref(sample);
	*outbuf = mediabuf;
	if (codec_data)
		free(codec_data);

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_seek(MMHandleType pHandle, gint64 pos1)
{
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	gint64 pos = 0, len = 0;
	gdouble rate = 1;
	track_info *head_track = &(gst_handle->info);
	track *temp = head_track->head;
	track *temp_track = head_track->head;
	int indx = 0;

	/* Setting each appsink to paused state before seek */
	while (temp_track) {
		if (gst_handle->selected_tracks[indx] == true) {
			if (gst_element_set_state(temp_track->appsink, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
				MD_E("Failed to set into PAUSED state");
				goto ERROR;
			}
			MD_I("set the state to paused\n");
		}
		indx++;
		if (temp_track->next) {
			track *next = temp_track->next;
			temp_track = next;
		} else {
			temp_track = NULL;
		}
	}

	if (gst_element_query_position(gst_handle->pipeline, GST_FORMAT_TIME, &pos) &&
	     gst_element_query_duration(gst_handle->pipeline, GST_FORMAT_TIME, &len)) {
		MD_I("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
		     GST_TIME_ARGS(pos),
		     GST_TIME_ARGS(len));
	}
	pos1 = pos + (pos1 * GST_SECOND);

	MD_I("\n\n");
	MD_I("NEW Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
	     GST_TIME_ARGS(pos1), GST_TIME_ARGS(len));

	indx = 0;
	while (temp) {
		MD_I("Got one element %p\n", temp->appsink);
		if (gst_handle->selected_tracks[indx] == true) {
			if (!gst_element_seek(temp->appsink, rate, GST_FORMAT_TIME,
			     GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, pos1,
			     GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
				MD_E("Seek failed!\n");
				goto ERROR;
			} else {
				MD_I("Seek success...setting appsink to playing state\n");
				if (gst_element_set_state(temp->appsink, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
					MD_E("Failed to set into PLAYING state");
					goto ERROR;
				}
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

int _gst_unset_appsink(track *temp, int index, int loop)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	int count = 0;

	while (count != index) {
		temp = temp->next;
		count++;
	}
	gst_app_sink_set_max_buffers((GstAppSink *)(temp->appsink), (guint) 0);
	gst_app_sink_set_drop((GstAppSink *)(temp->appsink), true);
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int gst_demuxer_unset_track(MMHandleType pHandle, int track)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *new_mediademuxer = (mdgst_handle_t *) pHandle;

	if (track >= new_mediademuxer->total_tracks || track < 0) {
		MD_E("total tracks is less then unselected track, So not support this track");
		ret = MD_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}
	new_mediademuxer->selected_tracks[track] = false;
	_gst_unset_appsink((((mdgst_handle_t *) pHandle)->info).head, track,
					new_mediademuxer->total_tracks);
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
		ret = MD_INTERNAL_ERROR;
		goto ERROR;
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
	if ((gst_handle->info).head)
		__gst_free_stuct(&(gst_handle->info).head);
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
	if (gst_handle->bus_watch_id) {
		GSource *source = NULL;
		source = g_main_context_find_source_by_id(gst_handle->thread_default, gst_handle->bus_watch_id);
		if (source) {
			g_source_destroy(source);
			LOGD("Deallocation bus watch id");
		}
	}

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
}

int gst_set_error_cb(MMHandleType pHandle,
			gst_error_cb callback, void *user_data)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	if (!gst_handle) {
		MD_E("fail invaild param (gst_handle)\n");
		ret = MD_INVALID_ARG;
		goto ERROR;
	}

	if (gst_handle->user_cb[_GST_EVENT_TYPE_ERROR]) {
		MD_E("Already set mediademuxer_error_cb\n");
		ret = MD_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	} else {
		if (!callback) {
			MD_E("fail invaild argument (callback)\n");
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

int gst_set_eos_cb(MMHandleType pHandle, gst_eos_cb callback, void *user_data)
{
	MEDIADEMUXER_FENTER();
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_CHECK_NULL(pHandle);
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) pHandle;

	if (!gst_handle) {
		MD_E("fail invaild param (gst_handle)\n");
		ret = MD_INVALID_ARG;
		goto ERROR;
	}

	if (gst_handle->user_cb[_GST_EVENT_TYPE_EOS]) {
		MD_E("Already set mediademuxer_eos_cb\n");
		ret = MD_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	} else {
		if (!callback) {
			MD_E("fail invaild argument (callback)\n");
			ret = MD_ERROR_INVALID_ARGUMENT;
			goto ERROR;
		}
	}

	MD_I("Set event handler callback(cb = %p, data = %p)\n", callback, user_data);
	gst_handle->user_cb[_GST_EVENT_TYPE_EOS] = (gst_eos_cb) callback;
	gst_handle->user_data[_GST_EVENT_TYPE_EOS] = user_data;
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int __gst_eos_callback(int track_num, void* user_data)
{
	if (user_data == NULL) {
		MD_E("Invalid argument");
		return MD_ERROR;
	}
	mdgst_handle_t *gst_handle = (mdgst_handle_t *) user_data;
	if (gst_handle->user_cb[_GST_EVENT_TYPE_EOS])
		((gst_eos_cb)gst_handle->user_cb[_GST_EVENT_TYPE_EOS])(track_num,
					gst_handle->user_data[_GST_EVENT_TYPE_EOS]);
	else
		MD_E("EOS received, but callback is not set!!!");
	return MD_ERROR_NONE;
}
