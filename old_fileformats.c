/* 
    (C) Copyright 2005, Stephen M. Cameron.

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
#include "gneutronica.h"

/* Keep old file format reading code in here so as not
   to clutter up the main code base */

int load_from_file_version_2(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j,count, rc;
	int hidden;
	int fileformatversion;


	printf("Hmm, old file format version 2, will update on save.\n");
	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
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

	for (i=0;i<ninsts;i++) {
		fscanf(f, "Instrument %*d: '%*[^']%*c %d\n", &hidden);
		/* this is questionable if drumkits don't match... */
		if (i<drumkit[kit].ninsts) {
			struct instrument_struct *inst = &drumkit[kit].instrument[i];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
				(gboolean) hidden);
		}
		song_gm_map[i] = -1;
	}
	rc = fscanf(f, "Patterns: %d\n", &npatterns);
	for (i=0;i<npatterns;i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		pattern[i]->gm_converted = 0;
		if (rc != 5)
			printf("Bad divisions...\n");
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %g DK: %d I: %d V: %d B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);

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
		for (j=0;j<count;j++) {
			int ins;
			long drag;
			fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	rc = fscanf(f, "Measures: %d\n", &nmeasures);
	for (i=0;i<nmeasures;i++) {
		fscanf(f, "m:%*d np:%d\n", &measure[i].npatterns);
		if (measure[i].npatterns != 0) {
			for (j=0;j<measure[i].npatterns;j++)
				fscanf(f, "%d ", &measure[i].pattern[j]);
			fscanf(f, "\n");
		}
		/* printf("m:%*d t:%d p:%d\n", measure[i].tempo, measure[i].pattern); */
	}
	rc = fscanf(f, "Tempo changes: %d\n", &ntempochanges);
	for (i=0;i<ntempochanges;i++) {
		fscanf(f, "m:%d bpm:%d\n", &tempo_change[i].measure,
			&tempo_change[i].beats_per_minute);
	}
	fclose(f);

	edit_pattern(0);
/*
	cpattern = 0;
	unflatten_pattern(kit, cpattern);
	for (i=0;i<drumkit[kit].ninsts; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas); */

	return 0;
}

int load_from_file_version_1(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j, rc;
	int hidden;
	int fileformatversion;

	printf("Hmm, old file format version 1, will update on save.\n");
	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
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

	for (i=0;i<ninsts;i++) {
		fscanf(f, "Instrument %*d: '%*[^']%*c %d\n", &hidden);
		/* this is questionable if drumkits don't match... */
		if (i<drumkit[kit].ninsts) {
			struct instrument_struct *inst = &drumkit[kit].instrument[i];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
				(gboolean) hidden);
		}
	}
	rc = fscanf(f, "Patterns: %d\n", &npatterns);
	for (i=0;i<npatterns;i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5)
			printf("Bad divisions...\n");
		pattern[i]->gm_converted = 0;
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %g DK: %d I: %d V: %d B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);

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
		make_new_pattern_widgets(i, i+1);
	}
	rc = fscanf(f, "Measures: %d\n", &nmeasures);
	for (i=0;i<nmeasures;i++) {
		fscanf(f, "m:%*d np:%d\n", &measure[i].npatterns);
		if (measure[i].npatterns != 0) {
			for (j=0;j<measure[i].npatterns;j++)
				fscanf(f, "%d ", &measure[i].pattern[j]);
			fscanf(f, "\n");
		}
		/* printf("m:%*d t:%d p:%d\n", measure[i].tempo, measure[i].pattern); */
	}
	rc = fscanf(f, "Tempo changes: %d\n", &ntempochanges);
	for (i=0;i<ntempochanges;i++) {
		fscanf(f, "m:%d bpm:%d\n", &tempo_change[i].measure,
			&tempo_change[i].beats_per_minute);
	}
	fclose(f);

	edit_pattern(0);
/*
	cpattern = 0;
	unflatten_pattern(kit, cpattern);
	for (i=0;i<drumkit[kit].ninsts; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas); */

	return 0;
}


int import_patterns_v2(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j,count, rc;
	int hidden;
	int fileformatversion;
	int fake_ninsts;
	int newpatterns;

	linecount = 1;
	rc = fscanf(f, "Songname: '%[^']%*c\n", songname);
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
	for (i=0;i<fake_ninsts;i++)
		fscanf(f, "Instrument %*d: '%*[^']%*c %*d\n");

	rc = fscanf(f, "Patterns: %d\n", &newpatterns);
	for (i=npatterns;i<npatterns+newpatterns;i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		fscanf(f, "Pattern %*d: %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, pattern[i]->patname);
		/* printf("patname %d = %s\n", i, pattern[i]->patname); */
		rc = fscanf(f, "Divisions: %d %d %d %d %d\n", 
			&pattern[i]->timediv[0].division,
			&pattern[i]->timediv[1].division,
			&pattern[i]->timediv[2].division,
			&pattern[i]->timediv[3].division,
			&pattern[i]->timediv[4].division);
		if (rc != 5)
			printf("Bad divisions...\n");
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			*h = malloc(sizeof(struct hitpattern));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %g DK: %d I: %d V: %d B:%d BPM:%d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure);

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
		for (j=0;j<count;j++) {
			int ins;
			long drag;
			fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	fclose(f);
	npatterns += newpatterns;
	return 0;
}
