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
#ifndef __GNEUTRONICA_H__
#define __GNEUTRONICA_H__

/* Here's some easily cnofigurable stuff, edit this at will */

	/* AUTOMAG_ON, set to 1 if you want automag on by default, 0 otherwise */
#define AUTOMAG_ON 1

	/* How much automag... 100.0 means none, 600.0 (max) means 6x */
#define DEFAULT_AUTOMAG 600.0

/* End of easily configurable stuff */

#ifdef INSTANTIATE_GNEUTRONICA_GLOBALS
#define GLOBAL
#define INIT(x, y) x = y
#else
#define GLOBAL extern
#define INIT(x, y) x
#endif

#define MAXMEASURES 1000
#define MAXPATTERNS 1000
#define MAXKITS 100
#define MAXINSTS 128
#define DRAW_WIDTH 600
#define DRAW_HEIGHT 25
#define MAGNIFIED_DRAW_HEIGHT 130 
#define MAXTIMEDIVS 5
#define MEASUREWIDTH 20 
#define MINTEMPO 10
#define MAXTEMPO 400
#define DEFAULT_VELOCITY 100
#define DRAGLIMIT 50.0
#define PROGNAME "Gneutronica"

GLOBAL struct schedule_t sched;

struct hit_struct {
	int pattern;
	int instrument_num;
	int drumkit;
	unsigned char velocity;
	double time; /* as a percentage of the measure */
	int beat;
	int beats_per_measure; /* this is a per-note value, used to position the note within a measure,
				/* and does not really reflect tempo information */
};

struct hitpattern {
		struct hit_struct h;
		struct hitpattern *next;
};

struct tempo_change_t {
	int measure;
	int beats_per_minute;
};

struct shared_info_struct {
	int measure;
	int percent;
	unsigned char midi_data[3];
};
GLOBAL unsigned char shared_buf[4096];

struct division_struct {
	int division;
	char *color;
	GtkWidget *spin; /* GtkSpinButton */
};

struct pattern_struct {
	char patname[40];
	struct hitpattern *hitpattern;
	struct division_struct timediv[MAXTIMEDIVS];
	int tracknum;
	int pattern_num;
	int beats_per_measure; /* this is tempo information for this pattern */
	int beats_per_minute; /* this is tempo information for this pattern for single pattern playback
				 Note: the same pattern may be played back several times in a song
			  	 at different tempos, so this is not the tempo within the context 
				of a song. */
	double drag[MAXINSTS];	/* amount of drag/rush as a percentage of a beat */
	int gm_converted;
	GtkWidget *copy_button;
	GtkWidget *del_button;
	GtkWidget *ins_button;
	GtkWidget *arr_button;
	GtkWidget *arr_darea; /* drawing area */
};

GLOBAL struct pattern_struct INIT(**pattern, NULL);

#define MAXPATSPERMEASURE 20
struct measure_struct {
	int npatterns;
	int pattern[MAXPATSPERMEASURE];
};

GLOBAL struct measure_struct INIT(*measure, NULL);

struct drumkit_struct {
	char make[30];
	char model[30];
	char name[30];
	int ninsts;
	unsigned char midi_channel;
	unsigned int midi_bank;
	unsigned char midi_patch;
	struct instrument_struct *instrument;
};

GLOBAL struct drumkit_struct drumkit[MAXKITS];

GLOBAL int song_gm_map[MAXINSTS];

struct instrument_struct {
	char name[40];
	char type[40];
	unsigned char midivalue;
	int instrument_num;
	int gm_equivalent; /* General MIDI equivalent */
	struct hitpattern *hit;
	GtkWidget *hidebutton;
	GtkWidget *button;
	GtkWidget *canvas;
	GtkObject *volume_adjustment;
	GtkWidget *volume_slider;
	GtkWidget *drag_spin_button;
	GtkWidget *clear_button;
	GtkWidget *name_entry;
	GtkWidget *type_entry;
	GtkWidget *midi_value_spin_button;
	GtkWidget *gm_value_spin_button;
}; 

/* commands to send player process */
#define PLAY_ONCE 0
#define PLAY_LOOP 1
#define PLAYER_QUIT 2
#define PERFORM_PATCH_CHANGE 4

GLOBAL int INIT(player_process_fd, -1);
GLOBAL int INIT(player_process_pid, -1);
GLOBAL int INIT(midi_reader_process_id, -1);

