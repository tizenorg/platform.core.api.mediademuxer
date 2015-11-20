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
 * @file mediademuxer_util.h
 * @brief contain information about utility function such as debug, error log and other.
 */

#ifndef __TIZEN_MEDIADEMUXER_UTIL_H__
#define __TIZEN_MEDIADEMUXER_UTIL_H__

#include <glib.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>

#include <mediademuxer_ini.h>
#include <media_format.h>

#ifdef __cplusplus
extern "C" {
#endif

/* #define PRINT_ON_CONSOLE */
#ifdef PRINT_ON_CONSOLE
#include <stdlib.h>
#include <stdio.h>
#define PRINT_F          g_print
#define MD_FENTER();     PRINT_F("function:[%s] ENTER\n", __func__);
#define MD_FLEAVE();     PRINT_F("function [%s] LEAVE\n", __func__);
#define MD_C             PRINT_F
#define MD_E             PRINT_F
#define MD_W             PRINT_F
#define MD_I             PRINT_F
#define MD_L             PRINT_F
#define MD_V             PRINT_F
#define MD_F             PRINT_F
#else
#include <stdlib.h>
#include <stdio.h>
#define MD_FENTER();     LOGI("function:[%s] ENTER\n", __func__);
#define MD_FLEAVE();     LOGI("function [%s] LEAVE\n", __func__);
#define MD_C             LOGE	/* MMF_DEBUG_LEVEL_0 */
#define MD_E             LOGE	/* MMF_DEBUG_LEVEL_1 */
#define MD_W             LOGW	/* MMF_DEBUG_LEVEL_2 */
#define MD_I             LOGI	/* MMF_DEBUG_LEVEL_3 */
#define MD_L             LOGI	/* MMF_DEBUG_LEVEL_4 */
#define MD_V             LOGV	/* MMF_DEBUG_LEVEL_5 */
#define MD_F             LOGF	/* MMF_DEBUG_LEVEL_6 - indicate that something in the executed code path is not fully implemented or handled yet */
#endif

/* general */
#define MEDIADEMUXER_FREEIF(x) \
	if (x) \
		g_free(x); \
	x = NULL;

#if 1
#define MEDIADEMUXER_FENTER();              MD_FENTER();
#define MEDIADEMUXER_FLEAVE();              MD_FLEAVE();
#else
#define MEDIADEMUXER_FENTER();
#define MEDIADEMUXER_FLEAVE();
#endif

#define MEDIADEMUXER_CHECK_NULL(x_var) \
	if (!x_var) { \
		MD_E("[%s] is NULL\n", #x_var); \
		return MD_ERROR_INVALID_ARGUMENT; \
	}

#define MEDIADEMUXER_CHECK_SET_AND_PRINT(x_var, x_cond, ret, ret_val, err_text) \
	if (x_var != x_cond) { \
		ret = ret_val; \
		MD_E("%s\n", #err_text); \
		goto ERROR; \
	}

#ifdef __cplusplus
}
#endif
#endif	/* __TIZEN_MEDIADEMUXER_UTIL_H__ */
