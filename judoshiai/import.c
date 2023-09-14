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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "sqlite3.h"
#include "judoshiai.h"

enum FIELDS{
    TXT_LAST = 0,
    TXT_FIRST,
    TXT_BIRTH,
    TXT_BELT,
    TXT_CLUB,
    TXT_COUNTRY,
    TXT_REGCAT,
    TXT_WEIGHT,
    TXT_ID,
    TXT_COACHID,
    TXT_SEEDING,
    TXT_CLUBSEEDING,
    TXT_GENDER,
    TXT_COMMENT1,
    TXT_COMMENT2,
    TXT_COMMENT3,
    NUM_TXTS
};

enum SETTINGS{
    TXT_GIRLSTR = 0,
    TXT_SEPARATOR,
    TXT_QUOTE,
    BOOL_UTF8,
    BOOL_HASHEADER,
    NUM_SETTINGS
};

struct i_text {
    GtkWidget *lineread;
    GtkWidget *fields[NUM_TXTS];
    GtkWidget *setting_fields[NUM_SETTINGS];
    gchar *filename;
    gchar separator[8];
    gchar quote[8];
    gchar girlstr[20];
    GtkWidget *labels[NUM_TXTS];
    gint columns[NUM_TXTS];
    gint errors;
    gint comp_added;
    gint comp_exists;
    gint comp_syntax;
    gboolean utf8;
    gboolean hasHeader;
};

#define MAX_NUM_COLUMNS 21
static gchar *combotxts[MAX_NUM_COLUMNS] = {0};

static gboolean valid_data(gint item, gchar **tokens, gint num_cols, struct i_text *d)
{
    if (d->columns[item] == 0)
        return FALSE;

    if (d->columns[item] > num_cols)
        return FALSE;

    return TRUE;
}

static gboolean print_item(gint item, gchar **tokens, gint num_cols, struct i_text *d)
{
    if (!valid_data(item, tokens, num_cols, d)){
        gtk_label_set_text(GTK_LABEL(d->labels[item]), "");
        return FALSE;
    }

    gtk_label_set_text(GTK_LABEL(d->labels[item]), tokens[d->columns[item] - 1]);
    //mylog("Item %d = %s\n", item, tokens[d->columns[item]-1]);
    return TRUE;
}

