#include <jim.h>
#include <ed25519.h>

int Jim_Ed25519VerifyCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const unsigned char *sig, *data, *key;
	int len;

	if (4 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "data sig key");
		return JIM_ERR;
	}

	key = (const unsigned char *) Jim_GetString(argv[3], &len);
	if (32 != len) {
		Jim_SetResultString(interp, "the public key must be 32 bytes long", -1);
		return JIM_ERR;
	}

	sig = (const unsigned char *) Jim_GetString(argv[2], &len);
	if (64 != len) {
		Jim_SetResultString(interp, "the signature must be 64 bytes long", -1);
		return JIM_ERR;
	}

	data = (const unsigned char *) Jim_GetString(argv[1], &len);
	if (0 == len) {
		Jim_SetResultString(interp, "the data cannot be an empty buffer", -1);
		return JIM_ERR;
	}

	if (0 == ed25519_verify(sig, data, (size_t) len, key)) {
		Jim_SetResultString(interp, "the digital signature is invalid", -1);
		return JIM_ERR;
	}

	Jim_SetEmptyResult(interp);

	return JIM_OK;
}
