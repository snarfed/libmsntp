/*  Copyright (C) 1996, 2000 N.M. Maclaren
    Copyright (C) 1996, 2000 The University of Cambridge

This includes all of the code needed to handle Berkeley sockets.  It is way
outside current POSIX, unfortunately.  It should be easy to convert to a system
that uses another mechanism.  It does not currently use socklen_t, because
the only system that the author uses that has it is Linux. */



#include "header.h"
#include "internet.h"
#include <fcntl.h>
#include <errno.h>

#define SOCKET
#include "kludges.h"
#undef SOCKET



/* The code needs to set some variables during the open, for use by later
functions. */

static int initial = 1,
    descriptors[MAX_SOCKETS];
static struct sockaddr_in here[MAX_SOCKETS], there[MAX_SOCKETS];



void display_in_hex (const void *data, int length) {
    int i;

    for (i = 0; i < length; ++i)
        fprintf(stderr,"%.2x",((const unsigned char *)data)[i]);
}



int open_socket (int which, char *hostname, int timespan) {

/* Locate the specified NTP server, set up a couple of addresses and open a
socket. */

    int port, k;
    struct in_addr address, anywhere, everywhere;

/* Initialise and find out the server and port number.  Note that the port
number is in network format. */

    if (initial) for (k = 0; k < MAX_SOCKETS; ++k) descriptors[k] = -1;
    initial = 0;
    if (which < 0 || which >= MAX_SOCKETS || descriptors[which] >= 0) {
        fatal(EMSNTP_INTERNAL,"socket index out of range or already open",NULL);
        return EMSNTP_INTERNAL;
    }
    if (verbose > 2) fprintf(stderr,"Looking for the socket addresses\n");
    find_address(&address,&anywhere,&everywhere,&port,hostname,timespan);
    if (verbose > 2) {
        fprintf(stderr,"Internet address: address=");
        display_in_hex(&address,sizeof(struct in_addr));
        fprintf(stderr," anywhere=");
        display_in_hex(&anywhere,sizeof(struct in_addr));
        fprintf(stderr," everywhere=");
        display_in_hex(&everywhere,sizeof(struct in_addr));
        fputc('\n',stderr);
    }

/* Set up our own and the target addresses.  Note that the target address will
be reset before use in server mode. */

    memset(&here[which],0,sizeof(struct sockaddr_in));
    here[which].sin_family = AF_INET;
    here[which].sin_port =
        (operation == op_listen || operation == op_server ? port : 0);
    here[which].sin_addr = anywhere;
    memset(&there[which],0,sizeof(struct sockaddr_in));
    there[which].sin_family = AF_INET;
    there[which].sin_port = port;
    there[which].sin_addr = (operation == op_broadcast ? everywhere : address);
    if (verbose > 2) {
        fprintf(stderr,"Initial sockets: here=");
        display_in_hex(&here[which].sin_addr,sizeof(struct in_addr));
        fputc('/',stderr);
        display_in_hex(&here[which].sin_port,sizeof(here[which].sin_port));
        fprintf(stderr," there=");
        display_in_hex(&there[which].sin_addr,sizeof(struct in_addr));
        fputc('/',stderr);
        display_in_hex(&there[which].sin_port,sizeof(there[which].sin_port));
        fputc('\n',stderr);
    }

/* Allocate a local UDP socket and configure it. */

    errno = 0;
    if ((descriptors[which] = socket(AF_INET,SOCK_DGRAM,0)) < 0 ||
            bind(descriptors[which],(struct sockaddr *)&here[which],
                    sizeof(here[which]))  < 0) {
        fatal(errno,"unable to allocate socket for NTP",NULL);
        return errno;
    }
    if (operation == op_broadcast) {
        errno = 0;
        k = setsockopt(descriptors[which],SOL_SOCKET,SO_BROADCAST,
                (void *)&k,sizeof(k));
        if (k != 0) {
            fatal(errno,"unable to set permission to broadcast",NULL);
            return errno;
        }
    }

    return 0;
}



extern int write_socket (int which, void *packet, int length) {

/* Any errors in doing this are fatal - including blocking.  Yes, this leaves a
server vulnerable to a denial of service attack. */

    int k;

    if (which < 0 || which >= MAX_SOCKETS || descriptors[which] < 0) {
        fatal(EMSNTP_INTERNAL,"socket index out of range or not open",NULL);
        return EMSNTP_INTERNAL;
    }
    errno = 0;
    k = sendto(descriptors[which],packet,(size_t)length,0,
            (struct sockaddr *)&there[which],sizeof(there[which]));
    if (k != length) {
        fatal(errno,"unable to send NTP packet",NULL);
        return errno;
    }

    return 0;
}



