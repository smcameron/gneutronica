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
#ifndef __LANG_H__
#define __LANG_H__

/* English is the native language for gneutronica, others are translations. */

#define EDIT_PATTERN_TIP "Edit this pattern.  Click the boxes to the right to assign this pattern to measures."
#define SELECT_PATTERN_TIP "Select this pattern (%s) for later pasting into another pattern"
#define INSERT_PATTERN_TIP "Insert new pattern before this pattern."
#define DELETE_PATTERN_TIP "Delete this pattern."
#define ASSIGN_P_TO_M_TIP "Click to assign patterns to measures."

#define TEMPO_CHANGE_MESSAGE "Measure:%d Beats/Minute"

#define SAVE_SONG_ITEM "Save Song"
#define LOAD_SONG_ITEM "Load Song"
#define IMPORT_PATTERNS_ITEM "Import Patterns from Song"
#define IMPORT_DRUM_TAB_ITEM "Import Drum Tablature"
#define EXPORT_SONG_TO_MIDI_ITEM "Export Song to MIDI file"

#define ARRANGER_TITLE_ITEM "%s v. %s - Arrangement Editor: %s"

#define CREATE_NEXT_PATTERN_LABEL "Create Next Pattern -->"
#define CREATE_NEXT_PATTERN_TIP "Create and edit the next pattern"

#define EDIT_NEXT_PATTERN_LABEL "Edit Next Pattern -->"
#define EDIT_NEXT_PATTERN_TIP "Edit the next pattern"
#define EDIT_PREV_PATTERN_LABEL "<-- Edit Previous Pattern"
#define CLEAR_PATTERN_LABEL "Clear Pattern"
#define SELECT_PATTERN_LABEL "Select Pattern"
#define PASTE_PATTERN_LABEL "Paste Pattern"
#define RECORD_LABEL "Record"
#define PLAY_LABEL "Play"
#define STOP_LABEL "Stop"

#define EDIT_PREV_PATTERN_TIP "Edit the previous pattern"
#define CLEAR_PATTERN_TIP "Clear all notes from this pattern"
#define SELECT_GEN_PATTERN_TIP "Select this pattern for later pasting."
#define PASTE_PATTERN_TIP "Superimpose all the notes of a previously " \
	"selected pattern onto this pattern."
#define RECORD_TIP "Record from MIDI input device"
#define PATTERN_PLAY_TIP "Send this pattern to MIDI device for playback"
#define PATTERN_STOP_TIP "Stop any playback currently in progress."

#define PASTE_MSG_TIP "Superimpose all the notes of the selected pattern (%s) onto this pattern."
#define HIDE_INSTRUMENTS_LABEL "Hide unchecked instruments"
#define HIDE_INSTRUMENTS_TIP "Hide the instruments below which do not have a check " \
			"beside them to reduce visual clutter."

#define HIDE_INSTRUMENT_ATTRS_LABEL "Hide instrument attributes"
#define HIDE_VOL_SLIDERS_TIP "Hide the volume sliders and drag settings which appear " \
		"to the left of the instrument buttons, below."

#define SNAP_TO_GRID_LABEL "Snap to grid"
#define SNAP_TO_GRID_TIP "Force newly placed notes to line up with the timing lines."

#define SAVE_DRUMKIT_LABEL "Save Drum Kit"
#define SAVE_DRUMKIT_TIP "Save instrument names, types, and MIDI note " \
	"assignments into a file for later re-use.  " \
	"Please consider sending your new drumkit file " \
	"to smcameron@users.sourceforge.net for inclusion with " \
	"future releases of Gneutronica.  Please include " \
	"the make and model of the MIDI device, and the name " \
	"of whatever preset you're using, if applicable."

#define EDIT_DRUMKIT_LABEL "Edit drum kit"
#define EDIT_DRUMKIT_TIP "Assign names, types, and MIDI note numbers to instruments."
#define REMAP_DRUMKIT_LABEL "Remap drum kit"
#define REMAP_DRUMKIT_TIP "Remap drum kit instruments for this pattern " \
	"via General MIDI to the current drumkit (sorta kinda works.)"

#define PATTERN_LABEL "Pattern:"
#define PATTERN_NAME_TIP "Assign a name to this pattern."
#define BEATS_PER_MIN_LABEL "Beats/min"
#define PATTERN_TEMPO_TIP "Controls tempo only for single pattern playback, " \
	"does not affect the tempo in the context of the song."
