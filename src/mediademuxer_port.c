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
 * @file mediademuxer_port.c
 * @brief general port based functions. sets specific function pointers
 */

#include <string.h>
#include <mm_types.h>
#include <mm_message.h>
#include <mm_debug.h>
#include <mediademuxer.h>
#include <mediademuxer_ini.h>
#include <mediademuxer_error.h>
#include <mediademuxer_private.h>
#include <mediademuxer_port.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_ERR_LEN 256

/* function type */
extern int gst_port_register(media_port_demuxer_ops *pOps);
extern int ffmpeg_port_register(media_port_demuxer_ops *pOps);
extern int custom_port_register(media_port_demuxer_ops *pOps);
int __md_util_exist_file_path(const char *file_path);
bool __md_util_is_sdp_file(const char *path);
mediademuxer_src_type __md_util_media_type(char **uri);
int _md_util_parse(MMHandleType demuxer, const char *type);

/*
  * Sequence of functions should be same as the port enumeration
  * "port_mode" in mm_demuxer_ini.h file
  */
typedef int (*register_port)(media_port_demuxer_ops *);
register_port register_port_func[] = {
	&gst_port_register,
	&ffmpeg_port_register,
	&custom_port_register
};

int md_create(MMHandleType *demuxer)
{
	int result = MD_ERROR_NONE;
	media_port_demuxer_ops *pOps = NULL;
	md_handle_t *new_demuxer = NULL;
	MEDIADEMUXER_FENTER();
	new_demuxer = (md_handle_t *) g_malloc(sizeof(md_handle_t));
	MD_I("md_create allocatiing new_demuxer %p:\n", new_demuxer);
	MEDIADEMUXER_CHECK_NULL(new_demuxer);
	memset(new_demuxer, 0, sizeof(md_handle_t));

	/* alloc ops structure */
	pOps = (media_port_demuxer_ops *) g_malloc(sizeof(media_port_demuxer_ops));
	MEDIADEMUXER_CHECK_NULL(pOps);

	new_demuxer->demuxer_ops = pOps;
	MD_I("md_create allocating new_demuxer->demuxer_ops %p:\n",
	     new_demuxer->demuxer_ops);
	pOps->n_size = sizeof(media_port_demuxer_ops);

	new_demuxer->uri_src = NULL;
	new_demuxer->uri_src_media_type = MEDIADEMUXER_SRC_INVALID;
	/* load ini files */
	result = md_ini_load(&new_demuxer->ini);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_COURRPTED_INI, "can't load ini");

	register_port_func[new_demuxer->ini.port_type](pOps);
	result = pOps->init(&new_demuxer->mdport_handle);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_NOT_INITIALIZED, "md_create failed");
	*demuxer = (MMHandleType) new_demuxer;
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	*demuxer = (MMHandleType) 0;
	if (pOps)
		g_free(pOps);
	if (new_demuxer)
		g_free(new_demuxer);
	MEDIADEMUXER_FLEAVE();
	return result;
}

int __md_util_exist_file_path(const char *file_path)
{
	int fd = 0;
	struct stat stat_results = { 0, };
	MEDIADEMUXER_FENTER();

	if (!file_path || !strlen(file_path))
		return MD_ERROR_FILE_NOT_FOUND;

	fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		char buf[MAX_ERR_LEN];
		int ret_err = 0;
		ret_err = strerror_r(errno, buf, MAX_ERR_LEN);
		if (0 == ret_err)
			MD_E("failed to open file by %s (%d)\n", buf, errno);
		else
			MD_E("File not found, strerror_r() failed with errno (%d)\n", errno);
		MEDIADEMUXER_FLEAVE();
		return MD_ERROR_FILE_NOT_FOUND;
	}

	if (fstat(fd, &stat_results) < 0) {
		MD_E("failed to get file status\n");
		close(fd);
		MEDIADEMUXER_FLEAVE();
		return MD_ERROR_FILE_NOT_FOUND;
	} else if (stat_results.st_size == 0) {
		MD_E("file size is zero\n");
		close(fd);
		MEDIADEMUXER_FLEAVE();
		return MD_ERROR_FILE_NOT_FOUND;
	} else {
		MD_E("file size : %lld bytes\n", (long long)stat_results.st_size);
	}
	close(fd);
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
}

