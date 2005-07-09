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
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <setjmp.h>
#include <netinet/in.h> /* . . . just for for htons() */

#include <gtk/gtk.h>

#include "sched.h"

#define MAXMEASURES 1000
#define MAXPATTERNS 1000
#define MAXKITS 100
#define MAXINSTS 128
#define DRAW_WIDTH 600
#define DRAW_HEIGHT 33 
#define MAXTIMEDIVS 5
#define MEASUREWIDTH 20
#define MINTEMPO 10
#define MAXTEMPO 400
#define DEFAULT_VELOCITY 100

#define PROGNAME "Gneutronica"
#define VERSION "0.21"

struct schedule_t sched;

/* commands to send player process */
#define PLAY_ONCE 0
#define PLAY_LOOP 1
#define PLAYER_QUIT 2
#define PERFORM_PATCH_CHANGE 4

int midi_fd = -1;
int player_process_fd = -1;
int player_process_pid = -1;

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

int changing_tempo_measure = -1;
int ntempochanges;
struct tempo_change_t tempo_change[MAXMEASURES];
struct tempo_change_t initial_change = { 0, 120 };

struct division_struct {
	int division;
	char *color;
	GtkWidget *spin; /* GtkSpinButton */
} timediv[] = {
	{  4, "red", NULL },
	{ 16, "blue", NULL },
	{  0, "green4", NULL },
	{  0, "orange", NULL },
	{  0, "purple4", NULL },
};

GtkWidget *tempolabel1, *tempolabel2, *tempospin1, *tempospin2;
GtkWidget *song_name_entry, *song_name_label;
GtkWidget *pattern_name_entry, *pattern_name_label;
GtkWidget *SaveBox;
GtkWidget *LoadBox;
GtkWidget *SaveDrumkitBox;
GtkWidget *quitbutton;
GtkWidget *midi_setup_activate_button;
GtkWidget *hide_instruments;
GtkWidget *hide_volume_sliders;
GtkWidget *drumkit_vbox;
GtkWidget *edit_instruments_toggle;
GtkWidget *save_drumkit_button;
GtkWidget *pattern_paste_button;
GtkWidget *nextbutton, *prevbutton;
GtkWidget *play_button;
GtkWidget *play_selection_button;
GtkTooltips *tooltips;

GtkWidget *TempoChWin;
GtkWidget *TempoChvbox1;
GtkWidget *TempoChhbox1;
GtkWidget *TempoChhbox2;
GtkWidget *TempoChOk;
GtkWidget *TempoChCancel;
GtkWidget *TempoChDelete;
GtkWidget *TempoChLabel;
GtkWidget *TempoChBPM;
char TempoChMsg[255];

int pattern_in_copy_buffer = -1;

int start_copy_measure = -1;
int end_copy_measure = -1;

/* Widgets for the top part of the arranger table */
	/* Labels for the "buttons," below */
	GtkWidget *TempoLabel;
	GtkWidget *SelectButton;
	GtkWidget *PasteLabel;
	GtkWidget *InsertButton;
	GtkWidget *DeleteButton;

	/* drawing areas to hold the "buttons" for tempo/copy/paste/ins/del */ 
	GtkWidget *Tempo_da;
	GtkWidget *Copy_da;
	GtkWidget *Paste_da;
	GtkWidget *Insert_da;
	GtkWidget *Delete_da;
	GtkWidget *arr_loop_check_button;


#define PSCROLLER_HEIGHT 100
#define PSCROLLER_WIDTH (DRAW_WIDTH + 130) 
GtkWidget *pattern_scroller;
GtkWidget *window; /* Pattern editor window */
GtkWidget *arranger_window;
GtkWidget *arranger_box;
#define ARRANGER_COLS 5
#define ARRANGER_HEIGHT (MEASUREWIDTH) 
#define ARRANGER_WIDTH (MAXMEASURES * MEASUREWIDTH)
GtkWidget *arranger_scroller;
GtkWidget *arranger_table;

/* Midi setup window Allows seting Midi channel to transmit on,
   and allows sending patch change messages. */
GtkWidget *midi_setup_window;
GtkWidget *midi_setup_vbox;
GtkWidget *midi_setup_hbox1;
GtkWidget *midi_setup_hbox2;
GtkWidget *midi_setup_hbox3;
GtkWidget *midi_bank_label;
GtkWidget *midi_bank_spin_button;
GtkWidget *midi_patch_label;
GtkWidget *midi_patch_spin_button;
GtkWidget *midi_channel_label;
GtkWidget *midi_channel_spin_button;
GtkWidget *midi_change_patch_button;
GtkWidget *midi_setup_ok_button;
GtkWidget *midi_setup_cancel_button;

char window_title[255];
char arranger_title[255];
char songname[100];

struct pattern_struct {
	char patname[40];
	struct hitpattern *hitpattern;
	struct division_struct timediv[MAXTIMEDIVS];
	int pattern_num;
	int beats_per_measure; /* this is tempo information for this pattern */
	int beats_per_minute; /* this is tempo information for this pattern for single pattern playback
				 Note: the same pattern may be played back several times in a song
			  	 at different tempos, so this is not the tempo within the context 
				of a song. */
	/* put stuff for drag/rush of instruments in here in the pattern.   Cmplicate it
	   by the (reasonable) presumption it will be sparsely populated?  Or should I
	   just brute force it and waste space? e.g. "double drag[MAXINSTS];" */

	GtkWidget *copy_button;
	GtkWidget *del_button;
	GtkWidget *ins_button;
	GtkWidget *arr_button;
	GtkWidget *arr_darea; /* drawing area */
} **pattern = NULL;

int ndivisions = sizeof(timediv) / sizeof(struct division_struct);

#define MAXPATSPERMEASURE 20
struct measure_struct {
	int npatterns;
	int pattern[MAXPATSPERMEASURE];
} *measure = NULL;

GdkColor whitecolor;
GdkColor bluecolor;
GdkColor blackcolor;
GdkGC *gc = NULL;
int ndrumkits = 0;
int npatterns = 0;
int cmeasure = 0;
int cpattern = 0;
int kit = 0;
int nmeasures = 0;

struct drumkit_struct {
	char make[30];
	char model[30];
	char name[30];
	int ninsts;
	unsigned char midi_channel;
	unsigned int midi_bank;
	unsigned char midi_patch;
	struct instrument_struct *instrument;
} drumkit[MAXKITS];

struct instrument_struct {
	char name[40];
	char type[40];
	unsigned char midivalue;
	int instrument_num;
	struct hitpattern *hit;
	GtkWidget *hidebutton;
	GtkWidget *button;
	GtkWidget *canvas;
	GtkObject *volume_adjustment;
	GtkWidget *volume_slider;
	GtkWidget *drag_spin_button;
	GtkWidget *name_entry;
	GtkWidget *type_entry;
	GtkWidget *midi_value_spin_button;
}; 

void note_on(int fd, unsigned char value, unsigned char volume);
void note_off(int fd, unsigned char value, unsigned char volume);
void send_midi_patch_change(int fd, unsigned short bank, unsigned char patch);

void int_note_on(int fd, unsigned char value, unsigned char volume);
void int_note_off(int fd, unsigned char value, unsigned char volume);
void int_send_midi_patch_change(int fd, unsigned short bank, unsigned char patch);

#define EXTERNAL_DEVICE 0
#define INTERNAL_DEVICE 1

struct device_access_method {
	void (*note_on)(int fd, unsigned char value, unsigned char volume);
	void (*note_off)(int fd, unsigned char value, unsigned char volume);
	void (*send_midi_patch_change)(int fd, unsigned short bank, unsigned char patch);
} access_method[] = { 
	{ note_on, note_off, send_midi_patch_change }, /* external MIDI device */
	{ int_note_on, int_note_off, int_send_midi_patch_change }, /* internal MIDI device */
};

struct device_access_method *access_device = &access_method[EXTERNAL_DEVICE];

int flatten_pattern(int ckit, int cpattern);
int unflatten_pattern(int ckit, int cpattern);

int read_drumkit_fileformat_1(char *filename, FILE *f, int *ndrumkits, struct drumkit_struct *drumkit)
{
	struct drumkit_struct *dk;
	int rc, line, n;
	char cmd[255];

	line = 1;
	dk = &drumkit[*ndrumkits];
	dk->ninsts = 0;
	rc = fscanf(f, "%[^,]%*c %[^,]%*c %[^\n]%*c", dk->make, dk->model, dk->name);
	/* g_print("rc = %d\n", rc); */
	if (rc != 3) {
		fprintf(stderr, "Error in %s at line %d\n", filename, line);
		pclose(f);
		return -1;
	}

	dk->instrument = malloc(sizeof(struct instrument_struct)*MAXINSTS);
	if (dk->instrument == NULL) {
		fprintf(stderr, "Out of memory\n");
		pclose(f);
		return -1;
	}
	memset(dk->instrument, 0, sizeof(struct instrument_struct)*MAXINSTS);

	n = 0;
	line++;
	while (!feof(f)) {
		rc = fscanf(f, "%[^,]%*c %[^,]%*c %d\n", 
			dk->instrument[n].name, 
			dk->instrument[n].type,
			&dk->instrument[n].midivalue);
		if (rc != 3) {
			fprintf(stderr, "Error in %s at line %d\n", filename, line);
			pclose(f);
			return -1;
		}
		dk->instrument[n].instrument_num = n;
		n++;
		line++;
		dk->instrument[n].hit = NULL;
		dk->instrument[n].button = NULL;
		dk->instrument[n].hidebutton = NULL;
		dk->instrument[n].canvas = NULL;
	}
	dk->ninsts = n;
	*ndrumkits++;
	pclose(f);
	return 0;
}

int read_drumkit(char *filename, int *ndrumkits, struct drumkit_struct *drumkit)
{
	FILE *f;
	struct drumkit_struct *dk;
	int rc, line, n;
	int fileformat;
	char cmd[255];

	if (*ndrumkits >= MAXKITS)
		return -1;

	sprintf(cmd, "grep -v '^#' %s", filename);
	f = popen(cmd, "r");
	if (f == NULL) {
		fprintf(stderr, "Can't open %s: %s\n", filename, strerror(errno));
		return -1;
	}
	rc = fscanf(f, "Gneutronica drumkit file format %d\n", &fileformat);
	if (rc != 1) {
		printf("%s does not appear to be a Gneutronica drumkit file\n", filename);
		pclose(f);
		return -1;
	}

	switch (fileformat) {
	case 1: rc = read_drumkit_fileformat_1(filename, f, ndrumkits, drumkit);
		break;
	default: printf("Unknown drumkit fileformat version %d\n", fileformat);
		rc = -1;
		break;
	}
	return rc;
}

save_drumkit_to_file(const char *filename) 
{
	struct drumkit_struct *dk;
	FILE *f;
	int i, fileformat = 1;

	printf("save_drumkit_to_file called\n");

	dk = &drumkit[kit];

	f = fopen(filename, "w");
	if (f == NULL) 
		return -1;

	fprintf(f, "Gneutronica drumkit file format %d\n", fileformat);
	fprintf(f, "%s, %s, %s\n", dk->make, dk->model, dk->name);
	for (i=0;i<dk->ninsts;i++) {
		fprintf(f, "%s, %s, %d\n",  
			dk->instrument[i].name, 
			dk->instrument[i].type,
			dk->instrument[i].midivalue);
	}
	fclose(f);
}

void destroy_event(GtkWidget *widget, gpointer data);
void cleanup_tempo_changes();

void tempo_change_ok_button(GtkWidget *widget, gpointer data)
{
	int new_tempo;
	printf("Tempo change ok button\n");
	new_tempo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(TempoChBPM));
	insert_tempo_change(changing_tempo_measure, new_tempo);
	cleanup_tempo_changes();
	gtk_widget_queue_draw(Tempo_da);
	gtk_widget_hide(TempoChWin);
}

void tempo_change_cancel_button(GtkWidget *widget, gpointer data)
{
	printf("Tempo change cancel button\n");
	cleanup_tempo_changes();
	gtk_widget_hide(TempoChWin);
	gtk_widget_queue_draw(Tempo_da);
}

void tempo_change_delete_button(GtkWidget *widget, gpointer data)
{
	int index;
	printf("Tempo change delete button\n");
	index = insert_tempo_change(changing_tempo_measure, -1);
	if (index > 0)
		tempo_change[index].beats_per_minute = tempo_change[index-1].beats_per_minute;
	cleanup_tempo_changes();
	gtk_widget_hide(TempoChWin);
	gtk_widget_queue_draw(Tempo_da);
}

void destroy_means_hide(GtkWidget *widget, gpointer data)
{
	/* hmm, this doesn't really work, the next time the widget is activated... boom! */
	gtk_widget_hide(widget); 
}


void make_tempo_change_editor()
{
	TempoChWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	TempoChvbox1 = gtk_vbox_new(FALSE, 0);
	TempoChhbox1 = gtk_hbox_new(FALSE, 0);
	TempoChhbox2 = gtk_hbox_new(FALSE, 0);

	TempoChLabel = gtk_label_new("Measure:xxx Beats/Minute:");
	TempoChBPM = gtk_spin_button_new_with_range(MINTEMPO, MAXTEMPO, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(TempoChBPM), (gdouble) 120);

	TempoChOk = gtk_button_new_with_label("Ok");
	g_signal_connect(G_OBJECT (TempoChOk), "clicked",
			G_CALLBACK (tempo_change_ok_button), NULL);
	TempoChCancel = gtk_button_new_with_label("Cancel");
	g_signal_connect(G_OBJECT (TempoChCancel), "clicked",
			G_CALLBACK (tempo_change_cancel_button), NULL);
	TempoChDelete = gtk_button_new_with_label("Delete tempo change");
	g_signal_connect(G_OBJECT (TempoChDelete), "clicked",
			G_CALLBACK (tempo_change_delete_button), NULL);

	g_signal_connect(G_OBJECT (TempoChWin), "delete_event", 
		// G_CALLBACK (delete_event), NULL);
		G_CALLBACK (destroy_means_hide), NULL);
	g_signal_connect(G_OBJECT (TempoChWin), "destroy", 
		G_CALLBACK (destroy_means_hide), NULL);

	gtk_container_set_border_width(GTK_CONTAINER (TempoChWin), 15);
	gtk_container_add(GTK_CONTAINER (TempoChWin), TempoChvbox1);
	gtk_box_pack_start(GTK_BOX(TempoChvbox1), TempoChhbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(TempoChvbox1), TempoChhbox2, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(TempoChhbox1), TempoChLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(TempoChhbox1), TempoChBPM, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(TempoChhbox2), TempoChOk, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(TempoChhbox2), TempoChCancel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(TempoChhbox2), TempoChDelete, TRUE, TRUE, 0);

	gtk_widget_show(TempoChOk);
	gtk_widget_show(TempoChCancel);
	gtk_widget_show(TempoChDelete);
	gtk_widget_show(TempoChhbox2);

	gtk_widget_show(TempoChLabel);
	gtk_widget_show(TempoChBPM);
	gtk_widget_show(TempoChhbox1);
	gtk_widget_show(TempoChvbox1);
	// gtk_widget_show(TempoChWin);
}