/* for communicating transport location from player process */
GLOBAL struct shared_info_struct INIT(*transport_location, NULL); 
GLOBAL gint measure_transport_tag; /* tag for cancelling idle function */
GLOBAL int INIT(pattern_play_mode, 0);

GLOBAL int INIT(changing_tempo_measure, -1);
GLOBAL int ntempochanges;
GLOBAL struct tempo_change_t tempo_change[MAXMEASURES];
GLOBAL struct tempo_change_t initial_change
#ifdef INSTANTIATE_GNEUTRONICA_GLOBALS
 = { 0, 120 }
#endif
;

GLOBAL struct division_struct timediv[] 
#ifdef INSTANTIATE_GNEUTRONICA_GLOBALS
= {
	{  4, "red", NULL },
	{ 16, "blue", NULL },
	{  0, "green4", NULL },
	{  0, "orange", NULL },
	{  0, "purple4", NULL },
}
#endif
;
GLOBAL int ndivisions
#ifdef INSTANTIATE_GNEUTRONICA_GLOBALS
 = sizeof(timediv) / sizeof(struct division_struct)
#endif
;

GLOBAL GtkWidget *tempolabel1, *tempolabel2, *tempospin1, *tempospin2;
GLOBAL GtkWidget *trackspin, *track_label;
GLOBAL GtkWidget *song_name_entry, *song_name_label;
GLOBAL GtkWidget *pattern_name_entry, *pattern_name_label;
GLOBAL GtkWidget *SaveBox;
GLOBAL GtkWidget *LoadBox;
GLOBAL GtkWidget *SaveDrumkitBox;
GLOBAL GtkWidget *ImportPatternsBox;
GLOBAL GtkWidget *ImportDrumtabBox;
GLOBAL GtkWidget *export_to_midi_box;
GLOBAL GtkWidget *midi_setup_activate_button;
GLOBAL GtkWidget *hide_instruments;
GLOBAL GtkWidget *hide_volume_sliders;
GLOBAL GtkWidget *snap_to_grid;
GLOBAL GtkWidget *drumkit_vbox;
GLOBAL GtkWidget *edit_instruments_toggle;
GLOBAL GtkWidget *save_drumkit_button;
GLOBAL GtkWidget *pattern_paste_button;
GLOBAL GtkWidget *pattern_select_button;
GLOBAL GtkWidget *pattern_metronome_chbox;
GLOBAL GtkWidget *pattern_loop_chbox;
GLOBAL GtkWidget *pattern_record_button;
GLOBAL GtkWidget *nextbutton, *prevbutton;
GLOBAL GtkWidget *play_button;
GLOBAL GtkWidget *play_selection_button;
GLOBAL GtkTooltips *tooltips;

GLOBAL GtkObject *volume_magnifier_adjustment;
GLOBAL GtkWidget *volume_magnifier;
GLOBAL GtkWidget *remap_drumkit_button;
GLOBAL GtkWidget *automag;
GLOBAL GtkWidget *autocrunch;

GLOBAL GtkWidget *remove_space_before_button;
GLOBAL GtkWidget *add_space_before_button;
GLOBAL GtkWidget *add_space_numerator_spin;
GLOBAL GtkWidget *add_space_denominator_spin;
GLOBAL GtkWidget *add_space_after_button;
GLOBAL GtkWidget *remove_space_after_button;

GLOBAL GtkWidget *TempoChWin;
GLOBAL GtkWidget *TempoChvbox1;
GLOBAL GtkWidget *TempoChhbox1;
GLOBAL GtkWidget *TempoChhbox2;
GLOBAL GtkWidget *TempoChOk;
GLOBAL GtkWidget *TempoChCancel;
GLOBAL GtkWidget *TempoChDelete;
GLOBAL GtkWidget *TempoChLabel;
GLOBAL GtkWidget *TempoChBPM;
GLOBAL char TempoChMsg[255];

GLOBAL int INIT(pattern_in_copy_buffer, -1);
GLOBAL int INIT(instrument_in_copy_buffer, -1);

GLOBAL int INIT(start_copy_measure,-1);
GLOBAL int INIT(end_copy_measure, -1);
GLOBAL int INIT(start_paint_measure,-1);

