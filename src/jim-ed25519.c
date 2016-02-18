/*
 * this file is part of packlim.
 *
 * Copyright (c) 2015, 2016 Dima Krasner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <jim.h>
#include <ed25519.h>

int Jim_Ed25519VerifyCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const unsigned char *sig, *data, *key;
	int len;

	if (argc != 4) {
		Jim_WrongNumArgs(interp, 0, argv, "data sig key");
		return JIM_ERR;
	}

	key = (const unsigned char *)Jim_GetString(argv[3], &len);
	if (len != 32) {
		Jim_SetResultString(interp, "the public key must be 32 bytes long", -1);
		return JIM_ERR;
	}

	sig = (const unsigned char *)Jim_GetString(argv[2], &len);
	if (len != 64) {
		Jim_SetResultString(interp, "the signature must be 64 bytes long", -1);
		return JIM_ERR;
	}

	data = (const unsigned char *)Jim_GetString(argv[1], &len);
	if (!len) {
		Jim_SetResultString(interp, "the data cannot be an empty buffer", -1);
		return JIM_ERR;
	}

	if (!ed25519_verify(sig, data, (size_t)len, key)) {
		Jim_SetResultString(interp, "the digital signature is invalid", -1);
		return JIM_ERR;
	}

	return JIM_OK;
}

int Jim_Ed25519SignCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	unsigned char sig[64];
	const unsigned char *data, *pub, *priv;
	int len, klen;

	if (argc != 4) {
		Jim_WrongNumArgs(interp, 0, argv, "data priv pub");
		return JIM_ERR;
	}

	data = (const unsigned char *)Jim_GetString(argv[1], &len);
	if (!len) {
		Jim_SetResultString(interp, "the data cannot be an empty buffer", -1);
		return JIM_ERR;
	}

	priv = (const unsigned char *)Jim_GetString(argv[2], &klen);
	if (klen != 64) {
		Jim_SetResultString(interp, "the private key must be 64 bytes long", -1);
		return JIM_ERR;
	}

	pub = (const unsigned char *)Jim_GetString(argv[3], &klen);
	if (klen != 32) {
		Jim_SetResultString(interp, "the public key must be 32 bytes long", -1);
		return JIM_ERR;
	}

	ed25519_sign(sig, data, len, pub, priv);

	Jim_SetResultString(interp, (const char *)sig, 64);
	return JIM_OK;
}

int Jim_Ed25519KeypairCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	unsigned char priv[64], pub[32], seed[32];
	Jim_Obj *keys[2];

	if (argc != 1) {
		Jim_WrongNumArgs(interp, 0, argv, NULL);
		return JIM_ERR;
	}

	if (ed25519_create_seed(seed) != 0) {
		return EXIT_FAILURE;
	}

	ed25519_create_keypair(pub, priv, seed);

	keys[0] = Jim_NewStringObj(interp, (const char *)priv, sizeof(priv));
	keys[1] = Jim_NewStringObj(interp, (const char *)pub, sizeof(pub));
	Jim_SetResult(interp, Jim_NewListObj(interp, keys, 2));

	return JIM_OK;
}
