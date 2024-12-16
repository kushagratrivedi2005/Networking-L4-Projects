#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void clear_screen() {
    printf("\033[H\033[J");  // ANSI escape code to clear the terminal
}

int main(int argc, char* argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    char* ip;
    if(argc == 2){
        ip = argv[1];
    }
    else{
        ip = "127.0.0.1";
    }
    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection failed\n");
        return -1;
    }

    // Game loop
    while (1) {
        // Clear screen for updated display
        clear_screen();

        // Read data from server (board state, messages, etc.)
            char response[2];
        memset(buffer, 0, BUFFER_SIZE);  // Clear buffer to avoid old data
        int valread = read(sock, buffer, BUFFER_SIZE);

        if (valread <= 0) {
            if(response[0]=='Y'){printf("Your opponent does not wish to play\n");}
            printf("Connection closed by server\n");
            break;
        }

        // Print the server's message (includes the board and current player's turn)
        if(strstr(buffer, "Do you want to play again?") == NULL)
        printf("%s\n", buffer);

        // Handle the "Are you ready to start?" prompt
        if (strstr(buffer, "Are you ready to start?") != NULL) {
            char response[2];
            scanf(" %c", &response[0]);

            // Send the response to the server
            send(sock, response, strlen(response), 0);

            // If the player is not ready, exit the game
            if (response[0] == 'n' || response[0] == 'N') {
                printf("You chose not to play. Exiting...\n");
                break;
            }
        }
     // Handle the "Do you want to play again?" prompt
        else if (strstr(buffer, "Do you want to play again?") != NULL) {
            printf("%s", buffer); // Print the server message

            // Clear the input buffer
            while (getchar() != '\n'); // Clear out any remaining input in the buffer
            scanf(" %c", &response[0]); // Read the player's response

            // Send the response to the server
            send(sock, response, 1, 0); // Send only one character

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
                while (!(strlen(row) == 1 && strlen(col) == 1 && row[0] >= '1' && row[0] <= '3' && col[0] >= '1' && col[0] <= '3')) {
                    clear_screen();  // Clear the screen after invalid input
                    printf("Invalid input: please enter numbers between 1 and 3\n");
                    printf("%s\n", buffer);  // Print the prompt again
                    scanf("%s %s", row, col);  // Take new input
                }

                // Convert row and col to integers and send the move to the server
                char move[BUFFER_SIZE];
                sprintf(move, "%d %d", row[0] - '0', col[0] - '0');  // Convert char to int
                send(sock, move, strlen(move), 0);
            }
        }

    // Close the socket after the game ends
    close(sock);
    return 0;
}
