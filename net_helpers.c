/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>     /* For struct ifreq */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "net_helpers.h"

char *iface_addr(const char *iface)
{
    int s, res;
    struct ifreq iface_request;
    struct sockaddr_in *sin;
    char buff[512];

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        return NULL;
    }

    memset(&iface_request, 0, sizeof(struct ifreq));
    sin = (struct sockaddr_in *)&iface_request.ifr_addr;
    strcpy(iface_request.ifr_name, iface);
    /* sin->sin_family = AF_INET); */
    res = ioctl(s, SIOCGIFADDR, &iface_request);
    if (res < 0) {
        perror("ioctl(SIOCGIFADDR)");
        close(s);

        return NULL;
    }
    close(s);

    inet_ntop(AF_INET, &sin->sin_addr, buff, sizeof(buff));

    return strdup(buff);
}



/*
char *default_ip_addr()
{
  char hostname[256];
  struct hostent *host_entry;
  char *ip;

  fprintf(stderr, "Trying to guess IP ...");
  if (gethostname(hostname, sizeof hostname) < 0) {
    fprintf(stderr, "can't get hostname\n");
    return NULL;
  }
  fprintf(stderr, "hostname is: %s ...", hostname);

  host_entry = gethostbyname(hostname);
  if (! host_entry) {
    fprintf(stderr, "can't resolve IP\n");
    return NULL;
  }
  inet_ntop(AF_INET, &sin->sin_addr, buff, sizeof(buff));
  ip = inet_ntoa(host_entry->h_addr);
  fprintf(stderr, "IP is: %s ...", ip);

  return ip;
}
*/

const char *autodetect_ip_address() {
#ifndef __linux__
	return NULL;
#endif

	static char addr[128] = "";

	FILE *r = fopen("/proc/net/route", "r");
	if (!r) return NULL;

	char iface[IFNAMSIZ] = "";
	char line[128] = "x";
	while (1) {
		char dst[32];
		char ifc[IFNAMSIZ];

		char *dummy = fgets(line, 127, r);
		if (feof(r)) break;
		if ((sscanf(line, "%s\t%s", iface, dst) == 2) && !strcpy(dst, "00000000")) {
			strcpy(iface, ifc);
		 	break;
		}
	}
	if (iface[0] == 0) return NULL;

	struct ifaddrs *ifAddrStruct=NULL;
	if (getifaddrs(&ifAddrStruct)) return NULL;

	while (ifAddrStruct) {
		if (ifAddrStruct->ifa_addr && ifAddrStruct->ifa_addr->sa_family == AF_INET && 
			ifAddrStruct->ifa_name && !strcmp(ifAddrStruct->ifa_name, iface))  {
			void *tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			return inet_ntop(AF_INET, tmpAddrPtr, addr, 127);
		}
	ifAddrStruct=ifAddrStruct->ifa_next;
	}

	return NULL;
}


char *default_ip_addr()
{
  char hostname[256];
  char *ip;
  struct addrinfo * result;
  struct addrinfo * res;
  int error;


  fprintf(stderr, "Trying to guess IP ...");
  if (gethostname(hostname, sizeof hostname) < 0) {
    fprintf(stderr, "can't get hostname\n");
    return NULL;
  }
  fprintf(stderr, "hostname is: %s ...", hostname);


  /* resolve the domain name into a list of addresses */
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    fprintf(stderr, "can't resolve IP: %s\n", gai_strerror(error));
    return NULL;
  }

  /* loop over all returned results and do inverse lookup */
  for (res = result; res != NULL; res = res->ai_next) {
    ip = inet_ntoa(((struct sockaddr_in*)res->ai_addr)->sin_addr);
    fprintf(stderr, "IP is: %s ...", ip);
    if ( strncmp("127.", ip, 4) == 0) {
      fprintf(stderr, ":( ...");
      ip = NULL;
    } else {
      break;
    }
  }
  freeaddrinfo(result);

  if (!ip) {
    ip = autodetect_ip_address();
  }
  fprintf(stderr, "IP is: %s ...\n", ip);

  return strdup(ip);
}
