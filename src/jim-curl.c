/*
 * this file is part of packlim.
 *
 * Copyright (c) 2015 Dima Krasner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <string.h>

#include <jim.h>
#include <curl/curl.h>

#define USER_AGENT "packlim/"VERSION

static int get_file(Jim_Interp *interp,
                    CURL *curl,
                    const char *url,
                    const char *path)
{
	FILE *fh;
	CURLcode code;

	fh = fopen(path, "w");
	if (NULL == fh) {
		Jim_SetResultFormatted(interp, "failed to open %s", path);
		return JIM_ERR;
	}

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_URL, url))
		goto delete_file;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, fh))
		goto delete_file;

	code = curl_easy_perform(curl);
	(void) fclose(fh);

	if (CURLE_OK == code)
		return JIM_OK;

	Jim_SetResultFormatted(interp, "failed to download %s", url);

delete_file:
	(void) unlink(path);

	return JIM_ERR;
}

static int JimCurlHandlerCommand(Jim_Interp *interp,
                                 int argc,
                                 Jim_Obj *const *argv)
{
	static const char *const options[] = { "get", NULL };
	const char *url;
	const char *path;
	CURL *curl = (CURL *) Jim_CmdPrivData(interp);
	int option;
	int len;
	enum { OPT_GET };

	if (4 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "method ?args ...?");
		return JIM_ERR;
	}

	Jim_SetEmptyResult(interp);

	if (JIM_OK != Jim_GetEnum(interp,
	                          argv[1],
	                          options,
	                          &option,
	                          "curl method",
	                          JIM_ERRMSG))
		return JIM_ERR;

	url = Jim_GetString(argv[2], &len);
	if (0 == len) {
		Jim_SetResultString(interp, "the URL cannot be an empty string", -1);
		return JIM_ERR;
	}

	path = Jim_GetString(argv[3], &len);
	if (0 == len) {
		Jim_SetResultString(interp,
		                    "the destination path cannot be an empty string",
		                    -1);
		return JIM_ERR;
	}

	return get_file(interp, curl, url, path);
}

static void JimCurlDelProc(Jim_Interp *interp, void *privData)
{
	curl_easy_cleanup((CURL *) privData);
}

static int iface_up(void)
{
	struct ifaddrs *ifap, *ifa;
	int ret = JIM_ERR;

	if (-1 == getifaddrs(&ifap))
		goto end;

	for (ifa = ifap; NULL != ifa; ifa = ifa->ifa_next) {
		if (NULL == ifa->ifa_addr)
			continue;

		if (0 == strcmp("lo", ifa->ifa_name))
			continue;

		if ((AF_INET == ifa->ifa_addr->sa_family) ||
		    (AF_INET6 == ifa->ifa_addr->sa_family)) {
			ret = JIM_OK;
			break;
		}
	}

	freeifaddrs(ifap);

end:
	return ret;
}

int Jim_CurlCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char buf[32];
	CURL *curl;

	if (1 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "");
		goto end;
	}

	Jim_SetEmptyResult(interp);

	if (JIM_OK != iface_up()) {
		Jim_SetResultString(interp, "network is unreachable", -1);
		goto end;
	}

	curl = curl_easy_init();
	if (NULL == curl)
		goto end;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1))
		goto curl_cleanup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1))
		goto curl_cleanup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY))
		goto curl_cleanup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite))
		goto curl_cleanup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT))
		goto curl_cleanup;

	(void) sprintf(buf, "curl.handle%ld", Jim_GetId(interp));
	Jim_CreateCommand(interp,
	                  buf,
	                  JimCurlHandlerCommand,
	                  curl,
	                  JimCurlDelProc);
	Jim_SetResult(interp,
	              Jim_MakeGlobalNamespaceName(interp,
	                                          Jim_NewStringObj(interp,
	                                                           buf,
	                                                           -1)));

	return JIM_OK;

curl_cleanup:
	curl_easy_cleanup(curl);

end:
	return JIM_ERR;
}
