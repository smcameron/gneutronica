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
#ifndef __OLD_FILEFORMATS_H__
#define __OLD_FILEFORMATS_H__

extern int load_from_file_version_1(FILE *f);
extern int load_from_file_version_2(FILE *f);
extern int load_from_file_version_3(FILE *f);
extern int import_patterns_v2(FILE *f);
extern int import_patterns_v3(FILE *f);

#endif
