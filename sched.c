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
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "sched.h"
#include "midi_file.h"

#define SEC 1000000
#define TIMING_PRECISION 10 /* microseconds */
#define MAX_ALLOWABLE_TARDINESS 200 /* microseconds */ 

int beats_per_measure = 4; 	/* default 4 beats per measure */
int beats_per_minute = 120;	/* default 120 beats per minute */
int measure_length = (60000000 * 4) / 120;
long long mindiff = 100000;

static int dummy_measure, dummy_percent;
static int *pmeasure = &dummy_measure;
static int *ppercent = &dummy_percent;


extern int midi_fd;
extern void note_on(int fd, unsigned char value, unsigned char volume);
extern void note_off(int fd, unsigned char value, unsigned char volume);

int rtime_to_atime(struct timeval *basetime, 
		struct timeval *rtime,
		struct timeval *atime)
{
	long long btime, r, a;

	btime = (long long) basetime->tv_sec;
	btime = btime * 1000000 + (long long) basetime->tv_usec;
	r = (long long) rtime->tv_sec;
	r = r * 1000000 + (long long) rtime->tv_usec;
	a = btime + r;
	/* printf("r = %lld, a=%lld\n", r, a); */
	atime->tv_usec = (long) (a % 1000000);
	atime->tv_sec = (long) ((a - atime->tv_usec) / 1000000);
	/* printf("btime:%lld = %d:%d, rtime:%d:%d -> atime:%d:%d\n", 
		btime,
		basetime->tv_sec, basetime->tv_usec,
		rtime->tv_sec, rtime->tv_usec,
		atime->tv_sec, atime->tv_usec); */
}

int msdiff(struct timeval *tm, struct timeval *prevtm)
{
	long long diff;
	int answer;

	diff = (long long) 1000000 * (long long) (tm->tv_sec - prevtm->tv_sec);
	diff = diff - (long long) prevtm->tv_usec + (long long) tm->tv_usec;
	diff = diff / (long long) 1000;
	/* printf("diff = %lld\n", diff); */
	answer = (int) diff;
	return answer;
}

int wait_for(struct timeval *tm)
{
	struct timeval now;
	long long diff;

	while (1) {
		gettimeofday(&now, NULL);
		/* printf("tm=%d:%d, now=%d:%d\n", tm->tv_sec, tm->tv_usec,
			now.tv_sec, now.tv_usec); */
		diff = (long long) 1000000 * (long long) (tm->tv_sec - now.tv_sec);
		diff = diff - (long long) now.tv_usec + (long long) tm->tv_usec; 
		if (diff > 1000) {
			/* printf("sleeping for %lld usecs\n", diff); */
			usleep(diff>>1);
		}
		if (diff < mindiff)
			mindiff = diff;
		if (diff <= TIMING_PRECISION) {
			/* printf("returning within %lld usecs of request\n", diff); */
			/* printf("."); fflush(stdout); */
			/* if (diff < -MAX_ALLOWABLE_TARDINESS )
				printf("Tardy: %lld usecs\n", -diff); */
			return 0;
		}
	}
}

static inline void set_transport_location(int measure, int percent)
{
	*pmeasure = measure;
	*ppercent = percent;
}

void do_event(struct event *e)
{
	/* printf("%6d:%6d event %2d:%3d:%4d\n", 
		e->rtime.tv_sec, e->rtime.tv_usec,
		e->e.eventtype, e->e.note, e->e.velocity); */

	if (midi_fd < 0)
		return;

	switch (e->e.eventtype) {
	case NOTE_ON: 
		set_transport_location(e->e.measure, e->e.percent);
		note_on(midi_fd, e->e.note, e->e.velocity);
		break;
	case NOTE_OFF: 
		set_transport_location(e->e.measure, e->e.percent);
		note_off(midi_fd, e->e.note, e->e.velocity);
		break;
	case NO_OP:
		set_transport_location(e->e.measure, e->e.percent);
		break;
	default:
		printf("Unknown event type %d\n", e->e.eventtype);
		break;
	}
}