bool __md_util_is_sdp_file(const char *path)
{
	gboolean ret = FALSE;
	gchar *uri = NULL;
	MEDIADEMUXER_FENTER();
	return_val_if_fail(path, FALSE);
	uri = g_ascii_strdown(path, -1);
	if (uri == NULL)
		return FALSE;
	/* trimming */
	g_strstrip(uri);
	/* strlen(".sdp") == 4 */
	if (strlen(uri) <= 4) {
		MD_W("path is too short.\n");
		return ret;
	}
	/* first, check extension name */
	ret = g_str_has_suffix(uri, "sdp");
	/* second, if no suffix is there, check it's contents */
	if (!ret)
		/* FIXIT : do it soon */
	g_free(uri);
	uri = NULL;
	MEDIADEMUXER_FLEAVE();
	return ret;
}

mediademuxer_src_type __md_util_media_type(char **uri)
{
	char *path = NULL;
	char *new_uristr = NULL;
	char *old_uristr = NULL;
	MEDIADEMUXER_FENTER();
	if ((path = strstr((*uri), "file://"))) {
		int file_stat = MD_ERROR_NONE;
		file_stat = __md_util_exist_file_path(path + 7);
		if (file_stat == MD_ERROR_NONE) {
			if (__md_util_is_sdp_file(path)) {
				MD_L("uri is actually a file but it's sdp file. giving it to rtspsrc\n");
				return MEDIADEMUXER_SRC_RTSP;
			} else {
				return MEDIADEMUXER_SRC_FILE;
			}
			return MD_ERROR_NONE;
		} else {
			MD_E("could  access %s.\n", path);
		}
	} else if ((path = strstr(*uri, "rtsp://"))) {
		if (strlen(path)) {
			if ((path = strstr(*uri, "/wfd1.0/")))
				return (MEDIADEMUXER_SRC_WFD);
			else
				return (MEDIADEMUXER_SRC_RTSP);
		}
	} else if ((path = strstr(*uri, "http://"))) {
		if (strlen(path)) {
			if (g_str_has_suffix(g_ascii_strdown(*uri, strlen(*uri)), ".ism/manifest") ||
			     g_str_has_suffix(g_ascii_strdown(*uri, strlen(*uri)), ".isml/manifest")) {
				return (MEDIADEMUXER_SRC_SS);
			} else {
				return (MEDIADEMUXER_SRC_HTTP);
			}
		}
	} else {
		int file_stat = MD_ERROR_NONE;
		file_stat = __md_util_exist_file_path(*uri);
		if (file_stat == MD_ERROR_NONE) {
			int len_uri = strlen(*uri);
			old_uristr = (char *)g_malloc(sizeof(char) * (len_uri + 1));
			MEDIADEMUXER_CHECK_NULL(old_uristr);
			MD_L("allocating temp old_uristr[%p] \n", old_uristr);
			strncpy(old_uristr, *uri, len_uri + 1);
			/* need to added 7 char for file:// + 1 for '\0'+ uri len */
			new_uristr = (char *)realloc(*uri, (7 + len_uri + 1) * sizeof(char));
			if (!new_uristr) {
				free(old_uristr);
				old_uristr = NULL;
				return MD_ERROR_INVALID_ARGUMENT;
			}
			MD_L("reallocating uri[%p] to new_uristr[%p] \n", *uri, new_uristr);
			*uri = new_uristr;
			g_snprintf(*uri, 7 + len_uri + 1, "file://%s", old_uristr);
			MD_L("deallocating old_uristr[%p] \n", old_uristr);
			free(old_uristr);
			old_uristr = NULL;
			if (__md_util_is_sdp_file((char *)(*uri))) {
				MD_L("uri is actually a file but it's sdp file. giving it to rtspsrc\n");
				return (MEDIADEMUXER_SRC_RTSP);
			} else {
				return (MEDIADEMUXER_SRC_FILE);
			}
		} else {
			goto ERROR;
		}
	}
	MEDIADEMUXER_FLEAVE();
	return MD_ERROR_NONE;
ERROR:
	MEDIADEMUXER_FLEAVE();
	return MEDIADEMUXER_SRC_INVALID;
}

