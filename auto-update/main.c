/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2019 by Hannu Jokinen
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
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#endif

#include "sha.h"

#ifndef WIN32
#define closesocket     close
#define SOCKET          int
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define SOCKADDR_BTH    struct sockaddr_rc
#define AF_BTH          PF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM
#endif

#ifdef WIN32

#define DIRSEP '\\'

#else // !WIN32

#define DIRSEP '/'

#endif // WIN32

int create_window(void) { return 0; }
int update_window(char *fmt, ...) { return 0; };
void handle_msgs(void) { }

#define TMP_FILE "saved.tmp"
static FILE *logfile = NULL;

static char filebuf[2048];
static char recvbuf[2048];
static char buf[1024], buf2[1024];
static char *instdir = NULL;

static struct fileinfo {
    char filename[128];
    long start;
    long len;
} *files = NULL;
static int filecnt = 0, filespace = 0, totfiles = 0;

static long tmp_file_len = 0;

#define RETURN(_code) \
    do { if (_code) log("Return with code %d (line %d)\n", _code, __LINE__); \
	return (_code); } while (0)

#define log(fmt, ...) do { printf(fmt, ##__VA_ARGS__);		\
	if (logfile) fprintf(logfile,  fmt, ##__VA_ARGS__);	\
    } while (0)

static int closesock(int s)
{
#if defined(__WIN32__) || defined(WIN32)
    shutdown(s, SD_SEND);
    return closesocket(s);
#else
    return close(s);
#endif
}

int process_exists(int id)
{
#if defined(__WIN32__) || defined(WIN32)
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, id);
    if (hProcess != NULL) {
	//TerminateProcess(hProcess, 9);
	CloseHandle(hProcess);
	return 1;
    }
#endif
    return 0;
}

static void set_progress(int pr)
{
    FILE *progrfile = fopen("progr.txt", "w");
    if (progrfile) {
	fprintf(progrfile, "%d", pr);
	fclose(progrfile);
    }
}

int rename_auto_update(void)
{
#if defined(__WIN32__) || defined(WIN32)
    snprintf(buf, sizeof(buf), "%s\\bin\\auto-update.exe", instdir);
    snprintf(buf2, sizeof(buf2), "%s\\bin\\auto-update-1.exe", instdir);

    if (GetFileAttributes(buf2) != INVALID_FILE_ATTRIBUTES) {
	log("Delete old auto-update-1.exe\n");
	if (!DeleteFile(buf2)) {
	    log("Cannot delete %s\n", buf2);
	    return -1;
	}
    }

    if (!MoveFile(buf, buf2)) {
	int err = GetLastError();
	log("---------------------------\n");
	log("Cannot change auto-update.exe name (err=%d)!\n", err);
	log("%s -> %s\n", buf, buf2);
	log("---------------------------\n");
	return -1;
    }

#endif
    return 0;
}

int restore_auto_update(void)
{
#if defined(__WIN32__) || defined(WIN32)
    snprintf(buf, sizeof(buf), "%s\\bin\\auto-update.exe", instdir);
    snprintf(buf2, sizeof(buf2), "%s\\bin\\auto-update-1.exe", instdir);

    if (GetFileAttributes(buf) != INVALID_FILE_ATTRIBUTES)
	return -1;

    if (!MoveFile(buf2, buf)) {
	int err = GetLastError();
	log("---------------------------\n");
	log("Cannot change auto-update-1.exe name (err=%d)!\n", err);
	log("%s -> %s\n", buf2, buf);
	log("---------------------------\n");
	return -1;
    }

#endif
    return 0;
}

static FILE *open_file_w(char *file)
{
    log("%s\n", file);
    FILE *outfile = fopen(file, "wb");
    // if directory doesn't exist create it
    if (!outfile) {
	strcpy(filebuf, file);
	char *p1 = filebuf;
	while (p1) {
	    p1 = strchr(p1+1, DIRSEP);
	    if (p1) {
		*p1 = 0;
#ifdef WIN32
		mkdir(filebuf);
#else
		mkdir(filebuf, 0777);
#endif
		*p1 = DIRSEP;
	    }
	}
	outfile = fopen(file, "wb");
	if (!outfile)
	    log("Cannot write to file %s\n", file);
    }
    return outfile;
}

