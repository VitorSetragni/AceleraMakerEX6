# AceleraMakerEX6

Projeto 6 COBOL - Sistema de Contas Bancárias com **OpenCOBOL/GnuCOBOL**, **C** e **IBM DB2 Community no Docker**.

Este projeto não usa TK5, TSO ou JCL real. O fluxo foi adaptado para ambiente local:

```text
COBOL -> CALL em C -> DB2 CLI/ODBC -> DB2 no Docker
```

## Fluxo do projeto

Nesta versão, as tabelas e os dados iniciais são preparados manualmente no DB2 pelo script SQL:

```text
sql/00_preparar_tabelas_e_clientes_manuais.sql
```

Esse script cria e popula:

- `CLIENTES`
- `TRANSACOES`
- `ERROS_PROCESSAMENTO`
- `HISTORICO_TRANSACOES`

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
cd ~/Downloads/AceleraMakerEX6
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

## Observação sobre o JCL

O enunciado original cita JCL por considerar ambiente mainframe. Nesta versão local, os scripts `.sh` cumprem esse papel operacional:

```text
JCL / SORT        -> scripts/ordenar_arquivos.sh
EXEC PGM          -> scripts/executar_processamento_manual.sh
Preparação DB2    -> scripts/preparar_banco_manual.sh
Consulta final    -> scripts/consultar_banco.sh
```

## Como explicar na entrega

O COBOL ficou responsável pela leitura do arquivo, validações, regras de negócio, contadores e relatórios. A linguagem C foi usada apenas como ponte para conectar ao DB2 e executar comandos SQL com o driver CLI/ODBC da IBM.
