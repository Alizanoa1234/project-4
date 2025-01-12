#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
<<<<<<< HEAD
#include <getopt.h> 
#include <netinet/icmp6.h>  

=======
#include <getopt.h>  // For getopt()
>>>>>>> origin/master

// Constants for IPv4/IPv6
#define ICMP_HDRLEN 8
#define IP4_HDRLEN 20
#define IP6_HDRLEN 40
#define PACKET_SIZE 1024

// Statistics variables
int packets_sent = 0;
int packets_received = 0;
double rtt_min = 1000000, rtt_max = 0, rtt_sum = 0;

// Function declarations
unsigned short calculate_checksum(unsigned short *paddress, int len);
void handle_sigint(int signum);
void parse_arguments(int argc, char *argv[], char **dest_ip, int *type, int *count, int *flood);

int main(int argc, char *argv[]) {
    char *dest_ip = NULL;
<<<<<<< HEAD
    int type = 4;     // IPv4 ddest_in.sin_addr.s_addrefault
=======
    int type = 4;     // IPv4 default
>>>>>>> origin/master
    int count = -1;   // Unlimited by default
    int flood = 0;    // No flood mode by default

    // Parse command-line arguments
    parse_arguments(argc, argv, &dest_ip, &type, &count, &flood);

    // Create a raw socket
    int sock = -1;
    if (type == 4) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    } else if (type == 6) {
        sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);  // Use ICMPv6 for IPv6
    } else {
        fprintf(stderr, "Only IPv4 (type 4) and IPv6 (type 6) are supported.\n");
        return -1;
    }

    if (sock == -1) {
        perror("socket() failed");
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 10;  // Set timeout to 10 seconds
    tv.tv_usec = 0;  // No microseconds
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) != 0) {
        perror("Error in setting timeout option");
        return -1;  // Exit if the timeout could not be set
    }

    // Destination address setup
    struct sockaddr_in dest_in;
    struct sockaddr_in6 dest_in6;
    memset(&dest_in, 0, sizeof(dest_in));
    memset(&dest_in6, 0, sizeof(dest_in6));
    
    if (type == 4) {
        dest_in.sin_family = AF_INET;
<<<<<<< HEAD
        // dest_in.sin_addr.s_addr = struct icmp6_hdr icmp6hdr; inet_addr(dest_ip);
        dest_in.sin_addr.s_addr = inet_addr(dest_ip);

=======
        dest_in.sin_addr.s_addr = inet_addr(dest_ip);
>>>>>>> origin/master
    } else if (type == 6){
        dest_in6.sin6_family = AF_INET6;
        inet_pton(AF_INET6, dest_ip, &dest_in6.sin6_addr);
    }

    signal(SIGINT, handle_sigint);  // Set up signal handler for Ctrl+C

    char packet[PACKET_SIZE];
    int sequence = 0;

    while (count == -1 || packets_sent < count) {
        struct icmp icmphdr;
        struct icmp6_hdr icmp6hdr;
        memset(&icmphdr, 0, sizeof(icmphdr));
        memset(&icmp6hdr, 0, sizeof(icmp6hdr));

        if (type == 4) {
            icmphdr.icmp_type = ICMP_ECHO;
            icmphdr.icmp_code = 0;
            icmphdr.icmp_id = getpid();
            icmphdr.icmp_seq = sequence++;
            icmphdr.icmp_cksum = 0;
        } else {
            icmp6hdr.icmp6_type = ICMP6_ECHO_REQUEST;
            icmp6hdr.icmp6_code = 0;
            icmp6hdr.icmp6_id = getpid();
            icmp6hdr.icmp6_seq = sequence++;
            icmp6hdr.icmp6_cksum = 0;
        }

        // Copy ICMP header and data to packet
        char data[] = "This is a ping message.";
        int datalen = strlen(data) + 1;
        memcpy(packet, (type == 4) ? &icmphdr : &icmp6hdr, type == 4 ? ICMP_HDRLEN : sizeof(icmp6hdr));
        memcpy(packet + (type == 4 ? ICMP_HDRLEN : sizeof(icmp6hdr)), data, datalen);

        if (type == 4) {
            icmphdr.icmp_cksum = calculate_checksum((unsigned short *)packet, ICMP_HDRLEN + datalen);
            memcpy(packet, &icmphdr, ICMP_HDRLEN);
<<<<<<< HEAD
            memcpy(packet + ICMP_HDRLEN, data, datalen);

        } else {
            icmp6hdr.icmp6_cksum = calculate_checksum((unsigned short *)packet, sizeof(icmp6hdr) + datalen);
            memcpy(packet, &icmp6hdr, sizeof(icmp6hdr));
            memcpy(packet + sizeof(icmp6hdr), data, datalen);

=======
        } else {
            icmp6hdr.icmp6_cksum = calculate_checksum((unsigned short *)packet, sizeof(icmp6hdr) + datalen);
            memcpy(packet, &icmp6hdr, sizeof(icmp6hdr));
>>>>>>> origin/master
        }

        gettimeofday(&(struct timeval){0}, 0);  // Start timing
        int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, 
            (type == 4) ? (struct sockaddr *)&dest_in : (struct sockaddr *)&dest_in6, 
            (type == 4) ? sizeof(dest_in) : sizeof(dest_in6));
        if (bytes_sent == -1) {
            perror("sendto() failed");
            continue;
        }

        packets_sent++;

        // Receive response
        struct timeval start, end;
        gettimeofday(&start, 0);
        char response[PACKET_SIZE];
        socklen_t len = (type == 4) ? sizeof(dest_in) : sizeof(dest_in6);
        ssize_t bytes_received = recvfrom(sock, response, sizeof(response), 0, 
            (type == 4) ? (struct sockaddr *)&dest_in : (struct sockaddr *)&dest_in6, &len);
        gettimeofday(&end, 0);

        if (bytes_received > 0) {
            if (type == 4) {
                struct iphdr *iphdr = (struct iphdr *)response;
                struct icmp *icmp_reply = (struct icmp *)(response + (iphdr->ihl * 4));

                if (icmp_reply->icmp_type == ICMP_ECHOREPLY) {
                    packets_received++;
                     double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
                    rtt_sum += rtt;
                    if (rtt < rtt_min) rtt_min = rtt;
                    if (rtt > rtt_max) rtt_max = rtt;
<<<<<<< HEAD
                    printf("%zd bytes from %s: icmp_seq=%d time=%.2f ms\n", bytes_received, dest_ip, icmp_reply->icmp_seq, rtt);
                }
            } else if (type == 6) {  // IPv6 Response Handling
                struct icmp6_hdr *icmp6_reply = (struct icmp6_hdr *)response;
                if (icmp6_reply->icmp6_type == ICMP6_ECHO_REPLY) {
                    packets_received++;
                    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
                    rtt_sum += rtt;
                    if (rtt < rtt_min) rtt_min = rtt;
                    if (rtt > rtt_max) rtt_max = rtt;
                    printf("%zd bytes from %s: icmp_seq=%d time=%.2f ms\n", bytes_received, dest_ip, icmp6_reply->icmp6_seq, rtt);
                }
    }
=======
                    printf("%d bytes from %s: icmp_seq=%d time=%.2f ms\n", bytes_received, dest_ip, icmp_reply->icmp_seq, rtt);
                }
            }
