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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define INSTANTIATE_DRUMTAB 1
#include "drumtab.h"

#include "fractions.h"

static struct inst_mapping {
	char *name;
	int midi_note;
	int alternate;
} imap[] = {
	{ "hh", 42, 46 },
	{ "c", 49, 52 },
	{ "ft", 41, 41 },
	{ "lt", 43, 43 },
	{ "mt", 45, 45 },
	{ "ht", 47, 47 },
	{ "r", 51, 53 },
	{ "s", 38, 40 },
	{ "b", 35, 36 },
};

static int used[256];

static void init_used()
{
	memset(used, 0, 256);
}

static nimappings = (sizeof(imap) / sizeof(imap[0]));


static int lookup_instrument(char *name)
{
	char newname[100];

	int i;
	memset(newname, 0, 100);
	strncpy(newname, name, 99);
	for (i=0;newname[i] != '\0';i++)
		newname[i] = tolower(newname[i]);
	for (i=0;i<nimappings;i++)
		if (strcmp(newname, imap[i].name) == 0) {
			used[imap[i].midi_note]= 1;
			return imap[i].midi_note;
		}

	for (i=127;i>0;i--)
		if (!used[i]) {
			used[i] = 1;
			return i;
		}
	return 127;
}

static int find_instrument(char *line)
{
	int i, j;
	char n[100];
	int found = 0;

	j = 0; n[j] = '\0';

	/* printf("line = %s\n", line); */
	for (i=0;i<strlen(line);i++) {
		if (line[i] == '|')
			break;
		if (isalpha(line[i])) {
			n[j] = line[i];
			j++;
			n[j] = '\0';
		}
	}
	/* printf("n = %s\n", n); */

	for (i=0;i<dt_ninsts;i++) {
		if (strcmp(n, dt_inst[i].name) == 0) {
			found = i;
			break;
		}
	}
	found = i;
	if (found >= dt_ninsts) {
		dt_inst[dt_ninsts].name = malloc(strlen(n)+1);
		if (dt_inst[dt_ninsts].name == NULL) {
			fprintf(stderr, "Out of memory %s:%d\n",
				__FILE__, __LINE__);
			exit(1);
		}
		strcpy(dt_inst[dt_ninsts].name, n);
		dt_inst[dt_ninsts].midi_value = lookup_instrument(n);
		dt_ninsts++;
	}
	return found;
}


static int read_tab_file(const char *filename, char *buffer[],
	int maxlinecount, int *linecount)
{
	FILE *f;
	char *s;

	char buf[1000];
	int i = 0, len, rc;

	rc = -1;
	f = fopen(filename, "r");
	if (f == NULL) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		return -1;
	}

	do {
		s = fgets(buf, 1000, f);
		if (s == NULL) {
			rc = 0;
			break;
		}
		len = strlen(s);
		buffer[i] = malloc((len + 1) *sizeof(char));
		if (buffer[i] == NULL)
			break;
		strcpy(buffer[i], s);
		i++;
	} while (1);

	fclose(f);
	*linecount = i;
	return rc;
}

static int add_hit(int instrument, int staff, int measure, int numer, int denom)
{
	struct dt_hit_type *p;

	p = dt_pat[dt_npats].hit;
	dt_pat[dt_npats].hit = malloc(sizeof(*dt_pat[dt_npats].hit));
	dt_pat[dt_npats].hit->next = p;
	p = dt_pat[dt_npats].hit;
	p->numerator = numer;
	p->denominator = denom;
	reduce_fraction(&p->numerator, &p->denominator);
	p->inst = instrument;
	dt_pat[dt_npats].staffline = staff;
	dt_pat[dt_npats].measure = measure;
	return 0;
}

static int process_line(char *line, int instrument,
	int current_measure, int current_staff,
	int *last_measure_of_staff)
{
	char *chunk;
	int denom, numer, i;

	for (chunk = strtok(line, "|"); chunk ; chunk = strtok(NULL, "|")) {
		denom = strlen(chunk);
		for (i=0;i<denom;i++) {
			if (chunk[i] == 'x' ||
				chunk[i] == 'o') {
				numer = i;
				add_hit(instrument, current_staff,
					current_measure, numer, denom);
			}
		}
		current_measure++;
		dt_npats++;
	}
	*last_measure_of_staff = current_measure - 1;
	return 0;
}

static int process_tab(char *buffer[], int nlines, int *nmeasures)
{
	int i;
	int current_measure = 0;
	int last_measure = -1;
	int current_instrument = -1;
	int new_staff_iminent = 1;
	int current_staff = -1;
	int last_measure_of_staff = -1;
	int this_measure;
	int in = -1;

	char *vbar;

	for (i=0;i<nlines;i++) {
		/* look for a a vertical bar */
		vbar = strstr(buffer[i], "|");
		if (vbar == NULL) {
			new_staff_iminent = 1;
			continue;
		}
		if (new_staff_iminent) {
			printf("%d: new staff begins\n", i);
			new_staff_iminent = 0;
			current_staff++;
			current_measure = last_measure_of_staff;
		}
		in = find_instrument(buffer[i]);
		process_line(vbar+1, in, current_measure,
			current_staff, &last_measure_of_staff);
	}

	*nmeasures = last_measure_of_staff;

	for (i=0;i<dt_ninsts;i++)
		printf("inst %d = %s\n", i, dt_inst[i].name);
}

static int print_pattern(struct dt_pattern_type *p)
{
	struct dt_hit_type *h;

	for (h=p->hit; h ; h = h->next) {
		printf("%s:%d/%d ", dt_inst[h->inst].name, h->numerator, h->denominator);
	}
	printf("\n");
}

static int print_data(int nmeasures)
{
	int i, j;

	for (i=0;i<nmeasures;i++) {
		printf("Measure %d\n", i);
		for (j=0;j<dt_npats;j++)
			if (dt_pat[j].measure == i)
				print_pattern(&dt_pat[j]);
	}
	return 0;
}

int process_drumtab_file(const char *filename)
{
	char *buf[MAXLINES];
	int nlines, rc;

	memset(dt_pat, 0, sizeof(dt_pat));

	rc = read_tab_file(filename, buf, MAXLINES, &nlines);
	printf("rc = %d, nlines = %d\n", rc, nlines);
	process_tab(buf, nlines, &dt_nmeasures);
	print_data(dt_nmeasures);
}
