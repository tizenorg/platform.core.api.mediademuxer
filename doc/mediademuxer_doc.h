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

#ifndef __TIZEN_MEDIADEMUXER_DOC_H__
#define __TIZEN_MEDIADEMUXER_DOC_H__

/**
 * @file mediademuxer_doc.h
 * @brief This file contains high level documentation of the CAPI MEDIA DEMUXER API.
 */

/**
 * @ingroup CAPI_MEDIA_FRAMEWORK
 * @defgroup CAPI_MEDIADEMUXER_MODULE Media Demuxer
 * @brief  The @ref CAPI_MEDIADEMUXER_MODULE  APIs provides functions for demuxing media data
 *
 * @section CAPI_MEDIADEMUXER_MODULE_HEADER Required Header
 *   \#include <mediademuxer.h>
 *
 * @section CAPI_MEDIADEMUXER_MODULE_OVERVIEW Overview
 *
 * MEDIADEMUXER API allows :<br>
 * 1) To extract elementary audio, video or text data from a
 * multiplexed stream<br>
 * 2) To choose one or multiple desired stream to extract.<br>
 * 3) To choose the local or remote input source.<br>
 * 4) To create single or multiple instances of media demuxer. One instance can demux only one<br>
 * input stream<br>
 * 5) To demux all the popular media formats such as MP4, AAC-NB, AAC-WB, MP3 etc.<br>
 * 6) To extract elementarty media sample information, such as timestamp, sample size, key-frame(I-frame) etc.<br>
 * 7) To identify encripted format<br>
 * 8) To seek to a different position-forward or backward- while extracting<br>
 * <br>
 * Typical Call Flow of mediamuxer APIs is:<br>
 * mediademuxer_create()<br>
 * mediademuxer_set_data_source()<br>
 * mediademuxer_prepare()<br>
 * mediademuxer_get_track_count()<br>
 * mediademuxer_select_track()<br>
 * mediademuxer_start()<br>
 * <pre>
 * while(EOS) {
 *	if(track1 is set) {
 *		mediademuxer_read_sample();
 * 		if(seek_request)
 *			mediademuxer_seek();
 * 	}
 *	else if(track2 is set) {
 *		 mediademuxer_read_sample();
 * 	}
 * 	if(track2_not_needed)
 * 		mediademuxer_unselect_track(track2);
 *  }
 * </pre>
 * mediademuxer_stop()<br>
 * mediademuxer_unprepare()<br>
 * mediademuxer_destroy()<br>
 */

#endif /* __TIZEN_MEDIADEMUXER_DOC_H__ */
