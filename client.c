#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080

void print_board(const char board[6][7]) {
    printf("\n");
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            printf("| %c ", board[row][col]);
        }
        printf("|\n");
    }
    printf("-----------------------------\n");
    printf("  1   2   3   4   5   6   7\n");
    printf("-----------------------------\n");
}

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char message[256];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server.\n");

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, sizeof(buffer));

        if (strcmp(buffer, "WIN") == 0) {
            printf("You won the game!\n");
            break;
        } else if (strcmp(buffer, "DRAW") == 0) {
            printf("The game ended in a draw.\n");
            break;
        } else if (strcmp(buffer, "INVALID") == 0) {
            printf("Invalid move. Please try again.\n");
        } else if (strcmp(buffer, "SUCCESS") == 0) {
            printf("Waiting for the other player...\n");
        } else {
            strcpy(message, buffer);
            print_board(message);
            printf("Your turn. Enter the column number (1-7): ");

            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            if (write(sock, buffer, strlen(buffer)) < 0) {
                printf("Send failed\n");
                return -1;
            }
        }
    }

    close(sock);

    return 0;
}