#define BEATS_PER_MEASURE_LABEL "Beats/Measure"
#define BEATS_PER_MEASURE_TIP "Controls tempo for single pattern playback, and DOES " \
	"affect the tempo in the context of the song.Also affects the instrument " \
	"drag/rush control."

#define VOLUME_ZOOM_LABEL "Volume Zoom"
#define VOLUME_ZOOM_TIP "Controls how much the volume scale is magnified " \
	"for the current instrument from no magnification to 6x.  It allows " \
	"note volumes to be more precisely specified."
#define DRAG_RUSH_TIP "Set percentage of a beat to drag (or rush) this " \
	"instrument. Use negative numbers for rushing."
#define INST_NAME_TIP "Assign a name to this instrument."
#define INST_TYPE_TIP "Assign a type to this instrument."
#define INST_MIDINOTE_TIP "Assign the MIDI note for this instrument"
#define INST_GM_TIP "Assign the closest General MIDI note for this instrument for remapping"

#define CLEAR_LABEL "Clear"
#define CLEAR_TIP "Delete all the notes for this instrument in this pattern."
#define VOLUME_SLIDER_TIP "Controls the default volume for this insrument."
#define SCRAMBLE_LABEL "Scramble"
#define SCRAMBLE_TIP "Scramble this measure by divisions randomly. " \
	"Is this a useful feature?  A million monkeys can't be wrong " \
	"all the time."

#define TIMEDIV_TIP "Use these to control the way the measure is divided " \
	"up for placement of beats. This first one is also used by the " \
	"Scramble function."
#define TIMEDIV2_TIP "Use these to control the way the measure is " \
		"divided up for placement of beats"

#define REMOVE_SPACE_BEFORE_LABEL "Remove Space Before"
#define REMOVE_SPACE_BEFORE_TIP "Removes space from the beginning of the " \
	"measure.  Set the numerator and denominator to the fraction of the " \
	"measure to be deleted as measured prior to deleting."

#define TRACK_LABEL "Track:"

#define ADD_SPACE_BEFORE_LABEL "Add Space Before"
#define ADD_SPACE_BEFORE_TIP "Adds space to the beginning of the measure, " \
	" squeezing all the notes to the right.  Set the numerator and " \
	"denominator to indicate the fraction of the total measure the new " \
	"space should occupy as measured after the operation of adding space " \
	"is complete.  For instance if the measure is 2 units long and you " \
	"want to be 3 units, use 1/3.  In general, if it's X units and you " \
	"want (X + Y) units, use (Y - X) / (X + Y)."

#define NUMERATOR_TIP "Numerator.  Use this when adding/removing space to this measure."
#define DENOMINATOR_TIP "Denominator.  Use this when adding/removing space to this measure."

#define ADD_SPACE_AFTER_LABEL "Add Space After"
#define ADD_SPACE_AFTER_TIP "Adds space to the end of the measure, " \
	"squeezing all the notes to the left.  Set the numerator and " \
	"denominator to indicate the fraction of the total measure the new " \
	"space should occupy as measured after the operation of adding space " \
	"is complete.  For instance if the measure is 2 units long and you " \
	"want to be 3 units, use 1/3.  In general, if it's X units and you " \
	"want (X + Y) units, use (Y - X) / (X + Y)."
#define REMOVE_SPACE_AFTER_LABEL "Remove Space After"
#define REMOVE_SPACE_AFTER_TIP "Removes space from the end of the " \
	"measure.Set the numerator and denominator to the fraction of " \
	"the measure to be deleted as measured prior to deleting."

#define LOOP_LABEL "Loop"
#define PATTERN_LOOP_TIP "When checked, will cause playback to loop until 'Stop' is pressed."

#define TEMPO_CHANGES_LABEL "Tempo changes"
#define SELECT_MEASURES_LABEL "Select Measures"
#define PASTE_MEASURES_LABEL "Paste Measures"
#define INSERT_MEASURES_LABEL "Insert Measures"
#define DELETE_MEASURES_LABEL "Delete Measures"
#define TRANSPORT_LOC_LABEL "Transport Location"

#define SELECT_MEASURES_TIP "Click this button to select all measures, " \
	"or twice to select no measures.  Click the buttons to the right " \
	"to select a single measure.  Click, drag, and release over the " \
	"buttons to the right to select a range of measures."

