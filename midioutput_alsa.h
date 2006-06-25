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
#ifndef __MIDIOUTPUT_ALSA_H__
#define __MIDIOUTPUT_ALSA_H__

#ifdef INSTANTIATE_MIDIOUTPUT_ALSA_GLOBALS
#define GLOBAL
#define INIT(x, y) x = y
#else
#define GLOBAL extern
#define INIT(x, y) x
#endif

GLOBAL midi_open_function midi_open_alsa;
GLOBAL midi_close_function midi_close_alsa;
GLOBAL midi_noteon_function midi_noteon_alsa;
GLOBAL midi_noteoff_function midi_noteoff_alsa;
GLOBAL midi_patch_change_function midi_patch_change_alsa;
GLOBAL midi_isopen_function midi_isopen_alsa;
GLOBAL midi_default_file_function midi_default_file_alsa;

GLOBAL struct midi_method midi_method_alsa
#ifdef INSTANTIATE_MIDIOUTPUT_ALSA_GLOBALS
= {
	midi_open_alsa,
	midi_close_alsa,
	midi_noteon_alsa,
	midi_noteoff_alsa,
	midi_patch_change_alsa,
	midi_isopen_alsa,
	midi_default_file_alsa,
}
#endif
;

#undef GLOBAL
#undef INIT

#endif
