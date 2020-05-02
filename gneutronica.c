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
#define COPYRIGHT "(c) Copyright 2005,2006, Stephen M. Cameron\n\n" \
"This program is free software; you can redistribute it and/or modify it under the terms\n" \
"of the GNU General Public License as published by the Free Software Foundation; either \n" \
"version 2 of the License, or (at your option) any later version.\n" \
"\n" \
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n" \
"without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n" \
"See the GNU General Public License for more details.\n\n" \
"You should have received a copy of the GNU General Public License along with this program;\n" \
"if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,\n" \
"Boston, MA  02111-1307  USA\n"

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
#include <errno.h>

#define UNUSED __attribute__((unused))

extern double trunc(double x); /* math.h doesn't have this?  What? */

#include <signal.h>
#include <setjmp.h>
#include <netinet/in.h> /* . . . just for for htons() */
#include <sys/mman.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

// #define GNEUTRONICA_FRENCH 1
#include "lang.h"

#define INSTANTIATE_GNEUTRONICA_GLOBALS
#include "gneutronica.h"
#include "sched.h"
#include "old_fileformats.h"
#include "fractions.h"
#include "drumtab.h"
#include "midi_reader.h"

#include "midioutput.h"
#include "midioutput_raw.h"
#include "midioutput_alsa.h"
/* struct midi_method *midi = &midi_method_raw; */
struct midi_method *midi = &midi_method_alsa;
struct midi_handle *midi_handle = NULL;
#define MIDI_CHANNEL (drumkit[kit].midi_channel & 0x0f)

#include "version.h"

/* commands to send player process */
#define PLAY_ONCE 0
#define PLAY_LOOP 1
#define PLAYER_QUIT 2
#define PERFORM_PATCH_CHANGE 4

void print_hello()
{
	printf("hello\n");
}

void load_button_clicked(GtkWidget *widget, gpointer data);
void save_button_clicked(GtkWidget *widget, gpointer data);
void import_patterns_button_clicked(GtkWidget *widget, gpointer data);
void import_drumtab_button_clicked(GtkWidget *widget, gpointer data);
void factor_drumtab_button_clicked(GtkWidget *widget, gpointer data);
static void paste_drumtab_clicked(GtkWidget *widget, gpointer data);
int about_activate(GtkWidget *widget, gpointer data);
void export_midi_button_clicked(GtkWidget *widget, gpointer data);
void remap_drumkit_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data);
void trigger_about_activate(GtkWidget *widget, gpointer data);
void destroy_event(GtkWidget *widget, gpointer data);
void widget_exclude_keypress(GtkWidget *w);
int unflatten_pattern(int ckit, int cpattern);
int translate_drumtab_data(int factor);
void pattern_record_button_clicked(GtkWidget *widget, gpointer data);
void pattern_play_button_clicked(GtkWidget *widget, gpointer data);
int import_patterns_from_file(const char *filename);
int save_to_file(const char *filename);
int export_to_midi_file(const char *filename);
int find_tempo(int measure);
static void remove_tempo_change(int index);
static void redraw_arranger(void);
int insert_tempo_change(int measure, int tempo);

static int get_drawing_width(void)
{
	if (pattern_scroller)
		return (int) (pattern_scroller->allocation.width * 0.8);
	return 800;
}

#define DRAW_WIDTH (get_drawing_width())

/* Main menu items.  Almost all of this menu code was taken verbatim from the 
   gtk tutorial at http://www.gtk.org/tutorial/sec-itemfactoryexample.html
   I tweaked it tweaked a bit for style and menu content, but that's about it.*/

static GtkItemFactoryEntry menu_items[] = {
	{ "/" FILE_LABEL, NULL, NULL, 0, "<Branch>", NULL },
	/* { "/File/_New", "<control>N", print_hello, 0, "<StockItem>", GTK_STOCK_NEW }, */
	{ "/" FILE_LABEL "/_" OPEN_LABEL, "<control>O", load_button_clicked, 0, "<StockItem>", GTK_STOCK_OPEN },
	{ "/" FILE_LABEL "/_" SAVE_LABEL, "<control>S", save_button_clicked, 0, "<StockItem>", GTK_STOCK_SAVE },
	{ "/" FILE_LABEL "/" SAVE_AS_LABEL, NULL, save_button_clicked, 0, "<Item>", NULL },
	{ "/" FILE_LABEL "/sep1", NULL, NULL, 0, "<Separator>", NULL },
	{ "/" FILE_LABEL "/" IMPORT_PATTERNS_LABEL, NULL, import_patterns_button_clicked, 0, "<Item>", NULL },
	{ "/" FILE_LABEL "/" IMPORT_DRUM_TAB_LABEL, NULL, import_drumtab_button_clicked, 0, "<Item>", NULL },
	{ "/" FILE_LABEL "/" EXPORT_TO_MIDI_FILE_LABEL, NULL, export_midi_button_clicked, 0, "<Item>", NULL },
	/* { "/File/_Quit", "<CTRL>Q", gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT }, */
	{ "/" FILE_LABEL "/" QUIT_LABEL,    "<CTRL>Q", destroy_event, 0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/" EDIT_LABEL, NULL, NULL, 0, "<Branch>", NULL },
	{ "/" EDIT_LABEL "/" PASTE_DRUM_TAB_LABEL, NULL, paste_drumtab_clicked, 0, "<Item>", NULL },
	{ "/" EDIT_LABEL "/" REMAP_DRUM_KIT_MENU_LABEL, NULL, remap_drumkit_clicked, 0, "<Item>", NULL },
	{ "/" HELP_LABEL, NULL, NULL, 0, "<LastBranch>", NULL },
	{ "/" HELP_LABEL "/" ABOUT_LABEL, NULL, trigger_about_activate, 0, "<Item>", NULL },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

GtkWidget *main_menubar, *main_popup_button, *main_option_menu;

/* Returns a menubar widget made from the above menu */
static GtkWidget *get_menubar_menu( GtkWidget *window )
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	/* Make an accelerator group (shortcut keys) */
	accel_group = gtk_accel_group_new ();

	/* Make an ItemFactory (that makes a menubar) */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
				       accel_group);

	/* This function generates the menu items. Pass the item factory,
	the number of items in the array, the array itself, and any
	callback data for the the menu items. */
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

	/* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}

/* Popup the menu when the popup button is pressed */
static gboolean popup_cb(UNUSED GtkWidget *widget, GdkEvent *event, GtkWidget *menu)
{
	GdkEventButton *bevent = (GdkEventButton *)event;

	/* Only take button presses */
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	/* Show the menu */
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL,
		NULL, NULL, bevent->button, bevent->time);

	return TRUE;
}

/* Same as with get_menubar_menu() but just return a button with a signal to
   call a popup menu */
GtkWidget *get_popup_menu( void )
{
	GtkItemFactory *item_factory;
	GtkWidget *button, *menu;

	/* Same as before but don't bother with the accelerators */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
	menu = gtk_item_factory_get_widget (item_factory, "<main>");

	/* Make a button to activate the popup menu */
	button = gtk_button_new_with_label ("Popup");
	/* Make the menu popup when clicked */
	g_signal_connect (G_OBJECT(button), "event", G_CALLBACK(popup_cb), 
		(gpointer) menu);

	return button;
}

/* Same again but return an option menu */
GtkWidget *get_option_menu( void )
{
	GtkItemFactory *item_factory;
	GtkWidget *option_menu;

	/* Same again, not bothering with the accelerators */
	item_factory = gtk_item_factory_new (GTK_TYPE_OPTION_MENU, "<main>", NULL);
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
	option_menu = gtk_item_factory_get_widget (item_factory, "<main>");

	return option_menu;
}
/*  . . . End of code copied from gtk tutorial. */

int read_drumkit_fileformat_1_or_2(char *filename, FILE *f, int *ndrumkits, 
	struct drumkit_struct *drumkit, int format)
{
	struct drumkit_struct *dk;
	int i;
	int rc, line, n;

	if (format == 1)
		fprintf(stderr, "Drumkit file %s is old file format version %d\n" 
			"and contains no instrument mapping info\n", 
			filename, format);
	line = 1;
	dk = &drumkit[*ndrumkits];
	dk->ninsts = 0;
	rc = fscanf(f, "%[^,]%*c %[^,]%*c %[^\n]%*c", dk->make, dk->model, dk->name);
	/* g_print("rc = %d\n", rc); */
	if (rc != 3) {
		fprintf(stderr, "Error in %s (make/model/name) at line %d, rc = %d\n", filename, line, rc);
		pclose(f);
		return -1;
	}
	fprintf(stderr, "Reading drumkit %s %s %s\n", dk->make, dk->model, dk->name);

	dk->instrument = malloc(sizeof(struct instrument_struct)*MAXINSTS);
	if (dk->instrument == NULL) {
		fprintf(stderr, "Out of memory\n");
		pclose(f);
		return -1;
	}
	memset(dk->instrument, 0, sizeof(struct instrument_struct)*MAXINSTS);
	
	for (i=0;i<MAXINSTS;i++) {
		strcpy(dk->instrument[i].name, "x");
		strcpy(dk->instrument[i].type, "x");
		dk->instrument[i].midivalue = i;
		dk->instrument[i].gm_equivalent = i;
		dk->instrument[i].hit = NULL;
		dk->instrument[i].button = NULL;
		dk->instrument[i].hidebutton = NULL;
		dk->instrument[i].canvas = NULL;
		dk->instrument[i].instrument_num = i;
	}

	n = 0;
	line++;
	while (!feof(f)) {
		switch(format) {
		case 2: rc = fscanf(f, "%[^,]%*c %[^,]%*c %hhu %d\n", 
				dk->instrument[n].name, 
				dk->instrument[n].type,
				&dk->instrument[n].midivalue,
				&dk->instrument[n].gm_equivalent);
			if (rc != 4) {
				fprintf(stderr, "Error in %s instrument at line %d, rc = %d\n", filename, line, rc);
				pclose(f);
				return -1;
			}
			break;
		case 1: rc = fscanf(f, "%[^,]%*c %[^,]%*c %hhu\n", 
				dk->instrument[n].name, 
				dk->instrument[n].type,
				&dk->instrument[n].midivalue);
			if (rc != 3) {
				fprintf(stderr, "Error in %s at line %d\n", filename, line);
				pclose(f);
				return -1;
			}
			dk->instrument[n].gm_equivalent = -1;
			break;
		default:fprintf(stderr, "Error in %s at line %d\n", filename, line);
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
	(*ndrumkits)++;
	pclose(f);
	return 0;
}

int make_default_drumkit(int *ndrumkits, struct drumkit_struct *drumkit)
{
	struct drumkit_struct *dk;
	int i;

	dk = &drumkit[*ndrumkits];
	dk->ninsts = MAXINSTS;
	strcpy(dk->make, "No name");
	strcpy(dk->model, "No name");
	strcpy(dk->name, "No name");

	if (dk->instrument != NULL)
		free(dk->instrument);

	dk->instrument = malloc(sizeof(struct instrument_struct)*MAXINSTS);
	if (dk->instrument == NULL) {
		fprintf(stderr, "Out of memory\n");
		return -1;
	}
	memset(dk->instrument, 0, sizeof(struct instrument_struct)*MAXINSTS);

	for (i=0;i<MAXINSTS;i++) {

		/* No space in name so editing later is easier, double click instead of triple click
		   to highlight and replace with real name. */
		sprintf(dk->instrument[i].name, "Instrument%d", i);  
		sprintf(dk->instrument[i].type, "description");
		dk->instrument[i].midivalue = i;
		dk->instrument[i].instrument_num = i;
		dk->instrument[i].gm_equivalent = -1;
		dk->instrument[i].hit = NULL;
		dk->instrument[i].button = NULL;
		dk->instrument[i].hidebutton = NULL;
		dk->instrument[i].canvas = NULL;
	}
	(*ndrumkits)++;
	return 0;
}

#define DEFAULT_DRUMKIT_DIR "/usr/local/share/gneutronica/drumkits"

int read_drumkit(char *filename, int *ndrumkits, struct drumkit_struct *drumkit)
{
	FILE *f;
	int rc;
	int fileformat;
	char cmd[255];
	char realfilename[300];
	struct stat statbuf;

	strncpy(realfilename, filename, 300);

	rc = stat(realfilename, &statbuf);
	if (rc != 0 && (strlen(DEFAULT_DRUMKIT_DIR) + strlen(filename) + 2) < 300) {
		/* try putting DEFAULT_DRUMKIT_DIR n the front and see if that helps. */
		sprintf(realfilename, "%s/%s", DEFAULT_DRUMKIT_DIR, filename);
		rc = stat(realfilename, &statbuf);
		if (rc != 0) /* Nope... put it back, and let it fail normally */
			strncpy(realfilename, filename, 300);
	}

	if (*ndrumkits >= MAXKITS)
		return -1;

	sprintf(cmd, "grep -v '^#' %s", realfilename);
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
	case 1: 
	case 2: rc = read_drumkit_fileformat_1_or_2(filename, f, ndrumkits, drumkit, fileformat);
		break;
	default: printf("Unknown drumkit fileformat version %d\n", fileformat);
		rc = -1;
		break;
	}
	return rc;
}

static int save_drumkit_to_file(const char *filename)
{
	struct drumkit_struct *dk;
	FILE *f;
	int i, fileformat = 2;

	printf("save_drumkit_to_file called\n");

	dk = &drumkit[kit];

	f = fopen(filename, "w");
	if (f == NULL) 
		return -1;

	fprintf(f, "Gneutronica drumkit file format %d\n", fileformat);
	fprintf(f, "%s, %s, %s\n", dk->make, dk->model, dk->name);
	for (i=0;i<dk->ninsts;i++) {
		fprintf(f, "%s, %s, %d %d\n",  
			dk->instrument[i].name, 
			dk->instrument[i].type,
			dk->instrument[i].midivalue,
			dk->instrument[i].gm_equivalent);
	}
	fclose(f);
	return 0;
}

void destroy_event(GtkWidget *widget, gpointer data);
void cleanup_tempo_changes();

void tempo_change_ok_button(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	int new_tempo;
	printf("Tempo change ok button\n");
	new_tempo = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(TempoChBPM));
	insert_tempo_change(changing_tempo_measure, new_tempo);
	cleanup_tempo_changes();
	gtk_widget_queue_draw(Tempo_da);
	gtk_widget_hide(TempoChWin);
}

void tempo_change_cancel_button(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	printf("Tempo change cancel button\n");
	cleanup_tempo_changes();
	gtk_widget_hide(TempoChWin);
	gtk_widget_queue_draw(Tempo_da);
}

void tempo_change_delete_button(UNUSED GtkWidget *widget, UNUSED gpointer data)
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

void destroy_means_hide(GtkWidget *widget, UNUSED gpointer data)
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
	widget_exclude_keypress(TempoChBPM);
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
void clear_hitpattern(struct hitpattern *p);

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
	if (p->ins_button != NULL)
		gtk_widget_destroy(GTK_WIDGET(p->ins_button));
		// gtk_object_unref(GTK_OBJECT(p->arr_button));

	/* FIXME: shouldn't we call clear_hitpattern(p->hitpattern) here? */
	clear_hitpattern(p->hitpattern);
	free(p);
}

void edit_pattern(int new_pattern);
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

	total_rows = total_rows + 6;  /* tempo/del/ins/copy/paste/transport */
	top = total_rows -1;

	p->arr_button = gtk_button_new_with_label(p->patname);
	gtk_tooltips_set_tip(tooltips, p->arr_button, EDIT_PATTERN_TIP, NULL);
	p->copy_button = gtk_button_new_with_label("Sel");
	sprintf(msg, SELECT_PATTERN_TIP, p->patname);
	gtk_tooltips_set_tip(tooltips, p->copy_button, msg, NULL);
	p->ins_button = gtk_button_new_with_label("Ins");
	gtk_tooltips_set_tip(tooltips, p->ins_button, 
		INSERT_PATTERN_TIP,NULL);
	p->del_button = gtk_button_new_with_label("Del");
	gtk_tooltips_set_tip(tooltips, p->del_button, 
		DELETE_PATTERN_TIP, NULL);
	p->arr_darea = gtk_drawing_area_new();
	gtk_widget_modify_bg(p->arr_darea, GTK_STATE_NORMAL, &whitecolor);
	gtk_tooltips_set_tip(tooltips, p->arr_darea, ASSIGN_P_TO_M_TIP, NULL);
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
	/* gtk_widget_show_all(arranger_window); */
	gtk_widget_queue_draw(arranger_table);
	return;
}

int lowlevel_add_hit(struct hitpattern **hit,
		int dkit, int pattern, int instnum, 
		int beat, int beats_per_measure, 
		int noteoff_beat, int noteoff_beats_per_measure, 
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
		this->h.noteoff_beat = noteoff_beat;
		this->h.noteoff_beats_per_measure = noteoff_beats_per_measure;
		this->h.noteoff_time = (double) noteoff_beat / (double) noteoff_beats_per_measure;
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
			this->h.noteoff_beat = noteoff_beat;
			this->h.noteoff_beats_per_measure = noteoff_beats_per_measure;
			this->h.noteoff_time = (double) noteoff_beat / (double) noteoff_beats_per_measure;
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
	this->h.noteoff_beat = noteoff_beat;
	this->h.noteoff_beats_per_measure = noteoff_beats_per_measure;
	this->h.noteoff_time = (double) noteoff_beat / (double) noteoff_beats_per_measure;
	this->h.drumkit = dkit;
	this->h.pattern = pattern;
	this->h.instrument_num = instnum;
	prev->next = this;
	return 0;
}

void remove_hit(struct hitpattern **hit, 
		double thetime, double measurelength, 
		UNUSED int dkit, UNUSED int instnum, UNUSED int pattern)
{

	struct hitpattern *prev, *this, *next, *matchprev;
	
	double distance, mindist,  percent;

	percent = thetime / measurelength;
	matchprev = prev = NULL;
	mindist = DRAW_WIDTH * 2.0; 
	for (this = *hit; this != NULL; this = next) {
		next = this->next;
		distance = this->h.time * measurelength - percent * measurelength;
		if (distance < 0) 
			distance = -distance;
		if (distance < mindist) {
			mindist = distance;
			matchprev = prev;
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

void find_bestbeat(double percent, double measurelength, 
	int *bestbeat, int *bestdivision)
{
	int i;
	double zero, diff, bestdiff;
	double target, divlen, x;

	*bestbeat = -1;
	*bestdivision = -1;
	bestdiff = 1e100; /* absurdly large initial value to shut scan-build up. */

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
			*bestbeat = (int) target;
			*bestdivision = (int) zero;
			/* bestpercent = (target / zero); */
			/* g_print("i=%d, bestdiff = %g\n", i, bestdiff); */
		}
	}
	return;
}


int add_hit(struct hitpattern **hit, 
		double thetime, double duration, double measurelength, 
		int dkit, int instnum, int tpattern, unsigned char velocity, 
		int change_velocity)
{
	/* figure out the nearest place */
	int noteoff_bestbeat = -1;
	int bestbeat = -1;
	int noteoff_best_division, best_division;
	int stg;
	double noteoff_percent, percent;

	if (thetime > measurelength || thetime < 0)
		return 0;

	percent = thetime / measurelength;
	noteoff_percent = (thetime+duration) / measurelength;

	stg = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (snap_to_grid));
	
	/* g_print("percent = %g\n", percent); */

	if (stg) {

		find_bestbeat(percent, measurelength, &bestbeat, &best_division);
		if (bestbeat == -1) {
			bestbeat = (int) thetime; 
			best_division = (int) measurelength;
		}
		find_bestbeat(noteoff_percent, measurelength, 
			&noteoff_bestbeat, &noteoff_best_division);
		if (noteoff_bestbeat == -1) {
			noteoff_bestbeat = (int) (thetime + duration);
			noteoff_best_division = (int) measurelength;
		}
	} else {
		bestbeat = (int) thetime; /* cast should be ok, it's mouse coord */
		best_division = (int) measurelength;
		noteoff_bestbeat = (int) (thetime + duration);
		noteoff_best_division = best_division;
	}

	reduce_fraction(&bestbeat, &best_division);
	reduce_fraction(&noteoff_bestbeat, &noteoff_best_division);

	/* If there are 16 beats, that's 0 thru 15, beat 16 belongs to the next measure... */
	if (bestbeat == best_division) {
		/* g_print("Rejecting beat %d of %d beats\n",
			bestbeat+1, best_division); */
		return 0;
	}
	return (lowlevel_add_hit(hit, dkit, tpattern, instnum, bestbeat, best_division, 
			noteoff_bestbeat, noteoff_best_division,
			velocity, change_velocity));
}

void channel_spin_change(GtkSpinButton *spinbutton, UNUSED void *data)
{
	pattern[cpattern]->channel = 
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
}

void track_spin_change(GtkSpinButton *spinbutton, UNUSED void *data)
{
	pattern[cpattern]->tracknum = 
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
}

