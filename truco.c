/* =======================================================
 * servidor_final_completo_v2.c - Truco Mineiro (Servidor)
 * CUMPRE TODOS OS REQUISITOS + Melhorias de Interface
 * Compilar:
 * gcc servidor_final_completo_v2.c -o truco.exe -lws2_32
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

// Definição para a Memória Compartilhada
#define SHM_NOME "TrucoPlacarSharedMemory"
typedef struct {
    int p1_pts;
    int p2_pts;
    int game_over;
} SharedPlacar;

// Struct para comunicação via Socket
typedef struct {
    int turn; int hand_num; int round_num;
    int client_hand[HAND_SIZE];
    int server_card_on_table; int client_card_on_table;
} GameState;

const char *valor_nome[10] = {"A","2","3","4","5","6","7","J","Q","K"};
const char *naipe_nome[4] = {"Paus","Copas","Espadas","Ouros"};
int truco_ranking(int card_index) { /* ... (código inalterado) ... */ if (card_index < 0 || card_index >= DECK_SIZE) return -1; int value = card_index / 4; int suit = card_index % 4; if (value == 3 && suit == 0) return 100; if (value == 6 && suit == 1) return 99; if (value == 0 && suit == 2) return 98; if (value == 6 && suit == 3) return 97; if (value == 2) return 90; if (value == 1) return 80; if (value == 0) return 70; if (value == 9) return 60; if (value == 7) return 50; if (value == 8) return 40; if (value == 6 && suit == 0) return 39; if (value == 6 && suit == 2) return 38; if (value == 5) return 30; if (value == 4) return 20; if (value == 3) return 10; return 0; }
void print_card(int card_index) { /* ... (código inalterado) ... */ if (card_index < 0 || card_index >= DECK_SIZE) { printf("(vazia)"); return; } int value = card_index / 4; int suit = card_index % 4; printf("%s de %s", valor_nome[value], naipe_nome[suit]); }
void shuffle_deck(int deck[]) { /* ... (código inalterado) ... */ for (int i = 0; i < DECK_SIZE; ++i) deck[i] = i; for (int i = DECK_SIZE - 1; i > 0; --i) { int j = rand() % (i + 1); int t = deck[i]; deck[i] = deck[j]; deck[j] = t; } }
void send_state(SOCKET client_socket, GameState* state) { send(client_socket, (char*)state, sizeof(GameState), 0); }

// NOVO: Função para exibir o estado do jogo no console do servidor
void display_server_view(GameState* state, SharedPlacar* placar, int server_hand[]) {
    system("cls");
    printf("=== MÃO %d | Rodada %d ===\n", state->hand_num, state->round_num);
    printf("PLACAR: Você (Servidor) %d x %d Cliente\n", placar->p1_pts, placar->p2_pts);
    printf("----------------------------------\n");
    // NOVO: Implementação da MESA, como solicitado
    printf("MESA:\n");
    printf("  Você:     ");
    if(state->server_card_on_table != -1) print_card(state->server_card_on_table); else printf("---");
    printf("\n  Cliente:  ");
    if(state->client_card_on_table != -1) print_card(state->client_card_on_table); else printf("---");
    printf("\n----------------------------------\n");
    
    printf("SUA MÃO:\n");
    for(int i=0; i<HAND_SIZE; i++){
        if(server_hand[i] != -1){
            printf("  %d) ", i+1); print_card(server_hand[i]); printf("\n");
        }
    }
    printf("----------------------------------\n");

    if (state->turn == 1) printf(">>> SUA VEZ DE JOGAR! <<<\n");
    else printf(">>> Aguardando jogada do cliente... <<<\n");
}

int get_server_choice() {
    int choice_idx = -1;
    printf("Escolha uma carta (1-3): ");
    scanf("%d", &choice_idx);
    return choice_idx - 1;
}

