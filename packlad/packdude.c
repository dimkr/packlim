#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#include <libpkg/pkgent.h>

#include "install.h"
#include "remove.h"
#include "update.h"
#include "list.h"

#define USAGE "Usage: %s [-u] [-n] [-p PREFIX] [-u URL] -l|-q|-c|-f|-i|-r " \
              "PACKAGE\n"
#define REPO_ENVIRONMENT_VARIABLE "REPO"

enum actions {
	ACTION_INSTALL        = 0,
	ACTION_REMOVE         = 1,
	ACTION_LIST_INSTALLED = 2,
	ACTION_LIST_AVAILABLE = 3,
	ACTION_LIST_REMOVABLE = 4,
	ACTION_LIST_FILES     = 5,
	ACTION_UPDATE         = 6,
	ACTION_INVALID        = 7
};

int main(int argc, char *argv[]) {
	struct passwd *user;
	const char *root = "/";
	char *reason = INST_REASON_USER;
	const char *package = NULL;
	const char *url = NULL;
	int option;
	int action = ACTION_INVALID;

	/* parse the command-line */
	do {
		option = getopt(argc, argv, "unlqcf:u:i:r:p:");
		switch (option) {
			case 'p':
				root = optarg;
				break;

			case 'u':
				action = ACTION_UPDATE;
				break;

			case 'r':
				action = ACTION_REMOVE;
				package = optarg;
				break;

			case 'i':
				action = ACTION_INSTALL;
				package = optarg;
				break;

			case 'n':
				reason = INST_REASON_CORE;
				break;

			case 'q':
				action = ACTION_LIST_INSTALLED;
				break;

			case 'l':
				action = ACTION_LIST_AVAILABLE;
				break;

			case 'c':
				action = ACTION_LIST_REMOVABLE;
				break;

			case 'f':
				action = ACTION_LIST_FILES;
				package = optarg;
				break;

			case (-1):
				switch (action) {
					case ACTION_REMOVE:
					case ACTION_LIST_FILES:
						if (NULL == package)
							goto help;
						break;

					case ACTION_INSTALL:
						if (NULL == package)
							goto help;

						/* fall-through */

					case ACTION_UPDATE:
					case ACTION_LIST_AVAILABLE:
						url = getenv(REPO_ENVIRONMENT_VARIABLE);
						if (NULL == url)
							goto help;
						break;

					case ACTION_INVALID:
						goto help;
				}

				if (0 != geteuid()) {
					user = getpwuid(0);
					if (NULL != user)
						(void) fprintf(stderr,
						               "%s: must run as %s.\n",
						               argv[0],
						               user->pw_name);
					return EXIT_FAILURE;
				}

				goto done;

			default:
				goto help;
		}
	} while (1);

done:
	switch (action) {
		case ACTION_INSTALL:
			if (false == packlad_install(package, root, url, reason))
				return EXIT_FAILURE;
			break;

		case ACTION_REMOVE:
			if (false == packlad_remove(package, root))
				return EXIT_FAILURE;
			break;

		case ACTION_LIST_INSTALLED:
			if (false == packlad_list_inst(root))
				return EXIT_FAILURE;
			break;

		case ACTION_LIST_AVAILABLE:
			if (false == packlad_list_avail(root))
				return EXIT_FAILURE;
			break;

		case ACTION_LIST_REMOVABLE:
			if (false == packlad_list_removable(root))
				return EXIT_FAILURE;
			break;

		case ACTION_LIST_FILES:
			if (false == packlad_list_files(root, package))
				return EXIT_FAILURE;
			break;

		case ACTION_UPDATE:
			if (false == packlad_update(root, url))
				return EXIT_FAILURE;
			break;
	}

	return EXIT_SUCCESS;

help:
	(void) fprintf(stderr, USAGE, argv[0]);
	return EXIT_FAILURE;
}