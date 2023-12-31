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
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>

#ifdef WIN32

#define  __USE_W32_SOCKETS
#include <winsock2.h>
#include <windows.h>
//#include <stdio.h>
#include <initguid.h>
#include <ws2tcpip.h>
#include <process.h>

#else

#include <sys/types.h>
#include <unistd.h>

#endif

#include "comm.h"

extern gboolean debug;

extern void serial_set_device(gchar *dev);
extern void serial_set_baudrate(gint baud);
extern void serial_set_type(gint type);
extern gpointer serial_thread(gpointer args);
extern gpointer websock_thread(gpointer args);
G_LOCK_EXTERN(send_mutex);

GTimer        *timer;
GKeyFile      *keyfile;
gchar         *conffile;
int webpwcrc32 = 0;

void open_comm_socket(void)
{
    g_mutex_init(&G_LOCK_NAME(send_mutex));

#ifdef WIN32
    WSADATA WSAData = {0};

    if (0 != WSAStartup(MAKEWORD(2, 2), &WSAData)) {
        printf("-FATAL- | Unable to initialize Winsock version 2.2\n");
        return;
    }
#endif
    /*
    g_timeout_add(4000, timeout_callback, NULL);
    g_timeout_add(500, conn_info_callback, NULL);
    g_timeout_add(20, check_for_input, NULL);
    */
//        g_idle_add(check_for_input, NULL);
}

gboolean   run_flag = TRUE;   /* used as exit flag for threads */

void close_all(void)
{
    run_flag = FALSE;
    g_usleep(1000000);
}

int main( int   argc,
          char *argv[] )
{
    time_t     now;
    struct tm *tm;
    GThread   *gth = NULL;         /* thread id */
    int i, demo = 0;

    putenv("UBUNTU_MENUPROXY=");

    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "dev")) {
	    if (++i >= argc) {
		printf("missing device\n");
		return 1;
	    }
	    serial_set_device(argv[i]);
	} else if (!strcmp(argv[i], "baud")) {
	    if (++i >= argc) {
		printf("missing baudrate\n");
		return 1;
	    }
	    serial_set_baudrate(atoi(argv[i]));
	} else if (!strcmp(argv[i], "type")) {
	    if (++i >= argc) {
		printf("missing scale type\n");
		return 1;
	    }
	    serial_set_type(atoi(argv[i]));
	} else if (!strcmp(argv[i], "demo")) {
	    demo = 1;
	} else if (!strcmp(argv[i], "debug")) {
	    debug = TRUE;
	}
    }

    /* timers */

    //timer = g_timer_new();

    open_comm_socket();

    /* Create a bg thread using glib */

    gth = g_thread_new("WebSocket",
                       (GThreadFunc)websock_thread,
                       (gpointer)&run_flag);
    gth = g_thread_new("Serial",
                       (GThreadFunc)serial_thread,
                       (gpointer)&run_flag);

    atexit(close_all);

    while (1) {
	g_usleep(1000000);
	if (demo) {
	    struct message msg;
	    memset(&msg, 0, sizeof(msg));
	    msg.type = MSG_SCALE;
	    msg.u.scale.weight = 50000 + (time(NULL)%10)*1000;
	    send_packet(&msg);
	}
    }

    run_flag = FALSE;     /* flag threads to stop and exit */
    //g_thread_join(gth);   /* wait for thread to exit */ 

    return 0;
}

gchar *logfile_name = NULL;

void judoweight_log(gchar *format, ...)
{
    time_t t;
    gchar buf[256];
    va_list args;
    va_start(args, format);
    gchar *text = g_strdup_vprintf(format, args);
    va_end(args);

    t = time(NULL);

    if (logfile_name == NULL) {
        struct tm *tm = localtime((time_t *)&t);
        sprintf(buf, "judoweight_%04d%02d%02d_%02d%02d%02d.log",
                tm->tm_year+1900,
                tm->tm_mon+1,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec);
        logfile_name = g_build_filename(g_get_user_data_dir(), buf, NULL);
        g_print("logfile_name=%s\n", logfile_name);
    }

    FILE *f = g_fopen(logfile_name, "a");
    if (f) {
        struct tm *tm = localtime((time_t *)&t);
#ifdef USE_ISO_8859_1
        gsize x;

        gchar *text_ISO_8859_1 =
            g_convert(text, -1, "ISO-8859-1", "UTF-8", NULL, &x, NULL);

        fprintf(f, "%02d:%02d:%02d %s\n",
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec,
                text_ISO_8859_1);

        g_free(text_ISO_8859_1);
#else
        fprintf(f, "%02d:%02d:%02d %s\n",
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec,
                text);
#endif
        fclose(f);
    } else {
        g_print("Cannot open log file\n");
        perror(logfile_name);
    }

    g_free(text);
}