extern int read_socket (int which, void *packet, int length, int waiting,
                        int *written) {

/* Read a packet and returns (in a parameter) the number of bytes written. Only
incorrect length and timeout are not fatal. Note that in msntp, this used
SIGALRM to handle timeouts, but in libmsntp, it uses select(). Also, a timeout
is only set in client mode; in server mode, read_socket is non-blocking. */

    struct sockaddr_in scratch, *ptr;
    int n;
    int k;
    int ret;
    struct timeval timeout = { 0, 0 };
    fd_set fd;

    *written = 0;
    if (operation == op_client) {
      timeout.tv_sec = waiting;
    }

/* Under normal circumstances, select on the socket for the given timeout. */

    if (which < 0 || which >= MAX_SOCKETS || descriptors[which] < 0) {
        fatal(EMSNTP_INTERNAL,"socket index out of range or not open",NULL);
        return EMSNTP_INTERNAL;
    }

    FD_ZERO(&fd);
    FD_SET(descriptors[which], &fd);

    ret = select(descriptors[which] + 1, &fd, NULL, NULL, &timeout);

    if (ret == 0) {
        if (verbose > 2)
          fprintf(stderr,"Receive timed out\n");
        else if (verbose > 1)
          fprintf(stderr,"%s: receive timed out after %d seconds\n",
                  argv0,waiting);
        errno = 0;
        return -1;
    } else if (ret < 0) {
        if (verbose > 1)
          fprintf(stderr,"select returned error: %s", strerror(errno));
        return -1;
    }

/* select returned 1, so we have a packet waiting. get the packet and clear the
   timeout, if any.  */

    if (operation == op_server)
        memcpy(ptr = &there[which],&here[which],sizeof(struct sockaddr_in));
    else
        memcpy(ptr = &scratch,&there[which],sizeof(struct sockaddr_in));
    n = sizeof(struct sockaddr_in);
    errno = 0;
    k = recvfrom(descriptors[which],packet,(size_t)length,0,
        (struct sockaddr *)ptr,&n);

/* Now issue some low-level diagnostics. */

    if (k <= 0) {
        fatal(errno,"unable to receive NTP packet from server",NULL);
        return errno;
    }
    if (verbose > 2) {
        fprintf(stderr,"Packet of length %d received from ",k);
        display_in_hex(&ptr->sin_addr,sizeof(struct in_addr));
        fputc('/',stderr);
        display_in_hex(&ptr->sin_port,sizeof(ptr->sin_port));
        fputc('\n',stderr);
    }

    *written = k;
    return 0;
}



extern int flush_socket (int which, int *count) {

/* Get rid of any outstanding input, because it may have been hanging around
for a while.  Ignore packet length oddities and return the number of packets
skipped. */

    struct sockaddr_in scratch;
    int n;
    char buffer[256];
    int flags, total = 0, k;

    *count = 0;

/* The code is the obvious. */

    if (which < 0 || which >= MAX_SOCKETS || descriptors[which] < 0) {
        fatal(EMSNTP_INTERNAL,"socket index out of range or not open",NULL);
        return EMSNTP_INTERNAL;
    }
    if (verbose > 2) fprintf(stderr,"Flushing outstanding packets\n");
    errno = 0;
    if ((flags = fcntl(descriptors[which],F_GETFL,0)) < 0 ||
        fcntl(descriptors[which],F_SETFL,flags|O_NONBLOCK) == -1) {
        fatal(errno,"unable to set non-blocking mode",NULL);
        return errno;
    }
    while (1) {
        n = sizeof(struct sockaddr_in);
        errno = 0;
        k = recvfrom(descriptors[which],buffer,256,0,
            (struct sockaddr *)&scratch,&n);
        if (k < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            fatal(errno,"unable to flush socket",NULL);
            return errno;
        }
        ++*count;
        total += k;
    }
    errno = 0;
    if (fcntl(descriptors[which],F_SETFL,flags) == -1) {
        fatal(errno,"unable to restore blocking mode",NULL);
        return errno;
    }
    if (verbose > 2)
        fprintf(stderr,"Flushed %d packets totalling %d bytes\n",*count,total);
    return 0;
}



extern int close_socket (int which) {

/* There is little point in shielding this with a timeout, because any hangs
are unlikely to be interruptible.  It can get called when the sockets haven't
been opened, so ignore that case. */

    if (which < 0 || which >= MAX_SOCKETS) {
        fatal(EMSNTP_INTERNAL,"socket index out of range",NULL);
        return EMSNTP_INTERNAL;
    }
    if (descriptors[which] < 0) return;
    errno = 0;
    if (close(descriptors[which])) {
        fatal(errno,"unable to close NTP socket",NULL);
        return errno;
    }
    descriptors[which] = -1;

    return 0;
}
