/* =======================================================
 * cliente_refatorado.c - Truco Mineiro (Cliente)
 * Lógica de exibição e comunicação aprimorada.
 * Compilar:
 * gcc cliente_refatorado.c -o cliente.exe -lws2_32 -lpthread
 * ======================================================= */
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib")

#define PORTA 12345
#define TAM_IP 30
#define HAND_SIZE 3
#define DECK_SIZE 40

typedef struct {
    int server_pts;
    int client_pts;
    int turn;
    int hand_num;
    int round_num;
    int game_over;
    int client_hand[HAND_SIZE];
    // NOVO: Campos para ver as cartas na mesa
    int server_card_on_table;
    int client_card_on_table;
} GameState;

const char *valor_nome[10] = {"A","2","3","4","5","6","7","J","Q","K"};
const char *naipe_nome[4] = {"Paus","Copas","Espadas","Ouros"};
void print_card(int card_index) {
    if (card_index < 0 || card_index >= DECK_SIZE) { printf("(vazia)"); return; }
    int value = card_index / 4; int suit = card_index % 4;
    printf("%s de %s", valor_nome[value], naipe_nome[suit]);
}

SOCKET sock;
GameState gameState;
int jogoAtivo = 1;

void display_game() {
    system("cls");
    printf("=== MÃO %d | Rodada %d ===\n", gameState.hand_num, gameState.round_num);
    printf("PLACAR: Servidor %d x %d Você\n", gameState.server_pts, gameState.client_pts);
    printf("----------------------------------\n");
    printf("MESA:\n");
    printf("  Servidor: ");
    if(gameState.server_card_on_table != -1) print_card(gameState.server_card_on_table);
    else printf("---");
    printf("\n  Você:     ");
    if(gameState.client_card_on_table != -1) print_card(gameState.client_card_on_table);
    else printf("---");
    printf("\n----------------------------------\n");
    
    printf("SUA MÃO:\n");
    for(int i=0; i<HAND_SIZE; i++){
        if(gameState.client_hand[i] != -1){
            printf("  %d) ", i+1); print_card(gameState.client_hand[i]); printf("\n");
        }
    }
    printf("----------------------------------\n");

    if (gameState.turn == 2) printf(">>> SUA VEZ DE JOGAR! <<<\n");
    else printf(">>> Aguardando jogada do servidor... <<<\n");
}

void* receber_thread(void* arg) {
    while (jogoAtivo) {
        int bytes = recv(sock, (char*)&gameState, sizeof(GameState), 0);
        if (bytes <= 0) {
            printf("\n[conexao] Servidor desconectou.\n"); jogoAtivo = 0; break;
        }
        if (gameState.game_over) jogoAtivo = 0;
        display_game();
    }
    return NULL;
}

int main() {
    SetConsoleOutputCP(65001);
    WSADATA wsa;
    struct sockaddr_in servidor;
    char ip_servidor[TAM_IP];

    printf("=== Cliente Truco Mineiro ===\nDigite o IP do servidor (ex: 127.0.0.1): ");
    scanf("%s", ip_servidor);

    WSAStartup(MAKEWORD(2, 2), &wsa);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORTA);
    servidor.sin_addr.s_addr = inet_addr(ip_servidor);

    if (connect(sock, (struct sockaddr*)&servidor, sizeof(servidor)) == SOCKET_ERROR) {
        printf("Erro ao conectar.\n"); closesocket(sock); return 1;
    }
    
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, receber_thread, NULL);

    while(jogoAtivo) {
        if(gameState.turn == 2) {
            int indice_escolhido = -1;
            do {
                printf("Escolha uma carta (1-3): ");
                scanf("%d", &indice_escolhido);
                indice_escolhido--;
            } while (indice_escolhido < 0 || indice_escolhido >= 3 || gameState.client_hand[indice_escolhido] == -1);
            
            send(sock, (char*)&indice_escolhido, sizeof(int), 0);
            gameState.turn = 1; 
            display_game();
        }
        Sleep(100);
    }

    pthread_join(thread_id, NULL);
    printf("\n--- FIM DE JOGO ---\n");
    printf("Placar final: Servidor %d x %d Você\n", gameState.server_pts, gameState.client_pts);
    printf("\n[encerrado] Pressione Enter para sair.\n");
    getchar(); getchar();

    closesocket(sock); WSACleanup(); return 0;
}