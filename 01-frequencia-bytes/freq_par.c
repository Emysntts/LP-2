#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define NPROC 10
#define NVAL 256

static long tamanho_arquivo(const char *caminho) {
    struct stat st;
    if (stat(caminho, &st) != 0) {
        perror("stat");
        exit(1);
    }
    return st.st_size;
}

/* conta os bytes do arquivo no intervalo [ini, fim) em freq */
static void conta_faixa(const char *caminho, long ini, long fim, long freq[NVAL]) {
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    if (fseek(f, ini, SEEK_SET) != 0) {
        perror("fseek");
        exit(1);
    }

    unsigned char buf[4096];
    long restante = fim - ini;

    while (restante > 0) {
        size_t pedir = restante < (long) sizeof(buf) ? (size_t) restante : sizeof(buf);
        size_t lidos = fread(buf, 1, pedir, f);
        if (lidos == 0) break;
        for (size_t i = 0; i < lidos; i++)
            freq[buf[i]]++;
        restante -= (long) lidos;
    }

    fclose(f);
}

int main(void) {
    const char *caminho = "dados.bin";
    long tamanho = tamanho_arquivo(caminho);

    long base = tamanho / NPROC;
    long resto = tamanho % NPROC;

    /* TODO 1: criar o pipe e tratar o erro */
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }

    long ini = 0;
    for (int i = 0; i < NPROC; i++) {
        long tam_filho = base + (i < resto ? 1 : 0);
        long fim = ini + tam_filho;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            /* TODO 2: no filho, fechar o extremo que ele nao usa */
            close(fd[0]);

            long freq[NVAL] = {0};
            conta_faixa(caminho, ini, fim, freq);

            /* TODO 3: escrever o vetor inteiro, fechar e _exit(0) */
            if (write(fd[1], freq, sizeof(freq)) != (ssize_t) sizeof(freq)) {
                perror("write");
                _exit(1);
            }
            close(fd[1]);
            _exit(0);
        }

        ini = fim;
    }

    /* TODO 4: no pai, fechar o extremo que ele nao usa */
    close(fd[1]);

    /* TODO 5: ler até o fim somando em global, e só então esperar os filhos */
    long global[NVAL] = {0};
    long parcial[NVAL];

    while (read(fd[0], parcial, sizeof(parcial)) == (ssize_t) sizeof(parcial)) {
        for (int v = 0; v < NVAL; v++)
            global[v] += parcial[v];
    }
    close(fd[0]);

    for (int i = 0; i < NPROC; i++)
        wait(NULL);

    long total = 0;
    for (int v = 0; v < NVAL; v++) {
        printf("%d %ld\n", v, global[v]);
        total += global[v];
    }
    printf("total %ld\n", total);

    return 0;
}
