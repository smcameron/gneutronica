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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "midioutput.h"
#define INSTANTIATE_MIDIOUTPUT_ALSA_GLOBALS
#include "midioutput_alsa.h"

struct midi_handle_alsa {
	int somethingorother;
};

void midi_close_alsa(struct midi_handle *mh)
{
	printf("%s:%s:%s, not yet implemented\n",
		 __FILE__, __LINE__, __FUNCTION__);
}

struct midi_handle *midi_open_alsa(unsigned char *name)
{
	printf("%s:%d, not yet implemented\n", __FILE__, __LINE__);
	return NULL;
}

void midi_noteon_alsa(struct midi_handle *mh,
	unsigned char channel,
	unsigned char value,
	unsigned char volume)
{
	printf("%s:%d, not yet implemented\n", __FILE__, __LINE__);
	return;
}

void midi_patch_change_alsa(struct midi_handle *mh,
	unsigned char channel,
	unsigned short bank,
	unsigned char patch)
{
	printf("%s:%d, not yet implemented\n", __FILE__, __LINE__);
	return;
}

void midi_noteoff_alsa(struct midi_handle *mh, unsigned char channel, unsigned char value)
{
	printf("%s:%d, not yet implemented\n", __FILE__, __LINE__);
	return;
}

int midi_isopen_alsa(struct midi_handle *mh)
{
	struct midi_handle_alsa *mha = (struct midi_handle_alsa *) mh;
	if (mha == NULL)
		return 0;
	return 1;
}
