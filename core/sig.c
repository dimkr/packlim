#include "../ed25519/ed25519.h"

#include "sig.h"

static const unsigned char g_pub_key[32] = {
#	include "pub_key.h"
};

bool sig_verify(const unsigned char *sig,
                const unsigned char *data,
                size_t len)
{
	if (1 == ed25519_verify(sig, data, len, g_pub_key))
		return true;

	return false;
}