void timediv_spin_change(UNUSED GtkSpinButton *spinbutton, UNUSED struct division_struct *data)
{
	int i;
	/* Make the timing lines redraw . . . */
	for (i=0;i<NINSTS;i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
}

static void mark_selection_endpoint(int m, int end)
{
	int s, e;

	s = start_copy_measure;
	e = end_copy_measure;
	if (!end) {
		start_copy_measure = m;
		if (m > end_copy_measure || end_copy_measure == -1)
			end_copy_measure = m;
	} else {
		end_copy_measure = m;
		if (start_copy_measure == -1)
			start_copy_measure = m;
		if (end_copy_measure < start_copy_measure) {
			int tmp = end_copy_measure;
			end_copy_measure = start_copy_measure;
			start_copy_measure = tmp;
		}
	}
	if (s != start_copy_measure || e != end_copy_measure)
		redraw_arranger();
}

static int arr_darea_button_press(UNUSED GtkWidget *w, GdkEventButton *event, 
		UNUSED struct pattern_struct *data)
{
	int m;

	m = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));

	if (event->button == 1)
		start_paint_measure = m;
	else if (event->button == 3)
		mark_selection_endpoint(m, 0);
	return 1;
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
void drag_spin_button_change(GtkSpinButton *spinbutton, struct instrument_struct *inst)
{
	int i = inst->instrument_num;
	pattern[cpattern]->drag[i] = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spinbutton));
}

void integer_spin_button_change(GtkSpinButton *spinbutton, int *value)
{
	*value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
}

void unsigned_char_spin_button_change(GtkSpinButton *spinbutton, unsigned char *value)
{
	*value = (unsigned char) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
}

static void redraw_arranger(void)
{
	int i;
	for (i=0;i<npatterns;i++) {
		gtk_widget_queue_draw(GTK_WIDGET(pattern[i]->arr_darea));
	}
	redraw_measure_op_buttons();
}

void insert_measures_button(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	int measures_to_copy, measures_to_move;
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

void delete_measures_button(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	int i, count;
	if (nmeasures <= 1) /* can't have zero measures. */
		return;
	if (start_copy_measure < 0 || 
		end_copy_measure < 0 || 
		end_copy_measure < start_copy_measure)
		return;
	count = end_copy_measure - start_copy_measure + 1;
	if (count >= nmeasures)
		return; /* Can't have zero measures */
	else {
		int new_tempo = -1;
		for (i=start_copy_measure;i<nmeasures-count;i++)
			measure[i] = measure[i+count];
		/* figure the new tempo, which will be the last tempo */
		/* change in the deleted region  */ 
		for (i=0;i<ntempochanges;i++) {
			if (tempo_change[i].measure < start_copy_measure ||  
				tempo_change[i].measure > end_copy_measure)
				continue;		
			new_tempo = tempo_change[i].beats_per_minute;
		}
		/* remove all the tempo changes in the range */
		for (i=0;i<ntempochanges;) {
			if (tempo_change[i].measure < start_copy_measure || 
				tempo_change[i].measure > end_copy_measure) {
				i++;
				continue;		
			}
			remove_tempo_change(i); /* Note, do not increment i here, we deleted the measure instead */
		}

		/* Adjust tempo changes beyond the deleted range by count measures */
		for (i=0;i<ntempochanges;i++)
			if (tempo_change[i].measure > end_copy_measure)
				tempo_change[i].measure -= count; 
		/* Insert the new tempo, if any */
		if (new_tempo != -1) {
			/* see if there's already a tempo change there (if there was one just beyond
			   the deleted range that fell into place */
			int found = 0;
			for (i=0;i<ntempochanges;i++)
				if (tempo_change[i].measure == start_copy_measure)
					found=1;
			if (!found)
				insert_tempo_change(start_copy_measure, new_tempo);
		}
	}
	nmeasures -= count;
	start_copy_measure = end_copy_measure = -1;
	redraw_arranger();
	return;
}

void select_measures_button(UNUSED GtkWidget *widget, UNUSED gpointer data)
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
		UNUSED gpointer data)
{
	int m;
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
		sprintf(TempoChMsg, TEMPO_CHANGE_MESSAGE, m); /* "measure:%d beats / minute" */
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
			return TRUE;

		measures_to_copy = end_copy_measure - start_copy_measure + 1;
		if (nmeasures + measures_to_copy > MAXMEASURES)
			measures_to_copy = MAXMEASURES - nmeasures; /* copy what we can */
		bytes_to_copy = sizeof(struct measure_struct) * measures_to_copy;

		measures_to_move = nmeasures - m + 1;
		bytes_to_move = measures_to_move * sizeof(struct measure_struct);
		/* Copy to a temporary buffer first, to deal with overlaps, etc. */
		tmp = malloc(bytes_to_copy);
		if (tmp == NULL)
			return TRUE;
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
		for (i=0;i<ntempochanges;i++)
			if (tempo_change[i].measure > m)
				tempo_change[i].measure++;
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
		int i, delete_tempo_change = 0;
		int this_measure_tempo = -1;
		/* printf("Delete clicked, measure = %d\n", m); */
		if (nmeasures <= 0)
			return TRUE;
		for (i=m;i<nmeasures-1;i++)
			measure[i] = measure[i+1];

		/* see if this measure has a tempo change */
		for (i=0;i<ntempochanges;i++)
			if (tempo_change[i].measure == m) {
				this_measure_tempo = i;
				break;
			}

		/* Adjust tempo changes */
		for (i=0;i<ntempochanges;i++) {
			/* If another tempo change gets moved on top of one for the measure 
			   being deleted, then remove the one for the deleted measure */
			if (tempo_change[i].measure == m+1) {
				/* only delete it if the next one is obliterating an existing tempo change */
				if (this_measure_tempo != -1)
					delete_tempo_change=this_measure_tempo + 1; /* plus 1 so it won't be zero */
			}
			if (tempo_change[i].measure > m)
				tempo_change[i].measure--;
		}
		if (delete_tempo_change)
			remove_tempo_change(delete_tempo_change-1); /* minus 1 to undo above plus 1 */
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

static int copy_measure_press(UNUSED GtkWidget *w, GdkEventButton *event, 
		UNUSED gpointer data)
{
	int m;
	m = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
	start_copy_measure = m;
	end_copy_measure = m;
	redraw_arranger();
	return 1;
}

static int copy_measure_release(UNUSED GtkWidget *w, GdkEventButton *event, 
		UNUSED gpointer data)
{
	int m;
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
	return 1;
}

static gint drumtab_selection_received(UNUSED GtkWidget* widget,
	GtkSelectionData *selection, UNUSED gpointer data)
{
	char *drumtab;
	int factor;

	if (selection->length < 0) {
		printf("Can't get selection.\n");
		return TRUE;
	}
	if (selection->type != GDK_SELECTION_TYPE_STRING) {
		printf("Selection can't be converted to string.\n");
		return TRUE;
	}

	drumtab = (char *) selection->data;
	factor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(arr_factor_check_button));

	/* printf("Drumtab is: %s\n", drumtab); */
	process_drumtab_buffer(drumtab, factor);
	translate_drumtab_data(factor);
	dt_free_memory();
	return TRUE;
}

static void paste_drumtab_selection()
{
	static GdkAtom targets_atom = GDK_NONE;

	/* Get the atom corresponding to the string "STRING" */
	if (targets_atom == GDK_NONE)
		targets_atom = gdk_atom_intern ("STRING", FALSE);

	/* And request the "STRING" target for the primary selection */
	gtk_selection_convert (arranger_window, GDK_SELECTION_PRIMARY, targets_atom, GDK_CURRENT_TIME);
	/* We will get called back with the selection */
}

static void paste_drumtab_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	/* Selected from the menu, same as middle mouse button, except from clipboard */
	static GdkAtom targets_atom = GDK_NONE;

	/* Get the atom corresponding to the string "STRING" */
	if (targets_atom == GDK_NONE)
		targets_atom = gdk_atom_intern ("STRING", FALSE);

	/* And request the "STRING" target for the primary selection */
	gtk_selection_convert (arranger_window, GDK_SELECTION_CLIPBOARD, targets_atom, GDK_CURRENT_TIME);
	/* We will get called back with the selection */
}

static int arr_darea_clicked(GtkWidget *w, GdkEventButton *event, 
		struct pattern_struct *data)
{
	struct measure_struct *m;
	int mn, i;
	int begin_measure, end_measure;
	mn = (int) trunc((0.0 + event->x) / (0.0 + MEASUREWIDTH));
	if (mn > nmeasures || mn == MAXMEASURES)
		return TRUE;

	begin_measure = start_paint_measure;
	if (mn == nmeasures) /* creating a new measure? */
		measure[mn].npatterns = 0;
	if (begin_measure == -1)
		begin_measure = mn;

	if (event->button == 1) {
		if (mn < begin_measure) {
			mn = begin_measure;
			begin_measure = mn;
		}
		end_measure = mn;
		printf("end_measure = %d\n", mn);
		for (mn = begin_measure; mn <= end_measure; mn++) {
			/* printf("selected measure m = %d\n", m); */
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
		start_paint_measure = -1; /* ready for next painting operation  */
	} else if (event->button == 2)
		paste_drumtab_selection();
	else if (event->button == 3)
		mark_selection_endpoint(mn, 1);
	return TRUE;
}

static int canvas_key_pressed(UNUSED GtkWidget *w, UNUSED GdkEventButton *event,
				UNUSED struct instrument_struct *data)
{
	printf("canvas key pressed\n");
	return TRUE; 
}

static int canvas_mousedown(UNUSED GtkWidget *w, GdkEventButton *event, UNUSED struct instrument_struct *data)
{
	if (!melodic_mode) 
		return TRUE; /* Drums don't have a duration, you just hit 'em..., on mouseup */
	mousedownx = event->x;
	mousedowny = event->y; 
	return TRUE;
	/* printf("mousedown, x=%d, y=%d\n", mousedownx, mousedowny); */
}

static int canvas_clicked(GtkWidget *w, GdkEventButton *event, struct instrument_struct *data)
{
	/* if (data != NULL)
		g_print("%s canvas clicked!, x=%g, y=%g\n", data->name, event->x, event->y); */
	int rc;
	unsigned char velocity = DEFAULT_VELOCITY;
	int change_velocity = (event->button == 1);
	int height;
	int hitx, duration;

	if (melodic_mode) {
		/* whichever is leftmost, mouseup/mousedown, we take as noteon time, */
		/* the other we take as noteoff time */
		if (mousedownx < event->x) {
			hitx = mousedownx;
			duration = event->x - mousedownx;
		} else {
			hitx = event->x;
			duration = mousedownx - hitx;;
		}
		if (duration < 0)
			duration = -duration;
	} else {
		/* percussion mode, mouseup is the one we want */
		hitx = event->x;
		duration = -1; 
	}

	if (current_instrument != data->instrument_num)
		height = melodic_mode ? PIANO_NOTE_HEIGHT : DRAW_HEIGHT;
	else {
		height = (int) (((double) 
			gtk_range_get_value(GTK_RANGE(volume_magnifier))) * 
				(melodic_mode ? 3.0 : 1.0) *
				(double) DRAW_HEIGHT / (double) 100.0) + 1;
	}

	/* 1st mouse button chooses velocity based on vertical position, like a graph, 
	   2nd mouse button uses the default velocity set by the volume slider */	
	if (change_velocity)
		velocity = 127 - (unsigned char) (((double) event->y / (double) height) * 127.0);
	else
		velocity = (unsigned char) gtk_range_get_value(GTK_RANGE(data->volume_slider));

	if (event->button == 1 || event->button == 2) {
		rc = add_hit(&data->hit, (double) hitx, (double) duration, (double) DRAW_WIDTH, 
			kit, data->instrument_num, cpattern, velocity, change_velocity);
		if (midi->isopen(midi_handle) && !melodic_mode)
			midi->noteon(midi_handle, pattern[cpattern]->tracknum, 
				pattern[cpattern]->channel, data->midivalue, velocity);
		if (rc == -2) /* Note was already there, so we really want to remove it. */
			remove_hit(&data->hit, (double) hitx, (double) DRAW_WIDTH,
				kit, data->instrument_num, cpattern);
	} else if (event->button == 3)
		remove_hit(&data->hit, (double) hitx, (double) DRAW_WIDTH,
			kit, data->instrument_num, cpattern);
	if (current_instrument != data->instrument_num) {
		int prev = current_instrument;
		int newheight;

		newheight = (int) (((double) 
			gtk_range_get_value(GTK_RANGE(volume_magnifier))) * 
				(double) DRAW_HEIGHT / (double) 100.0) + 1;
		
		current_instrument = data->instrument_num;
		gtk_widget_set_size_request(w, DRAW_WIDTH+1, newheight);
		gtk_widget_set_size_request(drumkit[kit].instrument[prev].canvas, 
			DRAW_WIDTH+1, DRAW_HEIGHT+1);
		gtk_widget_queue_draw(drumkit[kit].instrument[prev].canvas);
	}
	gtk_widget_queue_draw(w);
	return TRUE;
}

static int measure_transport_expose(GtkWidget *w, UNUSED GdkEvent *event, UNUSED gpointer p)
{
	int x, y2;

	y2 = ARRANGER_HEIGHT;
	gdk_draw_line(w->window, gc, 0,0, MEASUREWIDTH * (nmeasures), 0);
	gdk_draw_line(w->window, gc, 0, y2, MEASUREWIDTH * (nmeasures), y2);
	gdk_draw_line(w->window, gc, 0, 0, 0, ARRANGER_HEIGHT);
	gdk_draw_line(w->window, gc, MEASUREWIDTH * nmeasures, 0, 
		MEASUREWIDTH * nmeasures, y2);
	x = MEASUREWIDTH * transport_location->measure +
		(transport_location->percent * MEASUREWIDTH / 100);
	gdk_draw_line(w->window, gc, x-3, 0, x, y2); 
	gdk_draw_line(w->window, gc, x+3, 0, x, y2); 
	return TRUE;
}

gint transport_update_callback(UNUSED gpointer data)
{
	/* this is called back by gtk_main, as an idle function every so often
	   during playback to update the transport location.  Returns TRUE,
	   means it will get called again, returning false, means it's done. */

	static int lastmeasure = -1;
	static int lastpercent = -1; /* remember, so if transport hasn't moved, we don't redraw */

	if (transport_location->measure == -1) {
		/* The player process sets the measure to -1 when
		   when it finishes playing. */
		transport_location->measure = 0;
		transport_location->percent = 0;
		gtk_widget_queue_draw(measure_transport_da);

		/* looping the pattern?  */
		if (pattern_play_mode &&
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern_loop_chbox))) {
			/* play it again Sam... */
			pattern_play_button_clicked(NULL, NULL);
			printf("Playing again...\n");
		} else {
			/* Turn off record mode, in case it's on. */
			record_mode = 0;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern_record_button),
				(gboolean) 0);
		}
		return FALSE;
	}

	if (lastpercent != transport_location->percent || 
		lastmeasure != transport_location->measure) {
			struct instrument_struct *inst;
			gtk_widget_queue_draw(measure_transport_da);
			inst = &drumkit[kit].instrument[current_instrument];
			gtk_widget_queue_draw(inst->canvas);
	} else
		return TRUE;

	lastmeasure = transport_location->measure;
	lastpercent = transport_location->percent;
	return TRUE;
}

static int measure_da_expose(GtkWidget *w, UNUSED GdkEvent *event, UNUSED gpointer p)
{
	int i, x, y1, j;

	gdk_draw_line(w->window, gc, 0,0, MEASUREWIDTH * (nmeasures), 0);
	gdk_draw_line(w->window, gc, 0, ARRANGER_HEIGHT, MEASUREWIDTH * (nmeasures), ARRANGER_HEIGHT);
	x = 0; y1 = 0;
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

static int arr_darea_event(GtkWidget *w, UNUSED GdkEvent *event, struct pattern_struct *p)
{
	int i, x, y1, j;
	/* printf("arranger event\n"); */

	gdk_draw_line(w->window, gc, 0,0, MEASUREWIDTH * (nmeasures + 1), 0);
	gdk_draw_line(w->window, gc, 0, ARRANGER_HEIGHT-1, MEASUREWIDTH * (nmeasures + 1), ARRANGER_HEIGHT-1);
	if (p->pattern_num == pattern_in_copy_buffer) {
		/* probably could do something better by using color... */
		gdk_draw_line(w->window, gc, 0,1, MEASUREWIDTH * (nmeasures + 1), 1);
		gdk_draw_line(w->window, gc, 0, ARRANGER_HEIGHT-2, 
			MEASUREWIDTH * (nmeasures + 1), ARRANGER_HEIGHT-2);
	}
		
	x = 0; y1 = 0;
	if (start_copy_measure > -1 && end_copy_measure >= start_copy_measure)
		gdk_draw_line(w->window, gc, start_copy_measure*MEASUREWIDTH+1, y1+ARRANGER_HEIGHT/2, 
					(end_copy_measure+1)*MEASUREWIDTH-1, y1+ARRANGER_HEIGHT/2);
	for (i=0;i<nmeasures+2;i++) {
		/* probably could do something better by using color... */
		gdk_draw_line(w->window, gc, x, y1, x, y1+ARRANGER_HEIGHT);
		if ((i % 8) == 0)
			gdk_draw_line(w->window, gc, x+1, y1, x+1, y1+ARRANGER_HEIGHT);
		if (measure[i].npatterns != 0 && i<nmeasures ) {
			for (j=0;j<measure[i].npatterns;j++) {
				if (measure[i].pattern[j] == p->pattern_num) {
					int filled = (i < start_copy_measure || i > end_copy_measure || start_copy_measure < 0);
					gdk_draw_rectangle(w->window, gc, filled, x + 5, y1 + 5, MEASUREWIDTH - 10, ARRANGER_HEIGHT - 10);
					break;
				}
			}
		}
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
		x += MEASUREWIDTH;
	}
	/* gtk_widget_queue_draw(w); */
	return 1;
}

static int hide_all_the_canvas_widgets(struct instrument_struct *inst)
{
	gtk_widget_hide(GTK_WIDGET(inst->clear_button));
	gtk_widget_hide(GTK_WIDGET(inst->volume_slider));
	gtk_widget_hide(GTK_WIDGET(inst->drag_spin_button));
	gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
	gtk_widget_hide(GTK_WIDGET(inst->midi_value_spin_button));
	gtk_widget_hide(GTK_WIDGET(inst->name_entry));
	gtk_widget_hide(GTK_WIDGET(inst->type_entry));
	gtk_widget_hide(GTK_WIDGET(inst->hidebutton));
	gtk_widget_hide(GTK_WIDGET(inst->button));
	return 1;
}

static void bring_back_right_widgets(struct instrument_struct *inst,
	int vshidden, int unchecked_hidden, int editdrumkit)
{
	melodic_mode = (int) !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (percussion_toggle));

	if (melodic_mode) {
		vshidden = 1;
		editdrumkit = 1;
		unchecked_hidden = 0;
	}

	if (vshidden) {
		gtk_widget_hide(GTK_WIDGET(inst->clear_button));
		gtk_widget_hide(GTK_WIDGET(inst->volume_slider));
		gtk_widget_hide(GTK_WIDGET(inst->drag_spin_button));
	}
	if (editdrumkit) {
		gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
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
			gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
		}
		if (!vshidden) { /* otherwise, already hidden */
			gtk_widget_hide(GTK_WIDGET(inst->clear_button));
			gtk_widget_hide(GTK_WIDGET(inst->volume_slider));
			gtk_widget_hide(GTK_WIDGET(inst->drag_spin_button));
		}
	} else {
		if (melodic_mode) {
			gtk_widget_show(GTK_WIDGET(inst->canvas));
			gtk_widget_hide(GTK_WIDGET(inst->button));
			gtk_widget_hide(GTK_WIDGET(inst->hidebutton));
		} else {
			gtk_widget_show(GTK_WIDGET(inst->canvas));
			gtk_widget_show(GTK_WIDGET(inst->button));
			gtk_widget_show(GTK_WIDGET(inst->hidebutton));
		}
		if (!editdrumkit) {/* otherwise, already hidden */
			gtk_widget_show(GTK_WIDGET(inst->name_entry));
			gtk_widget_show(GTK_WIDGET(inst->type_entry));
			gtk_widget_show(GTK_WIDGET(inst->midi_value_spin_button));
			gtk_widget_show(GTK_WIDGET(inst->gm_value_spin_button));
		}
		if (!vshidden) { /* otherwise, already hidden */
			gtk_widget_show(GTK_WIDGET(inst->clear_button));
			gtk_widget_show(GTK_WIDGET(inst->volume_slider));
			gtk_widget_show(GTK_WIDGET(inst->drag_spin_button));
		}
	}
}

