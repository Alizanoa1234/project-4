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

int sockfd = -1; // ערך ראשוני לא תקין לסוקט
int global_id = 0;

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


void cleanup() {
    if (sockfd >= 0) {
        close(sockfd); // שחרור הסוקט
        printf("Socket closed successfully.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <destination>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    

    int max_hops = 30;
    int timeout = 1;
    int start_ttl = 1;  // TTL התחלתי
    int end_ttl = max_hops;  // TTL מקסימלי
    int opt;
    char *destination_ip = NULL;
    int count = 3;   
    int consecutive_failures = 0;
    int global_seq = 0; // משתנה גלובלי עבור מספר ייחודי
    srand(time(NULL)); // אתחול מחולל רנדומלי
    global_id = rand() & 0xFFFF; // קבלת מזהה ייחודי בתחום 16 ביט




    //flags
    while ((opt = getopt(argc, argv, "a:c:t:m:h")) != -1) {
        switch (opt) {
        case 'a':
            destination_ip = optarg;
            break;
        case 'c':
            count = atoi(optarg);
            if (count <= 0) {
                fprintf(stderr, "Invalid packet count: %s. Using default: %d\n", optarg, 3);
                count = 3;
            }
            break;
        // case 't':
        //     timeout = atoi(optarg);
        //     if (timeout <= 0) {
        //         fprintf(stderr, "Invalid timeout: %s. Using default: 1 second\n", optarg);
        //         timeout = 1;
        //     }
        //     break;
        case 'm':
            max_hops = atoi(optarg);
            if (max_hops <= 0) {
                fprintf(stderr, "Invalid max hops: %s. Using default: %d\n", optarg, 30);
                max_hops = 30;
            } else if (max_hops > 64) {
                fprintf(stderr, "Max hops too high (%d). Limiting to 64.\n", max_hops);
                max_hops = 64;
            }
            break;

        case 'h':
            printf("Usage: %s -a <destination_ip> [-c <count>] [-t <timeout>] [-m <max_hops>]\n", argv[0]);
            printf("Options:\n");
            printf("  -a <destination_ip>   Specify the target IP address\n");
            printf("  -c <count>            Number of packets to send per hop (default: 3)\n");
            printf("  -t <timeout>          Timeout in seconds for each packet (default: 1)\n");
            printf("  -m <max_hops>         Maximum number of hops (default: 30)\n");
            exit(EXIT_SUCCESS);
        default:
            printf("Unknown flag detected.\n");
            fprintf(stderr, "Usage: %s -a <destination_ip> [-c <count>] [-t <timeout>] [-m <max_hops>]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("Validating IP: %s\n", destination_ip);

    if (!destination_ip) {
        fprintf(stderr, "Error: Destination IP (-a) is required\n");
        exit(EXIT_FAILURE);
    }
     printf("Destination IP: %s\n", destination_ip);
    printf("Packet count: %d\n", count);
    printf("Timeout: %d seconds\n", timeout);
    printf("Max hops: %d\n", max_hops);


    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        if (errno == EPERM) { // הרשאות לא מספקות
            fprintf(stderr, "Error: Unable to create raw socket. You need elevated privileges (run with sudo).\n");
        } else if (errno == EAFNOSUPPORT) { // פרוטוקול לא נתמך
            fprintf(stderr, "Error: Address family not supported. Ensure your system supports ICMP raw sockets.\n");
        } else {
            perror("Socket creation failed");
        }
        exit(EXIT_FAILURE);
    }

    atexit(cleanup);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    
    if (inet_pton(AF_INET, destination_ip, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid destination address: %s\n", destination_ip);
        exit(EXIT_FAILURE);
    }

    struct icmp icmp_packet;
    memset(&icmp_packet, 0, sizeof(icmp_packet));
    icmp_packet.icmp_type = 8; // Echo Request
    icmp_packet.icmp_code = 0;
    icmp_packet.icmp_id = (getpid() ^ global_id) & 0xFFFF; // שילוב מזהה ריצה עם PID

    printf("Traceroute to %s, %d hops max\n", destination_ip, max_hops);
for (int ttl = start_ttl; ttl <= end_ttl; ttl++) {
    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("Failed to set TTL. Ensure you have permission to set socket options.");
        continue;
    }

    int failed_count = 0; // מעקב אחרי חבילות כושלות עבור hop זה
    printf("TTL=%-2d | ", ttl);

    for (int i = 0; i < count; i++) {
        struct timeval timeout_struct;
        timeout_struct.tv_sec = timeout;
        timeout_struct.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_struct, sizeof(timeout_struct));

        icmp_packet.icmp_seq = global_seq++;
        icmp_packet.icmp_cksum = 0;
        icmp_packet.icmp_cksum = calculate_checksum(&icmp_packet, sizeof(icmp_packet));

        struct timeval start, end;
        gettimeofday(&start, NULL);

        ssize_t sent = sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (sent < 0) {
            perror("Failed to send packet. Check your network connection or socket permissions.");
            continue;
        }

        char buffer[1024] = {0};
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);
        int received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (received <= 0) {
            printf("* "); // מסמנים חבילה כושלת
            failed_count++;
            continue;
        }
        gettimeofday(&end, NULL);
        if (end.tv_sec < start.tv_sec || 
            (end.tv_sec == start.tv_sec && end.tv_usec < start.tv_usec)) {
            fprintf(stderr, "Error: Negative RTT value detected.\n");
            failed_count++;
            continue;
        }

        // עיבוד חבילה מוצלחת
        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
        if (rtt < 0) {
            printf("RTT calculation error: negative value.\n");
            failed_count++;
            continue;
        }

        struct ip *ip_header = (struct ip *)buffer;
        struct icmp *icmp_reply = (struct icmp *)(buffer + (ip_header->ip_hl * 4));

        // בדיקת Checksum
        unsigned short checksum_received = icmp_reply->icmp_cksum;
        icmp_reply->icmp_cksum = 0;
        unsigned short checksum_calculated = calculate_checksum(icmp_reply, received - (ip_header->ip_hl * 4));
        if (checksum_received != checksum_calculated) {
            printf("Checksum mismatch! ");
            failed_count++;
            continue;
        }

        // עיבוד סוגי ICMP
        switch (icmp_reply->icmp_type) {
            case 0: // ICMP_ECHO_REPLY
                printf("%s (%.2f ms) ", inet_ntoa(recv_addr.sin_addr), rtt);
                return 0;
            case 11: // ICMP_TIME_EXCEEDED
                printf("%s (%.2f ms) ", inet_ntoa(recv_addr.sin_addr), rtt);
                break;
            case 3: // ICMP_DEST_UNREACHABLE
                printf("%s (Destination unreachable) ", inet_ntoa(recv_addr.sin_addr));
                break;
            default:
                printf("%s (Unknown ICMP type %d) ", inet_ntoa(recv_addr.sin_addr), icmp_reply->icmp_type);
                break;
        }
    }

    // אם כל החבילות נכשלו
    if (failed_count == count) {
        printf("| Status=No ICMP response (Router might be blocking ICMP)\n");
    } else {
        printf("\n"); // מעבר שורה עבור hop זה
    }

    // בדיקה אם הגענו למספר כשלונות רצופים
    consecutive_failures = (failed_count == count) ? consecutive_failures + 1 : 0;
    if (consecutive_failures >= 3) {
        printf("Target is unreachable: No response for 3 consecutive hops.\n");
        break;
    }
    }

    return 0;
}