static gboolean add_competitor(gchar **tokens, gint num_cols, struct i_text *d)
{
    gchar *newcat = NULL;
    struct judoka j;

    if (!valid_data(TXT_FIRST, tokens, num_cols, d))
        return FALSE;
    if (!valid_data(TXT_LAST, tokens, num_cols, d))
        return FALSE;

    memset(&j, 0, sizeof(j));
    j.index = comp_index_get_free();//current_index++;
    j.visible = TRUE;
    j.category = "?";
    j.club = "";
    j.country = "";
    j.id = "";
    j.comment = "";
    j.coachid = "";

    // remove extra spaces with convert_name(x, 0)
    j.first = convert_name(tokens[d->columns[TXT_FIRST] - 1], 0);

    // remove extra spaces with convert_name(x, 0)
    j.last = convert_name(tokens[d->columns[TXT_LAST] - 1], 0);

    /* year of birth. 0 == unknown */
    if (valid_data(TXT_BIRTH, tokens, num_cols, d))
        j.birthyear = atoi(tokens[d->columns[TXT_BIRTH] - 1]);

    /* belt */
    if (valid_data(TXT_BELT, tokens, num_cols, d)) {
        gchar *b = tokens[d->columns[TXT_BELT] - 1];
        j.belt = props_get_grade(b);
    }

    if (valid_data(TXT_CLUB, tokens, num_cols, d))
        j.club = tokens[d->columns[TXT_CLUB] - 1];

    if (valid_data(TXT_COUNTRY, tokens, num_cols, d))
        j.country = tokens[d->columns[TXT_COUNTRY] - 1];

    if (valid_data(TXT_ID, tokens, num_cols, d))
        j.id = tokens[d->columns[TXT_ID] - 1];

    if (valid_data(TXT_COACHID, tokens, num_cols, d))
        j.coachid = tokens[d->columns[TXT_COACHID] - 1];

    if (valid_data(TXT_REGCAT, tokens, num_cols, d))
        j.regcategory = tokens[d->columns[TXT_REGCAT] - 1];

    if (valid_data(TXT_WEIGHT, tokens, num_cols, d))
        j.weight = weight_grams(tokens[d->columns[TXT_WEIGHT] - 1]);

#if 0
    if (j.birthyear > 20 && j.birthyear <= 99)
        j.birthyear += 1900;
    else if (j.birthyear <= 20)
        j.birthyear += 2000;
#endif

    gint gender = 0;

    if (valid_data(TXT_GENDER, tokens, num_cols, d)) {
        if (strstr(tokens[d->columns[TXT_GENDER] - 1], d->girlstr)) {
            gender = IS_FEMALE;
            j.deleted |= GENDER_FEMALE;
        } else {
            gender = IS_MALE;
            j.deleted |= GENDER_MALE;
        }
    }

    if ((j.regcategory == 0 || j.regcategory[0] == 0) /*&& j.weight > 10000*/) {
        gint age = current_year - j.birthyear;

        j.regcategory = newcat = find_correct_category(age, j.weight, gender, NULL, TRUE);
        if (j.regcategory == NULL)
            j.regcategory = newcat = g_strdup("");
    }

    if (valid_data(TXT_SEEDING, tokens, num_cols, d))
        j.seeding = atoi(tokens[d->columns[TXT_SEEDING] - 1]);

    if (valid_data(TXT_CLUBSEEDING, tokens, num_cols, d))
        j.clubseeding = atoi(tokens[d->columns[TXT_CLUBSEEDING] - 1]);

    gchar *comments[NUM_COMMENT_COLS];

    if (valid_data(TXT_COMMENT1, tokens, num_cols, d)) comments[0] = tokens[d->columns[TXT_COMMENT1] - 1];
    else comments[0] = "";
    if (valid_data(TXT_COMMENT2, tokens, num_cols, d)) comments[1] = tokens[d->columns[TXT_COMMENT2] - 1];
    else comments[1] = "";
    if (valid_data(TXT_COMMENT3, tokens, num_cols, d)) comments[2] = tokens[d->columns[TXT_COMMENT3] - 1];
    else comments[2] = "";

    if (comments[0][0] || comments[1][0] || comments[2][0])
	j.comment = g_strjoin("#~", comments[0], comments[1], comments[2], NULL);
    else
	j.comment = g_strdup("");
    
    GtkTreeIter iter;
    if (find_iter_name_2(&iter, j.last, j.first, j.club, j.regcategory)) {
        d->comp_exists++;
        shiai_log(0, 0, "Competitor exists: %s %s, %s %s", j.last, j.first, j.country, j.club);
	g_free((gpointer)j.comment);
        g_free(newcat);
        return FALSE;
    }

    gint rc = db_add_judoka(j.index, &j);
    if (rc == SQLITE_OK)
        display_one_judoka(&j);
    else {
        d->comp_syntax++;
        shiai_log(0, 0, "Syntax error: %s %s, %s %s", j.last, j.first, j.country, j.club);
	g_free((gpointer)j.comment);
        g_free(newcat);
        return FALSE;
    }
    //update_competitors_categories(j.index);
    //matches_refresh();

    g_free((gpointer)j.comment);
    g_free(newcat);
    return TRUE;
}

static gint file_is_utf8(gchar *fname)
{
    gchar line[256];
    const gchar *end;
    FILE *f = g_fopen(fname, "r");
    if (!f)
        return -1;

    while (fgets(line, sizeof(line), f)) {
	if (!g_utf8_validate(line, -1, &end)) {
	    fclose(f);
	    return 0;
	}
    }

    fclose(f);
    return 1;
}

// adjust BUFFER_SIZE to suit longest line
#define BUFFER_SIZE 1024 * 1024

// char* array will point to fields
char *pFields[MAX_NUM_COLUMNS];

