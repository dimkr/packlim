#ifndef _SIG_H_INCLUDED
#	define _SIG_H_INCLUDED

#	include <stdbool.h>
#	include <sys/types.h>

bool sig_verify(const unsigned char *sig,
                const unsigned char *data,
                size_t len);

#endif