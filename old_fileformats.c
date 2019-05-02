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
#include <stdio.h>
#include <gtk/gtk.h>
#include <malloc.h>
#include <string.h>

#include "gneutronica.h"

/* Keep old file format reading code in here so as not
   to clutter up the main code base */
void set_old_noteoff(struct hitpattern *h)
{
	h->h.noteoff_time = 1.0;
	h->h.noteoff_beat = 4;
	h->h.noteoff_beats_per_measure = 4;	
}

extern int xpect(FILE *f, int *lc, char *line, char *value);
extern void make_new_pattern_widgets(int new_pattern, int total_rows);
extern void edit_pattern(int new_pattern);

int load_from_file_version_2(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j,count, rc;
	int hidden;

	printf("Hmm, old file format version 2, will update on save.\n");
	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Songname...\n", linecount);
		goto error_out;
	}
	linecount++;
	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Make:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Model:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Name:") == -1) return -1;

	rc = fscanf(f, "Instruments: %d\n", &ninsts);
	if (rc != 1)  {
		fprintf(stderr, "%d: Expected Instruments...\n", linecount);
		goto error_out;
	}
	linecount++;

	for (i = 0; i < ninsts; i++) {
		rc = fscanf(f, "Instrument %*d: '%*[^']%*c %d\n", &hidden);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected Instrument...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* this is questionable if drumkits don't match... */
		if (i<drumkit[kit].ninsts) {
			struct instrument_struct *inst = &drumkit[kit].instrument[i];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
				(gboolean) hidden);
		}
		song_gm_map[i] = -1;
	}
	rc = fscanf(f, "Patterns: %d\n", &npatterns);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Patterns...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0;i < npatterns; i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		rc = fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		if (rc != 3) {
			fprintf(stderr, "%d: Expected Pattern...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5) {
			fprintf(stderr, "%d: Expected Divisions...\n", linecount);
			goto error_out;
		}
		linecount++;
		pattern[i]->gm_converted = 0;
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			memset(*h, 0, sizeof(**h));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);
			if (rc != 6) {
				fprintf(stderr, "%d: Expected T, DK, I, V, B, BPM...\n", linecount);
				goto error_out;
			}
			linecount++;
			set_old_noteoff(*h);

			/* printf("T: %g DK: %d I: %d v: %d b:%d bpm:%d\n", 
				(*h)->h.time, (*h)->h.drumkit, (*h)->h.instrument_num,
				(*h)->h.velocity, (*h)->h.beat, (*h)->h.beats_per_measure); */
			/* Holy shit, scanf with %g doesn't actually work! */
			if ((*h)->h.beats_per_measure == 0) {
				printf("Corrupted file?  beats_per_measure was zero... Guessing 4.\n");
				(*h)->h.beats_per_measure = 4;
			}
			(*h)->h.time = (double) (*h)->h.beat / (double) (*h)->h.beats_per_measure;

			/* printf("new time is %g\n", (*h)->h.time); */
			h = &(*h)->next;
		}
		rc = fscanf(f, "dragging count: %d\n", &count);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected dragging count...\n", linecount);
			goto error_out;
		}
		linecount++;
		for (j = 0; j < count; j++) {
			int ins;
			long drag;
			rc = fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			if (rc != 2) {
				fprintf(stderr, "%d: Expected i, d...\n", linecount);
				goto error_out;
			}
			linecount++;
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	rc = fscanf(f, "Measures: %d\n", &nmeasures);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Measures...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < nmeasures; i++) {
		rc = fscanf(f, "m:%*d np:%d\n", &measure[i].npatterns);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected m:... np:...\n", linecount);
			goto error_out;
		}
		linecount++;
		if (measure[i].npatterns != 0) {
			for (j = 0; j < measure[i].npatterns; j++) {
				rc = fscanf(f, "%d ", &measure[i].pattern[j]);
				if (rc != 1) {
					fprintf(stderr, "%d: j = %d, Expected number\n", linecount, j);
					goto error_out;
				}
			}
			rc = fscanf(f, "\n");
			if (rc != 0) {
				fprintf(stderr, "%d: Failed to read newline\n", linecount);
				goto error_out;
			}
			linecount++;
		}
		/* printf("m:%*d t:%d p:%d\n", measure[i].tempo, measure[i].pattern); */
	}
	rc = fscanf(f, "Tempo changes: %d\n", &ntempochanges);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Tempo changes...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < ntempochanges; i++) {
		rc = fscanf(f, "m:%d bpm:%d\n", &tempo_change[i].measure,
			&tempo_change[i].beats_per_minute);
		if (rc != 2) {
			fprintf(stderr, "%d: expected m:... bpm:...\n", linecount);
			goto error_out;
		}
		linecount++;
	}
	fclose(f);

	edit_pattern(0);