static int loadValues(char *line, long lineno, struct i_text *d){
    if(line == NULL)
        return 0;

    // chop of last char of input if it is a CR or LF (e.g.Windows file loading in Unix env.)
    // can be removed if sure fgets has removed both CR and LF from end of line
    if(*(line + strlen(line)-1) == '\r' || *(line + strlen(line)-1) == '\n')
        *(line + strlen(line)-1) = '\0';
    if(*(line + strlen(line)-1) == '\r' || *(line + strlen(line)-1 )== '\n')
        *(line + strlen(line)-1) = '\0';

    char *cptr = line;
    int fld = 0;
    int inquote = FALSE;
    char ch;

    pFields[fld]=cptr;
    while((ch=*cptr) != '\0' && fld < MAX_NUM_COLUMNS){
        if(ch == d->quote[0]) {
            if(! inquote)
                pFields[fld]=cptr+1;
            else {
                *cptr = '\0';               // zero out " and jump over it
            }
            inquote = ! inquote;
        } else if(ch == d->separator[0] && ! inquote){
            *cptr = '\0';                   // end of field, null terminate it
            pFields[++fld]=cptr+1;
        }
        cptr++;
    }
    return fld+1;
}

static void import_txt(gchar *fname, gboolean test, struct i_text *d)
{
    gint num_cols = 0;
    gchar line [BUFFER_SIZE];
    long lineno = 0L;

    if (d->separator[0] == 0)
        return;

    FILE *f = g_fopen(fname, "r");
    if(f == NULL)
        return;

    d->errors = 0;
    d->comp_added = 0;
    d->comp_exists = 0;

    while (!feof(f)) {
        gchar *utf = NULL;

        // load line into static buffer
        if(fgets(line, BUFFER_SIZE-1, f)==NULL)
            break;

        // jump over empty lines
        if(strlen(line) < 2)
            continue;

        // convert encoding if required
        if (d->utf8 == FALSE)
            utf = g_convert(line, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);

        // skip first line (headers)
        if(++lineno==1) {
            gtk_label_set_text(GTK_LABEL(d->lineread), utf ? utf : line);
            if (d->hasHeader)
               continue;
        }

        // set pFields array pointers to null-terminated string fields in line
        if((num_cols = loadValues(utf ? utf : line,lineno,d))==0) {
            d->errors++;
        } else {
            // On return pFields array pointers point to loaded fields ready for load into DB or whatever
            // Fields can be accessed via pFields, e.g.
            if (test) {
                for (int i = 0; i < NUM_TXTS; i++) {
                    print_item(i, pFields, num_cols, d);
                }
            } else {
                if (add_competitor(pFields, num_cols, d))
                    d->comp_added++;
                else
                    d->errors++;
            }

            g_free(utf);

            if (test)
                break;
        }
    }
    fclose(f);
}

static void read_values(struct i_text *d)
{
    strncpy(d->separator,
           gtk_entry_get_text(GTK_ENTRY(d->setting_fields[TXT_SEPARATOR])),sizeof(d->separator));
    if (d->separator[0] == '\\' && d->separator[1] == 't') {
        d->separator[0] = '\t';
        d->separator[1] = 0;
    }

    strncpy(d->quote, gtk_entry_get_text(GTK_ENTRY(d->setting_fields[TXT_QUOTE])),sizeof(d->quote));

    strncpy(d->girlstr, gtk_entry_get_text(GTK_ENTRY(d->setting_fields[TXT_GIRLSTR])),sizeof(d->girlstr));

    d->hasHeader = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->setting_fields[BOOL_HASHEADER]));
    d->utf8 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->setting_fields[BOOL_UTF8]));

    for (gint i = 0; i < NUM_TXTS; i++) {
        d->columns[i] = gtk_combo_box_get_active(GTK_COMBO_BOX(d->fields[i]));
    }
}

static void selecter(GtkComboBox *combo, gpointer arg)
{
    struct i_text *d = arg;
    read_values(d);
    import_txt(d->filename, TRUE, d);
}