void schedule(struct schedule_t *s)
{
	int i;

	struct timeval now;

	gettimeofday(&now, NULL); /* add a little bit to now */	

	/* calculate the atimes from the rtimes */
	for (i=0;i<s->nevents;i++)
		rtime_to_atime(&now, &s->e[i]->rtime, &s->e[i]->atime);
	/* enter a loop, and fire off each event as the atimes arrive */
	for (i=0;i<s->nevents;i++) {
		wait_for(&s->e[i]->atime);
		do_event(s->e[i]);
	}
}

int add_to_schedule(struct schedule_t *s, 
		struct event *e)
{
	int n = s->nevents;
	int i, j, spot;
	
	if (n >= MAXEVENTS) {
		printf("MAXEVENTS exceeded!\n");
		return -1;
	}
	/* Find where this event goes */
	spot = 0;
	if (e->rtime.tv_usec > 1000000) {
		printf("add_to_schedule, non-normalized time %10d:%10d\n", 
			e->rtime.tv_sec, e->rtime.tv_usec);
	}
	for (spot=0;spot < s->nevents;spot++) {
		if (e->rtime.tv_sec > s->e[spot]->rtime.tv_sec)
			continue;
		if (e->rtime.tv_usec > s->e[spot]->rtime.tv_usec && 
			e->rtime.tv_sec == s->e[spot]->rtime.tv_sec)
			continue;
		break;
	}

	/* We found the spot.  Make room to insert */
	for (i=s->nevents+1;i>spot;i--)
		s->e[i] = s->e[i-1];

	/* Put the event in the spot */
	s->e[spot] = e;
	s->nevents++;
	// print_schedule(s);
	// printf("--------------------------\n");
	return 0;
}

#if 0
int set_timing(int bpmeasure, int bpm)
{
	beats_per_measure = bpmeasure;
	beats_per_minute = bpm;
	measure_length = (60000000 * bpmeasure) / bpm; /* 60 million usecs / minute */
};
#endif

int sched_note(struct schedule_t *s,
		struct timeval *measure_begins,
		unsigned char noteval,
		unsigned long measure_length, /* in microseconds */
		double time, /* fraction of the measure to elapse before starting note */
		int note_duration, /* have yet to figure units on this... */
		unsigned char velocity,
		int measure, 
		int percent,
		long drag
		)
#if 0
int sched_note(struct schedule_t *s,
		struct timeval *measure_begins,
		unsigned char noteval,
		int beats_per_minute,
		int beats_per_measure,
		int note_starts_on_beat,
		int note_duration, /* have yet to figure units on this... */
		unsigned char velocity
		)
#endif
{
	struct event *note, *note2;
	unsigned long long when;
	int secs, usecs;
	note = (struct event *) malloc(sizeof(*note));
	note2 = (struct event *) malloc(sizeof(*note));
	if (note == NULL)
		return -1;

	note->e.note = noteval;
	note->e.eventtype = NOTE_ON;
	note->e.velocity = velocity;
	note->e.measure = measure;
	note->e.percent = percent;
	note2->e.note = noteval;
	note2->e.eventtype = NOTE_OFF;
	note2->e.velocity = velocity;

	when = (unsigned long long) ((double) measure_length * time) + drag;
	usecs = when % 1000000;
	secs = (when - usecs) / 1000000;
	note->rtime.tv_sec = secs;
	note->rtime.tv_usec = usecs;

	
#if 0
	when += 1000000; /* for now, duration = 1 second, well, this is percussion mostly */
	note2->e.note = noteval;
	note2->e.eventtype = NOTE_OFF;
	note2->e.velocity = velocity;
	usecs = (int) (when % 1000000);
	secs = (int) ((when - usecs) / 1000000);
	/* printf("off: when=%lld, usecs=%d, secs=%d\n", when, usecs, secs); */
	note2->rtime.tv_sec = secs;
	note2->rtime.tv_usec = usecs;
#endif
	/* offset these times by the relative time when this measure begins */
	rtime_to_atime(measure_begins, &note->rtime, &note->rtime);
	// rtime_to_atime(measure_begins, &note2->rtime, &note2->rtime);

	add_to_schedule(s, note);  /* note on */
	/* add_to_schedule(s, note2); */ /* note off */ 
	free(note2);

	return 0;
}