/*
	cpattern = 0;
	unflatten_pattern(kit, cpattern);
	for (i=0;i<drumkit[kit].ninsts; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas); */

	return 0;

error_out:
	pattern_struct_free(pattern, npatterns);
	return -1;
}

int load_from_file_version_1(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j, rc;
	int hidden;

	printf("Hmm, old file format version 1, will update on save.\n");
	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Songname...\n", linecount);
		goto error_out;
	}
	linecount++;
	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Make:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Model:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Name:") == -1) return -1;

	rc = fscanf(f, "Instruments: %d\n", &ninsts);
	if (rc != 1)  {
		printf("%d: error\n", linecount);
		return -1;
	}
	linecount++;

	for (i = 0; i < ninsts; i++) {
		rc = fscanf(f, "Instrument %*d: '%*[^']%*c %d\n", &hidden);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected Instrument...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* this is questionable if drumkits don't match... */
		if (i<drumkit[kit].ninsts) {
			struct instrument_struct *inst = &drumkit[kit].instrument[i];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
				(gboolean) hidden);
		}
	}
	rc = fscanf(f, "Patterns: %d\n", &npatterns);
	for (i = 0; i < npatterns; i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		rc = fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		if (rc != 3) {
			fprintf(stderr, "%d: Expected Pattern...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5) {
			fprintf(stderr, "%d: Bad divisions...\n", linecount);
		}
		linecount++;
		pattern[i]->gm_converted = 0;
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (rc != 1) {
				fprintf(stderr, "%d: Failed to read line\n", linecount);
				goto error_out;
			}
			linecount++;
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			memset(*h, 0, sizeof(**h));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);
			if (rc != 6) {
				fprintf(stderr, "%d: Expected T:..DK:..I:..V:..B:..BPM:..\n", linecount);
				goto error_out;
			}
			linecount++;

			/* printf("T: %g DK: %d I: %d v: %d b:%d bpm:%d\n", 
				(*h)->h.time, (*h)->h.drumkit, (*h)->h.instrument_num,
				(*h)->h.velocity, (*h)->h.beat, (*h)->h.beats_per_measure); */
			/* Holy shit, scanf with %g doesn't actually work! */
			if ((*h)->h.beats_per_measure == 0) {
				printf("Corrupted file?  beats_per_measure was zero... Guessing 4.\n");
				(*h)->h.beats_per_measure = 4;
			}
			(*h)->h.time = (double) (*h)->h.beat / (double) (*h)->h.beats_per_measure;

			/* printf("new time is %g\n", (*h)->h.time); */
			h = &(*h)->next;
		}
		make_new_pattern_widgets(i, i+1);
	}
	rc = fscanf(f, "Measures: %d\n", &nmeasures);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Measures:...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < nmeasures; i++) {
		rc = fscanf(f, "m:%*d np:%d\n", &measure[i].npatterns);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected M:...np:...\n", linecount);
			goto error_out;
		}
		linecount++;
		if (measure[i].npatterns != 0) {
			for (j = 0; j < measure[i].npatterns; j++) {
				rc = fscanf(f, "%d ", &measure[i].pattern[j]);
				if (rc != 1) {
					fprintf(stderr, "%d: Expected number...\n", linecount);
					goto error_out;
				} 
				rc = fscanf(f, "\n");
				if (rc != 0) {
					fprintf(stderr, "%d: Failed to read newline\n", linecount);
					goto error_out;
				}
				linecount++;
			}
		}
		/* printf("m:%*d t:%d p:%d\n", measure[i].tempo, measure[i].pattern); */
	}
	rc = fscanf(f, "Tempo changes: %d\n", &ntempochanges);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Tempo changes...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < ntempochanges; i++) {
		rc = fscanf(f, "m:%d bpm:%d\n", &tempo_change[i].measure,
			&tempo_change[i].beats_per_minute);
		if (rc != 2) {
			fprintf(stderr, "%d: Expected m:...bpm:...\n", linecount);
			goto error_out;
		}
		linecount++;
	}
	fclose(f);

	edit_pattern(0);
