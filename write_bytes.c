#include <unistd.h>
#include <errno.h>

int write_bytes(int fd, void *buf, int bufsize)
{
	int rc;
	int bytesleft = bufsize;
	int offset = 0;
	char *buffer = buf;
	do {
		rc = write(fd, buffer + offset, bytesleft);
		if (rc < 0 && errno == EINTR)
			continue;
		if (rc < 0)
			return rc;
		bytesleft -= rc;
		offset += rc;
	} while (bytesleft > 0);
	return bufsize;
}

