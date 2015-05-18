#include <stdio.h>
#include <unistd.h>

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

int Jim_CurlCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char buf[32];
	CURL *curl;

	if (1 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "");
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