static int arr_darea_clicked(GtkWidget *w, GdkEventButton *event, 
		struct pattern_struct *data);
static int arr_darea_event(GtkWidget *w, GdkEvent *event, struct pattern_struct *p);
static int arr_darea_button_press(GtkWidget *w, GdkEventButton *event, 
		struct pattern_struct *data);

void free_pattern(struct pattern_struct *p)
{
	if (p == NULL)
		return;
	if (p->arr_darea != NULL) {
		gtk_widget_destroy(p->arr_darea);
		// gtk_object_unref(GTK_OBJECT(p->arr_darea));
	}
	if (p->arr_button != NULL)
		gtk_widget_destroy(GTK_WIDGET(p->arr_button));
	if (p->copy_button != NULL)
		gtk_widget_destroy(GTK_WIDGET(p->copy_button));
	if (p->del_button != NULL)
		gtk_widget_destroy(GTK_WIDGET(p->del_button));
	if (p->ins_button != NULL); 
		gtk_widget_destroy(GTK_WIDGET(p->ins_button));
		// gtk_object_unref(GTK_OBJECT(p->arr_button));
	free(p);
}

void edit_pattern_clicked(GtkWidget *widget,
	struct pattern_struct *data);
void ins_pattern_button_pressed(GtkWidget *widget,
	struct pattern_struct *data);
void del_pattern_button_pressed(GtkWidget *widget,
	struct pattern_struct *data);
void copy_pattern_button_pressed(GtkWidget *widget,
	struct pattern_struct *data);

void make_new_pattern_widgets(int new_pattern, int total_rows)
{
	struct pattern_struct *p = pattern[new_pattern];
	int top;
	int new_arranger_scroller_size;
	char msg[100];

	if (p == NULL)
		return;

	total_rows = total_rows + 5;  /* tempo/del/ins/copy/paste/ */
	top = total_rows -1;

	p->arr_button = gtk_button_new_with_label(p->patname);
	gtk_tooltips_set_tip(tooltips, p->arr_button, "Edit this pattern.  Click the boxes to the right to assign this pattern to measures.", NULL);
	p->copy_button = gtk_button_new_with_label("Sel");
	sprintf(msg, "Select this pattern (%s) for later pasting into another pattern", p->patname);
	gtk_tooltips_set_tip(tooltips, p->copy_button, msg, NULL);
	p->ins_button = gtk_button_new_with_label("Ins");
	gtk_tooltips_set_tip(tooltips, p->ins_button, 
		"Insert new pattern before this pattern.  "
		"Not yet implemented.  If you really must insert a "
		"pattern, save the song and use vi or another text editor "
		"to create a new empty pattern where you want it.  "
		"Make a backup first, of course.", NULL);
	p->del_button = gtk_button_new_with_label("Del");
	gtk_tooltips_set_tip(tooltips, p->del_button, 
		"Delete this pattern.  Not yet implemented.  "
		"If you really must delete a pattern, save the "
		"song, and use vi or another text editor to "
		"delete the pattern.  Make a backup first, of "
		"course.", NULL);
	p->arr_darea = gtk_drawing_area_new();
	gtk_tooltips_set_tip(tooltips, p->arr_darea, "Click to assign patterns to measures.", NULL);
	g_signal_connect(G_OBJECT (p->arr_darea), "expose_event",
			G_CALLBACK (arr_darea_event), p);
	gtk_widget_add_events(p->arr_darea, GDK_BUTTON_PRESS_MASK); 
	gtk_widget_add_events(p->arr_darea, GDK_BUTTON_RELEASE_MASK); 
	gtk_widget_add_events(p->arr_darea, GDK_DRAG_ENTER); 
	gtk_widget_add_events(p->arr_darea, GDK_DRAG_LEAVE); 
	//g_signal_connect(G_OBJECT (p->arr_darea), "button-release-event",
	g_signal_connect(G_OBJECT (p->arr_darea), "button-release-event",
			G_CALLBACK (arr_darea_clicked), p);
	g_signal_connect(G_OBJECT (p->arr_darea), "button-press-event",
			G_CALLBACK (arr_darea_button_press), p);
	gtk_widget_set_size_request(p->arr_darea, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	new_arranger_scroller_size = (total_rows+2) * ARRANGER_HEIGHT;
	if (new_arranger_scroller_size > 200)
		new_arranger_scroller_size = 200;
	gtk_widget_set_size_request (arranger_scroller, 750, new_arranger_scroller_size);
	gtk_table_resize(GTK_TABLE(arranger_table), total_rows, ARRANGER_COLS);

	/* How do I move the things in the table around?  For now,
		just add at the end. */
	gtk_table_attach(GTK_TABLE(arranger_table), p->ins_button, 
		0, 1, top, top+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->del_button, 
		1, 2, top, top+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->copy_button, 
		2, 3, top, top+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->arr_button, 
		3, 4, top, top+1, GTK_FILL, 0, 0, 0);
	g_signal_connect(G_OBJECT (p->arr_button), "clicked",
			G_CALLBACK (edit_pattern_clicked), p);
		// 0, 1, top, top+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->arr_darea, 
		4, 5, top, top+1, 0,0,0,0);
	g_signal_connect(G_OBJECT (p->ins_button), "clicked",
			G_CALLBACK (ins_pattern_button_pressed), p);
	g_signal_connect(G_OBJECT (p->del_button), "clicked",
			G_CALLBACK (del_pattern_button_pressed), p);
	g_signal_connect(G_OBJECT (p->copy_button), "clicked",
			G_CALLBACK (copy_pattern_button_pressed), p);
	gtk_widget_show(p->ins_button);
	gtk_widget_show(p->del_button);
	gtk_widget_show(p->copy_button);
	gtk_widget_show(p->arr_button);
	gtk_widget_show(p->arr_darea);
	gtk_widget_show_all(arranger_window);
	gtk_widget_queue_draw(arranger_table);
	return;
}

int gcd(int n,int d)
{
	/* Euclid's algorithm for greatest common divisor */
	int t;
	while (d > 0) {
		if (n > d) {
			t = n;
			n = d;
			d = t;
		}
		d = d-n;
	}
	return n;
}

int reduce_fraction(int *numerator, int *denominator)
{
	int xgcd;

	if (*denominator == 0 || *numerator == 0 || *numerator == 1)
		return;
	xgcd = gcd (*numerator, *denominator);
	*numerator = *numerator / xgcd;
	*denominator = *denominator / xgcd;
}

int lowlevel_add_hit(struct hitpattern **hit,
		int dkit, int pattern, int instnum, int beat, int beats_per_measure, 
		unsigned char velocity, int change_velocity)
{
	struct hitpattern *prev, *this, *next;
	double percent;

	percent = (double) beat / (double) beats_per_measure;

	/* g_print("Adding hit to %s at beat %d of %d beats, percent = %g\n",
		drumkit[dkit].instrument[instnum].name, 
		beat+1, beats_per_measure, percent); */

	/* adding to the very beginning */
	if (*hit == NULL) {
		this = (struct hitpattern *) malloc(sizeof(struct hitpattern));
		this->next = NULL;
		this->h.velocity = velocity;
		this->h.time = percent; /* as a percentage of the measure */
		this->h.beat = beat;
		this->h.beats_per_measure = beats_per_measure;
		this->h.drumkit = dkit;
		this->h.pattern = pattern;
		this->h.instrument_num = instnum;
		*hit = this;
		return 0;
	}

	/* First, see if this hit is already there... */
	for (this = *hit; this != NULL; this = this->next) {
		if (this->h.beat == beat && 
			this->h.beats_per_measure == beats_per_measure &&
			this->h.instrument_num == instnum &&
			this->h.drumkit == dkit) {
			// printf("lowlevel add hit rejecting duplicate\n");
			if (change_velocity) { /* just a velocity change */
				this->h.velocity = velocity;
				return 0;
			} else
				return -2;
		}
	}

	/* search through... */
	prev = NULL;
	for (this = *hit; this != NULL; this = this->next) {
		if (this->h.time > percent) {
			/* add somewhere in the middle */
			next = this;
			this = (struct hitpattern *) malloc(sizeof(struct hitpattern));
			this->next = next;
			this->h.velocity = velocity;
			this->h.time = percent; /* as a percentage of the measure */
			this->h.beat = beat;
			this->h.beats_per_measure = beats_per_measure;
			this->h.drumkit = dkit;
			this->h.pattern = pattern;
			this->h.instrument_num = instnum;
			if (prev == NULL)
				*hit = this;
			else
				prev->next = this;
			return 0;
		}
		prev = this;
	}
	/* must be adding to the end. */
	this = (struct hitpattern *) malloc(sizeof(struct hitpattern));
	this->next = NULL;
	this->h.velocity = velocity;
	this->h.time = percent; /* as a percentage of the measure */
	this->h.beat = beat;
	this->h.beats_per_measure = beats_per_measure;
	this->h.drumkit = dkit;
	this->h.pattern = pattern;
	this->h.instrument_num = instnum;
	prev->next = this;
	return 0;
}

void remove_hit(struct hitpattern **hit, 
		double thetime, double measurelength, 
		int dkit, int instnum, int pattern)
{

	struct hitpattern *prev, *this, *next, *matchprev, *matchnext;
	
	double distance, mindist,  percent;

	percent = thetime / measurelength;
	matchprev = prev = NULL;
	matchnext = next = NULL;
	distance = mindist = DRAW_WIDTH * 2.0; 
	for (this = *hit; this != NULL; this = next) {
		next = this->next;
		distance = this->h.time * measurelength - percent * measurelength;
		if (distance < 0) 
			distance = -distance;
		if (distance < mindist) {
			mindist = distance;
			matchprev = prev;
			matchnext = next;
		}
		prev = this;
	}
	if (mindist < 15.0) {
		if (matchprev == NULL) {
			this = *hit;
			*hit = this->next;
			free(this);
			return;
		} else {
			this = matchprev->next;
			if (this == NULL) 
				return;
			matchprev->next = this->next;
			free(this);
		}
	}
}


int add_hit(struct hitpattern **hit, 
		double thetime, double measurelength, 
		int dkit, int instnum, int pattern, unsigned char velocity, 
		int change_velocity)
{
	/* figure out the nearest place */
	int bestbeat = -1;
	int best_division;
	int found, i;
	double percent, bestpercent, diff, bestdiff;
	double target, divlen, x;
	double zero;

	struct hitpattern *prev, *this, *next;

	if (thetime > measurelength || thetime < 0)
		return;

	percent = thetime / measurelength;
	
	/* g_print("percent = %g\n", percent); */

	bestdiff = 1000.0;

	for (i=0;i<ndivisions;i++) {
		zero =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(timediv[i].spin));
		if (zero < 0.0)
			zero = -zero;
		if (zero < 0.00001)
			continue;

		divlen = measurelength / zero; 
		x = percent * measurelength / divlen;
		/* g_print("x = %g, == %g * %g / %g, zero = %G\n", percent, measurelength, divlen, zero);
		g_print("x = %g\n", x); */
		if (x - trunc(x) > 0.5) {
			target = (trunc(x) + 1.0);
			diff = (target - x) * divlen;
		} else {
			target = trunc(x);
			diff = (x - target) * divlen;
		}
		/* g_print("%d: diff = %g, target= %g, x = %g\n", i, diff, target,  x); */
		if (i == 0 || diff < bestdiff) {
			bestdiff = diff;
			bestbeat = (int) target;
			best_division = (int) zero;
			bestpercent = (target / zero);
			/* g_print("i=%d, bestdiff = %g\n", i, bestdiff); */
		}
	}
	if (bestbeat == -1) {
		g_print("No best beat found\n");
		return;
	}

	reduce_fraction(&bestbeat, &best_division);

	/* If there are 16 beats, that's 0 thru 15, beat 16 belongs to the next measure... */
	if (bestbeat == best_division) {
		g_print("Rejecting beat %d of %d beats\n",
			bestbeat+1, best_division);
		return;
	}
	return (lowlevel_add_hit(hit, dkit, pattern, instnum, bestbeat, best_division, 
			velocity, change_velocity));
}


