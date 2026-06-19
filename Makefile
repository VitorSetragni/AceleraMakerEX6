.SILENT:

COBC = cobc
CC = gcc
COBFLAGS = -x -free -I copybooks
CFLAGS = -I/opt/ibm/clidriver/include
LDFLAGS = -L/opt/ibm/clidriver/lib -ldb2

OBJ_BANCO = build/funcoes_banco_db2.o
PROGRAMAS = bin/processar-transacoes bin/gerar-relatorio-final

.PHONY: all clean preparar-pastas

all: preparar-pastas $(PROGRAMAS)

preparar-pastas:
	mkdir -p bin build saida temporario

$(OBJ_BANCO): c/funcoes_banco_db2.c
	$(CC) $(CFLAGS) -c c/funcoes_banco_db2.c -o $(OBJ_BANCO)

bin/processar-transacoes: cobol/processar_transacoes.cob $(OBJ_BANCO)
	$(COBC) $(COBFLAGS) cobol/processar_transacoes.cob $(OBJ_BANCO) $(LDFLAGS) -o bin/processar-transacoes

bin/gerar-relatorio-final: cobol/gerar_relatorio_final.cob $(OBJ_BANCO)
	$(COBC) $(COBFLAGS) cobol/gerar_relatorio_final.cob $(OBJ_BANCO) $(LDFLAGS) -o bin/gerar-relatorio-final

clean:
	rm -rf bin/* build/* temporario/*
