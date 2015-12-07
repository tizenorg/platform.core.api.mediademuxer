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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlog.h>
#include <mm.h>
#include <mm_types.h>

#include <mediademuxer.h>
#include <mediademuxer_private.h>
#include <mediademuxer_port.h>

#ifndef USE_TASK_QUEUE
#define USE_TASK_QUEUE
#endif

/*
* Public Implementation
*/
static gboolean _mediademuxer_error_cb(mediademuxer_error_e error, void *user_data);
static gboolean _mediademuxer_eos_cb(int track_num, void *user_data);

int mediademuxer_create(mediademuxer_h *demuxer)
{
	MD_I("mediademuxer_create\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);

	mediademuxer_s *handle;
	if (*demuxer == NULL) {
		handle = (mediademuxer_s *) g_malloc(sizeof(mediademuxer_s));
		if (handle != NULL) {
			memset(handle, 0, sizeof(mediademuxer_s));
			handle->demux_state = MEDIADEMUXER_NONE;
		} else {
			MD_E("[CoreAPI][%s] DEMUXER_ERROR_OUT_OF_MEMORY(0x%08x)",
				__FUNCTION__, MEDIADEMUXER_ERROR_OUT_OF_MEMORY);
			return MEDIADEMUXER_ERROR_OUT_OF_MEMORY;
		}
	} else {
		MD_E("Already created the instance\n");
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}

	ret = md_create(&handle->md_handle);

	if (ret != MEDIADEMUXER_ERROR_NONE) {
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		free(handle);
		handle = NULL;
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	} else {
		*demuxer = (mediademuxer_h) handle;
		handle->is_stopped = false;
		MD_I("[CoreAPI][%s] new handle : %p", __FUNCTION__, *demuxer);
	}
	/* set callback */
	md_set_error_cb(handle->md_handle,
			(mediademuxer_error_cb) _mediademuxer_error_cb,
			handle);
	md_set_eos_cb(handle->md_handle,
			(mediademuxer_eos_cb) _mediademuxer_eos_cb,
			handle);

	handle->demux_state = MEDIADEMUXER_IDLE;
	return MEDIADEMUXER_ERROR_NONE;
}

int mediademuxer_set_data_source(mediademuxer_h demuxer, const char *path)
{
	MD_I("mediademuxer_set_data_source\n");
	mediademuxer_error_e ret = MEDIADEMUXER_ERROR_NONE;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && path && handle->demux_state == MEDIADEMUXER_IDLE) {
		ret = md_set_data_source((MMHandleType) (handle->md_handle), path);
	} else {
		if (!path) {
			MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
				__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_PATH);
			return MEDIADEMUXER_ERROR_INVALID_PATH;
		} else {
			if (handle->demux_state != MEDIADEMUXER_IDLE)
				return MEDIADEMUXER_ERROR_INVALID_STATE;
			MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
				__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
			return MEDIADEMUXER_ERROR_INVALID_OPERATION;
		}
	}
	return ret;
}

