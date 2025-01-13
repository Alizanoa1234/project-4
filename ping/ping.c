#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>  
#include <getopt.h>
#include <math.h>

#define RECV_TIMEOUT_MS 10000
#define PACKET_BUFFER_SIZE 1024
#define PING_DELAY_SEC 1
#define MAX_ATTEMPTS 15
#define RETRY_LIMIT 3

// Track ping data
int total_packets_sent = 0;
int total_packets_received = 0;
float total_ping_time_ms = 0.0;
float *ping_times = NULL;  
int ping_count = 0;

// Function prototypes
unsigned short int calculate_checksum(void *data, unsigned int len);
void print_statistics(float *ping_times, char *target_address);
double compute_std_dev(float *values, int count, float avg);

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s -a <address> -t <type> [-c <count>] [-f]\n", argv[0]);
        return 1;
    }
    
    int max_pings = MAX_ATTEMPTS;  // Default max pings
    int continuous_mode = 0;       // Default is no flood mode
    int ip_version = 0;            // 4 for IPv4, 6 for IPv6
    char *ip_address = NULL;       // Target IP address
    int opt;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "a:t:c:f")) != -1) {
        switch (opt) {
            case 'a':
                ip_address = optarg;  
                break;
            case 't':
                ip_version = atoi(optarg);  // 4 for IPv4, 6 for IPv6
                if (ip_version != 4 && ip_version != 6) {
                    fprintf(stderr, "Error: Invalid type '%s'. Use 4 for IPv4 or 6 for IPv6.\n", optarg);
                    return 1;
                }
                break;
            case 'c':
                max_pings = atoi(optarg);  // Set custom count of pings
                break;
            case 'f':
                continuous_mode = 1;  // Enable flood mode
                break;
            default:
                fprintf(stderr, "Usage: %s -a <address> -t <type> [-c <count>] [-f]\n", argv[0]);
                return 1;
        }
    }

    if (ip_address == NULL) {
        fprintf(stderr, "Error: IP address is required (use -a flag).\n");
        return 1;
    }
    if (ip_version == 0) {
        fprintf(stderr, "Error: Communication type is required (use -t flag with 4 for IPv4 or 6 for IPv6).\n");
        return 1;
    }
    
    ping_times = (float *)malloc(max_pings * sizeof(float));
    if (ping_times == NULL) {
        perror("Memory allocation failed");
        return 1;
    }

    // Create socket and set destination
    struct sockaddr_storage target_address;
    memset(&target_address, 0, sizeof(target_address));

    if (ip_version == 4) {
        if (inet_pton(AF_INET, ip_address, &((struct sockaddr_in *)&target_address)->sin_addr) <= 0) {
            fprintf(stderr, "Error: \"%s\" is not a valid IPv4 address\n", ip_address);
            return 1;
        }
        ((struct sockaddr_in *)&target_address)->sin_family = AF_INET;
    } else if (ip_version == 6) {
        if (inet_pton(AF_INET6, ip_address, &((struct sockaddr_in6 *)&target_address)->sin6_addr) <= 0) {
            fprintf(stderr, "Error: \"%s\" is not a valid IPv6 address\n", ip_address);
            return 1;
        }
        ((struct sockaddr_in6 *)&target_address)->sin6_family = AF_INET6;
    }

    int sock = (ip_version == 4) ?
               socket(AF_INET, SOCK_RAW, IPPROTO_ICMP) :
               socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);

    if (sock < 0) {
        perror("Failed to create socket");
        if (errno == EACCES || errno == EPERM)
            fprintf(stderr, "You need to run the program with sudo.\n");
        return 1;
    }

    char buffer[PACKET_BUFFER_SIZE] = {0};
    char *data_msg = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    int data_length = strlen(data_msg) + 1;
    int seq_num = 0;
    struct pollfd poll_fds[1];
    poll_fds[0].fd = sock;
    poll_fds[0].events = POLLIN;

    printf("PING %s with %d bytes of data:\n", ip_address, data_length);

    while (max_pings-- > 0) {
        memset(buffer, 0, sizeof(buffer));

        if (ip_version == 4) {
            struct icmphdr icmp_hdr;
            icmp_hdr.type = ICMP_ECHO;
            icmp_hdr.code = 0;
            icmp_hdr.un.echo.id = htons(getpid());
            icmp_hdr.un.echo.sequence = htons(seq_num++);
            icmp_hdr.checksum = 0;

            memcpy(buffer, &icmp_hdr, sizeof(icmp_hdr));
            memcpy(buffer + sizeof(icmp_hdr), data_msg, data_length);
            ((struct icmphdr *)buffer)->checksum = calculate_checksum(buffer, sizeof(icmp_hdr) + data_length);
        } else {
            struct icmp6_hdr icmp6_hdr;
            icmp6_hdr.icmp6_type = ICMP6_ECHO_REQUEST;
            icmp6_hdr.icmp6_code = 0;
            icmp6_hdr.icmp6_id = htons(getpid());
            icmp6_hdr.icmp6_seq = htons(seq_num++);
            icmp6_hdr.icmp6_cksum = 0;

            memcpy(buffer, &icmp6_hdr, sizeof(icmp6_hdr));
            memcpy(buffer + sizeof(icmp6_hdr), data_msg, data_length);
        }

        struct timeval start, end;
        gettimeofday(&start, NULL);

        if (sendto(sock, buffer, (sizeof(struct icmphdr) + data_length), 0, (struct sockaddr *)&target_address, sizeof(target_address)) <= 0) {
            perror("sendto error");
            close(sock);
            return 1;
        }
        total_packets_sent++;

        int ret = poll(poll_fds, 1, RECV_TIMEOUT_MS);
        if (ret == 0) {
            fprintf(stderr, "Timeout: icmp_seq %d. Aborting..\n", seq_num);
            break;  // Exit the loop after the first timeout
        } else if (ret < 0) {
            perror("poll error");
            close(sock);
            return 1;  // Return on error
        }


        if (poll_fds[0].revents & POLLIN) {
            struct sockaddr_storage source_addr;
            socklen_t addr_len = sizeof(source_addr);
            memset(buffer, 0, sizeof(buffer));

            if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &addr_len) <= 0) {
                perror("recvfrom error");
                close(sock);
                return 1;
            }

            gettimeofday(&end, NULL);

            if (ip_version == 4) {
                struct iphdr *ip_hdr = (struct iphdr *)buffer;
                struct icmphdr *icmp_resp_hdr = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);

                if (icmp_resp_hdr->type == ICMP_ECHOREPLY) {
                    float round_trip_time = ((end.tv_usec - start.tv_usec) / 1000.0) + ((end.tv_sec - start.tv_sec) * 1000);
                    printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
                           (ntohs(ip_hdr->tot_len) - (ip_hdr->ihl * 4) - sizeof(struct icmphdr)),
                           inet_ntoa(((struct sockaddr_in *)&source_addr)->sin_addr),
                           ntohs(icmp_resp_hdr->un.echo.sequence),
                           ip_hdr->ttl, round_trip_time);
                    ping_times[ping_count++] = round_trip_time;
                    total_packets_received++;
                }
            }
        }

        if (!continuous_mode) sleep(PING_DELAY_SEC);
    }

    close(sock);
    print_statistics(ping_times, ip_address);
    return 0;
}

