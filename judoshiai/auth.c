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

#if defined(__WIN32__) || defined(WIN32)

#define  __USE_W32_SOCKETS
//#define Win32_Winsock

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include <ws2tcpip.h>

#else /* UNIX */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#endif /* WIN32 */

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) gettext(String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain)
#define bind_textdomain_codeset(Domain,Codeset) (Codeset)
#endif /* ENABLE_NLS */

#include <gtk/gtk.h>

#include "judoshiai.h"

#define NUM_USERS 8
    
static struct {
    char user[32];
    gint pass;
} users[NUM_USERS];

int auth_set_user(int n, char *u, char *p)
{
    char buf[64];

    if (n < 0 || n >= NUM_USERS)
        return -1;
    
    snprintf(users[n].user, sizeof(users[n].user), "%s", u);
    snprintf(buf, sizeof(buf), "%s%s", u, p);
    users[n].pass = pwcrc32(buf, strlen(buf));

    snprintf(buf, sizeof(buf), "user%d", n);
    g_key_file_set_string(keyfile, "preferences", buf, users[n].user);
    snprintf(buf, sizeof(buf), "pass%d", n);
    g_key_file_set_integer(keyfile, "preferences", buf, users[n].pass);
    
}

int auth_validate(char *u, char *p)
{
    int i;
    char buf[64];

    snprintf(buf, sizeof(buf), "%s%s", u, p);
    int crc = pwcrc32(buf, strlen(buf));

    for (i = 0; i < NUM_USERS; i++) {
        if (crc == users[i].pass)
            return 0;
    }

    return -1;
}

void auth_init(void)
{
    int i;
    char buf[64];
    GError  *error = NULL;
    gchar   *str;

    memset(&users, 0, sizeof(users));
    
    for (i = 0; i < NUM_USERS; i++) {
        error = NULL;
        snprintf(buf, sizeof(buf), "user%d", i);
        str = g_key_file_get_string(keyfile, "preferences", buf, &error);
        if (!error) {
            snprintf(users[i].user, sizeof(users[i].user), "%s", str);
            g_free(str);
        }

        snprintf(buf, sizeof(buf), "pass%d", i);
        users[i].pass = g_key_file_get_integer(keyfile, "preferences", buf, &error);
    }
}

void auth_edit()
{
    GtkWidget *dialog;
    GtkWidget *table = gtk_grid_new();
    GtkWidget *user[NUM_USERS], *pw[NUM_USERS], for_api[NUM_USERS];
    gint i;
#if 0
    dialog = gtk_dialog_new_with_buttons (_("Users"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Users")), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Password")), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("For API")), 2, 0, 1, 1);

    for (i = 0; i < NUM_USERS; i++) {
        GtkWidget *tmp = user[i] = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(tmp), 32);
        gtk_entry_set_text(GTK_ENTRY(tmp), users[i].user);
        gtk_grid_attach(GTK_GRID(table), 0, 1 + i, row, 1, 1);
        
        tmp = pw[i] = gtp_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(tmp), 32);
        gtk_grid_attach(GTK_GRID(table), 1, 1 + i, row, 1, 1);

        //tmp = for_api[i] = gtk_check_button_new();
        gtk_grid_attach(GTK_GRID(table), 2, 1 + i, row, 1, 1);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    }

    gtk_widget_destroy(dialog);
#endif
}
