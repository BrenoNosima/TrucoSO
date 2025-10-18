// server.c
// Truco Mineiro - Servidor (Jogador 1)
// Correcoes: terminação de buffers, inicializacao da memoria compartilhada,
// mensagens sem acentos (pra evitar problemas de encoding no cmd), mutex.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib") // link winsock

#define PORT 8080
#define MAX_CARTAS 40
#define MEM_NAME "Local\\TrucoMem"    // nome do segmento de memoria compartilhada
#define MUTEX_NAME "Local\\TrucoMutex"// nome do mutex para sincronizar acesso

// ----- Tipos e baralho -----
typedef struct {
    char nome[3];
    char naipe[10];
} Carta;

Carta baralho[MAX_CARTAS] = {
    {"4","Ouros"},{"5","Ouros"},{"6","Ouros"},{"7","Ouros"},{"Q","Ouros"},{"J","Ouros"},{"K","Ouros"},{"A","Ouros"},{"2","Ouros"},{"3","Ouros"},
    {"4","Copas"},{"5","Copas"},{"6","Copas"},{"7","Copas"},{"Q","Copas"},{"J","Copas"},{"K","Copas"},{"A","Copas"},{"2","Copas"},{"3","Copas"},
    {"4","Espadas"},{"5","Espadas"},{"6","Espadas"},{"7","Espadas"},{"Q","Espadas"},{"J","Espadas"},{"K","Espadas"},{"A","Espadas"},{"2","Espadas"},{"3","Espadas"},
    {"4","Paus"},{"5","Paus"},{"6","Paus"},{"7","Paus"},{"Q","Paus"},{"J","Paus"},{"K","Paus"},{"A","Paus"},{"2","Paus"},{"3","Paus"}
};

// estrutura que ficara na memoria compartilhada
typedef struct {
    int vez;                    // 1 = servidor, 2 = cliente
    int pontos_server;
    int pontos_client;
    char ultima_jogada[64];     // string descrevendo a ultima jogada
    char status[128];           // mensagem de status (ex: resultado da rodada)
} SharedMem;

