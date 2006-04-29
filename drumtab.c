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
	int velocity;
} imap[] = {
	{ "h", 42, 46, 80 },
	{ "hh", 42, 46, 80 },
	{ "hihat", 42, 46, 80 },
	{ "hc", 46, 46, 80 },
	{ "hhfoot", 46, 46, 80 },
	{ "hf", 46, 46, 80 },
	{ "c", 49, 52, 120 },
	{ "crash", 49, 52, 120 },
	{ "cc", 49, 52, 120 },
	{ "ft", 41, 41, 110 },
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
	{ "s", 38, 40, 125 },
	{ "sd", 40, 40, 125 },
	{ "snare", 40, 40, 125 },
	{ "b", 35, 36, 126 },
	{ "bass", 35, 36, 126 },
	{ "bd", 35, 36, 126 },
	{ "cb", 56, 56, 126 },
	{ "splash", 55, 55, 120 },
	{ "a", 55, 55, 120 },

};

static int used[256];

static void init_used()
{
	memset(used, 0, 256);
}

static nimappings = (sizeof(imap) / sizeof(imap[0]));


static int lookup_instrument(char *name, int *velocity)
{
	char newname[100];

	int i;
	*velocity = 110; /* default */
	memset(newname, 0, 100);
	strncpy(newname, name, 99);
	for (i=0;newname[i] != '\0';i++)
		newname[i] = tolower(newname[i]);
	for (i=0;i<nimappings;i++)
		if (strcmp(newname, imap[i].name) == 0) {
			used[imap[i].midi_note]= 1;
			*velocity = imap[i].velocity;
			return imap[i].midi_note;
		}

	printf("Unrecognized instrument '%s'\n", name);
	*velocity = UNRECOGNIZED_VOLUME;
	return UNRECOGNIZED_INSTRUMENT; ; /* cuica... weird little instrument, audibly stands out as wrong. */
#if 0
	This code just makes a mess... starts changing programs on my motif rack
	for (i=127;i>0;i--)
		if (!used[i]) {
			used[i] = 1;
			return i;
		}
	printf("Bad instrument '$s'\n", name);
	return 127; /* this is bad. */
#endif
}

int is_measure_separator(int c)
{
	return (strchr(MEASURE_SEPARATOR, c) != NULL);
}

static int find_instrument(char *line)
{
	int i, j;
	char n[100];
	int found = 0;

	j = 0; n[0] = '\0';

	/* printf("line = %s\n", line); */
	for (i=0;i<strlen(line);i++) {
		if (is_measure_separator(line[i]))
			break;
		if (isalpha(line[i])) {
			n[j] = line[i];
			j++;
			n[j] = '\0';
		}
	}
	printf("n = %s\n", n);

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
		dt_inst[dt_ninsts].midi_value = lookup_instrument(n, &dt_inst[dt_ninsts].velocity);

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

	printf("add_hit, measure=%d\n", measure);
	p = dt_pat[dt_npats].hit;
	dt_pat[dt_npats].hit = malloc(sizeof(*dt_pat[dt_npats].hit));
	dt_pat[dt_npats].hit->next = p;
	p = dt_pat[dt_npats].hit;
	p->numerator = numer;
	p->denominator = denom;
	p->velocity = dt_inst[instrument].velocity;
	reduce_fraction(&p->numerator, &p->denominator);
	p->inst = instrument;
	dt_pat[dt_npats].staffline = staff;
	dt_pat[dt_npats].measure = measure;
	dt_pat[dt_npats].gn_pattern = -1;
	return 0;
}

static int is_junk(char *s)
{
	int i, len;
	len = strlen(s);
	for (i=0;i<len;i++)
		if (!isspace(s[i]) && !is_measure_separator(s[i]) && s[i] != '\n')
			return 0;
	return 1;
}

