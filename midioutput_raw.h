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
#ifndef __MIDIOUTPUT_RAW_H__
#define __MIDIOUTPUT_RAW_H__

#ifdef INSTANTIATE_MIDIOUTPUT_RAW_GLOBALS
#define GLOBAL
#define INIT(x, y) x = y
#else
#define GLOBAL extern
#define INIT(x, y) x
#endif

GLOBAL midi_open_function midi_open_raw;
GLOBAL midi_close_function midi_close_raw;
GLOBAL midi_noteon_function noteon_raw;
GLOBAL midi_noteoff_function noteoff_raw;
GLOBAL midi_patch_change_function patch_change_raw;
GLOBAL midi_isopen_function isopen_raw;
GLOBAL midi_default_file_function default_file_raw;

GLOBAL struct midi_method midi_method_raw
#ifdef INSTANTIATE_MIDIOUTPUT_RAW_GLOBALS
= {
	midi_open_raw,
	midi_close_raw,
	noteon_raw,
	noteoff_raw,
	patch_change_raw,
	isopen_raw,
	default_file_raw,
}
#endif
;

#undef GLOBAL
#undef INIT

#endif