/* Widgets for the top part of the arranger table */
	/* Labels for the "buttons," below */
	GLOBAL GtkWidget *TempoLabel;
	GLOBAL GtkWidget *SelectButton;
	GLOBAL GtkWidget *PasteLabel;
	GLOBAL GtkWidget *InsertButton;
	GLOBAL GtkWidget *DeleteButton;
	GLOBAL GtkWidget *MeasureTransportLabel;

	/* drawing areas to hold the "buttons" for tempo/copy/paste/ins/del */ 
	GLOBAL GtkWidget *Tempo_da;
	GLOBAL GtkWidget *Copy_da;
	GLOBAL GtkWidget *Paste_da;
	GLOBAL GtkWidget *Insert_da;
	GLOBAL GtkWidget *Delete_da;
	GLOBAL GtkWidget *measure_transport_da;
	GLOBAL GtkWidget *arr_loop_check_button;
	GLOBAL GtkWidget *arr_factor_check_button;

#define PSCROLLER_HEIGHT 100
#define PSCROLLER_WIDTH (DRAW_WIDTH + 130) 
GLOBAL GtkWidget *pattern_scroller;
GLOBAL GtkWidget *top_window; /* Pattern editor window */
GLOBAL GtkWidget *arranger_window;
GLOBAL GtkWidget *notebook;
GLOBAL GtkWidget *arranger_box;
#define ARRANGER_COLS 5
#define ARRANGER_HEIGHT (MEASUREWIDTH) 
#define ARRANGER_WIDTH (MAXMEASURES * MEASUREWIDTH)
GLOBAL GtkWidget *arranger_scroller;
GLOBAL GtkWidget *arranger_table;

/* Midi setup window Allows seting Midi channel to transmit on,
   and allows sending patch change messages. */
GLOBAL GtkWidget *midi_setup_window;
GLOBAL GtkWidget *midi_setup_vbox;
GLOBAL GtkWidget *midi_setup_hbox1;
GLOBAL GtkWidget *midi_setup_hbox2;
GLOBAL GtkWidget *midi_setup_hbox3;
GLOBAL GtkWidget *midi_bank_label;
GLOBAL GtkWidget *midi_bank_spin_button;
GLOBAL GtkWidget *midi_patch_label;
GLOBAL GtkWidget *midi_patch_spin_button;
GLOBAL GtkWidget *midi_channel_label;
GLOBAL GtkWidget *midi_channel_spin_button;
GLOBAL GtkWidget *midi_change_patch_button;
GLOBAL GtkWidget *midi_setup_ok_button;
GLOBAL GtkWidget *midi_setup_cancel_button;

/* "About" stuff */
/* GLOBAL GtkWidget *about_button; */
GLOBAL GtkWidget INIT(*about_window, NULL);
GLOBAL GtkWidget *about_vbox;
GLOBAL GtkWidget *about_da;
GLOBAL GtkWidget *about_label;
GLOBAL GtkWidget *about_ok_button;
GLOBAL GdkPixbuf *robotdrummer;
GLOBAL GtkWidget *stop_button;

GLOBAL char window_title[255];
GLOBAL char arranger_title[255];
GLOBAL char songname[100];

GLOBAL GdkColor whitecolor;
GLOBAL GdkColor bluecolor;
GLOBAL GdkColor blackcolor;
GLOBAL GdkGC INIT(*gc, NULL);
GLOBAL int INIT(ndrumkits,  0);
GLOBAL int INIT(npatterns, 0);
GLOBAL int INIT(cmeasure, 0);
GLOBAL int INIT(cpattern, 0);
GLOBAL int INIT(kit, 0);
GLOBAL int INIT(nmeasures, 0);
GLOBAL int INIT(current_instrument,0);
GLOBAL int INIT(record_mode, 0);

GLOBAL void note_on(int fd, unsigned char value, unsigned char volume);
GLOBAL void note_off(int fd, unsigned char value, unsigned char volume);
GLOBAL void send_midi_patch_change(int fd, unsigned short bank, unsigned char patch);

GLOBAL void int_note_on(int fd, unsigned char value, unsigned char volume);
GLOBAL void int_note_off(int fd, unsigned char value, unsigned char volume);
GLOBAL void int_send_midi_patch_change(int fd, unsigned short bank, unsigned char patch);
GLOBAL struct pattern_struct *pattern_struct_alloc(int pattern_num);

GLOBAL int flatten_pattern(int ckit, int cpattern);
GLOBAL int unflatten_pattern(int ckit, int cpattern);

#define EXCLUDE_LIST_SIZE 2000
GLOBAL int INIT(n_exclude_keypress_widgets, 0);
GLOBAL GtkWidget *exclude_keypress_list[EXCLUDE_LIST_SIZE];

#undef GLOBAL
#undef INIT
#endif /* __GNEUTRONICA_H__ */