/*
	cpattern = 0;
	unflatten_pattern(kit, cpattern);
	for (i=0;i<drumkit[kit].ninsts; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas); */

	return 0;

error_out:
	pattern_struct_free(pattern, npatterns);
	return -1;
}


int import_patterns_v2(FILE *f)
{
	char line[255];
	int linecount;
	int i,j,count, rc;
	int fake_ninsts;
	int newpatterns;

	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Songname...\n", linecount);
		goto error_out;
	}
	linecount++;
	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Make:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Model:") == -1) return -1;
	if (xpect(f, &linecount, line, "Drumkit Name:") == -1) return -1;

	rc = fscanf(f, "Instruments: %d\n", &fake_ninsts);
	if (rc != 1)  {
		printf("%d: error\n", linecount);
		return -1;
	}
	linecount++;

	/* skip all the instrumets . . . may want to add drumkit remapping code here */
	for (i = 0; i < fake_ninsts; i++) {
		rc = fscanf(f, "Instrument %*d: '%*[^']%*c %*d\n");
		if (rc != 0) {
			fprintf(stderr, "%d Expected Instrument...\n", linecount);
			goto error_out;
		}
		linecount++;
	}

	rc = fscanf(f, "Patterns: %d\n", &newpatterns);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Patterns...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = npatterns; i < npatterns+newpatterns; i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		rc = fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		if (rc != 3) {
			fprintf(stderr, "%d: Expected Patern...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5) {
			fprintf(stderr, "%d: Bad divisions...\n", linecount);
			goto error_out;
		}
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (rc != 1) {
				fprintf(stderr, "%d: failed to read line\n", linecount);
				goto error_out;
			}
			linecount++;
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			memset(*h, 0, sizeof(**h));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);
			if (rc != 6) {
				fprintf(stderr, "%d: Expected T:..DK:..I:..V:..B:..BPM:..\n", linecount);
				goto error_out;
			}
			linecount++;
			set_old_noteoff(*h);

			/* printf("T: %g DK: %d I: %d v: %d b:%d bpm:%d\n", 
				(*h)->h.time, (*h)->h.drumkit, (*h)->h.instrument_num,
				(*h)->h.velocity, (*h)->h.beat, (*h)->h.beats_per_measure); */
			/* Holy shit, scanf with %g doesn't actually work! */
			if ((*h)->h.beats_per_measure == 0) {
				printf("Corrupted file?  beats_per_measure was zero... Guessing 4.\n");
				(*h)->h.beats_per_measure = 4;
			}
			(*h)->h.time = (double) (*h)->h.beat / (double) (*h)->h.beats_per_measure;

			/* printf("new time is %g\n", (*h)->h.time); */
			h = &(*h)->next;
		}
		rc = fscanf(f, "dragging count: %d\n", &count);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected dragging count...\n", linecount);
			goto error_out;
		}
		linecount++;
		for (j = 0; j < count; j++) {
			int ins;
			long drag;
			rc = fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			if (rc != 2) {
				fprintf(stderr, "%d: expected i:..d:...\n", linecount);
				goto error_out;
			}
			linecount++;
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	fclose(f);
	npatterns += newpatterns;
	return 0;

error_out:
	pattern_struct_free(pattern, npatterns + newpatterns);
	return -1;
}