static void check_melodic_mode()
{
	int i;
	int vshidden, unchecked_hidden, editdrumkit;

	melodic_mode = (int) !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (percussion_toggle));

	vshidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_volume_sliders));
	unchecked_hidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_instruments));
	editdrumkit = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (edit_instruments_toggle));


	for (i=0;i<MAXINSTS;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		if (melodic_mode) {
			gtk_widget_hide(GTK_WIDGET(inst->hidebutton));
			gtk_widget_hide(GTK_WIDGET(inst->button));
			gtk_widget_hide(GTK_WIDGET(inst->name_entry));
			gtk_widget_hide(GTK_WIDGET(inst->type_entry));
			gtk_widget_hide(GTK_WIDGET(inst->midi_value_spin_button));
			gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
			gtk_widget_set_size_request(inst->canvas, DRAW_WIDTH+1, DRAW_HEIGHT+1);
			gtk_widget_show(GTK_WIDGET(inst->canvas));
		} else {
			if (i>=drumkit[kit].ninsts) {
				gtk_widget_hide(GTK_WIDGET(inst->hidebutton));
				gtk_widget_hide(GTK_WIDGET(inst->button));
				gtk_widget_hide(GTK_WIDGET(inst->name_entry));
				gtk_widget_hide(GTK_WIDGET(inst->type_entry));
				gtk_widget_hide(GTK_WIDGET(inst->midi_value_spin_button));
				gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
				gtk_widget_set_size_request(inst->canvas, DRAW_WIDTH+1, DRAW_HEIGHT+1);
				gtk_widget_hide(GTK_WIDGET(inst->canvas));
			} else {
				gtk_widget_show(GTK_WIDGET(inst->button));
				gtk_widget_set_size_request(inst->canvas, DRAW_WIDTH+1, DRAW_HEIGHT+1);
				gtk_widget_show(GTK_WIDGET(inst->canvas));
				bring_back_right_widgets(inst, vshidden, unchecked_hidden, editdrumkit);
			}
		}
	}
}

void percussion_toggle_callback(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	check_melodic_mode();
}

void set_notelabel(int note)
{
	int n = note % 12;
	if (n < 0)
		n = -n;
	if (melodic_mode) {
		gtk_label_set_text(GTK_LABEL(NoteLabel), notename[n]);
	} else { 
		gtk_label_set_text(GTK_LABEL(NoteLabel), "--");
	}
	gtk_widget_queue_draw(GTK_WIDGET(NoteLabel));
}

static int canvas_event(GtkWidget *w, UNUSED GdkEvent *event, struct instrument_struct *instrument)
{
	int i,j; 
	double diff;
	GdkColor color;
	float divs, zero;
	struct hitpattern *this;
	int height;
	int autocrunch_is_on;
	int vshidden, unchecked_hidden, editdrumkit;

	autocrunch_is_on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (autocrunch));
	vshidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_volume_sliders));
	unchecked_hidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_instruments));
	editdrumkit = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (edit_instruments_toggle));

	/* Calculate the height of the horizontal strip for this instrument... */
	if (current_instrument == instrument->instrument_num) {
		set_notelabel(current_instrument);
		/* melodic mode is narrow, percussion taller, depending on volume zoom too... */
		height = (int) (((double) 
			gtk_range_get_value(GTK_RANGE(volume_magnifier))) * (melodic_mode ? 3.0 : 1.0) *
				(double) DRAW_HEIGHT / (double) 100.0) + 1;
		if (autocrunch_is_on)
			bring_back_right_widgets(instrument, vshidden, unchecked_hidden, editdrumkit);
		gtk_widget_set_size_request(instrument->canvas, DRAW_WIDTH+1, height);
		gdk_draw_line(w->window, gc, DRAW_WIDTH * transport_location->percent / 100,
			0, DRAW_WIDTH * transport_location->percent / 100, height-1); 
	} else if (!autocrunch_is_on || abs(current_instrument - instrument->instrument_num) < 9) {
		height = DRAW_HEIGHT+1;
		if (autocrunch_is_on)
			bring_back_right_widgets(instrument, vshidden, unchecked_hidden, editdrumkit);
		gtk_widget_set_size_request(instrument->canvas, DRAW_WIDTH+1, DRAW_HEIGHT+1);
	} else {
		/* height = 7; */
		if (autocrunch_is_on)
			hide_all_the_canvas_widgets(instrument);
		height = (double) 1.8 * (double) DRAW_HEIGHT / (abs(current_instrument - instrument->instrument_num) - 5 );
		if (height < 1) height = 1;
		gtk_widget_set_size_request(instrument->canvas, DRAW_WIDTH+1, height);
	}
	// gdk_colormap_alloc_color(gtk_widget_get_colormap(w), &whitecolor, FALSE, FALSE);
	// gdk_gc_set_background(gc, &whitecolor);
	
/*	gdk_draw_line(w->window, gc, 0,height / 2, DRAW_WIDTH, height / 2);
	gdk_draw_line(w->window, gc, 0,0, DRAW_WIDTH, height);
	gdk_draw_line(w->window, gc, 0,height, DRAW_WIDTH, 0); */

	gdk_draw_line(w->window, gc, 0, height-1, DRAW_WIDTH, height-1); 
	gdk_draw_line(w->window, gc, 0,0, 0, height);
	gdk_draw_line(w->window, gc, DRAW_WIDTH,0, DRAW_WIDTH, height);

	/* Put a rectangle around the instrument selected for copying */
	if (instrument_in_copy_buffer == instrument->instrument_num) {
		/* probably could do something better with color... */
		gdk_draw_line(w->window, gc, 0, height-3, DRAW_WIDTH, height-3); 
		gdk_draw_line(w->window, gc, 0, 3, DRAW_WIDTH, 3); 
		gdk_draw_line(w->window, gc, 2,0, 2, height);
		gdk_draw_line(w->window, gc, DRAW_WIDTH-2,0, DRAW_WIDTH-2, height);
	}

	/* Draw the timing lines... */
	for (i=ndivisions-1;i>=0;i--) {
		int k;
		divs =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(timediv[i].spin));
		zero = divs - 0.0;
		if (zero < 0) 
			zero = -zero;
		if (zero <= 0.000001) /* not using this one */
			continue;

		// diff = (double) DRAW_WIDTH / divs;
		diff = 0.0;
		memset(&color, 0, sizeof(color));

		/* Does this color crap really have to be done EVERY time? cache it somehow? */
		gdk_color_parse(timediv[i].color, &color);
		gdk_colormap_alloc_color(gtk_widget_get_colormap(w), &color, FALSE, FALSE);
		gdk_gc_set_foreground(gc, &color);
		for (j=0;j<divs;j++) {
			gdk_draw_line(w->window, gc, (int) diff, 0, (int) diff, height);
			if (current_instrument == instrument->instrument_num && height > 60) {
				for (k=0;k<10;k++)
					gdk_draw_line(w->window, gc, (int) (diff - 3), (height / 10) * k, 
						(int) (diff + 3), (height / 10) * k);
			}
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

	/* Draw the note-ons/note-offs for this perc. instrument/note... */
	for (this = instrument->hit; this != NULL; this = this->next) {
		double x1,y1,x2,y2;

		x1 = this->h.time * DRAW_WIDTH /* - 5 */ ;
		y1 = height - (int) (((double) height * (double) this->h.velocity) / 127.0);
		y2 = height;

		/* g_print("x1=%g, y1=%g, x2=%g, y2=%g\n", x1, y1, x2, y2); */

		/* Draw the "note" */
		if (melodic_mode) {
			if (this->h.noteoff_time < 0)
				x2 = x1 + 10; /* fill in something "sensible"... is this really needed? */
			else
				x2 = this->h.noteoff_time * DRAW_WIDTH /* - 5 */ ;
			gdk_draw_rectangle(w->window, gc, TRUE, x1, y1, x2-x1, y2-y1);
		} else {
			x2 = x1 + 10;
			/* front vertical line... */
			gdk_draw_line(w->window, gc, (int) x1, (int) y2, (int) x1, (int) y1);
			gdk_draw_line(w->window, gc, (int) x1 + 1, (int) y2, (int) x1 + 1, (int) y1);
			/* bottom line... */
			gdk_draw_line(w->window, gc, (int) x1, (int) y2, (int) x2, (int) y2);
			gdk_draw_line(w->window, gc, (int) x1, (int) y2 - 1, (int) x2, (int) y2 - 1);
			/* slanty line */
			gdk_draw_line(w->window, gc, (int) x1, (int) y1, (int) x2, (int) y2);
			gdk_draw_line(w->window, gc, (int) x1 + 1, (int) y1, (int) x2 + 1, (int) y2);
			/* horizontal line on top... */
			gdk_draw_line(w->window, gc, (int) x1-8, (int) y1, (int) x1+8, (int) y1);
		}
	}

	/* if (instrument != NULL) 
		g_print("%s\n", instrument->name); */
	return TRUE;
}

static int canvas_enter(UNUSED GtkWidget *w, UNUSED GdkEvent *event,
				struct instrument_struct *instrument)
{
#if 0
	struct instrument_struct *inst;
	int old_inst;

	old_inst = current_instrument;
	current_instrument =  instrument->instrument_num;
	inst = &drumkit[kit].instrument[old_inst];
	canvas_event(inst->canvas, NULL, inst);
	/* printf("Enter event for instrument %d\n", instrument->instrument_num); */
	inst = &drumkit[kit].instrument[current_instrument];
	canvas_event(inst->canvas, NULL, inst);
#endif
	int i;
	struct instrument_struct *inst;
	int old_inst;
	int automag_is_on, autocrunch_is_on;

	automag_is_on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (automag));
	autocrunch_is_on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (autocrunch));

	if (!automag_is_on && !autocrunch_is_on)
		return 0;

	if (autocrunch_is_on && abs(current_instrument - instrument->instrument_num) <= 4) 
		return 0;

	if ((!autocrunch_is_on && automag_is_on) || record_mode) {
		old_inst = current_instrument; 
		current_instrument =  instrument->instrument_num;
		inst = &drumkit[kit].instrument[old_inst];
		canvas_event(inst->canvas, NULL, inst);
		/* printf("Enter event for instrument %d\n", instrument->instrument_num); */
		inst = &drumkit[kit].instrument[current_instrument];
		canvas_event(inst->canvas, NULL, inst);
		return 0;
	}

	if (automag_is_on) {
		if (current_instrument < instrument->instrument_num)
			current_instrument++;
		else if (current_instrument > instrument->instrument_num)
			current_instrument--;
	}

	if (autocrunch_is_on) {	
		for (i=0;i<NINSTS;i++) {
			struct instrument_struct *inst = &drumkit[kit].instrument[i];
			canvas_event(inst->canvas, NULL, inst);
		}
	}
	
	return 0;
}


void check_hitpatterns(char *x);

void pattern_clear_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	printf("Pattern clear button.\n");
	int i;
	/* Clear out anything old */
	for (i=0;i<NINSTS; i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		if (inst->hit != NULL) {
			clear_hitpattern(inst->hit);
			inst->hit = NULL;
		}
		gtk_widget_queue_draw(GTK_WIDGET(drumkit[kit].instrument[i].canvas));
	}

}

void pattern_stop_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	printf("Pattern stop button.\n");
	if (player_process_pid != -1)
		kill(player_process_pid, SIGUSR1);
	record_mode = 0;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern_record_button),
					(gboolean) 0);
	if (measure_transport_tag != -1 && measure_transport_tag != 0) {
		g_source_remove(measure_transport_tag);
		measure_transport_tag = -1;
	}
}

void schedule_pattern(UNUSED int kit, int measure, int cpattern, int tempo, struct timeval *base)
{
	struct timeval basetime, orig_basetime;
	unsigned long measurelength;
	unsigned long beats_per_minute, beats_per_measure;
	struct hitpattern *this;
	int i;
	long drag; /* microsecs */
	struct timeval tmpbase;
	int track, channel;
	unsigned char note;

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

	track = pattern[cpattern]->tracknum;
	channel = pattern[cpattern]->channel;

	for (this = pattern[cpattern]->hitpattern; this != NULL; this = this->next) {
		struct drumkit_struct *dk = &drumkit[this->h.drumkit];
		struct instrument_struct *inst = &dk->instrument[this->h.instrument_num];
		int pct;

		drag = (long) (pattern[cpattern]->drag[this->h.instrument_num] * 
			(double) measurelength / (double) beats_per_measure / 100.0);

		pct = (int) (100.0 * (((this->h.time * measurelength) + drag) / measurelength));
		if (pct < 0)
			pct = 0;
		else if (pct > 100)
			pct = 100;
		
		if (pattern[cpattern]->music_type != PERCUSSION)
			note = this->h.instrument_num;
		else 
			note = inst->midivalue;
		/* printf("this->h.time = %g\n", this->h.time); */
		tmpbase = basetime;
		sched_note(&sched, &basetime, track, channel, note, 
			measurelength, this->h.time, 1000000, this->h.velocity, 
			measure, pct, drag);
		/* schedule note off if not percussion */
		pct = (int) (100.0 * (((this->h.noteoff_time * measurelength) + drag) / measurelength));
		if (pct < 0)
			pct = 0;
		else if (pct > 100)
			pct = 100;
		if (pattern[cpattern]->music_type != PERCUSSION)
			sched_note(&sched, &tmpbase, track, channel, note, 
				measurelength, this->h.noteoff_time, 1000000, /* velocity zero */ 0, 
				measure, pct, drag);
	}
	/* This no-op is just so the next measure doesn't before this one is really over. */
	orig_basetime = basetime;
	sched_noop(&sched, &basetime, track, 
		0, measurelength, 1.0, 1000000, 127, measure, 100); 

	/* FIXME, instead of scheduling all these crazy noops, probably should build them
	   into the player code algorithmically or something. */
	if (record_mode) {
		int metronome = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern_metronome_chbox));
		int timediv0;
		if (pattern[cpattern]->timediv[0].division == 0)
			timediv0 = -1;
		else
			timediv0 = 100 / pattern[cpattern]->timediv[0].division;
		for (i=0;i<100;i++) {
			tmpbase = orig_basetime;
			if (metronome && timediv0 > 0 && ((i % timediv0) == 0))
				sched_note(&sched, &tmpbase, track, channel,
					24, measurelength, (double) i / 100.0, 
					1000000, 127, measure, i, 0); 
			else
				sched_noop(&sched, &tmpbase, track,
					0, measurelength, (double) i / 100.0, 
					1000000, 127, measure, i); 
		}
	}
	if (base != NULL)
		*base = basetime;
}

void schedule_measures(int start, int end)
{
	int i,j;
	struct timeval base, tmpbase;
	int tempo, track, chan;

	/* Give ourself 1/10th millisecond per measure of lead time to 
	   calculate the schedule, otherwise if we're slow, the first beat
	   will be late.  Debug output in the scheduler will exacerbate this. */

	if (start < 0 || end < start)
		return;

	base.tv_usec = 5000L * nmeasures;
	base.tv_sec = 0L; 
	
	for (i=start;i<end;i++) {
		for (j=0;j<measure[i].npatterns;j++) {
			struct pattern_struct *p = pattern[measure[i].pattern[j]];
			tmpbase = base;
			tempo = find_tempo(i);
			track = p->tracknum;
			chan = p->channel;
			/* printf("Tempo for measure %d is %d beats per minute\n", i, tempo); */
			if (!transport_location->muted[track][chan])
				schedule_pattern(kit, i, measure[i].pattern[j], tempo, &tmpbase);
		}
		base = tmpbase;
	}
}

int export_to_midi_file(const char *filename)
{
	int start, end;

	start = 0;
	end = nmeasures;

	if (start < 0 || end < start)
		return FALSE;

	flatten_pattern(kit, cpattern);
	schedule_measures(start, end);
	/* print_schedule(&sched); */
	write_sched_to_midi_file(&sched, filename);
	free_schedule(&sched);
	return TRUE;
}

int load_from_file(const char *filename);

void loadbox_file_selected(UNUSED GtkWidget *widget, GtkFileSelection *FileBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(FileBox);
	gtk_widget_hide(GTK_WIDGET(FileBox));
	load_from_file(filename);
}

void savebox_file_selected(UNUSED GtkWidget *widget, GtkFileSelection *FileBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(FileBox);
	gtk_widget_hide(GTK_WIDGET(FileBox));
	save_to_file(filename);
}

void savedrumkitbox_file_selected(UNUSED GtkWidget *widget, GtkFileSelection *FileBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(FileBox);
	gtk_widget_hide(GTK_WIDGET(FileBox));
	save_drumkit_to_file(filename);
}

void import_patterns_file_selected(UNUSED GtkWidget *widget, GtkFileSelection *FileBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(FileBox);
	gtk_widget_hide(GTK_WIDGET(FileBox));
	import_patterns_from_file(filename);
}

void import_drumtab_from_file(const char *filename, int factor)
{
	process_drumtab_file(filename, factor);
	translate_drumtab_data(factor);
	dt_free_memory();
}


void import_drumtab_file_selected(UNUSED GtkWidget *widget, GtkFileSelection *FileBox)
{
	const char *filename;
	int factor;

	factor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(arr_factor_check_button));
	filename = gtk_file_selection_get_filename(FileBox);
	gtk_widget_hide(GTK_WIDGET(FileBox));
	import_drumtab_from_file(filename, factor);
}

void export_to_midi(UNUSED GtkWidget *widget, GtkFileSelection *FileBox)
{
	const char *filename;
	filename = gtk_file_selection_get_filename(FileBox);
	gtk_widget_hide(GTK_WIDGET(FileBox));
	export_to_midi_file(filename);
}

#define SAVE_SONG 0
#define LOAD_SONG 1
#define SAVE_DRUMKIT 2
#define IMPORT_PATTERNS 3
#define IMPORT_DRUMTAB 4
#define EXPORT_TO_MIDI 5
static struct file_dialog_descriptor {
	char *title;
	GtkWidget **widget;
	void (*file_selected_function)(GtkWidget *widget, GtkFileSelection *selection);
} file_dialog[] = {
	{ SAVE_SONG_ITEM, &SaveBox, savebox_file_selected, },
	{ LOAD_SONG_ITEM, &LoadBox, loadbox_file_selected, },
	{ SAVE_DRUMKIT_LABEL, &SaveDrumkitBox, savedrumkitbox_file_selected, },
	{ IMPORT_PATTERNS_ITEM, &ImportPatternsBox, import_patterns_file_selected, },
	{ IMPORT_DRUM_TAB_ITEM, &ImportDrumtabBox, import_drumtab_file_selected, },
	{ EXPORT_SONG_TO_MIDI_ITEM, &export_to_midi_box, export_to_midi, },
};
	
GtkWidget *make_file_dialog(int i)
{
	GtkWidget *w;
	void (*f)(GtkWidget *w, GtkFileSelection *s);

	if (i < 0)
		return NULL;
	if ((unsigned int) i >= sizeof(file_dialog) / sizeof(file_dialog[0]))
		return NULL;

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

void export_midi_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	make_file_dialog(EXPORT_TO_MIDI);
}

void import_patterns_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	make_file_dialog(IMPORT_PATTERNS);
}

void import_drumtab_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	make_file_dialog(IMPORT_DRUMTAB);
}

void save_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	make_file_dialog(SAVE_SONG);
}

void load_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	make_file_dialog(LOAD_SONG);
}

void save_drumkit_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	make_file_dialog(SAVE_DRUMKIT);
}

void remap_drumkit_song();
void remap_drumkit_pattern(struct pattern_struct *p, int set_toggles);

void remap_drumkit_clicked(UNUSED GtkWidget *widget, /* this is for the menu item, remaps whole song */
	UNUSED gpointer data)
{
	/* printf("remap song drumkit menu pressed\n"); */
	remap_drumkit_song();
}

void remap_drumkit_hit(struct hit_struct *h, int set_toggles);
void remap_drumkit_button_clicked(UNUSED GtkWidget *widget, /* this is for the pattern button item */
	UNUSED gpointer data)
{
	/* printf("remap pattern drumkit button pressed\n"); */
	flatten_pattern(kit, cpattern);
	remap_drumkit_pattern(pattern[cpattern], 1);
	unflatten_pattern(kit, cpattern);
	edit_pattern(cpattern);
}

void send_schedule(struct schedule_t *sched, int loop);
void arranger_play_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	int start, end;

	pattern_play_mode = 0;
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
	measure_transport_tag = g_timeout_add(record_mode ? 10 : 100, /* 10x per sec? */
			transport_update_callback, NULL);
}

void pattern_paste_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	int i, from = pattern_in_copy_buffer;
	struct hitpattern *f;

	/* superimpose the hits from one pattern into another */
	if (from < 0 || 
		from >= npatterns || 
		from == cpattern) /* can't paste onto self */
		return;

	flatten_pattern(kit, cpattern);

	for (f = pattern[cpattern]->hitpattern; f != NULL; f=f->next) {
		printf("%p: beat:%d/%d inst=%d\n", (void *) f, f->h.beat,
			f->h.beats_per_measure, f->h.instrument_num); fflush(stdout);
	}
	for (f = pattern[from]->hitpattern; f != NULL; f=f->next) {
		printf("%p: beat:%d/%d inst=%d\n", (void *) f, f->h.beat,
			f->h.beats_per_measure, f->h.instrument_num); fflush(stdout);
	}
	for (f = pattern[from]->hitpattern; f != NULL; f=f->next) {
		lowlevel_add_hit(&pattern[cpattern]->hitpattern, kit, cpattern, 
			f->h.instrument_num, f->h.beat, f->h.beats_per_measure, 
			f->h.noteoff_beat, f->h.noteoff_beats_per_measure,
			f->h.velocity, 1);
	}
	unflatten_pattern(kit, cpattern);
	for (i=0;i<NINSTS; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
	return;
}

