#ifndef PING_H
#define PING_H
#include <stddef.h>  


#define RECV_TIMEOUT_MS 10000
#define PACKET_BUFFER_SIZE 1024
#define PING_DELAY_SEC 1
#define MAX_ATTEMPTS 15


// Function prototypes
unsigned short int calculate_checksum(void *data, unsigned int len);
double compute_std_dev(float *values, int count, float avg);
void print_ping_info(float *results, char *address);
void sigint_handler(int sig);


// Track ping data
int total_packets_sent = 0;
int total_packets_received = 0;
float total_ping_time_ms = 0.0;
float *ping_times = NULL;  
int ping_count = 0;

#endif // PING_H