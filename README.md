# AceleraMakerEX6

Projeto 6 COBOL - Sistema de Contas Bancárias com **OpenCOBOL/GnuCOBOL**, **C** e **IBM DB2 Community no Docker**.

Este projeto não usa TK5, TSO ou JCL real. O fluxo foi adaptado para ambiente local:

```text
COBOL -> C -> DB2 CLI/ODBC -> DB2 no Docker
```

## Fluxo do projeto

Nesta versão, as tabelas e os dados iniciais são preparados no DB2 pelo script SQL:

```text
sql/00_preparar_tabelas_e_clientes_manuais.sql
```

Esse script cria e popula:

- `CLIENTES`
- `TRANSACOES`

Depois disso, o COBOL:

- lê `entrada/TRANSACOES.TXT`;
- consulta clientes no DB2;
- valida cliente existente, tipo da transação, valor e saldo suficiente;
- atualiza saldo em `CLIENTES`;
- grava erros em `ERROS_PROCESSAMENTO`;
- grava histórico em `HISTORICO_TRANSACOES`;
- gera relatórios e logs na pasta `saida/`.

## Estrutura

```text
AceleraMakerEX6/
├── c/
│   └── funcoes_banco_db2.c
├── cobol/
│   ├── processar_transacoes.cob
│   └── gerar_relatorio_final.cob
├── copybooks/
│   ├── registro_transacao.cpy
│   └── variaveis_banco.cpy
├── entrada/
│   └── TRANSACOES.TXT
├── scripts/
│   ├── preparar_banco_manual.sh
│   ├── executar_processamento_manual.sh
│   ├── ordenar_arquivos.sh
│   └── consultar_banco.sh
├── sql/
│   ├── 00_preparar_tabelas_e_clientes_manuais.sql
│   └── 02_consultas_validacao.sql
├── saida/
├── Dockerfile
├── docker-compose.yml
├── Makefile
└── README.md
```

## Requisitos

- Docker Desktop
- Docker Compose

O GnuCOBOL, GCC e o driver DB2 CLI são instalados dentro do container da aplicação.

## Como executar

```bash
cd ~caminho_para_o_diretorio/AceleraMakerEX6
chmod +x scripts/*.sh

docker compose down -v --remove-orphans
./scripts/preparar_banco_manual.sh

docker compose build --no-cache app
docker compose run --rm app bash -lc "make clean && ./scripts/executar_processamento_manual.sh"

./scripts/consultar_banco.sh
```

## Arquivos de saída

Após a execução, os arquivos principais ficam em `saida/`:

```text
REL_PROCESSAMENTO_TRANSACOES.TXT
REL_DETALHADO_TRANSACOES.TXT
ERROS_TRANSACOES.TXT
LOG_TRANSACOES.TXT
REL_FINAL_DB_CLIENTES.TXT
REL_FINAL_DB_ERROS.TXT
LOG_RELATORIO_FINAL.TXT
SAIDA_STATUS_TRANSACOES.TXT
```

## Layout de TRANSACOES.TXT

Tamanho do registro: 20 caracteres.

```text
CLI_ID        5 posições
TRX_ID        5 posições
TRX_TIPO      1 posição
TRX_VALOR     9 posições
```

Exemplo:

```text
0012300010C000000500
```

Tipos usados:

```text
C = crédito / depósito
D = débito / saque
```
##Evidências da execução
<img width="594" height="178" alt="clientes_db2" src="https://github.com/user-attachments/assets/e4c581bf-00af-4d74-bab9-88ce35e3bcde" />
<img width="2048" height="289" alt="erros_db2" src="https://github.com/user-attachments/assets/ba4b2b46-d2e7-4b64-8896-945a483cafe2" />
<img width="563" height="241" alt="transacoes_db2" src="https://github.com/user-attachments/assets/112571e3-bf3e-4af1-b858-f5d1d7a86a95" />
<img width="2048" height="731" alt="historico_db2" src="https://github.com/user-attachments/assets/005b19f1-66a3-460c-9eab-7c87f2c1e95e" />