void do_play()
{
	flatten_pattern(kit, cpattern);
	schedule_pattern(kit, 0, cpattern, -1, NULL);
	send_schedule(&sched, 0);
	free_schedule(&sched);
	pattern_play_mode = 1;
	measure_transport_tag = g_timeout_add(record_mode ? 10 : 100, /* 10x per sec? */
			transport_update_callback, NULL);
}

void pattern_play_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	do_play();
#if 0
	flatten_pattern(kit, cpattern);
	schedule_pattern(kit, 0, cpattern, -1, NULL);
	send_schedule(&sched, 0);
	free_schedule(&sched);
	pattern_play_mode = 1;
	measure_transport_tag = g_timeout_add(record_mode ? 10 : 100, /* 10x per sec? */
			transport_update_callback, NULL); */
#endif
}

void pattern_record_button_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	record_mode = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pattern_record_button));
	printf("Record pattern button clicked, record_mode = %d\n", record_mode);
	if (record_mode)
		do_play();
}

void set_arranger_window_title()
{
	sprintf(arranger_title, ARRANGER_TITLE_ITEM, /* "%s v. %s - Arrangement Editor: %s" */
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
	/* gtk_window_set_title(GTK_WINDOW(top_window), window_title); */
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
		gtk_button_set_label(GTK_BUTTON(nextbutton), CREATE_NEXT_PATTERN_LABEL);
		gtk_tooltips_set_tip(tooltips, nextbutton, CREATE_NEXT_PATTERN_TIP, NULL);
	} else {
		gtk_button_set_label(GTK_BUTTON(nextbutton), EDIT_NEXT_PATTERN_LABEL);
		gtk_tooltips_set_tip(tooltips, nextbutton, EDIT_NEXT_PATTERN_TIP, NULL);
	}
	/* Make the timing lines redraw . . . */
	check_melodic_mode();
	for (i=0;i<NINSTS;i++)
		gtk_widget_queue_draw(GTK_WIDGET(drumkit[kit].instrument[i].canvas));
}

struct pattern_struct *pattern_struct_alloc(int pattern_num);

void ins_pattern_button_pressed(UNUSED GtkWidget *widget, /* insert pattern */
	struct pattern_struct *data)
{
	int i, j, slot;
	int new_arranger_scroller_size;
	int total_rows;
	char msg[255];
	struct pattern_struct *p;
	int pn = data->pattern_num;
	struct division_struct *divs;

	flatten_pattern(kit, cpattern); /* save current pattern */

#if 0
	printf("insert pattern %d\n", data->pattern_num); */
	printf("-- before ---\n");
	for (i=0;i<npatterns;i++) 
		printf(" --> %d:%d\n", i, pattern[i]->pattern_num);
	printf("-----\n"); */
#endif
	/* Allocate and init the new pattern */

	p = pattern_struct_alloc(pn);
	if (p == NULL)
		return;
	memset(p, 0, sizeof(*p));
	p->pattern_num = data->pattern_num;
	p->tracknum = data->tracknum;
	p->channel = data->channel;
	p->music_type = data->music_type;
	p->hitpattern = NULL;
	sprintf(p->patname, "New%d", npatterns+1);

	/* printf("Pattern allocated: %s\n", p->patname); */

	/* Make room and insert the new pattern */

	for (i=npatterns;i>data->pattern_num;i--) {
		/* printf("Moving pattern %d to %d\n", i-1, i); */
		pattern[i] = pattern[i-1];
		pattern[i]->pattern_num = i;
	}
	/* printf("Storing new pattern in %d\n", pn); */
	pattern[pn] = p;
	npatterns++;

	/* For the new pattern, copy the time divisions from the */
	/* previous, next, or default pattern */
	if (pn > 0) {
		divs = pattern[pn-1]->timediv;
		pattern[pn]->beats_per_minute = pattern[pn-1]->beats_per_minute;
		pattern[pn]->beats_per_measure = pattern[pn-1]->beats_per_measure;
	} else if (pn < npatterns - 1) {
		divs = pattern[pn+1]->timediv;
		pattern[pn]->beats_per_minute = pattern[pn+1]->beats_per_minute;
		pattern[pn]->beats_per_measure = pattern[pn+1]->beats_per_measure;
	} else {
		divs = timediv;
		pattern[pn]->beats_per_minute = 120; 
		pattern[pn]->beats_per_measure = 4; 
	}
	for (i=0;i<ndivisions;i++)
		pattern[pn]->timediv[i] = divs[i];
#if 0
	printf("npatterns = %d\n", npatterns);
	printf("-- after ---\n");
	for (i=0;i<npatterns;i++) 
		printf(" --> %d:%d\n", i, pattern[i]->pattern_num);
	printf("-----\n");
#endif
	/* Set up all the patterns GTK junk, */
	/* this and code in make_new_pattern_widgets should be refactored */

	p->arr_button = gtk_button_new_with_label(p->patname);
	gtk_tooltips_set_tip(tooltips, p->arr_button, EDIT_PATTERN_TIP, NULL);
	p->copy_button = gtk_button_new_with_label("Sel");
	sprintf(msg, SELECT_PATTERN_TIP, p->patname);
	gtk_tooltips_set_tip(tooltips, p->copy_button, msg, NULL);
	p->ins_button = gtk_button_new_with_label("Ins");
	gtk_tooltips_set_tip(tooltips, p->ins_button, INSERT_PATTERN_TIP, NULL);
	p->del_button = gtk_button_new_with_label("Del");
	gtk_tooltips_set_tip(tooltips, p->del_button, DELETE_PATTERN_TIP, NULL);
	p->arr_darea = gtk_drawing_area_new();
	gtk_widget_modify_bg(p->arr_darea, GTK_STATE_NORMAL, &whitecolor);
	gtk_tooltips_set_tip(tooltips, p->arr_darea, ASSIGN_P_TO_M_TIP, NULL);
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
	total_rows = npatterns + 6;
	new_arranger_scroller_size = (total_rows+2) * ARRANGER_HEIGHT;
	if (new_arranger_scroller_size > 200)
		new_arranger_scroller_size = 200;
	gtk_widget_set_size_request (arranger_scroller, 750, new_arranger_scroller_size);

	/* gtk_table_attach(GTK_TABLE(arranger_table), p->ins_button, 
		0, 1, top, top+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->del_button, 
		1, 2, top, top+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->copy_button, 
		2, 3, top, top+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->arr_button, 
		3, 4, top, top+1, GTK_FILL, 0, 0, 0); */
	g_signal_connect(G_OBJECT (p->arr_button), "clicked",
			G_CALLBACK (edit_pattern_clicked), p);
		// 0, 1, top, top+1, GTK_FILL, GTK_FILL, 0, 0);
	/* gtk_table_attach(GTK_TABLE(arranger_table), p->arr_darea, 
		4, 5, top, top+1, 0,0,0,0); */
	g_signal_connect(G_OBJECT (p->ins_button), "clicked",
			G_CALLBACK (ins_pattern_button_pressed), p);
	g_signal_connect(G_OBJECT (p->del_button), "clicked",
			G_CALLBACK (del_pattern_button_pressed), p);
	g_signal_connect(G_OBJECT (p->copy_button), "clicked",
			G_CALLBACK (copy_pattern_button_pressed), p);


	/* Adjust all the measures */

	for (i=0;i<nmeasures;i++) {
		for (j=0;j < measure[i].npatterns;j++) {
			if (measure[i].pattern[j] >= pn)
				measure[i].pattern[j]++;
		}
	}

	/* Scoot all the GTk crap down one slot */
	/* This is how to insert items into a gtktable, */
	/* Use gtk_table_resize to make it one slot bigger, then */
	/* traverse from the highest item to insertion point, using */
	/* gtk_container_child_set to scoot everything down one slot */
	/* then use gtk_table_attach to attach the new item */

	total_rows = npatterns + 6;
	slot = total_rows-1;
	gtk_table_resize(GTK_TABLE(arranger_table), total_rows, ARRANGER_COLS);
	for (i=npatterns;i>pn+1;i--) {
		/* printf("Moving pattern %d to slot %d\n", i-1, slot); */
		p = pattern[i-1];
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->ins_button,
			"left_attach", 0, "right_attach", 1, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->del_button,
			"left_attach", 1, "right_attach", 2, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->copy_button,
			"left_attach", 2, "right_attach", 3, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->arr_button,
			"left_attach", 3, "right_attach", 4, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->arr_darea,
			"left_attach", 4, "right_attach", 5, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_widget_show(p->ins_button);
		gtk_widget_show(p->del_button);
		gtk_widget_show(p->copy_button);
		gtk_widget_show(p->arr_button);
		gtk_widget_show(p->arr_darea);
		slot--;
	}

	p = pattern[pn];
	slot = pn + 6;

	/* printf("Attaching new pattern %d to slot %d\n", pn, slot); */

	gtk_table_attach(GTK_TABLE(arranger_table), p->ins_button, 
		0, 1, slot, slot+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->del_button, 
		1, 2, slot, slot+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->copy_button, 
		2, 3, slot, slot+1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(arranger_table), p->arr_button, 
		3, 4, slot, slot+1, GTK_FILL, 0, 0, 0); 
	gtk_table_attach(GTK_TABLE(arranger_table), p->arr_darea, 
		4, 5, slot, slot+1, 0,0,0,0);
	gtk_widget_show(p->ins_button);
	gtk_widget_show(p->del_button);
	gtk_widget_show(p->copy_button);
	gtk_widget_show(p->arr_button);
	gtk_widget_show(p->arr_darea);

	cpattern = pn;
	edit_pattern(cpattern);
	redraw_arranger();
}

void del_pattern_button_pressed(UNUSED GtkWidget *widget, /* delete pattern */
	struct pattern_struct *data)
{
	int i, j, k;	
	struct pattern_struct *p;
	int slot;


	/* printf("delete pattern %d\n", data->pattern_num); */
	/* this involves shuffling things around in a gtk_table... not sure how. */

	slot = 6 + data->pattern_num;

	/* Get rid of all the GTK junk for the specified row . . . */
	/* This is how to delete a row in a gtktable.  Use gtk_container_remove */
	/* to remove the item, then travers the table from the deleted row up */
	/* to the highest item, and use gtk_container_child_set to scoot each */
	/* remaining item one slot up in the table. */

	gtk_container_remove(GTK_CONTAINER (arranger_table), data->ins_button);
	gtk_container_remove(GTK_CONTAINER (arranger_table), data->del_button);
	gtk_container_remove(GTK_CONTAINER (arranger_table), data->copy_button);
	gtk_container_remove(GTK_CONTAINER (arranger_table), data->arr_button);
	gtk_container_remove(GTK_CONTAINER (arranger_table), data->arr_darea);

	/* Scoot all the GTK junk below the specified row up one row */

	for (i=data->pattern_num; i<npatterns-1;i++) {
		p = pattern[i+1];

		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->ins_button,
			"left_attach", 0, "right_attach", 1, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->del_button,
			"left_attach", 1, "right_attach", 2, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->copy_button,
			"left_attach", 2, "right_attach", 3, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->arr_button,
			"left_attach", 3, "right_attach", 4, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_container_child_set(GTK_CONTAINER (arranger_table), p->arr_darea,
			"left_attach", 4, "right_attach", 5, 
			"top_attach", slot, "bottom_attach", slot+1, NULL);
		gtk_widget_show(p->ins_button);
		gtk_widget_show(p->del_button);
		gtk_widget_show(p->copy_button);
		gtk_widget_show(p->arr_button);
		gtk_widget_show(p->arr_darea);
		slot++;
	} 

	/* Adjust all measures to remove references to the deleted pattern */

	for (i=0;i<nmeasures;i++) {
		for (j=0;j < measure[i].npatterns;) {
			if ( measure[i].pattern[j] == data->pattern_num ) {
				/* delete this entry */
				for (k = j; k< measure[i].npatterns-1; k++)
					measure[i].pattern[k] = measure[i].pattern[k+1];
				measure[i].npatterns--;
				continue;
			} else if ( measure[i].pattern[j] > data->pattern_num )
				measure[i].pattern[j]--;
			j++;
		}
	}

	/* Adjust the pattern struct */
	for (i=data->pattern_num;i < npatterns-1; i++) {
		pattern[i] = pattern[i+1];
		pattern[i]->pattern_num = i;
	}

	free_pattern(data);
	npatterns--;
	pattern[npatterns] = NULL;

	if (cpattern >= npatterns)
		cpattern = npatterns-1; 
	if (cpattern < 0)
		cpattern = 0;

	edit_pattern(cpattern);
	redraw_arranger();
}

void set_select_message(char *pattern)
{
	char msg[255];
	/* "Superimpose all the notes of the selected pattern (%s) onto this pattern.", */
	sprintf(msg, PASTE_MSG_TIP, pattern);
	gtk_tooltips_set_tip(tooltips, pattern_paste_button, msg, NULL);
}

void select_pattern_button_pressed(UNUSED GtkWidget *widget, UNUSED void *unused)
{
	pattern_in_copy_buffer = cpattern;
	set_select_message(pattern[cpattern]->patname);
	redraw_arranger();
}

void copy_pattern_button_pressed(UNUSED GtkWidget *widget, /* copy pattern */
	struct pattern_struct *data)
{
	pattern_in_copy_buffer = data->pattern_num;
	set_select_message(data->patname);
	redraw_arranger();
}

void copy_current_instrument_hit_pattern()
{
	int previous = instrument_in_copy_buffer;
	instrument_in_copy_buffer = current_instrument;
	gtk_widget_queue_draw(drumkit[kit].instrument[instrument_in_copy_buffer].canvas);
	if (previous != -1) 
		gtk_widget_queue_draw(drumkit[kit].instrument[previous].canvas);
	printf("Recorded instrument %d\n", current_instrument);
}

void paste_current_instrument_hit_pattern()
{
	struct drumkit_struct *dk = &drumkit[kit];
	struct hitpattern *h;
	struct instrument_struct *from, *to;

	printf("Paste current instrument... %d into %d\n", instrument_in_copy_buffer, current_instrument);

	/* Something to copy from? */
	if (instrument_in_copy_buffer < 0 || 
		instrument_in_copy_buffer >= NINSTS)
		return;

	/* Something to copy to? */
	if (current_instrument < 0 || 
		current_instrument >= NINSTS)
		return;

	/* Copy the hit pattern ... */
 	from = &dk->instrument[instrument_in_copy_buffer];
 	to = &dk->instrument[current_instrument];
	for (h = from->hit; h != NULL; h=h->next)
		lowlevel_add_hit(&to->hit, kit, cpattern, to->instrument_num, 
			h->h.beat, h->h.beats_per_measure, 
			h->h.noteoff_beat, h->h.noteoff_beats_per_measure, 
			h->h.velocity, 1);
	gtk_widget_queue_draw(GTK_WIDGET(to->canvas));
	return;
}

void paste_all_current_instrument_hit_pattern()
{
	struct drumkit_struct *dk = &drumkit[kit];
	struct hitpattern *h;
	struct instrument_struct *from, *to;
	int i;

	printf("Paste all current instrument... %d into %d\n", instrument_in_copy_buffer, current_instrument);

	/* Something to copy from? */
	if (instrument_in_copy_buffer < 0 || 
		instrument_in_copy_buffer >= NINSTS)
		return;

	/* Something to copy to? */
	if (current_instrument < 0 || 
		current_instrument >= NINSTS)
		return;

	flatten_pattern(kit, cpattern); /* save current pattern */

	for (i=0;i<npatterns;i++) {
		unflatten_pattern(kit, i); /* load next pattern */
		/* Copy the hit pattern ... */
		from = &dk->instrument[instrument_in_copy_buffer];
		to = &dk->instrument[current_instrument];
		for (h = from->hit; h != NULL; h=h->next)
			lowlevel_add_hit(&to->hit, kit, i, to->instrument_num, 
				h->h.beat, h->h.beats_per_measure, 
				h->h.noteoff_beat, h->h.noteoff_beats_per_measure, 
				h->h.velocity, 1);
		flatten_pattern(kit, i);
	}
	unflatten_pattern(kit, cpattern);
	to = &dk->instrument[current_instrument];
	gtk_widget_queue_draw(GTK_WIDGET(to->canvas));
	return;
}

void clear_all_current_instrument_hit_pattern()
{
	struct drumkit_struct *dk = &drumkit[kit];
	struct instrument_struct *inst;
	int i;

	printf("Clear all current instrument... %d\n", current_instrument);

	/* Something to clear? */
	if (current_instrument < 0 || 
		current_instrument >= NINSTS)
		return;

	flatten_pattern(kit, cpattern); /* save current pattern */

	for (i=0;i<npatterns;i++) {
		unflatten_pattern(kit, i); /* load next pattern */
		inst = &dk->instrument[current_instrument];
		clear_hitpattern(inst->hit);
		inst->hit = NULL;
		flatten_pattern(kit, i);
	}

	unflatten_pattern(kit, cpattern);
	inst = &dk->instrument[current_instrument];
	gtk_widget_queue_draw(GTK_WIDGET(inst->canvas));
	return;
}

void edit_pattern_clicked(UNUSED GtkWidget *widget, /* this is the "pattern name" button in the arranger window */
	struct pattern_struct *data)
{
	flatten_pattern(kit, cpattern); /* save current pattern */
	edit_pattern(data->pattern_num);
}

void prevbutton_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	pattern_stop_button_clicked(NULL, NULL);
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


void nextbutton_clicked(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	pattern_stop_button_clicked(NULL, NULL);
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

void adjust_space(double add, double multiply)
{
	int i;
	struct pattern_struct *p = pattern[cpattern];
	struct hitpattern *h;
	struct hit_struct *hit;
	struct hitpattern *prev = NULL;

	flatten_pattern(kit, cpattern);

	for (h = p->hitpattern; h != NULL;) {
		hit = &h->h;
		hit->time = hit->time * multiply + add;
		hit->beat = (int) (hit->time * DRAW_WIDTH);
		hit->beats_per_measure = DRAW_WIDTH;
		reduce_fraction(&hit->beat, &hit->beats_per_measure);
		if (hit->time > 1.0 || hit->time < 0.0) {
			if (prev == NULL) {
				p->hitpattern = p->hitpattern->next;
				free(h);
				h = p->hitpattern;
				continue;
			} else {
				prev->next = h->next;
				free(h);
				h = prev->next;
				continue;
			}
		}
		prev = h;
		h = h->next;
	}
	unflatten_pattern(kit, cpattern);
	check_melodic_mode();
	for (i=0;i<NINSTS; i++)
		if (drumkit[kit].instrument[i].canvas == NULL)
			printf("drumkit[kit].instrument[%d] is null\n", i);
		else
			gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
}

int check_num_denom_ok(double *numerator, double *denominator)
{
	*numerator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_numerator_spin));
	*denominator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_denominator_spin));

	if (*denominator < 0.9999) {
		printf("denominator too small\n");
		return 0;
	}
	if (*numerator < 0.0001) {
		printf("numerator too small\n");
		return 0;
	}
	if (fabs(*numerator - *denominator) < 0.0001) {
		printf("numerator too close to denominator\n");
		return 0;
	}
	return 1;
}

void add_space_before_button_pressed(UNUSED GtkWidget *widget, UNUSED void *whatever)
{
	double numerator, denominator;
	double add, multiply;

	if (!check_num_denom_ok(&numerator,&denominator))
		return;

	add = numerator/denominator;
	multiply = (1 - numerator/denominator);
	adjust_space(add, multiply);
}

void add_space_after_button_pressed(UNUSED GtkWidget *widget, UNUSED void *whatever)
{
	double numerator, denominator;
	double add, multiply;

	numerator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_numerator_spin));
	denominator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_denominator_spin));
	printf("Add space after pressed, %g/%g\n", numerator, denominator);

	if (!check_num_denom_ok(&numerator,&denominator))
		return;

	add = 0.0;
	multiply = (1 - numerator/denominator);
	adjust_space(add, multiply);
}

void remove_space_before_button_pressed(UNUSED GtkWidget *widget, UNUSED void *whatever)
{
	double numerator, denominator;
	double add, multiply;

	numerator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_numerator_spin));
	denominator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_denominator_spin));
	printf("Remove space before pressed, %g/%g\n", numerator, denominator);

	if (!check_num_denom_ok(&numerator,&denominator))
		return;

	multiply = 1 / (1 - numerator/denominator);
	add = (-numerator/denominator) * multiply;
	adjust_space(add, multiply);
}

void remove_space_after_button_pressed(UNUSED GtkWidget *widget, UNUSED void *whatever)
{
	double numerator, denominator;
	double add, multiply;

	numerator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_numerator_spin));
	denominator =  gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(add_space_denominator_spin));
	printf("Remove space after pressed, %g/%g\n", numerator, denominator);

	if (!check_num_denom_ok(&numerator,&denominator))
		return;

	add = 0.0;
	multiply = 1/(1 - numerator/denominator);
	adjust_space(add, multiply);
}

int transpose_hitpattern(struct hitpattern *p, int interval, int for_real);
void transpose(int interval)
{
	/* pattern must be flattened */
	struct pattern_struct *p = pattern[cpattern];
	int rc;

	flatten_pattern(kit, cpattern);

	/* Dry run once, and see if any notes go out of bounds */
	rc = transpose_hitpattern(p->hitpattern, interval, 0);
	if (rc != 0)
		return;
	/* Must be ok, transpose for real. */
	(void) transpose_hitpattern(p->hitpattern, interval, 1);
	unflatten_pattern(kit, cpattern);
	edit_pattern(cpattern);
	return;
}

