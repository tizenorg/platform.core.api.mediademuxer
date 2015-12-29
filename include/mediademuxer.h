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

#ifndef __TIZEN_MEDIADEMUXER_H__
#define __TIZEN_MEDIADEMUXER_H__

#include <tizen.h>
#include <stdint.h>
#include <media_format.h>
#include <media_packet.h>

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifndef TIZEN_ERROR_MEDIA_DEMUXER
#define TIZEN_ERROR_MEDIA_DEMUXER -0x04000000
#endif

/**
 * @file mediademuxer.h
 * @brief This file contains the capi media demuxer API.
 */

/**
 * @addtogroup CAPI_MEDIADEMUXER_MODULE
 * @{
 */

/**
 * @brief Media Demuxer handle type.
 * @since_tizen 3.0
 */
typedef struct mediademuxer_s *mediademuxer_h;

/**
 * @brief Enumeration for media demuxer state
 * @since_tizen 3.0
 */
typedef enum {
	MEDIADEMUXER_NONE,		/**< The mediademuxer is not created */
	MEDIADEMUXER_IDLE,			/**< The mediademuxer is created, but not prepared */
	MEDIADEMUXER_READY,		/**< The mediademuxer is ready to demux media */
	MEDIADEMUXER_DEMUXING		/**< The mediademuxer is demuxing media */
} mediademuxer_state;

/**
 * @brief Enumeration for media demuxer error.
 * @since_tizen 3.0
 */
typedef enum {
	MEDIADEMUXER_ERROR_NONE = TIZEN_ERROR_NONE,	/*< Successful */
	MEDIADEMUXER_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of memory */
	MEDIADEMUXER_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	MEDIADEMUXER_ERROR_INVALID_OPERATION = TIZEN_ERROR_INVALID_OPERATION,	/**< Invalid operation */
	MEDIADEMUXER_ERROR_NOT_SUPPORTED = TIZEN_ERROR_NOT_SUPPORTED,	/**< Not supported */
	MEDIADEMUXER_ERROR_PERMISSION_DENIED = TIZEN_ERROR_PERMISSION_DENIED,	/**< Permission denied */
	MEDIADEMUXER_ERROR_INVALID_STATE = TIZEN_ERROR_MEDIA_DEMUXER | 0x01,	/**< Invalid state */
	MEDIADEMUXER_ERROR_INVALID_PATH = TIZEN_ERROR_MEDIA_DEMUXER | 0x02,	/**< Invalid path */
	MEDIADEMUXER_ERROR_RESOURCE_LIMIT = TIZEN_ERROR_MEDIA_DEMUXER | 0x03,	/**< Resource limit */
	MEDIADEMUXER_ERROR_SEEK_FAILED = TIZEN_ERROR_MEDIA_DEMUXER | 0x04,	/**< Seek operation failure */
	MEDIADEMUXER_ERROR_DRM_NOT_PERMITTED = TIZEN_ERROR_MEDIA_DEMUXER | 0x05	/**< Not permitted format */
} mediademuxer_error_e;

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
 *                         This data will be accessible from mediademuxer_error_cb()
 * @pre Create media demuxer handle by calling mediademuxer_create() function.
 * @see mediademuxer_set_error_cb()
 * @see mediademuxer_unset_error_cb()
 */
typedef void (*mediademuxer_error_cb) (mediademuxer_error_e error, void *user_data);

/**
 * @brief Called when end of stream occurs in media demuxer.
 * @since_tizen 3.0
 * @param[in] track_num  The track_num which indicate eos for which track number occured
 * @param[in] user_data   The user data passed from the code where
 *                         mediademuxer_set_eos_cb() was invoked
 *                         This data will be accessible from mediademuxer_eos_cb()
 * @pre Create media demuxer handle by calling mediademuxer_create() function.
 * @see mediademuxer_set_eos_cb()
 * @see mediademuxer_unset_eos_cb()
 */
typedef void (*mediademuxer_eos_cb) (int track_num, void *user_data);

/**
 * @brief Creates a media demuxer handle for demuxing.
 * @since_tizen 3.0
 * @remarks You must release @a demuxer using mediademuxer_destroy() function.
 * @param[out] demuxer    A new handle to media demuxer
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @retval #MEDIADEMUXER_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @post The media demuxer state will be #MEDIADEMUXER_STATE_IDLE.
 * @see mediademuxer_destroy()
 */
int mediademuxer_create(mediademuxer_h *demuxer);

