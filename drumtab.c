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

#include "fractions.h"

#define MAXLINES 1000
#define MAXPATS 1000

struct dt_inst_type {
	char *name;
	int midi_value;
} inst[1000];
int ninsts = 0;

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
} pat[MAXPATS];
int npats = 0;

int find_instrument(char *line)
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

	for (i=0;i<ninsts;i++) {
		if (strcmp(n, inst[i].name) == 0) {
			found = i;
			break;
		}
	}
	found = i;
	if (found >= ninsts) {
		inst[ninsts].name = malloc(strlen(n)+1);
		if (inst[ninsts].name == NULL) {
			fprintf(stderr, "Out of memory %s:%d\n",
				__FILE__, __LINE__);
			exit(1);
		}
		strcpy(inst[ninsts].name, n);
		inst[ninsts].midi_value = found;
		ninsts++;
	}
	return found;
}


int read_tab_file(char *filename, char *buffer[],
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

int add_hit(int instrument, int staff, int measure, int numer, int denom)
{
	struct dt_hit_type *p;

	p = pat[npats].hit;
	pat[npats].hit = malloc(sizeof(*pat[npats].hit));
	pat[npats].hit->next = p;
	p = pat[npats].hit;
	p->numerator = numer;
	p->denominator = denom;
	reduce_fraction(&p->numerator, &p->denominator);
	p->inst = instrument;
	pat[npats].staffline = staff;
	pat[npats].measure = measure;
	return 0;
}

int process_line(char *line, int instrument,
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
		npats++;
	}
	*last_measure_of_staff = current_measure - 1;
	return 0;
}

int process_tab(char *buffer[], int nlines, int *nmeasures)
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
			current_measure = last_measure_of_staff + 1;
		}
		in = find_instrument(buffer[i]);
		process_line(vbar+1, in, current_measure,
			current_staff, &last_measure_of_staff);
	}

	*nmeasures = last_measure_of_staff;

	for (i=0;i<ninsts;i++)
		printf("inst %d = %s\n", i, inst[i].name);
}

int print_pattern(struct dt_pattern_type *p)
{
	struct dt_hit_type *h;

	for (h=p->hit; h ; h = h->next) {
		printf("%s:%d/%d ", inst[h->inst].name, h->numerator, h->denominator);
	}
	printf("\n");
}

int print_data(int nmeasures)
{
	int i, j;

	for (i=0;i<nmeasures;i++) {
		printf("Measure %d\n", i);
		for (j=0;j<npats;j++)
			if (pat[j].measure == i)
				print_pattern(&pat[j]);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char *buf[MAXLINES];
	int nlines, rc, nmeasures;

	memset(pat, 0, sizeof(pat));

	rc = read_tab_file("stairway.txt", buf, MAXLINES, &nlines);
	printf("rc = %d, nlines = %d\n", rc, nlines);
	process_tab(buf, nlines, &nmeasures);
	print_data(nmeasures);
	exit(0);
}
