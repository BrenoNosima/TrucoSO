// client.c
// Truco Mineiro - Cliente (Jogador 2)
// Correcoes: terminação de buffers apos recv, uso do mutex para ler memoria compartilhada,
// mensagens sem acentos para evitar problemas de encoding.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define MEM_NAME "Local\\TrucoMem"
#define MUTEX_NAME "Local\\TrucoMutex"

typedef struct {
    int vez;
    int pontos_server;
    int pontos_client;
    char ultima_jogada[64];
    char status[128];
} SharedMem;

int main() {
    WSADATA wsa;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // --- inicializa Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup falhou\n");
        return 1;
    }

    // --- mapear memoria compartilhada (mesmo nome do servidor) ---
    HANDLE hMap = NULL;
    // tentativa de abrir a memoria mapeada (o servidor deve rodar primeiro)
    int attempts = 0;
    while (attempts < 5) {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MEM_NAME);
        if (hMap != NULL) break;
        attempts++;
        Sleep(200); // esperar um pouco
    }
    if (hMap == NULL) {
        printf("Nao encontrou memoria compartilhada. Inicie o servidor primeiro.\n");
        WSACleanup();
        return 1;
    }

    SharedMem *mem = (SharedMem*) MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMem));
    if (mem == NULL) {
        printf("Erro MapViewOfFile (cliente): %lu\n", GetLastError());
        CloseHandle(hMap);
        WSACleanup();
        return 1;
    }

    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, MUTEX_NAME);
    if (hMutex == NULL) {
        printf("Erro OpenMutex: %lu\n", GetLastError());
        UnmapViewOfFile(mem);
        CloseHandle(hMap);
        WSACleanup();
        return 1;
    }

    // --- conectar ao servidor ---
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Erro ao conectar.\n");
        UnmapViewOfFile(mem);
        CloseHandle(hMap);
        CloseHandle(hMutex);
        WSACleanup();
        return 1;
    }

    printf("Conectado ao servidor!\n");

    // loop principal: recebe mensagens do servidor e responde quando solicitado
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, (int)sizeof(buffer)-1, 0);
        if (bytes <= 0) break;

        // garantir terminação correta
        buffer[bytes] = '\0';
        // remover CRLF no fim (se houver)
        buffer[strcspn(buffer, "\r\n")] = 0;

        // imprime mensagem do servidor
        printf("%s\n", buffer);

        // se a mensagem contem "Fim de jogo", sai
        if (strstr(buffer, "Fim de jogo")) break;

        // se o servidor solicitou escolha da carta, jogue
        if (strstr(buffer, "Escolha sua carta")) {
            printf("> ");
            char entrada[64];
            if (!fgets(entrada, sizeof(entrada), stdin)) break;
            // envia escolha pro servidor
            char envio[64];
            // limpa newline
            entrada[strcspn(entrada, "\r\n")] = 0;
            sprintf(envio, "%s", entrada);
            send(sock, envio, (int)strlen(envio), 0);

            // atualizar memoria compartilhada indicando que cliente escolheu
            WaitForSingleObject(hMutex, INFINITE);
            mem->vez = 1; // volta a vez ao servidor apos enviar
            snprintf(mem->ultima_jogada, sizeof(mem->ultima_jogada), "Cliente escolheu carta indice %s", envio);
            snprintf(mem->status, sizeof(mem->status), "Cliente jogou (escolha enviada)");
            ReleaseMutex(hMutex);
        }

        // opcional: se recebeu placar ou status, podemos mostrar o estado compartilhado
        if (strstr(buffer, "Placar") || strstr(buffer, "Partida vencida") || strstr(buffer, "Rodada")) {
            // pegar snapshot da memoria compartilhada
            WaitForSingleObject(hMutex, INFINITE);
            printf("[MEM PARTILHADA] vez=%d | pontos_server=%d | pontos_client=%d\n",
                   mem->vez, mem->pontos_server, mem->pontos_client);
            printf("[MEM PARTILHADA] ultima_jogada=%s\n", mem->ultima_jogada);
            printf("[MEM PARTILHADA] status=%s\n", mem->status);
            ReleaseMutex(hMutex);
        }
    }

    // cleanup
    closesocket(sock);
    UnmapViewOfFile(mem);
    CloseHandle(hMap);
    CloseHandle(hMutex);
    WSACleanup();
    return 0;
}
