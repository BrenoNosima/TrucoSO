# 🎴 Truco Mineiro — Implementação em C com Sockets (Cliente/Servidor)

## 🧠 Descrição Geral

Este projeto implementa o **jogo Truco Mineiro** em linguagem **C**, com comunicação entre dois jogadores via **sockets TCP/IP**, utilizando a biblioteca **Winsock2.h**.

A aplicação é dividida em **duas partes**:

* 🖥️ **Servidor (Jogador 1)**
* 💻 **Cliente (Jogador 2)**

O servidor cria uma conexão local (porta 8080) e gerencia a partida, enquanto o cliente se conecta e interage enviando suas jogadas.
O jogo simula um **Truco simplificado**, com cálculo de força das cartas, três rodadas por partida e contagem de pontos até um dos jogadores alcançar **12 pontos**.

---

## ⚙️ Tecnologias Utilizadas

* Linguagem **C**
* **Winsock2.h** — para comunicação entre processos via rede (sockets TCP)
* **stdlib.h**, **string.h**, **time.h** — para manipulação de dados, strings e geração aleatória
* Testado em **Windows 10/11** com **MinGW**

---

## 🧩 Estrutura do Projeto

```
TrucoMineiro/
├── server.c    # Código do Servidor (Jogador 1)
├── client.c    # Código do Cliente (Jogador 2)
├── README.md   # Documentação do projeto
```

---

## 🕹️ Como Funciona o Jogo

1. O servidor embaralha o baralho (40 cartas do truco).
2. Cada jogador recebe **3 cartas**.
3. A partida ocorre em **melhor de 3 rodadas**:

   * Cada jogador escolhe uma carta (1, 2 ou 3).
   * As cartas são comparadas por **força**, e o vencedor leva a rodada.
4. O jogador que vencer **duas rodadas** ganha a partida e soma 1 ponto.
5. O jogo continua até alguém alcançar **12 pontos**.

---

## 💬 Protocolo de Comunicação

A comunicação ocorre por **mensagens trocadas entre servidor e cliente** através de sockets TCP.

| Direção            | Exemplo de Mensagem                     | Descrição                                                |                     |                         |
| ------------------ | --------------------------------------- | -------------------------------------------------------- | ------------------- | ----------------------- |
| Servidor → Cliente | `"Suas cartas: 5 de Copas               | 2 de Espadas                                             | A de Ouros"`        | Envia cartas do jogador |
| Servidor → Cliente | `"Rodada 1 - Escolha sua carta (1-3):"` | Solicita jogada                                          |                     |                         |
| Cliente → Servidor | `"2"`                                   | Jogador escolhe carta 2                                  |                     |                         |
| Servidor → Cliente | `"Rodada 1: Você jogou 2 de Espadas     | Adversário jogou 3 de Paus -> Cliente ganhou a rodada!"` | Resultado da rodada |                         |
| Servidor → Cliente | `"Placar: Servidor 3 x 1 Cliente"`      | Atualiza placar                                          |                     |                         |
| Servidor → Cliente | `"Fim de jogo! Servidor venceu!"`       | Finaliza partida                                         |                     |                         |

---

## 🧮 Cálculo da Força das Cartas

A função `calcularForca()` define a hierarquia das cartas do **Truco Mineiro**, onde as **manilhas** possuem maior valor:

| Carta | Naipe    | Força | Descrição         |
| ----- | -------- | ----- | ----------------- |
| 4     | Paus     | 14    | Zap (maior carta) |
| 7     | Copas    | 13    | Segunda manilha   |
| A     | Espadas  | 12    | Terceira manilha  |
| 7     | Ouros    | 11    | Quarta manilha    |
| 3     | Qualquer | 10    | Alta              |
| 2     | Qualquer | 9     | Alta              |
| A     | Outros   | 8     | Média             |
| K     | Qualquer | 7     | Média             |
| J     | Qualquer | 6     | Média             |
| Q     | Qualquer | 5     | Média             |
| 7     | Outros   | 4     | Baixa             |
| 6     | Qualquer | 3     | Baixa             |
| 5     | Qualquer | 2     | Baixa             |
| 4     | Outros   | 1     | Menor             |

---

## 🧱 Lógica de Execução

### Servidor (`server.c`)

* Cria o socket (`socket()`)
* Define o endereço e porta (`bind()`)
* Aguarda conexão do cliente (`listen()` / `accept()`)
* Embaralha e distribui cartas
* Controla as rodadas e pontuação
* Envia mensagens e recebe respostas do cliente
* Exibe logs da partida no terminal

### Cliente (`client.c`)

* Conecta ao servidor via IP (`127.0.0.1`)
* Recebe as cartas e mensagens do servidor
* Envia respostas (escolha de carta)
* Exibe todas as mensagens no terminal

---

## ⚙️ Compilação

### No Windows (MinGW)

```bash
gcc server.c -o servidor.exe -lws2_32
gcc client.c -o cliente.exe -lws2_32
```

---

## ▶️ Execução

### 1️⃣ Inicie o Servidor

```bash
servidor.exe
```

### 2️⃣ Em Outro Terminal, Inicie o Cliente

```bash
cliente.exe
```

Os dois terminais devem exibir as mensagens trocadas, por exemplo:

```
[Servidor]
Suas cartas: 5 de Ouros | Q de Espadas | 7 de Copas
Jogador conectado!
Rodada 1 - Escolha sua carta (1-3): 2
Rodada 1: Você jogou Q de Espadas | Adversário jogou 7 de Copas -> Servidor ganhou a rodada!
Placar: Servidor 1 x 0 Cliente
```

```
[Cliente]
Suas cartas: 4 de Copas | A de Ouros | 2 de Espadas
Rodada 1 - Escolha sua carta (1-3):
> 1
Rodada 1: Você jogou 4 de Copas | Adversário jogou 7 de Copas -> Cliente perdeu a rodada!
Placar: Servidor 1 x 0 Cliente
```

---

## 🔐 Recursos e Técnicas Utilizadas

| Recurso                             | Aplicação                               |
| ----------------------------------- | --------------------------------------- |
| **Sockets TCP (Winsock2)**          | Comunicação entre servidor e cliente    |
| **Mensagens em strings**            | Troca de dados estruturados via texto   |
| **Função `embaralhar()`**           | Geração aleatória da ordem das cartas   |
| **Função `calcularForca()`**        | Lógica de força do Truco Mineiro        |
| **Controle de pontuação e rodadas** | Vence quem atingir 12 pontos            |
| **Buffers de comunicação**          | Uso de `send()` e `recv()` com `char[]` |


## 🧑‍💻 Autores
Breno  Nosima
Felipe Galeti Gôngora

## 🧾 Avaliação

| Critério               | Descrição                                 
| ---------------------- | ----------------------------------------- 
| Originalidade          | Implementação fiel do Truco Mineiro             |
| Troca de mensagens     | Comunicação clara e eficiente via sockets 
| Técnicas implementadas | Embaralhamento, força das cartas, placar  
| Criatividade           | Regras simplificadas e notação legível    
| Organização e clareza  | Código estruturado e comentado            

---

## 🏁 Conclusão

O projeto **Truco Mineiro** demonstra o uso de **comunicação entre processos** via rede, utilizando **sockets TCP** em C.
Além disso, mostra uma aplicação prática de conceitos de **concorrência**, **troca de mensagens** e **sincronização de jogadas**.

O resultado é um jogo funcional, simples e divertido — um exemplo didático de como unir **programação em rede** e **lógica de jogos clássicos**.

