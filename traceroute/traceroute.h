#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// פונקציות לחישוב Checksum ולבניית כותרות
unsigned short calculate_checksum(void *buffer, int length);

struct ip build_ip_header(struct in_addr source, struct in_addr dest, int ttl);

struct icmp build_icmp_header(int seq);

#endif // TRACEROUTE_H