int mediademuxer_prepare(mediademuxer_h demuxer)
{
	MD_I("mediademuxer_prepare\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_IDLE) {
		ret = md_prepare((MMHandleType) (handle->md_handle));
		if (ret == MEDIADEMUXER_ERROR_NONE)
			handle->demux_state = MEDIADEMUXER_READY;
	} else {
		if (handle->demux_state != MEDIADEMUXER_IDLE)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_get_track_count(mediademuxer_h demuxer, int *count)
{
	MD_I("mediademuxer_get_track_count\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_READY) {
		ret = md_get_track_count((MMHandleType) (handle->md_handle), count);
	} else {
		if (handle->demux_state != MEDIADEMUXER_READY)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_select_track(mediademuxer_h demuxer, int track_index)
{
	MD_I("mediademuxer_select_track\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_READY) {
		ret = md_select_track((MMHandleType) (handle->md_handle), track_index);
	} else {
		if (handle->demux_state != MEDIADEMUXER_READY)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_start(mediademuxer_h demuxer)
{
	MD_I("mediademuxer_start\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_READY) {
		ret = md_start((MMHandleType) (handle->md_handle));
		if (ret == MEDIADEMUXER_ERROR_NONE)
			handle->demux_state = MEDIADEMUXER_DEMUXING;
	} else {
		if (handle->demux_state != MEDIADEMUXER_READY)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_get_track_info(mediademuxer_h demuxer, int track_index,
							media_format_h *format)
{
	MD_I("mediademuxer_get_track_info\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (track_index < 0 || format == NULL) {
		MD_E("Invalid input parameters\n");
		return MEDIADEMUXER_ERROR_INVALID_PARAMETER;
	}
	if (handle && (handle->demux_state == MEDIADEMUXER_READY
		|| handle->demux_state == MEDIADEMUXER_DEMUXING)) {
		ret = md_get_track_info((MMHandleType) (handle->md_handle), track_index, format);
	} else {
		if (handle->demux_state != MEDIADEMUXER_READY)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_read_sample(mediademuxer_h demuxer, int track_index,
							media_packet_h *outbuf)
{
	MD_I("mediademuxer_read_sample\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (outbuf == NULL || track_index < 0)
		return MEDIADEMUXER_ERROR_INVALID_PARAMETER;
	if (handle && handle->demux_state == MEDIADEMUXER_DEMUXING) {
		ret = md_read_sample((MMHandleType) (handle->md_handle), track_index, outbuf);
	} else {
		if (handle->demux_state != MEDIADEMUXER_DEMUXING)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_seek(mediademuxer_h demuxer, int64_t pos)
{
	MD_I("mediademuxer_seek\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_DEMUXING) {
		ret = md_seek((MMHandleType) (handle->md_handle), pos);
	} else {
		if (handle->demux_state != MEDIADEMUXER_DEMUXING)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_unselect_track(mediademuxer_h demuxer, int track_index)
{
	MD_I("mediademuxer_unselect_track\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && (handle->demux_state == MEDIADEMUXER_READY
		|| handle->demux_state == MEDIADEMUXER_DEMUXING)) {
		ret = md_unselect_track((MMHandleType) (handle->md_handle), track_index);
	} else {
		if (handle->demux_state != MEDIADEMUXER_READY)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_stop(mediademuxer_h demuxer)
{
	MD_I("mediademuxer_stop\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_DEMUXING) {
		ret = md_stop((MMHandleType) (handle->md_handle));
		if (ret == MEDIADEMUXER_ERROR_NONE)
			handle->demux_state = MEDIADEMUXER_READY;
	} else {
		if (handle->demux_state != MEDIADEMUXER_DEMUXING)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_unprepare(mediademuxer_h demuxer)
{
	MD_I("mediademuxer_unprepare\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_READY) {
		ret = md_unprepare((MMHandleType) (handle->md_handle));
		if (ret == MEDIADEMUXER_ERROR_NONE)
			handle->demux_state = MEDIADEMUXER_IDLE;
	} else {
		if (handle->demux_state != MEDIADEMUXER_READY)
			return MEDIADEMUXER_ERROR_INVALID_STATE;
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_destroy(mediademuxer_h demuxer)
{
	MD_I("mediademuxer_destroy\n");
	mediademuxer_error_e ret;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle && handle->demux_state == MEDIADEMUXER_IDLE) {
		ret = md_destroy(handle->md_handle);
		if (ret != MEDIADEMUXER_ERROR_NONE) {
			MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
				__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
			return MEDIADEMUXER_ERROR_INVALID_OPERATION;
		} else {
			MD_E("[CoreAPI][%s] destroy handle : %p", __FUNCTION__,
			     handle);
		}
	} else {
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
				 __FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		return MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	handle->demux_state = MEDIADEMUXER_NONE;
	return MEDIADEMUXER_ERROR_NONE;
}

int mediademuxer_get_state(mediademuxer_h demuxer, mediademuxer_state *state)
{
	MD_I("mediademuxer_get_state\n");
	int ret = MEDIADEMUXER_ERROR_NONE;
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle = (mediademuxer_s *)(demuxer);
	if (state != NULL) {
		*state = handle->demux_state;
	} else {
		MD_E("[CoreAPI][%s] DEMUXER_ERROR_INVALID_OPERATION(0x%08x)",
			__FUNCTION__, MEDIADEMUXER_ERROR_INVALID_OPERATION);
		ret = MEDIADEMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediademuxer_set_error_cb(mediademuxer_h demuxer,
			mediademuxer_error_cb callback, void *user_data)
{
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle->demux_state != MEDIADEMUXER_IDLE)
		return MEDIADEMUXER_ERROR_INVALID_STATE;
	handle->error_cb = callback;
	handle->error_cb_userdata = user_data;
	MD_I("set error_cb(%p)", callback);
	return MEDIADEMUXER_ERROR_NONE;
}

int mediademuxer_unset_error_cb(mediademuxer_h demuxer)
{
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle->demux_state != MEDIADEMUXER_IDLE)
		return MEDIADEMUXER_ERROR_INVALID_STATE;
	handle->error_cb = NULL;
	handle->error_cb_userdata = NULL;
	MD_I("mediademuxer_unset_error_cb\n");
	return MEDIADEMUXER_ERROR_NONE;
}

static gboolean _mediademuxer_error_cb(mediademuxer_error_e error, void *user_data)
{
	if (user_data == NULL) {
		MD_I("_mediademuxer_error_cb: ERROR %d to report. But call back is not set\n", error);
		return 0;
	}
	mediademuxer_s * handle = (mediademuxer_s *) user_data;
	if (handle->demux_state != MEDIADEMUXER_IDLE)
		return MEDIADEMUXER_ERROR_INVALID_STATE;
	if (handle->error_cb)
		((mediademuxer_error_cb)handle->error_cb)(error, handle->error_cb_userdata);
	else
		MD_I("_mediademuxer_error_cb: ERROR %d to report. But call back is not set\n", error);
	return 0;
}

int mediademuxer_set_eos_cb(mediademuxer_h demuxer,
			mediademuxer_eos_cb callback, void *user_data)
{
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle->demux_state != MEDIADEMUXER_IDLE)
		return MEDIADEMUXER_ERROR_INVALID_STATE;
	handle->eos_cb = callback;
	handle->eos_cb_userdata = user_data;
	MD_I("set eos_cb(%p)", callback);
	return MEDIADEMUXER_ERROR_NONE;
}

int mediademuxer_unset_eos_cb(mediademuxer_h demuxer)
{
	DEMUXER_INSTANCE_CHECK(demuxer);
	mediademuxer_s *handle;
	handle = (mediademuxer_s *)(demuxer);
	if (handle->demux_state != MEDIADEMUXER_IDLE)
		return MEDIADEMUXER_ERROR_INVALID_STATE;
	handle->eos_cb = NULL;
	handle->eos_cb_userdata = NULL;
	MD_I("mediademuxer_unset_eos_cb\n");
	return MEDIADEMUXER_ERROR_NONE;
}

static gboolean _mediademuxer_eos_cb(int track_num, void *user_data)
{
	if (user_data == NULL) {
		MD_I("_mediademuxer_eos_cb: EOS to report. But call back is not set\n");
		return 0;
	}
	mediademuxer_s *handle = (mediademuxer_s *)user_data;
	if (handle->demux_state != MEDIADEMUXER_DEMUXING)
		return MEDIADEMUXER_ERROR_INVALID_STATE;
	if (handle->eos_cb)
		((mediademuxer_eos_cb)handle->eos_cb)(track_num, handle->eos_cb_userdata);
	else
		MD_I("_mediademuxer_eos_cb: EOS %d to report. But call back is not set\n", track_num);
	return 0;
}
