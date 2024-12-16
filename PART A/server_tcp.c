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

void send_board_and_message(int socket, char *message) {
    char board_buffer[BUFFER_SIZE] = {0};
    print_board(board_buffer);
    char combined_message[BUFFER_SIZE];
    sprintf(combined_message, "Current Board:\n%s\n%s", board_buffer, message);
    send(socket, combined_message, strlen(combined_message), 0);
}

void handle_invalid_move(int socket) {
    char error_message[BUFFER_SIZE] = "Invalid move, try again.\n";
    send(socket, error_message, strlen(error_message), 0);
}

int main() {
    int server_fd, new_socket[2], valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    init_board();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Accept connections for both players
    for (int i = 0; i < 2; i++) {
        if ((new_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        printf("Player %d connected!\n", i + 1);
    }

    // Send "Are you ready to start?" to both players immediately
    char ready_message[BUFFER_SIZE] = "Are you ready to start? (Y/N): ";
    for (int i = 0; i < 2; i++) {
        send(new_socket[i], ready_message, strlen(ready_message), 0);
    }

    // Read readiness responses
    for (int i = 0; i < 2; i++) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(new_socket[i], buffer, BUFFER_SIZE);

        // If either player isn't ready, close connections
        if (valread < 0 || buffer[0] != 'Y') {
            printf("Player %d is not ready. Exiting.\n", i + 1);
            for (int j = 0; j < 2; j++) {
                close(new_socket[j]);
            }
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("Both players are ready. Starting the game...\n");

    // Main game loop
    while (1) {
    int game_over = 0;
    while (!game_over) {
        // Send board and turn messages to both players
        for (int i = 0; i < 2; i++) {
            if (i + 1 == current_player) {
                send_board_and_message(new_socket[i], "Your turn. Enter row and column (1-3): ");
            } else {
                send_board_and_message(new_socket[i], "Waiting for the other player.");
            }
        }

        // Receive player's move
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(new_socket[current_player - 1], buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("Read failed");
            break;
        }

        int row, col;
        if (sscanf(buffer, "%d %d", &row, &col) != 2 || row < 1 || col < 1 || row > 3 || col > 3) {
            handle_invalid_move(new_socket[current_player - 1]);
            continue;
        }

        row--; col--;  // Adjust to 0-based indexing

        // Validate the move
        if (board[row][col] == ' ') {
            board[row][col] = (current_player == 1) ? 'X' : 'O';

            // Check for winner
            if (check_winner() && !game_over) {
                game_over = 1;
                char win_message[BUFFER_SIZE];
                sprintf(win_message, "Player %d Wins!\nDo you want to play again? (Y/N): ", current_player);

                 // Send final board and message to both players
                for (int i = 0; i < 2; i++) {
                    send_board_and_message(new_socket[i], win_message);
                    fflush(stdout);
                }
                break;  // Break the loop immediately after a win
            } else if (check_draw()) {
                game_over = 1;
                char draw_message[BUFFER_SIZE] = "It's a Draw!\n";

                // Send final board and message to both players
                for (int i = 0; i < 2; i++) {
                    send_board_and_message(new_socket[i], draw_message);
                    fflush(stdout);
                }
                break;  // Break the loop immediately after a draw
            } else {
                current_player = 3 - current_player;  // Switch player
            }
        } else {
            handle_invalid_move(new_socket[current_player - 1]);
        }
    }

    // Ask if players want to play again simultaneously
    char response[2];

    for (int i = 0; i < 2; i++) {
        memset(buffer, 0, BUFFER_SIZE);
        read(new_socket[i], buffer, BUFFER_SIZE);
        response[i] = buffer[0];  // Assuming a single character response
    }

    // Handle players' responses
    if (response[0] == 'Y' && response[1] == 'Y') {
        reset_game();
        continue;
    } else if (response[0] == 'N' && response[1] == 'N') {
        printf("Both players have chosen to exit. Closing the server.\n");
        break;
    } else {
        if (response[0] == 'Y') {
            send(new_socket[1], "Player 1 wants to play again. Waiting for Player 2...\n", 58, 0);
        } else if (response[1] == 'Y') {
            send(new_socket[0], "Player 2 wants to play again. Waiting for Player 1...\n", 58, 0);
        }
        printf("One player wants to continue while the other does not.\n");
        break;  // Exit the game loop but keep connections open for further communication
    }
}



// Cleanup
for (int i = 0; i < 2; i++) {
    close(new_socket[i]);
}
close(server_fd);
return 0;}