int _md_util_parse(MMHandleType demuxer, const char *type)
{
	char *media_type_string = NULL;
	int lenght_string = 0;
	int result = MD_ERROR_NONE;
	md_handle_t *new_demuxer = NULL;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(demuxer);
	new_demuxer = (md_handle_t *) demuxer;
	lenght_string = strlen(type);
	media_type_string = (char *)g_malloc(sizeof(char) * (lenght_string + 1));
	MEDIADEMUXER_CHECK_NULL(media_type_string);
	MD_L("media_type_string allocating %p\n", media_type_string);
	strncpy(media_type_string, type, lenght_string + 1);
	/*Set media_type depending upon the header of string else consider using file protocol */
	if (new_demuxer->uri_src) {
		MD_L("new_demuxer->uri_src deallocating %p\n", new_demuxer->uri_src);
		free(new_demuxer->uri_src);
	}
	new_demuxer->uri_src_media_type = __md_util_media_type(&media_type_string);
	if (new_demuxer->uri_src_media_type != MEDIADEMUXER_SRC_INVALID) {
		new_demuxer->uri_src = media_type_string;
		MD_L("uri:%s\n uri_type:%d\n", new_demuxer->uri_src,
		     new_demuxer->uri_src_media_type);
	} else {
		MD_E("Error while setiing source\n");
		MD_E("deallocating media_type_string %p\n", media_type_string);
		free(media_type_string);
		goto ERROR;
	}
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_set_data_source(MMHandleType demuxer, const char *uri)
{
	int result = MD_ERROR_NONE;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(demuxer);
	result = _md_util_parse(demuxer, (const char *)uri);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR_INVALID_ARGUMENT,
									"error while parsing the file");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_prepare(MMHandleType demuxer)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->prepare(md_handle->mdport_handle, md_handle->uri_src);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error while doing prepare");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_get_track_count(MMHandleType demuxer, int *count)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->get_track_count(md_handle->mdport_handle, count);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE,
									result, MD_ERROR,
									"error while getting track count");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_select_track(MMHandleType demuxer, int track)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->set_track(md_handle->mdport_handle, track);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error select track");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_start(MMHandleType demuxer)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->start(md_handle->mdport_handle);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error while doing start");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_get_track_info(MMHandleType demuxer, int track, media_format_h *format)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->get_track_info(md_handle->mdport_handle, format, track);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE,
									result, MD_ERROR,
									"error while getting track count");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_read_sample(MMHandleType demuxer, int track_indx, media_packet_h *outbuf)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->read_sample(md_handle->mdport_handle, outbuf, track_indx);
	if (result == MD_ERROR_NONE) {
		MEDIADEMUXER_FLEAVE();
		return result;
	} else {
		MD_E("error while reading sample\n");
		goto ERROR;
	}
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_seek(MMHandleType demuxer, int64_t pos)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->seek(md_handle->mdport_handle, pos);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error while doing seek");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_unselect_track(MMHandleType demuxer, int track)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->unset_track(md_handle->mdport_handle, track);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error unselect track");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_stop(MMHandleType demuxer)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->stop(md_handle->mdport_handle);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error while doing stop");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_unprepare(MMHandleType demuxer)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_FENTER();
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->unprepare(md_handle->mdport_handle);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error while doing stop");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_destroy(MMHandleType demuxer)
{
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_CHECK_NULL(md_handle);
	MEDIADEMUXER_FENTER();
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->destroy(md_handle->mdport_handle);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
									MD_ERROR, "error while doing destroy");

	/* free mediademuxer structure */
	if (md_handle) {
		if (md_handle->demuxer_ops) {
			MD_I("md_destroy deallocating md_handle->demuxer_ops %p:\n", md_handle->demuxer_ops);
			g_free((void *)(md_handle->demuxer_ops));
		}
		if (md_handle->uri_src) {
			MD_I("md_destroy deallocating md_handle->uri_src %p:\n",
				md_handle->uri_src);
			g_free((void *)(md_handle->uri_src));
		}
		MD_I("md_destroy deallocating md_handle %p:\n", md_handle);
		g_free((void *)md_handle);
		md_handle = NULL;
	}
	MEDIADEMUXER_FLEAVE();
	return result;

ERROR:
	return result;
}

int md_set_error_cb(MMHandleType demuxer,
			mediademuxer_error_cb callback, void *user_data)
{
	MEDIADEMUXER_FENTER();
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->set_error_cb(md_handle->mdport_handle, callback, user_data);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
			MD_ERROR, "error while setting error call back");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}

int md_set_eos_cb(MMHandleType demuxer,
			mediademuxer_eos_cb callback, void *user_data)
{
	MEDIADEMUXER_FENTER();
	int result = MD_ERROR_NONE;
	md_handle_t *md_handle = (md_handle_t *) demuxer;
	MEDIADEMUXER_CHECK_NULL(md_handle);
	media_port_demuxer_ops *pOps = md_handle->demuxer_ops;
	MEDIADEMUXER_CHECK_NULL(pOps);
	result = pOps->set_eos_cb(md_handle->mdport_handle, callback, user_data);
	MEDIADEMUXER_CHECK_SET_AND_PRINT(result, MD_ERROR_NONE, result,
			MD_ERROR, "error while setting eos call back");
	MEDIADEMUXER_FLEAVE();
	return result;
ERROR:
	result = MD_ERROR_INVALID_ARGUMENT;
	MEDIADEMUXER_FLEAVE();
	return result;
}
