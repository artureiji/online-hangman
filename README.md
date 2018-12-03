# HangMan Evolution™ Multiplayer Online

Pojeto final da disciplina MC833. Desenvolvido _orgulhosamente_ por Samuel e Artur Eiji.

Implementação do jogo da forca através de uma arquitetura cliente servidor. O jogo pode tanto ser jogado em modo singleplayer quanto multiplayer.

# Começando

Ao final destas instruções você terá uma cópia do projeto rodando no seu computador para fins de desenvolvimento e testes. Veja a sessão _deployment_ para notas sobre como dar deploy numa versão funcional do projeto.

## Pré requisitos

O projeto foi desenvolvido para ser executado exclusivamente em sistemas Linux.
São necessárias apenas as bibliotecas padrões da linguagem C.

## Instalação

A compilação do código se dá através do sistema Make.
Para isso, simplesmente execute o comando _make_ na raíz do projeto:

```
# make
```

O projeto compilado se resume a dois executáveis: _cliente_ e _servidor_.
Você deverá executar uma instância do servidor, especificando a porta (default 9000):

```
./servidor 9000
```

Em seguida, você poderá executar uma instância do cliente, especificando ip e porta onde o servidor está rodando:

```
./cliente 127.0.0.1 9000
```

# Deployment

Simplesmente rode o executável _servidor_ especificando um IP e porta no qual os clientes poderão se conectar.
Pode exigir liberação de portas pelo firewall.


# Autores

 - **Artur Eiji**
 - **Samuel Chenatti**

# Licença

Este projeto está sob a licença MIT - veja LICENSE.md para mais detalhes.
