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
 * @file mediademuxer_port.h
 * @brief general port based functions. sets specific function pointers
 */

#ifndef __TIZEN_MEDIADEMUXER_PORT_H__
#define __TIZEN_MEDIADEMUXER_PORT_H__

/*===========================================================================================
|                                                                                           |
|  INCLUDE FILES                                        |
|                                                                                           |
========================================================================================== */

#include <glib.h>
#include <mm_types.h>
#include <mm_message.h>
#include <media_format.h>
#include <mediademuxer_util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
    @{

    @par
    This part describes APIs used for playback of multimedia contents.
    All multimedia contents are created by a media demuxer through handle of playback.
    In creating a demuxer, it displays the demuxer's status or information
    by registering callback function.

    @par
    In case of streaming playback, network has to be opend by using datanetwork API.
    If proxy, cookies and the other attributes for streaming playback are needed,
    set those attributes using mm_demuxer_set_attribute() before create demuxer.

    @par
    The subtitle for local video playback is supported. Set "subtitle_uri" attribute
    using mm_demuxer_set_attribute() before the application creates the demuxer.
    Then the application could receive MMMessageParamType which includes subtitle string and duration.

    @par
    MediaDemuxer can have 5 states, and each state can be changed by calling
    described functions on "Figure1. State of MediaDemuxer".

    @par
    @image html		demuxer_state.jpg	"Figure1. State of MediaDemuxer"	width=12cm
    @image latex	demuxer_state.jpg	"Figure1. State of MediaDemuxer"	width=12cm

    @par
    Most of functions which change demuxer state work as synchronous. But, mm_demuxer_start() should be used
    asynchronously. Both mm_demuxer_pause() and mm_demuxer_resume() should also be used asynchronously
    in the case of streaming data.
    So, application have to confirm the result of those APIs through message callback function.

    @par
    Note that "None" and Null" state could be reached from any state
    by calling mm_demuxer_destroy() and mm_demuxer_unrealize().

    @par
    <div><table>
    <tr>
    <td><B>FUNCTION</B></td>
    <td><B>PRE-STATE</B></td>
    <td><B>POST-STATE</B></td>
    <td><B>SYNC TYPE</B></td>
    </tr>
    <tr>
    <td>md_create()</td>
    <td>NONE</td>
    <td>NULL</td>
    <td>SYNC</td>
    </tr>
    <tr>
    <td>md_destroy()</td>
    <td>NULL</td>
    <td>NONE</td>
    <td>SYNC</td>
    </tr>
    <tr>
    <td>md_set_data_source()</td>
    <td>NULL</td>
    <td>READY</td>
    <td>SYNC</td>
    </tr>
    </table></div>

    @par
    Following are the attributes supported in demuxer which may be set after initialization. \n
    Those are handled as a string.

    @par
    <div><table>
    <tr>
    <td>PROPERTY</td>
    <td>TYPE</td>
    <td>VALID TYPE</td>
    </tr>
    <tr>
    <td>"profile_uri"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_duration"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_video_width"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_video_height"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"profile_user_param"</td>
    <td>data</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"profile_play_count"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_type"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_udp_timeout"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_user_agent"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_wap_profile"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_network_bandwidth"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_cookie"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_proxy_ip"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_proxy_port"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"subtitle_uri"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    </table></div>

    @par
    Following attributes are supported for playing stream data. Those value can be readable only and valid after starting playback.\n
    Please use mm_fileinfo for local playback.

    @par
    <div><table>
    <tr>
    <td>PROPERTY</td>
    <td>TYPE</td>
    <td>VALID TYPE</td>
    </tr>
    <tr>
    <td>"content_video_found"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_video_codec"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_video_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_audio_found"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_audio_codec"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_audio_bitrate"</td>
    <td>int</td>
    <td>array</td>
    </tr>
    <tr>
    <td>"content_audio_channels"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_audio_samplerate"</td>
    <td>int</td>
    <td>array</td>
    </tr>
    <tr>
    <td>"content_audio_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_text_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"tag_artist"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_title"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_album"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_genre"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_author"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_copyright"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_date"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_description"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    </table></div>

 */

/*===========================================================================================
|                                                                                           |
|  GLOBAL DEFINITIONS AND DECLARATIONS                                        |
|                                                                                           |
========================================================================================== */
/**
 * @brief Called when error occurs in media demuxer.
 * @details Following error codes can be delivered.
 *          #MEDIADEMUXER_ERROR_INVALID_OPERATION,
 *          #MEDIADEMUXER_ERROR_NOT_SUPPORTED,
 *          #MEDIADEMUXER_ERROR_INVALID_PATH,
 *          #MEDIADEMUXER_ERROR_RESOURCE_LIMIT,
 *          #MEDIADEMUXER_ERROR_SEEK_FAILED,
 *          #MEDIADEMUXER_ERROR_DRM_NOT_PERMITTED
 * @since_tizen 3.0
 * @param[in] error  The error that occurred in media demuxer
 * @param[in] user_data   The user data passed from the code where
 *                         mediademuxer_set_error_cb() was invoked
 *                         This data will be accessible from @a mediademuxer_error_cb
 * @pre Create media demuxer handle by calling mediademuxer_create() function.
 * @see mediademuxer_set_error_cb()
 * @see mediademuxer_unset_error_cb()
 */
typedef void (*md_error_cb)(mediademuxer_error_e error, void *user_data);

/**
 * @brief Called when eos occurs in media demuxer.
 * @since_tizen 3.0
 * @param[in] track_num  The track_num which indicate eos has occurred in which track numbe
 * @param[in] user_data   The user data passed from the code where
 *                         mediademuxer_set_eos_cb() was invoked
 *                         This data will be accessible from @a mediademuxer_eos_cb
 * @pre Create media demuxer handle by calling mediademuxer_create() function.
 * @see mediademuxer_set_eos_cb()
 * @see mediademuxer_unset_eos_cb()
 */
typedef void (*md_eos_cb)(int track_num, void *user_data);

/**
 * Enumerations of demuxer state.
 */
/**
 * Attribute validity structure
 */
typedef struct _media_port_demuxer_ops {
	unsigned int n_size;
	int (*init)(MMHandleType *pHandle);
	int (*prepare)(MMHandleType pHandle, char *uri);
	int (*get_track_count)(MMHandleType pHandle, int *count);
	int (*set_track)(MMHandleType pHandle, int track);
	int (*start)(MMHandleType pHandle);
	int (*get_track_info)(MMHandleType pHandle, media_format_h *format, int track);
	int (*read_sample)(MMHandleType pHandle, media_packet_h *outbuf, int track_indx);
	int (*seek)(MMHandleType pHandle, int64_t pos1);
	int (*unset_track)(MMHandleType pHandle, int track);
	int (*stop)(MMHandleType pHandle);
	int (*unprepare)(MMHandleType pHandle);
	int (*destroy)(MMHandleType pHandle);
	int (*set_error_cb)(MMHandleType demuxer, md_error_cb callback, void* user_data);
	int (*set_eos_cb)(MMHandleType demuxer, md_eos_cb callback, void* user_data);
	int (*get_data)(MMHandleType pHandle, char *buffer);
} media_port_demuxer_ops;

/*===========================================================================================
|                                                                                           |
|  GLOBAL FUNCTION PROTOTYPES                                        |
|                                                                                           |
========================================================================================== */

/**
 * This function creates a demuxer object for parsing multimedia contents. \n
 * The attributes of demuxer are created to get/set some values with application. \n
 * And, proper port is selected to do the actual parsing of the mdia.
 *
 * @param   demuxer [out]   Handle of demuxer
 *
 * @return  This function returns zero on success, or negative value with error code. \n
 *          Please refer 'mm_demuxer_error.h' to know it in detail.
 *
 * @par Example
 * @code
  MMHandleType demuxer;
  md_create(&demuxer);
  ...
  md_destroy(&demuxer);
 * @endcode
 */
int md_create(MMHandleType *demuxer);

/**
 * This function sets the input data source to parse. \n
 * The source can be a local file or remote server file
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   uri         [in]    absolute path for the source media
 *
 * @return  This function returns zero on success, or negative value with error code. \n
 *          Please refer 'mm_demuxer_error.h' to know it in detail.
 *
 * @par Example
 * @code
  MPhandle demuxer;
  md_create(&demuxer);
  if (md_set_data_source(demuxer) != MM_ERROR_NONE)
{
    MD_E("failed to set the source \n");
}
  md_destroy(&demuxer);
 * @endcode
 */
int md_set_data_source(MMHandleType demuxer, const char *uri);

/**
 * This function releases demuxer object and all resources which were created by md_create(). \n
 * And, demuxer handle will also be destroyed.
 *
 * @param   demuxer     [in]    Handle of demuxer
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_create
 *
 * @par Example
 * @code
if (md_destroy(g_demuxer) != MM_ERROR_NONE)
{
    MD_E("failed to destroy demuxer\n");
}
 * @endcode
 */
int md_destroy(MMHandleType demuxer);

/**
 * This function set up the internal structure of gstreamer, ffmpeg & custom to handle data set up md_set_data_source
 *
 * @param   demuxer     [in]    Handle of demuxer
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 *
 * @par Example
 * @code
if (md_prepare(g_demuxer) != MM_ERROR_NONE)
{
    MD_E("failed to prepare demuxer\n");
}
 * @endcode
 */
int md_prepare(MMHandleType demuxer);

/**
 * This function get the total number of tracks present in the stream.
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   count     [out]    Total track number of streams available in the media
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 *
 * @par Example
 * @code
if (md_get_track_count(g_demuxer,count) != MM_ERROR_NONE)
{
	MD_E("failed to get count\n");
}
 * @endcode
 */
int md_get_track_count(MMHandleType demuxer, int *count);

/**
 * This function get the  format of selected track present in the stream.
 *
 * @param   demuxer            [in]    Handle of demuxer
 * @param   track              [in]    index for the track
 * @param   format             [out]   pointer to the Handle of media_format
 *
 * @return  This function returns zero on success, or negative value with error code.
 *
 * @par Example
 * @code
if (md_get_track_info(g_demuxer, format, track) != MM_ERROR_NONE)
{
	MD_E("failed to get track info\n");
}
 * @endcode
 */
int md_get_track_info(MMHandleType demuxer, int track, media_format_h *format);

/**
 * This function start the pipeline(in case of gst port it set the pipeline to play state).
 *
 * @param   demuxer     [in]    Handle of demuxer
 *
 * @return  This function returns zero on success, or negative value with error code.
 */
int md_start(MMHandleType demuxer);

/**
 * This function read a packet of data from demuxer
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   track_indx  [in]   selected track number
 * @param   outbuf      [out]    demuxed packet
 *
 * @return  This function returns zero on success, or negative value with error code.
 */
int md_read_sample(MMHandleType demuxer, int track_indx, media_packet_h *outbuf);

/**
 * This function stop the pipeline in case of gst_port
 *
 * @param   demuxer     [in]    Handle of demuxer
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 */
int md_stop(MMHandleType demuxer);

/**
 * This function destroy the pipeline
 *
 * @param   demuxer     [in]    Handle of demuxer
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 */
int md_unprepare(MMHandleType demuxer);

/**
 * This function is to select a track on which read need to be performed
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   index       [in]    index for the track to be selected
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 */
int md_select_track(MMHandleType demuxer, int index);

/**
 * This function is to unselect a track on which readsample can't be performed
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   index       [in]    index for the track to be selected
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 */
int md_unselect_track(MMHandleType demuxer, int index);

/**
 * This function is to seek to a particular position relative to current position
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   pos         [in]    relative position to seek
 *
 * @return  This function returns zero on success, or negative value with error code.
 * @see     md_set_data_source
 */
int md_seek(MMHandleType demuxer, int64_t pos);

/**
 * This function is to set error call back function
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   callback    [in]    call back function pointer
 * @param   user_data   [in]    user specific data pointer
 *
 * @return  This function returns zero on success, or negative value with error code.
 */
int md_set_error_cb(MMHandleType demuxer, md_error_cb callback, void *user_data);

/**
 * This function is to set eos call back function
 *
 * @param   demuxer     [in]    Handle of demuxer
 * @param   callback    [in]    call back function pointer
 * @param   user_data   [in]    user specific data pointer
 *
 * @return  This function returns zero on success, or negative value with error code.
 */
int md_set_eos_cb(MMHandleType demuxer, md_eos_cb callback, void *user_data);

#ifdef __cplusplus
}
#endif
#endif	/* __TIZEN_MEDIADEMUXER_PORT_H__ */