int sched_noop(struct schedule_t *s,
		struct timeval *measure_begins,
		unsigned char noteval,
		unsigned long measure_length, /* in microseconds */
		double time, /* fraction of the measure to elapse before starting note */
		int note_duration, /* have yet to figure units on this... */
		unsigned char velocity,
		int measure,
		int percent
		)
{
	struct event *note;
	unsigned long long when;
	int secs, usecs;
	note = (struct event *) malloc(sizeof(*note));
	if (note == NULL)
		return -1;

	note->e.note = noteval;
	note->e.eventtype = NO_OP;
	note->e.velocity = velocity;
	note->e.measure = measure;
	note->e.percent = percent;

	when = (unsigned long long) ((double) measure_length * time);
	usecs = when % 1000000;
	secs = (when - usecs) / 1000000;
	note->rtime.tv_sec = secs;
	note->rtime.tv_usec = usecs;

	/* offset these times by the relative time when this measure begins */
	rtime_to_atime(measure_begins, &note->rtime, &note->rtime);

	add_to_schedule(s, note); /* no op */
	*measure_begins = note->rtime;

	return 0;
}

void free_schedule(struct schedule_t *s)
{
	int i;
	for (i=0;i<s->nevents;i++)
		free(s->e[i]);
	s->nevents = 0;
	return;
}

void print_schedule(struct schedule_t *s)
{
	int i;
	struct timeval rtime; /* relative time */
	struct timeval atime; /* absolute time */
	struct event_details e;
	for (i=0;i<s->nevents;i++) {
		printf("%4d: r:%10ld:%7ld a:%10ld:%7ld t:%d:n:%d:v:%d", 
			i,
			s->e[i]->rtime.tv_sec,
			s->e[i]->rtime.tv_usec,
			s->e[i]->atime.tv_sec,
			s->e[i]->atime.tv_usec,
			s->e[i]->e.eventtype,
			s->e[i]->e.note,
			s->e[i]->e.velocity);
		if (s->e[i]->atime.tv_usec > 1000000) printf("*");
		if (s->e[i]->rtime.tv_usec > 1000000) printf("*");
		printf("\n");
	}
}

void set_transport_meter(int *measure, int *percent)
{
	pmeasure = measure;
	ppercent = percent;
}

void write_midi_event(int fd, struct event *e)
{
	switch (e->e.eventtype) {
	case NOTE_ON: 
		write_note(fd, &e->rtime, 
			0x90, e->e.note, e->e.velocity);
	 // 0x90 | (drumkit[kit].midi_channel & 0x0f)
		break;
	case NOTE_OFF: 
		break;
	case NO_OP:
		break;
	default:
		printf("Unknown event type %d\n", e->e.eventtype);
		break;
	}
	
}

void write_sched_to_midi_file(struct schedule_t *sched, const char *filename)
{
	int fd, i;
	int currpos;

	fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s, %s\n", filename, strerror(errno));
		return;
	}
	write_MThd(fd);
	write_MTrk(fd);
	for (i=0;i<sched->nevents;i++)
		write_midi_event(fd, sched->e[i]);

	write_end_of_track(fd);	
	/* figure Mtrk size, and fixup file */
	currpos = lseek(fd, 0L, SEEK_CUR);
	currpos = htonl(currpos - 22L); 

	lseek(fd, 18L, SEEK_SET);
	write(fd, &currpos, 4); 
	close(fd);
	return;
}

