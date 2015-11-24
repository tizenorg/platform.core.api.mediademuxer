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
 * @file mediademuxer_ini.h
 * @brief INI laoding and parsing procedures
 */

#ifndef __TIZEN_MEDIADEMUXER_INI_H__
#define __TIZEN_MEDIADEMUXER_INI_H__

#include <glib.h>
#include <mm_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEDIADEMUXER_INI_DEFAULT_PATH   "/usr/etc/mmfw_mediademuxer.ini"
#define MEDIADEMUXER_INI_MAX_STRLEN     100
#define DEFAULT_PORT "GST_PORT"

typedef enum {
	GST_PORT = 0,
	FFMPEG_PORT,
	CUSTOM_PORT,
} port_mode;

/* @ mark means the item has tested */
typedef struct __md_ini {
	port_mode port_type;
	/* general */
	gchar port_name[MEDIADEMUXER_INI_MAX_STRLEN];
} md_ini_t;

/* NOTE : following content should be same with above default values */
/* FIXIT : need smarter way to generate default ini file. */
/* FIXIT : finally, it should be an external file */
#define MEDIADEMUXER_DEFAULT_INI \
	"\
[general] \n\
\n\
;Add general config parameters here\n\
\n\
\n\
\n\
[port_in_use] \n\
\n\
;mediademuxer_port = GST_PORT \n\
;mediademuxer_port = FFMPEG_PORT \n\
;mediademuxer_port = CUSTOM_PORT \n\
mediademuxer_port = GST_PORT \n\
\n\
[gst_port] \n\
\n\
;Add gst port specific config paramters here\n\
\n\
\n\
[ffmpeg_port] \n\
\n\
;Add ffmpeg port specific config paramters here\n\
\n\
\n\
[custom_port] \n\
\n\
;Add custom port specific config paramters here\n\
\n\
\n\
\n\
"

int md_ini_load(md_ini_t *ini);

#ifdef __cplusplus
}
#endif
#endif	/* __TIZEN_MEDIADEMUXER_INI_H__ */