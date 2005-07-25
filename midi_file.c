#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h> /* for htonl(), etc. */
#include <sys/time.h>

#include "sched.h"

static void write_weird_midi_int(int fd, unsigned int value)
{
	unsigned int buf;
	buf = value & 0x7F;
	unsigned char *x;
	int count;

	printf("Writing %d: ", value);

	while ( (value >>= 7) ) {
		buf <<= 8;
		buf |= ((value & 0x7F) | 0x80);
	}
	// buf = htonl(buf); /* put it in big endian order, big bytes first */

	count = 0;
	x = (unsigned char *) &buf;
	while (1) {
		write(fd, x, 1);
		printf(" 0x%02x", *x);
		if (*x & 0x80) {
			x++;
			count++;
			if (count >= 4) /* sanity check to keep from buffer overrun */
				break;
		} else
			break;
	}
	printf("\n");
}

int write_MThd(int fd)
{
	/* write the MThd chunk */

	unsigned int length;
	int rc;
	unsigned short format = htons(0);
	unsigned short tracks = htons(1);
	unsigned short divisions = htons(0xE728); /* millisecond resolution */ 

	length = htonl(6);
	rc = write(fd, "MThd", 4);
	if (rc != 4)
		return -1;
	rc = write(fd, &length, sizeof(length));
	if (rc != sizeof(length))
		return -1;
	rc = write(fd, &format, sizeof(format));
	if (rc != sizeof(format)) return -1;
	rc = write(fd, &tracks, sizeof(tracks));
	if (rc != sizeof(tracks)) return -1;
	rc = write(fd, &divisions, sizeof(divisions));
	if (rc != sizeof(divisions)) return -1;
	return 0;	
}

int write_end_of_track(int fd)
{
	char eot[] = { 0xFF,0x2F,0x00 };
	return (write(fd, eot, 3) != 3);
}

int write_tempo_change(int fd, int microsecs_per_quarternote)
{
	unsigned int buf;
	unsigned char *x;
	unsigned char msg[] = { 0xFF, 0x51, 0x03 };
	int rc;

	microsecs_per_quarternote &= 0x00ffffff;
	buf = htonl(microsecs_per_quarternote);
	x = (unsigned char *) &buf;
	rc = write(fd, msg, 3);
	rc += write(fd, &x[1], 3);
	return (rc != 6);
}

#if 0
int write_MThd(int fd)
{
	unsigned int length = htonl(6);
	unsigned short format, numtracks, divisions;

	write(fd, "MThd", 4);
	write(fd, &length, 4);
}
#endif

int write_MTrk(int fd)
{
	int length = htonl(0);
	write(fd, "MTrk", 4);
	write(fd, &length, 4);
}

static struct timeval prevtime = { 0L, 0L };

write_note(int fd, struct timeval *tm, unsigned char opcode, 
		unsigned char note, unsigned char velocity)
{
	unsigned long ms;
	ms = msdiff(tm, &prevtime);
	printf("ms = %ld\n", ms);
	write_weird_midi_int(fd, ms);
	write(fd, &opcode, 1);
	write(fd, &note, 1);
	write(fd, &velocity, 1); 
	prevtime = *tm;
}


