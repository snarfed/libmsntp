/**
 * libmsntp
 * http://snarfed.org/libmsntp
 *
 * Copyright 2005, Ryan Barrett <libmsntp@ryanb.org>
 *
 * libmsntp is a shared library that provides SNTP client and server
 * functionality. It allows you to embed an SNTP client and/or server in your
 * own programs. It's implemented on top of msntp, a command-line SNTP tool
 * from the HPCF group at Cambridge University.
 *
 * To use libmsntp in your own programs, include libmsntp.h and link with
 * libmsntp.a. For more information on building libmsntp, see the README file.
 */

#include "libmsntp.h"
#include "header.h"

#include <sys/time.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

extern int errno;


/* defined in main.c */
extern int operation, verbose, action, period, count, delay, waiting;
extern double minerr, maxerr, prompt, dispersion;

extern int run_client(char *hostnames[], int nhosts, double *offset);
extern int run_server();

/* globals */
int libmsntp_port;  /* used by internet.c; not assumed to be 16 bits */
int libmsntp_errno;
const char *libmsntp_strerror;


/* helper functions */

/**
 * Validates the hostname, stores the port, and initializes a number of msntp
 * settings.
 */
void setup(char *hostname, int port) {
    assert(hostname && strlen(hostname) > 0);
    libmsntp_port = port;

    action = 0;
    minerr = 0.1;
    maxerr = 5.0;
    count = 5;
    delay = 15;
    waiting = delay / count;
    prompt = (double)INT_MAX;
    verbose = 0;
}

/**
 * Takes a double parameter representing seconds since the epoch and returns
 * the corresponding timeval. The parameter may have a fractional part.
 */
struct timeval convert_timeval(double value) {
    struct timeval tv;
    int millisecs;

    tv.tv_sec = convert_time(value, &millisecs);
    tv.tv_usec = millisecs;
    return tv;
}


/* public functions */
int msntp_set_clock(char *hostname, int port) {
    int ret;
    double offset;

    setup(hostname, port);
    operation = op_client;

    if (ret = run_client(&hostname, 1, &offset))
        return ret;
    return adjust_time(offset, 1, 0);
}

int msntp_get_offset(char *hostname, int port, struct timeval *offset) {
    int ret;
    double offset_d;

    setup(hostname, port);
    operation = op_client;

    if (ret = run_client(&hostname, 1, &offset_d))
        return ret;
    *offset = convert_timeval(offset_d);
    return 0;
}

int msntp_get_time(char *hostname, int port, struct timeval *server_time) {
    int ret;
    double offset;

    setup(hostname, port);
    operation = op_client;

    if (ret = run_client(&hostname, 1, &offset))
        return ret;
    *server_time = convert_timeval(current_time(offset));
    return 0;
}

int msntp_start_server(int port) {
    setup("unused", port);
    operation = op_server;
    return open_socket(0, NULL, delay);
}

int msntp_serve() {
    operation = op_server;
    return run_server();
}

int msntp_stop_server (void) {
    return close_socket(0);
}
    
const char *msntp_strerror() {
    if (libmsntp_errno < 0) {
        return libmsntp_strerror;
    } else {
        return strerror(libmsntp_errno);
    }
}