int main() {
    SetConsoleOutputCP(65001);
    srand((unsigned)time(NULL));

    HANDLE hMapFile;
    SharedPlacar* shm_placar;
    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedPlacar), SHM_NOME);
    if (hMapFile == NULL) { /* ... (tratamento de erro inalterado) ... */ printf("Erro ao criar o mapeamento de arquivo de memória compartilhada (%d).\n", GetLastError()); return 1; }
    shm_placar = (SharedPlacar*) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedPlacar));
    if (shm_placar == NULL) { /* ... (tratamento de erro inalterado) ... */ printf("Erro ao mapear a view do arquivo (%d).\n", GetLastError()); CloseHandle(hMapFile); return 1; }
    printf("[mem_share] Memória compartilhada para o placar criada com sucesso.\n");
    
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;
    srv.sin_port = htons(PORTA);
    bind(listen_sock, (struct sockaddr*)&srv, sizeof(srv));
    listen(listen_sock, 1);
    printf("[socket] aguardando cliente em 127.0.0.1:%d ...\n", PORTA);
    SOCKET client_socket = accept(listen_sock, NULL, NULL);
    closesocket(listen_sock);

    // ALTERADO: Limpa a tela assim que o cliente se conecta
    system("cls"); 
    printf("[socket] Cliente conectado! Iniciando o jogo...\n");
    Sleep(1500);

    GameState state;
    memset(&state, 0, sizeof(GameState));
    shm_placar->p1_pts = 0; shm_placar->p2_pts = 0; shm_placar->game_over = 0;
    int deck[DECK_SIZE];
    int server_hand[HAND_SIZE];
    int mano = 1;

    while (shm_placar->game_over == 0) {
        state.hand_num++;
        shuffle_deck(deck);
        for (int i = 0; i < HAND_SIZE; i++) { server_hand[i] = deck[i * 2]; state.client_hand[i] = deck[i * 2 + 1]; }
        int vaza_winner[3] = {0,0,0}; int last_vaza_winner = 0;

        for (int round = 0; round < 3; round++) {
            state.round_num = round + 1;
            state.server_card_on_table = -1; state.client_card_on_table = -1;
            int first_player = (round == 0) ? mano : (last_vaza_winner != 0 ? last_vaza_winner : mano);
            int second_player = (first_player == 1) ? 2 : 1;
            int server_card = -1, client_card = -1;

            // --- JOGADA DO PRIMEIRO JOGADOR ---
            state.turn = first_player;
            display_server_view(&state, shm_placar, server_hand); // ALTERADO: Mostra a visão do jogo
            send_state(client_socket, &state);

            if (state.turn == 1) {
                int choice_idx;
                do { choice_idx = get_server_choice(); } while (choice_idx < 0 || choice_idx >= 3 || server_hand[choice_idx] == -1);
                server_card = server_hand[choice_idx]; server_hand[choice_idx] = -1; state.server_card_on_table = server_card;
            } else {
                int client_choice_idx = -1;
                if (recv(client_socket, (char*)&client_choice_idx, sizeof(int), 0) <= 0) { shm_placar->game_over = 1; break; }
                client_card = state.client_hand[client_choice_idx]; state.client_hand[client_choice_idx] = -1; state.client_card_on_table = client_card;
            }

            // --- JOGADA DO SEGUNDO JOGADOR ---
            state.turn = second_player;
            display_server_view(&state, shm_placar, server_hand); // ALTERADO: Mostra a visão do jogo
            send_state(client_socket, &state);
            
            if (state.turn == 1) {
                int choice_idx;
                do { choice_idx = get_server_choice(); } while (choice_idx < 0 || choice_idx >= 3 || server_hand[choice_idx] == -1);
                server_card = server_hand[choice_idx]; server_hand[choice_idx] = -1; state.server_card_on_table = server_card;
            } else {
                int client_choice_idx = -1;
                if (recv(client_socket, (char*)&client_choice_idx, sizeof(int), 0) <= 0) { shm_placar->game_over = 1; break; }
                client_card = state.client_hand[client_choice_idx]; state.client_hand[client_choice_idx] = -1; state.client_card_on_table = client_card;
            }
            if (shm_placar->game_over) break;
            
            display_server_view(&state, shm_placar, server_hand); // ALTERADO: Mostra o resultado na mesa
            
            int r_server = truco_ranking(state.server_card_on_table);
            int r_client = truco_ranking(state.client_card_on_table);
            if (r_server > r_client) { printf(">> Você (Servidor) venceu a rodada %d <<\n", state.round_num); vaza_winner[round] = 1; last_vaza_winner = 1; } 
            else if (r_client > r_server) { printf(">> O Cliente venceu a rodada %d <<\n", state.round_num); vaza_winner[round] = 2; last_vaza_winner = 2; } 
            else { printf(">> Rodada %d empatou! <<\n", state.round_num); vaza_winner[round] = 0; }
            Sleep(2500);
        }
        if (shm_placar->game_over) break;

        int server_vazas = 0, client_vazas = 0;
        for(int i=0; i<3; i++) { if(vaza_winner[i] == 1) server_vazas++; if(vaza_winner[i] == 2) client_vazas++; }
        if(server_vazas > client_vazas){ printf(">>>> Você (Servidor) venceu a MÃO! <<<<\n"); shm_placar->p1_pts += 1; } 
        else if (client_vazas > server_vazas){ printf(">>>> O Cliente venceu a MÃO! <<<<\n"); shm_placar->p2_pts += 1; } 
        else { printf(">>>> MÃO CANGOU (empatou)! <<<<\n"); }

        mano = (mano == 1) ? 2 : 1;
        if (shm_placar->p1_pts >= TARGET_POINTS || shm_placar->p2_pts >= TARGET_POINTS) { shm_placar->game_over = 1; }
        Sleep(2000);
    }
    
    // ... (final do main inalterado) ...
    send_state(client_socket, &state); 
    printf("\n--- FIM DE JOGO ---\nPlacar final: Você (Servidor) %d x %d Cliente\n", shm_placar->p1_pts, shm_placar->p2_pts);
    closesocket(client_socket);
    WSACleanup();
    UnmapViewOfFile(shm_placar);
    CloseHandle(hMapFile);
    printf("[mem_share] Memória compartilhada liberada.\n");
    return 0;
}