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
#ifndef __MIDIOUTPUT_H__
#define __MIDIOUTPUT_H__

struct midi_handle;

typedef void midi_noteon_function(struct midi_handle *mh,
	unsigned char port,
	unsigned char channel,
	unsigned char value,
	unsigned char volume);

typedef void midi_noteoff_function(struct midi_handle *mh,
	unsigned char port,
	unsigned char channel,
	unsigned char value);

typedef struct midi_handle * midi_open_function(unsigned char *name, int nports);

typedef void midi_close_function(struct midi_handle *mh);

typedef void  midi_patch_change_function(struct midi_handle *mh,
	unsigned char port, unsigned char channel, unsigned short bank, unsigned char patch);

typedef int midi_isopen_function(struct midi_handle *mh);

typedef const char *midi_default_file_function();

struct midi_method {
	midi_open_function *open;
	midi_close_function *close;
	midi_noteon_function *noteon;
	midi_noteoff_function *noteoff;
	midi_patch_change_function *patch_change;
	midi_isopen_function *isopen;
	midi_default_file_function *default_file;
};

#endif
