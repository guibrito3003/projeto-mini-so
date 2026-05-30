# Mini-SO — Simulador de Gerência de Processos e Memória
Alunos: Arthur Guilherme e Hellyne Andrade

> Projeto acadêmico em C que simula o comportamento básico de um Sistema Operacional,
> implementando escalonamento de CPU, gerência de memória RAM e prevenção de deadlock.

---

## Sumário

- [Visão Geral](#visão-geral)
- [Estrutura dos Arquivos](#estrutura-dos-arquivos)
- [Como Compilar e Executar](#como-compilar-e-executar)
- [Módulos Implementados](#módulos-implementados)
- [Arquivo de Entrada](#arquivo-de-entrada)
- [Saída do Programa](#saída-do-programa)
- [Exemplo de Execução](#exemplo-de-execução)

---

## Visão Geral

| Item               | Detalhe                          |
|--------------------|----------------------------------|
| Linguagem          | C (padrão C99)                   |
| Algoritmo de CPU   | Round-Robin                      |
| Quantum            | 2 segundos                       |
| RAM simulada       | 1024 bytes (1 KB)                |
| Algoritmo de Mem.  | First-Fit                        |
| Módulo extra       | Prevenção de Deadlock (Hierarquia) + Log em arquivo |

---

## Estrutura dos Arquivos

```
.
├── miniso.c          # Implementação completa (todos os módulos)
├── miniso.h          # Struct PCB, constantes e protótipos
├── processos.txt     # Arquivo de entrada com os processos
└── log_memoria.txt   # Gerado automaticamente após a simulação
```

---

## Como Compilar e Executar

### Compilar

```bash
gcc -o miniso miniso.c -Wall
```

### Executar

```bash
./miniso processos.txt
```

### Executar com arquivo personalizado

```bash
./miniso meu_arquivo.txt
```

> Se nenhum arquivo for passado como argumento, o programa tenta abrir `processos.txt`
> no diretório atual por padrão.

---

## Módulos Implementados

### Módulo 1 — Escalonamento Round-Robin

- Quantum fixo de **2 segundos**
- Processo não finalizado no quantum → retorna ao **fim da fila de prontos**
- Troca de contexto simulada impressa no console a cada preempção
- Fila de **espera separada** para processos aguardando memória disponível

### Módulo 2 — Gerência de Memória (First-Fit)

- Array de **1024 bytes** representando a RAM
- Alocação contígua pelo algoritmo **First-Fit** (primeiro bloco livre que couber)
- Desalocação imediata ao encerramento do processo
- Mapa visual da RAM impresso a cada unidade de tempo:

```
Mapa da RAM: [P1: 200B][P2: 100B][P3: 512B][P4: 128B][Livre: 84B]
```

### Módulo 3A — Prevenção de Deadlock (Hierarquia de Recursos)

- Regra: processo só pode solicitar o **Recurso 2** se já possuir o **Recurso 1**
- Violação da hierarquia → requisição **negada com mensagem no console**
- Implementado como verificação lógica (if/else) na admissão do processo

### Módulo 3B — Log em Arquivo

- Ao final da simulação, o arquivo `log_memoria.txt` é gerado automaticamente
- Contém: resumo de todos os processos (PID, burst, prioridade, tamanho, estado) e mapa final da RAM

---

## Arquivo de Entrada

### Formato

Cada linha representa um processo:

```
PID  BurstTime  TamanhoRAM  Prioridade
```

### Exemplo — `processos.txt`

```
1 10 200 2
2 4 100 1
3 6 512 3
4 2 128 2
```

| Campo      | Descrição                              |
|------------|----------------------------------------|
| PID        | Identificador único do processo        |
| BurstTime  | Tempo total de execução (segundos)     |
| TamanhoRAM | Espaço necessário na RAM (bytes)       |
| Prioridade | Nível de prioridade (informativo)      |

---

## Saída do Programa

A cada unidade de tempo, o simulador imprime no console:

```
--------------------------------------------------
Tempo Atual: 04 s
CPU: Executando PID 3 (Restam 5s)
Fila de Prontos: [PID 4] -> [PID 1] -> [PID 2]
Fila de Espera: [vazia]
Mapa da RAM: [P1: 200B][P2: 100B][P3: 512B][P4: 128B][Livre: 84B]
--------------------------------------------------
```

Eventos especiais são exibidos entre os blocos de status:

```
[PREEMPÇÃO] PID 1 retornou à fila. Restam 8s.
[ENCERRADO] PID 4 finalizado no tempo 8.
[DEADLOCK]  PID X tentou Recurso 2 sem ter Recurso 1. Negado.
```

---

## Exemplo de Execução

Com o `processos.txt` padrão (4 processos, RAM total = 940B de 1024B):

```
Tempo  CPU    Evento
-----  -----  ------------------------------------------
00     PID 1  Todos alocados em RAM
02     PID 2  Preempção PID 1 (restam 8s)
04     PID 3  Preempção PID 2 (restam 2s)
06     PID 4  Preempção PID 3 (restam 4s)
08     PID 1  PID 4 encerrado — RAM liberada (128B)
12     PID 3  PID 2 encerrado — RAM liberada (100B)
18     PID 1  PID 3 encerrado — RAM liberada (512B)
22     —      PID 1 encerrado — simulação concluída
```

**Tempo total de simulação: 22 segundos**

---

> Projeto desenvolvido para a disciplina de Sistemas Operacionais — 2º GQ 2026.