int load_from_file_version_3(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j,count, rc;
	int hidden;
	int gm_equiv;
	char dkmake[100];
	char dkmodel[100];
	char dkname[100];
	int same_drumkit;

	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Songname...\n", linecount);
		goto error_out;
	}
	linecount++;

	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;

	rc = fscanf(f, "Drumkit Make:%[^\n]%*c", dkmake); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit make\n");
	rc = fscanf(f, "Drumkit Model:%[^\n]%*c", dkmodel); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit model\n");
	rc = fscanf(f, "Drumkit Name:%[^\n]\%*c", dkname); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit name\n");

	
	
	same_drumkit = (strcmp(drumkit[kit].make, dkmake) == 0 && 
		strcmp(drumkit[kit].model, dkmodel) == 0 &&
		strcmp(drumkit[kit].name, dkname) == 0);

	/* printf("'%s','%s','%s'\n", dkmake, dkmodel, dkname);
	printf(same_drumkit ? "Same drumkit\n" : "Different drumkit\n"); */

	if (!same_drumkit)
		printf("WARNING: Different drum kit for this song, remap to adjust to current drum kit...\n");

	rc = fscanf(f, "Instruments: %d\n", &ninsts);
	if (rc != 1)  {
		printf("%d: error\n", linecount);
		return -1;
	}
	linecount++;

	for (i = 0; i < ninsts; i++) {
		rc = fscanf(f, "Instrument %*d: '%*[^']%*c %d %d\n", &hidden, &gm_equiv);
		if (rc != 2) {
			fprintf(stderr, "%d: Expected Instrument...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* this is questionable if drumkits don't match... */
		if (i<drumkit[kit].ninsts) {
			struct instrument_struct *inst = &drumkit[kit].instrument[i];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
				(gboolean) hidden);
			song_gm_map[i] = inst->gm_equivalent; 
		}
		if (!same_drumkit)
			song_gm_map[i] = gm_equiv;
		/* printf("song_gm_map[%d] = %d\n", i, song_gm_map[i]); */
	}
	rc = fscanf(f, "Patterns: %d\n", &npatterns);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Patterns...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < npatterns; i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		rc = fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		if (rc != 3) {
			fprintf(stderr, "%d: Pattern...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5) {
			fprintf(stderr, "%d: Expected Divisions...\n", linecount);
			goto error_out;
		}
		linecount++;
		pattern[i]->gm_converted = 0;
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (rc != 0) {
				fprintf(stderr, "%d: failed to read line...\n", linecount);
				goto error_out;
			}
			linecount++;
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			memset(*h, 0, sizeof(**h));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);
			if (rc != 6) {
				fprintf(stderr, "%d: Expected T:..DK:..I:..V:..B:..BPM:..\n", linecount);
				goto error_out;
			}
			linecount++;
			set_old_noteoff(*h);

			/* printf("T: %g DK: %d I: %d v: %d b:%d bpm:%d\n", 
				(*h)->h.time, (*h)->h.drumkit, (*h)->h.instrument_num,
				(*h)->h.velocity, (*h)->h.beat, (*h)->h.beats_per_measure); */
			/* Holy shit, scanf with %g doesn't actually work! */
			if ((*h)->h.beats_per_measure == 0) {
				printf("Corrupted file?  beats_per_measure was zero... Guessing 4.\n");
				(*h)->h.beats_per_measure = 4;
			}
			(*h)->h.time = (double) (*h)->h.beat / (double) (*h)->h.beats_per_measure;

			/* printf("new time is %g\n", (*h)->h.time); */
			if (rc != 6) 
				printf("rc != 6!\n");
			h = &(*h)->next;
		}
		rc = fscanf(f, "dragging count: %d\n", &count);
			if (rc != 1) {
				fprintf(stderr, "%d: Expected dragging count...\n", linecount);
				goto error_out;
			}
			linecount++;
		for (j = 0; j < count; j++) {
			int ins;
			long drag;
			rc = fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			if (rc != 2) {
				fprintf(stderr, "%d: Expected i:..d:..\n", linecount);
				goto error_out;
			}
			linecount++;
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	rc = fscanf(f, "Measures: %d\n", &nmeasures);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Measures...\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < nmeasures; i++) {
		rc = fscanf(f, "m:%*d np:%d\n", &measure[i].npatterns);
		if (rc != 1) {
			fprintf(stderr, "%d: m:..np:..\n", linecount);
			goto error_out;
		}
		linecount++;
		if (measure[i].npatterns != 0) {
			for (j = 0; j < measure[i].npatterns; j++) {
				rc = fscanf(f, "%d ", &measure[i].pattern[j]);
				if (rc != 1) {
					fprintf(stderr, "%d: m:..np:..\n", linecount);
					goto error_out;
				}
				linecount++;
			}
			rc = fscanf(f, "\n");
			if (rc != 0) {
				fprintf(stderr, "%d: Expected newline\n", linecount);
				goto error_out;
			}
			linecount++;
		}
		/* printf("m:%*d t:%d p:%d\n", measure[i].tempo, measure[i].pattern); */
	}
	rc = fscanf(f, "Tempo changes: %d\n", &ntempochanges);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Tempo changes\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < ntempochanges; i++) {
		rc = fscanf(f, "m:%d bpm:%d\n", &tempo_change[i].measure,
			&tempo_change[i].beats_per_minute);
		if (rc != 2) {
			fprintf(stderr, "%d: Expected m:..bpm:..\n", linecount);
			goto error_out;
		}
		linecount++;
	}
	fclose(f);

	edit_pattern(0);