void timediv_spin_change(GtkSpinButton *spinbutton, struct division_struct *data)
{
	int i;
	/* Make the timing lines redraw . . . */
	for (i=0;i<drumkit[kit].ninsts;i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
}

static int arr_darea_button_press(GtkWidget *w, GdkEventButton *event, 
		struct pattern_struct *data)
{
	int m, i;
	m = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
}

void redraw_measure_op_buttons()
{
	/* I should really make a measure_ops_da[] array, or something . . .  */
	gtk_widget_queue_draw(Tempo_da);
	gtk_widget_queue_draw(Copy_da);
	gtk_widget_queue_draw(Paste_da);
	gtk_widget_queue_draw(Insert_da);
	gtk_widget_queue_draw(Delete_da);
}

void integer_spin_button_change(GtkSpinButton *spinbutton, int *value)
{
	*value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
}

void unsigned_char_spin_button_change(GtkSpinButton *spinbutton, unsigned char *value)
{
	*value = (unsigned char) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
}

redraw_arranger()
{
	int i;
	for (i=0;i<npatterns;i++) {
		gtk_widget_queue_draw(GTK_WIDGET(pattern[i]->arr_darea));
	}
	redraw_measure_op_buttons();
}

void insert_measures_button(GtkWidget *widget, gpointer data)
{
	int measures_to_copy, measures_to_move;
	struct measure_struct *tmp;
	int bytes_to_copy, bytes_to_move;
	int m = start_copy_measure;

	// printf("Insert button clicked\n", m);
	if (start_copy_measure <= -1 || end_copy_measure < start_copy_measure)
		return;

	measures_to_copy = end_copy_measure - start_copy_measure + 1;
	if (nmeasures + measures_to_copy > MAXMEASURES)
		measures_to_copy = MAXMEASURES - nmeasures; /* copy what we can */
	bytes_to_copy = sizeof(struct measure_struct) * measures_to_copy;

	measures_to_move = nmeasures - m + 1;
	bytes_to_move = measures_to_move * sizeof(struct measure_struct);
#if 0
	/* Copy to a temporary buffer first, to deal with overlaps, etc. */
	tmp = malloc(bytes_to_copy);
	if (tmp == NULL)
		return;
	memcpy(tmp, &measure[start_copy_measure], bytes_to_copy); 
#endif
	/* Make room, mmmove handles overlapping regions . . . */
	memmove(&measure[m+measures_to_copy], &measure[m], bytes_to_move);
	/* Paste in the blank measures */
	memset(&measure[m], 0, bytes_to_copy);
	// free(tmp);
	nmeasures += measures_to_copy;
	redraw_arranger();
}

void delete_measures_button(GtkWidget *widget, gpointer data)
{
	int i, count;
	if (nmeasures <= 0)
		return;
	if (start_copy_measure < 0 || 
		end_copy_measure < 0 || 
		end_copy_measure < start_copy_measure)
		return;
	count = end_copy_measure - start_copy_measure + 1;
	if (count >= nmeasures) {
		memset(&measure[0], 0, nmeasures);
		nmeasures = 0;
	} else {
		for (i=start_copy_measure;i<nmeasures-count;i++)
			measure[i] = measure[i+count];
	}
	nmeasures -= count;
	start_copy_measure = end_copy_measure = -1;
	redraw_arranger();
	return;
}

void select_measures_button(GtkWidget *widget, gpointer data)
{
	if (start_copy_measure > -1)
		start_copy_measure = end_copy_measure = -1;
	else {
		start_copy_measure = 0;
		end_copy_measure = nmeasures -1;
	}
	redraw_arranger();
}

static int measure_da_clicked(GtkWidget *w, GdkEventButton *event, 
		gpointer data)
{
	int m, i;
	m = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
	if (m > nmeasures)
		return TRUE;
	if (m == nmeasures && w != Paste_da)
		return TRUE;
	if (w == Tempo_da) {
		int index;
		changing_tempo_measure = m;
		index = insert_tempo_change(m, -1);
		printf("index = %d, tempo = %d\n", index, tempo_change[index].beats_per_minute);
		sprintf(TempoChMsg, "Measure:%d Beats/Minute:", m);
		gtk_label_set_text(GTK_LABEL(TempoChLabel), TempoChMsg);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(TempoChBPM), 
			(gdouble) tempo_change[index].beats_per_minute);
		gtk_widget_queue_draw(GTK_WIDGET(TempoChLabel));
		gtk_widget_queue_draw(GTK_WIDGET(TempoChBPM));
		gtk_widget_show(TempoChWin);
	} else if (w == Copy_da) { 
		/* measure_in_copy_buffer = m;
		redraw_arranger(); */
	} else if (w == Paste_da) { 
		int measures_to_copy, measures_to_move;
		struct measure_struct *tmp;
		int bytes_to_copy, bytes_to_move;

		if (start_copy_measure <= -1 || end_copy_measure < start_copy_measure)
			return;

		measures_to_copy = end_copy_measure - start_copy_measure + 1;
		if (nmeasures + measures_to_copy > MAXMEASURES)
			measures_to_copy = MAXMEASURES - nmeasures; /* copy what we can */
		bytes_to_copy = sizeof(struct measure_struct) * measures_to_copy;

		measures_to_move = nmeasures - m + 1;
		bytes_to_move = measures_to_move * sizeof(struct measure_struct);
		/* Copy to a temporary buffer first, to deal with overlaps, etc. */
		tmp = malloc(bytes_to_copy);
		if (tmp == NULL)
			return;
		memcpy(tmp, &measure[start_copy_measure], bytes_to_copy); 
		/* Make room, mmmove handles overlapping regions . . . */
		memmove(&measure[m+measures_to_copy], &measure[m], bytes_to_move);
		/* Paste in the measures */
		memcpy(&measure[m], tmp, bytes_to_copy);
		free(tmp);
		nmeasures += measures_to_copy;
		redraw_arranger();
	} else if (w == Insert_da) { 
		int i;
		if (nmeasures >= MAXMEASURES)
			return TRUE;
		for (i=nmeasures;i>m;i--)
			measure[i] = measure[i-1];
		measure[m].npatterns = 0;
		nmeasures++;
		if (start_copy_measure > -1 && start_copy_measure > m)
			start_copy_measure++;
		if (end_copy_measure > -1 && end_copy_measure >= m)
			end_copy_measure++;
		if (start_copy_measure >= MAXMEASURES || end_copy_measure >= MAXMEASURES) {
			start_copy_measure = -1;
			end_copy_measure = -1;
		}
		redraw_arranger();
	} else if (w == Delete_da) { 
		int i;
		/* printf("Delete clicked, measure = %d\n", m); */
		if (nmeasures <= 0)
			return TRUE;
		for (i=m;i<nmeasures-1;i++)
			measure[i] = measure[i+1];
		nmeasures--;
		if (m == start_copy_measure || m == end_copy_measure) {
			start_copy_measure = -1;
			end_copy_measure = -1;
		}
		if (start_copy_measure > -1 && start_copy_measure > m)
			start_copy_measure--;
		if (end_copy_measure > -1 && end_copy_measure >= m)
			end_copy_measure--;
		if (start_copy_measure < 0 || end_copy_measure < 0) {
			start_copy_measure = -1;
			end_copy_measure = -1;
		}
		redraw_arranger();
	} else {
		printf("Unknown clicked, measure = %d\n", m);
	}
	return TRUE;
}

static int copy_measure_press(GtkWidget *w, GdkEventButton *event, 
		gpointer data)
{
	int m, i;
	m = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
	start_copy_measure = m;
	end_copy_measure = m;
	redraw_arranger();
}

static int copy_measure_release(GtkWidget *w, GdkEventButton *event, 
		gpointer data)
{
	int m, i;
	m = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
	if (start_copy_measure == -1)
		start_copy_measure = m;
	if (end_copy_measure == -1)
		end_copy_measure = m;
	if (m <= start_copy_measure)
		start_copy_measure = m;
	else
		end_copy_measure = m;
	redraw_arranger();
}

static int arr_darea_clicked(GtkWidget *w, GdkEventButton *event, 
		struct pattern_struct *data)
{
	struct measure_struct *m;
	int mn, i;
	mn = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
	if (mn > nmeasures || mn == MAXMEASURES)
		return TRUE;

	if (mn == nmeasures) /* creating a new measure? */
		measure[mn].npatterns = 0;
	/* printf("selected measure m = %d\n", m); */
	if (event->button == 1) {
		m = &measure[mn];
		int j, found;
		int p = data->pattern_num; 
		/* See if this pattern is already selected, if so, unselect it. */
		found = 0;
		for (i=0;i<m->npatterns;i++) {
			if (m->pattern[i] == p) {
				for (j=i;j<m->npatterns-1;j++)
					m->pattern[j] = m->pattern[j+1];
				m->npatterns--;
				found = 1;
			}
		}

		/* If not found and removed, then add it, if room */
		if (!found && m->npatterns < MAXPATSPERMEASURE) {
			m->pattern[m->npatterns] = p;
			m->npatterns++;
		}

		gtk_widget_queue_draw(w);
		if (mn+1 > nmeasures) {
			nmeasures++;
			redraw_arranger();
		}
	}
	return TRUE;
}

static int canvas_clicked(GtkWidget *w, GdkEventButton *event, struct instrument_struct *data)
{
	/* if (data != NULL)
		g_print("%s canvas clicked!, x=%g, y=%g\n", data->name, event->x, event->y); */
	int rc;
	unsigned char velocity = DEFAULT_VELOCITY;
	int change_velocity = (event->button == 1);


	/* 1st mouse button chooses velocity based on vertical position, like a graph, 
	   2nd mouse button uses the default velocity set by the volume slider */	
	if (change_velocity)
		velocity = 127 - (unsigned char) (((double) event->y / (double) DRAW_HEIGHT) * 127.0);
	else
		velocity = (unsigned char) gtk_range_get_value(GTK_RANGE(data->volume_slider));

	if (event->button == 1 || event->button == 2) {
		rc = add_hit(&data->hit, (double) event->x, (double) DRAW_WIDTH, 
			kit, data->instrument_num, cpattern, velocity, change_velocity);
		if (midi_fd >= 0)
			access_device->note_on(midi_fd, data->midivalue, velocity);
		if (rc == -2) /* Note was already there, so we really want to remove it. */
			remove_hit(&data->hit, (double) event->x, (double) DRAW_WIDTH,
				kit, data->instrument_num, cpattern);
	} else if (event->button == 3)
		remove_hit(&data->hit, (double) event->x, (double) DRAW_WIDTH,
			kit, data->instrument_num, cpattern);
	gtk_widget_queue_draw(w);
	return TRUE;
}

static int measure_da_expose(GtkWidget *w, GdkEvent *event, gpointer p)
{
	int i, x, y1, y2, j;

	gdk_draw_line(w->window, gc, 0,0, MEASUREWIDTH * (nmeasures), 0);
	gdk_draw_line(w->window, gc, 0, ARRANGER_HEIGHT, MEASUREWIDTH * (nmeasures), ARRANGER_HEIGHT);
	x = 0; y1 = 0; y2 = ARRANGER_HEIGHT;
	for (i=0;i<=nmeasures;i++) {
		if (i==nmeasures && w != Paste_da)
			break;
		gdk_draw_line(w->window, gc, x, y1, x, y1+ARRANGER_HEIGHT);

		gdk_draw_line(w->window, gc, x+4, y1+4, x+4, y1+ARRANGER_HEIGHT-4);
		gdk_draw_line(w->window, gc, x+MEASUREWIDTH-4, y1+4, x+MEASUREWIDTH-4, y1+ARRANGER_HEIGHT-4);
		gdk_draw_line(w->window, gc, x+4, y1+4, x+MEASUREWIDTH-4, y1+4);
		gdk_draw_line(w->window, gc, x+4, y1+ARRANGER_HEIGHT-4, x+MEASUREWIDTH-4, y1+ARRANGER_HEIGHT-4);
		if (w == Copy_da) {
			if (i >= start_copy_measure && i <= end_copy_measure) {
					gdk_draw_line(w->window, gc, 
						x+4, y1+4, 
						x+MEASUREWIDTH-4, y1+ARRANGER_HEIGHT-4);
					gdk_draw_line(w->window, gc, x+4, y1+ARRANGER_HEIGHT-4,
						x+MEASUREWIDTH-4, y1+4);
			}
		}
		if (w == Tempo_da) {
			for (j=0;j<ntempochanges;j++)
				if (tempo_change[j].measure == i) {
					gdk_draw_line(w->window, gc, 
						x+4, y1+4, 
						x+MEASUREWIDTH-4, y1+ARRANGER_HEIGHT-4);
					gdk_draw_line(w->window, gc, x+4, y1+ARRANGER_HEIGHT-4,
						x+MEASUREWIDTH-4, y1+4);
				}
		}

		x += MEASUREWIDTH;
	}
	gdk_draw_line(w->window, gc, x, y1, x, y1+ARRANGER_HEIGHT);
#if 0
	if (w == Tempo_da) {
		printf("Tempo expose\n");
	} else if (w == Copy_da) { 
		printf("Copy expose\n");
	} else if (w == Paste_da) { 
		printf("Paste expose\n");
	} else if (w == Insert_da) { 
		printf("Insert expose\n");
	} else if (w == Delete_da) { 
		printf("Delete expose\n");
	} else {
		printf("Unknown expose\n");
	}
#endif
	return TRUE;
}