static GtkWidget *set_col_entry(GtkWidget *table, int row,
				char *text, const gint defpos, struct i_text *data)
{
    GtkWidget *tmp;
    gint i;

    tmp = gtk_label_new(text);
#if (GTKVER == 3)
    gtk_grid_attach(GTK_GRID(table), tmp, 0, row, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 0, 1, row, row + 1);
#endif
    data->labels[row] = tmp = gtk_label_new("        ");
#if (GTKVER == 3)
    gtk_grid_attach(GTK_GRID(table), tmp, 2, row, 1, 1);

    tmp = gtk_combo_box_text_new();
    for (i = 0; i < MAX_NUM_COLUMNS; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(tmp), NULL, combotxts[i]);
    }
#else
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 2, 3, row, row + 1);

    tmp = gtk_combo_box_new_text();
    for (i = 0; i < MAX_NUM_COLUMNS; i++) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), combotxts[i]);
    }
#endif
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), defpos);

    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(selecter), data);

#if (GTKVER == 3)
    gtk_grid_attach(GTK_GRID(table), tmp, 1, row, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(table), tmp, 1, 2, row, row + 1);
#endif
    return tmp;
}

static void set_preferences_int(gchar *name, gint data)
{
    g_key_file_set_integer(keyfile, "preferences", name, data);
}

static void set_preferences_str(gchar *name, gchar *data)
{
    g_key_file_set_string(keyfile, "preferences", name, data);
}

static gboolean get_preferences_int(gchar *name, gint *out)
{
    GError  *error = NULL;
    gint     x;
    x = g_key_file_get_integer(keyfile, "preferences", name, &error);
    if (!error) {
        *out = x;
        return TRUE;
    }
    *out = 0;
    return FALSE;
}

static gboolean get_preferences_str(gchar *name, gchar *out)
{
    GError  *error = NULL;
    gchar   *x;
    x = g_key_file_get_string(keyfile, "preferences", name, &error);
    if (!error) {
        strcpy(out, x);
        g_free(x);
        return TRUE;
    }
    out[0] = 0;
    return FALSE;
}

void import_txt_dialog(GtkWidget *w, gpointer arg)
{
    GtkWidget *dialog, *table;
    gchar *name = NULL;
    gchar buf[32];
    gint i;

    if (!combotxts[0]) {
        combotxts[0] = g_strdup(_("Not used"));
        for (i = 1; i < MAX_NUM_COLUMNS; i++) {
            sprintf(buf, "%s %d", _("Column"), i);
            combotxts[i] = g_strdup(buf);
        }
    }

    dialog = gtk_file_chooser_dialog_new (_("Import from file"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);
    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    }

    gtk_widget_destroy (dialog);

    if (!name)
        return;

    valid_ascii_string(name);

    /* settings */

    struct i_text *data = g_malloc(sizeof(*data));
    memset(data, 0, sizeof(*data));

    data->filename = name;

    get_preferences_int("importtxtcollast",   &data->columns[TXT_LAST]);
    get_preferences_int("importtxtcolfirst",  &data->columns[TXT_FIRST]);
    get_preferences_int("importtxtcolbirth",  &data->columns[TXT_BIRTH]);
    get_preferences_int("importtxtcolbelt",   &data->columns[TXT_BELT]);
    get_preferences_int("importtxtcolclub",   &data->columns[TXT_CLUB]);
    get_preferences_int("importtxtcolcountry",&data->columns[TXT_COUNTRY]);
    get_preferences_int("importtxtcolid",     &data->columns[TXT_ID]);
    get_preferences_int("importtxtcolcoachid",&data->columns[TXT_COACHID]);
    get_preferences_int("importtxtcolregcat", &data->columns[TXT_REGCAT]);
    get_preferences_int("importtxtcolweight", &data->columns[TXT_WEIGHT]);
    get_preferences_int("importtxtcolgender", &data->columns[TXT_GENDER]);
    get_preferences_int("importtxtcolseeding",&data->columns[TXT_SEEDING]);
    get_preferences_int("importtxtcolclubseeding",&data->columns[TXT_CLUBSEEDING]);
    get_preferences_int("importtxtcolcomment",&data->columns[TXT_COMMENT1]);
    get_preferences_int("importtxtcolcomment2",&data->columns[TXT_COMMENT2]);
    get_preferences_int("importtxtcolcomment3",&data->columns[TXT_COMMENT3]);
    get_preferences_str("importtxtgirlstr",    data->girlstr);
    get_preferences_str("importtxtseparator",  data->separator);
    get_preferences_str("importtxtquote",      data->quote);
    get_preferences_int("importtxthasheader",  &data->hasHeader);
    if (data->separator[0] == 0)
        strcpy(data->separator, ",");
    if (data->quote[0] == 0)
            strcpy(data->quote, "\"");

    dialog = gtk_dialog_new_with_buttons (_("Preferences"),
					  GTK_WINDOW(main_window),
                                          GTK_DIALOG_MODAL,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);

    data->lineread = gtk_label_new("");
#if (GTKVER == 3)
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       data->lineread, FALSE, FALSE, 0);
    table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       table, FALSE, FALSE, 0);