#define INSERT_MEASURES_TIP "Click the buttons to the right to insert " \
	"a single measure, or use this button to insert blank measures for " \
	"the selected measures."

#define DELETE_MEASURES_TIP "Click the buttons to the right to delete " \
	" a single measure, or use this button to delete the selected measures."


#define TEMPO_CHANGES_TIP "Click the buttons to the right to insert tempo changes"
#define COPY_MEASURE_TIP "Click the buttons to the right to copy a measure"


#define PASTE_MEASURES_TIP "Click the buttons to the right to " \
	"insert a previously copied measure"

#define SONG_LOOP_TIP "When checked, will cause playback to loop " \
	"until 'Stop' is pressed"

#define FACTOR_DRUM_TAB_LABEL "Factor drum tablature"
#define FACTOR_DRUM_TAB_TIP "When checked, pasted ASCII drum tabs " \
	"will be factored to reduce duplicate patterns on a per-instrument " \
	"basis, rather than a per-measure basis."

#define MIDI_SETUP_LABEL "MIDI Setup"
#define MIDI_SETUP_TIP "Set the MIDI channel to transmit on, and " \
	"send  MIDI patch change messages."

#define PLAY_SONG_TIP "Send this song to MIDI device for playback"
#define PLAY_SELECTION_LABEL "Play Selection"
#define PLAY_SELECTION_TIP "Send selected measures to MIDI device for playback"

#define UNTITLED_SONG_LABEL "Untitled Song"
#define SONG_LABEL "Song"

#define FILE_LABEL "File"
#define NEW_LABEL "New"
#define OPEN_LABEL "Open"
#define SAVE_LABEL "Save"
#define SAVE_AS_LABEL "Save _As"
#define IMPORT_PATTERNS_LABEL "_Import Patterns"
#define IMPORT_DRUM_TAB_LABEL "Drum _Tablature"
#define EXPORT_TO_MIDI_FILE_LABEL "_Export Song to MIDI file"
#define QUIT_LABEL "Quit"
#define EDIT_LABEL "Edit"
#define HELP_LABEL "Help"
#define ABOUT_LABEL "About"
#define PASTE_DRUM_TAB_LABEL "Paste ASCII drum tablature"
#define REMAP_DRUM_KIT_MENU_LABEL "_Remap drum kit for whole song via GM"

#define METRONOME_LABEL "Metronome"
#define PATTERN_METRONOME_TIP "Provide metronome sound when recording."


#ifdef GNEUTRONICA_FRENCH
/*
 *
 * FRENCH
 *
 *
 */

/* These French translations were done by machine, */
/* so they are horrible.  Feel free to improve them. */

#warning
#warning These French translations are terrible.
#warning Please feel free to submit a patch with
#warning better translations to smcameron@sourceforge.net
#warning (see lang.h)
#warning

#undef EDIT_PATTERN_TIP
#define EDIT_PATTERN_TIP "Éditer ce modèle.  Cliquer les boîtes vers la droite d'assigner ce modèle aux mesures."

#undef SELECT_PATTERN_TIP
#define SELECT_PATTERN_TIP "Choisir ce modèle (%s) pour coller plus tard dans un autre modèle"

#undef INSERT_PATTERN_TIP
#define INSERT_PATTERN_TIP "Insérer le nouveau modèle avant ce modèle."

#undef DELETE_PATTERN_TIP
#define DELETE_PATTERN_TIP "Supprimer ce modèle."

#undef ASSIGN_P_TO_M_TIP
#define ASSIGN_P_TO_M_TIP "Cliquer pour assigner des modèles aux mesures."

#undef TEMPO_CHANGE_MESSAGE
#define TEMPO_CHANGE_MESSAGE "Mesure:%d battements/minute"

#undef SAVE_SONG_ITEM
#define SAVE_SONG_ITEM "Stocker la chanson"
#undef LOAD_SONG_ITEM
#define LOAD_SONG_ITEM "Chanson de charge"
#undef IMPORT_PATTERNS_ITEM
#define IMPORT_PATTERNS_ITEM "Modèles d'importation de chanson"
#undef IMPORT_DRUM_TAB_ITEM
#define IMPORT_DRUM_TAB_ITEM "tambour Tablature chanson d'importation"
#undef EXPORT_SONG_TO_MIDI_ITEM
#define EXPORT_SONG_TO_MIDI_ITEM "d'exportation au dossier du MIDI"

