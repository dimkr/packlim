#include <stdio.h>
#include <unistd.h>

#include "repository.h"

static size_t append(const void *ptr,
                     size_t size,
                     size_t nmemb,
                     void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *) stream);
}

static int fetch_file(struct repo *repo,
                      const char *name,
                      const char *path)
{
	char url[1024];
	FILE *fh;
	CURLcode code;
	int len;

	len = snprintf(url, sizeof(url), "%s/%s", repo->url, name);
	if ((sizeof(url) <= len) || (0 >= len))
		return JIM_ERR;

	fh = fopen(path, "wb");
	if (NULL == fh)
		return JIM_ERR;

	if (CURLE_OK != curl_easy_setopt(repo->curl, CURLOPT_URL, url))
		goto delete_file;

	if (CURLE_OK != curl_easy_setopt(repo->curl, CURLOPT_WRITEDATA, fh))
		goto delete_file;

	code = curl_easy_perform(repo->curl);
	(void) fclose(fh);

	if (CURLE_OK == code)
		return JIM_OK;

delete_file:
	(void) unlink(path);

	return JIM_ERR;
}

static int JimRepositoryHandlerCommand(Jim_Interp *interp,
                                       int argc,
                                       Jim_Obj *const *argv)
{
	static const char *const options[] = { "fetch", NULL };
	const char *name;
	const char *path;
	struct repo *repo = (struct repo *) Jim_CmdPrivData(interp);
	int option;
	int len;
	enum { OPT_FETCH };

	if (2 > argc) {
		Jim_WrongNumArgs(interp, 1, argv, "method ?args ...?");
		return JIM_ERR;
	}

	if (JIM_OK != Jim_GetEnum(interp,
	                          argv[1],
	                          options,
	                          &option,
	                          "repo method",
	                          JIM_ERRMSG))
		return JIM_ERR;

	switch (option) {
		case OPT_FETCH:
			if (4 != argc)
				return JIM_ERR;

			name = Jim_GetString(argv[2], &len);
			if (0 == len)
				return JIM_ERR;

			path = Jim_GetString(argv[3], &len);
			if (0 == len)
				return JIM_ERR;

			return fetch_file(repo, name,  path);
	}

	return JIM_OK;
}

static void JimRepositoryDelProc(Jim_Interp *interp, void *privData)
{
	struct repo *repo = (struct repo *) privData;

	curl_easy_cleanup(repo->curl);
	Jim_Free(repo);
}

int Jim_PacklimRepositoryCmd(Jim_Interp *interp,
                             int argc,
                             Jim_Obj *const *argv)
{
	struct repo *repo;
	int len;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "url");
		return JIM_ERR;
	}

	repo = Jim_Alloc(sizeof(*repo));
	if (NULL == repo)
		return JIM_ERR;

	repo->url = (char *) Jim_GetString(argv[1], &len);
	if (0 == len)
		goto free_repo;

	repo->url = Jim_StrDup(repo->url);
	if (NULL == repo->url)
		goto free_repo;

	repo->curl = curl_easy_init();
	if (NULL == repo->curl)
		goto free_url;

	if (CURLE_OK != curl_easy_setopt(repo->curl, CURLOPT_USERAGENT, USER_AGENT))
		goto curl_clenaup;

	if (CURLE_OK != curl_easy_setopt(repo->curl, CURLOPT_TCP_NODELAY, 1))
		goto curl_clenaup;

	if (CURLE_OK != curl_easy_setopt(repo->curl, CURLOPT_FAILONERROR, 1))
		goto curl_clenaup;

	if (CURLE_OK != curl_easy_setopt(repo->curl, CURLOPT_WRITEFUNCTION, append))
		goto curl_clenaup;

	Jim_CreateCommand(interp,
	                  repo->url,
	                  JimRepositoryHandlerCommand,
	                  repo,
	                  JimRepositoryDelProc);
	Jim_SetResult(interp, Jim_MakeGlobalNamespaceName(interp, argv[1]));

	return JIM_OK;

curl_clenaup:
	curl_easy_cleanup(repo->curl);

free_url:
	Jim_Free(repo->url);

free_repo:
	Jim_Free(repo);

	return JIM_ERR;
}
