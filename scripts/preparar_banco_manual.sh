#!/usr/bin/env bash
set -euo pipefail

mkdir -p temporario

printf "Iniciando DB2...\n"
docker compose up -d db2 >/dev/null

for tentativa in $(seq 1 60); do
    if docker compose exec -T db2 su - db2inst1 -c "db2 connect to BANCO6" >/dev/null 2>&1; then
        printf "DB2 pronto.\n"
        break
    fi
    sleep 10
    if [ "$tentativa" = "60" ]; then
        printf "Erro: o DB2 nao ficou pronto. Veja: docker compose logs db2\n" >&2
        exit 1
    fi
done

printf "Criando tabelas e inserindo dados iniciais...\n"
docker compose cp sql/00_preparar_tabelas_e_clientes_manuais.sql db2:/tmp/00_preparar_tabelas_e_clientes_manuais.sql >/dev/null

if ! docker compose exec -T db2 su - db2inst1 -c "db2 -tvf /tmp/00_preparar_tabelas_e_clientes_manuais.sql" > temporario/preparacao_banco.log 2>&1; then
    printf "Erro ao preparar o banco. Ultimas linhas do log:\n" >&2
    tail -40 temporario/preparacao_banco.log >&2
    exit 1
fi

printf "Validacao do banco:\n"
docker compose exec -T db2 su - db2inst1 -c "db2 connect to BANCO6 >/dev/null; db2 -x \"SELECT 'CLIENTES=' || CHAR(COUNT(*)) FROM CLIENTES\"; db2 -x \"SELECT 'TRANSACOES=' || CHAR(COUNT(*)) FROM TRANSACOES\"; db2 connect reset >/dev/null"

printf "Banco preparado com sucesso.\n"