/**
 * @brief Sets the source path of input stream.
 * @since_tizen 3.0
 * @remarks The mediastorage privilege(http://tizen.org/privilege/mediastorage) should be added if any video/audio files are used to play located in the internal storage.
 * @remarks The externalstorage privilege(http://tizen.org/privilege/externalstorage) should be added if any video/audio files are used to play located in the external storage.
 * @remarks You must release @a demuxer using mediademuxer_destroy() function.
 * @param[in] demuxer    The media demuxer handle
 * @param[in] path    The content location, such as the file path
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @retval #MEDIADEMUXER_ERROR_INVALID_PATH Invalid path
 * @pre The media muxer state will be #MEDIADEMUXER_STATE_IDLE by calling mediademuxer_create() function.
 * */
int mediademuxer_set_data_source(mediademuxer_h demuxer, const char *path);

/**
 * @brief Prepares the media demuxer for demuxing.
 * @since_tizen 3.0
 * @remark User should call this before mediademuxer_start() function.
 * @param[in] demuxer    The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state should be #MEDIADEMUXER_STATE_IDLE.
 * @post The media demuxer state will be #MEDIADEMUXER_STATE_READY.
 * @see mediademuxer_set_data_source()
 * @see mediademuxer_unprepare()
 * */
int mediademuxer_prepare(mediademuxer_h demuxer);

/**
 * @brief Gets the total track count present in the container stream.
 * @since_tizen 3.0
 * @param[in] demuxer    The media demuxer handle
 * @param[out] count     The number of tracks present
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state should be #MEDIADEMUXER_STATE_READY.
 * @see mediademuxer_prepare()
 * @see mediademuxer_select_track()
 * */
int mediademuxer_get_track_count(mediademuxer_h demuxer, int *count);

/**
 * @brief Selects the track to be performed.
 * @since_tizen 3.0
 * @param[in] demuxer    The media demuxer handle
 * @param[in] track_index      The track index on which is selected for read
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state should be #MEDIADEMUXER_STATE_READY.
 * @see mediademuxer_get_track_count()
 * @see mediademuxer_start()
 * */
int mediademuxer_select_track(mediademuxer_h demuxer, int track_index);

/**
 * @brief Starts the media demuxer.
 * @since_tizen 3.0
 * @remark User should call this before mediademuxer_read_sample() function.
 * @param[in] demuxer    The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state should be #MEDIADEMUXER_STATE_READY.
 * @post The media demuxer state will be #MEDIADEMUXER_STATE_DEMUXING.
 * @see mediademuxer_prepare()
 * @see mediademuxer_get_track_count()
 * @see mediademuxer_select_track()
 * @see mediademuxer_get_track_info()
 * */
int mediademuxer_start(mediademuxer_h demuxer);

/**
 * @brief Retrieves the track format of the read sample.
 * @since_tizen 3.0
 * @remarks The @a format should be released using media_format_unref() function.
 * @param[in] demuxer    The media demuxer handle
 * @param[in] track_index     The index of the track
 * @param[out] format    The media format handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state must be set to #MEDIADEMUXER_STATE_DEMUXING by calling
 *      mediademuxer_start() or set to #MEDIADEMUXER_STATE_READY by calling mediademuxer_prepare().
 * @see mediademuxer_get_track_count()
 * @see mediademuxer_select_track()
 * @see media_format_unref()
 * @see #media_format_h
 * */
int mediademuxer_get_track_info(mediademuxer_h demuxer, int track_index, media_format_h *format);

/**
 * @brief Reads a frame(sample) of one single track.
 * @since_tizen 3.0
 * @remark The @a outbuf should be released using media_packet_destroy() function.
 * @remark Once this API is called, user app can call the mediatool APIs to extract
 *          side information such as pts, size, duration, flags etc.
 * @param[in] demuxer    The media demuxer handle
 * @param[in] track_index      The index of track of which data is needed
 * @param[out] outbuf      The media packet handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state should be #MEDIADEMUXER_STATE_DEMUXING.
 * @see mediademuxer_start()
 * @see mediademuxer_get_track_info()
 * @see mediademuxer_seek() if need to seek to a particular location
 * @see mediademuxer_unselect_track()
 * @see mediademuxer_stop()
 * @see media_packet_destroy()
 * @see #media_packet_h
 * */
int mediademuxer_read_sample(mediademuxer_h demuxer, int track_index, media_packet_h *outbuf);

/**
 * @brief Seeks to a particular instance of time (in micro seconds).
 * @since_tizen 3.0
 * @param[in] demuxer    The media demuxer handle
 * @param[in] pos       The value of the new start position
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
* @pre The media demuxer state should be #MEDIADEMUXER_STATE_DEMUXING.
 * @see mediademuxer_read_sample()
 * @see mediademuxer_stop()
 * */
