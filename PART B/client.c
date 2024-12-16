#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 5
#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 100000  // 0.1 seconds for retransmission timeout
#define MAX_CHUNK 1000

// Struct to represent a data chunk with sequence number and data
struct DataChunk {
    int sequence_number;
    int total_chunks;
    char data[CHUNK_SIZE];
};

// Struct for acknowledgment packets
struct AckPacket {
    int sequence_number;
};

// Function to set a socket to non-blocking mode
void set_socket_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        exit(EXIT_FAILURE);
    }
}

// Function to send data where sending chunks is independent of receiving ACKs
void send_data_with_retransmission(int sockfd, struct sockaddr_in *addr, socklen_t addr_len, char *text, int text_len) {
    int total_chunks = (text_len + CHUNK_SIZE - 1) / CHUNK_SIZE;
    printf("Number of chunks to send: %d\n\n", total_chunks);
    struct DataChunk chunks[total_chunks];
    struct AckPacket ack;
    int ack_received[total_chunks];  // Track whether each chunk has been acknowledged
    memset(ack_received, 0, sizeof(ack_received));  // Initially, no chunk is acknowledged

    // Prepare the data chunks
    for (int i = 0; i < total_chunks; ++i) {
        chunks[i].sequence_number = i + 1;
        chunks[i].total_chunks = total_chunks;

        int chunk_len = (i == total_chunks - 1) ? (text_len % CHUNK_SIZE == 0 ? CHUNK_SIZE : text_len % CHUNK_SIZE) : CHUNK_SIZE;
        memset(chunks[i].data, ' ', CHUNK_SIZE); // Fill with spaces
        memcpy(chunks[i].data, text + i * CHUNK_SIZE, chunk_len);
    }

    fd_set read_fds;
    struct timeval timeout;

    // Send all chunks without waiting for ACKs
    for (int i = 0; i < total_chunks; ++i) {
        sendto(sockfd, &chunks[i], sizeof(chunks[i]), 0, (struct sockaddr *)addr, addr_len);
        printf("Sent chunk %d\n", chunks[i].sequence_number);
    }

    // Check for ACKs and retransmit if needed
    int chunks_pending = total_chunks;
    while (chunks_pending > 0) {
        // Set up select() to wait for ACK
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;  // 0.1 seconds for timeout

        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        // Process any ACKs received
        if (activity > 0 && FD_ISSET(sockfd, &read_fds)) {
            int bytes_received = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
            if (bytes_received > 0 && ack.sequence_number > 0 && ack.sequence_number <= total_chunks) {
                int ack_idx = ack.sequence_number - 1;
                if (!ack_received[ack_idx]) {
                    ack_received[ack_idx] = 1;  // Mark this chunk as acknowledged
                    chunks_pending--;
                    printf("Received ACK for chunk %d\n", ack.sequence_number);
                }
            }
        } else if (activity == 0) { // No ACKs received after the timeout
            // Retransmit any unacknowledged chunks
            for (int i = 0; i < total_chunks; ++i) {
                if (!ack_received[i]) {
                    sendto(sockfd, &chunks[i], sizeof(chunks[i]), 0, (struct sockaddr *)addr, addr_len);
                    printf("Retransmitting chunk %d\n", chunks[i].sequence_number);
                }
            }
        }
    }

    printf("All chunks sent and acknowledged\n");
}

// Function to receive data and send ACKs
void receive_data(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    struct DataChunk chunks[MAX_CHUNK];  // Assuming a max of MAX_CHUNK chunks
    int received_chunks = 0;
    int total_chunks = 0;

        int count = 0;
    while (received_chunks < total_chunks || total_chunks == 0) {
        struct DataChunk chunk;
        int bytes_received = recvfrom(sockfd, &chunk, sizeof(chunk), 0, (struct sockaddr *)client_addr, &addr_len);
        if (bytes_received > 0) {
            if (total_chunks == 0) {
                total_chunks = chunk.total_chunks;
            }
            count++;
            if(count%5==0){
                // flag = 0;
                continue;
            }
            chunks[chunk.sequence_number - 1] = chunk;  // Store the chunk
            received_chunks++;

            // Send ACK for the received chunk
            struct AckPacket ack;
            ack.sequence_number = chunk.sequence_number;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)client_addr, addr_len);
        }
    }

    printf("\n\nnumber of chunks received: %d\nreceived message: ", total_chunks);
    for (int i = 0; i < total_chunks; ++i) {
        char chunk_buffer[CHUNK_SIZE + 1]; // +1 for null terminator
        memcpy(chunk_buffer, chunks[i].data, CHUNK_SIZE); // Copy the chunk data
        chunk_buffer[CHUNK_SIZE] = '\0'; // Null-terminate the buffer
        printf("%s",chunk_buffer); // Print the current chunk
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    char* ip;
    if(argc == 2){
        ip = argv[1];
    }
    else{
        ip = "127.0.0.1";
    }

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    // server_addr.sin_addr.s_addr = INADDR_ANY;

    // Set the socket to non-blocking mode
    set_socket_non_blocking(sockfd);

    // Send a message to initiate communication
    strcpy(buffer, "Client has connected");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, addr_len);

    // Wait for server acknowledgment
    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null-terminate the message
            printf("connected to the server\n");
            break;
        }
    }

    printf("Client is ready to send and receive messages...\n");
    printf("Enter message to send: ");
    fflush(stdout);

    // Main loop for sending and receiving messages
    fd_set read_fds;
    int max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        // Use select to wait for data on either the socket or standard input
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        fflush(stdout);

        if (activity > 0) {
            // Check if there's data on the socket
            if (FD_ISSET(sockfd, &read_fds)) {
                receive_data(sockfd, &client_addr, addr_len);
                printf("Enter message to send: ");
                fflush(stdout);
            }

            // Check if there's input from the user
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                fgets(buffer, BUFFER_SIZE, stdin);
                int text_len = strlen(buffer);
                send_data_with_retransmission(sockfd, &server_addr, addr_len, buffer, text_len);
                printf("Enter message to send: ");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    return 0;
}