#else
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), data->lineread);
    table = gtk_table_new(3, 16, FALSE);
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), table);
#endif

    data->fields[TXT_LAST]      = set_col_entry (table, 0, _("Last Name:"),        data->columns[TXT_LAST],   data);
    data->fields[TXT_FIRST]     = set_col_entry (table, 1, _("First Name:"),       data->columns[TXT_FIRST],  data);
    data->fields[TXT_BIRTH]     = set_col_entry (table, 2, _("Year of Birth:"),    data->columns[TXT_BIRTH],  data);
    data->fields[TXT_BELT]      = set_col_entry (table, 3, _("Grade:"),            data->columns[TXT_BELT],   data);
    data->fields[TXT_CLUB]      = set_col_entry (table, 4, _("Club:"),             data->columns[TXT_CLUB],   data);
    data->fields[TXT_COUNTRY]   = set_col_entry (table, 5, _("Country:"),          data->columns[TXT_COUNTRY],data);
    data->fields[TXT_REGCAT]    = set_col_entry (table, 6, _("Category:"),         data->columns[TXT_REGCAT], data);
    data->fields[TXT_WEIGHT]    = set_col_entry (table, 7, _("Weight:"),           data->columns[TXT_WEIGHT], data);
    data->fields[TXT_ID]        = set_col_entry (table, 8, _("Id:"),               data->columns[TXT_ID],     data);
    data->fields[TXT_COACHID]   = set_col_entry (table, 9, _("Coach Id:"),         data->columns[TXT_COACHID],data);
    data->fields[TXT_SEEDING]   = set_col_entry (table,10, _("Seeding:"),          data->columns[TXT_SEEDING],data);
    data->fields[TXT_CLUBSEEDING] = set_col_entry (table,11, _("Club Seeding:"),   data->columns[TXT_CLUBSEEDING],data);
    data->fields[TXT_GENDER]    = set_col_entry (table,12, _("Sex:"),              data->columns[TXT_GENDER], data);
    data->fields[TXT_COMMENT1]   = set_col_entry (table,13, _("Comment:"),          data->columns[TXT_COMMENT1],data);
    data->fields[TXT_COMMENT2]   = set_col_entry (table,14, _("Comment:"),          data->columns[TXT_COMMENT2],data);
    data->fields[TXT_COMMENT3]   = set_col_entry (table,15, _("Comment:"),          data->columns[TXT_COMMENT3],data);

