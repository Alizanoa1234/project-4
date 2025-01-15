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
#include <errno.h>
#include <getopt.h>
#include <signal.h>

// Global variables
extern int sockfd; // Initial invalid value for the socket
extern int global_id;
extern int total_sent;
extern int total_received;
extern int total_failed; // Total number of failed packets
extern int ttl_processed;
extern double total_rtt;
extern double min_rtt;
extern double max_rtt;
extern struct timeval program_start, program_end;

// Function declarations
unsigned short calculate_checksum(void *buffer, int length);
void cleanup(void);
void handle_sigint(int sig);

#endif // TRACEROUTE_H