#undef ARRANGER_TITLE_ITEM
#define ARRANGER_TITLE_ITEM "%s v. %s - Rédacteur d'arrangement : %s"

#undef CREATE_NEXT_PATTERN_LABEL
#define CREATE_NEXT_PATTERN_LABEL "Créer Prochain Modèle -->"
#undef CREATE_NEXT_PATTERN_TIP
#define CREATE_NEXT_PATTERN_TIP "Créer et éditer le prochain modèle"

#undef EDIT_NEXT_PATTERN_LABEL
#define EDIT_NEXT_PATTERN_LABEL "Éditer Prochain Modèle "
#undef EDIT_NEXT_PATTERN_TIP
#define EDIT_NEXT_PATTERN_TIP "Éditer le prochain modèle "
#undef EDIT_PREV_PATTERN_LABEL
#define EDIT_PREV_PATTERN_LABEL "<-- Éditer le modèle précédent"
#undef CLEAR_PATTERN_LABEL
#define CLEAR_PATTERN_LABEL "Modèle Clair"
#undef SELECT_PATTERN_LABEL
#define SELECT_PATTERN_LABEL "Choisir modèle"
#undef PASTE_PATTERN_LABEL
#define PASTE_PATTERN_LABEL "Coller modèle"
#undef RECORD_LABEL
#define RECORD_LABEL "Enregistrer"
#undef PLAY_LABEL
#define PLAY_LABEL "Jouer"
#undef STOP_LABEL
#define STOP_LABEL "Arrêt"

#undef EDIT_PREV_PATTERN_TIP
#define EDIT_PREV_PATTERN_TIP "Éditer le modèle précédent"
#undef CLEAR_PATTERN_TIP
#define CLEAR_PATTERN_TIP "Dégager toutes les notes de ce modèle"
#undef SELECT_GEN_PATTERN_TIP
#define SELECT_GEN_PATTERN_TIP "Choisir ce modèle pour coller plus tard."
#undef PASTE_PATTERN_TIP
#define PASTE_PATTERN_TIP "Superposer toutes notes d'un modèle " \
	"précédemment choisi sur ce modèle. "
#undef RECORD_TIP
#define RECORD_TIP "Enregistrer du dispositif d'entrée du MIDI"
#undef PATTERN_PLAY_TIP
#define PATTERN_PLAY_TIP "Envoyer ce modèle au dispositif du MIDI pour le playback"
#undef PATTERN_STOP_TIP
#define PATTERN_STOP_TIP "Arrêter n'importe quel playback actuellement en marche."

#undef PASTE_MSG_TIP
#define PASTE_MSG_TIP "Superposer toutes notes du modèle choisi (%s) sur ce modèle. "

#undef HIDE_INSTRUMENTS_LABEL
#define HIDE_INSTRUMENTS_LABEL "Instruments non réprimés de peau "
#undef HIDE_INSTRUMENTS_TIP
#define HIDE_INSTRUMENTS_TIP "Cacher les instruments au-dessous dont ne pas avoir " \
	"un contrôle près de eux pour réduire l'image de fond visuelle."
#undef HIDE_INSTRUMENT_ATTRS_LABEL
#define HIDE_INSTRUMENT_ATTRS_LABEL "Attributs d'instrument de peau"

#undef HIDE_VOL_SLIDERS_TIP
#define HIDE_VOL_SLIDERS_TIP "Cacher les glisseurs de volume et traîner les arrangements " \
		"qui apparaissent à la gauche des boutons d'instrument, ci-dessous."

#undef SNAP_TO_GRID_LABEL
#define SNAP_TO_GRID_LABEL "Adhérer à la grille"

#undef SNAP_TO_GRID_TIP
#define SNAP_TO_GRID_TIP "Forcer les notes nouvellement placées pour aligner avec " \
	"les lignes de synchronisation. "

#undef SAVE_DRUMKIT_LABEL
#define SAVE_DRUMKIT_LABEL "Stocker le kit de tambour "

