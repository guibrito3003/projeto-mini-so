/* =====================================================
 *  Mini-SO: Simulador de Gerência de Processos e Memória
 *  Arquivo: miniso.c
 *
 *  Compilar: gcc -o miniso miniso.c -Wall
 *  Executar: ./miniso processos.txt
 * ===================================================== */

#include "miniso.h"

/* ====================================================
 *  MÓDULO AUXILIAR: Fila de Prioridades
 *
 *  Ordenação: prioridade crescente (1 = mais urgente).
 *  Dentro da mesma prioridade: FIFO (ordem de inserção preservada).
 *  Isso garante Round-Robin correto entre processos de igual prioridade.
 * ==================================================== */

void fila_init(Fila *f) {
    f->tamanho = 0;
}

int fila_vazia(Fila *f) {
    return f->tamanho == 0;
}

/*
 * Insere `idx` na posição correta por prioridade.
 * Processos de menor número de prioridade ficam na frente.
 * Processos de mesma prioridade ficam atrás dos já existentes (FIFO).
 */
void fila_enqueue(Fila *f, int idx, PCB procs[]) {
    if (f->tamanho >= MAX_PROCESSOS) {
        fprintf(stderr, "[ERRO] Fila cheia.\n");
        return;
    }
    /* Encontra posição de inserção: após todos com prioridade <= */
    int pos = f->tamanho;
    for (int i = 0; i < f->tamanho; i++) {
        if (procs[idx].prioridade < procs[f->dados[i]].prioridade) {
            pos = i;
            break;
        }
    }
    /* Desloca elementos para abrir espaço */
    for (int i = f->tamanho; i > pos; i--)
        f->dados[i] = f->dados[i - 1];
    f->dados[pos] = idx;
    f->tamanho++;
}

/* Fila de espera não usa prioridade — FIFO simples */
void fila_enqueue_simples(Fila *f, int idx) {
    if (f->tamanho >= MAX_PROCESSOS) {
        fprintf(stderr, "[ERRO] Fila cheia.\n");
        return;
    }
    f->dados[f->tamanho++] = idx;
}

int fila_dequeue(Fila *f) {
    if (fila_vazia(f)) return -1;
    int val = f->dados[0];
    for (int i = 0; i < f->tamanho - 1; i++)
        f->dados[i] = f->dados[i + 1];
    f->tamanho--;
    return val;
}

int fila_peek(Fila *f) {
    if (fila_vazia(f)) return -1;
    return f->dados[0];
}

/* ====================================================
 *  MÓDULO 2: Gerência de Memória (First-Fit)
 *
 *  A RAM é representada por um array de 1024 bytes (unsigned char).
 *  Cada posição contém 0 (livre) ou o PID do processo ocupante.
 * ==================================================== */

/*
 * Tenta alocar `tamanho` bytes contíguos na RAM usando First-Fit.
 * Retorna a posição inicial do bloco alocado ou -1 se não houver espaço.
 */
int mem_alocar(unsigned char ram[], int tamanho, int pid) {
    int inicio = -1;
    int contador = 0;

    for (int i = 0; i <= RAM_SIZE; i++) {
        /* Ao atingir o fim ou encontrar posição ocupada, reseta busca */
        if (i == RAM_SIZE || ram[i] != 0) {
            if (contador >= tamanho) {
                /* Bloco encontrado: aloca */
                inicio = i - contador;
                for (int j = inicio; j < inicio + tamanho; j++)
                    ram[j] = (unsigned char) pid;
                return inicio;
            }
            contador = 0;
        } else {
            contador++;
        }
    }
    return -1; /* sem espaço */
}

/*
 * Libera os bytes ocupados pelo processo a partir de `inicio`.
 */
void mem_liberar(unsigned char ram[], int inicio, int tamanho, int pid) {
    (void) pid; /* parâmetro reservado para extensões futuras */
    for (int i = inicio; i < inicio + tamanho && i < RAM_SIZE; i++)
        ram[i] = 0;
}

/*
 * Imprime representação compacta da RAM: blocos contíguos são agrupados.
 * Exemplo: [P1: 200B][P2: 100B][Livre: 724B]
 */
