/* =======================================================
 * cliente.c - Cliente do Truco Mineiro (Versão Final)
 * CUMPRE TODOS OS REQUISITOS: Sockets, Memória Compartilhada e
 * Interface de Jogo Completa.
 * Compilar:
 * gcc cliente.c -o cliente.exe -lws2_32 -lpthread
 * ======================================================= */
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORTA 12345
#define TAM_IP 30
#define HAND_SIZE 3
#define DECK_SIZE 40

// Definição para a Memória Compartilhada
#define SHM_NOME "TrucoPlacarSharedMemory"
typedef struct
{
    int p1_pts;
    int p2_pts;
    int game_over;
} SharedPlacar;

// Struct para comunicação via Socket
typedef struct
{
    int turn;
    int hand_num;
    int round_num;
    int client_hand[HAND_SIZE];
    int server_card_on_table;
    int client_card_on_table;
    char message[100]; // Campo para mensagens de status
    // Apostas da mão (Truco)
    int hand_value;              // 1,3,6,9,12
    int truco_pending;           // 0/1
    int truco_requester;         // 1=Servidor, 2=Cliente
    int truco_target;            // próximo valor se aceito
    int awaiting_truco_response; // 0/1
    int raise_rights;            // 1=Servidor, 2=Cliente
    int message_seq;             // sequência de mensagem para evitar duplicação
} GameState;

// Códigos de comando do cliente -> servidor
#define CMD_TRUCO_REQ   -100
#define CMD_TRUCO_ACCEPT -101
#define CMD_TRUCO_RUN   -102

// Histórico da mão atual (cliente local)
typedef struct {
    int rodadaAtual;
    char resultadoRodada[3][50];
} HistoricoMao;

HistoricoMao historico;

void iniciarNovaMao() {
    historico.rodadaAtual = 0;
    for (int i = 0; i < 3; i++) strcpy(historico.resultadoRodada[i], "-");
}

void registrarResultadoRodada(const char *vencedor) {
    if (historico.rodadaAtual < 3) {
        strcpy(historico.resultadoRodada[historico.rodadaAtual], vencedor);
        historico.rodadaAtual++;
    }
}

void exibirHistorico() {
    printf("\n--- Histórico da Mão Atual ---\n");
    for (int i = 0; i < 3; i++) printf("Rodada %d: %s\n", i+1, historico.resultadoRodada[i]);
    printf("-----------------------------\n");
}

// Função de ranking (mesma lógica do servidor)
int truco_ranking(int card_index) {
    if (card_index < 0 || card_index >= DECK_SIZE) return -1;
    int value = card_index / 4;
    int suit = card_index % 4;
    if (value == 3 && suit == 0) return 100;
    if (value == 6 && suit == 1) return 99;
    if (value == 0 && suit == 2) return 98;
    if (value == 6 && suit == 3) return 97;
    if (value == 2) return 90;
    if (value == 1) return 80;
    if (value == 0) return 70;
    if (value == 9) return 60;
    if (value == 7) return 50;
    if (value == 8) return 40;
    if (value == 6 && suit == 0) return 39;
    if (value == 6 && suit == 2) return 38;
    if (value == 5) return 30;
    if (value == 4) return 20;
    if (value == 3) return 10;
    return 0;
}

const char *valor_nome[10] = {"A", "2", "3", "4", "5", "6", "7", "J", "Q", "K"};
const char *naipe_nome[4] = {"Paus", "Copas", "Espadas", "Ouros"};
void print_card(int card_index)
{
    if (card_index < 0 || card_index >= DECK_SIZE)
    {
        printf("(vazia)");
        return;
    }
    int value = card_index / 4;
    int suit = card_index % 4;
    printf("%s de %s", valor_nome[value], naipe_nome[suit]);
}

SOCKET sock;
GameState gameState;
SharedPlacar *shm_placar;
int jogoAtivo = 1;