int mediademuxer_seek(mediademuxer_h demuxer, int64_t pos);

/**
 * @brief Unselects the selected track.
 * @since_tizen 3.0
 * @param[in] demuxer    The media demuxer handle
 * @param[in] track_index      The track index to be unselected
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state must be set to #MEDIADEMUXER_STATE_DEMUXING by calling
 *      mediademuxer_read_sample() or set to #MEDIADEMUXER_STATE_READY by calling mediademuxer_select_track().
 * @see mediademuxer_select_track()
 * @see mediademuxer_read_sample()
 * */
int mediademuxer_unselect_track(mediademuxer_h demuxer, int track_index);

/**
 * @brief Stops the media demuxer.
 * @since_tizen 3.0
 * @remark User can call this if need to stop demuxing if needed.
 * @param[in] demuxer    The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state must be set to #MEDIADEMUXER_STATE_DEMUXING.
 * @post The media demuxer state will be in  #MEDIADEMUXER_READY.
 * @see mediademuxer_start()
 * @see mediademuxer_unprepare()
 * */
int mediademuxer_stop(mediademuxer_h demuxer);

/**
 * @brief Resets the media demuxer.
 * @since_tizen 3.0
 * @remarks User should call this before mediademuxer_destroy() function.
 * @param[in] demuxer    The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre The media demuxer state should be #MEDIADEMUXER_STATE_READY.
 * @post The media demuxer state will be #MEDIADEMUXER_STATE_IDLE.
 * @see mediademuxer_prepare()
 * */
int mediademuxer_unprepare(mediademuxer_h demuxer);

/**
 * @brief Removes the instance of media demuxer and clear all its context memory.
 * @since_tizen 3.0
 * @param[in] demuxer    The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid Operation
 * @pre Create a media demuxer handle by calling mediademuxer_create() function.
 * @post The media demuxer state will be #MEDIADEMUXER_STATE_NONE.
 * @see mediademuxer_create()
 * */
int mediademuxer_destroy(mediademuxer_h demuxer);

/**
 * @brief Gets media demuxer state.
 * @since_tizen 3.0
 * @param[in] demuxer   The media demuxer handle
 * @param[out] state   The media demuxer sate
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_OPERATION Invalid operation
 * @pre Create a media demuxer handle by calling mediademuxer_create() function.
 * @see #mediademuxer_state
 * */
int mediademuxer_get_state(mediademuxer_h demuxer, mediademuxer_state *state);

/**
 * @brief Registers an error callback function to be invoked when an error occurs.
 * @since_tizen 3.0
 * @param[in] demuxer   The media demuxer handle
 * @param[in] callback  Callback function pointer
 * @param[in] user_data   The user data passed from the code where
 *                         mediademuxer_set_error_cb() was invoked
 *                         This data will be accessible from mediademuxer_error_cb()
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @pre Create a media demuxer handle by calling mediademuxer_create() function.
 * @post mediademuxer_error_cb() will be invoked.
 * @see mediademuxer_unset_error_cb()
 * @see mediademuxer_error_cb()
 * */
int mediademuxer_set_error_cb(mediademuxer_h demuxer, mediademuxer_error_cb callback, void *user_data);

/**
 * @brief Unregisters the error callback function.
 * @since_tizen 3.0
 * @param[in] demuxer   The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @see mediademuxer_error_cb()
 * */
int mediademuxer_unset_error_cb(mediademuxer_h demuxer);

/**
 * @brief Registers an eos callback function to be invoked when an eos occurs.
 * @since_tizen 3.0
 * @param[in] demuxer   The media demuxer handle
 * @param[in] callback  Callback function pointer
 * @param[in] user_data   The user data passed from the code where
 *                         mediademuxer_set_eos_cb() was invoked
 *                         This data will be accessible from mediademuxer_eos_cb()
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @pre Create a media demuxer handle by calling mediademuxer_create() function.
 * @post mediademuxer_eos_cb() will be invoked.
 * @see mediademuxer_unset_eos_cb()
 * @see mediademuxer_eos_cb()
 * */
int mediademuxer_set_eos_cb(mediademuxer_h demuxer, mediademuxer_eos_cb callback, void *user_data);

/**
 * @brief Unregisters the eos callback function.
 * @since_tizen 3.0
 * @param[in] demuxer   The media demuxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIADEMUXER_ERROR_NONE Successful
 * @retval #MEDIADEMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIADEMUXER_ERROR_INVALID_STATE Invalid state
 * @see mediademuxer_eos_cb()
 * */
int mediademuxer_unset_eos_cb(mediademuxer_h demuxer);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif	/* __TIZEN_MEDIADEMUXER_H__ */
