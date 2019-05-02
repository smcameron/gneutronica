/* 
    (C) Copyright 2005,2006 Stephen M. Cameron.

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
#ifndef __MIDI_FILE_H__
#define __MIDI_FILE_H__

#include <sys/time.h>

int write_MThd(int fd);
int write_end_of_track(int fd);
int write_tempo_change(int fd, int microsecs_per_quarternote);
int write_MTrk(int fd);
void write_note(int fd, struct timeval *tm, unsigned char opcode, 
	unsigned char note, unsigned char velocity);

#endif
