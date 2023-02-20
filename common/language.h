/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright 2006-2023 Hannu Jokinen
 * 
 * This file is part of JudoShiai.
 *
 * JudoShiai is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * JudoShiai is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with JudoShiai. If not, see
 * <https://www.gnu.org/licenses/>.
 */ 

#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

enum {
    LANG_FI = 0,
    LANG_SW,
    LANG_EN,
    LANG_ES,
    LANG_EE,
    LANG_UK,
    LANG_IS,
    LANG_NO,
    LANG_PL,
    LANG_SK,
    LANG_NL,
    LANG_CS,
    LANG_DE,
    //LANG_RU,
    LANG_DA,
    LANG_HE,
    LANG_FR,
    LANG_FA,
    NUM_LANGS
};

extern GtkWidget *TEST;

extern const gchar *timer_help_file_names[NUM_LANGS];

typedef gboolean (*cb_t)(GtkWidget *eventbox, GdkEventButton *event, void *param);

GtkWidget *get_language_menu(GtkWidget *window, cb_t cb);
void set_gui_language(gint l);
void start_help(GtkWidget *w, gpointer data);

#endif