static int arr_darea_event(GtkWidget *w, GdkEvent *event, struct pattern_struct *p)
{
	int i, x, y1, y2, j;
	/* printf("arranger event\n"); */

	gdk_draw_line(w->window, gc, 0,0, MEASUREWIDTH * (nmeasures + 1), 0);
	gdk_draw_line(w->window, gc, 0, ARRANGER_HEIGHT-1, MEASUREWIDTH * (nmeasures + 1), ARRANGER_HEIGHT-1);
	if (p->pattern_num == pattern_in_copy_buffer) {
		/* probably could do something better by using color... */
		gdk_draw_line(w->window, gc, 0,1, MEASUREWIDTH * (nmeasures + 1), 1);
		gdk_draw_line(w->window, gc, 0, ARRANGER_HEIGHT-2, 
			MEASUREWIDTH * (nmeasures + 1), ARRANGER_HEIGHT-2);
	}
		
	x = 0; y1 = 0; y2 = ARRANGER_HEIGHT;
	if (start_copy_measure > -1 && end_copy_measure >= start_copy_measure)
		gdk_draw_line(w->window, gc, start_copy_measure*MEASUREWIDTH+1, y1+ARRANGER_HEIGHT/2, 
					(end_copy_measure+1)*MEASUREWIDTH-1, y1+ARRANGER_HEIGHT/2);
	for (i=0;i<nmeasures+2;i++) {
		/* probably could do something better by using color... */
		if (i == start_copy_measure) {
			gdk_draw_line(w->window, gc, x+1, y1, x+1, y1+ARRANGER_HEIGHT);
			gdk_draw_line(w->window, gc, x+1, ARRANGER_HEIGHT / 2, x+MEASUREWIDTH/2, ARRANGER_HEIGHT/3); 
			gdk_draw_line(w->window, gc, x+1, ARRANGER_HEIGHT / 2, x+MEASUREWIDTH/2, 2*ARRANGER_HEIGHT/3); 
		}
		if (i == end_copy_measure) {
			gdk_draw_line(w->window, gc, x+MEASUREWIDTH-1, y1, 
				x+MEASUREWIDTH-1, y1+ARRANGER_HEIGHT);
			gdk_draw_line(w->window, gc, x+MEASUREWIDTH-1, ARRANGER_HEIGHT / 2, x+MEASUREWIDTH/2, ARRANGER_HEIGHT/3); 
			gdk_draw_line(w->window, gc, x+MEASUREWIDTH-1, ARRANGER_HEIGHT / 2, x+MEASUREWIDTH/2, 2*ARRANGER_HEIGHT/3); 
		}
		gdk_draw_line(w->window, gc, x, y1, x, y1+ARRANGER_HEIGHT);
		if (measure[i].npatterns != 0 && i<nmeasures ) {
			for (j=0;j<measure[i].npatterns;j++) {
				if (measure[i].pattern[j] == p->pattern_num) {
					gdk_draw_line(w->window, gc, x, y1, x+MEASUREWIDTH, y1+ARRANGER_HEIGHT);
					gdk_draw_line(w->window, gc, x+MEASUREWIDTH, y1, x, y1+ARRANGER_HEIGHT);
					break;
				}
			}
		}
		x += MEASUREWIDTH;
	}
	/* gtk_widget_queue_draw(w); */
}

static int canvas_event(GtkWidget *w, GdkEvent *event, struct instrument_struct *instrument)
{
	int i,j; 
	double diff;
	GdkColor color, bg;
	float divs, zero;
	struct hitpattern *this;

	gdk_colormap_alloc_color(gtk_widget_get_colormap(w), &whitecolor, FALSE, FALSE);
	gdk_gc_set_background(gc, &whitecolor);
	
	/* gdk_draw_line(w->window, gc, 0,DRAW_HEIGHT / 2, DRAW_WIDTH, DRAW_HEIGHT / 2); */
	/* gdk_draw_line(w->window, gc, 0,0, DRAW_WIDTH, DRAW_HEIGHT);
	gdk_draw_line(w->window, gc, 0,DRAW_HEIGHT, DRAW_WIDTH, 0); */
	gdk_draw_line(w->window, gc, 0,DRAW_HEIGHT, DRAW_WIDTH, DRAW_HEIGHT); 
	gdk_draw_line(w->window, gc, 0,0, 0, DRAW_HEIGHT);
	gdk_draw_line(w->window, gc, DRAW_WIDTH,0, DRAW_WIDTH, DRAW_HEIGHT);

	// for (i=0;i<ndivisions;i++) {
	for (i=ndivisions-1;i>=0;i--) {
		divs =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(timediv[i].spin));
		zero = divs - 0.0;
		if (zero < 0) 
			zero = -zero;
		if (zero <= 0.000001) /* not using this one */
			continue;

		diff = (double) DRAW_WIDTH / divs;
		memset(&color, 0, sizeof(color));
		gdk_color_parse(timediv[i].color, &color);
		gdk_colormap_alloc_color(gtk_widget_get_colormap(w), &color, FALSE, FALSE);
		gdk_gc_set_foreground(gc, &color);
		for (j=0;j<divs;j++) {
			gdk_draw_line(w->window, gc, (int) diff, 0, (int) diff, DRAW_HEIGHT);
			diff += (double) DRAW_WIDTH / divs;
		}
		gdk_colormap_free_colors(gtk_widget_get_colormap(w), &color, 1);
		gdk_color_parse("black", &color);
		gdk_colormap_alloc_color(gtk_widget_get_colormap(w), &color, FALSE, FALSE);
		gdk_gc_set_foreground(gc, &color);
	}

	/* if (instrument->hit == NULL)
		g_print("instrument hit is null\n");
	else
		g_print("instrument hit is NOT null\n");
	*/

	for (this = instrument->hit; this != NULL; this = this->next) {
		double x1,y1,x2,y2;

		y1 = (DRAW_HEIGHT / 2)-5;
		x1 = this->h.time * DRAW_WIDTH /* - 5 */ ;
		y1 = DRAW_HEIGHT - (int) (((double) DRAW_HEIGHT * (double) this->h.velocity) / 127.0);
		y2 = DRAW_HEIGHT;
		x2 = x1 + 10;
		/* g_print("x1=%g, y1=%g, x2=%g, y2=%g\n", x1, y1, x2, y2); */
		gdk_draw_line(w->window, gc, (int) x1, (int) y1, (int) x2, (int) y2);
		// gdk_draw_line(w->window, gc, (int) x1, (int) y2, (int) x2, (int) y1);
		gdk_draw_line(w->window, gc, (int) x1, (int) y2, (int) x1, (int) y1);
		// gdk_draw_line(w->window, gc, (int) x2, (int) y2, (int) x2, (int) y1);
		// gdk_draw_line(w->window, gc, (int) x1, (int) y1, (int) x2, (int) y1);
		gdk_draw_line(w->window, gc, (int) x1, (int) y2, (int) x2, (int) y2);
	}

	/* if (instrument != NULL) 
		g_print("%s\n", instrument->name); */
	return TRUE;
}

void check_hitpatterns(char *x);
void clear_hitpattern(struct hitpattern *p);

void pattern_clear_button_clicked(GtkWidget *widget,
	gpointer data)
{
	printf("Pattern clear button.\n");
	int i;
	/* Clear out anything old */
	for (i=0;i<drumkit[kit].ninsts; i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		if (inst->hit != NULL) {
			clear_hitpattern(inst->hit);
			inst->hit = NULL;
		}
		gtk_widget_queue_draw(GTK_WIDGET(drumkit[kit].instrument[i].canvas));
	}

}

void pattern_stop_button_clicked(GtkWidget *widget,
	gpointer data)
{
	printf("Pattern stop button.\n");
	kill(player_process_pid, SIGUSR1);
}

void schedule_pattern(int kit, int cpattern, int tempo, struct timeval *base)
{
	struct timeval basetime;
	unsigned long measurelength;
	unsigned long beats_per_minute, beats_per_measure;
	struct hitpattern *this;
	int rc, i;

	/* printf("schedule_pattern called\n"); */
	basetime.tv_usec = 0L;
	basetime.tv_sec = 0L; /* relative time, means, right now. */
	if (base != NULL)
		basetime = *base;

	/* FIXME tempo */
	if (tempo == -1)
		beats_per_minute = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tempospin1));
	else
		beats_per_minute = tempo;
	// beats_per_measure = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tempospin2));
	beats_per_measure = pattern[cpattern]->beats_per_measure;

	/* printf("bpmin = %d, bpmeas=%d\n", beats_per_minute, beats_per_measure); */

	measurelength = (60000000 / beats_per_minute) * beats_per_measure;

	for (this = pattern[cpattern]->hitpattern; this != NULL; this = this->next) {
		struct drumkit_struct *dk = &drumkit[this->h.drumkit];
		struct instrument_struct *inst = &dk->instrument[this->h.instrument_num];
		/* printf("this->h.time = %g\n", this->h.time); */
		rc = sched_note(&sched, &basetime, inst->midivalue, 
			measurelength, this->h.time, 1000000, this->h.velocity);
	}
	rc = sched_noop(&sched, &basetime, 0, measurelength, 1.0, 1000000, 127); 
	if (base != NULL)
		*base = basetime;
}

void schedule_measures(int start, int end)
{
	int i,j;
	struct timeval base, tmpbase;
	int tempo;

	/* Give ourself 1/10th millisecond per measure of lead time to 
	   calculate the schedule, otherwise if we're slow, the first beat
	   will be late.  Debug output in the scheduler will exacerbate this. */

	if (start < 0 || end < start)
		return;

	base.tv_usec = 5000L * nmeasures;
	base.tv_sec = 0L; 
	
	for (i=start;i<end;i++) {
		for (j=0;j<measure[i].npatterns;j++) {
			tmpbase = base;
			tempo = find_tempo(i);
			/* printf("Tempo for measure %d is %d beats per minute\n", i, tempo); */
			schedule_pattern(kit, measure[i].pattern[j], tempo, &tmpbase);
		}
		base = tmpbase;
	}
}

int load_from_file(const char *filename);

void loadbox_file_selected(GtkWidget *widget,
	GtkFileSelection *SaveBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(SaveBox);
	gtk_widget_hide(GTK_WIDGET(SaveBox));
	load_from_file(filename);
}

void savebox_file_selected(GtkWidget *widget,
	GtkFileSelection *SaveBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(SaveBox);
	gtk_widget_hide(GTK_WIDGET(SaveBox));
	save_to_file(filename);
}

void savedrumkitbox_file_selected(GtkWidget *widget,
	GtkFileSelection *SaveBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(SaveBox);
	gtk_widget_hide(GTK_WIDGET(SaveBox));
	save_drumkit_to_file(filename);
}

#define SAVE_SONG 0
#define LOAD_SONG 1
#define SAVE_DRUMKIT 2
static struct file_dialog_descriptor {
	char *title;
	GtkWidget **widget;
	void *file_selected_function;
} file_dialog[] = {
	{ "Save Song", &SaveBox, (void *) savebox_file_selected, },
	{ "Load Song", &LoadBox, (void *) loadbox_file_selected, },
	{ "Save Drum Kit", &SaveDrumkitBox, (void *) savedrumkitbox_file_selected, },
};
	
GtkWidget *make_file_dialog(int i)
{
	GtkWidget *w;
	void *f;

	if (i< 0 || i >= sizeof(file_dialog) / sizeof(file_dialog[0]))
		return;

	w = gtk_file_selection_new (file_dialog[i].title);
	f = file_dialog[i].file_selected_function;
	*(file_dialog[i].widget) = w;
	g_signal_connect(G_OBJECT (w), "destroy",
		G_CALLBACK (destroy_means_hide), NULL); /* FIXME, this is not correct */
	g_signal_connect(G_OBJECT (GTK_FILE_SELECTION (w)->ok_button),
		"clicked", G_CALLBACK (f), (gpointer) w);
	g_signal_connect_swapped(G_OBJECT (GTK_FILE_SELECTION (w)->cancel_button),
		"clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (w));
	gtk_widget_show(w);
	return w;
}

void save_button_clicked(GtkWidget *widget,
	gpointer data)
{
	make_file_dialog(SAVE_SONG);
}

void load_button_clicked(GtkWidget *widget,
	gpointer data)
{
	make_file_dialog(LOAD_SONG);
}

void save_drumkit_button_clicked(GtkWidget *widget,
	gpointer data)
{
	make_file_dialog(SAVE_DRUMKIT);
}

void send_schedule(struct schedule_t *sched, int loop);
void arranger_play_button_clicked(GtkWidget *widget,
	gpointer data)
{
	int start, end;

	if (widget == play_button) {
		start = 0;
		end = nmeasures;
	} else {
		start = start_copy_measure;
		end = end_copy_measure+1;
	}

	if (start < 0 || end < start)
		return;

	flatten_pattern(kit, cpattern);
	schedule_measures(start, end);
	/* print_schedule(&sched); */
	send_schedule(&sched, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(arr_loop_check_button)));
	free_schedule(&sched);
}

