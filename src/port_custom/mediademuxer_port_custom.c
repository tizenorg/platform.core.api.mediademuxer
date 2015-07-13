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
 * @file mediademuxer_port_custom.c
 * @brief Handling for Custom Port, defined function and there implementation
 */

#include <mm_debug.h>
#include <mediademuxer_error.h>
#include <mediademuxer_private.h>
#include <mediademuxer_port.h>
#include <mediademuxer_port_custom.h>

static int custom_demuxer_init(MMHandleType *pHandle);
static int custom_demuxer_prepare(MMHandleType pHandle, char *uri);
static int custom_demuxer_get_data_count(MMHandleType pHandle, int *count);
static int custom_demuxer_get_track_info(MMHandleType pHandle,
                                         media_format_h format, int track);
static int custom_demuxer_set_track(MMHandleType pHandle, int track);
static int custom_demuxer_get_track(MMHandleType pHandle, char *buffer);

/*Media Demuxer API common*/
static media_port_demuxer_ops def_demux_ops = {
	.n_size = 0,
	.init = custom_demuxer_init,
	.prepare = custom_demuxer_prepare,
	.get_track_count = custom_demuxer_get_data_count,
	.get_track_info = custom_demuxer_get_track_info,
	.set_track = custom_demuxer_set_track,
	.get_data = custom_demuxer_get_track,
};

int custom_port_register(media_port_demuxer_ops *pOps)
{
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(pOps);

	def_demux_ops.n_size = sizeof(def_demux_ops);

	memcpy((char *)pOps + sizeof(pOps->n_size),
	       (char *)&def_demux_ops + sizeof(def_demux_ops.n_size),
	       pOps->n_size - sizeof(pOps->n_size));

	MEDIADEMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MD_ERROR_INVALID_ARGUMENT;
	return ret;
}

static int custom_demuxer_init(MMHandleType *pHandle)
{
	int ret = MD_ERROR_NONE;
	MEDIADEMUXER_FENTER();
	MD_E("%s:exit: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return ret;
}

static int custom_demuxer_get_data_count(MMHandleType pHandle, int *count)
{
	MEDIADEMUXER_FENTER();
	MD_E("%s:exit: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return 0;
}

static int custom_demuxer_get_track_info(MMHandleType pHandle,
                                         media_format_h format, int track)
{
	MEDIADEMUXER_FENTER();
	MD_E("%s:exit: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return 0;
}

static int custom_demuxer_set_track(MMHandleType pHandle, int track)
{
	MEDIADEMUXER_FENTER();
	MD_E("%s:exit: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return 0;
}

static int custom_demuxer_get_track(MMHandleType pHandle, char *buffer)
{
	MEDIADEMUXER_FENTER();
	MD_E("%s:exit: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return 0;
}

static int custom_demuxer_prepare(MMHandleType pHandle, char *uri)
{
	MEDIADEMUXER_FENTER();
	MD_E("%s:exit: Not implemented\n", __func__);
	MEDIADEMUXER_FLEAVE();
	return 0;
}