void scramble_button_pressed(UNUSED GtkWidget *widget, UNUSED void *whatever)
{
	/* this function takes a pattern, divides it up into equal sized sections
	   according to the first timediv spinbox, then scrambles it by shuffling
	   those divisions.  Worthwhile thing to do?  Who knows. */

	struct pattern_struct *p = pattern[cpattern];
	long int divisions;
	int *map;
	int i;
	struct hitpattern *hp;
	struct hitpattern *h;
	struct hit_struct *hit;
	struct hit_struct temp;
	struct hitpattern *next_h;
	int done;
	long int r;

	divisions = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(timediv[0].spin));
	/* printf("Scramble button pressed, divisions = %d.\n", divisions); */

	/* construct a mapping of original division to new division */
	map = (int *) malloc(sizeof(int) * divisions);
	if (map == NULL) {
		printf("out of memory, cannot scramble.\n");
		return;
	}
	for (i=0;i<divisions;i++)
		map[i] = i;

	for (i=0;i<divisions;i++) {
		int tmp, s;

		/* swap with a random division */
		r = random();
		s = (int) (r  / (RAND_MAX / divisions));
		/* printf("random returns, %ld, i=%d, s=%d\n", r, i, s); */
		tmp = map[i];
		map[i] = map[s];
		map[s] = tmp;
	}

	/* for (i=0;i<divisions;i++)
		printf("map[%d] = %d\n", i, map[i]); */

	flatten_pattern(kit, cpattern);

	hp = p->hitpattern;
	for (h = hp; h != NULL; h = h->next) {
		int from_div, to_div;
		double sub, add;

		hit = &h->h;

		/* Calculate what zone this starts in, use map to figure the new zone */
		from_div = (int) trunc(hit->time * (double) divisions);
		to_div = map[from_div];

		/* Adjust hit->time, hit->beat and hit->beats_per_measure to reflect the posititon */
		/* and use reduce_fraction on those two. */
		sub = (double) from_div / (double) divisions;
		add = (double) to_div / (double) divisions;

		/* printf("from_div = %d, to_div = %d, divisions = %d"
			" time = %g sub = %g, add=%g, new time = %g\n",
			from_div, to_div, divisions, 
			hit->time, sub, add, hit->time - sub  + add); */

		hit->time = hit->time - sub + add;
		hit->beat = (int) (hit->time * DRAW_WIDTH);
		hit->beats_per_measure = DRAW_WIDTH;
		hit->noteoff_time = hit->noteoff_time - sub + add;
		hit->noteoff_beat = (int) (hit->noteoff_time * DRAW_WIDTH);
		hit->noteoff_beats_per_measure = DRAW_WIDTH;
		reduce_fraction(&hit->beat, &hit->beats_per_measure);
	}
	free(map);

	/* Sort h by h->h->time, bubble sort is good enough for now */
	do {
		done = 1;
		h = hp;
		while (h != NULL) { /* traverse through the list */
			next_h = h->next;
			if (next_h == NULL)
				break;
			if (h->h.time > next_h->h.time) {  /* find a pair out of order? */
				done = 0;		   /* found something out of order */
				temp = next_h->h;	   /* not done, swap them. */
				next_h->h = h->h;
				h->h = temp;
			}
			h = h->next;
		}
	} while (!done); /* repeat until we didn't swap anything */

	unflatten_pattern(kit, cpattern);
	check_melodic_mode();
	for (i=0;i<NINSTS; i++)
		gtk_widget_queue_draw(drumkit[kit].instrument[i].canvas);
}

void instrument_clear_button_pressed(UNUSED GtkWidget *widget, 
	struct instrument_struct *inst)
{
	if (inst->hit != NULL) {
		clear_hitpattern(inst->hit);
		inst->hit = NULL;
		gtk_widget_queue_draw(drumkit[kit].instrument[inst->instrument_num].canvas);
	}
}

void instrument_button_pressed(UNUSED GtkWidget *widget, struct instrument_struct *data)
{
	unsigned char velocity;

	velocity = (unsigned char) gtk_range_get_value(GTK_RANGE(data->volume_slider));
	if (data != NULL) {
		int prev = current_instrument;
		printf("%s\n", data->name);
		/* should be changed to *schedule* a noteon + noteoff */
		if (midi->isopen(midi_handle))
			midi->noteon(midi_handle, pattern[cpattern]->tracknum, 
				pattern[cpattern]->channel, data->midivalue, velocity);
		current_instrument = data->instrument_num;
		gtk_widget_queue_draw(drumkit[kit].instrument[prev].canvas);
		gtk_widget_queue_draw(data->canvas);
	}
}


void silence(struct midi_handle *mh);
void destroy_event(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	/* g_print("destroy event!\n"); */

	printf("\n\n\n\n\n\n      Gneutronica quits... your File is saved in /tmp/zzz.gdt\n\n\n\n\n\n");
	save_to_file("/tmp/zzz.gdt");
	if (player_process_pid != -1)
		kill(player_process_pid, SIGTERM); /* a little brutal, but effective . . . */
	if (midi_reader_process_id != -1)
		kill(midi_reader_process_id, SIGTERM);
	usleep(100);
	silence(midi_handle);
	gtk_main_quit();
}


gint delete_event(UNUSED GtkWidget *widget,
		UNUSED GdkEvent *event,
		UNUSED gpointer data)
{
	return TRUE;
}

void song_name_entered(UNUSED GtkWidget *widget, GtkWidget *entry)
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

void hide_instruments_button_callback(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	int i, vshidden, unchecked_hidden, editdrumkit;

	vshidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_volume_sliders));
	unchecked_hidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (hide_instruments));
	editdrumkit = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (edit_instruments_toggle));

	for (i=0;i<drumkit[kit].ninsts;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		bring_back_right_widgets(inst, vshidden, unchecked_hidden, editdrumkit);
#if 0
		if (vshidden) {
			gtk_widget_hide(GTK_WIDGET(inst->clear_button));
			gtk_widget_hide(GTK_WIDGET(inst->volume_slider));
			gtk_widget_hide(GTK_WIDGET(inst->drag_spin_button));
		}
		if (editdrumkit) {
			gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
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
				gtk_widget_hide(GTK_WIDGET(inst->gm_value_spin_button));
			}
			if (!vshidden) { /* otherwise, already hidden */
				gtk_widget_hide(GTK_WIDGET(inst->clear_button));
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
				gtk_widget_show(GTK_WIDGET(inst->gm_value_spin_button));
			}
			if (!vshidden) { /* otherwise, already hidden */
				gtk_widget_show(GTK_WIDGET(inst->clear_button));
				gtk_widget_show(GTK_WIDGET(inst->volume_slider));
				gtk_widget_show(GTK_WIDGET(inst->drag_spin_button));
			}
		}
#endif
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

int transpose_hitpattern(struct hitpattern *p, int interval, int for_real)
{
	int newvalue;
	struct hitpattern *h, *next;
	int out_of_bounds = 0;

	if (p == NULL) {
		return -1;
	}
	for (h = p; h != NULL; h = next) {
		next = h->next;
		newvalue = h->h.instrument_num + interval;
		if (newvalue < 0 || newvalue > 127) {
			out_of_bounds = 1;
		} else {
			if (for_real)
				h->h.instrument_num = newvalue;
		}
	}
	return out_of_bounds;
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

int lookup_gm_equiv(int gm_equiv)
{
	int i;
	struct drumkit_struct *dk = &drumkit[kit];
	struct instrument_struct *inst;
	for (i=0;i<dk->ninsts;i++) {
		inst = &dk->instrument[i];
		if (inst->gm_equivalent == gm_equiv)
			return i;
	}
	printf("Bad instrument gm_equiv %d\n", gm_equiv);
	return -1;
}

void remap_drumkit_hit(struct hit_struct *h, int set_toggles)
{
	int i;
	int gm_equiv;
	struct drumkit_struct *dk = &drumkit[kit];
	struct instrument_struct *inst;

	/* printf("looking up gm equiv for %d -> %d\n",h->instrument_num, song_gm_map[h->instrument_num]); */
	/* lookup instrument_num in the song_gm_map to find the gm equiv */
	gm_equiv = song_gm_map[h->instrument_num];
	if (gm_equiv == -1)
		return; /* nothing we can do... */

	/* look up the gm_equiv in the current drumkit to find the instrument */
	for (i=0;i<dk->ninsts;i++) {
		inst = &dk->instrument[i];
		/* printf("%d ?= %d\n", inst->gm_equivalent, gm_equiv); */
		if (inst->gm_equivalent == gm_equiv) {
			/* printf("Found match, new instnum = %d\n", i); */
			/* reassign the instrument_num */
			h->instrument_num = i;
			if (set_toggles)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
					(gboolean) 1);
			return;
		}
	}
	/* printf("No match found for %d\n", h->instrument_num); */
}

void remap_drumkit_hitpattern (struct hitpattern *p, int set_toggles)
{
	struct hitpattern *h;
	int i;
	struct drumkit_struct *dk = &drumkit[kit];
	struct instrument_struct *inst;
	int hidden = 0;

	if (p == NULL)
		return;

	if (set_toggles)
		for (i=0;i<dk->ninsts;i++) {
			inst = &dk->instrument[i];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inst->hidebutton),
				(gboolean) hidden);
		}
	for (h = p; h != NULL; h = h->next)
		remap_drumkit_hit(&h->h, set_toggles);
}

void remap_drumkit_pattern(struct pattern_struct *p, int set_toggles)
{
	struct hitpattern *hp;

	if (p->gm_converted)
		return;

	hp = p->hitpattern;
	remap_drumkit_hitpattern(hp, set_toggles);
	p->gm_converted = 1;
}

void remap_drumkit_song()
{
	int i;

	flatten_pattern(kit, cpattern);
	for (i=0;i<npatterns;i++)
		if (pattern[i]->music_type == PERCUSSION)
			remap_drumkit_pattern(pattern[i], i == cpattern);
	unflatten_pattern(kit, cpattern);
	edit_pattern(cpattern);
}

void clear_kit_pattern(int ckit)
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
	gboolean percussion;

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
	p->tracknum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(trackspin));
	p->channel = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(channelspin));
	percussion  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(percussion_toggle));
	if (percussion)
		p->music_type = PERCUSSION;
	else
		p->music_type = MELODIC;
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
	for (i=0;i<NINSTS;i++) {
		struct hitpattern *h;
		struct instrument_struct *inst = &dk->instrument[i];
		for (h = inst->hit; h != NULL; h=h->next) {
			lowlevel_add_hit(&p->hitpattern, ckit, cpattern, inst->instrument_num, 
				h->h.beat, h->h.beats_per_measure, 
				h->h.noteoff_beat, h->h.noteoff_beats_per_measure, 
				h->h.velocity, 1);
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
	p->tracknum = 0;
	p->channel = 0;
	p->music_type = PERCUSSION;
	p->hitpattern = NULL;
	p->pattern_num = pattern_num;
	p->beats_per_measure = 4;
	p->beats_per_minute = 120;
	memset(p->drag, 0, sizeof(p->drag[0]) * MAXINSTS);
	sprintf(p->patname, "Pattern %d", pattern_num);
	return p;
}

void pattern_struct_free(struct pattern_struct **p, int npatterns)
{
	int i;
	struct hitpattern *hp, *next;

	for (i = 0; i < npatterns; i++) {
		if (!p[i])
			continue;
		hp = p[i]->hitpattern;
		while (hp) {
			next = hp->next;
			free(hp);
			hp = next;
		}
		free(p[i]);
		p[i] = NULL;
	}
}

int unflatten_pattern(int ckit, int cpattern)
{
	/* copy the flattened pattern instrument by instrument into the 
	   pattern editor struct */
	struct hitpattern *h;
	int i;

	/* Clear out anything old */
	for (i=0;i<NINSTS; i++) {
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
			pattern[cpattern]->tracknum = pattern[cpattern-1]->tracknum;
			pattern[cpattern]->channel = pattern[cpattern-1]->channel;
			pattern[cpattern]->music_type = pattern[cpattern-1]->music_type;
			/* printf("copying tempo, %d, %d\n", 
				pattern[cpattern]->beats_per_measure,
				pattern[cpattern]->beats_per_minute); */
		} else {
			pattern[cpattern]->channel = DEFAULT_MIDI_CHANNEL;
		}
	}
	if (pattern[cpattern]->arr_button != NULL)
		gtk_button_set_label(GTK_BUTTON(pattern[cpattern]->arr_button), 
			pattern[cpattern]->patname);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin1), (gdouble) pattern[cpattern]->beats_per_minute);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin2), (gdouble) pattern[cpattern]->beats_per_measure);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(trackspin), (gdouble) pattern[cpattern]->tracknum);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(channelspin), (gdouble) pattern[cpattern]->channel);
	if (pattern[cpattern]->music_type == PERCUSSION) 
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(percussion_toggle),
			(gboolean) 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(percussion_toggle),
			(gboolean) 0);

	for (h = pattern[cpattern]->hitpattern; h != NULL; h = h->next) {
		/* g_print("inst = %d, beat=%d, bpm =%d\n", h->h.instrument_num, 
			h->h.beat, h->h.beats_per_measure); fflush(stdout); */
		/* printf("instrument_num = %d\n", h->h.instrument_num); */
		if (h->h.instrument_num != -1)
			lowlevel_add_hit(&drumkit[ckit].instrument[h->h.instrument_num].hit,
				ckit, cpattern, h->h.instrument_num, 
				h->h.beat, h->h.beats_per_measure, 
				h->h.noteoff_beat, h->h.noteoff_beats_per_measure, 
				h->h.velocity, 1);
		else
			printf("Bad instrument number in pattern %d...\n", cpattern);
		/* add_hit(&drumkit[ckit].instrument[h->h.instrument_num].hit, 
			h->h.beat, h->h.beats_per_measure,
			ckit, h->h.instrument_num, cpattern); */
	}

	for (i=0;i<ndivisions;i++) {
		timediv[i].division = pattern[cpattern]->timediv[i].division;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(timediv[i].spin), 
			(gdouble) timediv[i].division);
	}
	for (i=0;i<NINSTS; i++) {
		struct instrument_struct *inst = &drumkit[ckit].instrument[i];
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(inst->drag_spin_button), 
			(gdouble) pattern[cpattern]->drag[i]);
	}
	set_pattern_window_title();
	return 0;
}

int xpect(FILE *f, int *lc, char *line, char *value)
{
	int rc;

	/* TODO: fix this, use fgets() for fuck's sake. */
	rc = fscanf(f, "%[^\n]%*c", line);
	if (rc != 1) {
		printf("Failed to read a string at line %d: %s\n", *lc, line);
		return -1;
	}
	if (strncmp(line, value, strlen(value)) != 0) {
		printf("Error at line %d:%s\n", *lc, line);
		printf("Expected '%s', got '%s'\n", value, line); 
		return -1;
	}
	(*lc)++;
	return 0;	
}

