/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2017 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "sqlite3.h"
#include "judoshiai.h"

#define PROTO_FTP     0
#define PROTO_HTTP    1
#define PROTO_RFC1738 2

static void ftp_log(gchar *format, ...);

struct url {
    GtkWidget *address, *port, *path, *start;
    GtkWidget *user, *password, *proxy_address, *proxy_port;
    GtkWidget *do_it, *proto, *proxy_user, *proxy_password;
};

static gchar *ftp_server = NULL, *ftp_path = NULL, *proxy_host = NULL,
    *ftp_user = NULL, *ftp_password = NULL, *proxy_user, *proxy_password;
static gint ftp_port = 21, proxy_port = 8080, proto = PROTO_FTP;
static gboolean ftp_update = FALSE, do_ftp = FALSE;

static const gchar *progr_file;
static gchar progr_pros[32];
static GtkWidget *progr_1w = NULL, *progr_2w = NULL, *progr_3w = NULL;
static gint xfer_ok = 0, xfer_nok = 0;

#define STRDUP(_d, _s) do { g_free(_d); _d = g_strdup(_s); } while (0)
#define GETSTR(_d, _s) do { g_free(_d);                                 \
        _d = g_key_file_get_string(keyfile, "preferences", _s, NULL);   \
        if (!_d) _d = g_strdup(""); } while (0)
#define GETINT(_d, _s) do { GError *x=NULL; _d = g_key_file_get_integer(keyfile, "preferences", _s, &x); } while (0)

static void ftp_to_server_callback(GtkWidget *widget,
                                   GdkEvent *event,
                                   GtkWidget *data)
{
    struct url *uri = (struct url *)data;

    if (ptr_to_gint(event) == GTK_RESPONSE_OK) {
        proto = gtk_combo_box_get_active(GTK_COMBO_BOX(uri->proto));
        if (proto < 0) proto = 0;
        g_key_file_set_integer(keyfile, "preferences", "ftpproto", proto);

        STRDUP(ftp_server, gtk_entry_get_text(GTK_ENTRY(uri->address)));
        g_key_file_set_string(keyfile, "preferences", "ftpserver", ftp_server);

        ftp_port = atoi(gtk_entry_get_text(GTK_ENTRY(uri->port)));
        g_key_file_set_integer(keyfile, "preferences", "ftpport", ftp_port);

        STRDUP(ftp_path, gtk_entry_get_text(GTK_ENTRY(uri->path)));
        g_key_file_set_string(keyfile, "preferences", "ftppath", ftp_path);

        STRDUP(proxy_host, gtk_entry_get_text(GTK_ENTRY(uri->proxy_address)));
        g_key_file_set_string(keyfile, "preferences", "ftpproxyaddress", proxy_host);

        proxy_port = atoi(gtk_entry_get_text(GTK_ENTRY(uri->proxy_port)));
        g_key_file_set_integer(keyfile, "preferences", "ftpproxyport", proxy_port);

        STRDUP(ftp_user, gtk_entry_get_text(GTK_ENTRY(uri->user)));
        g_key_file_set_string(keyfile, "preferences", "ftpuser", ftp_user);

        STRDUP(ftp_password, gtk_entry_get_text(GTK_ENTRY(uri->password)));
        g_key_file_set_string(keyfile, "preferences", "ftppassword", ftp_password);

        STRDUP(proxy_user, gtk_entry_get_text(GTK_ENTRY(uri->proxy_user)));
        g_key_file_set_string(keyfile, "preferences", "ftpproxyuser", proxy_user);

        STRDUP(proxy_password, gtk_entry_get_text(GTK_ENTRY(uri->proxy_password)));
        g_key_file_set_string(keyfile, "preferences", "ftpproxypassword", proxy_password);

        do_ftp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(uri->do_it));

        ftp_update = TRUE;

	if (progr_1w) gtk_label_set_text(GTK_LABEL(progr_1w), "OK");
	if (progr_2w) gtk_label_set_text(GTK_LABEL(progr_2w),
					 do_ftp ? "(starting)" : "(stopped)");
	return;
    }

    progr_1w = progr_2w = progr_3w = NULL;
    g_free(uri);
    gtk_widget_destroy(widget);
}