void pattern_paste_button_clicked(GtkWidget *widget,
	gpointer data)
{
	int i, from = pattern_in_copy_buffer;
	struct hitpattern *f, **hit;

	/* superimpose the hits from one pattern into another */
	if (from < 0 || 
		from >= npatterns || 
		from == cpattern) /* can't paste onto self */
		return;

	flatten_pattern(kit, cpattern);

	for (f = pattern[cpattern]->hitpattern; f != NULL; f=f->next) {
		printf("%p: beat:%d/%d inst=%d\n", f, f->h.beat,
			f->h.beats_per_measure, f->h.instrument_num); fflush(stdout);
	}
	for (f = pattern[from]->hitpattern; f != NULL; f=f->next) {
		printf("%p: beat:%d/%d inst=%d\n", f, f->h.beat,
			f->h.beats_per_measure, f->h.instrument_num); fflush(stdout);
	}
	hit == &pattern[cpattern]->hitpattern;
	for (f = pattern[from]->hitpattern; f != NULL; f=f->next) {
		lowlevel_add_hit(&pattern[cpattern]->hitpattern, kit, cpattern, 
			f->h.instrument_num, f->h.beat, f->h.beats_per_measure, 
			f->h.velocity, 1);
	}
	unflatten_pattern(kit, cpattern);
	for (i=0;i<drumkit[kit].ninsts; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
	return;
}

void pattern_play_button_clicked(GtkWidget *widget,
	gpointer data)
{
	flatten_pattern(kit, cpattern);
	schedule_pattern(kit, cpattern, -1, NULL);
	send_schedule(&sched, 0);
	free_schedule(&sched);
}

void set_arranger_window_title()
{
	sprintf(arranger_title, "%s v. %s - Arrangement Editor: %s",
		PROGNAME, VERSION, songname);
	gtk_window_set_title(GTK_WINDOW(arranger_window), arranger_title);
	gtk_entry_set_text(GTK_ENTRY(song_name_entry), songname);
}

void set_pattern_window_title()
{
	gchar pattern_name[100];

	if (pattern[cpattern] != NULL && strcmp(pattern[cpattern]->patname, "") != 0)
		sprintf(pattern_name, "%s", pattern[cpattern]->patname);	
	else
		sprintf(pattern_name, "Pattern %d", cpattern);
	sprintf(window_title, "%s v. %s - Pattern Editor: %s", 
		PROGNAME, VERSION, pattern_name);
	gtk_window_set_title(GTK_WINDOW(window), window_title);
	gtk_entry_set_text(GTK_ENTRY(pattern_name_entry), pattern_name);
}

void edit_pattern(int new_pattern)
{
	int i;
	cpattern = new_pattern;
	unflatten_pattern(kit, cpattern); /* load next pattern */
	set_pattern_window_title();
	flatten_pattern(kit, cpattern); /* cause widgets and stuff to be made... */
	if (cpattern == npatterns - 1) {
		gtk_button_set_label(GTK_BUTTON(nextbutton), "Create Next Pattern -->"); 
		gtk_tooltips_set_tip(tooltips, nextbutton, "Create and edit the next pattern", NULL);
	} else {
		gtk_button_set_label(GTK_BUTTON(nextbutton), "Edit Next Pattern -->"); 
		gtk_tooltips_set_tip(tooltips, nextbutton, "Edit the next pattern", NULL);
	}
	/* Make the timing lines redraw . . . */
	for (i=0;i<drumkit[kit].ninsts;i++)
		gtk_widget_queue_draw(GTK_WIDGET(drumkit[kit].instrument[i].canvas));
}

void ins_pattern_button_pressed(GtkWidget *widget, /* insert pattern */
	struct pattern_struct *data)
{
	printf("insert pattern, not yet implemented.\n");
	/* this involves shuffling things around in a gtk_table... not sure how. */

	/* Hmmm.  It occurs to me I could do it easily by saving the song to a file, editing the file,
	   then reading the file back in... ha ha.  Brute force. Same for deleting patterns. 

	   So for now, if you *REALLY* must insert a pattern, or delete a pattern, save the
	   file, crank up vi, and do it.  Really there should be no reason you
	   should really need to delete/insert patterns though, apart from aesthetic ones.
	*/
}

void del_pattern_button_pressed(GtkWidget *widget, /* delete pattern */
	struct pattern_struct *data)
{
	
	printf("delete pattern, not implemented yet.\n");
	/* this involves shuffling things around in a gtk_table... not sure how. */

}

void copy_pattern_button_pressed(GtkWidget *widget, /* copy pattern */
	struct pattern_struct *data)
{
	char msg[255];
	pattern_in_copy_buffer = data->pattern_num;
	sprintf(msg, "Superimpose all the notes of the selected pattern (%s) onto this pattern.", 
		data->patname);
	gtk_tooltips_set_tip(tooltips, pattern_paste_button, msg, NULL);
	redraw_arranger();
}

void edit_pattern_clicked(GtkWidget *widget, /* this is the "pattern name" button in the arranger window */
	struct pattern_struct *data)
{
	int i;

	flatten_pattern(kit, cpattern); /* save current pattern */
	edit_pattern(data->pattern_num);
}

void prevbutton_clicked(GtkWidget *widget,
	gpointer data)
{
	int i;
	if (cpattern <= 0) {
		g_print("No previous pattern.\n");
		return;
	}
	flatten_pattern(kit, cpattern); /* save current pattern */
	// cpattern--;
	edit_pattern(cpattern-1);
#if 0
	unflatten_pattern(kit, cpattern); /* load next pattern */

	set_pattern_window_title();

	/* Make the timing lines redraw . . . */
	for (i=0;i<drumkit[kit].ninsts;i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
#endif
}


void nextbutton_clicked(GtkWidget *widget,
	gpointer data)
{
	int i;
	if (cpattern >= MAXPATTERNS-1) {
		g_print("No room for more patterns\n");
		return;
	}

	flatten_pattern(kit, cpattern); /* save current pattern */
	cpattern++; 
	if (cpattern == npatterns) {
		npatterns++;
	}
	edit_pattern(cpattern);
#if 0
	unflatten_pattern(kit, cpattern); /* load next pattern */
	if (cpattern == npatterns-1)
		make_new_pattern_widgets(cpattern, npatterns);
	
	set_pattern_window_title();

	/* Make the timing lines redraw . . . */
	for (i=0;i<drumkit[kit].ninsts;i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
#endif
}


void hello(GtkWidget *widget,
	struct instrument_struct *data)
{
	unsigned char velocity;

	velocity = (unsigned char) gtk_range_get_value(GTK_RANGE(data->volume_slider));
	if (data != NULL) {
		printf("%s\n", data->name);
		/* should be changed to *schedule* a noteon + noteoff */
		if (midi_fd >= 0)
			access_device->note_on(midi_fd, data->midivalue, velocity);
	}
}


void destroy_event(GtkWidget *widget,
	gpointer data)
{
	/* g_print("destroy event!\n"); */
	save_to_file("/tmp/zzz");
	kill(player_process_pid, SIGTERM); /* a little brutal, but effective . . . */
	gtk_main_quit();
}


gint delete_event(GtkWidget *widget,
		GdkEvent *event,
		gpointer data)
{
	return TRUE;
}

void song_name_entered(GtkWidget *widget, GtkWidget *entry)
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
	strcpy(songname, entry_text);
	set_arranger_window_title();
}

void instrument_name_entered(GtkWidget *widget, struct instrument_struct *inst)
{
	strncpy(inst->name, gtk_entry_get_text(GTK_ENTRY(widget)), 39);
	gtk_button_set_label(GTK_BUTTON(inst->button), inst->name); 
}

void instrument_type_entered(GtkWidget *widget, struct instrument_struct *inst)
{
	strncpy(inst->type, gtk_entry_get_text(GTK_ENTRY(widget)), 39);
	gtk_tooltips_set_tip(tooltips, inst->button, inst->type, NULL);
}

void hide_instruments_button_callback (GtkWidget *widget, gpointer data)
{
	int i, vshidden, unchecked_hidden, editdrumkit;

	vshidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_volume_sliders));
	unchecked_hidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_instruments));
	editdrumkit = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (edit_instruments_toggle));

	for (i=0;i<drumkit[kit].ninsts;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		if (vshidden) {
			gtk_widget_hide(GTK_WIDGET(inst->volume_slider));
			gtk_widget_hide(GTK_WIDGET(inst->drag_spin_button));
		}
		if (editdrumkit) {
			gtk_widget_hide(GTK_WIDGET(inst->midi_value_spin_button));
			gtk_widget_hide(GTK_WIDGET(inst->name_entry));
			gtk_widget_hide(GTK_WIDGET(inst->type_entry));
		}

		if (unchecked_hidden)
			gtk_widget_hide(GTK_WIDGET(inst->hidebutton));
		else
			gtk_widget_show(GTK_WIDGET(inst->hidebutton));

		if (unchecked_hidden && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(inst->hidebutton))) {
			gtk_widget_hide(GTK_WIDGET(inst->canvas));
			gtk_widget_hide(GTK_WIDGET(inst->button));
			if (!editdrumkit) { /* otherwise, already hidden */
				gtk_widget_hide(GTK_WIDGET(inst->name_entry));
				gtk_widget_hide(GTK_WIDGET(inst->type_entry));
				gtk_widget_hide(GTK_WIDGET(inst->midi_value_spin_button));
			}
			if (!vshidden) { /* otherwise, already hidden */
				gtk_widget_hide(GTK_WIDGET(inst->volume_slider));
				gtk_widget_hide(GTK_WIDGET(inst->drag_spin_button));
			}
		} else {
			gtk_widget_show(GTK_WIDGET(inst->canvas));
			gtk_widget_show(GTK_WIDGET(inst->button));
			if (!editdrumkit) {/* otherwise, already hidden */
				gtk_widget_show(GTK_WIDGET(inst->name_entry));
				gtk_widget_show(GTK_WIDGET(inst->type_entry));
				gtk_widget_show(GTK_WIDGET(inst->midi_value_spin_button));
			}
			if (!vshidden) { /* otherwise, already hidden */
				gtk_widget_show(GTK_WIDGET(inst->volume_slider));
				gtk_widget_show(GTK_WIDGET(inst->drag_spin_button));
			}
		}
	}
}

void check_hitpatterns(char *x)
{
	int i;
	for (i=0;i<drumkit[kit].ninsts;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		if (inst->hit != NULL)
			g_print("%s: Non null hitpattern at inst = %d\n", x, i);
	}
	for (i=0;i<npatterns;i++) {
		if (pattern[i] != NULL)
			if (pattern[i]->hitpattern != NULL)
				g_print("%s, pattern %d has non null hitpattern\n", x, i);
	}
}

void clear_hitpattern(struct hitpattern *p)
{
	struct hitpattern *h, *next;
	if (p == NULL) {
		return;
	}
	for (h = p; h != NULL; h = next) {
		if (h == NULL) {
			g_print("h is null!!!\n");
			break;
		}
		next = h->next;
		h->next = NULL;
		if (h != NULL) { 
			free(h);
			h = NULL;
		}
	}
	return;
}

int clear_kit_pattern(int ckit)
{
	/* clears all the recorded strikes for all instruments in the current pattern */
	struct drumkit_struct *dk = &drumkit[ckit];
	int i;
	for (i=0;i<dk->ninsts;i++) {
		struct instrument_struct *inst = &dk->instrument[i];
		if (inst->hit != NULL) {
			clear_hitpattern(inst->hit);
			inst->hit = NULL;
		}
	}
}

int flatten_pattern(int ckit, int cpattern)
{
	/* In the pattern editor, there is a linked list of strikes per instrument, this function
	   collapses that down into a single linked list of strikes per pattern */

	int i;
	struct drumkit_struct *dk = &drumkit[ckit];
	struct pattern_struct *p = pattern[cpattern];

	if (npatterns < cpattern+1) {
		npatterns = cpattern+1;
	}

	if (p == NULL) {
		p = malloc(sizeof(*p));
		if (p == NULL)
			return -1;
		memset(p, 0, sizeof(*p));
		p->pattern_num = cpattern;
		p->hitpattern = NULL;
		pattern[cpattern] = p;
	}
	if (p->arr_button == NULL)
		make_new_pattern_widgets(cpattern, npatterns);
	
	p->beats_per_minute = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tempospin1));
	p->beats_per_measure = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tempospin2));
	strncpy(p->patname, gtk_entry_get_text(GTK_ENTRY(pattern_name_entry)), 39);
	if (p->arr_button != NULL)
		gtk_button_set_label(GTK_BUTTON(p->arr_button), p->patname);

	if (p->hitpattern != NULL) {
		clear_hitpattern(p->hitpattern);
		p->hitpattern = NULL;
	}

	for (i=0;i<ndivisions;i++) {
		p->timediv[i].division = 
			gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(timediv[i].spin));
	}

	/* copy patterns from per instrument patterns area to single pattern */
	for (i=0;i<dk->ninsts;i++) {
		struct hitpattern *h;
		struct instrument_struct *inst = &dk->instrument[i];
		for (h = inst->hit; h != NULL; h=h->next) {
			lowlevel_add_hit(&p->hitpattern, ckit, cpattern, inst->instrument_num, 
				h->h.beat, h->h.beats_per_measure, h->h.velocity, 1);
		}
	}
	return 0;
}

struct pattern_struct *pattern_struct_alloc(int pattern_num)
{
	struct pattern_struct *p;
	p = malloc(sizeof(*p));
	if (p == NULL)
		return NULL;
	memset(p, 0, sizeof(*p));
	p->hitpattern = NULL;
	p->pattern_num = pattern_num;
	p->beats_per_measure = 4;
	p->beats_per_minute = 120;
	sprintf(p->patname, "Pattern %d", pattern_num);
	return p;
}

int unflatten_pattern(int ckit, int cpattern)
{
	/* copy the flattened pattern instrument by instrument into the 
	   pattern editor struct */
	struct hitpattern *h;
	int i;

	/* Clear out anything old */
	for (i=0;i<drumkit[ckit].ninsts; i++) {
		struct instrument_struct *inst = &drumkit[ckit].instrument[i];
		if (inst->hit != NULL) {
			clear_hitpattern(inst->hit);
			inst->hit = NULL;
		}
	}

	if (pattern[cpattern] == NULL) {
		pattern[cpattern] = pattern_struct_alloc(cpattern); 
		if (pattern[cpattern] == NULL)
			return -1;
		for (i=0;i<ndivisions;i++)
			pattern[cpattern]->timediv[i].division = 
				gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(timediv[i].spin));
		if (cpattern > 0) {
			pattern[cpattern]->beats_per_measure = pattern[cpattern-1]->beats_per_measure;
			pattern[cpattern]->beats_per_minute = pattern[cpattern-1]->beats_per_minute;
			/* printf("copying tempo, %d, %d\n", 
				pattern[cpattern]->beats_per_measure,
				pattern[cpattern]->beats_per_minute); */
		}
	}
	if (pattern[cpattern]->arr_button != NULL)
		gtk_button_set_label(GTK_BUTTON(pattern[cpattern]->arr_button), 
			pattern[cpattern]->patname);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin1), (gdouble) pattern[cpattern]->beats_per_minute);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin2), (gdouble) pattern[cpattern]->beats_per_measure);
	for (h = pattern[cpattern]->hitpattern; h != NULL; h = h->next) {
		/* g_print("inst = %d, beat=%d, bpm =%d\n", h->h.instrument_num, 
			h->h.beat, h->h.beats_per_measure); fflush(stdout); */
		lowlevel_add_hit(&drumkit[ckit].instrument[h->h.instrument_num].hit, 
				ckit, cpattern, h->h.instrument_num, 
				h->h.beat, h->h.beats_per_measure, h->h.velocity, 1);
		/* add_hit(&drumkit[ckit].instrument[h->h.instrument_num].hit, 
			h->h.beat, h->h.beats_per_measure,
			ckit, h->h.instrument_num, cpattern); */
	}

	for (i=0;i<ndivisions;i++) {
		timediv[i].division = pattern[cpattern]->timediv[i].division;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(timediv[i].spin), 
			(gdouble) timediv[i].division);
	}
	set_pattern_window_title();
}

int xpect(FILE *f, int *lc, char *line, char *value)
{
	int rc;
	
	rc = fscanf(f, "%[^\n]%*c", line);
	if (strncmp(line, value, strlen(value)) != 0) {
		printf("Error at line %d:%s\n", *lc, line);
		printf("Expected '%s', got '%s'\n", value, line); 
		return -1;
	}
	*lc++;
	return 0;	
}

