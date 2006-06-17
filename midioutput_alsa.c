/*
    (C) Copyright 2006 Stephen M. Cameron.

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>


#include "midioutput.h"
#define INSTANTIATE_MIDIOUTPUT_ALSA_GLOBALS
#include "midioutput_alsa.h"

struct midi_handle_alsa {
	snd_seq_t *seqp; /* alsa sequencer port */
	int outputport;
};

void midi_close_alsa(struct midi_handle *mh)
{
	printf("%s:%s:%s, not yet implemented\n",
		 __FILE__, __LINE__, __FUNCTION__);
}

struct midi_handle *midi_open_alsa(unsigned char *name)
{
	int rc;
	struct midi_handle_alsa *mh;
	unsigned char clientname[255], portname[255];

	sprintf(clientname, "Gneutronica (%d)", getpid());

	mh = (struct midi_handle_alsa *) malloc(sizeof(*mh));
	if (mh == NULL)
		return NULL;

	rc = snd_seq_open(&mh->seqp, name, SND_SEQ_OPEN_OUTPUT, 0666);
	if (rc < 0) {
		printf("snd_seq_open returns %d\n", rc);
		free(mh);
		return NULL;
	}
	rc = snd_seq_set_client_name(mh->seqp, clientname);
	if (rc < 0)
		printf("snd_seq_set_client_name failed \n");
	sprintf(portname, "Gneutronica output (%d:%d)", getpid(), 0);
	mh->outputport = snd_seq_create_simple_port(mh->seqp, portname,
                        SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                        SND_SEQ_PORT_TYPE_MIDI_GENERIC);
	if (mh->outputport < 0)
		printf("snd_seq_create_simple_port failed\n");
	return (struct midi_handle *) mh;
}

void midi_noteon_alsa(struct midi_handle *mh,
	unsigned char channel,
	unsigned char value,
	unsigned char volume)
{
	struct midi_handle_alsa *mha = (struct midi_handle_alsa *) mh;
	snd_seq_event_t ev;
	struct snd_seq_real_time tstamp;

	memset(&tstamp, 0, sizeof(tstamp));
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, mha->outputport);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	/* snd_seq_ev_schedule_tick(&ev,  SND_SEQ_EVENT_PORT_SUBSCRIBED, 1, 0); */
	snd_seq_ev_schedule_real(&ev,  SND_SEQ_EVENT_PORT_SUBSCRIBED, 1, &tstamp);
	ev.type = SND_SEQ_EVENT_NOTEON;
	ev.data.note.channel = 0;
	ev.data.note.note = value;
	ev.data.note.velocity = volume;
	ev.data.note.off_velocity = 0;
	ev.data.note.duration = 100; /* it's drums... there is no note off. */

	printf("Sending event to port %d, note=%d, vel=%d, pid=%d\n",
		mha->outputport, ev.data.note.note, ev.data.note.velocity, getpid());
        snd_seq_event_output(mha->seqp, &ev);
	return;
}

void midi_patch_change_alsa(struct midi_handle *mh,
	unsigned char channel,
	unsigned short bank,
	unsigned char patch)
{
	printf("%s:%d, not yet implemented\n", __FILE__, __LINE__);
	return;
}

void midi_noteoff_alsa(struct midi_handle *mh, unsigned char channel, unsigned char value)
{
	midi_noteon_alsa(mh, channel, value, 0);
	return;
}

int midi_isopen_alsa(struct midi_handle *mh)
{
	struct midi_handle_alsa *mha = (struct midi_handle_alsa *) mh;
	if (mha == NULL)
		return 0;
	return 1;
}