>>>>>>> origin/master
            // Add similar parsing for IPv6 echo reply handling here
        } else {
            printf("Request timed out.\n");
        }

        if (!flood) sleep(1);  // Wait 1 second unless in flood mode
    }

    close(sock);
    return 0;
}

// Handle Ctrl+C (SIGINT) and print final statistics
void handle_sigint(int signum) {
<<<<<<< HEAD
    (void)signum;  // Suppress unused parameter warning
=======
>>>>>>> origin/master
    printf("\n--- Ping statistics ---\n");
    printf("%d packets transmitted, %d received, %.2f%% packet loss\n", packets_sent, packets_received,
           ((packets_sent - packets_received) / (double)packets_sent) * 100.0);
    if (packets_received > 0) {
        printf("rtt min/avg/max = %.2f/%.2f/%.2f ms\n", rtt_min, rtt_sum / packets_received, rtt_max);
    }
    exit(0);
}

// Parse command-line arguments
void parse_arguments(int argc, char *argv[], char **dest_ip, int *type, int *count, int *flood) {
    int opt;
    while ((opt = getopt(argc, argv, "a:t:c:f")) != -1) {
        switch (opt) {
            case 'a': *dest_ip = optarg; break;
            case 't': *type = atoi(optarg); break;
            case 'c': *count = atoi(optarg); break;
            case 'f': *flood = 1; break;
            default:
                fprintf(stderr, "Usage: %s -a <address> -t <4|6> [-c count] [-f]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (*dest_ip == NULL || (*type != 4 && *type != 6)) {
        fprintf(stderr, "Address and type are mandatory.\n");
        exit(EXIT_FAILURE);
    }
}

// Compute checksum (RFC 1071)
unsigned short calculate_checksum(unsigned short *paddress, int len) {
    int sum = 0;
    unsigned short *ptr = paddress;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len == 1) sum += *(unsigned char *)ptr;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
<<<<<<< HEAD
}
=======
}
>>>>>>> origin/master
