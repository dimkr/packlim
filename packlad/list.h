#ifndef _LIST_H_INCLUDED
#	define _LIST_H_INCLUDED

#	include <stdbool.h>

bool packlad_list_avail(const char *root);
bool packlad_list_inst(const char *root);
bool packlad_list_removable(const char *root);

bool packlad_list_files(const char *name, const char *root);

#endif
