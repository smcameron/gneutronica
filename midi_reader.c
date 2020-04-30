/*
    (C) Copyright 2005,2006, Stephen M. Cameron.

    This file is part of Gneutronica.

    Gneutronica is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Gneutronica is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gneutronica; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

static int read_bytes(int fd, unsigned char *c, int count)
{
	int rc;
	int bytes_left = count;

	do {
		rc = read(fd, &c[count - bytes_left], bytes_left);
		if (rc < 0 && errno == EINTR)
			continue;
		if (rc < 0)
			break;
		bytes_left -= rc;
	} while (bytes_left > 0);
	return rc;
}

int midi_reader(int fd, unsigned char *shared_data)
{
	int rc;
	unsigned char ch;
	int ppid;
	unsigned char note, velocity;



	ppid = getpid();
	rc = fork();

	if (rc != 0)
		return rc;

	printf("Midi reader process running.\n");
	while (1) {
		rc = read_bytes(fd, &ch, 1);
		if (rc < 0)
			goto read_error;
		if (ch != 0xfe) {
			printf("0x%02x ", ch);
			fflush(stdout);
		}
		if (ch == 0x90) { /* note on */
			rc = read_bytes(fd, &note, 1);
			if (rc < 0)
				goto read_error;
			rc = read_bytes(fd, &velocity, 1);
			if (rc < 0)
				goto read_error;
			if (velocity != 0) { /* we don't send note-offs, correct?  meh. */
				/* get lock */
				shared_data[0] = ch;
				shared_data[1] = note;
				shared_data[2] = velocity;
				/* release lock */
				kill(ppid, SIGUSR1);
			}
		}
	}

read_error:
	if (rc)
		printf("rc = %d, errno='%s'\n", rc, strerror(errno));
	exit(0);
}

