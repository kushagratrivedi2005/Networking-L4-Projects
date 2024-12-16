#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
char board[3][3];

void clear_screen() {
    printf("\033[H\033[J");  // ANSI escape code to clear the terminal
}

int check_empty(int x,int y){
    x--;y--;
    if(board[x-'0'][y-'0']=='X'){return 0;}
    if(board[x-'0'][y-'0']=='O'){return 0;}
    return 1;
}
char response[2];
int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(serv_addr);

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));

    // Fill server information
    serv_addr.sin_family = AF_INET;
    char* ip;
    if(argc == 2){
        ip = argv[1];
    }
    else{
        ip = SERVER_IP;
    }
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(PORT);

    // Send initial message to server to indicate connection
    char *init_message = "Player ready!";
    sendto(sock, init_message, strlen(init_message), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Game loop
    while (1) {
        // Clear screen for updated display
        clear_screen();

        // Read data from server (board state, messages, etc.)
        memset(buffer, 0, BUFFER_SIZE);  // Clear buffer to avoid old data

        // Use recvfrom to receive messages from the server
        int valread = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serv_addr, &addr_len);

        if (valread < 0) {
            perror("recvfrom error"); // Use perror to get detailed error information
            break;
        }
        if (valread > 0) {
            // Assuming the board data starts at a fixed position in the buffer
            int start_of_board = 31;  // The first character of the board in the message

            // Extract the board data from the received message
            int k = start_of_board;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    board[i][j] = buffer[k];
                    k += 4;  // Move to the next cell, skipping the separator characters like '|'
                }
                k += 15;  // Skip to the next row (past the row separator line)
            }
        }
         if (strstr(buffer, "Game over") != NULL) {
            if(response[0]=='Y'){printf("Your opponent does not wish to play. Exiting\n");break;}
        printf("The server has closed the connection. Exiting...\n");
        break;  // Exit the game loop
    }
        // Print the server's message (includes the board and current player's turn)
        printf("%s\n", buffer);

        // Handle the "Are you ready to start?" prompt
        if (strstr(buffer, "Are you ready to start?") != NULL) {
            scanf(" %c", &response[0]);

            // Send the response to the server
            sendto(sock, response, 1, 0, (struct sockaddr *)&serv_addr, addr_len);

            // If the player is not ready, exit the game
            if (response[0] == 'n' || response[0] == 'N') {
                printf("You chose not to play. Exiting...\n");
                break;
            }
        }
        // Handle the "Do you want to play again?" prompt
        else if (strstr(buffer, "Do you want to play again?") != NULL) {
            // Clear the input buffer
            while (getchar() != '\n'); // Clear out any remaining input in the buffer
            scanf(" %c", &response[0]); // Read the player's response

            // Send the response to the server
            sendto(sock, response, 1, 0, (struct sockaddr *)&serv_addr, addr_len); // Send only one character

            // Exit if the player chooses 'n' or 'N'
            if (response[0] == 'n' || response[0] == 'N') {
                printf("You chose not to play. Exiting...\n");
                break;
            }
        }

        // Handle the move input for the game (row and column)
        else if (strstr(buffer, "Enter row and column") != NULL) {
            char row[1024], col[1024];

            // Take input from the user
            scanf("%s %s", row, col);

            // Loop until valid input (length of 1 and between '1' and '3')
            while (!(strlen(row) == 1 && strlen(col) == 1 && row[0] >= '1' && row[0] <= '3' && col[0] >= '1' && col[0] <= '3' 
            && check_empty(row[0],col[0]))) {
                clear_screen();  // Clear the screen after invalid input
                if(check_empty(row[0],col[0]))
                printf("Invalid input: please enter numbers between 1 and 3\n");
                else{printf("Invalid input: the cell is already filled\n");}
                printf("%s\n", buffer);  // Print the prompt again
                scanf("%s %s", row, col);  // Take new input
            }

            // Convert row and col to integers and send the move to the server
            char move[BUFFER_SIZE];
            sprintf(move, "%d %d", row[0] - '0', col[0] - '0');  // Convert char to int
            sendto(sock, move, strlen(move), 0, (struct sockaddr *)&serv_addr, addr_len);
        }
    }

    // Close the socket after the game ends
    close(sock);
    return 0;
}
