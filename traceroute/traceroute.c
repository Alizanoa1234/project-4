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
#include <getopt.h> // Include this for getopt and optarg
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>

int sockfd = -1; // Initial invalid value for the socket
int global_id = 0;
int total_sent = 0;
int total_received = 0;
int total_failed = 0; // Total number of failed packets
int ttl_processed = 0;
double total_rtt = 0.0;
double min_rtt = 1e9;
double max_rtt = 0.0;
struct timeval program_start, program_end;

// Function to calculate checksum
unsigned short calculate_checksum(void *buffer, int length)
{
    unsigned short *data = buffer;
    unsigned long sum = 0;

    while (length > 1)
    {
        sum += *data++;
        length -= 2;
    }

    if (length == 1)
    {
        sum += *(unsigned char *)data;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

void cleanup()
{
    if (sockfd >= 0)
    {
        close(sockfd); // Release the socket
        printf("Socket closed successfully.\n");
    }
}

void handle_sigint(int sig __attribute__((unused)))
{
    gettimeofday(&program_end, NULL); // Record end time
    double runtime = (program_end.tv_sec - program_start.tv_sec) * 1000.0 +
                     (program_end.tv_usec - program_start.tv_usec) / 1000.0;

    printf("\n--- Program Statistics (Interrupted) ---\n");
    printf("Stopped at TTL: %d\n", ttl_processed); // Print the TTL where the program stopped
    printf("Total packets sent: %d\n", total_sent);
    printf("Total packets received: %d\n", total_received);
    printf("Total packets failed: %d\n", total_failed);
    printf("Program runtime: %.2f ms\n", runtime);
    if (total_received > 0)
    {
        double avg_rtt = total_rtt / total_received;
        printf("RTT (ms): min=%.2f, max=%.2f, avg=%.2f\n", min_rtt, max_rtt, avg_rtt);
    }
    else
    {
        printf("No packets received. RTT statistics unavailable.\n");
    }
    exit(0); // Exit the program
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <destination>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    gettimeofday(&program_start, NULL); // Record start time

    int max_hops = 30;
    int timeout = 1;
    int opt;
    char *destination_ip = NULL;
    int count = 3;
    int consecutive_failures = 0;
    int global_seq = 0;          // Global variable for unique sequence number
    srand(time(NULL));           // Initialize random number generator
    global_id = rand() & 0xFFFF; // Get a unique ID in the 16-bit range
    int exit_status = 0;
    int flood_mode = 0;

    // Flags
    while ((opt = getopt(argc, argv, "a:c:t:m:h:f")) != -1)
    {
        switch (opt)
        {
        case 'a':
            destination_ip = optarg;
            break;
        case 'c':
            count = atoi(optarg);
            if (count <= 0)
            {
                fprintf(stderr, "Invalid packet count: %s. Using default: %d\n", optarg, 3);
                count = 3;
            }
            break;
        case 't':
            timeout = atoi(optarg);
            if (timeout <= 0)
            {
                fprintf(stderr, "Invalid timeout: %s. Using default: 1 second\n", optarg);
                timeout = 1;
            }
            break;
        case 'm':
            max_hops = atoi(optarg);
            if (max_hops <= 0)
            {
                fprintf(stderr, "Invalid max hops: %s. Using default: %d\n", optarg, 30);
                max_hops = 30;
            }
            else if (max_hops > 64)
            {
                fprintf(stderr, "Max hops too high (%d). Limiting to 64.\n", max_hops);
                max_hops = 64;
            }
            break;
        case 'f':
            printf("Flood mode activated\n");
            flood_mode = 1;
            break;
        case 'h':
            printf("Usage: %s -a <destination_ip> [-c <count>] [-t <timeout>] [-m <max_hops>]\n", argv[0]);
            printf("Options:\n");
            printf("  -a <destination_ip>   Specify the target IP address\n");
            printf("  -c <count>            Number of packets to send per hop (default: 3)\n");
            printf("  -t <timeout>          Timeout in seconds for each packet (default: 1)\n");
            printf("  -m <max_hops>         Maximum number of hops (default: 30)\n");
            printf("  -f                    Flood mode (send packets without delay)\n");
            exit(EXIT_SUCCESS);
        default:
            printf("Unknown flag detected.\n");
            fprintf(stderr, "Usage: %s -a <destination_ip> [-c <count>] [-t <timeout>] [-m <max_hops>]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    int start_ttl = 1;      // Initial TTL
    int end_ttl = max_hops; // Maximum TTL

    if (!destination_ip)
    {
        fprintf(stderr, "Error: Destination IP (-a) is required\n");
        exit(EXIT_FAILURE);
    }

    printf("Destination IP: %s\n", destination_ip);
    printf("Packet count: %d\n", count);
    printf("Timeout: %d seconds\n", timeout);
    printf("Max hops: %d\n", max_hops);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        if (errno == EPERM)
        { // Insufficient permissions
            fprintf(stderr, "Error: Unable to create raw socket. You need elevated privileges (run with sudo).\n");
        }
        else if (errno == EAFNOSUPPORT)
        { // Unsupported protocol
            fprintf(stderr, "Error: Address family not supported. Ensure your system supports ICMP raw sockets.\n");
        }
        else
        {
            perror("Socket creation failed");
        }
        exit(EXIT_FAILURE);
    }

    atexit(cleanup);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, destination_ip, &dest_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid destination address: %s\n", destination_ip);
        exit(EXIT_FAILURE);
    }

    struct icmphdr icmp_hdr; // Use icmphdr structure
    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.type = ICMP_ECHO; // Echo Request
    icmp_hdr.code = 0;
    icmp_hdr.un.echo.id = htons((getpid() ^ global_id) & 0xFFFF); // Unique ID using PID

    printf("Traceroute to %s, %d hops max\n", destination_ip, max_hops);

    int reached_destination = 0;  // Flag to identify destination reached


    for (int ttl = start_ttl; ttl <= end_ttl; ttl++)
    {
        ttl_processed++; // Update the number of TTLs processed
        if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("Failed to set TTL. Ensure you have permission to set socket options.");
            continue;
        }

        int failed_count = 0;         // Track failed packets for this hop
        char result[256] = "";        // Temporary variable to store results
        struct sockaddr_in recv_addr; // Define recv_addr here to make it accessible for printing

        for (int i = 0; i < count; i++)
        {
            struct timeval timeout_struct;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_struct, sizeof(timeout_struct));

            timeout_struct.tv_sec = timeout;
            timeout_struct.tv_usec = 0;

            icmp_hdr.un.echo.sequence = htons(global_seq++);
            icmp_hdr.checksum = 0; // Reset checksum before calculation
            icmp_hdr.checksum = calculate_checksum(&icmp_hdr, sizeof(icmp_hdr));

            struct timeval start, end;
            gettimeofday(&start, NULL);

            ssize_t sent = sendto(sockfd, &icmp_hdr, sizeof(icmp_hdr), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (sent < 0)
            {
                perror("Failed to send packet. Check your network connection or socket permissions.");
                continue;
            }

            char buffer[1024] = {0};
            socklen_t addr_len = sizeof(recv_addr);

            int is_failed = 1; // Assume the packet failed by default

            int received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);
            if (received > 0)
            {
                is_failed = 0; // If the packet is received, cancel the failure
                gettimeofday(&end, NULL);

                if (!(end.tv_sec < start.tv_sec || (end.tv_sec == start.tv_sec && end.tv_usec < start.tv_usec)))
                {
                    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

                    total_sent++; // Update the number of packets sent

                    if (rtt >= 0)
                    {
                        total_rtt += rtt;
                        if (rtt < min_rtt)
                            min_rtt = rtt;
                        if (rtt > max_rtt)
                            max_rtt = rtt;
                        total_received++;

                        struct iphdr *ip_hdr = (struct iphdr *)buffer;
                        struct icmphdr *icmp_reply = (struct icmphdr *)(buffer + (ip_hdr->ihl * 4));

                        unsigned short checksum_received = icmp_reply->checksum;
                        icmp_reply->checksum = 0;
                        unsigned short checksum_calculated = calculate_checksum(icmp_reply, received - (ip_hdr->ihl * 4));

                        if (checksum_received == checksum_calculated)
                        {
                            is_failed = 0; // The packet passed all checks successfully

                            if (icmp_reply->type == ICMP_ECHOREPLY &&
                                strcmp(inet_ntoa(recv_addr.sin_addr), destination_ip) == 0)
                            {
                                reached_destination = 1;
                            }
                            char temp[64];
                            snprintf(temp, sizeof(temp), "%.2fms   ", rtt); // Add only the RTT
                            strcat(result, temp);
                        }
                    }
                }
            }

            // If the packet failed, add an asterisk and update failed_count
            if (is_failed)
            {
                strcat(result, "* ");
                failed_count++;
            }

            if (i < count - 1 && !flood_mode)
            { // No need to delay after the last packet
                sleep(1);
            }
        }

        if (failed_count == count)
        {
            printf("%-4d*  *  *\n", ttl); // Print TTL and asterisks
        }
        else
        {
            printf("%-4d%-15s %s\n", ttl, inet_ntoa(recv_addr.sin_addr), result); // Print the IP address once
        }

        total_failed += failed_count;

        if (reached_destination)
        {
            printf("Reached destination: %s\n", destination_ip);
            exit_status = 0; // Destination identified, normal termination
            break;
        }

        consecutive_failures = (failed_count == count) ? consecutive_failures + 1 : 0;
        if (consecutive_failures >= 3)
        {
            printf("Target is unreachable: No response for 3 consecutive hops.\n");
            exit_status = 1;
            break;
        }
    }
    if (!reached_destination && exit_status == 0)
    {

        printf("Failed to reach destination: %s\n", destination_ip);
    }
    gettimeofday(&program_end, NULL);
    double runtime = (program_end.tv_sec - program_start.tv_sec) * 1000.0 +
                     (program_end.tv_usec - program_start.tv_usec) / 1000.0;

    printf("\n--- Program Statistics ---\n");
    printf("Total TTLs processed: %d\n", ttl_processed);
    printf("Total packets sent: %d\n", total_sent);
    printf("Total packets received: %d\n", total_received);
    printf("Program runtime: %.2f ms\n", runtime);
    printf("Total packets failed: %d\n", total_failed);

    if (total_received > 0)
    {
        double avg_rtt = total_rtt / total_received;
        printf("RTT (ms): min=%.2f, max=%.2f, avg=%.2f\n", min_rtt, max_rtt, avg_rtt);
    }
    else
    {
        printf("No packets received. RTT statistics unavailable.\n");
    }

    return exit_status;
}