int load_from_file_version_4(FILE *f)
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
		fprintf(stderr, "gneutronica: Expected Songname at line %d.\n", linecount);
		goto error_out;
	}
	linecount++;

	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;

	rc = fscanf(f, "Drumkit Make:%[^\n]%*c", dkmake); linecount++;
	if (rc != 1) {
		printf("Failed to read Drumkit make\n");
		goto error_out;
	}
	rc = fscanf(f, "Drumkit Model:%[^\n]%*c", dkmodel); linecount++;
	if (rc != 1) {
		printf("Failed to read Drumkit model\n");
		goto error_out;
	}
	rc = fscanf(f, "Drumkit Name:%[^\n]%*c", dkname); linecount++;
	if (rc != 1) {
		printf("Failed to read Drumkit name\n");
		goto error_out;
	}

	
	
	same_drumkit = (strcmp(drumkit[kit].make, dkmake) == 0 && 
		strcmp(drumkit[kit].model, dkmodel) == 0 &&
		strcmp(drumkit[kit].name, dkname) == 0);

	/* printf("'%s','%s','%s'\n", dkmake, dkmodel, dkname);
	printf(same_drumkit ? "Same drumkit\n" : "Different drumkit\n"); */

	if (!same_drumkit)
		printf("WARNING: Different drum kit for this song, remap to adjust to current drum kit...\n");

	rc = fscanf(f, "Instruments: %d\n", &ninsts);
	if (rc != 1)  {
		printf("%d: error, expected Instruments\n", linecount);
		goto error_out;
	}
	linecount++;

	for (i=0;i<ninsts;i++) {
		rc = fscanf(f, "Instrument %*d: '%*[^']%*c %d %d\n", &hidden, &gm_equiv);
		if (rc != 2) {
			fprintf(stderr, "%d: expected Instrument line\n", linecount);
		}
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
		linecount++;
	}
	rc = fscanf(f, "Patterns: %d\n", &npatterns);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected Patterns\n", linecount);
		return -1;
	}
	linecount++;
	for (i = 0; i < npatterns; i++) {
		struct hitpattern **h;
		pattern[i] = pattern_struct_alloc(i);
		h = &pattern[i]->hitpattern;
		rc = fscanf(f, "Pattern %*d: %d %d %d %d %d %[^\n]%*c", 
				&pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, 
				&pattern[i]->tracknum,
				&pattern[i]->channel,
				&pattern[i]->music_type,
				pattern[i]->patname);
		if (rc != 6) {
			fprintf(stderr, "%d: Expected Pattern ...", linecount);
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
		pattern[i]->gm_converted = 0;
		while (1) {
			rc = fscanf(f, "%[^\n]%*c", line);
			if (rc != 1) {
				fprintf(stderr, "%d: failed to read line\n", linecount);
				goto error_out;
			}
			if (strcmp(line, "END-OF-PATTERN") == 0) {
				linecount++;
				/* printf("end of pattern\n"); fflush(stderr); */
				break;
			}
			linecount++;
			*h = malloc(sizeof(struct hitpattern));
			memset(*h, 0, sizeof(**h));
			(*h)->next = NULL;
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d %lg %d %d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure,
				&(*h)->h.noteoff_time,
				&(*h)->h.noteoff_beat,
				&(*h)->h.noteoff_beats_per_measure);

			if (rc != 9) {
				fprintf(stderr, "%d: failed to read first line of hitpattern\n", linecount);
				goto error_out;
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
			(*h)->h.noteoff_time = (double) (*h)->h.noteoff_beat /
				(double) (*h)->h.noteoff_beats_per_measure;

			/* printf("new time is %g\n", (*h)->h.time); */
			h = &(*h)->next;
		}
		rc = fscanf(f, "dragging count: %d\n", &count);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected dragging count\n", linecount);
			goto error_out;	
		}
		linecount++;
		for (j = 0; j < count; j++) {
			int ins;
			long drag;
			rc = fscanf(f, "i:%d, d:%ld\n", &ins, &drag);
			if (rc != 2) {
				fprintf(stderr, "%d: failed to read instrument drag\n", linecount);
				goto error_out;
			}
			linecount++;
			pattern[i]->drag[ins] = (double) (drag / 1000.0);
		}
		make_new_pattern_widgets(i, i+1);
	}
	rc = fscanf(f, "Measures: %d\n", &nmeasures);
	if (rc != 1) {
		fprintf(stderr, "%d: Expected measure count\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < nmeasures; i++) {
		rc = fscanf(f, "m:%*d np:%d\n", &measure[i].npatterns);
		if (rc != 1) {
			fprintf(stderr, "%d: Expected pattern count for measure\n", linecount);
			goto error_out;
		}
		linecount++;
		if (measure[i].npatterns != 0) {
			for (j = 0; j < measure[i].npatterns; j++) {
				rc = fscanf(f, "%d ", &measure[i].pattern[j]);
				if (rc != 1) {
					fprintf(stderr, "%d: Expected pattern number\n", linecount);
					goto error_out; 
				}
			}
			rc = fscanf(f, "\n");
			if (rc != 0) {
				fprintf(stderr, "%d: expected newline\n", linecount);
				goto error_out;
			}
			linecount++;
		}
		/* printf("m:%*d t:%d p:%d\n", measure[i].tempo, measure[i].pattern); */
	}
	rc = fscanf(f, "Tempo changes: %d\n", &ntempochanges);
	if (rc != 1) {
		fprintf(stderr, "%d: expected Tempo changes\n", linecount);
		goto error_out;
	}
	linecount++;
	for (i = 0; i < ntempochanges; i++) {
		rc = fscanf(f, "m:%d bpm:%d\n", &tempo_change[i].measure,
			&tempo_change[i].beats_per_minute);
		if (rc != 2) {
			fprintf(stderr, "%d: Expected tempo change instance\n", linecount);
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

static void remove_tempo_change(int index)
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

void init_measures();

int import_patterns_v4(FILE *f)
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
	if (rc != 1)
		printf("Expected song name at line %d\n", linecount);
	linecount++;
	if (xpect(f, &linecount, line, "Comment:") == -1) return -1;

	rc = fscanf(f, "Drumkit Make:%[^\n]%*c", dkmake); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit make\n");
	rc = fscanf(f, "Drumkit Model:%[^\n]%*c", dkmodel); linecount++;
	if (rc != 1)
		printf("Failed to read Drumkit model\n");
	rc = fscanf(f, "Drumkit Name:%[^\n]%*c", dkname); linecount++;
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
			return -1;
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
		rc = fscanf(f, "Pattern %*d: %d %d %d %d %d %[^\n]%*c", &pattern[i]->beats_per_measure,
				&pattern[i]->beats_per_minute, 
				&pattern[i]->tracknum, 
				&pattern[i]->channel, 
				&pattern[i]->music_type, 
				pattern[i]->patname);
		if (rc != 6) {
			fprintf(stderr, "%d: Expected Pattern ...\n", linecount);
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
				fprintf(stderr, "%d: Failed to read a line\n", linecount);
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
			rc = sscanf(line, "T: %lg DK: %d I: %d V: %hhu B:%d BPM:%d %lg %d %d\n",
				&(*h)->h.time, &(*h)->h.drumkit, &(*h)->h.instrument_num,
				&(*h)->h.velocity, &(*h)->h.beat, &(*h)->h.beats_per_measure,
				&(*h)->h.noteoff_time,
				&(*h)->h.noteoff_beat,
				&(*h)->h.noteoff_beats_per_measure);
			if (rc != 9) {
				fprintf(stderr, "%d: Failed to read hit\n", linecount);
				goto error_out;
			}
			linecount++;
			gm = import_inst_map[(*h)->h.instrument_num];
			if (gm != -1)
				for (k = 0; k < drumkit[kit].ninsts; k++) {
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
			(*h)->h.noteoff_time = (double) (*h)->h.noteoff_beat /
				(double) (*h)->h.noteoff_beats_per_measure;

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
				fprintf(stderr, "%d: Expected instrument drag...\n", linecount);
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

int translate_drumtab_data(UNUSED int factor)
{
	int i, j;
	struct hitpattern **h;
	struct pattern_struct *p;
	struct dt_hit_type *dth;
	int maxmeasure = 0;

	/* Add patterns */
	for (i=0;i<dt_npats;i++) {
		if (dt_pat[i].measure > maxmeasure)
				maxmeasure = dt_pat[i].measure;
		if (dt_pat[i].duplicate_of != -1)
			continue;
		if (dt_pat[i].hit == NULL) /* empty pattern */
			continue;
		/* printf("Adding pattern %d\n", i); */
		pattern[npatterns] = pattern_struct_alloc(npatterns);
		p = pattern[npatterns];
		if (p == NULL)
			return -1;
		h = &p->hitpattern;
		p->timediv[0].division = 4,
		p->timediv[1].division = 16,
		p->timediv[2].division = 0;
		p->timediv[3].division = 0,
		p->timediv[4].division = 0;
		p->beats_per_minute = 120;
		p->beats_per_measure = 4;
		if (npatterns == 0)
			p->channel = DEFAULT_MIDI_CHANNEL;
		else
			p->channel = pattern[npatterns - 1]->channel;
		/* sprintf(p->patname, "%s %d", dt_inst[dt_pat[i].hit->inst].name, i); */
		sprintf(p->patname, "Pattern %d",npatterns);
		dt_pat[i].gn_pattern = npatterns;
		for (dth = dt_pat[i].hit; dth; dth = dth->next) {
			*h = malloc(sizeof(struct hitpattern));
			(*h)->next = NULL;
			(*h)->h.time = (double) dth->numerator / (double) dth->denominator;
			(*h)->h.drumkit = kit;
			(*h)->h.instrument_num = lookup_gm_equiv(dt_inst[dth->inst].midi_value);
			(*h)->h.beat = dth->numerator;
			(*h)->h.beats_per_measure = dth->denominator;
			(*h)->h.velocity = dth->velocity;
			h = &(*h)->next;
			/* printf("Add hit, %s: velocity = %d, midi=%d, gm inst=%d.\n",
				dt_inst[dth->inst].name,
				dth->velocity, dt_inst[dth->inst].midi_value,
				lookup_gm_equiv(dt_inst[dth->inst].midi_value)); */
		}
		make_new_pattern_widgets(npatterns, npatterns+1);
		npatterns++;
	}
	for (i=0;i<dt_npats;i++) {
		if (dt_pat[i].duplicate_of != -1)
			dt_pat[i].gn_pattern = dt_pat[dt_pat[i].duplicate_of].gn_pattern;
	}
	/* Add the measures */

	/* printf("maxmeasure = %d\n", maxmeasure); */
	for (i=0;i<maxmeasure+1;i++)
		measure[i+nmeasures].npatterns = 0;
	for (i=0;i<maxmeasure+1;i++) {
		for (j=0;j<dt_npats;j++) {
			if (dt_pat[j].measure == i) {
				measure[i+nmeasures].pattern[measure[i+nmeasures].npatterns] = dt_pat[j].gn_pattern;
				measure[i+nmeasures].npatterns++;
			}
		}
	}
	nmeasures += maxmeasure+1;
	redraw_arranger();
	return 0;
}

int import_patterns_from_file(const char *filename)
{
	/* imports the patterns from another song into the current song. */
	FILE *f;
	int rc;
	int fileformatversion;

	/* printf("Import patterns from: %s\n", filename); */

	f = fopen(filename, "r");
	if (f == NULL) {
		printf("Nope, can't open %s, %s\n", filename, strerror(errno));
		return -1;
	}
	rc = fscanf(f, "Gneutronica file format version: %d\n", &fileformatversion);
	if (rc != 1) {
		printf("File %s does not appear to be a %s file.\n", filename,
			PROGNAME);
		fclose(f);
		return -1;
	}

	switch (fileformatversion) {
		case 1: printf("\n\nSorry, can't import patterns from this old file format\n");
			printf("(version %d).\n\n", fileformatversion); 
			printf("Load this song file into %s, then save it to a new file,\n", PROGNAME);
			printf("and then import the patterns from the new file.\n\n\n"); 
			break;
		case 2: rc = import_patterns_v2(f);
			break;
		case 3: rc = import_patterns_v3(f);
			break;
		case 4: rc = import_patterns_v4(f);
			break;
		default: printf("Unsupported file format version: %d\n", 
			fileformatversion);
			return -1;
	}
	fclose(f);
	return rc;
}

int load_from_file(const char *filename)
{
	FILE *f;
	int i, rc;
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
		fclose(f);
		return -1;
	}

	rc = fscanf(f, "Gneutronica file format version: %d\n", &fileformatversion);
	if (rc != 1) {
		printf("File does not appear to be a %s file.\n",
			PROGNAME);
		fclose(f);
		return -1;
	}

	printf("FILEFORMAT is %d\n", fileformatversion);

	switch (fileformatversion) {
		case 1: rc = load_from_file_version_1(f);
			break;
		case 2: rc = load_from_file_version_2(f);
			break;
		case 3: rc = load_from_file_version_3(f);
			break;
		case 4: rc = load_from_file_version_4(f);
			break;
		default: printf("Unsupported file format version: %d\n", 
			fileformatversion);
			fclose(f);
			return -1;
	}
	if (rc == 0)
		set_arranger_window_title();
	fclose(f);
	return rc;
}


int current_file_format_version = 4;
int save_to_file(const char *filename)
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
		fprintf(f, "Instrument %d: '%s' %d %d\n", i, drumkit[kit].instrument[i].type, 
				(int) hidden, drumkit[kit].instrument[i].gm_equivalent);
	}

	/* Save the patterns */
	fprintf(f, "Patterns: %d\n", npatterns);
	for (i=0;i<npatterns;i++) {
		int count;
		long drag;
		struct hitpattern *h;
		fprintf(f, "Pattern %d: %d %d %d %d %d %s\n", i, pattern[i]->beats_per_measure,
			pattern[i]->beats_per_minute, 
			pattern[i]->tracknum, 
			pattern[i]->channel, 
			pattern[i]->music_type, 
			pattern[i]->patname);
		fprintf(f, "Divisions: %d %d %d %d %d\n", 
			pattern[i]->timediv[0].division,
			pattern[i]->timediv[1].division,
			pattern[i]->timediv[2].division,
			pattern[i]->timediv[3].division,
			pattern[i]->timediv[4].division);
		for (h = pattern[i]->hitpattern; h != NULL; h=h->next) {
			fprintf(f, "T: %g DK: %d I: %d V: %d B:%d BPM:%d %g %d %d\n",
				h->h.time, h->h.drumkit, h->h.instrument_num, h->h.velocity,
				h->h.beat, h->h.beats_per_measure, 
				h->h.noteoff_time,
				h->h.noteoff_beat,
				h->h.noteoff_beats_per_measure);
		}
		fprintf(f, "END-OF-PATTERN\n");
		count = 0;
		for (j = 0;j<drumkit[kit].ninsts;j++)
			if (pattern[i]->drag[j] != 0.0)
				count++;
		fprintf(f, "dragging count: %d\n", count);
		for (j = 0;j<drumkit[kit].ninsts;j++)
			if (pattern[i]->drag[j] != 0.0) {
				drag = (long) (pattern[i]->drag[j] * 1000.0);
				fprintf(f, "i:%d, d:%ld\n", j, drag);
			}
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

void silence(struct midi_handle *mh)
{
	int i,j,k;
	for (i=0;i<MAXINSTS;i++)
		for (j=0;j<MAXTRACKS;j++)
			for (k=0;j<MAXCHANS;j++)
				midi->noteoff(mh, j, k, i);
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
		fprintf(stderr, "Error writing command to player process: %s.\n", strerror(errno));
		return;
	}
	rc = write(player_process_fd, &sched->nevents, sizeof(sched->nevents));
	if (rc == -1) {
		fprintf(stderr, "Error writing event count to player process: %s\n", strerror(errno));
		return;
	}

	for (i=0;i<sched->nevents;i++) {
		rc = write(player_process_fd, sched->e[i], sizeof(*sched->e[i]));
		if (rc == -1) {
			fprintf(stderr, "Error writing event to player process: %s\n", strerror(errno));
		}
	}
	return;
}

void receive_midi_data(UNUSED int signal)
{
	/* This is called whenever the midi_reader process gets some midi data */
	/* from a midi input device, eg. Akai MPD16, or some such. */
	struct shared_info_struct *s = (struct shared_info_struct *) transport_location;
	unsigned char data[3];
	double thetime;
	struct instrument_struct *inst;
	int i;

	printf("Received MIDI data:");

	/* get semaphore */
	memcpy(data, &s->midi_data[0], 3);
	/* release semaphore */

	printf("0x%02x 0x%02x 0x%02x\n",
		data[0], data[1], data[2]);
	fflush(stdout);
	/* release semaphore */
	if (data[0] == 0x90) {
		midi->noteon(midi_handle, pattern[cpattern]->tracknum, 
			pattern[cpattern]->channel, data[1], data[2]);
		if (record_mode) {
			/* There is quite likely a much better way to do this time calc... */ 
			thetime = ((double) transport_location->percent);
			printf("percent = %d, thetime = %g\n", transport_location->percent, thetime);

			/* Find the instrument with this midi note. */
			if (!melodic_mode) {
				for (i=0;i<drumkit[kit].ninsts; i++) {
					inst = &drumkit[kit].instrument[i];
					if (inst->midivalue == data[1]) {
						add_hit(&inst->hit, (double) thetime, -1.0, 
							(double) 100.0, 
							kit, inst->instrument_num, cpattern, data[2], 1);
						
						gtk_widget_queue_draw(inst->canvas);
						break;
					}
				}
			} else {
				inst = &drumkit[kit].instrument[data[1]];
				add_hit(&inst->hit, (double) thetime, -1.0, (double) 100.0, 
						kit, inst->instrument_num, cpattern, data[2], 1);
				gtk_widget_queue_draw(inst->canvas);
			}
		}
	}
	/* process data */
	return;
}

void setup_midi_receiver()
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_handler = receive_midi_data;
	sigemptyset(&action.sa_mask);
	sigaction(SIGUSR1, &action, NULL);
	return;
}

static jmp_buf the_beginning;

void sigusr1_handler(UNUSED int signal)
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

	silence(midi_handle);

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
				transport_location->measure = -1;
				break;
				}
			case PLAYER_QUIT:
				silence(midi_handle);
				transport_location->measure = -1;
				exit(0);
			case PERFORM_PATCH_CHANGE: {
				unsigned short bank;
				unsigned char patch;
				rc = read(fd, &bank, sizeof(bank));
				if (rc != sizeof(bank)) {
					fprintf(stderr, "Failed to read bank\n");
					break;
				}
				rc = read(fd, &patch, sizeof(patch));
				if (rc != sizeof(patch)) {
					fprintf(stderr, "Failed to read patch\n");
					break;
				}
				printf("Player: Changing to bank %d, patch %d\n", bank, patch);
				midi->patch_change(midi_handle, 0, MIDI_CHANNEL, bank, patch);
				break;
			}
			default:
				printf("Player received unknown cmd: %d\n", cmd);
		}
	}
}

int fork_player_process(UNUSED char *device, int *fd)
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

int midi_change_patch(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	/* sends a command to the player process to make it send a bank/patch change to 
	   the MIDI device */
	unsigned short bank;
	unsigned char patch;
	unsigned char cmd = PERFORM_PATCH_CHANGE;
	int rc;

	bank = (unsigned short) (gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(midi_bank_spin_button))) & 0x00ffff;
	patch = (unsigned char) (gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(midi_patch_spin_button))) & 0x0f;
	
	drumkit[kit].midi_bank = bank;
	drumkit[kit].midi_patch = patch;

	rc = write(player_process_fd, &cmd, 1);
	rc += write(player_process_fd, &bank, sizeof(bank));
	rc += write(player_process_fd, &patch, sizeof(patch));
	if (rc != 1 + sizeof(bank) + sizeof(patch))
		printf("Failed to change patch.\n");
	return TRUE;
}

int midi_setup_activate(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	gtk_widget_show(midi_setup_window);
	return TRUE;
}

int midi_setup_cancel(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(midi_channel_spin_button), 
		(gdouble) drumkit[kit].midi_channel);
	gtk_widget_hide(midi_setup_window);
	return TRUE;
}

int midi_setup_ok(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
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

void about_ok_callback(__attribute__((unused)) GtkWidget *widget, __attribute__((unused)) gpointer data)
{
	gtk_widget_hide(about_window);
}

int about_activate(__attribute__((unused)) GtkWidget *widget, __attribute__((unused)) gpointer data)
{

	static char about_msg[1024];

	if (about_window == NULL) {
		snprintf(about_msg, sizeof(about_msg), "\n\n%s v. %s\n\n"
			"Gneutronica is a MIDI drum machine\n\n"
			COPYRIGHT "\n\n"
			"http://sourceforge.net/projects/gneutronica\n\n",
			PROGNAME, VERSION);

		robotdrummer = gdk_pixbuf_new_from_file ("/usr/local/share/gneutronica/documentation/gneutronica_robot.png", NULL);
		about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		about_vbox = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER (about_window), about_vbox);
		if (robotdrummer != NULL)
			about_da = gtk_image_new_from_pixbuf(robotdrummer);
		about_label = gtk_label_new(about_msg);
		about_ok_button = gtk_button_new_with_label("Ok");
		if (about_da != NULL)
			gtk_box_pack_start(GTK_BOX(about_vbox), about_da, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(about_vbox), about_label, TRUE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(about_vbox), about_ok_button, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT (about_window), "destroy", 
			G_CALLBACK (gtk_widget_hide), NULL);
		g_signal_connect(G_OBJECT (about_window), "delete_event", 
			G_CALLBACK (gtk_widget_hide), NULL);
		g_signal_connect(G_OBJECT (about_ok_button), "clicked",
				G_CALLBACK (about_ok_callback), NULL);
	}
	gtk_widget_show_all(about_window);
	return TRUE;
}

void trigger_about_activate(GtkWidget *widget, gpointer data)
{
	about_activate(widget, data);
}

int volume_magnifier_changed(__attribute__((unused)) GtkWidget *widget, __attribute__((unused)) gpointer data)
{
	gtk_widget_queue_draw(drumkit[kit].instrument[current_instrument].canvas);
	return TRUE;
}


/* This "exclude_keypress" stuff is used for ignoring keypresses when 
   certain widgets have focus */

void widget_exclude_keypress(GtkWidget *w)
{
	
	if (n_exclude_keypress_widgets >= EXCLUDE_LIST_SIZE)
		return;
	exclude_keypress_list[n_exclude_keypress_widgets] = w;
	n_exclude_keypress_widgets++;
}

int should_exclude_keypress(GtkWidget *w)
{
	int i;
	for (i=0;i<n_exclude_keypress_widgets;i++)
		if (exclude_keypress_list[i] == w)
			return 1;
	return 0;
}

static gint key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
	char *x = (char *) data;
	int mycontext;
	GtkWidget *who_has_focus;

#define ARRANGER_CONTEXT 1
#define PATTERN_CONTEXT 2
#define UNKNOWN_CONTEXT 3

	mycontext = UNKNOWN_CONTEXT;

	if (strcmp(x, "arranger") == 0)
		mycontext = ARRANGER_CONTEXT;
	else if (strcmp(x, "main_window") == 0)
		mycontext = PATTERN_CONTEXT;

	who_has_focus = GTK_WINDOW(arranger_window)->focus_widget;
	if (who_has_focus != NULL && should_exclude_keypress(who_has_focus)) {
		/* printf("Ignoring keypress due to widget with focus.\n"); */
		return FALSE;
	}
#if 0
	if (event->length > 0)
		printf("The key event's string is `%s'\n", event->string);

	printf("The name of this keysym is `%s'\n", 
		gdk_keyval_name(event->keyval));
#endif
	switch (event->keyval)
	{
	case GDK_q:
		destroy_event(widget, NULL);
		return TRUE;	
#if 0
	case GDK_Home:
		printf("The Home key was pressed.\n");
		break;
	case GDK_Up:
		printf("The Up arrow key was pressed.\n");
		break;
#endif
	case GDK_Right:
	case GDK_period:
	case GDK_greater:
		if (mycontext == PATTERN_CONTEXT)
			nextbutton_clicked(nextbutton, NULL);		
		return TRUE;
	case GDK_Left:
	case GDK_comma:
	case GDK_less:
		if (mycontext == PATTERN_CONTEXT)
			prevbutton_clicked(prevbutton, NULL);		
		return TRUE;
	case GDK_space:
		pattern_stop_button_clicked(stop_button, NULL);
		break;	
	case GDK_c:
		if (mycontext == PATTERN_CONTEXT)
			copy_current_instrument_hit_pattern();
		return TRUE;
	case GDK_x:
		if (mycontext == PATTERN_CONTEXT &&
			current_instrument >= 0 && 
			current_instrument < NINSTS)
			instrument_clear_button_pressed(NULL, 
				&drumkit[kit].instrument[current_instrument]);
		return TRUE;
	case GDK_p:
		if (mycontext == PATTERN_CONTEXT)
			paste_current_instrument_hit_pattern();
		return TRUE;
	case GDK_P:
		if (mycontext == PATTERN_CONTEXT)
			paste_all_current_instrument_hit_pattern();
		return TRUE;
	case GDK_X:
		if (mycontext == PATTERN_CONTEXT)
			clear_all_current_instrument_hit_pattern();
		return TRUE;
	case GDK_K: /* transpose up -- like vi. */
		if (mycontext == PATTERN_CONTEXT && melodic_mode)
			transpose(-1);
		return TRUE;
	case GDK_J: /* transpose down -- like vi */
		if (mycontext == PATTERN_CONTEXT && melodic_mode)
			transpose(1);
		return TRUE;
	default:
		break;
	}

#if 0
	printf("Keypress: GDK_%s\n", gdk_keyval_name(event->keyval));
	if (gdk_keyval_is_lower(event->keyval)) {
		printf("A non-uppercase key was pressed.\n");
	} else if (gdk_keyval_is_upper(event->keyval)) {
		printf("An uppercase letter was pressed.\n");
	}
#endif
	return FALSE;
}

void track_mute_toggle_callback(GtkWidget *widget, gpointer data)
{
	/* What's passed in in "data" ia a pointer to a specific element
	   in the transport_location->muted array that corresponds to the
	   toggle.  The entire transport_location structure is in shared
	   memory with the player process.  The player process checks the
	   track/channel as it plays each note, and if muted, substitutes
	   a noteoff for noteon, thus muting the channel.  No synchronization
	   needed because this is the only place that writes, and the player
	   process is the only place that reads. */

	int *muted = (int *) data;

	/* mute (or unmute) the track/channel */
	*muted = (int) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
}

void create_default_config_file()
{
	FILE *f;
	char path[PATH_MAX];
	char *home;

	home = getenv("HOME");
	if (home == NULL)
		return;	

	sprintf(path, "%s/.gneutronica", home);	
	(void) mkdir(path, 0775);

	sprintf(path, "%s/.gneutronica/gneutronica.conf", home);
	f = fopen(path, "w+");
	if (f == NULL) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		return;
	}

	fprintf(f, "# gneutronica configuration file\n");
	fprintf(f, "#\n");
	fprintf(f, "#maximize_windows=0\n");
	fprintf(f, "#drumkitfile=/usr/local/share/gneutronica/drumkits/general_midi_standard.dk\n");
	fprintf(f, "#input_device=\n");
	fprintf(f, "#output_device=hw\n");
	fprintf(f, "#automag=1\n");
	fclose(f);
}