void init_measures();

int load_from_file_version_1(FILE *f)
{
	char line[255];
	int linecount;
	int ninsts;
	int i,j, rc;
	int hidden;
	int fileformatversion;


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

void cleanup_tempo_changes()
{
	struct tempo_change_t tmp[MAXMEASURES];
	int i, ntc = 0;
	int last = -99;
	int did_some_cleaning = 0;

	for (i=0;i<ntempochanges;i++) {
		if (tempo_change[i].beats_per_minute != last) {
			tmp[ntc] = tempo_change[i];
			ntc++;
			last = tempo_change[i].beats_per_minute;
		} else
			did_some_cleaning = 1;
	}
	if (did_some_cleaning) {
		for (i=0;i<ntc;i++)
			tempo_change[i] = tmp[i];
		ntempochanges = ntc;
	}
	return;
}

int insert_tempo_change(int measure, int tempo)
{
	int i, j, rc = -1;

	printf("ntempochanges = %d\n", ntempochanges);
	for (i=0;i<ntempochanges;i++) {
	cleanup_tempo_changes();	
		if (tempo_change[i].measure > measure) {
			break;
		} else if (tempo_change[i].measure == measure) {
			if (tempo == -1)
				return i;
			tempo_change[i].beats_per_minute = tempo;
			tempo_change[i].measure = measure;
			return i;	
		}
	}
	if (ntempochanges <= 0) { /* shouldn't happen */
		ntempochanges = 1;
		if (tempo != -1)
			tempo_change[0].beats_per_minute = tempo;
		else
			tempo_change[0].beats_per_minute = 120;
		tempo_change[0].measure = measure;
		return 0;
	}

	if (ntempochanges >= MAXMEASURES)
		return -1;

	if (i>=ntempochanges) {
		if (tempo == -1 && ntempochanges > 0)
			tempo_change[ntempochanges].beats_per_minute = 
				tempo_change[ntempochanges-1].beats_per_minute;
		else
			tempo_change[ntempochanges].beats_per_minute = tempo;
		tempo_change[ntempochanges].measure = measure;
		rc = ntempochanges;
	} else {
		for (j=ntempochanges;j>i;j--)
			tempo_change[j] = tempo_change[j-1];
		if (tempo == -1 && i > 0)
			tempo_change[i].beats_per_minute = tempo_change[i-1].beats_per_minute;
		else
			tempo_change[i].beats_per_minute = tempo;
		tempo_change[i].measure = measure;
		rc = i;
	}
	ntempochanges++;
	return rc;
}

int remove_tempo_change(int index)
{
	int i;
	for (i=index;i<ntempochanges-1;i++)
		tempo_change[i] = tempo_change[i+1];
	ntempochanges--;
}

int find_tempo(int measure)
{
	int i, tempo;
	tempo = tempo_change[0].beats_per_minute;
	for (i=0;i<ntempochanges;i++) {
		if (tempo_change[i].measure > measure)
			break;
		tempo = tempo_change[i].beats_per_minute;
	}
	return tempo;
}

int load_from_file(const char *filename)
{
	FILE *f;
	char line[255];
	int linecount;
	int ninsts;
	int i,j, rc;
	int hidden;
	int fileformatversion;

	/* First clear everything old out of the way */

	init_measures();
	for (i=0;i<drumkit[kit].ninsts; i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		if (inst->hit != NULL) {
			clear_hitpattern(inst->hit);
			inst->hit = NULL;
		}
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
	}
	for (i=0;i<npatterns;i++) {
		free_pattern(pattern[i]);
		pattern[i] = NULL;
	}

	/* Ok, start loading fromt the file */
	printf("load from file %s\n", filename);
	f = fopen(filename, "r");
	if (f == NULL) {
		printf("Nope\n");
		return -1;
	}

	rc = fscanf(f, "Gneutronica file format version: %d\n", &fileformatversion);
	if (rc != 1) {
		printf("File does not appear to be a %s file.\n",
			PROGNAME);
		return -1;
	}

	switch (fileformatversion) {
		case 1: rc = load_from_file_version_1(f);
			break;
		default: printf("Unsupported file format version: %d\n", 
			fileformatversion);
			return -1;
	}
	if (rc == 0)
		set_arranger_window_title();
	return rc;
}


int current_file_format_version = 1;
int save_to_file(char *filename)
{
	int i,j;
	FILE *f;	

	f = fopen(filename, "w+");
	if (f == NULL) {
		printf("Nope\n");	
		return -1;
	}
	fprintf(f, "%s file format version: %d\n", PROGNAME,
		current_file_format_version);
	fprintf(f, "Songname: '%s'\n", songname);
	fprintf(f, "Comment:\n");
	fprintf(f, "Drumkit Make: %s\n", drumkit[kit].make);
	fprintf(f, "Drumkit Model: %s\n", drumkit[kit].model);
	fprintf(f, "Drumkit Name: %s\n", drumkit[kit].name);
	fprintf(f, "Instruments: %d\n", drumkit[kit].ninsts);

	/* Save the instrument types, so that translating to another drumkit is possible */ 
	for (i=0;i<drumkit[kit].ninsts;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		gboolean hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(inst->hidebutton));
		fprintf(f, "Instrument %d: '%s' %d\n", i, drumkit[kit].instrument[i].type, (int) hidden);
	}

	/* Save the patterns */
	fprintf(f, "Patterns: %d\n", npatterns);
	for (i=0;i<npatterns;i++) {
		struct hitpattern *h;
		fprintf(f, "Pattern %d: %d %d %s\n", i, pattern[i]->beats_per_measure,
			pattern[i]->beats_per_minute, pattern[i]->patname);
		fprintf(f, "Divisions: %d %d %d %d %d\n", 
			pattern[i]->timediv[0].division,
			pattern[i]->timediv[1].division,
			pattern[i]->timediv[2].division,
			pattern[i]->timediv[3].division,
			pattern[i]->timediv[4].division);
		for (h = pattern[i]->hitpattern; h != NULL; h=h->next) {
			fprintf(f, "T: %g DK: %d I: %d V: %d B:%d BPM:%d\n",
				h->h.time, h->h.drumkit, h->h.instrument_num, h->h.velocity,
				h->h.beat, h->h.beats_per_measure);
		}
		fprintf(f, "END-OF-PATTERN\n");
	}

	fprintf(f, "Measures: %d\n", nmeasures);
	for (i=0;i<nmeasures;i++) {
		fprintf(f, "m:%d np:%d\n", i, measure[i].npatterns);
		if (measure[i].npatterns != 0) {
			for (j=0;j<measure[i].npatterns;j++)
				fprintf(f, "%d ", measure[i].pattern[j]);
			fprintf(f, "\n");
		}
	}

	fprintf(f, "Tempo changes: %d\n", ntempochanges);
	for (i=0;i<ntempochanges;i++)
		fprintf(f, "m: %d bpm: %d\n", 
			tempo_change[i].measure,
			tempo_change[i].beats_per_minute);

	fclose(f);
	return 0;
}

void send_midi_patch_change(int fd, unsigned short bank, unsigned char patch)
{
	/* sends a MIDI bank change and patch change message to a MIDI device */
	unsigned short b;
	char bank_ch[3];
	char patch_ch[2];

	printf("Changing MIDI device to bank %d, patch %d\n", bank, patch);

	bank_ch[0] = 0xb0 | (0x0f & drumkit[kit].midi_channel);
	b = htons(bank); /* put it in big endian order */
	memcpy(&bank_ch[1], &b, sizeof(b)); 

	patch_ch[0] = 0xc0 | (0x0f & drumkit[kit].midi_channel);
	patch_ch[1] = patch;

	write(fd, bank_ch, 3);
	write(fd, patch_ch, 2);

	return;
}

void int_note_on(int fd, unsigned char value, unsigned char volume)
{
	printf("Internal note_on access method not yet implemented.\n");
}

void int_note_off(int fd, unsigned char value, unsigned char volume)
{
	printf("Internal note_off access method not yet implemented.\n");
}

void int_send_midi_patch_change(int fd, unsigned short bank, unsigned char patch)
{
	printf("Internal send_midi_patch_change  access method not yet implemented.\n");
}

void note_on(int fd, unsigned char value, unsigned char volume)
{
	unsigned char data[3];
	/* printf("NOTE ON, value=%d, volume=%d\n", value, volume); */
	data[0] = 0x90 | (drumkit[kit].midi_channel & 0x0f);
	data[1] = value;
	data[2] = volume; 
	write(fd, data, 3); /* This needs to be atomic */
}

void note_off(int fd, unsigned char value, unsigned char volume)
{
	unsigned char data[3];
	/* printf("NOTE OFF, value=%d, volume=%d\n", value, volume); */
	data[0] = 0x80 | (drumkit[kit].midi_channel & 0x0f);
	data[1] = value;
	data[2] = volume; 
	write(fd, data, 3); /* This needs to be atomic */
}

void silence(int fd)
{
	int i;
	for (i=0;i<127;i++)
		access_device->note_off(fd, i, 0);
}

void init_measures()
{
	int i;
	if (measure != NULL)
		free(measure);
	measure = (struct measure_struct *) malloc(sizeof(struct measure_struct) * MAXMEASURES);
	for (i=0;i<MAXMEASURES;i++) {
		measure[i].npatterns = 0;
		memset(measure[i].pattern, -1, (sizeof(int)*MAXPATSPERMEASURE));
	}
}

void send_schedule(struct schedule_t *sched, int loop)
{
	int i, rc;
	unsigned char cmd;

	/* Make sure the player is not busy playing something else... */
	kill(player_process_pid, SIGUSR1);
	usleep(5000);

	if (loop)
		cmd = PLAY_LOOP;
	else
		cmd = PLAY_ONCE;

	rc = write(player_process_fd, &cmd, 1);
	if (rc != 1) {
		fprintf(stderr, "Error writing to player process.\n");
		return;
	}
	rc = write(player_process_fd, &sched->nevents, sizeof(sched->nevents));
	if (rc == -1) {
		fprintf(stderr, "Error writing to player process.\n");
		return;
	}

	for (i=0;i<sched->nevents;i++) {
		rc = write(player_process_fd, sched->e[i], sizeof(*sched->e[i]));
		if (rc == -1) {
			fprintf(stderr, "Error writing to player process.\n");
		}
	}
	return;
}

static jmp_buf the_beginning;

void sigusr1_handler(int signal)
{
	printf("Got SIGUSR1\n");
	siglongjmp(the_beginning, 1); /* Go back to the beginning */
}

void player_process_requests(int fd)
{
	int i, rc;	
	struct sigaction action;
	unsigned char cmd;

	/* Initialize our copy of the schedule to nothing. */
	sched.nevents = 0;
	for (i=0;i<MAXEVENTS;i++)
		sched.e[i] = NULL;

	rc = sigsetjmp(the_beginning, 1);	/* Remember this place, We will come back here */

	memset(&action, 0, sizeof(action));
	action.sa_handler = sigusr1_handler;
	sigemptyset(&action.sa_mask);
	sigaction(SIGUSR1, &action, NULL);

	silence(midi_fd);

	if (rc) {
		free_schedule(&sched);	/* does nothing first time through, but after longjmp . . . */
		sched.nevents = 0;
		for (i=0;i<MAXEVENTS;i++)
			sched.e[i] = NULL;
	}

	while (1) {
		int count = 0;
		errno=0;
		printf("\nPlayer waiting for requests\n");
		/* Read a schedule of events from the main process */
		do {
			rc = read(fd, &cmd, 1);
			if (rc != 0)
				count = 0;
			count++;
			if (count == 20)
				exit(1);
		} while (rc == 0);

		switch (cmd) {
			case PLAY_ONCE:
			case PLAY_LOOP: {
				rc = read(fd, &sched.nevents, sizeof(sched.nevents));
				if (rc == -1 && errno == 2) /* parent process probably quit */
					exit(0); 
				if (rc != sizeof(sched.nevents)) {
					printf("xxx rc = %d, errno = %d, %s\n", rc, errno, strerror(errno));
					exit(1);
				}
				if (rc == -1 && errno == EINTR)
					continue;
				for (i=0;i<sched.nevents;i++) {
					sched.e[i] = malloc(sizeof(*sched.e[i]));
					rc = read(fd, sched.e[i], sizeof(*sched.e[i]));
					if (rc == -1) {
						fprintf(stderr,"Error reading schedule from parent process\n");
						exit(1);
					}
				}
	
				printf("\nPlayer: playing requests\n");	
				/* play the scheduled events */
				do {
					schedule(&sched);
				} while (cmd == PLAY_LOOP); /* SIGUSR1 triggered longjmp gets us out of here. */

				/* Empty the schedule */
				free_schedule(&sched);
				sched.nevents = 0;
				break;
				}
			case PLAYER_QUIT:
				silence(midi_fd);
				exit(0);
			case PERFORM_PATCH_CHANGE: {
				unsigned short bank;
				unsigned char patch;
				read(fd, &bank, sizeof(bank));
				read(fd, &patch, sizeof(patch));
				printf("Player: Changing to bank %d, patch %d\n", bank, patch);
				access_device->send_midi_patch_change(midi_fd, bank, patch);
				break;
			}
			default:
				printf("Player received unknown cmd: %d\n", cmd);
		}
	}
}

int fork_player_process(char *device, int *fd)
{
	int p[2];
	int pid, rc;
	rc = pipe(p);
	if (rc != 0) {
		fprintf(stderr, "Can't create pipe: %s\n", strerror(errno));
		return -1;
	}

	pid = fork();
	if (pid == 0) {
		close(p[1]);
		printf("pid = %d\n", pid);
		/* printf("Opening midi device\n");
		midi_fd = open(device, O_RDWR);
		if (midi_fd < 0) {
			printf("Can't open MIDI file %s, oh well, NO SOUND FOR YOU!!!\n", device);
		} */
		player_process_requests(p[0]);
	}
	printf("parent, pid =%d\n", pid);
	close(p[0]);
	*fd = p[1];
	return(pid);
}