void display_game()
{
    system("cls");
    printf("=== MÃO %d | Rodada %d ===\n", gameState.hand_num, gameState.round_num);
    printf("PLACAR: Servidor %d x %d Você(Cliente)\n", shm_placar->p1_pts, shm_placar->p2_pts);
    printf("VALENDO: %d ponto(s)\n", gameState.hand_value > 0 ? gameState.hand_value : 1);
    printf("----------------------------------\n");
    printf("MESA:\n");
    printf("  Servidor: ");
    if (gameState.server_card_on_table != -1)
        print_card(gameState.server_card_on_table);
    else
        printf("---");
    printf("\n  Você(Cliente):     ");
    if (gameState.client_card_on_table != -1)
        print_card(gameState.client_card_on_table);
    else
        printf("---");
    printf("\n----------------------------------\n");
    printf("SUA MÃO:\n");
    for (int i = 0; i < HAND_SIZE; i++)
    {
        if (gameState.client_hand[i] != -1)
        {
            printf("  %d) ", i + 1);
            print_card(gameState.client_hand[i]);
            printf("\n");
        }
    }
    printf("----------------------------------\n");

    // Mensagem de status da rodada (convertida para perspectiva do cliente)
    static int last_seq_printed = -1;
    if (gameState.message[0] != '\0' && gameState.message_seq != last_seq_printed)
    {
        char client_message[100];
        strcpy(client_message, gameState.message);
        if (strstr(client_message, "Você (Servidor)"))
            strcpy(client_message, "O Servidor venceu a rodada!");
        else if (strstr(client_message, "O Cliente"))
            strcpy(client_message, "Parabéns, você(Cliente) venceu a rodada!");
        printf(">>> %s <<<\n", client_message);
        printf("----------------------------------\n");
        last_seq_printed = gameState.message_seq;
    }

    // Exibir histórico antes do prompt, para manter o prompt como última linha
    exibirHistorico();

    if (gameState.turn == 2) {
        printf(">>> SUA VEZ DE JOGAR! <<<\n");
        if (gameState.hand_value < 12 && gameState.raise_rights == 2 && !gameState.awaiting_truco_response) {
            int nv = (gameState.hand_value>=12?12:(gameState.hand_value>=9?12:(gameState.hand_value>=6?9:(gameState.hand_value>=3?6:3))));
            printf("(1-3 carta, 9=PEDIR %d) ", nv);
        } else printf("(1-3 carta) ");
        fflush(stdout);
    } else {
        printf(">>> Aguardando jogada do servidor... <<<\n");
    }
}

DWORD WINAPI receber_thread(LPVOID arg)
{
    (void)arg;
    while (jogoAtivo)
    {
        int bytes = recv(sock, (char *)&gameState, sizeof(GameState), 0);
        if (bytes <= 0)
        {
            printf("\n[socket] Servidor desconectou.\n");
            jogoAtivo = 0;
            break;
        }
        if (shm_placar->game_over)
            jogoAtivo = 0;
        // Detecta mudança de mão/rodada para gerenciar histórico local
        static int last_hand_num = 0;
        static int last_round_num = 0;

        if (gameState.hand_num != last_hand_num) {
            // Nova mão: reinicia histórico local
            iniciarNovaMao();
            last_hand_num = gameState.hand_num;
            last_round_num = 0;
        }

        // Quando a rodada mudou e ambas as cartas estão na mesa, registra o resultado
        if (gameState.round_num != last_round_num && gameState.server_card_on_table != -1 && gameState.client_card_on_table != -1) {
            int r_server = truco_ranking(gameState.server_card_on_table);
            int r_client = truco_ranking(gameState.client_card_on_table);
            if (r_server > r_client) registrarResultadoRodada("Servidor");
            else if (r_client > r_server) registrarResultadoRodada("Cliente");
            else registrarResultadoRodada("Empate");
            last_round_num = gameState.round_num;
        }

        display_game();
    }
    return 0;
}

