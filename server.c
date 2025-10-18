#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>

#define PORT 8080
#define MAX_CARTAS 40

typedef struct {
    char nome[3];
    char naipe[10]; // Exemplo: "Ouros", "Copas", etc.
} Carta;

Carta baralho[MAX_CARTAS] = {
    {"4","Ouros"},{"5","Ouros"},{"6","Ouros"},{"7","Ouros"},{"Q","Ouros"},{"J","Ouros"},{"K","Ouros"},{"A","Ouros"},{"2","Ouros"},{"3","Ouros"},
    {"4","Copas"},{"5","Copas"},{"6","Copas"},{"7","Copas"},{"Q","Copas"},{"J","Copas"},{"K","Copas"},{"A","Copas"},{"2","Copas"},{"3","Copas"},
    {"4","Espadas"},{"5","Espadas"},{"6","Espadas"},{"7","Espadas"},{"Q","Espadas"},{"J","Espadas"},{"K","Espadas"},{"A","Espadas"},{"2","Espadas"},{"3","Espadas"},
    {"4","Paus"},{"5","Paus"},{"6","Paus"},{"7","Paus"},{"Q","Paus"},{"J","Paus"},{"K","Paus"},{"A","Paus"},{"2","Paus"},{"3","Paus"}
};

int calcularForca(Carta c) {
    if (strcmp(c.nome,"4")==0 && strcmp(c.naipe,"Paus")==0) return 14; // Zap
    if (strcmp(c.nome,"7")==0 && strcmp(c.naipe,"Copas")==0) return 13;
    if (strcmp(c.nome,"A")==0 && strcmp(c.naipe,"Espadas")==0) return 12;
    if (strcmp(c.nome,"7")==0 && strcmp(c.naipe,"Ouros")==0) return 11;

    if (strcmp(c.nome,"3")==0) return 10;
    if (strcmp(c.nome,"2")==0) return 9;
    if (strcmp(c.nome,"A")==0) return 8;
    if (strcmp(c.nome,"K")==0) return 7;
    if (strcmp(c.nome,"J")==0) return 6;
    if (strcmp(c.nome,"Q")==0) return 5;
    if (strcmp(c.nome,"7")==0) return 4;
    if (strcmp(c.nome,"6")==0) return 3;
    if (strcmp(c.nome,"5")==0) return 2;
    if (strcmp(c.nome,"4")==0) return 1;
    return 0;
}

void embaralhar(int *cartas, int n) {
    for (int i = 0; i < n; i++) {
        int j = rand() % n;
        int temp = cartas[i];
        cartas[i] = cartas[j];
        cartas[j] = temp;
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, client_socket;
    struct sockaddr_in server, client;
    int c;
    char buffer[1024];
    int pontos_server = 0, pontos_client = 0;
    srand(time(NULL));

    WSAStartup(MAKEWORD(2,2), &wsa);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    listen(server_fd, 1);
    c = sizeof(struct sockaddr_in);
    client_socket = accept(server_fd, (struct sockaddr*)&client, &c);

    printf("Jogador conectado!\n");

    while (pontos_server < 12 && pontos_client < 12) {
        int cartas_idx[MAX_CARTAS];
        for (int i = 0; i < MAX_CARTAS; i++) cartas_idx[i] = i;
        embaralhar(cartas_idx, MAX_CARTAS);

        Carta mao_server[3], mao_client[3];
        for (int i = 0; i < 3; i++) {
            mao_server[i] = baralho[cartas_idx[i]];
            mao_client[i] = baralho[cartas_idx[i+3]];
        }

        char msg[256];
        sprintf(msg, "Suas cartas: %s de %s | %s de %s | %s de %s\n",
                mao_client[0].nome, mao_client[0].naipe,
                mao_client[1].nome, mao_client[1].naipe,
                mao_client[2].nome, mao_client[2].naipe);
        send(client_socket, msg, strlen(msg), 0);

        printf("Suas cartas: %s de %s | %s de %s | %s de %s\n",
               mao_server[0].nome, mao_server[0].naipe,
               mao_server[1].nome, mao_server[1].naipe,
               mao_server[2].nome, mao_server[2].naipe);

        int rodada = 1;
        int vitorias_server = 0, vitorias_client = 0;

        while (vitorias_server < 2 && vitorias_client < 2) {
            printf("\nRodada %d - Escolha sua carta (1-3): ", rodada);
            int esc_server;
            scanf("%d", &esc_server);
            esc_server--;

            sprintf(msg, "Rodada %d - Escolha sua carta (1-3): ", rodada);
            send(client_socket, msg, strlen(msg), 0);

            memset(buffer, 0, sizeof(buffer));
            recv(client_socket, buffer, sizeof(buffer), 0);
            int esc_client = atoi(buffer) - 1;

            Carta carta_s = mao_server[esc_server];
            Carta carta_c = mao_client[esc_client];

            int forca_s = calcularForca(carta_s);
            int forca_c = calcularForca(carta_c);

            char resultado[256];
            if (forca_s > forca_c) {
                vitorias_server++;
                sprintf(resultado, "Rodada %d: Você jogou %s de %s | Adversário jogou %s de %s -> Servidor ganhou a rodada!\n",
                        rodada, carta_c.nome, carta_c.naipe, carta_s.nome, carta_s.naipe);
            } else if (forca_s < forca_c) {
                vitorias_client++;
                sprintf(resultado, "Rodada %d: Você jogou %s de %s | Adversário jogou %s de %s -> Cliente ganhou a rodada!\n",
                        rodada, carta_c.nome, carta_c.naipe, carta_s.nome, carta_s.naipe);
            } else {
                sprintf(resultado, "Rodada %d: Cartas iguais! (%s de %s vs %s de %s) -> Empate!\n",
                        rodada, carta_c.nome, carta_c.naipe, carta_s.nome, carta_s.naipe);
            }

            send(client_socket, resultado, strlen(resultado), 0);
            printf("%s", resultado);
            rodada++;
        }

        if (vitorias_server > vitorias_client) pontos_server++;
        else pontos_client++;

        sprintf(msg, "\nPlacar: Servidor %d x %d Cliente\n", pontos_server, pontos_client);
        send(client_socket, msg, strlen(msg), 0);
        printf("%s", msg);
    }

    sprintf(buffer, "Fim de jogo! %s venceu!\n", (pontos_server >= 12 ? "Servidor" : "Cliente"));
    send(client_socket, buffer, strlen(buffer), 0);
    printf("%s", buffer);

    closesocket(client_socket);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