static gint timer_callback(gpointer data)
{
    static char *lines[4] = {"=___", "_=__", "__=_", "___="};
    static int i = 0;

    if (progr_1w == NULL || progr_2w == NULL || progr_3w == NULL)
	return FALSE;

    if (progr_file) {
	gtk_label_set_text(GTK_LABEL(progr_1w), progr_file);
	gtk_label_set_text(GTK_LABEL(progr_2w), progr_pros);
    } else {
	gtk_label_set_text(GTK_LABEL(progr_1w), "");
	gtk_label_set_text(GTK_LABEL(progr_2w), do_ftp ? lines[i & 3] : "");
	i = (i + 1) & 3;
    }

    if (do_ftp) {
	gchar buf[32];
	snprintf(buf, sizeof(buf), "OK: %d, FAIL: %d", xfer_ok, xfer_nok);
	gtk_label_set_text(GTK_LABEL(progr_3w), buf);
    }

    return TRUE;
}

void ftp_to_server(GtkWidget *w, gpointer data)
{
    gchar buf[128];
    GtkWidget *dialog, *hbox, *hbox1, *hbox2, *hbox3, *hbox4;
    struct url *uri = g_malloc0(sizeof(*uri));
    GtkWidget *proxy_lbl = gtk_label_new(_("Proxy:"));

    dialog = gtk_dialog_new_with_buttons (_("Server URL"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                          NULL);

    GETINT(proto, "ftpproto");
    uri->proto = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(uri->proto), NULL, "ftp");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(uri->proto), NULL, "http (put)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(uri->proto), NULL, "http (post)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(uri->proto), proto);

    GETSTR(ftp_server, "ftpserver");
    uri->address = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->address), ftp_server);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->address), 20);

    GETINT(ftp_port, "ftpport");
    sprintf(buf, "%d", ftp_port);
    uri->port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->port), buf);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->port), 4);

    GETSTR(ftp_path, "ftppath");
    uri->path = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->path), ftp_path);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->path), 16);

    GETSTR(proxy_host, "ftpproxyaddress");
    uri->proxy_address = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->proxy_address), proxy_host);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->proxy_address), 20);

    GETINT(proxy_port, "ftpproxyport");
    sprintf(buf, "%d", proxy_port);
    uri->proxy_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->proxy_port), buf);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->proxy_port), 4);

    GETSTR(ftp_user, "ftpuser");
    uri->user = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->user), ftp_user);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->user), 20);

    GETSTR(ftp_password, "ftppassword");
    uri->password = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->password), ftp_password);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->password), 20);

    GETSTR(proxy_user, "ftpproxyuser");
    uri->proxy_user = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->proxy_user), proxy_user);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->proxy_user), 20);

    GETSTR(proxy_password, "ftpproxypassword");
    uri->proxy_password = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(uri->proxy_password), proxy_password);
    gtk_entry_set_width_chars(GTK_ENTRY(uri->proxy_password), 20);

    uri->do_it = gtk_check_button_new_with_label(_("Copy to Server"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(uri->do_it), do_ftp);

    //gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

    hbox = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(hbox), uri->proto, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), gtk_label_new("://"), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), uri->address, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), gtk_label_new(":"), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), uri->port, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), gtk_label_new("/"), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), uri->path, NULL, GTK_POS_RIGHT, 1, 1);

    hbox2 = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(hbox2), gtk_label_new(_("User:")), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox2), uri->user, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox2), gtk_label_new(_("Password:")), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox2), uri->password, NULL, GTK_POS_RIGHT, 1, 1);

    hbox1 = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(hbox1), proxy_lbl, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox1), uri->proxy_address, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox1), gtk_label_new(":"), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox1), uri->proxy_port, NULL, GTK_POS_RIGHT, 1, 1);

    hbox3 = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(hbox3), gtk_label_new(_("Proxy User:")), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox3), uri->proxy_user, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox3), gtk_label_new(_("Proxy Password:")), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox3), uri->proxy_password, NULL, GTK_POS_RIGHT, 1, 1);

    hbox4 = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(hbox4), TRUE);
    progr_1w = gtk_label_new("");
    progr_2w = gtk_label_new("");
    progr_3w = gtk_label_new("");
    gtk_grid_attach_next_to(GTK_GRID(hbox4), progr_1w, NULL, GTK_POS_RIGHT, 1, 1);
    //gtk_grid_attach_next_to(GTK_GRID(hbox4), gtk_label_new("    "), NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox4), progr_2w, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox4), progr_3w, NULL, GTK_POS_RIGHT, 1, 1);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       uri->do_it, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox4, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(ftp_to_server_callback), uri);

    g_timeout_add_seconds(1, timer_callback, NULL);
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t retcode = fread(ptr, size, nmemb, stream);
    return retcode;
}

