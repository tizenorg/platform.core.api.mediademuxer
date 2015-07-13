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

#ifndef __TIZEN_MEDIADEMUXER_ERROR_H__
#define __TIZEN_MEDIADEMUXER_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MD_ERROR_NONE = 0,
	MD_ERROR = -1,		    /**< codec happens error */
	MD_MEMORY_ERROR = -2,	    /**< codec memory is not enough */
	MD_PARAM_ERROR = -3,	    /**< codec parameter is error */
	MD_INVALID_ARG = -4,	    /** < codec has invalid arguments */
	MD_PERMISSION_DENIED = -5,
	MD_INVALID_STATUS = -6,	    /**< codec works at invalid status */
	MD_NOT_SUPPORTED = -7,	    /**< codec can't support this specific video format */
	MD_INVALID_IN_BUF = -8,
	MD_INVALID_OUT_BUF = -9,
	MD_INTERNAL_ERROR = -10,
	MD_HW_ERROR = -11,	    /**< codec happens hardware error */
	MD_NOT_INITIALIZED = -12,
	MD_INVALID_STREAM = -13,
	MD_OUTPUT_BUFFER_EMPTY = -14,
	MD_OUTPUT_BUFFER_OVERFLOW = -15,    /**< codec output buffer is overflow */
	MD_MEMORY_ALLOCED = -16,    /**< codec has got memory and can decode one frame */
	MD_COURRPTED_INI = -17,	    /**< value in the ini file is not valid */
	MD_ERROR_FILE_NOT_FOUND = -18,
	MD_EOS,  /** read sample reached end of stream */
} md_ret_e;

#define MD_ERROR_CLASS              0x80000000	    /**< Definition of number describing error group */
#define MD_ERROR_COMMON_CLASS       0x80000100	    /**< Category for describing common error group */
#define MD_ERROR_GST_PORT_CLASS     0x80000200	    /**< Category for describing gst_port error group */
#define MD_ERROR_FFMPEG_PORT_CLASS  0x80000300	    /**< Category for describing ffmpeg port error group */
#define MD_ERROR_CUSTOM_PORT_CLASS       0x80000400	 /**< Category for describing custom error group */

/*
      MD_ERROR_CLASS
*/
#define MD_ERROR_UNKNOWN            (MD_ERROR_CLASS | 0x00)	/**< Unclassified error */
#define MD_ERROR_INVALID_ARGUMENT       (MD_ERROR_CLASS | 0x01)	    /**< Invalid argument */
#define MD_ERROR_OUT_OF_MEMORY          (MD_ERROR_CLASS | 0x02)	    /**< Out of memory */
#define MD_ERROR_OUT_OF_STORAGE         (MD_ERROR_CLASS | 0x03)	    /**< Out of storage */
#define MD_ERROR_INVALID_HANDLE         (MD_ERROR_CLASS | 0x04)	    /**< Invalid handle */
#define MD_ERROR_FILE_NOT_FOUND         (MD_ERROR_CLASS | 0x05)	    /**< Cannot find file */
#define MD_ERROR_FILE_READ          (MD_ERROR_CLASS | 0x06)	/**< Fail to read data from file */
#define MD_ERROR_FILE_WRITE         (MD_ERROR_CLASS | 0x07)	/**< Fail to write data to file */
#define MD_ERROR_END_OF_FILE        (MD_ERROR_CLASS | 0x08)	/**< End of file */
#define MD_ERROR_NOT_SUPPORT_API        (MD_ERROR_CLASS | 0x09)	    /**< Not supported API*/
#define MD_ERROR_PORT_REG_FAILED        (MD_ERROR_CLASS | 0x0a)	    /**< port regitstration failed error */

/*
	MD_ERROR_COMMON_CLASS
*/
#define MD_ERROR_COMMON_INVALID_ARGUMENT    (MD_ERROR_COMMON_CLASS | 1)	    /**< Invalid argument */
#define MD_ERROR_COMMON_NO_FREE_SPACE       (MD_ERROR_COMMON_CLASS | 2)	    /**< Out of storage */
#define MD_ERROR_COMMON_OUT_OF_MEMORY       (MD_ERROR_COMMON_CLASS | 3)	    /**< Out of memory */
#define MD_ERROR_COMMON_UNKNOWN             (MD_ERROR_COMMON_CLASS | 4)	    /**< Unknown error */
#define MD_ERROR_COMMON_INVALID_ATTRTYPE    (MD_ERROR_COMMON_CLASS | 5)	    /**< Invalid argument */
#define MD_ERROR_COMMON_INVALID_PERMISSION  (MD_ERROR_COMMON_CLASS | 6)	    /**< Invalid permission */
#define MD_ERROR_COMMON_OUT_OF_ARRAY        (MD_ERROR_COMMON_CLASS | 7)	    /**< Out of array */
#define MD_ERROR_COMMON_OUT_OF_RANGE        (MD_ERROR_COMMON_CLASS | 8)	    /**< Out of value range*/
#define MD_ERROR_COMMON_ATTR_NOT_EXIST      (MD_ERROR_COMMON_CLASS | 9)	    /**< Attribute doesn't exist. */

/*
 *	MD_ERROR_GST_PORT_CLASS
 */
#define MD_ERROR_GST_PORT_NOT_INITIALIZED   (MD_ERROR_GST_PORT_CLASS | 0x01)	    /**< GST Port  instance is not initialized */

/*
    MD_ERROR_FFMPEG_PORT_CLASS
 */
#define MD_ERROR_FFMPEG_PORT_NOT_INITIALIZED    (MD_ERROR_FFMPEG_PORT_CLASS | 0x01)	/**< FFMPEG Port instance is not initialized */

/*
    MD_ERROR_CUSTOM_PORT_CLASS
*/
#define MD_ERROR_CUSTOM_PORT_NOT_INITIALIZED    (MD_ERROR_CUSTOM_PORT_CLASS | 0x01)	/**< CUSTOM Port instance is not initialized */

/**
    @}
*/

#ifdef __cplusplus
}
#endif
#endif /* __TIZEN_MEDIADEMUXER_ERROR_H__ */
