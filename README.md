# Truco Mineiro em C (Cliente/Servidor) 🃏

Este projeto implementa o jogo Truco Mineiro em linguagem C, utilizando uma arquitetura cliente/servidor para permitir partidas entre dois jogadores via rede. A comunicação é realizada através de Sockets TCP, com a biblioteca `Winsock2` para o ambiente Windows.

O servidor gerencia toda a lógica do jogo, como distribuição de cartas, contagem de pontos, controle de turnos e validação das regras, garantindo que seja a única fonte da verdade. O cliente é responsável por se conectar ao servidor, exibir o estado atual do jogo e enviar as ações do jogador.

## ✨ Funcionalidades Atuais

  - **Arquitetura Cliente/Servidor:** Jogo totalmente funcional em rede local (localhost).
  - **Comunicação em Tempo Real:** Uso de Sockets TCP para uma comunicação estável e sequencial.
  - **Lógica do Truco Mineiro:** Implementação do ranking de cartas, incluindo as manilhas (Zap, Copeta, Espadilha e Pica-fumo).
  - **Sistema de Jogo Completo:** Controle de mãos, rodadas (vazas), contagem de pontos e determinação do vencedor da partida.
  - **Interface de Linha de Comando (CLI):** Interação simples e direta através do terminal para ambos os jogadores.
  - **Multi-threading no Cliente:** A recepção de dados do servidor ocorre em uma thread separada para não bloquear a interface do usuário.

## 🛠️ Tecnologias Utilizadas

  - **Linguagem:** C
  - **Comunicação em Rede:** Sockets TCP (Biblioteca `Winsock2` para Windows)
  - **Compilador:** MinGW-w64 (GCC para Windows)
  - **Threading:** `pthreads` (utilizada no cliente)

## ⚙️ Pré-requisitos

Para compilar e executar este projeto, é necessário ter o seguinte ambiente configurado:

  - Windows 10 ou superior.
  - **MSYS2 com o toolchain MinGW-w64:** Essencial para ter acesso ao compilador `gcc` e às bibliotecas necessárias no Windows. Você pode baixá-lo [aqui](https://www.msys2.org/).

## 🚀 Como Compilar e Executar

Siga os passos abaixo para iniciar uma partida. É necessário ter **dois terminais** MinGW-w64 abertos.

### 1\. Clone o Repositório

```bash
git clone https://github.com/BrenoNosima/TrucoSO.git
cd TrucoSO
```

### 2\. Terminal 1 - Iniciar o Servidor

Neste terminal, você irá compilar e executar o servidor. Ele ficará aguardando a conexão do cliente.

```bash
# Compilar o servidor
gcc servidor.c -o truco.exe -lws2_32

# Executar o servidor
./truco.exe
```

### 3\. Terminal 2 - Iniciar o Cliente

Neste segundo terminal, você irá compilar e executar o cliente para se conectar ao servidor.

```bash
# Compilar o cliente (é necessário linkar a biblioteca pthread)
gcc cliente.c -o cliente.exe -lws2_32 -lpthread

# Executar o cliente
./cliente.exe
```

Após executar, o cliente solicitará o endereço IP do servidor. Digite `127.0.0.1` e pressione Enter para se conectar localmente.

O jogo começará\!

## 📂 Estrutura do Projeto

  - `servidor.c`: Contém toda a lógica principal do jogo, gerenciamento de estado, regras de negócio e comunicação com o cliente. Atua como o anfitrião da partida.
  - `cliente.c`: Responsável por se conectar ao servidor, exibir a interface para o jogador (mão, placar, mesa) e capturar o input do usuário para enviar ao servidor.

## 👥 Autores

Este projeto está sendo desenvolvido por:

  - **Felipe Galeti Gôngora** - [FGaleti](https://www.google.com/search?q=https://github.com/FGaleti)
  - **Breno Nosima** - [BrenoNosima](https://www.google.com/search?q=https://github.com/BrenoNosima)