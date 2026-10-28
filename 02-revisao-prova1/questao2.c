#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 12   /* tam. do array */
#define P 5    /* num. de filhos */

int main(void) {
    int dados[N] =
      {2,5,1,7,3,9,4,6,8,2,5,3};
    int fd[2];

    /* TODO 1 (1,0): o pipe precisa ser criado antes do fork
     * porque fork() duplica a tabela de descritores do processo
     * atual. Se o pipe for criado antes, pai e todos os filhos
     * passam a compartilhar o MESMO par fd[0]/fd[1]. Se fosse
     * criado depois do fork, cada processo criaria seu proprio
     * pipe privado e o pai nunca receberia nada dos filhos. */
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }

    for (int i = 0; i < P; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork"); exit(1);
        }
        if (pid == 0) { /* filho i */
            /* TODO 2 (1,0): divide N entre P tratando o resto:
             * os primeiros 'resto' filhos recebem uma posicao extra */
            int base  = N / P;
            int resto = N % P;
            int inicio = i * base + (i < resto ? i : resto);
            int fim    = inicio + base + (i < resto ? 1 : 0);

            /* TODO 3 (1,0): soma os cubos da fatia e envia o parcial */
            close(fd[0]); /* filho nao le do pipe */

            long parcial = 0;
            for (int j = inicio; j < fim; j++)
                parcial += (long) dados[j] * dados[j] * dados[j];

            write(fd[1], &parcial, sizeof(parcial));
            close(fd[1]);

            _exit(0);
        }
    }

    for (int i = 0; i < P; i++)
        wait(NULL);

    /* TODO 4 (1,0): le os P parciais e agrega em 'total' */
    long total = 0;
    close(fd[1]); /* pai nao escreve no pipe */

    long parcial;
    for (int i = 0; i < P; i++) {
        read(fd[0], &parcial, sizeof(parcial));
        total += parcial;
    }
    close(fd[0]);

    printf("Soma dos cubos = %ld\n", total);
    /* a adicao e comutativa e associativa, entao somar os parciais
     * em qualquer ordem de chegada produz sempre o mesmo total */

    return 0;
}
