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
#ifndef __MIDI_READER_H__
#define __MIDI_READER_H__

#ifdef __DEFINE_MIDI_READER_GLOBALS
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int midi_reader(int fd, unsigned char *shared_data);

#endif
