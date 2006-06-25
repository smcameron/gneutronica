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
#ifndef __SCHED_H__
#define __SCHED_H__

#define NOTE_ON 1
#define NOTE_OFF 2
#define NO_OP 3
#define SEC 1000000
#define MAXEVENTS 1000000
#define TIMING_PRECISION 10 /* microseconds */

struct event_details {
	int eventtype;
	unsigned char track;
	unsigned char channel;
	unsigned char note;
	unsigned char velocity;
	int measure; /* transport location, measure, and percent through measure */
	int percent; 
};

struct event {
	struct timeval rtime; /* relative time */
	struct timeval atime; /* absolute time */
	struct event_details e;
};

struct schedule_t {
	int nevents;
	struct event *e[MAXEVENTS];
};

int rtime_to_atime(struct timeval *basetime, struct timeval *rtime,
		struct timeval *atime);
int wait_for(struct timeval *tm);
void do_event(struct event *e);
void schedule(struct schedule_t *s);
int add_to_schedule(struct schedule_t *s, struct event *e);
int sched_note(struct schedule_t *s,
		struct timeval *measure_begins,
		unsigned char track,
		unsigned char channel,
		unsigned char noteval,
		unsigned long measure_length, /* in microseconds */
		double time, /* fraction of the measure to elapse before starting note */
		int note_duration, /* have yet to figure units on this... */
		unsigned char velocity,
		int measure,  /* for transport location */
		int percent,  /* for transport location */
		long drag
		);
int sched_noop(struct schedule_t *s,
		struct timeval *measure_begins,
		unsigned char track,
		unsigned char noteval,
		unsigned long measure_length, /* in microseconds */
		double time, /* fraction of the measure to elapse before starting note */
		int note_duration, /* have yet to figure units on this... */
		unsigned char velocity,
		int measure, /* transport location */
		int percent
		);
int msdiff(struct timeval *tm, struct timeval *prevtm);
void free_schedule(struct schedule_t *s);
void print_schedule(struct schedule_t *s);

void set_transport_meter(int *measure, int *percent);
void write_sched_to_midi_file(struct schedule_t *sched, const char *filename);

#endif
