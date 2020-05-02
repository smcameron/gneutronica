/*
    (C) Copyright 2006, Stephen M. Cameron.

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
#ifndef __DT_KNOWN_INSTS_H__

static struct inst_mapping {
	char *name;
	int midi_note;
	int alternate;
	int velocity;
} imap[] = {
	{ "h", 42, 46, 80 },
	{ "hh", 42, 46, 80 },
	{ "hihat", 42, 46, 80 },
	{ "hc", 46, 46, 80 },
	{ "hg", 46, 46, 80 },
	{ "hhfoot", 46, 46, 80 },
	{ "hf", 46, 46, 80 },
	{ "c", 49, 52, 120 },
	{ "c1", 49, 49, 120 },
	{ "c2", 57, 57, 120 },
	{ "cy", 49, 49, 120 },
	{ "crash", 49, 52, 120 },
	{ "cc", 49, 52, 120 },
	{ "ft", 41, 41, 110 },
	{ "f", 41, 41, 110 },
	{ "lt", 43, 43, 110 },
	{ "mt", 45, 45, 110 },
	{ "t", 45, 45, 110 },
	{ "tom", 45, 45, 110 },
	{ "tom1", 43, 43, 110 },
	{ "tom2", 45, 45, 110 },
	{ "tom3", 47, 47, 110 },
	{ "t1", 43, 43, 110 },
	{ "t2", 45, 45, 110 },
	{ "t3", 47, 47, 110 },
	{ "1", 43, 43, 110 },
	{ "2", 45, 45, 110 },
	{ "3", 47, 47, 110 },
	{ "ht", 47, 47, 110 },
	{ "ride", 51, 53, 80 },
	{ "r", 51, 53, 80 },
	{ "rc", 51, 53, 80 },
	{ "s", 40, 40, 125 },
	{ "s1", 40, 40, 125 },
	{ "sd", 40, 40, 125 },
	{ "sn", 40, 40, 125 },
	{ "snare", 40, 40, 125 },
	{ "b", 35, 36, 126 },
	{ "bass", 35, 36, 126 },
	{ "bd", 35, 36, 126 },
	{ "cb", 56, 56, 126 },
	{ "splash", 55, 55, 120 },
	{ "a", 55, 55, 120 },
	{ "cl", 39, 39, 120 },
	{ "clap", 39, 39, 120 },
	{ "handclap", 39, 39, 120 },
};

#endif