void mem_imprimir(unsigned char ram[]) {
    int i = 0;
    printf("Mapa da RAM: ");
    while (i < RAM_SIZE) {
        int pid_bloco = ram[i];
        int inicio    = i;
        while (i < RAM_SIZE && ram[i] == pid_bloco) i++;
        int tam = i - inicio;
        if (pid_bloco == 0)
            printf("[Livre: %dB]", tam);
        else
            printf("[P%d: %dB]", pid_bloco, tam);
    }
    printf("\n");
}

/* ====================================================
 *  MÓDULO 3A: Prevenção de Deadlock (Hierarquia de Recursos)
 *
 *  Regra: um processo só pode solicitar o Recurso 2 se já
 *         possuir o Recurso 1. Implementado como if/else lógico.
 * ==================================================== */

int alocar_recurso1(PCB *p) {
    if (p->recurso1 == 0) {
        p->recurso1 = 1;
        return 1; /* sucesso */
    }
    return 0; /* já possui */
}

int alocar_recurso2(PCB *p) {
    if (p->recurso1 == 0) {
        /* Violaria a hierarquia — prevenção ativa */
        printf("[DEADLOCK] PID %d tentou Recurso 2 sem ter Recurso 1. Negado.\n", p->pid);
        return 0;
    }
    if (p->recurso2 == 0) {
        p->recurso2 = 1;
        return 1;
    }
    return 0; /* já possui */
}

void liberar_recursos(PCB *p) {
    p->recurso1 = 0;
    p->recurso2 = 0;
}

/* ====================================================
 *  MÓDULO 3B: Log de Estado Final em Arquivo
 * ==================================================== */

void salvar_log(unsigned char ram[], PCB procs[], int n, int tempo_total) {
    FILE *f = fopen(LOG_FILE, "w");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível criar %s\n", LOG_FILE);
        return;
    }

    fprintf(f, "==============================================\n");
    fprintf(f, "  MINI-SO — LOG DE ESTADO FINAL\n");
    fprintf(f, "  Tempo total de simulação: %d s\n", tempo_total);
    fprintf(f, "==============================================\n\n");

    fprintf(f, "--- Processos ---\n");
    fprintf(f, "%-5s %-12s %-11s %-10s %-10s\n",
            "PID", "Burst Total", "Prioridade", "Tam(bytes)", "Estado");
    for (int i = 0; i < n; i++) {
        fprintf(f, "%-5d %-12d %-11d %-10d %-10s\n",
                procs[i].pid,
                procs[i].burst_total,
                procs[i].prioridade,
                procs[i].tamanho,
                estado_str(procs[i].estado));
    }

    fprintf(f, "\n--- Mapa Final da RAM ---\n");
    int i = 0;
    while (i < RAM_SIZE) {
        int pid_bloco = ram[i];
        int inicio    = i;
        while (i < RAM_SIZE && ram[i] == pid_bloco) i++;
        int tam = i - inicio;
        if (pid_bloco == 0)
            fprintf(f, "[Livre: %dB]", tam);
        else
            fprintf(f, "[P%d: %dB]", pid_bloco, tam);
    }
    fprintf(f, "\n");
    fclose(f);
    printf("\n[LOG] Estado final salvo em '%s'.\n", LOG_FILE);
}

/* ====================================================
 *  SAÍDA: Impressão de Status por Unidade de Tempo
 * ==================================================== */

const char *estado_str(Estado e) {
    switch (e) {
        case NOVO:       return "Novo";
        case PRONTO:     return "Pronto";
        case EXECUTANDO: return "Executando";
        case ESPERA:     return "Espera";
        case ENCERRADO:  return "Encerrado";
        default:         return "?";
    }
}

