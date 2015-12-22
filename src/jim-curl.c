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
#include <errno.h>

#include <jim.h>
#include <jim-subcmd.h>
#include <curl/curl.h>

#define USER_AGENT "packlim/"VERSION
#define CONNECT_TIMEOUT 30
#define TIMEOUT 180

static int JimCurlNetworkReachable(void)
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

static int curl_cmd_get(Jim_Interp *interp,
                        int argc,
                        Jim_Obj *const *argv)
{
	CURLM *cm;
	CURL **cs;
	FILE **fhs;
	const char **urls, **paths;
	const struct CURLMsg *info;
	int n, i, len, j, act, nfds, q, ret = JIM_ERR;
	CURLMcode m;

	if (argc % 2 == 1) {
		Jim_WrongNumArgs(interp, 0, argv, "progress url path ...");
		return JIM_ERR;
	}

	if (JIM_OK != JimCurlNetworkReachable()) {
		Jim_SetResultString(interp, "network is unreachable", -1);
		return JIM_ERR;
	}

	n = argc / 2;
	cs = Jim_Alloc(n * sizeof(CURL *));
	fhs = Jim_Alloc(n * sizeof(FILE *));
	urls = Jim_Alloc(n * sizeof(char *));
	paths = Jim_Alloc(n * sizeof(char *));

	cm = curl_multi_init();
	if (NULL == cm) {
		goto free_arrs;
	}

	for (i = 0; argc > i; i += 2) {
		j = i / 2;

		urls[j] = Jim_GetString(argv[i], &len);
		if (0 == len) {
			Jim_SetResultString(interp, "a URL cannot be an empty string", -1);
			goto cleanup_cm;
		}

		paths[j] = Jim_GetString(argv[1 + i], &len);
		if (0 == len) {
			Jim_SetResultString(interp,
			                    "a destination path cannot be an empty string",
			                    -1);
			goto cleanup_cm;
		}
	}

	for (i = 0; n > i; ++i) {
		fhs[i] = fopen(paths[i], "w");
		if (NULL == fhs[i]) {
			for (--i; 0 <= i; --i) {
				fclose(fhs[i]);
				unlink(paths[i]);
			}

			Jim_SetResultFormatted(interp, "failed to open %s: %s", paths[i], strerror(errno));
			goto cleanup_cm;
		}
	}

	for (i = 0; n > i; ++i) {
		cs[i] = curl_easy_init();
		if (NULL == cs[i]) {
			for (--i; 0 <= i; --i) {
				curl_easy_cleanup(cs[i]);
			}

			goto close_fhs;
		}
	}

	for (i = 0; n > i; ++i) {
		if ((CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_FAILONERROR, 1)) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_TCP_NODELAY, 1)) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_USE_SSL, CURLUSESSL_TRY)) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_WRITEFUNCTION, fwrite)) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_WRITEDATA, fhs[i])) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_USERAGENT, USER_AGENT)) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_URL, urls[i])) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT)) ||
		    (CURLE_OK != curl_easy_setopt(cs[i], CURLOPT_TIMEOUT, TIMEOUT))) {
			goto cleanup_cs;
		}
	}

	for (i = 0; n > i; ++i) {
		if (CURLM_OK != curl_multi_add_handle(cm, cs[i])) {
			for (--i; 0 <= i; --i) {
				curl_multi_remove_handle(cm, cs[j]);
			}

			goto cleanup_cs;
		}
	}

	do {
		m = curl_multi_perform(cm, &act);
		if (CURLM_OK != m) {
			Jim_SetResultString(interp, curl_multi_strerror(m), -1);
			break;
		}
		if (0 == act) {
			info = curl_multi_info_read(cm, &q);
			if ((NULL != info) && (CURLMSG_DONE == info->msg)) {
				if (CURLE_OK == info->data.result) {
					ret = JIM_OK;
				}
				else {
					Jim_SetResultString(interp, curl_easy_strerror(info->data.result), -1);
				}
			}
			break;
		}

		m = curl_multi_wait(cm, NULL, 0, 1000, &nfds);
		if (CURLM_OK != m) {
			Jim_SetResultString(interp, curl_multi_strerror(m), -1);
			break;
		}
		if (0 == nfds) {
			usleep(100000);
		}
	} while (1);

	for (i = 0; n > i; ++i) {
		curl_multi_remove_handle(cm, cs[i]);
	}

cleanup_cs:
	for (i = 0; n > i; ++i) {
		curl_easy_cleanup(cs[i]);
	}

close_fhs:
	for (i = 0; n > i; ++i) {
		fclose(fhs[i]);
	}

	if (JIM_OK != ret) {
		for (i = 0; n > i; ++i) {
			unlink(paths[i]);
		}
	}

cleanup_cm:
	curl_multi_cleanup(cm);

free_arrs:
	Jim_Free(paths);
	Jim_Free(urls);
	Jim_Free(fhs);
	Jim_Free(cs);

	return ret;
}

static const jim_subcmd_type curl_command_table[] = {
	{
		"get",
		"url path ...",
		curl_cmd_get,
		2,
		-1
	},
	{ NULL }
};

int Jim_CurlCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    return Jim_CallSubCmd(interp, Jim_ParseSubCmd(interp, curl_command_table, argc, argv), argc, argv);
}
