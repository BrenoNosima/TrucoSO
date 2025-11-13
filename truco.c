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
// Códigos de comando do cliente -> servidor
#define CMD_TRUCO_REQ   -100
#define CMD_TRUCO_ACCEPT -101
#define CMD_TRUCO_RUN   -102

typedef struct {
    int turn; int hand_num; int round_num;
    int client_hand[HAND_SIZE];
    int server_card_on_table; int client_card_on_table;
    char message[100];
    // Apostas da mão (Truco)
    int hand_value;              // 1,3,6,9,12
    int truco_pending;           // 0/1
    int truco_requester;         // 1=Servidor, 2=Cliente
    int truco_target;            // próximo valor se aceito
    int awaiting_truco_response; // 0/1
    int raise_rights;            // 1=Servidor pode pedir, 2=Cliente pode pedir, 0=ninguém
    int message_seq;             // incrementa a cada nova mensagem
} GameState;

// Histórico da mão atual
typedef struct {
    int rodadaAtual;            // 1, 2 ou 3 (número de resultados já registrados)
    char resultadoRodada[3][50]; // Guarda quem ganhou cada rodada ("Cliente", "Servidor", "Empate")
} HistoricoMao;

HistoricoMao historico;

void iniciarNovaMao() {
    historico.rodadaAtual = 0;
    for (int i = 0; i < 3; i++) {
        strcpy(historico.resultadoRodada[i], "-");
    }
}

void registrarResultadoRodada(const char *vencedor) {
    if (historico.rodadaAtual < 3) {
        strcpy(historico.resultadoRodada[historico.rodadaAtual], vencedor);
        historico.rodadaAtual++;
    }
}

void exibirHistorico() {
    printf("\n--- Histórico da Mão Atual ---\n");
    for (int i = 0; i < 3; i++) {
        printf("Rodada %d: %s\n", i + 1, historico.resultadoRodada[i]);
    }
    printf("-----------------------------\n");
}

const char *valor_nome[10] = {"A","2","3","4","5","6","7","J","Q","K"};
const char *naipe_nome[4] = {"Paus","Copas","Espadas","Ouros"};
int truco_ranking(int card_index) { /* ... (código inalterado) ... */ if (card_index < 0 || card_index >= DECK_SIZE) return -1; int value = card_index / 4; int suit = card_index % 4; if (value == 3 && suit == 0) return 100; if (value == 6 && suit == 1) return 99; if (value == 0 && suit == 2) return 98; if (value == 6 && suit == 3) return 97; if (value == 2) return 90; if (value == 1) return 80; if (value == 0) return 70; if (value == 9) return 60; if (value == 7) return 50; if (value == 8) return 40; if (value == 6 && suit == 0) return 39; if (value == 6 && suit == 2) return 38; if (value == 5) return 30; if (value == 4) return 20; if (value == 3) return 10; return 0; }
void print_card(int card_index) { /* ... (código inalterado) ... */ if (card_index < 0 || card_index >= DECK_SIZE) { printf("(vazia)"); return; } int value = card_index / 4; int suit = card_index % 4; printf("%s de %s", valor_nome[value], naipe_nome[suit]); }
void shuffle_deck(int deck[]) { /* ... (código inalterado) ... */ for (int i = 0; i < DECK_SIZE; ++i) deck[i] = i; for (int i = DECK_SIZE - 1; i > 0; --i) { int j = rand() % (i + 1); int t = deck[i]; deck[i] = deck[j]; deck[j] = t; } }
void send_state(SOCKET client_socket, GameState* state) { send(client_socket, (char*)state, sizeof(GameState), 0); }

static int next_truco_value(int v){
    if (v >= 12) return 12; if (v >= 9) return 12; if (v >= 6) return 9; if (v >= 3) return 6; return 3;
}

static void set_message(GameState* s, const char* msg){
    strncpy(s->message, msg, sizeof(s->message)-1);
    s->message[sizeof(s->message)-1] = '\0';
    s->message_seq++;
}

