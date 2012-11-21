/*  Copyright (C) 1996 N.M. Maclaren
    Copyright (C) 1996 The University of Cambridge

This includes all of the code needed to handle Internet addressing.  It is way
outside current POSIX, unfortunately.  It should be easy to convert to a system
that uses another mechanism.  The signal handling is not necessary for its
function, but is an attempt to avoid the program hanging when the name server
is inaccessible. */



#include "header.h"
#include "internet.h"

#include <netdb.h>
#include <arpa/inet.h>

#define INTERNET
#include "kludges.h"
#undef INTERNET

/* defined in libmsntp.c */
extern int libmsntp_port;


/* There needs to be some disgusting grobble for handling timeouts, which is
identical to the grobble in socket.c. */

static jmp_buf jump_buffer;

static void jump_handler (int sig) {
    longjmp(jump_buffer,1);
}

static int clear_alarm (void) {
    int k;

    k = errno;
    alarm(0);
    errno = 0;
    if (signal(SIGALRM,SIG_DFL) == SIG_ERR) {
        fatal(errno,"unable to reset signal handler",NULL);
        return errno;
    }
    errno = k;

    return 0;
}



int find_address (struct in_addr *address, struct in_addr *anywhere,
    struct in_addr *everywhere, int *port, char *hostname, int timespan) {

/* Locate the specified NTP server and return its Internet address and port 
number. */

    unsigned long ipaddr;
    struct in_addr nowhere[1];
    struct hostent *host;
    struct servent *service;

/* Set up the reserved Internet addresses, attempting not to assume that
addresses are 32 bits. */

    local_to_address(nowhere,INADDR_LOOPBACK);
    local_to_address(anywhere,INADDR_ANY);
    local_to_address(everywhere,INADDR_BROADCAST);

/* In libmsntp, as opposed to msntp, the caller specifies the port. Therefore,
we don't look it up with getservbyname() or default to NTP port 123. The
msntp_port global var is set by the libmsntp wrapper functions. */

    *port = htons((unsigned short)libmsntp_port);

/* Check the address, if any.  This assumes that the DNS is reliable, or is at
least checked by someone else.  But it doesn't assume that it is accessible, so
it needs to set up a timeout. */

    if (hostname == NULL)
        *address = *anywhere;
    else {
        if (setjmp(jump_buffer)) {
            fatal(EMSNTP_INTERNAL,
                  "unable to set up access to NTP server %s",hostname);
            return EMSNTP_INTERNAL;
        }
        errno = 0;
        if (signal(SIGALRM,jump_handler) == SIG_ERR) {
            fatal(errno,"unable to set up signal handler",NULL);
            return errno;
        }
        alarm((unsigned int)timespan);

/* Look up the Internet name or IP number. */

        if (! isdigit(hostname[0])) {
            errno = 0;
            host = gethostbyname(hostname);
        } else {
            if ((ipaddr = inet_addr(hostname)) == (unsigned long)-1) {
                fatal(EMSNTP_IP_ADDRESS,"invalid IP number %s",hostname);
                return EMSNTP_IP_ADDRESS;
            }
            network_to_address(address,ipaddr);
            errno = 0;
            host = gethostbyaddr((void *)address,sizeof(struct in_addr),
                AF_INET);
        }

/* Now clear the timer and check the result. */

        clear_alarm();
        if (host == NULL) {
            fatal(errno,"unable to locate IP address/number",NULL);
            return errno;
        }
        if (host->h_length != sizeof(struct in_addr)) {
            fatal(EMSNTP_AF_INET,
                  "the address does not seem to be an Internet one",NULL);
            return EMSNTP_AF_INET;
        }
        *address = *((struct in_addr **)host->h_addr_list)[0];

/* Note that in libmsntp, "reserved" IP addresses such as 127.0.0.1 are
allowed, for greater flexibility. */

        if (verbose)
            fprintf(stderr,
                "%s: using NTP server %s (%s) port %d\n",
                argv0,host->h_name,inet_ntoa(*address), ntohs(*port));
    }

    return 0;
}
