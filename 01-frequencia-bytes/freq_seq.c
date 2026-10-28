#include <stdio.h>
#include <stdlib.h>

#define NVAL 256

int main(void) {
    const char *caminho = "dados.bin";
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    long freq[NVAL] = {0};
    unsigned char buf[4096];
    size_t lidos;

    while ((lidos = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < lidos; i++)
            freq[buf[i]]++;
    }

    fclose(f);

    long total = 0;
    for (int v = 0; v < NVAL; v++) {
        printf("%d %ld\n", v, freq[v]);
        total += freq[v];
    }
    printf("total %ld\n", total);

    return 0;
}