// NOVO: Função para exibir o estado do jogo no console do servidor
void display_server_view(GameState* state, SharedPlacar* placar, int server_hand[]) {
    system("cls");
    printf("=== MÃO %d | Rodada %d ===\n", state->hand_num, state->round_num);
    printf("PLACAR: Você (Servidor) %d x %d Cliente\n", placar->p1_pts, placar->p2_pts);
    printf("VALENDO: %d ponto(s)\n", state->hand_value > 0 ? state->hand_value : 1);
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

    // Exibe o histórico da mão atual logo abaixo da mesa
    exibirHistorico();
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
        // Inicia histórico para a nova mão
        iniciarNovaMao();
        // Reset de aposta da mão
        state.hand_value = 1;
        state.truco_pending = 0;
        state.truco_requester = 0;
        state.truco_target = 0;
        state.awaiting_truco_response = 0;
        state.raise_rights = mano; // apenas o jogador da vez pode pedir truco inicialmente
        state.message_seq = 0;
        shuffle_deck(deck);
        for (int i = 0; i < HAND_SIZE; i++) { server_hand[i] = deck[i * 2]; state.client_hand[i] = deck[i * 2 + 1]; }
    int vaza_winner[3] = {0,0,0}; int last_vaza_winner = 0;
    int hand_ended_early = 0; // 1 = mão encerrada antes das 3 rodadas
    int hand_winner = 0;      // 1 = servidor, 2 = cliente

        for (int round = 0; round < 3; round++) {
            state.round_num = round + 1;
            state.server_card_on_table = -1; state.client_card_on_table = -1;
            state.message[0] = '\0';
            state.awaiting_truco_response = 0; state.truco_pending = 0; state.truco_requester = 0; state.truco_target = 0;
            int first_player = (round == 0) ? mano : (last_vaza_winner != 0 ? last_vaza_winner : mano);
            int second_player = (first_player == 1) ? 2 : 1;
            int server_card = -1, client_card = -1;
            state.raise_rights = first_player; // em cada rodada, quem inicia pode pedir o próximo aumento

            // --- JOGADA DO PRIMEIRO JOGADOR ---
            state.turn = first_player;
            display_server_view(&state, shm_placar, server_hand); // ALTERADO: Mostra a visão do jogo
            send_state(client_socket, &state);

            if (state.turn == 1) {
                int choice_idx;
                for(;;){
                    if (state.hand_value < 12 && state.raise_rights == 1) printf("(1-3 carta, 9=PEDIR %d) \n", next_truco_value(state.hand_value));
                    else printf("(1-3 carta) \n");
                    choice_idx = get_server_choice();
                    if (choice_idx == 8) { // usuário digitou 9 -> get_server_choice retorna 8 (9-1)
                        if (state.hand_value >= 12 || state.raise_rights != 1) {
                            strcpy(state.message, "Já está valendo 12. Não é possível aumentar.");
                            display_server_view(&state, shm_placar, server_hand);
                            continue;
                        }
                        // Solicita TRUCO ao cliente
                        state.truco_pending = 1; state.truco_requester = 1;
                        state.truco_target = next_truco_value(state.hand_value);
                        state.awaiting_truco_response = 1;
                        set_message(&state, "Servidor pediu TRUCO!");
                        send_state(client_socket, &state);
                        // Pergunta ao cliente (cliente responderá). Aqui servidor espera a resposta do cliente (ACCEPT/RUN)
                        printf("Aguardando resposta do cliente ao TRUCO...\n");
                        int client_response = 0;
                        if (recv(client_socket, (char*)&client_response, sizeof(int), 0) <= 0) { shm_placar->game_over = 1; break; }
                        if (client_response == CMD_TRUCO_ACCEPT) {
                            state.hand_value = state.truco_target;
                            state.awaiting_truco_response = 0;
                            state.raise_rights = 2; // direito de aumentar passa para quem aceitou (cliente)
                            char buf[100]; sprintf(buf, "Cliente aceitou! Rodada agora vale %d pontos!", state.hand_value);
                            set_message(&state, buf);
                            send_state(client_socket, &state);
                            // volta a pedir jogada (continuar for loop)
                            continue;
                        } else if (client_response == CMD_TRUCO_RUN) {
                            // Cliente correu: encerra a mão com vitória do servidor valendo o valor atual
                            hand_ended_early = 1; hand_winner = 1;
                            char buf[100]; sprintf(buf, "Cliente correu! Servidor ganha %d ponto(s)!", state.hand_value);
                            set_message(&state, buf);
                            send_state(client_socket, &state);
                            round = 3; state.round_num = 3; // sair das rodadas
                            goto end_of_hand_fast;
                        }
                    } else if (choice_idx >= 0 && choice_idx < 3 && server_hand[choice_idx] != -1) {
                        break; // jogada válida
                    } else {
                        // inválido
                        continue;
                    }
                }
                server_card = server_hand[choice_idx]; server_hand[choice_idx] = -1; state.server_card_on_table = server_card;
            } else {
                int client_choice_idx = -1;
                for(;;){
                    if (recv(client_socket, (char*)&client_choice_idx, sizeof(int), 0) <= 0) { shm_placar->game_over = 1; break; }
                    if (client_choice_idx == CMD_TRUCO_REQ) {
                        if (state.hand_value >= 12 || state.raise_rights != 2) {
                            strcpy(state.message, "Já está valendo 12. Não é possível aumentar.");
                            send_state(client_socket, &state);
                            continue; // permanecer aguardando jogada do cliente
                        }
                        // Cliente pediu TRUCO: servidor decide
                        state.truco_pending = 1; state.truco_requester = 2; state.truco_target = next_truco_value(state.hand_value);
                        state.awaiting_truco_response = 1;
                        set_message(&state, "Cliente pediu TRUCO!");
                        send_state(client_socket, &state);

                        int resp = 0;
                        do {
                            printf("Cliente pediu TRUCO! Aceitar (1) ou correr (2)? ");
                            scanf("%d", &resp);
                        } while (resp != 1 && resp != 2);

                        if (resp == 1) {
                            state.hand_value = state.truco_target;
                            state.awaiting_truco_response = 0;
                            state.raise_rights = 1; // direito de aumentar passa para quem aceitou (servidor)
                            char buf[100]; sprintf(buf, "Servidor aceitou! Rodada agora vale %d pontos!", state.hand_value);
                            set_message(&state, buf);
                            send_state(client_socket, &state);
                            // volta a aguardar a jogada do cliente
                            continue;
                        } else {
                            hand_ended_early = 1; hand_winner = 2;
                            char buf[100]; sprintf(buf, "Servidor correu! Cliente ganha %d ponto(s)!", state.hand_value);
                            set_message(&state, buf);
                            send_state(client_socket, &state);
                            round = 3; state.round_num = 3;
                            goto end_of_hand_fast;
                        }
                    } else if (client_choice_idx >= 0 && client_choice_idx < 3 && state.client_hand[client_choice_idx] != -1) {
                        break; // jogada válida
                    } else {
                        // inválido: continua aguardando
                        continue;
                    }
                }
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
            if (r_server > r_client) {
                printf(">> Você (Servidor) venceu a rodada %d <<\n", state.round_num);
                vaza_winner[round] = 1; last_vaza_winner = 1;
                registrarResultadoRodada("Servidor");
                set_message(&state, "Você (Servidor) venceu a rodada!");
            } else if (r_client > r_server) {
                printf(">> O Cliente venceu a rodada %d <<\n", state.round_num);
                vaza_winner[round] = 2; last_vaza_winner = 2;
                registrarResultadoRodada("Cliente");
                set_message(&state, "O Cliente venceu a rodada!");
            } else {
                printf(">> Rodada %d empatou! <<\n", state.round_num);
                vaza_winner[round] = 0;
                registrarResultadoRodada("Empate");
                set_message(&state, "Rodada empatou!");
            }

            // Envia o estado final da rodada para o cliente (mesa completa + mensagem)
            send_state(client_socket, &state);

            // Regra 1: se alguém venceu 2 rodadas consecutivas, encerra a mão
            if (round >= 1 && vaza_winner[round] != 0 && vaza_winner[round] == vaza_winner[round - 1]) {
                hand_ended_early = 1;
                hand_winner = vaza_winner[round];
                break;
            }
            // Regra 2A: se a 1ª empatou e a 2ª teve vencedor, a mão termina com o vencedor da 2ª
            if (round == 1 && vaza_winner[0] == 0 && vaza_winner[1] != 0) {
                hand_ended_early = 1;
                hand_winner = vaza_winner[1];
                printf(">>> 1ª empatou, 2ª decidiu a MÃO.\n");
                Sleep(1500);
                break;
            }
            // Regra 2 (pedido): se ganhou a 1ª e empatou a 2ª, quem ganhou a 1ª vence a mão
            if (round == 1 && vaza_winner[round] == 0 && last_vaza_winner != 0) {
                hand_ended_early = 1;
                hand_winner = last_vaza_winner; // quem ganhou a 1ª
                printf(">>> Empate na 2ª rodada: vence quem ganhou a 1ª.\n");
                Sleep(1500);
                break;
            }
            Sleep(2500);
        }
end_of_hand_fast:
        if (shm_placar->game_over) break;

        if (hand_ended_early) {
            if (hand_winner == 1) { printf(">>>> Você (Servidor) venceu a MÃO! <<<<\n"); shm_placar->p1_pts += state.hand_value; }
            else if (hand_winner == 2) { printf(">>>> O Cliente venceu a MÃO! <<<<\n"); shm_placar->p2_pts += state.hand_value; }
            else { printf(">>>> MÃO CANGOU (empatou)! <<<<\n"); }
        } else {
            int server_vazas = 0, client_vazas = 0;
            for(int i=0; i<3; i++) { if(vaza_winner[i] == 1) server_vazas++; if(vaza_winner[i] == 2) client_vazas++; }
            if(server_vazas > client_vazas){ printf(">>>> Você (Servidor) venceu a MÃO! <<<<\n"); shm_placar->p1_pts += state.hand_value; } 
            else if (client_vazas > server_vazas){ printf(">>>> O Cliente venceu a MÃO! <<<<\n"); shm_placar->p2_pts += state.hand_value; } 
            else { printf(">>>> MÃO CANGOU (empatou)! <<<<\n"); }
        }

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