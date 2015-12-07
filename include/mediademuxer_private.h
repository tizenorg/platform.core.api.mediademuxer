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
 * @file mediademuxer_private.h
 * @brief contain information about the pError and return type, enum and structure prototype
 */

#ifndef __TIZEN_MEDIADEMUXER_PRIVATE_H__
#define __TIZEN_MEDIADEMUXER_PRIVATE_H__

#include <mediademuxer.h>
#include <mediademuxer_port.h>
#include <mediademuxer_ini.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_MEDIADEMUXER"

#define DEMUXER_CHECK_CONDITION(condition, error, msg)     \
	if (condition) {} else \
		{ LOGE("[%s] %s(0x%08x)", __FUNCTION__, msg, error); return error; }; \

#define DEMUXER_INSTANCE_CHECK(demuxer)   \
	DEMUXER_CHECK_CONDITION(demuxer != NULL, MEDIADEMUXER_ERROR_INVALID_PARAMETER, "DEMUXER_ERROR_INVALID_PARAMETER")

#define DEMUXER_STATE_CHECK(demuxer, expected_state)       \
	DEMUXER_CHECK_CONDITION(demuxer->state == expected_state, MEDIADEMUXER_ERROR_INVALID_STATE, "DEMUXER_ERROR_INVALID_STATE")

#define DEMUXER_NULL_ARG_CHECK(arg)      \
	DEMUXER_CHECK_CONDITION(arg != NULL, MEDIADEMUXER_ERROR_INVALID_PARAMETER, "DEMUXER_ERROR_INVALID_PARAMETER")

/**
 * @brief Enumeration for media demuxer source type
 * @since_tizen 3.0
 */
typedef enum {
	MEDIADEMUXER_SRC_NONE = 0,		/**<  Not defined src type */
	MEDIADEMUXER_SRC_FILE,		/**<  Local file src type */
	MEDIADEMUXER_SRC_RTP,		/**<  Rtp src type */
	MEDIADEMUXER_SRC_WFD,		/**<  Wfd src type */
	MEDIADEMUXER_SRC_HTTP,		/**<  Http src type */
	MEDIADEMUXER_SRC_SS,		/**<  Smooth streaming src type */
	MEDIADEMUXER_SRC_RTSP,		/**<  Rtsp src type */
	MEDIADEMUXER_SRC_UNKNOWN,	/**< Unknown src type */
	MEDIADEMUXER_SRC_INVALID	/**<  Invalid src type */
} mediademuxer_src_type;

typedef struct _mediademuxer_s {
	MMHandleType md_handle;
	int state;
	bool is_stopped;
	pthread_t prepare_async_thread;
	mediademuxer_error_cb error_cb;
	void* error_cb_userdata;
	mediademuxer_eos_cb eos_cb;
	void* eos_cb_userdata;
	mediademuxer_state demux_state;
} mediademuxer_s;

typedef struct {
	/* initialize values */
	media_port_demuxer_ops *demuxer_ops;
	/* initialize values */
	md_ini_t ini;
	/* port specific handle */
	/* Source information */
	char *uri_src;
	mediademuxer_src_type uri_src_media_type;
	MMHandleType mdport_handle;
} md_handle_t;

int __convert_error_code(int code, char *func_name);

#ifdef __cplusplus
}
#endif
#endif	/* __TIZEN_MEDIADEMUXER_PRIVATE_H__ */
