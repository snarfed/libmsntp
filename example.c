/**
 * libmsntp
 * http://snarfed.org/libmsntp
 *
 * Copyright 2005, Ryan Barrett <libmsntp@ryanb.org>
 *
 * This is an example program that uses the libmsntp library.
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define USAGE "Usage: example [HOSTNAME] PORT\n" \
  "If hostname is specified, connects to an SNTP server and prints the \n" \
  "server's time and the difference from the local clock. Otherwise, runs \n" \
  "an SNTP server on the specified port.\n\n"

struct tm to_struct_tm(struct timeval tv) {
  return *gmtime((time_t *) &tv.tv_sec);
}

int serve(int port) {
  int ret;

  ret = msntp_start_server(port);
  if (ret)
    return ret;

  printf("Listening for SNTP clients on port %d...", port);

  while (1) {
    ret = msntp_serve();
    if (ret > 0 || ret < -1)
      return ret;
  }

  msntp_stop_server();
}

int get(char *hostname, int port) {
  struct timeval tv;
  struct tm tm;
  int err;

  err = msntp_get_time(hostname, port, &tv);
  if (err)
    return err;

  tm = to_struct_tm(tv);
  printf(asctime(&tm));
  return 0;
}

int main(int argc, char **argv) {
  int port, ret;
  char *portstr;

  if (argc == 1 || argc > 3 ||
      !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    fprintf(stderr, USAGE);
    exit(1);
  }

  if (sscanf(argv[argc - 1], "%d", &port) != 1) {
    fprintf(stderr, "Invalid port: %s\n", argv[argc - 1]);
    exit(-1);
  }
  assert(port <= 65535);

  if (argc == 2) {
    ret = serve(port);
  } else {
    assert(argc == 3);
    ret = get(argv[1], port);
  }

  if (ret > 0 || ret < -1) {
    fprintf(stderr, msntp_strerror());
  }
  return ret;
}
