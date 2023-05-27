#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 2

char board[6][7];
int player_turn;
int client_sockets[MAX_CLIENTS];
pthread_t thread_id[MAX_CLIENTS];

void initialize_board() {
    memset(board, ' ', sizeof(board));
}

bool is_valid_move(int column) {
    return (column >= 0 && column < 7 && board[0][column] == ' ');
}

void make_move(int column) {
    int row;
    for (row = 5; row >= 0; row--) {
        if (board[row][column] == ' ') {
            board[row][column] = (player_turn == 0) ? 'X' : 'O';
            break;
        }
    }
}

bool check_winner() {
    // Check rows for a win
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 4; col++) {
            if (board[row][col] != ' ' && board[row][col] == board[row][col + 1] &&
                board[row][col] == board[row][col + 2] && board[row][col] == board[row][col + 3]) {
                return true;
            }
        }
    }

    // Check columns for a win
    for (int col = 0; col < 7; col++) {
        for (int row = 0; row < 3; row++) {
            if (board[row][col] != ' ' && board[row][col] == board[row + 1][col] &&
                board[row][col] == board[row + 2][col] && board[row][col] == board[row + 3][col]) {
                return true;
            }
        }
    }

    // Check diagonals (top-left to bottom-right) for a win
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            if (board[row][col] != ' ' && board[row][col] == board[row + 1][col + 1] &&
                board[row][col] == board[row + 2][col + 2] && board[row][col] == board[row + 3][col + 3]) {
                return true;
            }
        }
    }

    // Check diagonals (top-right to bottom-left) for a win
    for (int row = 0; row < 3; row++) {
        for (int col = 3; col < 7; col++) {
            if (board[row][col] != ' ' && board[row][col] == board[row + 1][col - 1] &&
                board[row][col] == board[row + 2][col - 2] && board[row][col] == board[row + 3][col - 3]) {
                return true;
            }
        }
    }

    return false;
}

bool is_board_full() {
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            if (board[row][col] == ' ') {
                return false;
            }
        }
    }
    return true;
}

void send_message_to_clients(const char* message) {
    char board_message[1024];

    // Append the board state to the message
    snprintf(board_message, sizeof(board_message), "%s\n", message);
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            snprintf(board_message, sizeof(board_message), "%s%c ", board_message, board[row][col]);
        }
        snprintf(board_message, sizeof(board_message), "%s\n", board_message);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            write(client_sockets[i], board_message, strlen(board_message));
        }
    }
}


void* handle_client(void* arg) {
    int player_id = *(int*)arg;
    int client_socket = client_sockets[player_id];

    char message[1024];

    while (true) {
        memset(message, 0, sizeof(message));
        int valread = read(client_socket, message, sizeof(message));

        if (valread <= 0) {
            printf("Player %d disconnected.\n", player_id + 1);
            close(client_socket);
            client_sockets[player_id] = 0;
            pthread_exit(NULL);
        }

        int column = atoi(message) - 1;
        if (is_valid_move(column)) {
            make_move(column);

            if (check_winner()) {
                send_message_to_clients("WIN");
                printf("Player %d won the game!\n", player_id + 1);
                break;
            } else if (is_board_full()) {
                send_message_to_clients("DRAW");
                printf("The game ended in a draw.\n");
                break;
            } else {
                player_turn = (player_turn + 1) % MAX_CLIENTS;
                send_message_to_clients("SUCCESS");
            }
        } else {
            send_message_to_clients("INVALID");
        }
    }

    initialize_board();
    player_turn = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            close(client_sockets[i]);
            client_sockets[i] = 0;
        }
    }

    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    initialize_board();
    player_turn = 0;

    memset(client_sockets, 0, sizeof(client_sockets));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        client_sockets[i] = new_socket;
        printf("Player %d connected.\n", i + 1);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int* player_id = malloc(sizeof(int));
        *player_id = i;
        pthread_create(&thread_id[i], NULL, handle_client, (void*)player_id);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(thread_id[i], NULL);
    }

    return 0;
}
