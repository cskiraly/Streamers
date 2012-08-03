/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <sys/types.h>
#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>     /* For struct ifreq */
#include <netdb.h>
#else
#include <winsock2.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helpers.h"

char *iface_addr(const char *iface)
{
#ifndef _WIN32
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
#else
    if(iface != NULL && strcmp(iface, "lo") == 0) return "127.0.0.1";
    if(iface != NULL && inet_addr(iface) != INADDR_NONE) return strdup(iface);
    return default_ip_addr();
#endif
}




char *simple_ip_addr()
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
  ip = strdup(inet_ntoa(*(struct in_addr*)host_entry->h_addr));
  fprintf(stderr, "IP is: %s ...", ip);

  return ip;
}


const char *autodetect_ip_address() {
#ifdef __linux__

	static char addr[128] = "";
	char iface[IFNAMSIZ] = "";
	char line[128] = "x";
	struct ifaddrs *ifaddr, *ifa;
	char *ret = NULL;

	FILE *r = fopen("/proc/net/route", "r");
	if (!r) return NULL;

	while (1) {
		char dst[32];
		char ifc[IFNAMSIZ];

		fgets(line, 127, r);
		if (feof(r)) break;
		if ((sscanf(line, "%s\t%s", iface, dst) == 2) && !strcpy(dst, "00000000")) {
			strcpy(iface, ifc);
		 	break;
		}
	}
	if (iface[0] == 0) return NULL;

	if (getifaddrs(&ifaddr) < 0) {
		perror("getifaddrs");
		return NULL;
	}

	ifa = ifaddr;
	while (ifa) {
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && 
			ifa->ifa_name && !strcmp(ifa->ifa_name, iface))  {
			void *tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			if (inet_ntop(AF_INET, tmpAddrPtr, addr, 127)) {
				ret = addr;
			} else {
				perror("inet_ntop error");
				ret = NULL;
			}
			break;
		}
	ifa=ifa->ifa_next;
	}

	freeifaddrs(ifaddr);
	return ret;
#else
        return simple_ip_addr();
#endif
}


const char *hostname_ip_addr()
{
#ifndef _WIN32
  const char *ip;
  char hostname[256];
  struct addrinfo * result;
  struct addrinfo * res;
  int error;

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

  return ip;
#else
  return NULL;
#endif
}

char *default_ip_addr()
{
  const char *ip = NULL;

  fprintf(stderr, "Trying to guess IP ...");

  //ip = hostname_ip_addr();

  if (!ip) {
    ip = autodetect_ip_address();
  }
  if (!ip) {
    fprintf(stderr, "cannot detect IP!\n");
    return NULL;
  }
  fprintf(stderr, "IP is: %s ...\n", ip);

  return strdup(ip);
}
