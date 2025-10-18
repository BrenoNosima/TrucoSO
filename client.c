#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[1024];

    WSAStartup(MAKEWORD(2,2), &wsa);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Erro ao conectar.\n");
        return 1;
    }

    printf("Conectado ao servidor!\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        printf("%s\n", buffer);

        if (strstr(buffer, "Fim de jogo")) break;
        if (strstr(buffer, "Escolha sua carta")) {
            printf("> ");
            fgets(buffer, sizeof(buffer), stdin);
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