int calcularForca(Carta c) {
    // mesma tabela de forca do seu codigo
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
    // --- inicializacao Winsock ---
    WSADATA wsa;
    SOCKET server_fd = INVALID_SOCKET, client_socket = INVALID_SOCKET;
    struct sockaddr_in server, client;
    int c;
    char buffer[1024];

    srand((unsigned)time(NULL));

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup falhou\n");
        return 1;
    }

    // --- criar memoria compartilhada e mutex ---
    BOOL createdNew = TRUE;
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMem), MEM_NAME);
    if (hMap == NULL) {
        printf("Erro CreateFileMapping: %lu\n", GetLastError());
        WSACleanup();
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) createdNew = FALSE;

    SharedMem *mem = (SharedMem*) MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMem));
    if (mem == NULL) {
        printf("Erro MapViewOfFile: %lu\n", GetLastError());
        CloseHandle(hMap);
        WSACleanup();
        return 1;
    }

    // inicializa mutex nomeado para sincronizar acesso a memoria
    HANDLE hMutex = CreateMutexA(NULL, FALSE, MUTEX_NAME);
    if (hMutex == NULL) {
        printf("Erro CreateMutex: %lu\n", GetLastError());
        UnmapViewOfFile(mem);
        CloseHandle(hMap);
        WSACleanup();
        return 1;
    }

    // se a memoria foi criada agora, zera e inicializa
    WaitForSingleObject(hMutex, INFINITE);
    if (createdNew) {
        memset(mem, 0, sizeof(SharedMem));
        mem->vez = 1; // servidor comeca por convencao
        mem->pontos_server = 0;
        mem->pontos_client = 0;
        strcpy(mem->ultima_jogada, "nenhuma");
        strcpy(mem->status, "aguardando inicio");
    } else {
        // se ja existia, so garantir campos validos (nao sobrescrever)
        if (mem->vez != 1 && mem->vez != 2) mem->vez = 1;
    }
    ReleaseMutex(hMutex);

    // --- criar socket, bind e listen ---
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Erro ao criar socket\n");
        goto cleanup;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("bind falhou: %d\n", WSAGetLastError());
        goto cleanup;
    }

    listen(server_fd, 1);
    printf("Aguardando conexao na porta %d...\n", PORT);
    c = sizeof(struct sockaddr_in);
    client_socket = accept(server_fd, (struct sockaddr*)&client, &c);
    if (client_socket == INVALID_SOCKET) {
        printf("accept falhou\n");
        goto cleanup;
    }
    printf("Jogador conectado!\n");

    // laco principal do jogo (placar ate 12)
    while (1) {
        // ler placar da memoria compartilhada para decidir se continua
        WaitForSingleObject(hMutex, INFINITE);
        int pontos_server = mem->pontos_server;
        int pontos_client = mem->pontos_client;
        ReleaseMutex(hMutex);
        if (pontos_server >= 12 || pontos_client >= 12) break;

        // embaralhar e distribuir
        int cartas_idx[MAX_CARTAS];
        for (int i = 0; i < MAX_CARTAS; i++) cartas_idx[i] = i;
        embaralhar(cartas_idx, MAX_CARTAS);

        Carta mao_server[3], mao_client[3];
        for (int i = 0; i < 3; i++) {
            mao_server[i] = baralho[cartas_idx[i]];
            mao_client[i] = baralho[cartas_idx[i+3]];
        }

        // envia as cartas do cliente (sempre envie de forma clara)
        char msg[512];
        sprintf(msg, "Suas cartas: %s de %s | %s de %s | %s de %s\n",
                mao_client[0].nome, mao_client[0].naipe,
                mao_client[1].nome, mao_client[1].naipe,
                mao_client[2].nome, mao_client[2].naipe);
        send(client_socket, msg, (int)strlen(msg), 0);

        // mostra as cartas do servidor no console
        printf("Suas cartas (servidor): %s de %s | %s de %s | %s de %s\n",
               mao_server[0].nome, mao_server[0].naipe,
               mao_server[1].nome, mao_server[1].naipe,
               mao_server[2].nome, mao_server[2].naipe);

        int rodada = 1;
        int vitorias_server = 0, vitorias_client = 0;

        // cada partida e melhor de 3 (quem ganha 2 leva a partida)
        while (vitorias_server < 2 && vitorias_client < 2) {
            // 1) servidor escolhe carta
            int esc_server = -1;
            while (esc_server < 0 || esc_server > 2) {
                printf("\nRodada %d - Escolha sua carta (1-3): ", rodada);
                if (scanf("%d", &esc_server) != 1) {
                    // limpeza se input invalido
                    while (getchar() != '\n');
                    esc_server = -1;
                    continue;
                }
                esc_server--; // de 1..3 para 0..2
            }

            // 2) avisar cliente que e sua vez de escolher (e atualizar memoria compartilhada)
            char prompt[64];
            sprintf(prompt, "Rodada %d - Escolha sua carta (1-3): ", rodada);
            send(client_socket, prompt, (int)strlen(prompt), 0);

            // atualiza memoria compartilhada: vez = 2 (cliente escolhe)
            WaitForSingleObject(hMutex, INFINITE);
            mem->vez = 2;
            snprintf(mem->ultima_jogada, sizeof(mem->ultima_jogada), "Servidor escolheu (oculto) - aguardando cliente");
            snprintf(mem->status, sizeof(mem->status), "rodada %d: aguardando escolha do cliente", rodada);
            ReleaseMutex(hMutex);

            // 3) receber escolha do cliente
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(client_socket, buffer, (int)sizeof(buffer)-1, 0);
            if (bytes <= 0) {
                printf("Cliente desconectou ou erro recv\n");
                goto cleanup;
            }
            // garantir terminação correta
            buffer[bytes] = '\0';
            // remover CR/LF
            buffer[strcspn(buffer, "\r\n")] = '\0';
            int esc_client = atoi(buffer) - 1;
            if (esc_client < 0 || esc_client > 2) esc_client = 0; // fallback

            // montar cartas escolhidas
            Carta carta_s = mao_server[esc_server];
            Carta carta_c = mao_client[esc_client];

            int forca_s = calcularForca(carta_s);
            int forca_c = calcularForca(carta_c);

            char resultado[512];
            if (forca_s > forca_c) {
                vitorias_server++;
                snprintf(resultado, sizeof(resultado),
                         "Rodada %d: Voce jogou %s de %s | Adversario jogou %s de %s -> Servidor ganhou a rodada!\n",
                         rodada, carta_c.nome, carta_c.naipe, carta_s.nome, carta_s.naipe);
            } else if (forca_s < forca_c) {
                vitorias_client++;
                snprintf(resultado, sizeof(resultado),
                         "Rodada %d: Voce jogou %s de %s | Adversario jogou %s de %s -> Cliente ganhou a rodada!\n",
                         rodada, carta_c.nome, carta_c.naipe, carta_s.nome, carta_s.naipe);
            } else {
                snprintf(resultado, sizeof(resultado),
                         "Rodada %d: Cartas iguais! (%s de %s vs %s de %s) -> Empate!\n",
                         rodada, carta_c.nome, carta_c.naipe, carta_s.nome, carta_s.naipe);
            }

            // atualizar memoria compartilhada com resultado e jogadas
            WaitForSingleObject(hMutex, INFINITE);
            mem->vez = 1; // volta a vez ao servidor
            snprintf(mem->ultima_jogada, sizeof(mem->ultima_jogada),
                     "S:%s/%s vs C:%s/%s",
                     carta_s.nome, carta_s.naipe, carta_c.nome, carta_c.naipe);
            snprintf(mem->status, sizeof(mem->status), "%s", resultado);
            ReleaseMutex(hMutex);

            // enviar resultado ao cliente e imprimir no servidor
            send(client_socket, resultado, (int)strlen(resultado), 0);
            printf("%s", resultado);

            rodada++;
        }

        // atualizar placar global na memoria
        WaitForSingleObject(hMutex, INFINITE);
        if (vitorias_server > vitorias_client) {
            mem->pontos_server += 1;
            snprintf(mem->status, sizeof(mem->status), "Partida vencida pelo servidor");
        } else {
            mem->pontos_client += 1;
            snprintf(mem->status, sizeof(mem->status), "Partida vencida pelo cliente");
        }
        int pts_s = mem->pontos_server;
        int pts_c = mem->pontos_client;
        ReleaseMutex(hMutex);

        // reportar placar para cliente
        char placar_msg[128];
        sprintf(placar_msg, "\nPlacar: Servidor %d x %d Cliente\n", pts_s, pts_c);
        send(client_socket, placar_msg, (int)strlen(placar_msg), 0);
        printf("%s", placar_msg);
    }

    // fim de jogo: anunciar vencedor (com base na memoria compartilhada)
    WaitForSingleObject(hMutex, INFINITE);
    int final_s = mem->pontos_server;
    int final_c = mem->pontos_client;
    ReleaseMutex(hMutex);

    char fim[128];
    if (final_s >= 12) sprintf(fim, "Fim de jogo! Servidor venceu!\n");
    else sprintf(fim, "Fim de jogo! Cliente venceu!\n");

    send(client_socket, fim, (int)strlen(fim), 0);
    printf("%s", fim);

cleanup:
    // liberar recursos
    if (client_socket != INVALID_SOCKET) closesocket(client_socket);
    if (server_fd != INVALID_SOCKET) closesocket(server_fd);

    // unmap e fechar handles
    UnmapViewOfFile(mem);
    CloseHandle(hMap);
    CloseHandle(hMutex);

    WSACleanup();
    return 0;
}