#if (GTKVER == 3)
    table = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       table, FALSE, FALSE, 0);

    GtkWidget *tmp;
    //GIRL TEXT
    tmp = gtk_label_new(_("Girl Text:"));
    gtk_grid_attach(GTK_GRID(table), tmp, 0, 0, 1, 1);
    tmp = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(tmp), 4);
    gtk_entry_set_text(GTK_ENTRY(tmp), data->girlstr);
    data->setting_fields[TXT_GIRLSTR]=tmp;
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(selecter), data);
    gtk_grid_attach(GTK_GRID(table), tmp, 1, 0, 1, 1);
    //HAS HEADER
    tmp = gtk_check_button_new_with_label(_("First line is header"));
    data->setting_fields[BOOL_HASHEADER]=tmp;
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(selecter), data);
    gtk_grid_attach(GTK_GRID(table), tmp, 2, 0, 2, 1);
    //SEPARATOR
    tmp = gtk_label_new(_("Column Separator:"));
    gtk_grid_attach(GTK_GRID(table), tmp, 0, 1, 1, 1);
    tmp = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(tmp), 4);
    gtk_entry_set_text(GTK_ENTRY(tmp), data->separator);
    data->setting_fields[TXT_SEPARATOR]=tmp;
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(selecter), data);
    gtk_grid_attach(GTK_GRID(table), tmp, 1, 1, 1, 1);
    //QUOTE
    tmp = gtk_label_new(_("Quote Character:"));
    gtk_grid_attach(GTK_GRID(table), tmp, 2, 1, 1, 1);
    tmp = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(tmp), 4);
    gtk_entry_set_text(GTK_ENTRY(tmp), data->quote);
    data->setting_fields[TXT_QUOTE]=tmp;
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(selecter), data);
    gtk_grid_attach(GTK_GRID(table), tmp, 3, 1, 1, 1);
    //UTF8
    tmp = gtk_check_button_new_with_label(_("UTF-8"));;
    data->setting_fields[BOOL_UTF8]=tmp;
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(selecter), data);
    gtk_grid_attach(GTK_GRID(table), tmp, 0, 2, 2, 1);

    if (file_is_utf8(name) == 1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->setting_fields[BOOL_UTF8]), TRUE);
	data->utf8 = TRUE;
    }
#endif

    gtk_widget_show_all(dialog);

    read_values(data);
    import_txt(name, TRUE, data);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
        read_values(data);
        import_txt(name, FALSE, data);
        //matches_refresh();

        set_preferences_int("importtxtcollast",   data->columns[TXT_LAST]);
        set_preferences_int("importtxtcolfirst",  data->columns[TXT_FIRST]);
        set_preferences_int("importtxtcolbirth",  data->columns[TXT_BIRTH]);
        set_preferences_int("importtxtcolbelt",   data->columns[TXT_BELT]);
        set_preferences_int("importtxtcolclub",   data->columns[TXT_CLUB]);
        set_preferences_int("importtxtcolcountry",data->columns[TXT_COUNTRY]);
        set_preferences_int("importtxtcolid",     data->columns[TXT_ID]);
        set_preferences_int("importtxtcolcoachid",data->columns[TXT_COACHID]);
        set_preferences_int("importtxtcolregcat", data->columns[TXT_REGCAT]);
        set_preferences_int("importtxtcolweight", data->columns[TXT_WEIGHT]);
        set_preferences_int("importtxtcolgender", data->columns[TXT_GENDER]);
        set_preferences_int("importtxtcolseeding",data->columns[TXT_SEEDING]);
        set_preferences_int("importtxtcolclubseeding",data->columns[TXT_CLUBSEEDING]);
        set_preferences_str("importtxtgirlstr",   data->girlstr);
        set_preferences_str("importtxtseparator", data->separator);
        set_preferences_str("importtxtquote",     data->quote);
        set_preferences_int("importtxthasheader", data->hasHeader);
        set_preferences_int("importtxtcolcomment",data->columns[TXT_COMMENT1]);
        set_preferences_int("importtxtcolcomment2",data->columns[TXT_COMMENT2]);
        set_preferences_int("importtxtcolcomment3",data->columns[TXT_COMMENT3]);
    }

    gtk_widget_destroy(dialog);

    SHOW_MESSAGE("%d %s, %d %s (%d %s, %d %s).",
                 data->comp_added, _("competitors added"),
                 data->errors, _("errors"),
                 data->comp_exists, _("competitors existed already"),
                 data->comp_syntax, _("syntax errors"));

    g_free(data);
    g_free(name);
}
