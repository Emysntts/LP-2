# Revisão Primeira Prova — LPII — Programação Concorrente

## Questão 1 — Classificação para execução concorrente

### a) Classificação

**i) Normalização por fator fixo — adequado.**
Cada iteração calcula `out[i] = v[i] / fator` usando apenas `v[i]` e a constante `fator`. Não há leitura nem escrita que dependa do resultado de outra iteração: as posições de `out` são todas disjuntas. O trabalho pode ser dividido em fatias contíguas do array e distribuído entre processos ou threads sem nenhuma sincronização além de garantir que `out` já esteja alocado.

**ii) Máximo corrente (running max) — inadequado.**
`m[i]` depende diretamente de `m[i-1]`, formando uma cadeia sequencial (é um *prefix scan*). Uma paralelização ingênua que divide o array em fatias e processa cada fatia de forma independente, sem levar em conta o máximo acumulado das fatias anteriores, produz resultado incorreto.

Contra-exemplo com `v = {2,5,1,7,3,9,4,6,8,2,5,3}` dividido em duas fatias de 6 elementos, cada uma calculando seu máximo corrente isoladamente:

- Fatia 1 `{2,5,1,7,3,9}` → máximo corrente correto: `{2,5,5,7,7,9}`
- Fatia 2 `{4,6,8,2,5,3}`, processada isoladamente (sem saber que o máximo da fatia 1 foi 9) → resultado ingênuo: `{4,6,8,8,8,8}`

O resultado correto da fatia 2 deveria ser `{9,9,9,9,9,9}`, pois o máximo global até aquele ponto já era 9. A divisão ingênua erra justamente porque ignora a dependência entre as fatias.

### b) Divisão entre T threads — normalização por fator fixo

**1) Struct de argumentos**
```c
typedef struct {
    double *v;
    double *out;
    int inicio;   /* inclusive */
    int fim;      /* exclusivo */
    double fator;
} arg_t;
```

**2) Corpo da thread**
```c
void *tarefa(void *p) {
    arg_t *a = (arg_t *) p;
    for (int i = a->inicio; i < a->fim; i++)
        a->out[i] = a->v[i] / a->fator;
    return NULL;
}
```

**3) Divisão e combinação no main**
O `main` divide `n` em `T` fatias contíguas (`base = n / T`, distribuindo o resto entre as primeiras fatias), preenche um `arg_t` por thread com `inicio`/`fim` da fatia e cria as T threads com `pthread_create`. Em seguida faz `pthread_join` de todas. Como cada thread escreve em posições disjuntas de `out`, não existe etapa de combinação: o array `out` já está completo assim que todas as threads terminam.

## Questão 3 — Monitoramento de câmeras

### a) Crítica à arquitetura (1 processo por câmera + SIGUSR1)

1. **Sinais não enfileiram e não carregam informação.** Se dois sinais `SIGUSR1` chegam quase ao mesmo tempo (ou enquanto o handler anterior ainda está em execução), o segundo pode ser perdido — o kernel só garante a entrega de uma instância pendente por sinal, não uma fila. Com 30 câmeras mandando sinais de forma independente a cada ~200 ms, alertas simultâneos de câmeras diferentes podem se fundir em um único incremento do contador, subcontando eventos reais.
2. **O coletor não sabe qual câmera gerou o alerta.** `SIGUSR1` é apenas uma interrupção: o handler sabe que "algo aconteceu", mas não recebe o número da câmera, o instante exato nem qualquer dado do quadro. Isso torna impossível, por exemplo, mostrar no painel *qual* câmera disparou o alerta sem um canal adicional de dados.
3. (consequência extra, decorrente de 1) O polling do coletor a cada 50 ms sobre um contador incrementado por handler de sinal exige que esse contador seja acessado de forma segura (tipo `volatile sig_atomic_t` no mínimo); mesmo assim, o polling introduz até 50 ms de atraso para refletir um evento que o handler já registrou.

### b) Arquitetura alternativa

Proposta: **threads dentro de um único processo**, uma thread por câmera, compartilhando uma estrutura de alertas protegida por mutex (ou uma fila), em vez de processos trocando sinais.

```
[thread câmera 1] --\
[thread câmera 2] ---\
      ...              > fila de alertas (mutex + condvar) --> [thread coletor] --> painel
[thread câmera 30] --/
```

- Cada thread de câmera captura o quadro, roda a detecção de movimento e, ao detectar, monta um registro de alerta (id da câmera, timestamp, score) e insere numa fila compartilhada protegida por mutex, sinalizando uma variável de condição.
- A thread coletora fica bloqueada em `pthread_cond_wait` até haver itens na fila, retira todos os alertas pendentes e atualiza o painel — sem polling e sem perda de eventos, já que a fila acumula todos os registros em vez de apenas incrementar um contador.
- Threads compartilham memória diretamente (sem necessidade de pipes/sinais entre processos), e cada alerta carrega a identificação completa da câmera, resolvendo os dois problemas apontados no item (a).

**Novo problema introduzido:** a fila compartilhada agora é uma região de dados concorrente — múltiplas threads de câmera podem tentar inserir alertas ao mesmo tempo, e a thread coletora lê essa mesma estrutura. É necessário um mecanismo de exclusão mútua (mutex) protegendo a fila, combinado com uma variável de condição para a thread coletora não ficar em busy-waiting enquanto não há alertas — exatamente a dupla `pthread_mutex_t` + `pthread_cond_t`.
