# Truco Mineiro em C (Cliente/Servidor) ðŸƒ

Este projeto implementa o jogo Truco Mineiro em linguagem C, utilizando uma arquitetura cliente/servidor para permitir partidas entre dois jogadores via rede.

A soluÃ§Ã£o emprega um **modelo de comunicaÃ§Ã£o hÃ­brido**, utilizando duas tÃ©cnicas distintas de comunicaÃ§Ã£o entre processos para diferentes finalidades:
1.  **Sockets TCP (`Winsock2`):** Para a comunicaÃ§Ã£o **ativa e de turnos**, como o envio de jogadas, a distribuiÃ§Ã£o de cartas e a sincronizaÃ§Ã£o de aÃ§Ãµes que exigem uma resposta imediata.
2.  **MemÃ³ria Compartilhada (`Windows API`):** Para o gerenciamento do **estado global e passivo** do jogo, como o placar e a condiÃ§Ã£o de fim de partida, permitindo que ambos os processos leiam o estado geral de forma eficiente.

O servidor gerencia toda a lÃ³gica do jogo, sendo a Ãºnica fonte da verdade para as regras e pontuaÃ§Ã£o.

## âœ¨ Funcionalidades

-   **Arquitetura Cliente/Servidor:** Jogo totalmente funcional em um ambiente local.
-   **Modelo de ComunicaÃ§Ã£o HÃ­brido:** Uso combinado de Sockets para aÃ§Ãµes de turno e MemÃ³ria Compartilhada para o placar global.
-   **LÃ³gica do Truco Mineiro:** ImplementaÃ§Ã£o do ranking de cartas, incluindo as manilhas (Zap, Copeta, Espadilha e Pica-fumo).
-   **Sistema de Jogo Completo:** Controle de mÃ£os, rodadas (vazas), contagem de pontos e determinaÃ§Ã£o do vencedor da partida.
-   **Interface de Linha de Comando (CLI):** InteraÃ§Ã£o simples e direta atravÃ©s do terminal.
-   **Multi-threading no Cliente:** A recepÃ§Ã£o de dados do servidor ocorre em uma thread separada para nÃ£o bloquear a interface do usuÃ¡rio.

## ðŸ› ï¸ Tecnologias Utilizadas

-   **Linguagem:** C
-   **ComunicaÃ§Ã£o Ativa:** Sockets TCP (Biblioteca `Winsock2`)
-   **ComunicaÃ§Ã£o Passiva:** MemÃ³ria Compartilhada (API do Windows - `windows.h`)
-   **Compilador:** MinGW-w64 (GCC para Windows)
-   **Threading:** `pthreads` (utilizada no cliente)

## âš™ï¸ PrÃ©-requisitos

Para compilar e executar este projeto, Ã© necessÃ¡rio ter o seguinte ambiente configurado:

-   Windows 10 ou superior.
-   **MSYS2 com o toolchain MinGW-w64:** Essencial para ter acesso ao compilador `gcc` e Ã s bibliotecas necessÃ¡rias no Windows. VocÃª pode baixÃ¡-lo [aqui](https://www.msys2.org/).

## ðŸš€ Como Compilar e Executar

Siga os passos abaixo para iniciar uma partida. Ã‰ necessÃ¡rio ter **dois terminais** MinGW-w64 abertos. O servidor **deve ser iniciado primeiro** para criar a memÃ³ria compartilhada.

### 1. Clone o RepositÃ³rio

```bash
git clone [https://github.com/BrenoNosima/TrucoSO.git](https://github.com/BrenoNosima/TrucoSO.git)
cd TrucoSO
````

### 2\. Terminal 1 - Iniciar o Servidor

Neste terminal, vocÃª irÃ¡ compilar e executar o servidor.

```bash
# Compilar o servidor
gcc servidor.c -o truco.exe -lws2_32

# Executar o servidor
./truco.exe
```

### 3\. Terminal 2 - Iniciar o Cliente

Neste segundo terminal, vocÃª irÃ¡ compilar e executar o cliente.

```bash
# Compilar o cliente (Ã© necessÃ¡rio linkar a biblioteca pthread)
gcc cliente.c -o cliente.exe -lws2_32 -lpthread

# Executar o cliente
./cliente.exe
```

ApÃ³s executar, o cliente solicitarÃ¡ o endereÃ§o IP do servidor. Digite `127.0.0.1` e pressione Enter. O jogo comeÃ§arÃ¡\!

## ðŸ“‚ Estrutura do Projeto

  - `servidor.c`: ContÃ©m a lÃ³gica principal do jogo e atua como o anfitriÃ£o. **Cria e escreve** na memÃ³ria compartilhada e gerencia a comunicaÃ§Ã£o ativa via sockets.
  - `cliente.c`: Conecta-se ao servidor para a troca de mensagens de turno e **lÃª** a memÃ³ria compartilhada para exibir o placar atualizado.

## ðŸ‘¥ Autores

Este projeto foi desenvolvido por:

  - **Felipe Galeti GÃ´ngora** - [FGaleti](https://www.google.com/search?q=https://github.com/FGaleti)
  - **Breno Nosima** - [BrenoNosima](https://www.google.com/search?q=https://github.com/BrenoNosima)
