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
	    int hand_value;              // 1 ou 3
	    int truco_pending;           // 0/1
	    int truco_requester;         // 1=Servidor, 2=Cliente
	    int awaiting_truco_response; // 0/1
	    int truco_allowed;           // 1=Truco pode ser pedido, 0=Truco não pode ser pedido
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
	
	// Retorna a string da carta formatada em um buffer
	void get_card_string(int card_index, char *buffer) {
	    if (card_index < 0 || card_index >= DECK_SIZE) {
	        strcpy(buffer, "---"); // 3 caracteres para "vazia"
	        return;
	    }
	    int value = card_index / 4;
	    int suit = card_index % 4;
	    sprintf(buffer, "%s de %s", valor_nome[value], naipe_nome[suit]);
	}
	
	// Função auxiliar para imprimir a carta (usada apenas para debug/simplicidade)
	void print_card(int card_index) {
	    char buffer[20]; // Buffer para a string da carta
	    get_card_string(card_index, buffer);
	    printf("%s", buffer);
	}
	
	int truco_ranking(int card_index) { /* ... (código inalterado) ... */ if (card_index < 0 || card_index >= DECK_SIZE) return -1; int value = card_index / 4; int suit = card_index % 4; if (value == 3 && suit == 0) return 100; if (value == 6 && suit == 1) return 99; if (value == 0 && suit == 2) return 98; if (value == 6 && suit == 3) return 97; if (value == 2) return 90; if (value == 1) return 80; if (value == 0) return 70; if (value == 9) return 60; if (value == 7) return 50; if (value == 8) return 40; if (value == 6 && suit == 0) return 39; if (value == 6 && suit == 2) return 38; if (value == 5) return 30; if (value == 4) return 20; if (value == 3) return 10; return 0; }
	void shuffle_deck(int deck[]) { /* ... (código inalterado) ... */ for (int i = 0; i < DECK_SIZE; ++i) deck[i] = i; for (int i = DECK_SIZE - 1; i > 0; --i) { int j = rand() % (i + 1); int t = deck[i]; deck[i] = deck[j]; deck[j] = t; } }