int midi_change_patch(GtkWidget *widget, gpointer data)
{
	/* sends a command to the player process to make it send a bank/patch change to 
	   the MIDI device */
	unsigned short bank;
	unsigned char patch;
	unsigned char cmd = PERFORM_PATCH_CHANGE;
	int rc;

	drumkit[kit].midi_bank = bank;
	drumkit[kit].midi_patch = patch;

	bank = (unsigned short) (gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(midi_bank_spin_button))) & 0x00ffff;
	patch = (unsigned char) (gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(midi_patch_spin_button))) & 0x0f;
	
	rc = write(player_process_fd, &cmd, 1);
	rc = write(player_process_fd, &bank, sizeof(bank));
	rc = write(player_process_fd, &patch, sizeof(patch));
}

int midi_setup_activate(GtkWidget *widget, gpointer data)
{
	gtk_widget_show(midi_setup_window);
	return TRUE;
}

int midi_setup_cancel(GtkWidget *widget, gpointer data)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(midi_channel_spin_button), 
		(gdouble) drumkit[kit].midi_channel);
	gtk_widget_hide(midi_setup_window);
	return TRUE;
}

int midi_setup_ok(GtkWidget *widget, gpointer data)
{
	unsigned char midi_channel;
	drumkit[kit].midi_channel = (unsigned char) (gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(midi_channel_spin_button))) & 0x0f;
	gtk_widget_hide(midi_setup_window);
	return TRUE;
}

void setup_midi_setup_window()
{
	char windowname[100];
	midi_setup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER (midi_setup_window), 15);
	midi_setup_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER (midi_setup_window), midi_setup_vbox);
	midi_setup_hbox1 = gtk_hbox_new(FALSE, 0);
	midi_setup_hbox2 = gtk_hbox_new(FALSE, 0);
	midi_setup_hbox3 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_vbox), midi_setup_hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_vbox), midi_setup_hbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_vbox), midi_setup_hbox3, FALSE, FALSE, 0);

	midi_bank_label = gtk_label_new("MIDI Bank:");
	midi_bank_spin_button = gtk_spin_button_new_with_range(0, 0x0ffff, 1);
	midi_patch_label = gtk_label_new("MIDI Patch:");
	midi_patch_spin_button = gtk_spin_button_new_with_range(0,0xff,1);
	midi_channel_label = gtk_label_new("Transmit on MIDI channel:");
	midi_channel_spin_button = gtk_spin_button_new_with_range(0, 15, 1);
	midi_change_patch_button = gtk_button_new_with_label("Send Change Patch Message to device");
	g_signal_connect(G_OBJECT (midi_change_patch_button), "clicked", 
				G_CALLBACK (midi_change_patch), NULL);
	midi_setup_ok_button = gtk_button_new_with_label(" Ok ");
	midi_setup_cancel_button = gtk_button_new_with_label(" Cancel ");
	g_signal_connect(G_OBJECT (midi_setup_ok_button), "clicked", 
				G_CALLBACK (midi_setup_ok), NULL);
	g_signal_connect(G_OBJECT (midi_setup_cancel_button), "clicked", 
				G_CALLBACK (midi_setup_cancel), NULL);

	gtk_box_pack_start(GTK_BOX(midi_setup_hbox1), midi_bank_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox1), midi_bank_spin_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox1), midi_patch_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox1), midi_patch_spin_button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(midi_setup_hbox1), midi_change_patch_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox2), midi_channel_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox2), midi_channel_spin_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox3), midi_setup_ok_button, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midi_setup_hbox3), midi_setup_cancel_button, TRUE, FALSE, 0);

	sprintf(windowname, "%s MIDI Setup", PROGNAME);
	gtk_window_set_title(GTK_WINDOW(midi_setup_window), windowname);

	g_signal_connect(G_OBJECT (midi_setup_window), "delete_event", 
		// G_CALLBACK (delete_event), NULL);
		G_CALLBACK (midi_setup_cancel), NULL);
	g_signal_connect(G_OBJECT (midi_setup_window), "destroy", 
		G_CALLBACK (midi_setup_cancel), NULL);

	gtk_widget_show_all(midi_setup_vbox);
}

