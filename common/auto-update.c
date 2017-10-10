/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2016 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>

#ifdef WIN32

#define  __USE_W32_SOCKETS
#include <windows.h>
//#include <stdio.h>
#include <initguid.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#else

#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#endif

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

#include "comm.h"

/* System-dependent definitions */
#ifndef WIN32
#define SOCKET          gint
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#endif

extern gchar *installation_dir;
extern gchar *program_path;
extern GtkWindow *main_window;
extern void destroy(GtkWidget *widget, gpointer data);

static int closesock(int s)
{
#if defined(__WIN32__) || defined(WIN32)
    shutdown(s, SD_SEND);
    return closesocket(s);
#else
    return close(s);
#endif
}

static void update_callback(GtkWidget *widget,
			    GdkEvent *event,
			    GtkWidget *data)
{
    if (ptr_to_gint(event) == GTK_RESPONSE_OK) {
	gchar buf[1024];
	struct in_addr addr;
	addr.s_addr = (gint)(uintptr_t)data;
#ifdef WIN32
	gchar *updater = g_build_filename(installation_dir, "bin", "auto-update.exe", NULL);
	snprintf(buf, sizeof(buf), "%s \"%s\" \"%s\" %d",
		 inet_ntoa(addr),
		 installation_dir, program_path,
		 GetCurrentProcessId());
	ShellExecute(NULL, NULL, updater, buf, installation_dir, SW_SHOWNORMAL);
	g_free(updater);
#if 0
	GtkWidget *dialog = gtk_message_dialog_new (main_window,
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_MESSAGE_INFO,
						   GTK_BUTTONS_OK,
						   "Close application to continue\n(%s)",
						   program_path);
	g_signal_connect_swapped (dialog, "response",
				  G_CALLBACK(gtk_widget_destroy),
				  dialog);
	gtk_widget_show(dialog);
#else
	destroy(NULL, NULL);
#endif
#endif

    }
    gtk_widget_destroy(widget);
}

void check_for_update(gulong addr, gchar *app)
{
    struct sockaddr_in node;
    SOCKET comm_fd;
    gchar buf[1024];

#ifndef WIN32
    return;
#endif
    g_print("Check for version, app=%s\n", app);
    if ((comm_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
	perror("socket");
	return;
    }

    memset(&node, 0, sizeof(node));
    node.sin_family      = AF_INET;
    node.sin_port        = htons(2308);
    node.sin_addr.s_addr = addr;

    if (connect(comm_fd, (struct sockaddr *)&node, sizeof(node))) {
	perror("check_for_update: connect");
	closesock(comm_fd);
	return;
    }

#if 0
    gchar *verfile = g_build_filename("etc", "version.txt", NULL);
    send(comm_fd, verfile, strlen(verfile), 0);
    g_free(verfile);
#else
    send(comm_fd, "VER", 3, 0);
#endif
    int n = recv(comm_fd, buf, sizeof(buf)-1, 0);
    closesock(comm_fd);

    buf[n] = 0;
    gchar *p1, *p = strchr(buf, '\n');
    if (p) *p = 0;
    p = strchr(buf, '\r');
    if (p) *p = 0;

    if (strlen(buf) == 0) {
	g_print("No version received\n");
	return;
    }

    p = full_version();

    g_print("Local: %s, remote: %s\n", p, buf);

    if (!strstr(p, "Windows")) {
	g_print("This is not Windows\n");
	return;
    }

    if (!strstr(buf, "Windows")) {
	g_print("Remote is not Windows\n");
	return;
    }

    p1 = strchr(buf, ' ');
    if (!p1) return;
    *p1 = 0;

    p1 = strchr(p, ' ');
    if (!p1) return;
    *p1 = 0;

    if (strcmp(buf, p) == 0)
	return;

    g_print("SW NEEDS UPDATE\n");

    GtkWidget *dialog, *hbox, *label;

    dialog = gtk_dialog_new_with_buttons (_("Update"),
                                          main_window,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          NULL);

    hbox = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

    label = gtk_label_new(_("New version:"));
    gtk_grid_attach(GTK_GRID(hbox), label, 0, 0, 1, 1);

    label = gtk_label_new(_("Current version:"));
    gtk_grid_attach(GTK_GRID(hbox), label, 0, 1, 1, 1);

    label = gtk_label_new(buf);
    gtk_grid_attach(GTK_GRID(hbox), label, 1, 0, 1, 1);

    label = gtk_label_new(SHIAI_VERSION);
    gtk_grid_attach(GTK_GRID(hbox), label, 1, 1, 1, 1);

    label = gtk_label_new(_("Update to new version?"));
    gtk_grid_attach(GTK_GRID(hbox), label, 0, 2, 2, 1);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		       hbox, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(update_callback), (void*)(uintptr_t)(addr));
}
