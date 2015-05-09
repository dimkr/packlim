#include <arpa/inet.h>

#include <jim.h>
#include <archive.h>

#define PKG_MAGIC htonl((uint32_t) 0x686a6b6c)

#define EXTRACTION_OPTIONS (ARCHIVE_EXTRACT_OWNER | \
                            ARCHIVE_EXTRACT_PERM | \
                            ARCHIVE_EXTRACT_TIME | \
                            ARCHIVE_EXTRACT_UNLINK | \
                            ARCHIVE_EXTRACT_ACL | \
                            ARCHIVE_EXTRACT_FFLAGS | \
                            ARCHIVE_EXTRACT_XATTR)

struct pkg_hdr {
	uint32_t magic;
	unsigned char sig[64];
} __attribute__((packed));

struct pkg {
	int fd;
	size_t len;
	size_t tar_len;
	void *data;
	const struct pkg_hdr *hdr;
};

int Jim_PacklimPackageCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