int main(int argc, char *argv[])
{
	GtkWidget *abox;
	GtkWidget *a_button_box;
	GtkWidget *stop_button;
	GtkWidget *save_button;
	GtkWidget *load_button;
	GtkWidget *pattern_play_button;
	GtkWidget *pattern_stop_button;
	GtkWidget *pattern_clear_button;
	GtkWidget *box1, *box2, *middle_box, *linebox;
	GtkWidget *topbox;
	GtkWidget *table;
	

	struct drumkit_struct *dk;
	int i, rc;

	/* ----- open the midi device ------------------------ */
        int fd, c;
        char device[255], drumkitfile[255];


        strcpy(device, "/dev/snd/midi1");
	strcpy(drumkitfile, "drumkits/default_drumkit.dk");
	strcpy(drumkitfile, "drumkits/generic.dk");
	strcpy(drumkitfile, "drumkits/Roland_Dr660_Standard.dk");
	strcpy(drumkitfile, "drumkits/yamaha_motifr_rockst1.dk");
	strcpy(drumkitfile, "/usr/local/share/gneutronica/drumkits/general_midi_standard.dk");
        while ((c = getopt(argc, argv, "k:d:")) != -1) {
                switch (c) {
                case 'd': strcpy(device, optarg); break;
                case 'k': strcpy(drumkitfile, optarg); break;
                }
        }
	fd = open(device, O_RDWR);
	if (fd >= 0) {
		unsigned char bankchange[] = { 0xb0, 0x00, 0x00 };
		unsigned char patchchange[] = { 0xc0, 117};

		midi_fd = fd;

		/* do some ioctl or something here to determine how to access 
		   the device, then set access_device pointer to the correct
		   set of access methods for the device type, like:

		   if (device type is internal)
			access_device = &access_method[INTERNAL_DEVICE];

		   If you get this working with soundfonts on soundcard
		   midi devices, send me a patch. <smcameron@users.sourceforge.net>

		 */
	} else
		printf("Can't open MIDI file %s, oh well, NO SOUND FOR YOU!!!\n", device);
	
	player_process_pid = fork_player_process(device, &player_process_fd);
	pattern = malloc(sizeof(struct pattern_struct *) * MAXPATTERNS);
	memset(pattern, 0, sizeof(struct pattern_struct *) * MAXPATTERNS);
	init_measures();
	npatterns = 0;
	nmeasures = 0;
	cpattern = 0;
	cmeasure = 0;
	ntempochanges = 1;
	tempo_change[0] = initial_change;
	sprintf(songname, "Untitled Song");
	
	rc = read_drumkit(drumkitfile, &ndrumkits, drumkit);
	if (rc != 0) {
		fprintf(stderr, "Can't read drumkit file, "
			"perhaps you need to specify '-k drumkitfile' option?\n");
		exit(1);
	}
	kit = 0;
	dk = &drumkit[kit];

	g_print("Using drumkit: %s %s %s\n", 
		dk->make, dk->model, dk->name);

	gdk_color_parse("white", &whitecolor);
	gdk_color_parse("blue", &bluecolor);
	gdk_color_parse("black", &blackcolor);

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	tooltips = gtk_tooltips_new();

	g_signal_connect(G_OBJECT (window), "delete_event", 
		// G_CALLBACK (delete_event), NULL);
		G_CALLBACK (destroy_event), NULL);
	g_signal_connect(G_OBJECT (window), "destroy", 
		G_CALLBACK (destroy_event), NULL);

	gtk_container_set_border_width(GTK_CONTAINER (window), 15);

	box1 = gtk_vbox_new(FALSE, 0);
	topbox = gtk_hbox_new(FALSE, 0);
	box2 = gtk_hbox_new(FALSE, 0);
	middle_box = gtk_hbox_new(FALSE, 0);
	linebox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER (window), box1);

	pattern_scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (pattern_scroller), 10);
	gtk_widget_set_size_request (pattern_scroller, 
		PSCROLLER_WIDTH, PSCROLLER_HEIGHT);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pattern_scroller),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	table = gtk_table_new(dk->ninsts + 1,  8, FALSE);
	gtk_box_pack_start(GTK_BOX(box1), topbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box1), middle_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(middle_box), pattern_scroller, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(middle_box), linebox, FALSE, FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pattern_scroller), table);
	/* gtk_box_pack_start(GTK_BOX(box1), table, TRUE, TRUE, 0); */
	gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

	hide_instruments = gtk_check_button_new_with_label("Hide unchecked\ninstruments");
	g_signal_connect(G_OBJECT (hide_instruments), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, hide_instruments, 
		"Hide the instruments below which do not have a "
		"check beside them to reduce visual clutter.", NULL);
	hide_volume_sliders = gtk_check_button_new_with_label("Hide instrument\nattributes");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_volume_sliders), TRUE);
	g_signal_connect(G_OBJECT (hide_volume_sliders), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, hide_volume_sliders, 
		"Hide the volume sliders and drag settings which appear to the left of"
		" the instrument buttons, below.", NULL);
	
	drumkit_vbox = gtk_vbox_new(FALSE, 0);
	save_drumkit_button = gtk_button_new_with_label("Save Drum Kit");
	gtk_tooltips_set_tip(tooltips, save_drumkit_button, 
		"Save instrument names, types, and MIDI note "
		"assignments into a file for later re-use.", NULL);
	g_signal_connect(G_OBJECT (save_drumkit_button), "clicked", 
			G_CALLBACK (save_drumkit_button_clicked), NULL);
	edit_instruments_toggle = gtk_toggle_button_new_with_label("Edit Drum Kit");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_instruments_toggle), FALSE);
	gtk_tooltips_set_tip(tooltips, edit_instruments_toggle, 
		"Assign names, types, and MIDI note numbers to instruments.", NULL);
	g_signal_connect(G_OBJECT (edit_instruments_toggle), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	pattern_name_label = gtk_label_new("Pattern:");
	pattern_name_entry = gtk_entry_new();
	gtk_tooltips_set_tip(tooltips, pattern_name_entry, 
		"Assign a name to this pattern.", NULL);
	tempolabel1 = gtk_label_new("Beats/Min");
	tempospin1 = gtk_spin_button_new_with_range(10, 400, 1);
	gtk_tooltips_set_tip(tooltips, tempospin1, "Controls tempo only for single pattern playback, "
			"does not affect the tempo in the context of the song.", NULL);
	tempolabel2 = gtk_label_new("Beats/Measure");
	tempospin2 = gtk_spin_button_new_with_range(1,  400, 1);
	gtk_tooltips_set_tip(tooltips, tempospin2, "Controls tempo for single pattern playback, "
			"and DOES affect the tempo in the context of the song.", NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin1), (gdouble) 120);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin2), (gdouble) 4);

	gtk_box_pack_start(GTK_BOX(topbox), hide_instruments, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), hide_volume_sliders, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), drumkit_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(drumkit_vbox), save_drumkit_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(drumkit_vbox), edit_instruments_toggle, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), pattern_name_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), pattern_name_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempolabel1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempospin1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempolabel2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempospin2, TRUE, TRUE, 0);


	for (i=0;i<dk->ninsts;i++) {
		int col;
		struct instrument_struct *inst = &dk->instrument[i];

		inst->hidebutton = gtk_check_button_new();
		inst->button = gtk_button_new_with_label(inst->name);
		gtk_tooltips_set_tip(tooltips, inst->button, inst->type, NULL);
		inst->drag_spin_button = gtk_spin_button_new_with_range(-10,  10, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(inst->drag_spin_button), 
			(gdouble) 0);
		gtk_tooltips_set_tip(tooltips, inst->drag_spin_button,
			"Set percentage of a beat to drag (or rush) this instrument.  "
			"Use negative numbers for rushing."
			"(Sorry, this doesn't actually do anything yet.)", NULL);
		inst->volume_adjustment = gtk_adjustment_new((gdouble) DEFAULT_VELOCITY, 
			0.0, 127.0, 1.0, 1.0, 0.0);
		inst->volume_slider = gtk_hscale_new(GTK_ADJUSTMENT(inst->volume_adjustment));

		inst->name_entry = gtk_entry_new();
		gtk_tooltips_set_tip(tooltips, inst->name_entry, 
			"Assign a name to this instrument", NULL);
		gtk_entry_set_text(GTK_ENTRY(inst->name_entry), inst->name);
		g_signal_connect (G_OBJECT (inst->name_entry), "activate",
		      G_CALLBACK (instrument_name_entered), (gpointer) inst);

		inst->type_entry = gtk_entry_new();
		gtk_tooltips_set_tip(tooltips, inst->type_entry, 
			"Assign a type to this instrument", NULL);
		gtk_entry_set_text(GTK_ENTRY(inst->type_entry), inst->type);
		g_signal_connect (G_OBJECT (inst->type_entry), "activate",
		      G_CALLBACK (instrument_type_entered), (gpointer) inst);

		inst->midi_value_spin_button = gtk_spin_button_new_with_range(0, 127, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(inst->midi_value_spin_button), 
			(gdouble) inst->midivalue);
		g_signal_connect(G_OBJECT (inst->midi_value_spin_button), "value-changed", 
				G_CALLBACK (unsigned_char_spin_button_change), &inst->midivalue);
		gtk_tooltips_set_tip(tooltips, inst->midi_value_spin_button, 
			"Assign the MIDI note for this instrument", NULL);

		gtk_widget_set_size_request(inst->volume_slider, 80, 33);
		gtk_scale_set_digits(GTK_SCALE(inst->volume_slider), 0);
		gtk_tooltips_set_tip(tooltips, inst->volume_slider, 
			"Controls the default volume for this insrument.", NULL);

		inst->canvas = gtk_drawing_area_new();
		g_signal_connect(G_OBJECT (inst->button), "clicked", 
				G_CALLBACK (hello), inst);
		g_signal_connect(G_OBJECT (inst->canvas), "expose_event",
				G_CALLBACK (canvas_event), inst);
		gtk_widget_add_events(inst->canvas, GDK_BUTTON_PRESS_MASK); 
		gtk_widget_add_events(inst->canvas, GDK_BUTTON_RELEASE_MASK); 
		g_signal_connect(G_OBJECT (inst->canvas), "button-release-event",
				G_CALLBACK (canvas_clicked), inst);
		gtk_widget_set_size_request(inst->canvas, DRAW_WIDTH+1, DRAW_HEIGHT+1);
		// gtk_widget_set_usize(canvas, 400, 10);

		col = 0;
		gtk_table_attach(GTK_TABLE(table), inst->hidebutton, 
			col, col+1, i, i+1, 0, 0, 0,0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->drag_spin_button,
			col, col+1, i, i+1, 0, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->volume_slider,
			col, col+1, i, i+1, 0, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->button, 
			col, col+1, i, i+1,
			GTK_FILL,
			0,
			0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->name_entry,
			col, col+1, i, i+1, GTK_FILL, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->type_entry,
			col, col+1, i, i+1, GTK_FILL, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->midi_value_spin_button,
			col, col+1, i, i+1, GTK_FILL, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->canvas, 
			col, col+1, i, i+1, 0, 0, 0, 0); col++;
	}

	for (i=0;i<ndivisions;i++) {
		timediv[i].spin = gtk_spin_button_new_with_range(0, 300, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(timediv[i].spin), (gdouble) timediv[i].division);
		g_signal_connect(G_OBJECT(timediv[i].spin), "value-changed", 
			G_CALLBACK(timediv_spin_change), &timediv[i]);
		gtk_tooltips_set_tip(tooltips, timediv[i].spin, 
			"Use this to control the way the measure is divided up for placement of beats", 
			NULL);

		/* gtk_table_attach_defaults(GTK_TABLE(table), timediv[i].spin, 
			2, 3, i, i+1); */
		gtk_box_pack_start(GTK_BOX(linebox), timediv[i].spin, FALSE, FALSE, 0);
	}

	prevbutton = gtk_button_new_with_label("<- Edit Previous Pattern");
	nextbutton = gtk_button_new_with_label("Create Next Pattern ->");
	pattern_clear_button = gtk_button_new_with_label("Clear Pattern");
	pattern_paste_button = gtk_button_new_with_label("Paste Pattern");
	pattern_play_button = gtk_button_new_with_label("Play");
	pattern_stop_button = gtk_button_new_with_label("Stop");

	g_signal_connect(G_OBJECT (nextbutton), "clicked",
			G_CALLBACK (nextbutton_clicked), NULL);
	g_signal_connect(G_OBJECT (prevbutton), "clicked",
			G_CALLBACK (prevbutton_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_clear_button), "clicked",
			G_CALLBACK (pattern_clear_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_paste_button), "clicked",
			G_CALLBACK (pattern_paste_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_play_button), "clicked",
			G_CALLBACK (pattern_play_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_stop_button), "clicked",
			G_CALLBACK (pattern_stop_button_clicked), NULL);

	gtk_tooltips_set_tip(tooltips, nextbutton, "Create and edit the next pattern", NULL);
	gtk_tooltips_set_tip(tooltips, prevbutton, "Edit the previous pattern", NULL);
	gtk_tooltips_set_tip(tooltips, pattern_clear_button, "Clear all notes from this pattern", NULL);
	gtk_tooltips_set_tip(tooltips, pattern_paste_button, 
		"Superimpose all the notes of a previously selected pattern onto this pattern.", NULL);
	gtk_tooltips_set_tip(tooltips, pattern_play_button, 
		"Send this pattern to MIDI device for playback", NULL);
	gtk_tooltips_set_tip(tooltips, pattern_stop_button, 
		"Stop any playback currently in progress.", NULL);

	gtk_box_pack_start(GTK_BOX(box2), prevbutton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_play_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_stop_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_clear_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_paste_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), nextbutton, TRUE, TRUE, 0);

	

	/* ---------------- arranger window ------------------ */
	arranger_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER (arranger_window), 15);

	/* 1 row, 2 colums, 1 row per pattern, will resize table as necc.
	   First column is the pattern name, 2nd column is a drawing
	   area that allows the measures to be specified. */

	arranger_scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (arranger_scroller, 750, ARRANGER_HEIGHT);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (arranger_scroller),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	arranger_table = gtk_table_new(6, ARRANGER_COLS, FALSE);

	TempoLabel = gtk_label_new("Tempo changes ->");
	SelectButton = gtk_button_new_with_label("Select measures ->");
	PasteLabel = gtk_label_new("Paste measures ->");
	InsertButton = gtk_button_new_with_label("Insert measures ->:");
	DeleteButton = gtk_button_new_with_label("Delete measures ->");
	
	gtk_tooltips_set_tip(tooltips, SelectButton, 
		"Click this button to select all measures, or twice to select no measures.  "
		"Click the buttons to the right to select a single measure.  "
		"Click, drag, and release over the buttons to the right to select a range of measures.",
		NULL);
	gtk_tooltips_set_tip(tooltips, InsertButton, 
		"Click the buttons to the right to insert a single measure, "
		 "or use this button to insert blank measures for the selected measures.", NULL);
	gtk_tooltips_set_tip(tooltips, DeleteButton, 
		"Click the buttons to the right to delete a single measure, "
		 "or use this button to delete the selected measures.", NULL);

	g_signal_connect(G_OBJECT (SelectButton), "clicked",
			G_CALLBACK (select_measures_button), NULL);
	g_signal_connect(G_OBJECT (InsertButton), "clicked",
			G_CALLBACK (insert_measures_button), NULL);
	g_signal_connect(G_OBJECT (DeleteButton), "clicked",
			G_CALLBACK (delete_measures_button), NULL);

	gtk_table_attach(GTK_TABLE(arranger_table), TempoLabel, 3, 4, 0, 1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), SelectButton, 3, 4, 1, 2, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), PasteLabel, 3, 4, 2, 3, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), InsertButton, 3, 4, 3, 4, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), DeleteButton, 3, 4, 4, 5, 0, 0, 0, 0);

	gtk_widget_show(TempoLabel);
	gtk_widget_show(SelectButton);
	gtk_widget_show(PasteLabel);
	gtk_widget_show(InsertButton);
	gtk_widget_show(DeleteButton);

	Tempo_da = gtk_drawing_area_new();
	Copy_da = gtk_drawing_area_new();
	Paste_da = gtk_drawing_area_new();
	Insert_da = gtk_drawing_area_new();
	Delete_da = gtk_drawing_area_new();

	gtk_widget_add_events(Tempo_da, GDK_BUTTON_PRESS_MASK); 
	gtk_widget_add_events(Copy_da, GDK_BUTTON_PRESS_MASK); 
	gtk_widget_add_events(Paste_da, GDK_BUTTON_PRESS_MASK); 
	gtk_widget_add_events(Insert_da, GDK_BUTTON_PRESS_MASK); 
	gtk_widget_add_events(Delete_da, GDK_BUTTON_PRESS_MASK); 

	gtk_widget_add_events(Tempo_da, GDK_BUTTON_RELEASE_MASK); 
	gtk_widget_add_events(Copy_da, GDK_BUTTON_RELEASE_MASK); 
	gtk_widget_add_events(Paste_da, GDK_BUTTON_RELEASE_MASK); 
	gtk_widget_add_events(Insert_da, GDK_BUTTON_RELEASE_MASK); 
	gtk_widget_add_events(Delete_da, GDK_BUTTON_RELEASE_MASK); 

	g_signal_connect(G_OBJECT (Tempo_da), "expose_event", G_CALLBACK (measure_da_expose), NULL);
	g_signal_connect(G_OBJECT (Copy_da), "expose_event", G_CALLBACK (measure_da_expose), NULL);
	g_signal_connect(G_OBJECT (Paste_da), "expose_event", G_CALLBACK (measure_da_expose), NULL);
	g_signal_connect(G_OBJECT (Insert_da), "expose_event", G_CALLBACK (measure_da_expose), NULL);
	g_signal_connect(G_OBJECT (Delete_da), "expose_event", G_CALLBACK (measure_da_expose), NULL);

	g_signal_connect(G_OBJECT (Tempo_da), "button-release-event",
			G_CALLBACK (measure_da_clicked), NULL);
	g_signal_connect(G_OBJECT (Copy_da), "button-press-event",
			G_CALLBACK (copy_measure_press), NULL);
	g_signal_connect(G_OBJECT (Copy_da), "button-release-event",
			G_CALLBACK (copy_measure_release), NULL);
	g_signal_connect(G_OBJECT (Paste_da), "button-release-event",
			G_CALLBACK (measure_da_clicked), NULL);
	g_signal_connect(G_OBJECT (Insert_da), "button-release-event",
			G_CALLBACK (measure_da_clicked), NULL);
	g_signal_connect(G_OBJECT (Delete_da), "button-release-event",
			G_CALLBACK (measure_da_clicked), NULL);

	gtk_table_attach(GTK_TABLE(arranger_table), Tempo_da, 4, 5, 0, 1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), Copy_da, 4, 5, 1, 2, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), Paste_da, 4, 5, 2, 3, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), Insert_da, 4, 5, 3, 4, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), Delete_da, 4, 5, 4, 5, 0, 0, 0, 0);

	gtk_tooltips_set_tip(tooltips, Tempo_da, 
		"Click the buttons to the right to insert tempo changes", NULL);
	gtk_tooltips_set_tip(tooltips, Copy_da, 
		"Click the buttons to the right to copy a measure", NULL);
	gtk_tooltips_set_tip(tooltips, Paste_da, 
		"Click the buttons to the right to insert a previously copied measure", NULL);
	gtk_tooltips_set_tip(tooltips, Insert_da, 
		"Click the buttons to the right to insert a new blank measure", NULL);
	gtk_tooltips_set_tip(tooltips, Delete_da, 
		"Click the buttons to the right to delete a measure", NULL);

	gtk_widget_set_size_request(Tempo_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Copy_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Paste_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Insert_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Delete_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);

	gtk_widget_show(Tempo_da);
	gtk_widget_show(Copy_da);
	gtk_widget_show(Paste_da);
	gtk_widget_show(Insert_da);
	gtk_widget_show(Delete_da);

	song_name_label = gtk_label_new("Song:");
	song_name_entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (song_name_entry), "activate",
		      G_CALLBACK (song_name_entered), (gpointer) song_name_entry);
	arr_loop_check_button = gtk_check_button_new_with_label("Loop");
	gtk_tooltips_set_tip(tooltips, arr_loop_check_button, 
		"When checked, will cause playback to loop until 'Stop' is pressed.", NULL);
	midi_setup_activate_button = gtk_button_new_with_label("MIDI Setup");
	gtk_tooltips_set_tip(tooltips, midi_setup_activate_button,
		"Set the MIDI channel to transmit on, and send "
		"MIDI patch change messages.", NULL);
	g_signal_connect(G_OBJECT (midi_setup_activate_button), "clicked",
			G_CALLBACK (midi_setup_activate), NULL);
	abox = gtk_vbox_new(FALSE, 0);
	a_button_box = gtk_hbox_new(FALSE, 0);
	arranger_box = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER (arranger_window), abox);
	gtk_box_pack_start(GTK_BOX(abox), arranger_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), song_name_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), song_name_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), arr_loop_check_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), midi_setup_activate_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(abox), arranger_scroller, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(arranger_scroller), 
		arranger_table);
	gtk_box_pack_start(GTK_BOX(abox), a_button_box, FALSE, FALSE, 0);
	play_button = gtk_button_new_with_label("Play");
	gtk_tooltips_set_tip(tooltips, play_button, 
		"Send this song to MIDI device for playback", NULL);
	play_selection_button = gtk_button_new_with_label("Play Selection");
	gtk_tooltips_set_tip(tooltips, play_selection_button, 
		"Send selected measures to MIDI device for playback", NULL);
	stop_button = gtk_button_new_with_label("Stop");
	gtk_tooltips_set_tip(tooltips, stop_button, "Stop any MIDI playback in progress", NULL);
	save_button = gtk_button_new_with_label("Save");
	gtk_tooltips_set_tip(tooltips, save_button, "Save this song to a file", NULL);
	load_button = gtk_button_new_with_label("Load");
	gtk_tooltips_set_tip(tooltips, load_button, 
		"Discard the current song and load another one from a file.", NULL);
	quitbutton = gtk_button_new_with_label("Quit");
	gtk_tooltips_set_tip(tooltips, quitbutton, 
		"Discard the current song and quit Gneutronica.", NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), play_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (play_button), "clicked",
			G_CALLBACK (arranger_play_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), play_selection_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (play_selection_button), "clicked",
			G_CALLBACK (arranger_play_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), stop_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (stop_button), "clicked",
			G_CALLBACK (pattern_stop_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), save_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (save_button), "clicked", 
			G_CALLBACK (save_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), load_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (load_button), "clicked", 
			G_CALLBACK (load_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), quitbutton, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (quitbutton), "clicked", 
			G_CALLBACK (destroy_event), NULL);
	g_signal_connect(G_OBJECT (arranger_window), "delete_event", 
		// G_CALLBACK (delete_event), NULL);
		G_CALLBACK (destroy_event), NULL);
	g_signal_connect(G_OBJECT (arranger_window), "destroy", 
		G_CALLBACK (destroy_event), NULL);

	make_tempo_change_editor();

	set_pattern_window_title();
	set_arranger_window_title();

	setup_midi_setup_window();
	/* ---------------- start showing stuff ------------------ */
	gtk_widget_show_all(arranger_window);
	gtk_widget_show_all(window);
	gc = gdk_gc_new(dk->instrument[0].canvas->window);

	for (i=0;i<drumkit[kit].ninsts;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		gtk_widget_hide(inst->name_entry);
		gtk_widget_hide(inst->type_entry);
		gtk_widget_hide(inst->midi_value_spin_button);
		gtk_widget_hide(inst->volume_slider);
		gtk_widget_hide(inst->drag_spin_button);
	}

	flatten_pattern(kit, cpattern);
	gtk_main();

	if (fd > 0) 
		close(fd);

	exit(0);
}
