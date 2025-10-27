# Truco Mineiro em C (Cliente/Servidor) 🃏

Este projeto implementa o jogo Truco Mineiro em linguagem C, utilizando uma arquitetura cliente/servidor para permitir partidas entre dois jogadores via rede.

A solução emprega um **modelo de comunicação híbrido**, utilizando duas técnicas distintas de comunicação entre processos para diferentes finalidades:
1.  **Sockets TCP (`Winsock2`):** Para a comunicação **ativa e de turnos**, como o envio de jogadas, a distribuição de cartas e a sincronização de ações que exigem uma resposta imediata.
2.  **Memória Compartilhada (`Windows API`):** Para o gerenciamento do **estado global e passivo** do jogo, como o placar e a condição de fim de partida, permitindo que ambos os processos leiam o estado geral de forma eficiente.

O servidor gerencia toda a lógica do jogo, sendo a única fonte da verdade para as regras e pontuação.

## ✨ Funcionalidades

-   **Arquitetura Cliente/Servidor:** Jogo totalmente funcional em um ambiente local.
-   **Modelo de Comunicação Híbrido:** Uso combinado de Sockets para ações de turno e Memória Compartilhada para o placar global.
-   **Lógica do Truco Mineiro:** Implementação do ranking de cartas, incluindo as manilhas (Zap, Copeta, Espadilha e Pica-fumo).
-   **Sistema de Jogo Completo:** Controle de mãos, rodadas (vazas), contagem de pontos e determinação do vencedor da partida.
-   **Interface de Linha de Comando (CLI):** Interação simples e direta através do terminal.
-   **Multi-threading no Cliente:** A recepção de dados do servidor ocorre em uma thread separada para não bloquear a interface do usuário.

## 🛠️ Tecnologias Utilizadas

-   **Linguagem:** C
-   **Comunicação Ativa:** Sockets TCP (Biblioteca `Winsock2`)
-   **Comunicação Passiva:** Memória Compartilhada (API do Windows - `windows.h`)
-   **Compilador:** MinGW-w64 (GCC para Windows)
-   **Threading:** `pthreads` (utilizada no cliente)

## ⚙️ Pré-requisitos

Para compilar e executar este projeto, é necessário ter o seguinte ambiente configurado:

-   Windows 10 ou superior.
-   **MSYS2 com o toolchain MinGW-w64:** Essencial para ter acesso ao compilador `gcc` e às bibliotecas necessárias no Windows. Você pode baixá-lo [aqui](https://www.msys2.org/).

## 🚀 Como Compilar e Executar

Siga os passos abaixo para iniciar uma partida. É necessário ter **dois terminais** MinGW-w64 abertos. O servidor **deve ser iniciado primeiro** para criar a memória compartilhada.

### 1. Clone o Repositório

```bash
git clone [https://github.com/BrenoNosima/TrucoSO.git](https://github.com/BrenoNosima/TrucoSO.git)
cd TrucoSO
````

### 2\. Terminal 1 - Iniciar o Servidor

Neste terminal, você irá compilar e executar o servidor.

```bash
# Compilar o servidor
gcc servidor.c -o truco.exe -lws2_32

# Executar o servidor
./truco.exe
```

### 3\. Terminal 2 - Iniciar o Cliente

Neste segundo terminal, você irá compilar e executar o cliente.

```bash
# Compilar o cliente (é necessário linkar a biblioteca pthread)
gcc cliente.c -o cliente.exe -lws2_32 -lpthread

# Executar o cliente
./cliente.exe
```

Após executar, o cliente solicitará o endereço IP do servidor. Digite `127.0.0.1` e pressione Enter. O jogo começará\!

## 📂 Estrutura do Projeto

  - `servidor.c`: Contém a lógica principal do jogo e atua como o anfitrião. **Cria e escreve** na memória compartilhada e gerencia a comunicação ativa via sockets.
  - `cliente.c`: Conecta-se ao servidor para a troca de mensagens de turno e **lê** a memória compartilhada para exibir o placar atualizado.

## 👥 Autores

Este projeto foi desenvolvido por:

  - **Felipe Galeti Gôngora** - [FGaleti](https://www.google.com/search?q=https://github.com/FGaleti)
  - **Breno Nosima** - [BrenoNosima](https://www.google.com/search?q=https://github.com/BrenoNosima)