static int get_file(struct in_addr addr, char *name, long length)
{
    struct sockaddr_in node;
    SOCKET comm_fd;
    FILE *f;
    int err = 0;

    if (name == NULL || name[0] == 0)
	RETURN(-1);

    if (!files) {
	filespace = 1024;
	files = malloc(filespace*sizeof(struct fileinfo));

	f = fopen(TMP_FILE, "wb");
	if (!f) {
	    perror("temporary file");
	    RETURN(-1);
	}
	tmp_file_len = 0;
    } else {
	f = fopen(TMP_FILE, "ab");
	if (!f) {
	    perror("temporary file");
	    RETURN(-1);
	}
    }

    if (filecnt >= filespace) {
	filespace *= 2;
	files = realloc(files, filespace);
	//log("REALLOC, new size=%d\n", filespace);
    }

    if ((comm_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
	perror("socket");
	fclose(f);
	RETURN(-1);
    }

    memset(&node, 0, sizeof(node));
    node.sin_family      = AF_INET;
    node.sin_port        = htons(2308);
    node.sin_addr.s_addr = addr.s_addr;

    if (connect(comm_fd, (struct sockaddr *)&node, sizeof(node))) {
	perror("get_file: connect");
	closesock(comm_fd);
	fclose(f);
	RETURN(-1);
    }

    //files[filecnt].filename = strdup(name);
    strncpy(files[filecnt].filename, name, sizeof(files[filecnt].filename)-1);
    files[filecnt].filename[sizeof(files[filecnt].filename)-1] = 0;
    send(comm_fd, name, strlen(name), 0);

    files[filecnt].start = tmp_file_len;
    long tot = 0;
    while (1) {
	int n = recv(comm_fd, filebuf, sizeof(filebuf), 0);
	if (n <= 0)
	    break;
	fwrite(filebuf, 1, n, f);
	tmp_file_len += n;
	tot += n;
    }
    closesock(comm_fd);

    files[filecnt].len = tot;
    if (files[filecnt].len == length) {
	filecnt++;
	set_progress(filecnt);
    } else {
	log("ERROR: %s transfer corrupted: expected len=%ld, got %ld, tot=%ld\n",
	    name, length, files[filecnt].len, tot);
	err = -1;
    }
    fclose(f);

    if (err) RETURN(err);
    return err;
}

int save_files(void)
{
    FILE *tmp_file = fopen(TMP_FILE, "rb");
    if (!tmp_file) {
	perror("temporary file");
	RETURN(-1);
    }

    log("---------------------------\n");
    log("Save files\n");
    log("---------------------------\n");
    if (files) {
	int i;

	rename_auto_update();

	for (i = 0; i < filecnt; i++) {
	    if (!files[i].filename[0])
		continue;
	    log("File %s\n", files[i].filename);

	    FILE *outfile = fopen(files[i].filename, "wb");
	    // if directory doesn't exist create it
	    if (!outfile) {
		strcpy(filebuf, files[i].filename);
		char *p1 = filebuf;
		while (p1) {
		    p1 = strchr(p1+1, DIRSEP);
		    if (p1) {
			*p1 = 0;
			#ifdef WIN32
			mkdir(filebuf);
			#else
			mkdir(filebuf, 0777);
			#endif
			*p1 = DIRSEP;
		    }
		}
		outfile = fopen(files[i].filename, "wb");
		if (!outfile) {
		    log("Cannot write to file %s\n", files[i].filename);
		    if (!strstr(files[i].filename, "auto-update"))
			RETURN(-1);
		    else
			continue;
		}
	    }

	    fseek(tmp_file, files[i].start, SEEK_SET);
	    long len = files[i].len;
	    while (len > 0) {
		int n = fread(filebuf, 1,
			      len > sizeof(filebuf) ? sizeof(filebuf) : len,
			      tmp_file);
		if (n < 0) {
		    fclose(tmp_file);
		    fclose(outfile);
		    RETURN(-1);
		}

		fwrite(filebuf, 1, n, outfile);
		len -= n;
	    }
	    fclose(outfile);

	    //free(files[i].filename);
	    //files[i].filename = NULL;
	} // for

	free(files);
    }

    fclose(tmp_file);
    return 0;
}

#define ERRMSG(_test, _str) \
    do { if (_test) { log(_str); closesock(comm_fd); RETURN(-1); }} while (0)



int compare_file(char *file, long length, char *cksum)
{
    int i, n = 0;
    SHA_CTX context;
    sha1_byte digest[SHA1_DIGEST_LENGTH];
    struct stat st;

    if (stat(file, &st)) {
	log("File %s doesn't exist\n", file);
	return -1;
    }

    if (length != st.st_size) {
	log("File %s length changed\n", file);
	return -1;
    }

    FILE *f = fopen(file, "rb");
    if (!f) {
	log("File %s doesn't exist\n", file);
	return -1;
    }

    memset(digest, 0, sizeof(digest));
    SHA1_Init(&context);
    while ((n = fread(buf2, 1, sizeof(buf2), f)) > 0)
	SHA1_Update(&context, (unsigned char *)buf2, n);
    SHA1_Final(digest, &context);

    fclose(f);

    n = 0;
    for (i = 0; i < SHA1_DIGEST_LENGTH; i++)
	n += snprintf(buf2+n, sizeof(buf2)-n, "%02x", digest[i]);
    buf2[n] = 0;

    if ((n = strcmp(buf2, cksum)))
	log("File %s changed\n", file);

    return n;
}


int main(int argc, char *argv[])
{
    struct in_addr addr;
    SOCKET comm_fd;
    struct sockaddr_in node;
    fd_set read_fd, fds;
    long filelen = 0, totrec = 0;
    char *cksum_s, *len_s, *fname;
    int err = 0;
    int i;
    char *app = NULL;
    int pid = 0;
    int all = 0;
    FILE *outfile = NULL;
    int ix;

    if (argc < 3) RETURN(-1);

    instdir = argv[2];
    if (argc > 3) {
	if (!strcmp(argv[3], "-all")) {
	    all = 1;
	} else
	    app = argv[3];
    }
    if (argc > 4)
	pid = atoi(argv[4]);

    chdir(argv[2]);

    logfile = fopen("log.txt", "w");

    log("Command line:\n");
    for (i = 0; i < argc; i++)
	log("  '%s'\n", argv[i]);
    log("\n---------------------------\n");
    if (all) log("Get files\n");
    else log("Check files\n");
    log("---------------------------\n");

    if ((addr.s_addr = inet_addr(argv[1])) == -1) RETURN(-1);

#ifdef WIN32
    AllocConsole();
    freopen("CON", "w", stdout);

    WSADATA WSAData = {0};

    if (0 != WSAStartup(MAKEWORD(2, 2), &WSAData)) {
        log("-FATAL- | Unable to initialize Winsock version 2.2\n");
        RETURN(-1);
    }
#endif

    if ((comm_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
	perror("socket");
	RETURN(-1);
    }

    memset(&node, 0, sizeof(node));
    node.sin_family      = AF_INET;
    node.sin_port        = htons(all ? 2308 : 2309);
    node.sin_addr.s_addr = addr.s_addr;

    if (connect(comm_fd, (struct sockaddr *)&node, sizeof(node))) {
	perror("connect");
	closesock(comm_fd);
	RETURN(-1);
    }

    FD_ZERO(&read_fd);
    FD_SET(comm_fd, &read_fd);

    int state = 0;

    if (all) {
	rename_auto_update();
	send(comm_fd, "ALL", 3, 0);
    }

    while (1) {
	struct timeval timeout;
	int r, n;

	fds = read_fd;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	r = select(comm_fd+1, &fds, NULL, NULL, &timeout);
	if (r <= 0)
	    continue;

	if (FD_ISSET(comm_fd, &fds)) {
	    n = recv(comm_fd, recvbuf, sizeof(recvbuf), 0);
	    if (n <= 0) {
		closesock(comm_fd);
		if (outfile) fclose(outfile);
		if (pid) {
		    log("WAITING %s TO QUIT\n", app);
		    while (process_exists(pid)) {
			usleep(1000000);
		    }
		}
		if (!all) save_files();
		restore_auto_update();
		set_progress(1000000);

		if (0 && app) {
#ifdef WIN32
		    ShellExecute(NULL, NULL, app, NULL, NULL, SW_SHOWNORMAL);
#else
		    snprintf(buf, sizeof(buf), "%s &", app);
		    system(buf);
#endif
		}
		RETURN(0);
	    }

	    if (state == 0) {
		int is_file = n > 10 && strncmp(recvbuf, "FILE", 4) == 0;
		int is_name = n > 10 && strncmp(recvbuf, "NAME", 4) == 0;
		char *bin = NULL;

		if (is_file || is_name) {
		    for (ix = 0; ix < n; ix++) {
			if (recvbuf[ix] == 0 && state < 4) {
			    switch (state) {
			    case 0:	cksum_s = &recvbuf[ix+1]; break;
			    case 1:	len_s = &recvbuf[ix+1]; break;
			    case 2:	fname = &recvbuf[ix+1]; break;
			    case 3:	if (ix < n-1) bin = &recvbuf[ix+1]; break;
			    }
			    state++;
			    if (state == 4) break;
			}
		    }
		    totrec = 0;
		} else {
		    log("Error in receiving!\n");
		    break;
		}

		filelen = atol(len_s);

		if (!all) {
		    state = 0;
		    if (compare_file(fname, filelen, cksum_s)) {
			log("GET %s\n", fname);
			err = get_file(addr, fname, filelen);
			send(comm_fd, "+", 1, 0);
			if (err) {
			    log("Cannot update!\n");
			    break;
			}
		    } else {
			send(comm_fd, "+", 1, 0);
		    }
		} else {
		    outfile = open_file_w(fname);
		    if (!outfile) break;
		    if (bin) {
			totrec = n - ix - 1;
			fwrite(bin, 1, totrec, outfile);
		    }
		    set_progress(++totfiles);
		}
	    } else { // state != 0
		if (totrec + n >= filelen) {
		    fwrite(recvbuf, 1, filelen-totrec, outfile);
		    fclose(outfile);
		    outfile = NULL;
		    state = 0;
		    send(comm_fd, "+", 1, 0);
		} else {
		    fwrite(recvbuf, 1, n, outfile);
		    totrec += n;
		}
	    }
	}
    }

    closesock(comm_fd);

    RETURN(0);
}
