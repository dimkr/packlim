#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <ed25519/ed25519.h>

static int write_key_h(const char *path,
                       const unsigned char *key,
                       const size_t len)
{
	FILE *fh;
	size_t i;
	int ret = 1;

	fh = fopen(path, "w");
	if (NULL == fh)
		goto end;

	for (i = 0; len - 1 > i; ++i) {
		if (0 > fprintf(fh, "0x%02x, ", key[i]))
			goto close_key;
	}
	if (0 > fprintf(fh, "0x%02x", key[i]))
		goto close_key;

	ret = 0;

close_key:
	(void) fclose(fh);

end:
	return ret;
}

static int write_key_raw(const char *path,
                         const unsigned char *key,
                         const size_t len) {
	int fd;
	int ret = 1;

	fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (-1 == fd)
		goto end;

	if ((ssize_t) len == write(fd, (void *) key, len))
		ret = 0;

	(void) close(fd);

end:
	return ret;
}

int main(int argc, char *argv[])
{
	unsigned char seed[32];
	unsigned char pub_key[32];
	unsigned char priv_key[64];

	if (5 != argc)
		goto error;

	if (0 != ed25519_create_seed(seed))
		goto error;

	ed25519_create_keypair(pub_key, priv_key, seed);

	if (0 != write_key_raw(argv[1], pub_key, sizeof(pub_key)))
		goto error;
	if (0 != write_key_h(argv[2], pub_key, sizeof(pub_key)))
		goto error;

	if (0 != write_key_raw(argv[3], priv_key, sizeof(priv_key)))
		goto error;
	if (0 != write_key_h(argv[4], priv_key, sizeof(priv_key)))
		goto error;

	return EXIT_SUCCESS;

error:
	return EXIT_FAILURE;
}