#undef SAVE_DRUMKIT_TIP
#define SAVE_DRUMKIT_TIP "Stocker des noms d'instrument, des types, et la note du MIDI " \
	"tâches dans un dossier pour la réutilisation postérieure.  Considérer svp " \
	"envoyer votre nouveau dossier de drumkit  à smcameron@users.sourceforge.net pour " \
	"l'inclusion avec de futurs dégagements de Gneutronica.  Veuillez inclure la " \
	"marque et le modèle du dispositif du MIDI, et le nom de celui qui vous prérèglent " \
	"emploient, si c'est approprié."

#undef EDIT_DRUMKIT_LABEL
#define EDIT_DRUMKIT_LABEL "éditer le kit de tambour"
#undef EDIT_DRUMKIT_TIP
#define EDIT_DRUMKIT_TIP "Des nombres assigner les noms, les types, et du MIDI note aux instruments."
#undef REMAP_DRUMKIT_LABEL
#define REMAP_DRUMKIT_LABEL "remap le kit de tambour"

#undef REMAP_DRUMKIT_TIP
#define REMAP_DRUMKIT_TIP "Remap les instruments de kit de tambour pour ce " \
	"modèle par l'intermédiaire du MIDI général au drumkit courant "

#undef PATTERN_LABEL
#define PATTERN_LABEL "Modele:"

#undef PATTERN_NAME_TIP
#define PATTERN_NAME_TIP "Assigner un nom à ce modèle."
#undef BEATS_PER_MIN_LABEL
#define BEATS_PER_MIN_LABEL "Battements/min"

#undef PATTERN_TEMPO_TIP
#define PATTERN_TEMPO_TIP "Commande le tempo seulement pour le playback " \
	"simple de modèle, n'affecte pas le tempo dans le contexte de la chanson. "

#undef BEATS_PER_MEASURE_LABEL
#define BEATS_PER_MEASURE_LABEL "Battements/mesure"

#undef BEATS_PER_MEASURE_TIP
#define BEATS_PER_MEASURE_TIP "Commande le tempo pour le playback simple de modèle, " \
	" et affecte le tempo dans le contexte de la chanson. Affecte en outre la " \
	"drague d'instrument/commande de précipitations."

#undef VOLUME_ZOOM_LABEL
#define VOLUME_ZOOM_LABEL "Bourdonnement de Volume"
#undef VOLUME_ZOOM_TIP
#define VOLUME_ZOOM_TIP "Commande combien la balance de volume est magnifiée " \
	"pour l'instrument courant d'aucun rapport optique à 6x.  Elle permet " \
	"à des volumes de note d'être indiqués plus avec précision. "

#undef DRAG_RUSH_TIP
#define DRAG_RUSH_TIP "Placer le pourcentage d'un battement à la drague " \
	"(ou aux précipitations) cet instrument. Employer les nombres négatifs " \
	"pour la précipitation. "

#undef CLEAR_LABEL
#define CLEAR_LABEL "Dégager"
#undef CLEAR_TIP
#define CLEAR_TIP "Supprimer toutes notes pour cet instrument dans ce modèle."

#undef INST_NAME_TIP
#define INST_NAME_TIP "Assigner un nom à cet instrument."
#undef INST_TYPE_TIP
#define INST_TYPE_TIP "Assigner un type à cet instrument."
#undef INST_MIDINOTE_TIP
#define INST_MIDINOTE_TIP "Assigner la note du MIDI pour cet instrument."
#undef INST_GM_TIP
#define INST_GM_TIP "Assigner la note de MIDI générale la plus étroite " \
	"pour cet instrument pour remapping."

#undef VOLUME_SLIDER_TIP
#define VOLUME_SLIDER_TIP "Commande le volume de défaut pour cet insrument."
#undef SCRAMBLE_LABEL
#define SCRAMBLE_LABEL "Brouiller"

#undef SCRAMBLE_TIP
#define SCRAMBLE_TIP "Brouiller cette mesure par des divisions aléatoirement.  " \
	"Est-ce que c'est un dispositif utile?  Million de singes ne peuvent " \
	"pas être erronés toute heure."

#undef TIMEDIV_TIP
#define TIMEDIV_TIP "Employer ces derniers pour commander la manière que " \
	"la mesure est divisée pour le placement des battements. Ce premier " \
	"est également employé par la fonction de bousculade."
#undef TIMEDIV2_TIP
#define TIMEDIV2_TIP "Employer ces derniers pour commander la manière que " \
	"la mesure est divisée pour le placement des battements."

#undef REMOVE_SPACE_BEFORE_LABEL
#define REMOVE_SPACE_BEFORE_LABEL "Enlever l'espace avant"

