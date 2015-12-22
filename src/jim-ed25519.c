/*
 * this file is part of packlim.
 *
 * Copyright (c) 2015 Dima Krasner
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