static int process_line(char *line, int instrument,
	int current_measure, int current_staff,
	int *last_measure_of_staff)
{
	char *chunk;
	int denom, numer, i;
	int added_hit;

	for (chunk = strtok(line, MEASURE_SEPARATOR); chunk ; chunk = strtok(NULL, MEASURE_SEPARATOR)) {
		printf("chunk = '%s'\n", chunk);
		if (is_junk(chunk))
			continue;
		denom = strlen(chunk);
		added_hit = 0;
		for (i=0;i<denom;i++) {
			if (chunk[i] == 'x' ||
				chunk[i] == 'f' ||
				chunk[i] == 'o') {
				numer = i;
				add_hit(instrument, current_staff,
					current_measure, numer, denom);
				added_hit = 1;
			}
		}
		current_measure++;
		if (added_hit)
			dt_npats++;
	}
	*last_measure_of_staff = current_measure - 1;
	return 0;
}

static int process_tab(char *buffer[], int nlines, int *nmeasures)
{
	int i, j;
	char *sepr;
	int current_measure = 0;
	int last_measure = -1;
	int current_instrument = -1;
	int new_staff_iminent = 1;
	int current_staff = -1;
	int last_measure_of_staff = -1;
	int this_measure;
	int in = -1;

	char *vbar;

	sepr = MEASURE_SEPARATOR;

	for (i=0;i<nlines;i++) {
		/* look for a a vertical bar */
		for (j=0;j<strlen(sepr);j++) {
			vbar = strchr(buffer[i], sepr[j]);
			if (vbar != NULL)
				break;
		}
		if (vbar == NULL) {
			new_staff_iminent = 1;
			continue;
		}
		if (new_staff_iminent) {
			printf("%d: new staff begins\n", i);
			new_staff_iminent = 0;
			current_staff++;
			current_measure = last_measure_of_staff+1;
		}
		in = find_instrument(buffer[i]);
		process_line(vbar, in, current_measure,
			current_staff, &last_measure_of_staff);
	}

	*nmeasures = last_measure_of_staff+1;

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
		for (j=0;j<dt_npats;j++) {
			if (dt_pat[j].measure == i)
				print_pattern(&dt_pat[j]);
			if (dt_pat[j].hit == NULL)
				printf("pat %d is null\n", j);
		}
	}
	return 0;
}

static int patterns_equal(struct dt_pattern_type *p1, struct dt_pattern_type *p2)
{

	struct dt_hit_type *h1, *h2;

	h1 = p1->hit;
	h2 = p2->hit;

	if (h1 == NULL && h2 == NULL)
		return 1;
	if (h1 == NULL)
		return 0;
	if (h2 == NULL)
		return 0;
	for (h1 = p1->hit; h1; h1 = h1->next) {
		if (h2 == NULL)
			return 0;
		if (h1->inst != h2->inst ||
			h1->numerator != h2->numerator ||
			h1->denominator != h2->denominator)
				return 0;
		h2 = h2->next;
	}
	if (h2 != NULL)
		return 0;
	return 1;
}

static int sort_by_measure()
{
	/* sort the patterns by measure order, but with a measure, leave the patterns in order. */

	int i, j;
	struct dt_pattern_type temp;
	int insert_before = 0;
	int measure = 0;
	int done;
	int maxmeasure = 0;
	int k;

	for (i=0;i<dt_npats;i++)
		if (dt_pat[i].measure > maxmeasure)
			maxmeasure = dt_pat[i].measure;

	for (measure = 0;measure < maxmeasure + 1; measure++)
		do {
			/* Find insertion point */
			for (i=0;i<dt_npats;i++)
				if (dt_pat[i].measure > measure) {
					insert_before = i;
					break;
				}
			/* printf("insert_before = %d, measure=%d\n", insert_before, measure); */
			done = 1;
			for (i=insert_before+1;i<dt_npats;) {
				if (dt_pat[i].measure == measure) {
					temp = dt_pat[i];
					/* this is horribly inefficient, guaranteed almost 100% overlap */
					memmove(&dt_pat[insert_before+1], &dt_pat[insert_before],
						sizeof(temp) * (i - insert_before));
					dt_pat[insert_before] = temp;
					done = 0;
					insert_before++;
				}
				i++;
			}
		} while (!done);
	return 0;
}

