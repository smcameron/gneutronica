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
#ifndef __DRUMTAB_H__
#define __DRUMTAB_H__

#ifdef INSTANTIATE_DRUMTAB
#define GLOBAL
#define INIT(x, y) x = y
#else
#define GLOBAL extern
#define INIT(x, y) x
#endif

struct dt_inst_type {
	char *name;
	int midi_value;
};

struct dt_hit_type {
	int numerator;
	int denominator;
	int inst;
	struct dt_hit_type *next;
};

struct dt_pattern_type {
	int staffline;
	int measure;
	struct dt_hit_type *hit;
};

#define MAXLINES 1000
#define MAXPATS 1000

GLOBAL struct dt_inst_type dt_inst[1000];
GLOBAL int INIT(dt_ninsts, 0);
GLOBAL struct dt_pattern_type dt_pat[MAXPATS];
GLOBAL int INIT(dt_npats, 0);

#undef GLOBAL
#endif
