#ifndef _INSTALL_H_INCLUDED
#	define _INSTALL_H_INCLUDED

#	include <stdbool.h>

#	define PKG_ARCHIVE_DIR VAR_DIR"/archive"

bool packlad_install(const char *name,
                      const char *root,
                      const char *url,
                      const char *reason);

#endif