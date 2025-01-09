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

// פונקציה לחישוב Checksum
unsigned short calculate_checksum(void *buffer, int length) {
    unsigned short *data = buffer;
    unsigned long sum = 0;

    while (length > 1) {
        sum += *data++;
        length -= 2;
    }

    if (length == 1) {
        sum += *(unsigned char *)data;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

// פונקציה לשליחת ping ובדיקת תגובה
int send_ping(const char *ip_address) {
    char command[256];
    snprintf(command, sizeof(command), "ping -c 1 -w 1 %s > /dev/null 2>&1", ip_address);
    return system(command);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <destination>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, argv[1], &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid destination address: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    int ttl = 1, max_hops = 30;
    struct timeval start, end;

    printf("Traceroute to %s, %d hops max\n", argv[1], max_hops);

    for (int hop = 1; hop <= max_hops; hop++) {
        setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

        struct icmp icmp_packet;
        memset(&icmp_packet, 0, sizeof(icmp_packet));
        icmp_packet.icmp_type = 8; // Echo Request
        icmp_packet.icmp_code = 0;
        icmp_packet.icmp_id = getpid();
        icmp_packet.icmp_seq = hop;
        icmp_packet.icmp_cksum = 0;
        icmp_packet.icmp_cksum = calculate_checksum(&icmp_packet, sizeof(icmp_packet));

        gettimeofday(&start, NULL);
        sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        char buffer[1024];
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);
        int received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);

        gettimeofday(&end, NULL);

        if (received > 0) {
            double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
            printf("%d. %s RTT=%.2f ms\n", hop, inet_ntoa(recv_addr.sin_addr), rtt);

            if (strcmp(inet_ntoa(recv_addr.sin_addr), argv[1]) == 0) {
                printf("Destination reached!\n");
                break;
            }
        } else {
            printf("%d. * ", hop);

            // נסה לשלוח ping
            char *last_hop_ip = inet_ntoa(recv_addr.sin_addr);
            if (send_ping(last_hop_ip) == 0) {
                printf("(Likely ICMP blocked)\n");
            } else {
                printf("(Unreachable or timeout)\n");
            }
        }

        ttl++;
    }

    close(sockfd);
    return 0;
}