#undef REMOVE_SPACE_BEFORE_TIP
#define REMOVE_SPACE_BEFORE_TIP "Enlève l'espace du commencement de la " \
	"mesure.  Placer le numérateur et le dénominateur à la fraction " \
	"de la « mesure d'être supprimé comme mesuré avant de supprimer. "

#undef ADD_SPACE_BEFORE_LABEL
#define ADD_SPACE_BEFORE_LABEL "Ajouter l'espace avant"
#undef ADD_SPACE_BEFORE_TIP
#define ADD_SPACE_BEFORE_TIP "Ajoute l'espace au commencement de la " \
	"mesure, serrant toutes notes vers la droite.  Placer le numérateur " \
	"et dénominateur pour indiquer la fraction de toute la mesure que le " \
	"nouvel espace devrait occuper comme mesuré après que l'opération " \
	"d'ajouter l'espace soit complète.  Par exemple si la mesure est 2 " \
	"unités longues et vous voulez être 3 unités, l'utilisation 1/3.  " \
	"Généralement si c'est des unités de X et vous voulez (X + Y) des " \
	"unités, emploient (Y - X) / (X + Y). "

#undef NUMERATOR_TIP
#define NUMERATOR_TIP "Numérateur.  Employer ceci en s'ajoutant/enlevant " \
	"l'espace à cette mesure."
#undef DENOMINATOR_TIP
#define DENOMINATOR_TIP "Dénominateur.  Employer ceci en s'ajoutant/enlevant " \
	"l'espace à cette mesure."

#undef ADD_SPACE_AFTER_LABEL
#define ADD_SPACE_AFTER_LABEL "Ajouter l'espace ensuite"
#undef ADD_SPACE_AFTER_TIP
#define ADD_SPACE_AFTER_TIP "Ajoute l'espace à la fin de la mesure, " \
	"serrant toutes notes vers la gauche.  Placer le numérateur et " \
	"dénominateur pour indiquer la fraction de toute la mesure que le " \
	"nouvel espace devrait occuper comme mesuré après que l'opération " \
	"d'ajouter l'espace soit complète.  Par exemple si la mesure est 2 " \
	"unités longues et vous voulez être 3 unités, l'utilisation 1/3.  " \
	"Généralement si c'est des unités de X et vous voulez (X + Y) des " \
	"unités, emploient (Y -X ) / (X + Y). "

#undef REMOVE_SPACE_AFTER_LABEL
#define REMOVE_SPACE_AFTER_LABEL "Enlever l'espace ensuite"
#undef REMOVE_SPACE_AFTER_TIP
#define REMOVE_SPACE_AFTER_TIP "Enlève l'espace de la fin de la mesure. " \
	"Placer le numérateur et le dénominateur à la fraction de la mesure " \
	"d'être supprimé comme mesuré avant de supprimer."

#undef LOOP_LABEL
#define LOOP_LABEL "Boucle"
#undef PATTERN_LOOP_TIP
#define PATTERN_LOOP_TIP "Quand vérifié, causera le playback à la " \
	"boucle jusqu'à ce que le « 'ARRÊT' » soit serré."

#undef TEMPO_CHANGES_LABEL
#define TEMPO_CHANGES_LABEL "Changements de tempo"
#undef SELECT_MEASURES_LABEL
#define SELECT_MEASURES_LABEL "Choisir les mesures"
#undef PASTE_MEASURES_LABEL
#define PASTE_MEASURES_LABEL "Coller les mesures"
#undef INSERT_MEASURES_LABEL
#define INSERT_MEASURES_LABEL "Insérer les mesures"
#undef DELETE_MEASURES_LABEL
#define DELETE_MEASURES_LABEL "Suppriment les mesures "
#undef TRANSPORT_LOC_LABEL
#define TRANSPORT_LOC_LABEL "L'endroit de transport"

#undef SELECT_MEASURES_TIP
#define SELECT_MEASURES_TIP "Cliquer ce bouton pour choisir toutes les " \
	"mesures, ou pour ne choisir deux fois aucune mesure.  Cliquer les " \
	"boutons vers la droite de choisir une seule mesure.  Cliquer, " \
	"traîner, et libérer au-dessus des boutons vers la droite de choisir " \
	"une gamme des mesures. "