/*
	cpattern = 0;
	unflatten_pattern(kit, cpattern);
	for (i=0;i<drumkit[kit].ninsts; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas); */

	return 0;

error_out:
	pattern_struct_free(pattern, npatterns);
	return -1;
}

int import_patterns_v3(FILE *f)
{
	char line[255];
	int linecount;
	int i,j,k,count, rc;
	int fake_ninsts;
	int newpatterns;
	char dkmake[100], dkmodel[100], dkname[100];
	int same_drumkit;
	int import_inst_map[MAXINSTS], gm;

	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Songname...\n", linecount);
		goto error_out;
	}
	linecount++;
	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;

	rc = fscanf(f, "Drumkit Make:%[^\n]%*c", dkmake); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit make\n");
	rc = fscanf(f, "Drumkit Model:%[^\n]%*c", dkmodel); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit model\n");
	rc = fscanf(f, "Drumkit Name:%[^\n]\%*c", dkname); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit name\n");

	same_drumkit = (strcmp(drumkit[kit].make, dkmake) == 0 && 
		strcmp(drumkit[kit].model, dkmodel) == 0 &&
		strcmp(drumkit[kit].name, dkname) == 0);

	if (!same_drumkit)
		printf("Import file uses different drumkit... will attempt to remap.\n");

	rc = fscanf(f, "Instruments: %d\n", &fake_ninsts);
	if (rc != 1)  {
		printf("%d: error\n", linecount);
		return -1;
	}
	linecount++;

	for (i = 0; i < fake_ninsts; i++) {
		rc = fscanf(f, "Instrument %*d: '%*[^']%*c %*d %d\n", &import_inst_map[i]);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected Instrument...\n", linecount);
			goto error_out;
		}
		linecount++;
	}

	rc = fscanf(f, "Patterns: %d\n", &newpatterns);
	for (i=npatterns;i<npatterns+newpatterns;i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		rc = fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		if (rc != 3) {
			fprintf(stderr, "%d: expected Pattern...\n", linecount);
			goto error_out;
		}
		linecount++;
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5) {
			fprintf(stderr, "%d: Bad divisions...\n", linecount);
			goto error_out;
		}
		linecount++;
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (rc != 1) {
				fprintf(stderr, "%d: failed to read line\n", linecount);
				goto error_out;
			}
			linecount++;
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			memset(*h, 0, sizeof(**h));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);
			if (rc != 6) {
				fprintf(stderr, "%d: Expected T:...DK:..I:...V:...B:...BPM:...\n", linecount);
				goto error_out;
			}
			linecount++;

			set_old_noteoff(*h);

			gm = import_inst_map[(*h)->h.instrument_num];
			if (gm != -1)
				for (k=0;k<drumkit[kit].ninsts;k++) {
					if (gm == drumkit[kit].instrument[k].gm_equivalent) {
						/* printf("remapping %d to %d\n", (*h)->h.instrument_num, k); */
						(*h)->h.instrument_num = k;
						break;	
					}
				}

			/* printf("T: %g DK: %d I: %d v: %d b:%d bpm:%d\n", 
				(*h)->h.time, (*h)->h.drumkit, (*h)->h.instrument_num,
				(*h)->h.velocity, (*h)->h.beat, (*h)->h.beats_per_measure); */
			/* Holy shit, scanf with %g doesn't actually work! */
			if ((*h)->h.beats_per_measure == 0) {
				printf("Corrupted file?  beats_per_measure was zero... Guessing 4.\n");
				(*h)->h.beats_per_measure = 4;
			}
			(*h)->h.time = (double) (*h)->h.beat / (double) (*h)->h.beats_per_measure;

			/* printf("new time is %g\n", (*h)->h.time); */
			if (rc != 6) 
				printf("rc != 6!\n");
			h = &(*h)->next;
		}
		rc = fscanf(f, "dragging count: %d\n", &count);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected dragging count...\n", linecount);
			goto error_out;
		}
		linecount++;
		for (j = 0; j < count; j++) {
			int ins;
			long drag;
			rc = fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			if (rc != 2) {
				fprintf(stderr, "%d: expected i:... d:...\n", linecount);
				goto error_out;
			}
			linecount++;
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	fclose(f);
	npatterns += newpatterns;
	return 0;
error_out:
	pattern_struct_free(pattern, npatterns + newpatterns);
	return -1;
}
