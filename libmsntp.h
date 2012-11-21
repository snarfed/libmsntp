/**
 * libmsntp
 * http://snarfed.org/libmsntp
 *
 * Copyright 2005, Ryan Barrett <libmsntp@ryanb.org>
 *
 * libmsntp is a shared library that provides SNTP client and server
 * functionality. It allows you to embed an SNTP client and/or server in your
 * own programs. It's implemented on top of msntp, a command-line SNTP tool
 * from Cambridge.
 *
 * Errors can be handled programmatically by examing each function's return
 * value - if positive, it's an errno constant; if negative, it's one of the
 * EMSNTP_ constants defined immediately below. The msntp_strerror function
 * returns a human-readable string describing the last error encountered.
 *
 * To use libmsntp in your own programs, include libmsntp.h and link with
 * libmsntp.a. For more information on building libmsntp, see the README file.
 */

#ifndef _LIBMSNTP_H
#define _LIBMSNTP_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif 


/**
 * Error constants, used as return values from the libmsntp functions.
 */
#define EMSNTP_UNKNOWN               -10
#define EMSNTP_INTERNAL              -11
#define EMSNTP_IP_ADDRESS            -12
#define EMSNTP_AF_INET               -13
#define EMSNTP_TOO_FEW_RESPONSES     -14
#define EMSNTP_BAD_RESPONSES         -15
#define EMSNTP_NO_GOOD_RESPONSE      -16
#define EMSNTP_NTP_INCONSISTENCY     -17
#define EMSNTP_NTP_INSANITY          -18


/**
 * Connects to an SNTP server and synchronizes the local clock to the server's
 * clock.
 *
 * The port should be in host byte order.
 */
int msntp_set_clock(char *hostname, int port);

/**
 * Connects to an SNTP server and returns the difference between its clock and
 * the local clock, as a struct timeval. If positive, the server clock is ahead
 * of the local clock; if negative, the server clock is behind the local clock.
 *
 * The port should be in host byte order.
 */
int msntp_get_offset(char *hostname, int port, struct timeval *offset);

/**
 * Connects to an SNTP server and returns its current time. Since the value of
 * this is largely based on how quickly you can use it, it's recommended that
 * you use msntp_set_clock or msntp_get_offset instead.
 *
 * The port should be in host byte order.
 */
int msntp_get_time(char *hostname, int port, struct timeval *server_time);

/**
 * Starts the SNTP server. The port should be in host byte order.
 */
int msntp_start_server(int port);

/**
 * Handles incoming conections from SNTP clients. This call is non-blocking; it
 * will either accept and handle a single SNTP request, or time out and return.
 * Should only be called after msntp_start_server.
 */
int msntp_serve();

/**
 * Stops the SNTP server. Should only be called after msntp_start_server.
 */
int msntp_stop_server();

/**
 * Returns a a detailed, human-readable string describing the last error
 * encountered.
 */
const char *msntp_strerror();

#ifdef __cplusplus
}
#endif 

#endif //_LIBMSNTP_H