#undef INSERT_MEASURES_TIP
#define INSERT_MEASURES_TIP "Cliquer les boutons vers la droite d'insérer " \
	" une seule mesure, ou utiliser ce bouton pour insérer des mesures " \
	"blanches pour les mesures choisies."

#undef DELETE_MEASURES_TIP
#define DELETE_MEASURES_TIP "Cliquer les boutons vers la droite de " \
	"supprimer une seule mesure, ou utiliser ce bouton pour " \
	"supprimer les mesures choisies."

#undef TEMPO_CHANGES_TIP
#define TEMPO_CHANGES_TIP "Cliquer les boutons vers la droite d'insérer " \
	"des changements de tempo "

#undef COPY_MEASURE_TIP
#define COPY_MEASURE_TIP "Cliquer les boutons vers la droite de copier une mesure"

#undef PASTE_MEASURES_TIP
#define PASTE_MEASURES_TIP "Cliquer les boutons vers la droite d'insérer " \
	" une mesure précédemment copiée "

#undef SONG_LOOP_TIP
#define SONG_LOOP_TIP "Quand vérifié, causera le playback à la boucle " \
	"jusqu'à ce que le « 'ARRÊT' » soit serré "

#undef FACTOR_DRUM_TAB_LABEL
#define FACTOR_DRUM_TAB_LABEL "Facteur le tablature de tambour"
#undef FACTOR_DRUM_TAB_TIP
#define FACTOR_DRUM_TAB_TIP "Quand vérifiées, des étiquettes collées " \
	"de tambour d'ASCII seront factorisées pour réduire des " \
	"modèles doubles sur une base de par-instrument, plutôt qu'une " \
	"base de par-mesure."

#undef MIDI_SETUP_LABEL
#define MIDI_SETUP_LABEL "Installation du MIDI"
#undef MIDI_SETUP_TIP
#define MIDI_SETUP_TIP "Placer le canal du MIDI pour transmettre " \
	"dessus, et envoyer les messages de changement de pièce " \
	"rapportée du MIDI. "

#undef PLAY_SONG_TIP
#define PLAY_SONG_TIP "Envoyer cette chanson au dispositif du MIDI pour le playback"

#undef PLAY_SELECTION_LABEL
#define PLAY_SELECTION_LABEL "Jouer le Choix"

#undef PLAY_SELECTION_TIP
#define PLAY_SELECTION_TIP "Envoyer les mesures choisies au dispositif " \
	"du MIDI pour le playback "
#undef SONG_LABEL
#define SONG_LABEL "Chanson"
#undef UNTITLED_SONG_LABEL
#define UNTITLED_SONG_LABEL "Chanson d'Untitled"

#undef FILE_LABEL
#define FILE_LABEL "Dossier"
#undef NEW_LABEL
#define NEW_LABEL "Nouveau"
#undef OPEN_LABEL
#define OPEN_LABEL "Ouvert"

#undef SAVE_LABEL
#undef SAVE_AS_LABEL
#undef IMPORT_PATTERNS_LABEL
#undef IMPORT_DRUM_TAB_LABEL
#undef EXPORT_TO_MIDI_FILE_LABEL
#undef QUIT_LABEL
#undef EDIT_LABEL

#define SAVE_LABEL "Stocker"
#define SAVE_AS_LABEL "Stocker avec le nouveau nom"
#define IMPORT_PATTERNS_LABEL "Importer les modèles"
#define IMPORT_DRUM_TAB_LABEL "Importer le tablature de tambour"
#define EXPORT_TO_MIDI_FILE_LABEL "Exporter la chanson vers un dossier du MIDI"
#define QUIT_LABEL "Stopper"
#define EDIT_LABEL "Éditer"
#undef HELP_LABEL
#define HELP_LABEL "Aide"
#undef ABOUT_LABEL
#define ABOUT_LABEL "Au sujet du Gneutronica"
#undef PASTE_DRUM_TAB_LABEL
#define PASTE_DRUM_TAB_LABEL "Coller le tablature de tambour d'ASCII"
#undef REMAP_DRUM_KIT_MENU_LABEL
#define REMAP_DRUM_KIT_MENU_LABEL "Remap le kit de tambour pour la chanson entière par l'intermédiaire du GM"

#undef METRONOME_LABEL
#define METRONOME_LABEL "Metronome"

#endif /* GNEUTRONICA_FRENCH */

#endif