static int find_duplicates()
{
	int nduplicates_found = 0;;
	int i, j;
	for (i=0;i<dt_npats;i++) {
		if (dt_pat[i].duplicate_of != -1)
			continue;
		for (j=0;j<dt_npats;j++) {
			if (i == j)  		/* skip self-compare */
				continue;
			/* if (dt_pat[j].duplicate_of == -1)
				continue; */
			if (patterns_equal(&dt_pat[i], &dt_pat[j])) {
				if (i<j) {
					dt_pat[j].duplicate_of = i;
					dt_pat[i].is_unique = 0;
					dt_pat[j].is_unique = 0;
					printf("%d duplicate of %d\n", j, i);
				} else {
					dt_pat[i].duplicate_of = j;
					dt_pat[i].is_unique = 0;
					dt_pat[j].is_unique = 0;
					printf("%d duplicate of %d\n", i, j);
					nduplicates_found++;
					break;
				}
				nduplicates_found++;
			}
		}
	}
	printf("npats = %d, nduplicates = %d, unique = %d\n",
		dt_npats,  nduplicates_found, dt_npats - nduplicates_found);
}

int collapse_unique_patterns()
{
	/* for each measure which contains several unique_patterns,
	   collapse those patterns into a single pattern */

	int i, j;
	int first_unique, measure;
	struct dt_hit_type *last, *h;

	printf("Scanning for unique patterns to collapse\n");
	for (i=0;i<dt_npats;i++) {
		if (!dt_pat[i].is_unique)
			continue;

		first_unique = i;
		measure = dt_pat[i].measure;
		printf("First unique pattern is %d, m=%d\n", i, measure);

		for (j=i+1;j<dt_npats;) {
			if (dt_pat[j].measure > measure)
				break;
			if (dt_pat[j].is_unique && dt_pat[i].measure == measure) {
				printf("Joining pat %d with %d\n", j, first_unique);
				/* join this pattern with pattern first_unique */
				for (h = dt_pat[j].hit; h != NULL; h=h->next)
					if (h->next == NULL)
						last = h;
				last->next = dt_pat[first_unique].hit;
				dt_pat[first_unique].hit = dt_pat[j].hit;
				dt_pat[j].hit = NULL;
				if (j < dt_npats-1)
					memmove(&dt_pat[j], &dt_pat[j+1], sizeof(dt_pat[0])*(dt_npats-1-j));
				dt_npats--;
			} else
				j++;
		}
	}
}

static void initialize()
{
	int i;
	memset(dt_pat, 0, sizeof(dt_pat));
	for (i=0;i<MAXPATS;i++) {
		dt_pat[i].duplicate_of = -1;
		dt_pat[i].is_unique = 1;
	}
	dt_npats = 0;
	dt_nmeasures = 0;
}


int process_drumtab_lines(char *buf[], int nlines, int factor)
{
	process_tab(buf, nlines, &dt_nmeasures);
	sort_by_measure();
	if (factor)
		find_duplicates();
	collapse_unique_patterns();
	find_duplicates();
	collapse_unique_patterns();
	print_data(dt_nmeasures);
}

int process_drumtab_file(const char *filename, int factor)
{
	char *buf[MAXLINES];
	int nlines, rc, i;

	initialize();

	rc = read_tab_file(filename, buf, MAXLINES, &nlines);
	printf("rc = %d, nlines = %d\n", rc, nlines);
	process_drumtab_lines(buf, nlines, factor);
}

void process_drumtab_buffer(char *buffer, int factor)
{
	/* convert a straight char buffer to an array of pointers to char */
	char *buf[MAXLINES];
	int i, nlines;
	int len;
	char *spot;

	initialize();
	spot = buffer;
	len = strlen(buffer);
	buf[0] = spot;
	nlines = 0;
	for (i=0;i<=len;i++) {
		if (buffer[i] == '\n' || buffer[i] == '\0') {
			buf[nlines] = spot;
			buffer[i] = '\0';
			spot = &buffer[i+1];
			nlines++;
		}
	}
	process_drumtab_lines(buf, nlines, factor);
}


