#include <stdio.h>
#include <unistd.h>

#include <jim.h>
#include <curl/curl.h>

#define USER_AGENT "packlim/"VERSION

static size_t append(const void *ptr,
                     size_t size,
                     size_t nmemb,
                     void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *) stream);
}

static int fetch_file(CURL *curl, const char *url, const char *path)
{
	FILE *fh;
	CURLcode code;

	fh = fopen(path, "wb");
	if (NULL == fh)
		return JIM_ERR;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_URL, url))
		goto delete_file;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, fh))
		goto delete_file;

	code = curl_easy_perform(curl);
	(void) fclose(fh);

	if (CURLE_OK == code)
		return JIM_OK;

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

	if (2 > argc) {
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

	switch (option) {
		case OPT_GET:
			if (4 != argc)
				return JIM_ERR;

			url = Jim_GetString(argv[2], &len);
			if (0 == len)
				return JIM_ERR;

			path = Jim_GetString(argv[3], &len);
			if (0 == len)
				return JIM_ERR;

			return fetch_file(curl, url, path);
	}

	return JIM_OK;
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

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT))
		goto curl_clenaup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1))
		goto curl_clenaup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1))
		goto curl_clenaup;

	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append))
		goto curl_clenaup;

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

curl_clenaup:
	curl_easy_cleanup(curl);

end:
	return JIM_ERR;
}