void imprimir_status(int tempo, int cpu_idx, Fila *prontos, Fila *espera,
                     unsigned char ram[], PCB procs[], int n) {
    (void) n; /* parâmetro reservado para extensões futuras */
    printf("--------------------------------------------------\n");
    printf("Tempo Atual: %02d s\n", tempo);

    /* CPU */
    if (cpu_idx >= 0)
        printf("CPU: Executando PID %d (Restam %ds)\n",
               procs[cpu_idx].pid, procs[cpu_idx].burst_restante);
    else
        printf("CPU: Ociosa\n");

    /* Fila de Prontos — exibe PID e prioridade para evidenciar ordenação */
    printf("Fila de Prontos: ");
    if (fila_vazia(prontos)) {
        printf("[vazia]");
    } else {
        Fila copia = *prontos;
        int primeiro = 1;
        while (!fila_vazia(&copia)) {
            int idx = fila_dequeue(&copia);
            if (!primeiro) printf(" -> ");
            printf("[PID %d (P%d)]", procs[idx].pid, procs[idx].prioridade);
            primeiro = 0;
        }
    }
    printf("\n");

    /* Fila de Espera (aguardando memória) */
    printf("Fila de Espera: ");
    if (fila_vazia(espera)) {
        printf("[vazia]");
    } else {
        Fila copia = *espera;
        int primeiro = 1;
        while (!fila_vazia(&copia)) {
            int idx = fila_dequeue(&copia);
            if (!primeiro) printf(" -> ");
            printf("[PID %d]", procs[idx].pid);
            primeiro = 0;
        }
    }
    printf("\n");

    /* Mapa da RAM */
    mem_imprimir(ram);
    printf("--------------------------------------------------\n\n");
}

/* ====================================================
 *  LEITURA DO ARQUIVO processos.txt
 *
 *  Formato de cada linha: PID BurstTime TamanhoRAM Prioridade
 * ==================================================== */

int carregar_processos(const char *caminho, PCB procs[]) {
    FILE *f = fopen(caminho, "r");
    if (!f) {
        fprintf(stderr, "[ERRO] Arquivo '%s' não encontrado.\n", caminho);
        return -1;
    }
    int n = 0;
    while (n < MAX_PROCESSOS &&
           fscanf(f, "%d %d %d %d",
                  &procs[n].pid,
                  &procs[n].burst_total,
                  &procs[n].tamanho,
                  &procs[n].prioridade) == 4) {
        procs[n].burst_restante = procs[n].burst_total;
        procs[n].estado         = NOVO;
        procs[n].mem_inicio     = -1;
        procs[n].recurso1       = 0;
        procs[n].recurso2       = 0;
        n++;
    }
    fclose(f);
    return n;
}

/* ====================================================
 *  MÓDULO 1: Escalonamento Round-Robin
 *
 *  Fluxo principal da simulação.
 * ==================================================== */

