#include <stdio.h>
#include <stdlib.h>

#define TAMANHO (64L * 1024 * 1024)
#define SEMENTE 42

int main(void) {
    FILE *f = fopen("dados.bin", "wb");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    srand(SEMENTE);

    unsigned char buf[65536];
    long restante = TAMANHO;

    while (restante > 0) {
        size_t bloco = restante < (long) sizeof(buf) ? (size_t) restante : sizeof(buf);
        for (size_t i = 0; i < bloco; i++)
            buf[i] = (unsigned char) (rand() % 256);
        fwrite(buf, 1, bloco, f);
        restante -= (long) bloco;
    }

    fclose(f);
    return 0;
}
