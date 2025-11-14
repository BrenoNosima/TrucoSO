# Truco Skadoosh (1x1) - Cliente/Servidor

Este projeto implementa o jogo de Truco na modalidade 1 contra 1, utilizando arquitetura Cliente/Servidor com comunicação via Sockets (Winsock2) e Memória Compartilhada para o placar.

## 🃏 Regras do Jogo

O jogo segue as regras básicas do Truco Mineiro, com as seguintes especificações de pontuação:

*   **Pontuação Normal:** Cada mão (rodada de 3 vazas) vale **1 ponto**.
*   **Truco:** O único aumento de aposta disponível é o **Truco**, que aumenta o valor da mão para **3 pontos**.
*   **Regra de Aposta Única:** O Truco só pode ser pedido **uma vez por mão**. Se for aceito, a mão passa a valer 3 pontos e não pode haver mais aumentos até a próxima mão.
*   **Correr:** Se um jogador pede Truco e o adversário "corre", o jogador que pediu Truco ganha **1 ponto** (o valor anterior da mão).
*   **Mão de Onze:** O jogador que atinge 11 pontos decide se joga a mão (valendo 1 ponto) ou corre (dando 1 ponto ao adversário e perdendo o jogo). O Truco é desabilitado.
*   **Mão de Ferro:** Se ambos os jogadores atingem 11 pontos, a mão vale **12 pontos** e é jogada "às escuras" (sem Truco). O vencedor da mão vence o jogo.

## 💻 Como Compilar e Executar

O projeto é escrito em C e utiliza a biblioteca Winsock2, sendo ideal para compilação em ambientes Windows (como MinGW).

### 1. Compilação

Utilize os seguintes comandos no seu terminal (ex: PowerShell ou Prompt de Comando com MinGW configurado):

```bash
# Compilar o Servidor
gcc truco.c -o truco.exe -lws2_32

# Compilar o Cliente
gcc client.c -o cliente.exe -lws2_32 
```

### 2. Execução

1.  **Iniciar o Servidor:**
    Abra um terminal e execute o servidor.
    ```bash
    ./truco.exe
    ```
    O servidor aguardará a conexão do cliente.

2.  **Iniciar o Cliente:**
    Abra um segundo terminal e execute o cliente.
    ```bash
    ./cliente.exe
    ```
    O cliente solicitará o IP do servidor (geralmente `127.0.0.1` se estiver na mesma máquina).

## 🎮 Comandos de Jogo

Durante o seu turno, você terá as seguintes opções:

| Comando | Ação | Descrição |
| :---: | :--- | :--- |
| **1** | Jogar Carta 1 | Joga a primeira carta da sua mão. |
| **2** | Jogar Carta 2 | Joga a segunda carta da sua mão. |
| **3** | Jogar Carta 3 | Joga a terceira carta da sua mão. |
| **9** | Pedir Truco | Aumenta o valor da mão para 3 pontos (disponível apenas se a mão vale 1 ponto e o Truco ainda não foi pedido). |
| **1** | Aceitar Truco | (Quando o adversário pede Truco) Aceita o Truco, e a mão passa a valer 3 pontos. |
| **2** | Correr | (Quando o adversário pede Truco) Desiste da mão, e o adversário ganha 1 ponto. |

**Observação:** O jogador que pede Truco (comando **9**) e tem o Truco aceito, deve ser o próximo a jogar a carta. O prompt de jogada aparecerá imediatamente após a aceitação.
`)
