#ifndef MINISO_H
#define MINISO_H

/* =====================================================
 *  Mini-SO: Simulador de Gerência de Processos e Memória
 *  Módulos: Escalonamento Round-Robin, Memória First-Fit,
 *           Prevenção de Deadlock (Hierarquia) e Log em arquivo
 * ===================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- Constantes Globais ---------- */
#define RAM_SIZE        1024   /* 1 KB de RAM simulada            */
#define QUANTUM         2      /* Quantum fixo para Round-Robin   */
#define MAX_PROCESSOS   64     /* Máximo de processos suportados  */
#define LOG_FILE        "log_memoria.txt"

/* ---------- Estados do Processo ---------- */
typedef enum {
    NOVO,
    PRONTO,
    EXECUTANDO,
    ESPERA,       /* aguardando memória                          */
    ENCERRADO
} Estado;

/* ---------- PCB – Process Control Block ---------- */
typedef struct {
    int    pid;
    int    burst_total;    /* tempo de execução original          */
    int    burst_restante; /* tempo restante de execução          */
    int    prioridade;
    int    tamanho;        /* bytes necessários na RAM             */
    Estado estado;
    int    mem_inicio;     /* posição inicial na RAM (-1 = livre)  */
    int    recurso1;       /* controle de Deadlock: 0=livre 1=alocado */
    int    recurso2;
} PCB;

/* ---------- Fila de prioridades (índices de processo) ----------
 * Ordenada por prioridade crescente (menor número = maior prioridade).
 * Dentro da mesma prioridade, mantém ordem de inserção (FIFO).
 * Round-Robin opera entre processos de mesma prioridade.
 * ---------------------------------------------------------------- */
typedef struct {
    int dados[MAX_PROCESSOS];
    int tamanho;
} Fila;

/* ---------- Protótipos ---------- */
void    fila_init           (Fila *f);
int     fila_vazia          (Fila *f);
void    fila_enqueue        (Fila *f, int idx, PCB procs[]);
void    fila_enqueue_simples(Fila *f, int idx);
int     fila_dequeue        (Fila *f);
int     fila_peek           (Fila *f);

int     mem_alocar     (unsigned char ram[], int tamanho, int pid);
void    mem_liberar    (unsigned char ram[], int inicio, int tamanho, int pid);
void    mem_imprimir   (unsigned char ram[]);

void    imprimir_status(int tempo, int cpu_idx, Fila *prontos, Fila *espera,
                        unsigned char ram[], PCB procs[], int n);
void    salvar_log     (unsigned char ram[], PCB procs[], int n, int tempo_total);

int     alocar_recurso1(PCB *p);
int     alocar_recurso2(PCB *p);
void    liberar_recursos(PCB *p);

const char *estado_str (Estado e);

#endif /* MINISO_H */