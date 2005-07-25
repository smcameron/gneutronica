#ifndef __MIDI_FILE_H__
#define __MIDI_FILE_H__

#include <sys/time.h>

static void write_weird_midi_int(int fd, unsigned int value);
int write_MThd(int fd);
int write_end_of_track(int fd);
int write_tempo_change(int fd, int microsecs_per_quarternote);
int write_MTrk(int fd);
void write_note(int fd, struct timeval *tm, unsigned char opcode, 
	unsigned char note, unsigned char velocity);

#endif