unsigned short int calculate_checksum(void *data, unsigned int len) {
    unsigned short int *ptr = (unsigned short int *)data;
    unsigned int sum = 0;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len > 0) {
        sum += *((unsigned char *)ptr);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

void print_statistics(float *results, char *address) {
    if (ping_count == 0) {
        printf("No ping data to display.\n");
        return;
    }

    float min_time = results[0], max_time = results[0], total_time = results[0];
    for (int i = 1; i < ping_count; i++) {
        if (results[i] < min_time) min_time = results[i];
        if (results[i] > max_time) max_time = results[i];
        total_time += results[i];
    }
    float avg_time = total_time / ping_count;
    printf("--- %s ping statistics ---\n", address);
    printf("%d packets sent, %d received, %.2f%% packet loss\n", total_packets_sent, total_packets_received,
           100.0 * (total_packets_sent - total_packets_received) / total_packets_sent);
    printf("rtt min/avg/max/stddev = %.2f/%.2f/%.2f/%.2f ms\n",
           min_time, avg_time, max_time, compute_std_dev(results, ping_count, avg_time));
}

double compute_std_dev(float *arr, int size, float avg) {
    double sum_squared_diffs = 0.0;
    for (int i = 0; i < size; i++) {
        sum_squared_diffs += (arr[i] - avg) * (arr[i] - avg);
    }
    return sqrt(sum_squared_diffs / size);
}