int main(int argc, char *argv[]) {
    /* ---- Arquivo de entrada ---- */
    const char *arquivo = (argc >= 2) ? argv[1] : "processos.txt";

    PCB procs[MAX_PROCESSOS];
    int n = carregar_processos(arquivo, procs);
    if (n <= 0) {
        fprintf(stderr, "[ERRO] Nenhum processo carregado. Encerrando.\n");
        return 1;
    }

    printf("\n==============================================\n");
    printf("  MINI-SO — Simulador de Processos e Memória\n");
    printf("  Processos carregados: %d | RAM: %dB | Quantum: %ds\n",
           n, RAM_SIZE, QUANTUM);
    printf("==============================================\n\n");

    /* ---- Estruturas principais ---- */
    unsigned char ram[RAM_SIZE];
    memset(ram, 0, sizeof(ram));

    Fila prontos, espera;
    fila_init(&prontos);
    fila_init(&espera);

    int tempo         = 0;
    int encerrados    = 0;
    int cpu_idx       = -1;   /* índice do processo na CPU          */
    int quantum_usado = 0;    /* contador de quantum atual          */

    /* ---- Admissão inicial: tenta alocar todos os processos ---- */
    for (int i = 0; i < n; i++) {
        int pos = mem_alocar(ram, procs[i].tamanho, procs[i].pid);
        if (pos >= 0) {
            procs[i].mem_inicio = pos;
            procs[i].estado     = PRONTO;
            /* Alocação de Recurso 1 (hierarquia de deadlock) */
            alocar_recurso1(&procs[i]);
            /* Recurso 2 só é alocado se Recurso 1 já foi obtido */
            alocar_recurso2(&procs[i]);
            fila_enqueue(&prontos, i, procs);   /* inserção por prioridade */
        } else {
            procs[i].estado = ESPERA;
            fila_enqueue_simples(&espera, i);   /* espera: FIFO simples    */
        }
    }

    /* ---- Loop de simulação ---- */
    while (encerrados < n) {
        /* 1. Tenta promover processos da fila de espera quando há memória */
        if (!fila_vazia(&espera)) {
            Fila temp;
            fila_init(&temp);
            while (!fila_vazia(&espera)) {
                int idx = fila_dequeue(&espera);
                int pos = mem_alocar(ram, procs[idx].tamanho, procs[idx].pid);
                if (pos >= 0) {
                    procs[idx].mem_inicio = pos;
                    procs[idx].estado     = PRONTO;
                    alocar_recurso1(&procs[idx]);
                    alocar_recurso2(&procs[idx]);
                    fila_enqueue(&prontos, idx, procs); /* inserção por prioridade */
                } else {
                    fila_enqueue_simples(&temp, idx);
                }
            }
            espera = temp;
        }

        /* 2. Seleciona próximo processo se CPU estiver ociosa */
        if (cpu_idx < 0 && !fila_vazia(&prontos)) {
            cpu_idx       = fila_dequeue(&prontos);
            procs[cpu_idx].estado = EXECUTANDO;
            quantum_usado = 0;
        }

        /* 3. Imprime estado atual */
        imprimir_status(tempo, cpu_idx, &prontos, &espera, ram, procs, n);

        /* 4. Executa 1 unidade de tempo */
        if (cpu_idx >= 0) {
            procs[cpu_idx].burst_restante--;
            quantum_usado++;

            /* Processo terminou */
            if (procs[cpu_idx].burst_restante <= 0) {
                printf("[ENCERRADO] PID %d finalizado no tempo %d.\n\n",
                       procs[cpu_idx].pid, tempo + 1);
                procs[cpu_idx].estado = ENCERRADO;
                mem_liberar(ram, procs[cpu_idx].mem_inicio,
                            procs[cpu_idx].tamanho, procs[cpu_idx].pid);
                liberar_recursos(&procs[cpu_idx]);
                cpu_idx = -1;
                quantum_usado = 0;
                encerrados++;

            /* Quantum esgotado: preempção — reinsere por prioridade */
            } else if (quantum_usado >= QUANTUM) {
                printf("[PREEMPÇÃO] PID %d retornou à fila. Restam %ds.\n\n",
                       procs[cpu_idx].pid, procs[cpu_idx].burst_restante);
                procs[cpu_idx].estado = PRONTO;
                fila_enqueue(&prontos, cpu_idx, procs); /* mantém prioridade */
                cpu_idx = -1;
                quantum_usado = 0;
            }
        }

        tempo++;

        /* Proteção contra loop infinito (tempo máximo = soma de todos os bursts * 2) */
        int burst_soma = 0;
        for (int i = 0; i < n; i++) burst_soma += procs[i].burst_total;
        if (tempo > burst_soma * 2 + 10) {
            fprintf(stderr, "[AVISO] Tempo máximo atingido. Possível deadlock ou erro.\n");
            break;
        }
    }

    /* ---- Resumo final ---- */
    printf("==============================================\n");
    printf("  Simulação concluída. Tempo total: %d s\n", tempo);
    printf("==============================================\n");
    printf("\nResumo dos Processos:\n");
    printf("%-5s %-12s %-11s %-10s %-10s\n",
           "PID", "Burst Total", "Prioridade", "Tam(bytes)", "Estado");
    for (int i = 0; i < n; i++) {
        printf("%-5d %-12d %-11d %-10d %-10s\n",
               procs[i].pid,
               procs[i].burst_total,
               procs[i].prioridade,
               procs[i].tamanho,
               estado_str(procs[i].estado));
    }

    /* ---- Módulo 3B: Salva log em arquivo ---- */
    salvar_log(ram, procs, n, tempo);

    return 0;
}