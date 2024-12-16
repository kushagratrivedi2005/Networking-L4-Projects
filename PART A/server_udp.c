#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

char board[3][3];
int current_player = 1;  // 1 for Player 1 (X), 2 for Player 2 (O)

void init_board() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

void print_board(char *buffer) {
    sprintf(buffer, "   1   2   3\n"
                    " 1 %c | %c | %c\n"
                    "  ---|---|---\n"
                    " 2 %c | %c | %c\n"
                    "  ---|---|---\n"
                    " 3 %c | %c | %c\n",
            board[0][0], board[0][1], board[0][2],
            board[1][0], board[1][1], board[1][2],
            board[2][0], board[2][1], board[2][2]);
}

int check_winner() {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return current_player;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return current_player;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return current_player;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return current_player;
    return 0;
}

int check_draw() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ')
                return 0;
        }
    }
    return 1;
}

void reset_game() {
    init_board();
    current_player = 1;  // Reset to Player 1
}

void send_board_and_message(int socket, char *message, struct sockaddr_in *client_addr, socklen_t client_len) {
    char board_buffer[BUFFER_SIZE] = {0};
    print_board(board_buffer);
    char combined_message[BUFFER_SIZE];
    sprintf(combined_message, "Current Board:\n%s\n%s", board_buffer, message);
    sendto(socket, combined_message, strlen(combined_message), 0, (struct sockaddr *)client_addr, client_len);
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr[2];
    socklen_t client_len = sizeof(client_addr[0]);
    char buffer[BUFFER_SIZE] = {0};

    init_board();

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Receive connections from both players
    for (int i = 0; i < 2; i++) {
        recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[i], &client_len);
        printf("Player %d connected!\n", i + 1);
    }

    // Send "Are you ready to start?" to both players
    char ready_message[BUFFER_SIZE] = "Are you ready to start? (Y/N): ";
    for (int i = 0; i < 2; i++) {
        sendto(server_fd, ready_message, strlen(ready_message), 0, (struct sockaddr *)&client_addr[i], client_len);
    }

    // Read readiness responses
    char response[2]; // Store both players' responses
    for (int i = 0; i < 2; i++) {
        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[i], &client_len);
        response[i] = buffer[0]; // Store the response
    }

    // Check readiness
    if (response[0] != 'Y' && response[1] != 'Y') {
        printf("Both players are not ready. Exiting.\n");
        close(server_fd);
        return 0;
    } else if (response[0] != 'Y' || response[1] != 'Y') {
        for (int i = 0; i < 2; i++) {
            if (response[i] != 'Y') {
                printf("Player %d is not ready. Exiting.\n", i + 1);
                for (int j = 0; j < 2; j++) {
                char end_message[] = "Game over. Server closing.";
                sendto(server_fd, end_message, strlen(end_message), 0, (struct sockaddr *)&client_addr[j], client_len);
             }
            }
        }
        close(server_fd);
        return 0;
    }

    printf("Both players are ready. Starting the game...\n");

    // Main game loop
    while (1) {
        int game_over = 0;
        while (!game_over) {
            for (int i = 0; i < 2; i++) {
                if (i + 1 == current_player) {
                    send_board_and_message(server_fd, "Your turn. Enter row and column (1-3): ", &client_addr[i], client_len);
                } else {
                    send_board_and_message(server_fd, "Waiting for the other player.", &client_addr[i], client_len);
                }
            }

            // Receive player's move
            memset(buffer, 0, BUFFER_SIZE);
            recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[current_player - 1], &client_len);

            int row, col;
            if (sscanf(buffer, "%d %d", &row, &col) != 2 || row < 1 || col < 1 || row > 3 || col > 3) {
                continue;
            }

            row--; col--;  // Adjust to 0-based indexing

            // Validate the move
            if (board[row][col] == ' ') {
                board[row][col] = (current_player == 1) ? 'X' : 'O';

                // Check for winner
                if (check_winner()) {
                    game_over = 1;
                    char win_message[BUFFER_SIZE];
                    sprintf(win_message, "Player %d Wins!\nDo you want to play again? (Y/N): ", current_player);

                    for (int i = 0; i < 2; i++) {
                        send_board_and_message(server_fd, win_message, &client_addr[i], client_len);
                    }
                    break;
                } else if (check_draw()) {
                    game_over = 1;
                    char draw_message[BUFFER_SIZE] = "It's a Draw!\nDo you want to play again? (Y/N): ";

                    for (int i = 0; i < 2; i++) {
                        send_board_and_message(server_fd, draw_message, &client_addr[i], client_len);
                    }
                    break;
                } else {
                    current_player = 3 - current_player;  // Switch player
                }
            } 
        }

        // Ask if players want to play again simultaneously
        char replay_response[2];

        for (int i = 0; i < 2; i++) {
            memset(buffer, 0, BUFFER_SIZE);
            recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[i], &client_len);
            replay_response[i] = buffer[0];
        }

        if (replay_response[0] == 'Y' && replay_response[1] == 'Y') {
            reset_game();
            continue;
        } else {
            break;  // Exit the main game loop
        }
    }

   

printf("Game has ended.\n");
    for (int i = 0; i < 2; i++) {
        {
            char end_message[] = "Game over. Server closing.";
            sendto(server_fd, end_message, strlen(end_message), 0, (struct sockaddr *)&client_addr[i], client_len);
        }
    }
close(server_fd);
    return 0;
}
