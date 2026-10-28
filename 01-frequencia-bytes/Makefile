CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all dados verifica clean

all: gera_dados freq_seq freq_par

gera_dados: gera_dados.c
	$(CC) $(CFLAGS) -o $@ $<

freq_seq: freq_seq.c
	$(CC) $(CFLAGS) -o $@ $<

freq_par: freq_par.c
	$(CC) $(CFLAGS) -o $@ $<

dados: gera_dados
	./gera_dados

verifica: all dados
	@./freq_seq > saida_seq.txt
	@for i in 1 2 3 4 5 6 7 8 9 10; do \
		inicio=$$(date +%s); \
		./freq_par > saida_par_$$i.txt; \
		fim=$$(date +%s); \
		if [ $$((fim - inicio)) -gt 60 ]; then echo "FALHOU: tempo limite excedido"; exit 1; fi; \
		diff -q saida_seq.txt saida_par_$$i.txt > /dev/null || (echo "FALHOU: saida diferente na execucao $$i"; exit 1); \
		grep -q "^total 67108864$$" saida_par_$$i.txt || (echo "FALHOU: total incorreto na execucao $$i"; exit 1); \
	done
	@rm -f saida_par_*.txt
	@echo "PASSOU"

clean:
	rm -f gera_dados freq_seq freq_par dados.bin saida_seq.txt saida_par_*.txt