int main()
{
    SetConsoleOutputCP(65001);

    HANDLE hMapFile;
    hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NOME);
    if (hMapFile == NULL)
    {
        printf("Erro ao abrir file mapping (%d).\nO servidor precisa ser iniciado primeiro.\n", GetLastError());
        getchar();
        return 1;
    }
    shm_placar = (SharedPlacar *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedPlacar));
    if (shm_placar == NULL)
    {
        printf("Erro ao mapear a view do arquivo (%d).\n", GetLastError());
        CloseHandle(hMapFile);
        getchar();
        return 1;
    }
    printf("[mem_share] Conectado à memória compartilhada do placar.\n");

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
    if (connect(sock, (struct sockaddr *)&servidor, sizeof(servidor)) == SOCKET_ERROR)
    {
        printf("Erro ao conectar ao servidor.\n");
        closesocket(sock);
        getchar();
        return 1;
    }

    HANDLE thread_id = CreateThread(NULL, 0, receber_thread, NULL, 0, NULL);
    if (thread_id == NULL) {
        printf("Erro ao criar thread de recepção.\n");
        closesocket(sock);
        return 1;
    }

    while (jogoAtivo)
    {
        // Se há um pedido de TRUCO do servidor aguardando resposta, perguntar imediatamente
        if (gameState.awaiting_truco_response && gameState.truco_requester == 1)
        {
            int opc = 0; int ch;
            do {
                printf("Servidor pediu TRUCO! Aceita (1) ou corre (2)?: ");
                if (scanf("%d", &opc) != 1) {
                    while ((ch = getchar()) != '\n' && ch != EOF) ;
                    opc = 0; continue;
                }
                while ((ch = getchar()) != '\n' && ch != EOF) ;
            } while (opc != 1 && opc != 2);

            int cmd = (opc == 1) ? CMD_TRUCO_ACCEPT : CMD_TRUCO_RUN;
            send(sock, (char *)&cmd, sizeof(int), 0);
            // Após enviar resposta, aguardar estado atualizado do servidor pelo thread receptor
            Sleep(100);
        }

        if (gameState.turn == 2)
        {
            int indice_escolhido = -1;
            int ch;
            do
            {
                if (scanf("%d", &indice_escolhido) != 1)
                {
                    while ((ch = getchar()) != '\n' && ch != EOF)
                        ;
                    indice_escolhido = -1;
                    printf("Entrada inválida. Informe um número entre 1 e 3.\n");
                    if (gameState.hand_value < 12) printf("(1-3 carta, 9=TRUCO) "); else printf("(1-3 carta) ");
                    fflush(stdout);
                    continue;
                }
                while ((ch = getchar()) != '\n' && ch != EOF)
                    ;
                if (indice_escolhido == 9) {
                    // Pedir TRUCO
                    if (gameState.hand_value >= 12 || gameState.raise_rights != 2 || gameState.awaiting_truco_response) {
                        printf("Já está valendo 12. Não é possível aumentar.\n");
                        continue;
                    }
                    int cmd = CMD_TRUCO_REQ;
                    send(sock, (char *)&cmd, sizeof(int), 0);
                    // Não altera o turno; aguarda resposta do servidor
                    indice_escolhido = -1; // para sair do loop
                    break;
                }
                indice_escolhido--;
                if (indice_escolhido < 0 || indice_escolhido >= 3 || gameState.client_hand[indice_escolhido] == -1) {
                    if (gameState.hand_value < 12) printf("Carta indisponível. (1-3 carta, 9=TRUCO): "); else printf("Carta indisponível. (1-3 carta): ");
                    fflush(stdout);
                }
            } while ((indice_escolhido < 0 || indice_escolhido >= 3 || gameState.client_hand[indice_escolhido] == -1) && indice_escolhido != -1);

            if (indice_escolhido != -1) {
                send(sock, (char *)&indice_escolhido, sizeof(int), 0);
                gameState.turn = 1;
                display_game();
            }
        }
        Sleep(100);
    }

    WaitForSingleObject(thread_id, INFINITE);
    CloseHandle(thread_id);

    printf("\n--- FIM DE JOGO ---\n");
    printf("Placar final: Servidor %d x %d Você(Cliente)\n", shm_placar->p1_pts, shm_placar->p2_pts);
    printf("\n[encerrado] Pressione Enter para sair.\n");
    getchar();
    getchar();

    UnmapViewOfFile(shm_placar);
    CloseHandle(hMapFile);
    closesocket(sock);
    WSACleanup();

    return 0;
}