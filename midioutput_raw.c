/*
    (C) Copyright 2006 Stephen M. Cameron.

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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "write_bytes.h"
#include "midioutput.h"
#define INSTANTIATE_MIDIOUTPUT_RAW_GLOBALS
#include "midioutput_raw.h"

struct midi_handle_raw {
	int fd; /* file descriptor connected to /dev/snd/midiC*D* */
};

void midi_close_raw(struct midi_handle *mh)
{
	struct midi_handle_raw *mhr = (struct midi_handle_raw *) mh;
	close(mhr->fd);
	free(mhr);
}

struct midi_handle *midi_open_raw(unsigned char *name, 
	__attribute__((unused)) int nports)
{
	struct midi_handle_raw *mhr;
	int fd;
	int rc;
	struct stat statbuf;

	mhr = malloc(sizeof(*mhr));
	if (mhr == NULL)
		return NULL;

	fd = open((char *) name, O_RDWR);
	if (fd < 0)
		goto cleanup;

	/* Check to see that it's a device file at least... */
	rc = fstat(fd, &statbuf);
	if (rc < 0) {
		printf("Can't stat MIDI device file %s, %s\n",
			name, strerror(errno));
		goto cleanup;
	}
	if (!S_ISCHR(statbuf.st_mode)) {
		printf("%s is not a character special device file\n",
			name);
		goto cleanup;
	}
	mhr->fd = fd;
	return (struct midi_handle *) mhr;

cleanup:
	if (fd >= 0)
		close(fd);
	free (mhr);
	return NULL;
}

void noteon_raw(struct midi_handle *mh,
	unsigned char __attribute__((unused)) port, /* not used for raw midi */
	unsigned char channel,
	unsigned char value,
	unsigned char volume)
{
	struct midi_handle_raw *mhr = (struct midi_handle_raw *) mh;

	unsigned char data[3];
	printf("NOTE ON, fd=%d, value=%d, volume=%d\n", mhr->fd, value, volume);
	data[0] = 0x90 | (channel & 0x0f);
	data[1] = value;
	data[2] = volume;
	write_bytes(mhr->fd, data, 3); /* This needs to be atomic */
}

void noteoff_raw(struct midi_handle *mh, 
	unsigned char __attribute__((unused)) port, /* not used for raw midi */
	unsigned char channel, unsigned char value)
{
	struct midi_handle_raw *mhr = (struct midi_handle_raw *) mh;
	unsigned char data[3];

	/* printf("NOTE OFF, value=%d, volume=%d\n", value, volume); */
	/* data[0] = 0x80 | (channel & 0x0f); */
	data[0] = 0x90 | (channel & 0x0f);
	data[1] = value;
	data[2] = 0;
	write_bytes(mhr->fd, data, 3); /* This needs to be atomic */
}

void patch_change_raw(struct midi_handle *mh,
	unsigned char __attribute__((unused)) port, /* not used for raw midi */
	unsigned char channel,
	unsigned short bank,
	unsigned char patch)
{
	/* sends a MIDI bank change and patch change message to a MIDI device */
	struct midi_handle_raw *mhr = (struct midi_handle_raw *) mh;
	unsigned short b;
	char bank_ch[3];
	char patch_ch[2];

	printf("Changing MIDI device to bank %d, patch %d\n", bank, patch);

	bank_ch[0] = 0xb0 | (channel & 0x0f);
	b = htons(bank); /* put it in big endian order */
	memcpy(&bank_ch[1], &b, sizeof(b));

	patch_ch[0] = 0xc0 | (channel & 0x0f);
	patch_ch[1] = patch;

	write_bytes(mhr->fd, bank_ch, 3);
	write_bytes(mhr->fd, patch_ch, 2);

	return;
}

int isopen_raw(struct midi_handle *mh)
{
	struct midi_handle_raw *mhr = (struct midi_handle_raw *) mh;

	if (mhr == NULL)
		return 0;
	if (mhr->fd < 0)
		return 0;
	return 1;
}

const char *default_file_raw(void)
{
	static const char *filename = "/dev/snd/midiC0D0";
	return(filename);
}