static int progress_callback(void *ptr,
			     curl_off_t tot_dl,
			     curl_off_t finished_dl,
			     curl_off_t tot_ul,
			     curl_off_t now_ul)
{
    if (tot_ul)
	snprintf(progr_pros, sizeof(progr_pros), "%d %%", (int)(now_ul*100/tot_ul));
    else
	progr_pros[0] = 0;
    return 0;
}

static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
    return progress_callback(p,
			     (curl_off_t)dltotal,
			     (curl_off_t)dlnow,
			     (curl_off_t)ultotal,
			     (curl_off_t)ulnow);
}

static void init_curl(CURL *curl)
{
    CURLcode err;

    /* verbose printing */
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* user & password */
    if (ftp_user && ftp_user[0]) {
        gchar pwd[64];
        snprintf(pwd, sizeof(pwd), "%s:%s", ftp_user,
                 ftp_password?ftp_password:"");
        if ((err = curl_easy_setopt(curl, CURLOPT_USERPWD, pwd)))
            ftp_log("Username/password option error: %s!", curl_easy_strerror(err));
    }

    /* proxy */
    if (proxy_host && proxy_host[0]) {
        if ((err = curl_easy_setopt(curl, CURLOPT_PROXY, proxy_host)))
            ftp_log("Proxy option error: %s!", curl_easy_strerror(err));
        if ((err = curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long)proxy_port)))
            ftp_log("Proxy port option error: %s!", curl_easy_strerror(err));
    }

    /* proxy user & password */
    if (proxy_user && proxy_user[0]) {
        gchar pwd[64];
        snprintf(pwd, sizeof(pwd), "%s:%s", proxy_user,
                 proxy_password?proxy_password:"");
        if ((err = curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, pwd)))
            ftp_log("Proxy user/password option error: %s!", curl_easy_strerror(err));
    }

    if (ftp_port) {
        if ((err = curl_easy_setopt(curl, CURLOPT_PORT, (long)ftp_port)))
            ftp_log("FTP port option error: %s!", curl_easy_strerror(err));
    }

    if ((err = curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)20)))
        ftp_log("Timeout option error: %s!", curl_easy_strerror(err));

#if 1
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);
#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);
#endif
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
#endif
}