void read_config_file(char *drumkitfile, char *output_device,
		char *midi_input_device, int *maximize_windows,
		int *automag_initial_state)
{
	FILE *f;
	char data[256];
	char path[PATH_MAX];
	char *home;
	char s[255];
	int x, rc;
	int line = 0;
	char *fgetsrc;

	home = getenv("HOME");
	if (home == NULL)
		return;

	sprintf(path, "%s/.gneutronica/gneutronica.conf", home);
	
	f = fopen(path, "r"); 
	if (f == NULL) {
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		if (errno == ENOENT)
			create_default_config_file();
		else
			fprintf(stderr, "gneutronica: error opening '%s': %s\n", 
				"~/.gneutronica/gneutronica.conf",
				strerror(errno));
		return;
	}

	while (!feof(f)) {

		memset(data, 0, sizeof(data));

		fgetsrc = fgets(data, 200, f);
		if (fgetsrc == NULL)
			break;

		line++;
		if (data[0] == '#')
			continue; /* skip comments. */

		rc = sscanf(data, "maximize_windows=%d", &x);
		if (rc == 1) {
			*maximize_windows = x;
			continue;
		}

		rc = sscanf(data, "drumkitfile=%s%*c", s);
		if (rc == 1) {
			strcpy(drumkitfile, s);
			continue;
		}

		rc = sscanf(data, "input_device=%s%*c", s);
		if (rc == 1) {
			strcpy(midi_input_device, s);
			continue;
		}

		rc = sscanf(data, "output_device=%s%*c", s);
		if (rc == 1) {
			strcpy(output_device, s);
			continue;
		}

		rc = sscanf(data, "automag=%d%*c", &x);
		if (rc == 1) {
			*automag_initial_state = x;
			continue;
		}
	}
	fclose(f);
}

int main(int argc, char *argv[])
{
	GtkWidget *track_control_box;
	GtkWidget *track_table;
	GtkWidget *track_mute[MAXTRACKS][MAXCHANS];
	GtkWidget *track_num_label[MAXTRACKS];
	GtkWidget *chan_label[MAXCHANS];
	GtkWidget *channel_mute_label;
	GtkWidget *track_control_label;

	GtkWidget *abox;
	GtkWidget *menu_box;
	GtkWidget *a_button_box;
	/* GtkWidget *save_button;
	GtkWidget *load_button; */
	GtkWidget *pattern_play_button;
	GtkWidget *pattern_stop_button;
	GtkWidget *pattern_clear_button;
	GtkWidget *box1, *box2, *middle_box, *linebox;
	GtkWidget *topbox;
	GtkWidget *checkvbox;
	GtkWidget *table;
	GtkWidget *misctable;
	GtkWidget *magbox;
	GtkWidget *volume_zoom_label;
	GtkWidget *scramble_button;
	GtkWidget *pattern_editor_label;
	GtkWidget *arranger_label;
	GtkWidget *arranger_top_box;
	GtkWidget *track_box;
	GtkWidget *channel_box;
	int maximize_windows = 1;
	int automag_initial_state = AUTOMAG_ON;

	struct drumkit_struct *dk;
	int i,j, rc;

	/* ----- open the midi output device ------------------------ */
        int ifd, c;
        char output_device[255], drumkitfile[255], midi_input_device[255];

	memset(&transport_location, 0, sizeof(transport_location));
	transport_location = (struct shared_info_struct *) 
		mmap(shared_buf, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	if (transport_location == (void *) -1) {
		perror("mmap");
		exit(1);
	}
	set_transport_meter(&transport_location->measure, &transport_location->percent);
	set_muted_array(&transport_location->muted[0][0], MAXTRACKS);

	transport_location->measure = 0;
	transport_location->percent = 0;

	strcpy(output_device, midi->default_file());
	strcpy(drumkitfile, "drumkits/default_drumkit.dk");
	strcpy(drumkitfile, "drumkits/generic.dk");
	strcpy(drumkitfile, "drumkits/Roland_Dr660_Standard.dk");
	strcpy(drumkitfile, "drumkits/yamaha_motifr_rockst1.dk");
	strcpy(drumkitfile, "/usr/local/share/gneutronica/drumkits/general_midi_standard.dk");
	strcpy(midi_input_device, "");

	read_config_file(drumkitfile, output_device,
		midi_input_device, &maximize_windows,
		&automag_initial_state);

        while ((c = getopt(argc, argv, "mi:k:d:")) != -1) {
                switch (c) {
                case 'd': strcpy(output_device, optarg); break;
                case 'i': strcpy(midi_input_device, optarg); break;
                case 'k': strcpy(drumkitfile, optarg); break;
		case 'm': maximize_windows = 0; break;
                }
        }

	midi_handle = midi->open((unsigned char *) output_device, MAXTRACKS);
	if (midi_handle == NULL) {
		fprintf(stderr, "Can't open MIDI device file %s\n", output_device);
		exit(1);
	}
	
	player_process_pid = fork_player_process(output_device, &player_process_fd);
	pattern = malloc(sizeof(struct pattern_struct *) * MAXPATTERNS);
	memset(pattern, 0, sizeof(struct pattern_struct *) * MAXPATTERNS);
	init_measures();
	npatterns = 0;
	nmeasures = 0;
	cpattern = 0;
	cmeasure = 0;
	ntempochanges = 1;
	tempo_change[0] = initial_change;
	sprintf(songname, UNTITLED_SONG_LABEL);

	/* open midi input device */
	ifd = -1;
	if (strcmp(midi_input_device, "") != 0) {
		printf("Starting midi reader\n");
		ifd = open(midi_input_device, O_RDONLY);
		printf("ifd = %d\n", ifd);
		if (ifd >= 0) {
			struct shared_info_struct *sis = (struct shared_info_struct *) transport_location;
			setup_midi_receiver();
			midi_reader_process_id = midi_reader(ifd, &sis->midi_data[0]);
			printf("midi_reader started, pid=%d.\n",
				midi_reader_process_id);
		} else {
			fprintf(stderr, "Error opening midi input device: %s\n", strerror(errno));
		}
	}

	rc = read_drumkit(drumkitfile, &ndrumkits, drumkit);
	if (rc != 0) {
		fprintf(stderr, "Can't read drumkit file, "
			"perhaps you need to specify '-k drumkitfile' option?\n");
		fprintf(stderr, "Proceeding anyway with a very generic default kit instead.\n");
		make_default_drumkit(&ndrumkits, drumkit);
	}
	kit = 0;
	dk = &drumkit[kit];

	gtk_init(&argc, &argv);

	g_print("Using drumkit: %s %s %s\n", 
		dk->make, dk->model, dk->name);

	gdk_color_parse("white", &whitecolor);
	gdk_color_parse("light gray", &lightgraycolor);
	gdk_color_parse("blue", &bluecolor);
	gdk_color_parse("black", &blackcolor);

	arranger_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL); */
	top_window = gtk_vbox_new(FALSE,0); /* contains everything in pattern editor tab */
	arranger_top_box = gtk_vbox_new(FALSE,0); /* Contains everything in arranger notebook tab */

	g_signal_connect(G_OBJECT (arranger_window), "key_press_event",
			G_CALLBACK (key_press_cb), "main_window");

	tooltips = gtk_tooltips_new();

#if 0
	g_signal_connect(G_OBJECT (top_window), "delete_event", 
		// G_CALLBACK (delete_event), NULL);
		G_CALLBACK (destroy_event), NULL);
	g_signal_connect(G_OBJECT (top_window), "destroy", 
		G_CALLBACK (destroy_event), NULL);
#endif

	gtk_container_set_border_width(GTK_CONTAINER (top_window), 15);

	box1 = gtk_vbox_new(FALSE, 0);
	topbox = gtk_hbox_new(FALSE, 0);
	checkvbox = gtk_vbox_new(FALSE,0);
	box2 = gtk_hbox_new(FALSE, 0);
	middle_box = gtk_hbox_new(FALSE, 0);
	linebox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER (top_window), box1);

	pattern_scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (pattern_scroller), 10);
	gtk_widget_set_size_request (pattern_scroller, 
		PSCROLLER_WIDTH, PSCROLLER_HEIGHT);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pattern_scroller),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	table = gtk_table_new(MAXINSTS,  10, FALSE);
	gtk_box_pack_start(GTK_BOX(box1), topbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box1), middle_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(middle_box), pattern_scroller, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(middle_box), linebox, FALSE, FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pattern_scroller), table);
	/* gtk_box_pack_start(GTK_BOX(box1), table, TRUE, TRUE, 0); */
	gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

	hide_instruments = gtk_check_button_new_with_label(HIDE_INSTRUMENTS_LABEL);
	g_signal_connect(G_OBJECT (hide_instruments), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, hide_instruments, HIDE_INSTRUMENTS_TIP, NULL);
	hide_volume_sliders = gtk_check_button_new_with_label(HIDE_INSTRUMENT_ATTRS_LABEL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_volume_sliders), TRUE);
	g_signal_connect(G_OBJECT (hide_volume_sliders), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, hide_volume_sliders, HIDE_VOL_SLIDERS_TIP, NULL);
	snap_to_grid = gtk_check_button_new_with_label(SNAP_TO_GRID_LABEL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(snap_to_grid), TRUE);
	gtk_tooltips_set_tip(tooltips, snap_to_grid, SNAP_TO_GRID_TIP, NULL);
	
	drumkit_vbox = gtk_vbox_new(FALSE, 0);
	save_drumkit_button = gtk_button_new_with_label(SAVE_DRUMKIT_LABEL);
	gtk_tooltips_set_tip(tooltips, save_drumkit_button, SAVE_DRUMKIT_TIP, NULL);
	g_signal_connect(G_OBJECT (save_drumkit_button), "clicked", 
			G_CALLBACK (save_drumkit_button_clicked), NULL);
	edit_instruments_toggle = gtk_toggle_button_new_with_label(EDIT_DRUMKIT_LABEL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_instruments_toggle), FALSE);
	remap_drumkit_button = gtk_button_new_with_label(REMAP_DRUMKIT_LABEL);
	gtk_tooltips_set_tip(tooltips, remap_drumkit_button, REMAP_DRUMKIT_TIP, NULL);
	g_signal_connect(G_OBJECT (remap_drumkit_button), "clicked", 
			G_CALLBACK (remap_drumkit_button_clicked), NULL);
	gtk_tooltips_set_tip(tooltips, edit_instruments_toggle,
		EDIT_DRUMKIT_TIP, NULL);
	g_signal_connect(G_OBJECT (edit_instruments_toggle), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	pattern_name_label = gtk_label_new(PATTERN_LABEL);
	gtk_label_set_justify(GTK_LABEL(pattern_name_label), GTK_JUSTIFY_RIGHT);
	pattern_name_entry = gtk_entry_new();
	widget_exclude_keypress(pattern_name_entry);
	gtk_tooltips_set_tip(tooltips, pattern_name_entry, PATTERN_NAME_TIP, NULL);
	tempolabel1 = gtk_label_new(BEATS_PER_MIN_LABEL);
	gtk_label_set_justify(GTK_LABEL(tempolabel1), GTK_JUSTIFY_RIGHT);
	tempospin1 = gtk_spin_button_new_with_range(10, 400, 1);
	widget_exclude_keypress(tempospin1);
	gtk_tooltips_set_tip(tooltips, tempospin1, PATTERN_TEMPO_TIP, NULL);
	tempolabel2 = gtk_label_new(BEATS_PER_MEASURE_LABEL);
	gtk_label_set_justify(GTK_LABEL(tempolabel2), GTK_JUSTIFY_RIGHT);
	tempospin2 = gtk_spin_button_new_with_range(1,  400, 1);
	widget_exclude_keypress(tempospin2);
	gtk_tooltips_set_tip(tooltips, tempospin2, BEATS_PER_MEASURE_TIP, NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin1), (gdouble) 120);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(tempospin2), (gdouble) 4);

	gtk_box_pack_start(GTK_BOX(topbox), checkvbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(checkvbox), hide_instruments, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(checkvbox), hide_volume_sliders, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(checkvbox), snap_to_grid, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), drumkit_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(drumkit_vbox), save_drumkit_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(drumkit_vbox), edit_instruments_toggle, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(drumkit_vbox), remap_drumkit_button, TRUE, TRUE, 0);

	misctable = gtk_table_new(2, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(topbox), misctable, TRUE, TRUE, 0);
	/* gtk_box_pack_start(GTK_BOX(topbox), pattern_name_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), pattern_name_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempolabel1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempospin1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempolabel2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), tempospin2, TRUE, TRUE, 0); */
	gtk_table_attach(GTK_TABLE(misctable), pattern_name_label, 0, 1, 0, 1, 0, 0, 1,1);
	gtk_table_attach(GTK_TABLE(misctable), pattern_name_entry, 1, 2, 0, 1, GTK_FILL, 0, 1,1);
	gtk_table_attach(GTK_TABLE(misctable), tempolabel1, 0, 1, 1, 2, 0, 0, 1,1);
	gtk_table_attach(GTK_TABLE(misctable), tempospin1, 1, 2, 1, 2, GTK_FILL, 0,1,1);
	gtk_table_attach(GTK_TABLE(misctable), tempolabel2, 0, 1, 2, 3, 0, 0, 0,0);
	gtk_table_attach(GTK_TABLE(misctable), tempospin2, 1, 2, 2, 3, GTK_FILL, 0, 1,1);

	magbox = gtk_table_new(2, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(topbox), magbox, TRUE, TRUE, 0);
	automag = gtk_check_button_new_with_label("AutoMag");
	if (AUTOMAG_ON && automag_initial_state)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(automag), TRUE);
	g_signal_connect(G_OBJECT (automag), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	autocrunch = gtk_check_button_new_with_label("AutoCrunch");
	g_signal_connect(G_OBJECT (autocrunch), "toggled", 
				G_CALLBACK (hide_instruments_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, autocrunch, "Turns on experimental UI feature...  You'll see. ;-)", NULL);
	volume_zoom_label= gtk_label_new(VOLUME_ZOOM_LABEL);
	volume_magnifier_adjustment = gtk_adjustment_new((gdouble) DEFAULT_AUTOMAG,
			100.0, 600.0, 10.0, 1.0, 0.0);
	volume_magnifier = gtk_hscale_new(GTK_ADJUSTMENT(volume_magnifier_adjustment));
	gtk_tooltips_set_tip(tooltips, volume_magnifier, VOLUME_ZOOM_TIP, NULL);
	gtk_table_attach(GTK_TABLE(magbox), volume_zoom_label, 0, 1, 0, 1, 0, 0, 1,1);
	gtk_table_attach(GTK_TABLE(magbox), volume_magnifier, 1, 2, 0, 1, GTK_FILL, 0, 1,1);
	gtk_table_attach(GTK_TABLE(magbox), automag, 1, 2, 1, 2, GTK_FILL, 0, 1,1);
	gtk_table_attach(GTK_TABLE(magbox), autocrunch, 1, 2, 2, 3, GTK_FILL, 0, 1,1);
	
	/* gtk_box_pack_start(GTK_BOX(topbox), volume_zoom_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), volume_magnifier, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), automag, TRUE, TRUE, 0); */
	g_signal_connect(G_OBJECT (volume_magnifier), "value-changed", 
				G_CALLBACK (volume_magnifier_changed), NULL);

	// for (i=0;i<dk->ninsts;i++) {
	for (i=0;i<MAXINSTS;i++) {
		int col;
		struct instrument_struct *inst = &dk->instrument[i];

		inst->hidebutton = gtk_check_button_new();
		inst->button = gtk_button_new_with_label(inst->name);
		gtk_tooltips_set_tip(tooltips, inst->button, inst->type, NULL);
		inst->drag_spin_button = gtk_spin_button_new_with_range(-DRAGLIMIT,  DRAGLIMIT, 0.1);
		widget_exclude_keypress(inst->drag_spin_button);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(inst->drag_spin_button), 
			(gdouble) 0);
		g_signal_connect(G_OBJECT (inst->drag_spin_button), "value-changed", 
				G_CALLBACK (drag_spin_button_change), inst);
		gtk_tooltips_set_tip(tooltips, inst->drag_spin_button, DRAG_RUSH_TIP, NULL);
		inst->volume_adjustment = gtk_adjustment_new((gdouble) DEFAULT_VELOCITY, 
			0.0, 127.0, 1.0, 1.0, 0.0);
		inst->volume_slider = gtk_hscale_new(GTK_ADJUSTMENT(inst->volume_adjustment));
		inst->clear_button = gtk_button_new_with_label(CLEAR_LABEL);
		gtk_tooltips_set_tip(tooltips, inst->clear_button, CLEAR_TIP, NULL);
		g_signal_connect(G_OBJECT (inst->clear_button), "clicked",
			G_CALLBACK(instrument_clear_button_pressed), (gpointer) inst);

		inst->name_entry = gtk_entry_new();
		widget_exclude_keypress(inst->name_entry);
		gtk_tooltips_set_tip(tooltips, inst->name_entry, INST_NAME_TIP, NULL);
		gtk_entry_set_text(GTK_ENTRY(inst->name_entry), inst->name);
		g_signal_connect (G_OBJECT (inst->name_entry), "activate",
		      G_CALLBACK (instrument_name_entered), (gpointer) inst);

		inst->type_entry = gtk_entry_new();
		widget_exclude_keypress(inst->type_entry);
		gtk_tooltips_set_tip(tooltips, inst->type_entry, INST_TYPE_TIP, NULL);
		gtk_entry_set_text(GTK_ENTRY(inst->type_entry), inst->type);
		g_signal_connect (G_OBJECT (inst->type_entry), "activate",
		      G_CALLBACK (instrument_type_entered), (gpointer) inst);

		inst->midi_value_spin_button = gtk_spin_button_new_with_range(0, 127, 1);
		widget_exclude_keypress(inst->midi_value_spin_button);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(inst->midi_value_spin_button), 
			(gdouble) inst->midivalue);
		g_signal_connect(G_OBJECT (inst->midi_value_spin_button), "value-changed", 
				G_CALLBACK (unsigned_char_spin_button_change), &inst->midivalue);
		gtk_tooltips_set_tip(tooltips, inst->midi_value_spin_button, 
			INST_MIDINOTE_TIP, NULL);

		inst->gm_value_spin_button = gtk_spin_button_new_with_range(0, 127, 1);
		widget_exclude_keypress(inst->gm_value_spin_button);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(inst->gm_value_spin_button), 
			(gdouble) inst->gm_equivalent);
		g_signal_connect(G_OBJECT (inst->gm_value_spin_button), "value-changed", 
				G_CALLBACK (unsigned_char_spin_button_change), &inst->gm_equivalent);
		gtk_tooltips_set_tip(tooltips, inst->gm_value_spin_button, 
			INST_GM_TIP, NULL);

		gtk_widget_set_size_request(inst->volume_slider, 80, 33);
		gtk_scale_set_digits(GTK_SCALE(inst->volume_slider), 0);
		gtk_tooltips_set_tip(tooltips, inst->volume_slider, 
			VOLUME_SLIDER_TIP, NULL);

		inst->canvas = gtk_drawing_area_new();
		if ((i % 12) == 1 || /* .#.#..#.#.#. */
			(i % 12) == 3 ||
			(i % 12) == 6 ||
			(i % 12) == 8 ||
			(i % 12) == 10)
			gtk_widget_modify_bg(inst->canvas, GTK_STATE_NORMAL, &lightgraycolor);
		else
			gtk_widget_modify_bg(inst->canvas, GTK_STATE_NORMAL, &whitecolor);

		g_signal_connect(G_OBJECT (inst->button), "clicked", 
				G_CALLBACK (instrument_button_pressed), inst);
		g_signal_connect(G_OBJECT (inst->canvas), "expose_event",
				G_CALLBACK (canvas_event), inst);
		gtk_widget_add_events(inst->canvas, GDK_ENTER_NOTIFY_MASK); 
		g_signal_connect(G_OBJECT (inst->canvas), "enter-notify-event",
				G_CALLBACK (canvas_enter), inst);
		gtk_widget_add_events(inst->canvas, GDK_BUTTON_PRESS_MASK); 
		gtk_widget_add_events(inst->canvas, GDK_BUTTON_RELEASE_MASK); 
		g_signal_connect(G_OBJECT (inst->canvas), "button-release-event",
				G_CALLBACK (canvas_clicked), inst);
		g_signal_connect(G_OBJECT (inst->canvas), "button-press-event",
				G_CALLBACK (canvas_mousedown), inst);

		/* Hmm, this doesn't seem to work... can't get key press events in canvas...? */
		widget_exclude_keypress(inst->canvas); /* filter */
		gtk_widget_add_events(inst->canvas, GDK_KEY_PRESS); /* so we have have hotkeys */
		g_signal_connect(G_OBJECT (inst->canvas), "key-press-event", 
				G_CALLBACK (canvas_key_pressed), inst);

		gtk_widget_set_size_request(inst->canvas, DRAW_WIDTH+1, DRAW_HEIGHT+1);
		// gtk_widget_set_usize(canvas, 400, 10);

		col = 0;
		gtk_table_attach(GTK_TABLE(table), inst->hidebutton, 
			col, col+1, i, i+1, 0, 0, 0,0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->drag_spin_button,
			col, col+1, i, i+1, 0, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->volume_slider,
			col, col+1, i, i+1, 0, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->clear_button,
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
		gtk_table_attach(GTK_TABLE(table), inst->gm_value_spin_button,
			col, col+1, i, i+1, GTK_FILL, 0, 0, 0); col++;
		gtk_table_attach(GTK_TABLE(table), inst->canvas, 
			col, col+1, i, i+1, 0, 0, 0, 0); col++;
	}
	NoteLabel = gtk_label_new("--");
	channel_box = gtk_hbox_new(FALSE, 0);
	channel_label = gtk_label_new(CHANNEL_LABEL);
	channelspin = gtk_spin_button_new_with_range(0, 15, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(channelspin), (gdouble) DEFAULT_MIDI_CHANNEL);
	gtk_tooltips_set_tip(tooltips, channelspin, MIDI_CHANNEL_TIP, NULL);
	g_signal_connect(G_OBJECT(channelspin), "value-changed", 
		G_CALLBACK(channel_spin_change), NULL);
	widget_exclude_keypress(channelspin);
	gtk_box_pack_start(GTK_BOX(channel_box), channel_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(channel_box), channelspin, FALSE, FALSE, 0);

	track_box = gtk_hbox_new(FALSE, 0);
	track_label = gtk_label_new(TRACK_LABEL);
	trackspin = gtk_spin_button_new_with_range(0, 15, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(trackspin), (gdouble) 0.0);
	g_signal_connect(G_OBJECT(trackspin), "value-changed", 
		G_CALLBACK(track_spin_change), NULL);
	widget_exclude_keypress(trackspin);
	gtk_box_pack_start(GTK_BOX(track_box), track_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(track_box), trackspin, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(linebox), NoteLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(linebox), channel_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(linebox), track_box, FALSE, FALSE, 0);

	percussion_toggle = gtk_check_button_new_with_label(PERCUSSION_LABEL);
	gtk_box_pack_start(GTK_BOX(linebox), percussion_toggle, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(percussion_toggle),
				(gboolean) 1);
	g_signal_connect(G_OBJECT (percussion_toggle), "toggled", 
				G_CALLBACK (percussion_toggle_callback), NULL);

	scramble_button = gtk_button_new_with_label(SCRAMBLE_LABEL);
	gtk_tooltips_set_tip(tooltips, scramble_button, SCRAMBLE_TIP, NULL);
	g_signal_connect(G_OBJECT (scramble_button), "clicked",
		G_CALLBACK(scramble_button_pressed), (gpointer) NULL);
	gtk_box_pack_start(GTK_BOX(linebox), scramble_button, FALSE, FALSE, 0);

	for (i=0;i<ndivisions;i++) {
		timediv[i].spin = gtk_spin_button_new_with_range(0, 300, 1);
		widget_exclude_keypress(timediv[i].spin);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(timediv[i].spin), (gdouble) timediv[i].division);
		g_signal_connect(G_OBJECT(timediv[i].spin), "value-changed", 
			G_CALLBACK(timediv_spin_change), &timediv[i]);
		if (i==0)
			gtk_tooltips_set_tip(tooltips, timediv[i].spin, TIMEDIV_TIP, NULL);
		else
			gtk_tooltips_set_tip(tooltips, timediv[i].spin, TIMEDIV2_TIP, NULL);

		/* gtk_table_attach_defaults(GTK_TABLE(table), timediv[i].spin, 
			2, 3, i, i+1); */
		gtk_box_pack_start(GTK_BOX(linebox), timediv[i].spin, FALSE, FALSE, 0);
	}
	remove_space_before_button = gtk_button_new_with_label(REMOVE_SPACE_BEFORE_LABEL);
	gtk_box_pack_start(GTK_BOX(linebox), remove_space_before_button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, remove_space_before_button,
		REMOVE_SPACE_BEFORE_TIP, NULL);
	g_signal_connect(G_OBJECT (remove_space_before_button), "clicked",
		G_CALLBACK(remove_space_before_button_pressed), (gpointer) NULL);
	add_space_before_button = gtk_button_new_with_label(ADD_SPACE_BEFORE_LABEL);
	gtk_box_pack_start(GTK_BOX(linebox), add_space_before_button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, add_space_before_button, ADD_SPACE_BEFORE_TIP, NULL);
	add_space_numerator_spin = gtk_spin_button_new_with_range(1, 400, 1);
	gtk_tooltips_set_tip(tooltips, add_space_numerator_spin, NUMERATOR_TIP, NULL);
	g_signal_connect(G_OBJECT (add_space_before_button), "clicked",
		G_CALLBACK(add_space_before_button_pressed), (gpointer) NULL);
	widget_exclude_keypress(add_space_numerator_spin);
	gtk_box_pack_start(GTK_BOX(linebox), add_space_numerator_spin, FALSE, FALSE, 0);
	add_space_denominator_spin = gtk_spin_button_new_with_range(1, 400, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(add_space_denominator_spin), (gdouble) 2.0);
	gtk_tooltips_set_tip(tooltips, add_space_denominator_spin, DENOMINATOR_TIP, NULL);
	widget_exclude_keypress(add_space_denominator_spin);
	gtk_box_pack_start(GTK_BOX(linebox), add_space_denominator_spin, FALSE, FALSE, 0);
	add_space_after_button = gtk_button_new_with_label(ADD_SPACE_AFTER_LABEL);
	g_signal_connect(G_OBJECT (add_space_after_button), "clicked",
		G_CALLBACK(add_space_after_button_pressed), (gpointer) NULL);
	gtk_tooltips_set_tip(tooltips, add_space_after_button,
		ADD_SPACE_AFTER_TIP, NULL);
	gtk_box_pack_start(GTK_BOX(linebox), add_space_after_button, FALSE, FALSE, 0);
	remove_space_after_button = gtk_button_new_with_label(REMOVE_SPACE_AFTER_LABEL);
	g_signal_connect(G_OBJECT (remove_space_after_button), "clicked",
		G_CALLBACK(remove_space_after_button_pressed), (gpointer) NULL);
	gtk_tooltips_set_tip(tooltips, remove_space_after_button,
		REMOVE_SPACE_AFTER_TIP, NULL);
	gtk_box_pack_start(GTK_BOX(linebox), remove_space_after_button, FALSE, FALSE, 0);


	pattern_metronome_chbox = gtk_check_button_new_with_label(METRONOME_LABEL);
	pattern_loop_chbox = gtk_check_button_new_with_label(LOOP_LABEL);
	prevbutton = gtk_button_new_with_label(EDIT_PREV_PATTERN_LABEL);
	nextbutton = gtk_button_new_with_label(CREATE_NEXT_PATTERN_LABEL);
	pattern_clear_button = gtk_button_new_with_label(CLEAR_PATTERN_LABEL);
	pattern_select_button = gtk_button_new_with_label(SELECT_PATTERN_LABEL);
	pattern_paste_button = gtk_button_new_with_label(PASTE_PATTERN_LABEL);
	pattern_record_button = gtk_toggle_button_new_with_label(RECORD_LABEL);
	pattern_play_button = gtk_button_new_with_label(PLAY_LABEL);
	pattern_stop_button = gtk_button_new_with_label(STOP_LABEL);

	g_signal_connect(G_OBJECT (nextbutton), "clicked",
			G_CALLBACK (nextbutton_clicked), NULL);
	g_signal_connect(G_OBJECT (prevbutton), "clicked",
			G_CALLBACK (prevbutton_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_clear_button), "clicked",
			G_CALLBACK (pattern_clear_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_select_button), "clicked",
			G_CALLBACK (select_pattern_button_pressed), NULL);
	g_signal_connect(G_OBJECT (pattern_paste_button), "clicked",
			G_CALLBACK (pattern_paste_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_record_button), "clicked",
			G_CALLBACK (pattern_record_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_play_button), "clicked",
			G_CALLBACK (pattern_play_button_clicked), NULL);
	g_signal_connect(G_OBJECT (pattern_stop_button), "clicked",
			G_CALLBACK (pattern_stop_button_clicked), NULL);

	gtk_tooltips_set_tip(tooltips, pattern_metronome_chbox, PATTERN_METRONOME_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_loop_chbox, PATTERN_LOOP_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, nextbutton, CREATE_NEXT_PATTERN_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, prevbutton, EDIT_PREV_PATTERN_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_clear_button, CLEAR_PATTERN_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_select_button, SELECT_GEN_PATTERN_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_paste_button, PASTE_PATTERN_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_record_button, RECORD_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_play_button, PATTERN_PLAY_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, pattern_stop_button, PATTERN_STOP_TIP, NULL);

	gtk_box_pack_start(GTK_BOX(box2), pattern_metronome_chbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_loop_chbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), prevbutton, TRUE, TRUE, 0);
	if (ifd >= 0)
		gtk_box_pack_start(GTK_BOX(box2), pattern_record_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_play_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_stop_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_clear_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_select_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), pattern_paste_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), nextbutton, TRUE, TRUE, 0);

	

	/* ---------------- arranger window ------------------ */
	g_signal_connect(G_OBJECT (arranger_window), "key_press_event",
			G_CALLBACK (key_press_cb), "arranger");
	g_signal_connect(G_OBJECT (arranger_window), "selection_received",
			G_CALLBACK (drumtab_selection_received), NULL);
	gtk_container_set_border_width(GTK_CONTAINER (arranger_window), 15);

	/* 1 row, 2 colums, 1 row per pattern, will resize table as necc.
	   First column is the pattern name, 2nd column is a drawing
	   area that allows the measures to be specified. */

	arranger_scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (arranger_scroller, 750, ARRANGER_HEIGHT);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (arranger_scroller),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	arranger_table = gtk_table_new(6, ARRANGER_COLS, FALSE);

	TempoLabel = gtk_label_new(TEMPO_CHANGES_LABEL);
	SelectButton = gtk_button_new_with_label(SELECT_MEASURES_LABEL);
	PasteLabel = gtk_label_new(PASTE_MEASURES_LABEL);
	InsertButton = gtk_button_new_with_label(INSERT_MEASURES_LABEL);
	DeleteButton = gtk_button_new_with_label(DELETE_MEASURES_LABEL);
	MeasureTransportLabel = gtk_label_new(TRANSPORT_LOC_LABEL);
	
	gtk_tooltips_set_tip(tooltips, SelectButton, SELECT_MEASURES_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, InsertButton, INSERT_MEASURES_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, DeleteButton, DELETE_MEASURES_TIP, NULL);

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
	gtk_table_attach(GTK_TABLE(arranger_table), MeasureTransportLabel, 3, 4, 5, 6, 0, 0, 0, 0);

	/* gtk_widget_show(TempoLabel);
	gtk_widget_show(SelectButton);
	gtk_widget_show(PasteLabel);
	gtk_widget_show(InsertButton);
	gtk_widget_show(DeleteButton); */

	Tempo_da = gtk_drawing_area_new();
	Copy_da = gtk_drawing_area_new();
	Paste_da = gtk_drawing_area_new();
	Insert_da = gtk_drawing_area_new();
	Delete_da = gtk_drawing_area_new();
	measure_transport_da = gtk_drawing_area_new();

	gtk_widget_modify_bg(measure_transport_da, GTK_STATE_NORMAL, &whitecolor);
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
	g_signal_connect(G_OBJECT (measure_transport_da), "expose_event", 
		G_CALLBACK (measure_transport_expose), NULL);

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
	gtk_table_attach(GTK_TABLE(arranger_table), measure_transport_da, 4, 5, 5, 6, 0, 0, 0, 0);

