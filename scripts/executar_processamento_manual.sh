#!/usr/bin/env bash
set -euo pipefail

mkdir -p saida temporario bin build
rm -f saida/*.TXT saida/*.txt

printf "Ordenando transacoes...\n"
./scripts/ordenar_arquivos.sh

printf "Compilando COBOL + C...\n"
make all

printf "Processando transacoes...\n"
./bin/processar-transacoes

printf "Gerando relatorio final...\n"
./bin/gerar-relatorio-final

printf "Execucao concluida. Arquivos gerados em saida/.\n"