int put_using_ftp(const char *fullname, const char *fname)
{
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;
    gchar *u;

    struct curl_slist *headerlist = NULL;

    /* get the file size of the local file */
    if (g_stat(fullname, &file_info)) {
	g_print("Couldnt open '%s': %s\n", fullname, strerror(errno));
	return 1;
    }
    fsize = (curl_off_t)file_info.st_size;

    /* get a FILE * of the same file */
    hd_src = fopen(fullname, "rb");
    if (!hd_src)
	return -1;

    /* In windows, this will init the winsock stuff */
    //curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
	progr_file = fname;

	init_curl(curl);

	/* build a list of commands to pass to libcurl */
	headerlist = curl_slist_append(headerlist, "RNFR TMPXYZ");
	gchar *n = g_strdup_printf("RNTO %s", fname);
	headerlist = curl_slist_append(headerlist, n);
	g_free(n);
	//n = g_strdup_printf("chmod 0644 %s", fname);
	//headerlist = curl_slist_append(headerlist, n);
	//g_free(n);

	/* we want to use our own read function */
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

	/* enable uploading */
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	/* specify target */
	if (ftp_path && ftp_path[0])
	    u = g_strdup_printf("ftp://%s/%s/TMPXYZ", ftp_server, ftp_path);
	else
	    u = g_strdup_printf("ftp://%s/TMPXYZ", ftp_server);

	curl_easy_setopt(curl, CURLOPT_URL, u);
	g_free(u);

	/* pass in that last of FTP commands to run after the transfer */
	curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

	/* now specify which file to upload */
	curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

	/* Set the size of the file to upload (optional).  If you give a *_LARGE
	   option you MUST make sure that the type of the passed-in argument is a
	   curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
	   make sure that to pass in a type 'long' argument. */
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
			 (curl_off_t)fsize);

	/* Now run off and do what you've been told! */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
	    xfer_nok++;
	    snprintf(progr_pros, sizeof(progr_pros), "%s", curl_easy_strerror(res));
	    usleep(2000000);
	} else xfer_ok++;

	/* clean up the FTP commands list */
	curl_slist_free_all(headerlist);

	/* always cleanup */
	curl_easy_cleanup(curl);

	progr_file = NULL;
    }

    fclose(hd_src); /* close the local file */
    //curl_global_cleanup();
    return 0;
}

int put_using_post(const char *fullname, const char *fname)
{
    CURL *curl;
    CURLcode res;
    gchar *u;

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";

    //curl_global_init(CURL_GLOBAL_ALL);

    /* Fill in the file upload field */
    curl_formadd(&formpost,
		 &lastptr,
		 CURLFORM_COPYNAME, "file",
		 CURLFORM_FILE, fullname,
		 CURLFORM_END);

    /* Fill in the submit field too, even if this is rarely needed */
    curl_formadd(&formpost,
		 &lastptr,
		 CURLFORM_COPYNAME, "submit",
		 CURLFORM_COPYCONTENTS, "send",
		 CURLFORM_END);

    curl = curl_easy_init();
    /* initalize custom header list (stating that Expect: 100-continue is not
       wanted */
    headerlist = curl_slist_append(headerlist, buf);
    if (curl) {
	progr_file = fname;

	init_curl(curl);

	/* what URL that receives this POST */
	if (ftp_path && ftp_path[0])
	    u = g_strdup_printf("http://%s/%s", ftp_server, ftp_path);
	else
	    u = g_strdup_printf("http://%s", ftp_server);

	curl_easy_setopt(curl, CURLOPT_URL, u);
	g_free(u);

	/* only disable 100-continue header if explicitly requested */
	//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
	    xfer_nok++;
	    snprintf(progr_pros, sizeof(progr_pros), "%s", curl_easy_strerror(res));
	    usleep(2000000);
	} else xfer_ok++;

	/* always cleanup */
	curl_easy_cleanup(curl);

	/* then cleanup the formpost chain */
	curl_formfree(formpost);
	/* free slist */
	curl_slist_free_all(headerlist);

	progr_file = NULL;
    }

    return 0;
}

