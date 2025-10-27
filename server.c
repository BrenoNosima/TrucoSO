/* =======================================================
 * truco_servidor_refatorado.c - Truco Mineiro (Servidor)
 * Lógica de comunicação sequencial para evitar deadlocks.
 * Compilar:
 * gcc truco_servidor_refatorado.c -o truco.exe -lws2_32
 * ======================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORTA 12345
#define DECK_SIZE 40
#define HAND_SIZE 3
#define TARGET_POINTS 12

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
int truco_ranking(int card_index) {
    if (card_index < 0 || card_index >= DECK_SIZE) return -1;
    int value = card_index / 4; int suit = card_index % 4;
    if (value == 3 && suit == 0) return 100; if (value == 6 && suit == 1) return 99;
    if (value == 0 && suit == 2) return 98; if (value == 6 && suit == 3) return 97;
    if (value == 2) return 90; if (value == 1) return 80; if (value == 0) return 70;
    if (value == 9) return 60; if (value == 7) return 50; if (value == 8) return 40;
    if (value == 6 && suit == 0) return 39; if (value == 6 && suit == 2) return 38;
    if (value == 5) return 30; if (value == 4) return 20; if (value == 3) return 10;
    return 0;
}
void print_card(int card_index) {
    if (card_index < 0 || card_index >= DECK_SIZE) { printf("(vazia)"); return; }
    int value = card_index / 4; int suit = card_index % 4;
    printf("%s de %s", valor_nome[value], naipe_nome[suit]);
}

void shuffle_deck(int deck[]) {
    for (int i = 0; i < DECK_SIZE; ++i) deck[i] = i;
    for (int i = DECK_SIZE - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int t = deck[i]; deck[i] = deck[j]; deck[j] = t;
    }
}

void send_state(SOCKET client_socket, GameState* state) {
    send(client_socket, (char*)state, sizeof(GameState), 0);
}

int get_server_choice(int hand[]) {
    int choice_idx = -1;
    do {
        printf("Sua mão: ");
        for (int i = 0; i < HAND_SIZE; i++) if (hand[i] != -1) { printf("%d) ", i + 1); print_card(hand[i]); printf(" | "); }
        printf("\nEscolha uma carta (1-3): ");
        scanf("%d", &choice_idx);
        choice_idx--;
    } while (choice_idx < 0 || choice_idx >= 3 || hand[choice_idx] == -1);
    return choice_idx;
}

int main() {
    SetConsoleOutputCP(65001);
    srand((unsigned)time(NULL));
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;
    srv.sin_port = htons(PORTA);
    bind(listen_sock, (struct sockaddr*)&srv, sizeof(srv));
    listen(listen_sock, 1);

    printf("[servidor] aguardando cliente em 127.0.0.1:%d ...\n", PORTA);
    SOCKET client_socket = accept(listen_sock, NULL, NULL);
    printf("[servidor] Cliente conectado!\n");
    closesocket(listen_sock);

    GameState state;
    memset(&state, 0, sizeof(GameState));
    int deck[DECK_SIZE];
    int server_hand[HAND_SIZE];
    int mano = 1;

    while (!state.game_over && state.server_pts < TARGET_POINTS && state.client_pts < TARGET_POINTS) {
        state.hand_num++;
        printf("\n=== MÃO %d (Servidor %d x %d Cliente) ===\n", state.hand_num, state.server_pts, state.client_pts);
        
        shuffle_deck(deck);
        for (int i = 0; i < HAND_SIZE; i++) {
            server_hand[i] = deck[i * 2];
            state.client_hand[i] = deck[i * 2 + 1];
        }

        int vaza_winner[3] = {0,0,0};
        int last_vaza_winner = 0;

        for (int round = 0; round < 3; round++) {
            state.round_num = round + 1;
            state.server_card_on_table = -1;
            state.client_card_on_table = -1;

            int first_player = (round == 0) ? mano : (last_vaza_winner != 0 ? last_vaza_winner : mano);
            int second_player = (first_player == 1) ? 2 : 1;
            int server_card = -1, client_card = -1;

            // --- JOGADA DO PRIMEIRO JOGADOR ---
            state.turn = first_player;
            printf("\n-- Rodada %d: vez do jogador %d --\n", state.round_num, state.turn);
            send_state(client_socket, &state);

            if (state.turn == 1) {
                int choice_idx = get_server_choice(server_hand);
                server_card = server_hand[choice_idx];
                server_hand[choice_idx] = -1;
                state.server_card_on_table = server_card;
                printf("Você jogou: "); print_card(server_card); printf("\n");
            } else {
                printf("Aguardando jogada do cliente...\n");
                int client_choice_idx = -1;
                if (recv(client_socket, (char*)&client_choice_idx, sizeof(int), 0) <= 0) { state.game_over = 1; break; }
                client_card = state.client_hand[client_choice_idx];
                state.client_hand[client_choice_idx] = -1;
                state.client_card_on_table = client_card;
                printf("Cliente jogou: "); print_card(client_card); printf("\n");
            }

            // --- JOGADA DO SEGUNDO JOGADOR ---
            state.turn = second_player;
            printf("-- Rodada %d: vez do jogador %d --\n", state.round_num, state.turn);
            send_state(client_socket, &state);
            
            if (state.turn == 1) {
                int choice_idx = get_server_choice(server_hand);
                server_card = server_hand[choice_idx];
                server_hand[choice_idx] = -1;
                state.server_card_on_table = server_card;
                printf("Você jogou: "); print_card(server_card); printf("\n");
            } else {
                printf("Aguardando jogada do cliente...\n");
                int client_choice_idx = -1;
                if (recv(client_socket, (char*)&client_choice_idx, sizeof(int), 0) <= 0) { state.game_over = 1; break; }
                client_card = state.client_hand[client_choice_idx];
                state.client_hand[client_choice_idx] = -1;
                state.client_card_on_table = client_card;
                printf("Cliente jogou: "); print_card(client_card); printf("\n");
            }
            if (state.game_over) break;

            int r_server = truco_ranking(state.server_card_on_table);
            int r_client = truco_ranking(state.client_card_on_table);
            if (r_server > r_client) {
                printf(">> Servidor venceu a rodada %d <<\n", state.round_num);
                vaza_winner[round] = 1; last_vaza_winner = 1;
            } else if (r_client > r_server) {
                printf(">> Cliente venceu a rodada %d <<\n", state.round_num);
                vaza_winner[round] = 2; last_vaza_winner = 2;
            } else {
                printf(">> Rodada %d empatou! <<\n", state.round_num);
                vaza_winner[round] = 0;
            }
            Sleep(2000); // Pausa para ver o resultado da rodada
        }
        if (state.game_over) break;

        int server_vazas = 0, client_vazas = 0;
        for(int i=0; i<3; i++) { if(vaza_winner[i] == 1) server_vazas++; if(vaza_winner[i] == 2) client_vazas++; }
        if(server_vazas > client_vazas){ printf(">>>> Servidor venceu a MÃO! <<<<\n"); state.server_pts += 1;
        } else if (client_vazas > server_vazas){ printf(">>>> Cliente venceu a MÃO! <<<<\n"); state.client_pts += 1;
        } else { printf(">>>> MÃO CANGOU (empatou)! <<<<\n"); }

        mano = (mano == 1) ? 2 : 1;
        if (state.server_pts >= TARGET_POINTS || state.client_pts >= TARGET_POINTS) state.game_over = 1;
    }

    send_state(client_socket, &state);
    printf("\n--- FIM DE JOGO ---\nPlacar final: Servidor %d x %d Cliente\n", state.server_pts, state.client_pts);

    closesocket(client_socket); WSACleanup(); return 0;
}