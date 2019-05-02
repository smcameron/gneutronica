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
		rc = read(fd, &ch, 1);
		if (rc == 1) {
			if (ch != 0xfe) {
				printf("0x%02x ", ch);
				fflush(stdout);
			}
			if (ch == 0x90) { /* note on */
				rc = read(fd, &note, 1);
				rc = read(fd, &velocity, 1);
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
		if (rc < 0 && errno != EINTR) {
			printf("rc = %d, errno='%s'\n", rc, strerror(errno));
			break;
		}
	}
	exit(0);
}