/* I don't think these tool tips do anything... tooltips don't show for a drawing area...
	gtk_tooltips_set_tip(tooltips, Tempo_da, TEMPO_CHANGES_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, Copy_da, COPY_MEASURE_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, Paste_da, PASTE_MEASURES_TIP, NULL);
	gtk_tooltips_set_tip(tooltips, Insert_da, 
		"Click the buttons to the right to insert a new blank measure", NULL);
	gtk_tooltips_set_tip(tooltips, Delete_da, 
		"Click the buttons to the right to delete a measure", NULL);
*/

	gtk_widget_set_size_request(Tempo_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Copy_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Paste_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Insert_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(Delete_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);
	gtk_widget_set_size_request(measure_transport_da, ARRANGER_WIDTH, ARRANGER_HEIGHT);

	/* gtk_widget_show(Tempo_da);
	gtk_widget_show(Copy_da);
	gtk_widget_show(Paste_da);
	gtk_widget_show(Insert_da);
	gtk_widget_show(Delete_da);
	gtk_widget_show(measure_transport_da); */

	song_name_label = gtk_label_new(SONG_LABEL);
	song_name_entry = gtk_entry_new();
	widget_exclude_keypress(song_name_entry);
	g_signal_connect (G_OBJECT (song_name_entry), "activate",
		      G_CALLBACK (song_name_entered), (gpointer) song_name_entry);
	arr_loop_check_button = gtk_check_button_new_with_label(LOOP_LABEL);
	gtk_tooltips_set_tip(tooltips, arr_loop_check_button, SONG_LOOP_TIP, NULL);
	arr_factor_check_button = gtk_check_button_new_with_label(FACTOR_DRUM_TAB_LABEL);
	gtk_tooltips_set_tip(tooltips, arr_factor_check_button,
		FACTOR_DRUM_TAB_TIP, NULL);
	midi_setup_activate_button = gtk_button_new_with_label(MIDI_SETUP_LABEL);
	gtk_tooltips_set_tip(tooltips, midi_setup_activate_button,
		MIDI_SETUP_TIP, NULL);
	g_signal_connect(G_OBJECT (midi_setup_activate_button), "clicked",
			G_CALLBACK (midi_setup_activate), NULL);
#if 0
	about_button = gtk_button_new_with_label("About Gneutronica");
	g_signal_connect(G_OBJECT (about_button), "clicked",
			G_CALLBACK (about_activate), NULL);
#endif

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM); /* Tabs at the bottom */

	abox = gtk_vbox_new(FALSE, 0);
	menu_box = gtk_vbox_new(FALSE, 0);
	a_button_box = gtk_hbox_new(FALSE, 0);
	arranger_box = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER (arranger_window), arranger_top_box);
	arranger_label = gtk_label_new("Arranger");
	pattern_editor_label = gtk_label_new("Pattern Editor");

	/* Track control stuff */
	track_control_box = gtk_vbox_new(FALSE, 0);
	track_control_label = gtk_label_new("Track/Channel Mute");
	track_table = gtk_table_new(MAXCHANS+2, MAXTRACKS+2, FALSE);
	gtk_container_add(GTK_CONTAINER (track_control_box), track_table);
	channel_mute_label = gtk_label_new("Channel:");
	gtk_table_attach(GTK_TABLE(track_table), channel_mute_label, 0, 1, 0, 1, 0, 0, 0, 0);
	/* Set up channel labels across the top */
	for (i=0;i<MAXCHANS;i++) {
		char chan_text[15];
		sprintf(chan_text, " %d ", i);
		chan_label[i] = gtk_label_new(chan_text);
		gtk_table_attach(GTK_TABLE(track_table), chan_label[i], i+1, i+2, 0, 1, 0, 0, 0, 0);
	}
	/* Set up track labels down the side */
	for (i=0;i<MAXTRACKS;i++) {
		char track_text[15];
		sprintf(track_text, "Track %d", i);
		track_num_label[i] = gtk_label_new(track_text);
		gtk_table_attach(GTK_TABLE(track_table), track_num_label[i], 0, 1, i+1, i+2, 0, 0, 0, 0);
	}
	/* Set up the chan/track mute checkboxes */
	for (i=0;i<MAXTRACKS;i++) {
		for (j=0;j<MAXCHANS;j++) {
			track_mute[i][j] = gtk_toggle_button_new();
			gtk_table_attach(GTK_TABLE(track_table), track_mute[i][j],
				i+1, i+2, j+1, j+2, 0,0,0,0);
			g_signal_connect(G_OBJECT (track_mute[i][j]), "toggled",
				G_CALLBACK (track_mute_toggle_callback),
					&transport_location->muted[i][j]);
		}
	}

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), top_window, pattern_editor_label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), abox, arranger_label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), track_control_box, track_control_label);

	/* Menu code taken from gtk tutorial . . .  */

	/* Get the three types of menu.  Note: all three menus are */
	/* separately created, so they are not the same menu */
	main_menubar = get_menubar_menu(arranger_window);
	main_popup_button = get_popup_menu();
	main_option_menu = get_option_menu();

	/* Pack it all together */
	gtk_box_pack_start(GTK_BOX (menu_box), main_menubar, FALSE, TRUE, 0);
	/* gtk_box_pack_end(GTK_BOX (menu_box), main_popup_button, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX (menu_box), main_option_menu, FALSE, TRUE, 0); */

	/* . . . End menu code taken from gtk tutorial. */

	gtk_box_pack_start(GTK_BOX(arranger_top_box), menu_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_top_box), notebook, TRUE, TRUE, 0);
	/* gtk_box_pack_start(GTK_BOX(arranger_top_box), abox, FALSE, FALSE, 0); */

	/* gtk_box_pack_start(GTK_BOX(abox), menu_box, FALSE, FALSE, 0); */
	gtk_box_pack_start(GTK_BOX(abox), arranger_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), song_name_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), song_name_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), arr_loop_check_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), arr_factor_check_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(arranger_box), midi_setup_activate_button, FALSE, FALSE, 0);
#if 0
	gtk_box_pack_start(GTK_BOX(arranger_box), about_button, FALSE, FALSE, 0);
#endif
	gtk_box_pack_start(GTK_BOX(abox), arranger_scroller, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(arranger_scroller), 
		arranger_table);
	gtk_box_pack_start(GTK_BOX(abox), a_button_box, FALSE, FALSE, 0);
	play_button = gtk_button_new_with_label(PLAY_LABEL);
	gtk_tooltips_set_tip(tooltips, play_button, PLAY_SONG_TIP, NULL);
	play_selection_button = gtk_button_new_with_label(PLAY_SELECTION_LABEL);
	gtk_tooltips_set_tip(tooltips, play_selection_button, PLAY_SELECTION_TIP, NULL);
	stop_button = gtk_button_new_with_label(STOP_LABEL);
	gtk_tooltips_set_tip(tooltips, stop_button, PATTERN_STOP_TIP, NULL);
#if 0
	save_button = gtk_button_new_with_label("Save");
	gtk_tooltips_set_tip(tooltips, save_button, "Save this song to a file", NULL);
	load_button = gtk_button_new_with_label("Load");
	gtk_tooltips_set_tip(tooltips, load_button, 
		"Discard the current song and load another one from a file.", NULL);
#endif
	gtk_box_pack_start(GTK_BOX(a_button_box), play_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (play_button), "clicked",
			G_CALLBACK (arranger_play_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), play_selection_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (play_selection_button), "clicked",
			G_CALLBACK (arranger_play_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), stop_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (stop_button), "clicked",
			G_CALLBACK (pattern_stop_button_clicked), NULL);
#if 0
	gtk_box_pack_start(GTK_BOX(a_button_box), save_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (save_button), "clicked", 
			G_CALLBACK (save_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(a_button_box), load_button, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT (load_button), "clicked", 
			G_CALLBACK (load_button_clicked), NULL);
#endif
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
	if (maximize_windows) {
		/* gtk_window_maximize((GtkWindow *) top_window); */
		gtk_window_maximize((GtkWindow *) arranger_window);
	}
	gtk_widget_show_all(arranger_window);
	/* gtk_widget_show_all(top_window); */
	/* gc = gdk_gc_new(dk->instrument[0].canvas->window); */
	gc = gdk_gc_new(GTK_WIDGET(arranger_window)->window);

	for (i=0;i<drumkit[kit].ninsts;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		gtk_widget_hide(inst->name_entry);
		gtk_widget_hide(inst->type_entry);
		gtk_widget_hide(inst->midi_value_spin_button);
		gtk_widget_hide(inst->gm_value_spin_button);
		gtk_widget_hide(inst->volume_slider);
		gtk_widget_hide(inst->clear_button);
		gtk_widget_hide(inst->drag_spin_button);
	}
	for (i=drumkit[kit].ninsts;i<MAXINSTS;i++) {
		struct instrument_struct *inst = &drumkit[kit].instrument[i];
		gtk_widget_hide(inst->name_entry);
		gtk_widget_hide(inst->type_entry);
		gtk_widget_hide(inst->midi_value_spin_button);
		gtk_widget_hide(inst->gm_value_spin_button);
		gtk_widget_hide(inst->volume_slider);
		gtk_widget_hide(inst->clear_button);
		gtk_widget_hide(inst->drag_spin_button);
		gtk_widget_hide(inst->drag_spin_button);
		gtk_widget_hide(inst->hidebutton);
		gtk_widget_hide(inst->canvas);
		gtk_widget_hide(inst->button);
	}

	flatten_pattern(kit, cpattern);

	gtk_main();

	if (ifd > 0)
		close(ifd);

	exit(0);
}