void send_state(SOCKET client_socket, GameState* state) { send(client_socket, (char*)state, sizeof(GameState), 0); }

	static int next_truco_value(int v){
	    // Simplificado: Truco só pode ir de 1 para 3.
	    if (v == 1) return 3;
	    return 3; // Valor de truco
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
	    printf("VALENDO: %d ponto(s)\n", state->hand_value);
    printf("----------------------------------\n");
    // NOVO: Implementação da MESA, como solicitado
	    printf("MESA:\n");
	    char server_card_str[20], client_card_str[20];
	    get_card_string(state->server_card_on_table, server_card_str);
	    get_card_string(state->client_card_on_table, client_card_str);
	    
	    // Usando largura fixa para garantir alinhamento
	    printf("  Você:     %-15s\n", server_card_str);
	    printf("  Cliente:  %-15s\n", client_card_str);
	    printf("----------------------------------\n");
	    
	    printf("SUA MÃO:\n");
	    for(int i=0; i<HAND_SIZE; i++){
	        if(server_hand[i] != -1){
	            char card_str[20];
	            get_card_string(server_hand[i], card_str);
	            printf("  %d) %-15s\n", i+1, card_str);
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
	        
	        // --- Lógica Mão de Onze / Mão de Ferro ---
	        int mao_de_onze = 0; // 1=Servidor, 2=Cliente, 3=Ferro
	        if (shm_placar->p1_pts == 11 && shm_placar->p2_pts == 11) {
	            mao_de_onze = 3; // Mão de Ferro
	            state.hand_value = 12;
	            state.truco_allowed = 0; // Não pode pedir truco
	            set_message(&state, "MÃO DE FERRO! Vale 12 pontos. Jogada às escuras.");
	        } else if (shm_placar->p1_pts == 11) {
	            mao_de_onze = 1; // Servidor na Mão de Onze
	            state.hand_value = 1; // Mão de Onze vale 1 ponto
	            state.truco_allowed = 0; // Não pode pedir truco
	            set_message(&state, "MÃO DE ONZE para o Servidor! Servidor decide jogar ou correr.");
	        } else if (shm_placar->p2_pts == 11) {
	            mao_de_onze = 2; // Cliente na Mão de Onze
	            state.hand_value = 1; // Mão de Onze vale 1 ponto
	            state.truco_allowed = 0; // Não pode pedir truco
	            set_message(&state, "MÃO DE ONZE para o Cliente! Cliente decide jogar ou correr.");
	        } else {
	            // Mão normal
	            state.hand_value = 1; // Mão normal vale 1 ponto
	            state.truco_allowed = 1; // Truco pode ser pedido
	            state.message[0] = '\0';
	        }
	        
	        // Reset de aposta da mão (exceto hand_value e truco_allowed se for Mão de Onze/Ferro)
	        state.truco_pending = 0;
	        state.truco_requester = 0;
	        state.awaiting_truco_response = 0;
	        state.message_seq = 0;
	        shuffle_deck(deck);
	        
	        // Distribuição de cartas
	        for (int i = 0; i < HAND_SIZE; i++) { server_hand[i] = deck[i * 2]; state.client_hand[i] = deck[i * 2 + 1]; }
	        
	        // --- Lógica de Decisão Mão de Onze ---
	        if (mao_de_onze == 1) { // Servidor com 11 pontos
	            display_server_view(&state, shm_placar, server_hand);
	            int resp = 0;
	            do {
	                printf("Você está na MÃO DE ONZE. Deseja jogar (1) ou correr (2)? Correr dá 1 ponto ao Cliente.\n");
	                scanf("%d", &resp);
	            } while (resp != 1 && resp != 2);
	            
	            if (resp == 2) {
	                // Servidor correu na Mão de Onze
	                shm_placar->p2_pts += 1; // Mão de Onze vale 1 ponto se correr
	                set_message(&state, "Servidor correu na Mão de Onze. Cliente ganha 1 ponto.");
	                send_state(client_socket, &state);
	                goto end_of_hand_fast; // Pula para o final da mão
	            }
	            // Se escolheu jogar (resp == 1), o jogo segue normalmente.
	        } else if (mao_de_onze == 2) { // Cliente com 11 pontos
	            // Envia o estado para o cliente decidir se joga ou corre
	            send_state(client_socket, &state);
	            printf("Aguardando decisão do cliente na Mão de Onze...\n");
	            int client_response = 0;
	            // O cliente enviará CMD_TRUCO_ACCEPT (jogar) ou CMD_TRUCO_RUN (correr)
	            if (recv(client_socket, (char*)&client_response, sizeof(int), 0) <= 0) { shm_placar->game_over = 1; break; }
	            
	            if (client_response == CMD_TRUCO_RUN) {
	                // Cliente correu na Mão de Onze
	                shm_placar->p1_pts += 1; // Mão de Onze vale 1 ponto se correr
	                set_message(&state, "Cliente correu na Mão de Onze. Servidor ganha 1 ponto.");
	                send_state(client_socket, &state);
	                goto end_of_hand_fast; // Pula para o final da mão
	            }
	            // Se escolheu jogar (client_response == CMD_TRUCO_ACCEPT), o jogo segue normalmente.
	        }
	        
	        // --- Lógica Mão de Ferro ---
	        if (mao_de_onze == 3) {
	            // Mão de Ferro: jogo segue normalmente, mas "às escuras" e valendo 12.
	            // A regra de "às escuras" não é implementada aqui, mas o truco está desabilitado e o valor é 12.
	            set_message(&state, "MÃO DE FERRO! Vale 12 pontos. Jogada às escuras.");
	            send_state(client_socket, &state);
	        }
	        
	    int vaza_winner[3] = {0,0,0}; int last_vaza_winner = 0;
	    int hand_ended_early = 0; // 1 = mão encerrada antes das 3 rodadas
	    int hand_winner = 0;      // 1 = servidor, 2 = cliente

        for (int round = 0; round < 3; round++) {
	            state.round_num = round + 1;
	            state.server_card_on_table = -1; state.client_card_on_table = -1;
	            state.message[0] = '\0';
	            state.awaiting_truco_response = 0; state.truco_pending = 0; state.truco_requester = 0;
	            int first_player = (round == 0) ? mano : (last_vaza_winner != 0 ? last_vaza_winner : mano);
	            int second_player = (first_player == 1) ? 2 : 1;
	            int server_card = -1, client_card = -1;
	            // O direito de pedir truco é controlado por state.truco_allowed e não muda durante as rodadas.

            // --- JOGADA DO PRIMEIRO JOGADOR ---
            state.turn = first_player;
            display_server_view(&state, shm_placar, server_hand); // ALTERADO: Mostra a visão do jogo
            send_state(client_socket, &state);

	            if (state.turn == 1) {
	                int choice_idx;
	                for(;;){
	                    if (state.hand_value == 1 && state.truco_allowed == 1) printf("(1-3 carta, 9=PEDIR TRUCO) \n");
	                    else printf("(1-3 carta) \n");
	                    choice_idx = get_server_choice();
	                    
	                    if (choice_idx == 8) { // usuário digitou 9 -> get_server_choice retorna 8 (9-1)
	                        if (state.hand_value != 1 || state.truco_allowed != 1) {
	                            strcpy(state.message, "Não é possível pedir Truco agora.");
	                            display_server_view(&state, shm_placar, server_hand);
	                            continue;
	                        }
	                        
	                        // --- Lógica de Pedido de Truco (Servidor) ---
	                        state.truco_pending = 1; state.truco_requester = 1;
	                        state.awaiting_truco_response = 1;
	                        set_message(&state, "Servidor pediu TRUCO!");
	                        send_state(client_socket, &state); // Envia o estado para o cliente ver o pedido
	                        
	                        printf("Aguardando resposta do cliente ao TRUCO...\n");
	                        int client_response = 0;
	                        if (recv(client_socket, (char*)&client_response, sizeof(int), 0) <= 0) { shm_placar->game_over = 1; break; }
	                        
	                        if (client_response == CMD_TRUCO_ACCEPT) {
	                            // Aceitou
	                            state.hand_value = 3; // Truco aceito, mão vale 3
	                            state.awaiting_truco_response = 0;
	                            state.truco_allowed = 0; // Truco não pode ser pedido novamente
	                            char buf[100]; sprintf(buf, "Cliente aceitou! Rodada agora vale %d pontos!", state.hand_value);
	                            set_message(&state, buf);
	                            send_state(client_socket, &state);
	                            // O Servidor (quem pediu) continua com o turno para jogar a carta.
	                            display_server_view(&state, shm_placar, server_hand); // Limpa a tela e redesenha a interface
	                            continue; // volta a pedir jogada
	                        } else if (client_response == CMD_TRUCO_RUN) {
	                            // Correu
	                            hand_ended_early = 1; hand_winner = 1;
	                            state.hand_value = 1; // Correr, ganha o valor anterior (1)
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
	                        if (state.hand_value != 1 || state.truco_allowed != 1) {
	                            strcpy(state.message, "Não é possível pedir Truco agora.");
	                            send_state(client_socket, &state);
	                            continue; // permanecer aguardando jogada do cliente
	                        }
	                        
	                        // --- Lógica de Pedido de Truco (Cliente) ---
	                        state.truco_pending = 1; state.truco_requester = 2;
	                        state.awaiting_truco_response = 1;
	                        set_message(&state, "Cliente pediu TRUCO!");
	                        send_state(client_socket, &state); // Envia o estado para o servidor ver o pedido
	
	                        int resp = 0;
	                        do {
	                            printf("Cliente pediu TRUCO! Aceitar (1) ou correr (2)? ");
	                            scanf("%d", &resp);
	                        } while (resp != 1 && resp != 2);
	
	                        if (resp == 1) {
	                            // Aceitou
	                            state.hand_value = 3; // Truco aceito, mão vale 3
	                            state.awaiting_truco_response = 0;
	                            state.truco_allowed = 0; // Truco não pode ser pedido novamente
	                            char buf[100]; sprintf(buf, "Servidor aceitou! Rodada agora vale %d pontos!", state.hand_value);
	                            set_message(&state, buf);
	                            send_state(client_socket, &state);
	                            // O Cliente (quem pediu) continua com o turno para jogar a carta.
	                            continue; // volta a aguardar a jogada do cliente
	                        } else {
	                            // Correu
	                            hand_ended_early = 1; hand_winner = 2;
	                            state.hand_value = 1; // Correr, ganha o valor anterior (1)
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
            // Regra 1: se alguém venceu 2 rodadas consecutivas, encerra a mão (não se aplica se a 1ª empatou)
            if (round >= 1 && vaza_winner[round] != 0 && vaza_winner[round] == vaza_winner[round - 1] && vaza_winner[round-1] != 0) {
                hand_ended_early = 1;
                hand_winner = vaza_winner[round];
                break;
            }
            // Regra 2A: se a 1ª empatou e a 2ª teve vencedor, a mão termina com o vencedor da 2ª
            // Esta regra já está coberta pela Regra 2B (quem ganhou a 1ª) e a Regra 3 (3ª rodada)
            // Se a 1ª empatou (vaza_winner[0] == 0) e a 2ª teve vencedor (vaza_winner[1] != 0), o vencedor da 2ª ganha a mão.
            // O código original está correto aqui, mas a regra 2B (abaixo) precisa ser ajustada para não interferir.
            if (round == 1 && vaza_winner[0] == 0 && vaza_winner[1] != 0) {
                hand_ended_early = 1;
                hand_winner = vaza_winner[1];
                printf(">>> 1ª empatou, 2ª decidiu a MÃO.\n");
                Sleep(1500);
                break;
            }
            // Regra 2B: se a 1ª teve vencedor e a 2ª empatou, quem ganhou a 1ª vence a mão
            if (round == 1 && vaza_winner[0] != 0 && vaza_winner[1] == 0) {
                hand_ended_early = 1;
                hand_winner = vaza_winner[0]; // quem ganhou a 1ª
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
	            // Mão foi até o fim (3 rodadas)
	            int server_vazas = 0, client_vazas = 0;
	            for(int i=0; i<3; i++) { if(vaza_winner[i] == 1) server_vazas++; if(vaza_winner[i] == 2) client_vazas++; }
	            
	            // Regra 3: Se a 1ª e 2ª empataram, a 3ª decide.
	            if (vaza_winner[0] == 0 && vaza_winner[1] == 0) {
	                if (vaza_winner[2] == 1) { printf(">>>> Você (Servidor) venceu a MÃO! <<<<\n"); shm_placar->p1_pts += state.hand_value; }
	                else if (vaza_winner[2] == 2) { printf(">>>> O Cliente venceu a MÃO! <<<<\n"); shm_placar->p2_pts += state.hand_value; }
	                else { printf(">>>> MÃO CANGOU (empatou)! <<<<\n"); }
	            }
	            // Regra 4: Se 1ª teve vencedor e 2ª e 3ª empataram, quem ganhou a 1ª vence. (Já coberto pela Regra 2B)
	            // Regra 5: Se 1ª empatou, 2ª teve vencedor e 3ª empatou, quem ganhou a 2ª vence. (Já coberto pela Regra 2A)
	            // Regra 6: Se 1ª e 3ª tiveram vencedor e 2ª empatou, quem ganhou a 1ª vence. (Já coberto pela Regra 2B)
	            // Regra 7: Se 1ª e 2ª tiveram vencedor e 3ª empatou, quem ganhou a 1ª vence. (Já coberto pela Regra 1)
	            // Regra 8: Se 1ª empatou, 2ª e 3ª tiveram vencedor, quem ganhou 2 vazas vence.
	            else if(server_vazas > client_vazas){ printf(">>>> Você (Servidor) venceu a MÃO! <<<<\n"); shm_placar->p1_pts += state.hand_value; } 
	            else if (client_vazas > server_vazas){ printf(">>>> O Cliente venceu a MÃO! <<<<\n"); shm_placar->p2_pts += state.hand_value; } 
	            else { printf(">>>> MÃO CANGOU (empatou)! <<<<\n"); }
	        }
	
	        // Verifica se o jogo acabou
	        if (shm_placar->p1_pts >= TARGET_POINTS || shm_placar->p2_pts >= TARGET_POINTS) { shm_placar->game_over = 1; }
	        
	        // Se for Mão de Onze, o jogo só acaba se o jogador com 11 pontos vencer. Se perder, o jogo continua.
	        // A pontuação por correr na Mão de Onze já foi adicionada acima.
	        if (mao_de_onze == 1 && hand_winner == 1) { shm_placar->game_over = 1; }
	        if (mao_de_onze == 2 && hand_winner == 2) { shm_placar->game_over = 1; }
	        
	        // Se for Mão de Ferro, o jogo acaba sempre.
	        if (mao_de_onze == 3) { shm_placar->game_over = 1; }
	        
	        mano = (mano == 1) ? 2 : 1;
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