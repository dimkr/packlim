#include <jim.h>
#include <curl/curl.h>

#	define USER_AGENT "packlim/"VERSION

#	define REPO_ENV_VAR "REPO"

struct repo {
	CURL *curl;
	char *url;
};

int Jim_PacklimRepositoryCmd(Jim_Interp *interp,
                             int argc,
                             Jim_Obj *const *argv);