int put_using_put(const char *fullname, const char *fname)
{
    CURL *curl;
    CURLcode res;
    FILE * hd_src;
    struct stat file_info;
    gchar *u;

    /* get the file size of the local file */
    g_stat(fullname, &file_info);

    /* get a FILE * of the same file, could also be made with
       fdopen() from the previous descriptor, but hey this is just
       an example! */
    hd_src = fopen(fullname, "rb");
    if (!hd_src)
	return -1;

    /* In windows, this will init the winsock stuff */
    //curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
	progr_file = fname;

	init_curl(curl);

	/* we want to use our own read function */
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

	/* enable uploading */
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	/* HTTP PUT please */
	curl_easy_setopt(curl, CURLOPT_PUT, 1L);

	/* specify target URL, and note that this URL should include a file
	   name, not only a directory */
	if (ftp_path && ftp_path[0])
	    u = g_strdup_printf("http://%s/%s/%s", ftp_server, ftp_path, fname);
	else
	    u = g_strdup_printf("http://%s/%s", ftp_server, fname);

	curl_easy_setopt(curl, CURLOPT_URL, u);
	g_free(u);

	/* now specify which file to upload */
	curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

	/* provide the size of the upload, we specicially typecast the value
	   to curl_off_t since we must be sure to use the correct data size */
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
			 (curl_off_t)file_info.st_size);

	/* Now run off and do what you've been told! */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
	    xfer_nok++;
	    snprintf(progr_pros, sizeof(progr_pros), "%s", curl_easy_strerror(res));
	    usleep(2000000);
	} else xfer_ok++;

	/* always cleanup */
	curl_easy_cleanup(curl);

	progr_file = NULL;
    }
    fclose(hd_src); /* close the local file */

    //curl_global_cleanup();
    return 0;
}

gpointer ftp_thread(gpointer args)
{
    CURLcode res;


    ftp_log("Starting FTP");

    while ((res = curl_global_init(CURL_GLOBAL_DEFAULT))) {
        ftp_log("Cannot init curl: %s!", curl_easy_strerror(res));
        g_usleep(120000000);
    }

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        time_t last_copy = 0;
        ftp_update = FALSE;
	xfer_ok = xfer_nok = 0;

        while (!do_ftp || !ftp_server || !ftp_server[0] ||
               !current_directory[0] || current_directory[0] == '.') {
            g_usleep(1000000);
        }

        ftp_log("Server=%s directory=%s", ftp_server, current_directory);

        while (!ftp_update) {
            GDir *dir = g_dir_open(current_directory, 0, NULL);
            if (dir) {
		time_t copy_start = time(NULL);
                const gchar *fname = g_dir_read_name(dir);
                while (fname && !ftp_update) {
                    struct stat statbuf;
                    gchar *fullname = g_build_filename(current_directory, fname, NULL);
                    if (!g_stat(fullname, &statbuf)) {
                        if (statbuf.st_mtime >= last_copy &&
                            (statbuf.st_mode & S_IFREG)) {
			    if (proto == PROTO_FTP) {
				put_using_ftp(fullname, fname);
			    } else if (proto == PROTO_RFC1738) {
				put_using_post(fullname, fname);
			    } else {
				put_using_put(fullname, fname);
                            } // if (proto)
                        } // if mtime >
                    } // if (!g_stat)
                    g_free(fullname);
                    fname = g_dir_read_name(dir);
                    g_usleep(1000);
                } // while fname

                g_dir_close(dir);
                last_copy = copy_start;
            } // if (dir)
            g_usleep(2000000);
        } // while (!ftp_update)
        ftp_log("Configuration update.");
    } // main loop

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}

static gchar *ftp_logfile_name = NULL;

static void ftp_log(gchar *format, ...)
{
    time_t t;
    gchar buf[256];
    va_list args;
    va_start(args, format);
    gchar *text = g_strdup_vprintf(format, args);
    va_end(args);

    t = time(NULL);

    if (ftp_logfile_name == NULL) {
        struct tm *tm = localtime(&t);
        if (tm) {
            sprintf(buf, "judoftp_%04d%02d%02d_%02d%02d%02d.log",
                    tm->tm_year+1900,
                    tm->tm_mon+1,
                    tm->tm_mday,
                    tm->tm_hour,
                    tm->tm_min,
                    tm->tm_sec);
            ftp_logfile_name = g_build_filename(g_get_user_data_dir(), buf, NULL);
        }
    }

    FILE *f = fopen(ftp_logfile_name, "a");
    if (f) {
        struct tm *tm = localtime(&t);

        fprintf(f, "%02d:%02d:%02d %s\n",
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec,
                text);
        fclose(f);
    } else {
        g_print("Cannot open ftp log file\n");
        perror("ftp_logfile_name");
    }

    g_free(text);
}
