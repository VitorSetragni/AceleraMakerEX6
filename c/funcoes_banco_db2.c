/*
 * Ponte COBOL -> C -> DB2
 * Projeto 6 - Sistema de Contas Bancarias
 *
 * Este arquivo foi escrito para ser chamado pelo GnuCOBOL/OpenCOBOL.
 * O COBOL envia campos fixos e o C executa SQL no DB2 usando CLI/ODBC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlcli1.h>

#define TAM_MSG 200
#define TAM_DESC 100
#define TAM_CAMINHO 100

#ifndef SQL_NO_DATA_FOUND
#define SQL_NO_DATA_FOUND SQL_NO_DATA
#endif

static SQLHENV ambiente = SQL_NULL_HENV;
static SQLHDBC conexao = SQL_NULL_HDBC;
static int conectado = 0;

static void preencher_mensagem(char *destino, const char *texto) {
    size_t n;
    if (destino == NULL) return;
    memset(destino, ' ', TAM_MSG);
    if (texto == NULL) return;
    n = strlen(texto);
    if (n > TAM_MSG) n = TAM_MSG;
    memcpy(destino, texto, n);
}

static void copiar_trim(const char *origem, int tamanho, char *destino, int tamanho_destino) {
    int inicio = 0;
    int fim = tamanho - 1;
    int n;

    while (inicio < tamanho && isspace((unsigned char)origem[inicio])) inicio++;
    while (fim >= inicio && isspace((unsigned char)origem[fim])) fim--;

    n = fim - inicio + 1;
    if (n < 0) n = 0;
    if (n >= tamanho_destino) n = tamanho_destino - 1;

    if (n > 0) memcpy(destino, origem + inicio, n);
    destino[n] = '\0';
}

static void escapar_sql(const char *origem, char *destino, int tamanho_destino) {
    int i = 0;
    int j = 0;
    if (tamanho_destino <= 0) return;
    if (origem == NULL) {
        destino[0] = '\0';
        return;
    }

    while (origem[i] != '\0' && j < tamanho_destino - 1) {
        if (origem[i] == '\'' && j < tamanho_destino - 2) {
            destino[j++] = '\'';
            destino[j++] = '\'';
        } else {
            destino[j++] = origem[i];
        }
        i++;
    }
    destino[j] = '\0';
}

static int numero_de_campo(const char *campo, int tamanho) {
    char texto[32];
    int n = tamanho;
    if (n >= (int)sizeof(texto)) n = (int)sizeof(texto) - 1;
    memcpy(texto, campo, n);
    texto[n] = '\0';
    return atoi(texto);
}

static void formatar_9(int valor, char *destino) {
    char texto[16];
    if (valor < 0) valor = 0;
    snprintf(texto, sizeof(texto), "%09d", valor);
    memcpy(destino, texto, 9);
}

static void obter_diagnostico(SQLSMALLINT tipo_handle, SQLHANDLE handle, char *mensagem) {
    SQLCHAR estado[6];
    SQLINTEGER erro_nativo = 0;
    SQLCHAR texto[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT tamanho_texto = 0;
    SQLRETURN rc;
    char buffer[TAM_MSG + 100];

    memset(estado, 0, sizeof(estado));
    memset(texto, 0, sizeof(texto));

    rc = SQLGetDiagRec(tipo_handle, handle, 1, estado, &erro_nativo,
                       texto, sizeof(texto), &tamanho_texto);

    if (SQL_SUCCEEDED(rc)) {
        snprintf(buffer, sizeof(buffer), "SQLSTATE=%s SQLCODE=%d MSG=%s",
                 estado, (int)erro_nativo, texto);
        preencher_mensagem(mensagem, buffer);
    } else {
        preencher_mensagem(mensagem, "Erro DB2 sem detalhe disponivel");
    }
}

static int verificar_conexao(char *mensagem) {
    if (!conectado || conexao == SQL_NULL_HDBC) {
        preencher_mensagem(mensagem, "Conexao com DB2 nao esta aberta");
        return -900;
    }
    return 0;
}

static int finalizar_stmt(SQLHSTMT stmt, int retorno) {
    if (stmt != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return retorno;
}

static int executar_sql_direto(const char *sql, char *mensagem) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN rc;

    if (verificar_conexao(mensagem) != 0) return -900;

    rc = SQLAllocHandle(SQL_HANDLE_STMT, conexao, &stmt);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -901;
    }

    rc = SQLExecDirect(stmt, (SQLCHAR *)sql, SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -902);
    }

    return finalizar_stmt(stmt, 0);
}

int BANCO_CONECTAR(char *mensagem) {
    SQLRETURN rc;
    char conexao_texto[512];
    const char *host = getenv("DB2_HOST");
    const char *porta = getenv("DB2_PORT");
    const char *banco = getenv("DB2_DATABASE");
    const char *usuario = getenv("DB2_USER");
    const char *senha = getenv("DB2_PASSWORD");

    if (conectado) {
        preencher_mensagem(mensagem, "Conexao DB2 ja estava aberta");
        return 0;
    }

    if (!host) host = "localhost";
    if (!porta) porta = "50000";
    if (!banco) banco = "BANCO6";
    if (!usuario) usuario = "db2inst1";
    if (!senha) senha = "SenhaDb2@123";

    rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &ambiente);
    if (!SQL_SUCCEEDED(rc)) {
        preencher_mensagem(mensagem, "Falha ao alocar ambiente DB2 CLI");
        return -1;
    }

    rc = SQLSetEnvAttr(ambiente, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_ENV, ambiente, mensagem);
        return -2;
    }

    rc = SQLAllocHandle(SQL_HANDLE_DBC, ambiente, &conexao);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_ENV, ambiente, mensagem);
        return -3;
    }

    snprintf(conexao_texto, sizeof(conexao_texto),
             "DATABASE=%s;HOSTNAME=%s;PORT=%s;PROTOCOL=TCPIP;UID=%s;PWD=%s;",
             banco, host, porta, usuario, senha);

    rc = SQLDriverConnect(conexao, NULL, (SQLCHAR *)conexao_texto, SQL_NTS,
                          NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -4;
    }

    rc = SQLSetConnectAttr(conexao, SQL_ATTR_AUTOCOMMIT,
                           (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -5;
    }

    conectado = 1;
    preencher_mensagem(mensagem, "Conexao com DB2 realizada com sucesso");
    return 0;
}

int BANCO_DESCONECTAR(char *mensagem) {
    if (conectado && conexao != SQL_NULL_HDBC) {
        SQLDisconnect(conexao);
        SQLFreeHandle(SQL_HANDLE_DBC, conexao);
    }
    if (ambiente != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, ambiente);

    conectado = 0;
    conexao = SQL_NULL_HDBC;
    ambiente = SQL_NULL_HENV;
    preencher_mensagem(mensagem, "Conexao DB2 encerrada");
    return 0;
}

int BANCO_COMMIT(char *mensagem) {
    SQLRETURN rc;
    if (verificar_conexao(mensagem) != 0) return -900;
    rc = SQLEndTran(SQL_HANDLE_DBC, conexao, SQL_COMMIT);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -10;
    }
    preencher_mensagem(mensagem, "COMMIT executado com sucesso");
    return 0;
}

int BANCO_ROLLBACK(char *mensagem) {
    SQLRETURN rc;
    if (verificar_conexao(mensagem) != 0) return -900;
    rc = SQLEndTran(SQL_HANDLE_DBC, conexao, SQL_ROLLBACK);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -11;
    }
    preencher_mensagem(mensagem, "ROLLBACK executado com sucesso");
    return 0;
}

int BANCO_SALVAR_CLIENTE(char *cli_id, char *cli_nome, char *cli_saldo, char *mensagem) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    SQLLEN linhas = 0;
    SQLINTEGER id = numero_de_campo(cli_id, 5);
    SQLINTEGER saldo = numero_de_campo(cli_saldo, 9);
    char nome[31];
    char nome_sql[80];
    char sql[512];

    if (verificar_conexao(mensagem) != 0) return -900;

    copiar_trim(cli_nome, 30, nome, sizeof(nome));
    if (strlen(nome) == 0) {
        preencher_mensagem(mensagem, "Nome do cliente e obrigatorio");
        return 11;
    }
    escapar_sql(nome, nome_sql, sizeof(nome_sql));

    rc = SQLAllocHandle(SQL_HANDLE_STMT, conexao, &stmt);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -20;
    }

    snprintf(sql, sizeof(sql),
             "UPDATE CLIENTES SET CLI_NOME = '%s', CLI_SALDO = %d, DT_ATUALIZACAO = CURRENT DATE WHERE CLI_ID = %d",
             nome_sql, (int)saldo, (int)id);

    rc = SQLExecDirect(stmt, (SQLCHAR *)sql, SQL_NTS);

    /*
     * No DB2 CLI, um UPDATE que nao encontra linhas pode retornar
     * SQL_NO_DATA / SQLSTATE 02000. Para a carga de clientes isso
     * NAO e erro: significa que o cliente ainda nao existe e deve
     * ser inserido logo abaixo.
     */
    if (rc == SQL_NO_DATA || rc == SQL_NO_DATA_FOUND) {
        linhas = 0;
    } else if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -22);
    } else {
        SQLRowCount(stmt, &linhas);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    stmt = SQL_NULL_HSTMT;

    if (linhas > 0) {
        preencher_mensagem(mensagem, "Cliente atualizado no DB2");
        return 2;
    }

    snprintf(sql, sizeof(sql),
             "INSERT INTO CLIENTES (CLI_ID, CLI_NOME, CLI_SALDO, DT_ATUALIZACAO) VALUES (%d, '%s', %d, CURRENT DATE)",
             (int)id, nome_sql, (int)saldo);

    rc = executar_sql_direto(sql, mensagem);
    if (rc != 0) return -25;

    preencher_mensagem(mensagem, "Cliente inserido no DB2");
    return 1;
}

int BANCO_OBTER_SALDO(char *cli_id, char *saldo_saida, char *mensagem) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    SQLINTEGER id = numero_de_campo(cli_id, 5);
    SQLINTEGER saldo = 0;
    SQLLEN ind_saldo = 0;
    char sql[256];

    if (verificar_conexao(mensagem) != 0) return -900;

    rc = SQLAllocHandle(SQL_HANDLE_STMT, conexao, &stmt);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -30;
    }

    snprintf(sql, sizeof(sql),
             "SELECT CLI_SALDO FROM CLIENTES WHERE CLI_ID = %d",
             (int)id);

    rc = SQLExecDirect(stmt, (SQLCHAR *)sql, SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -32);
    }

    SQLBindCol(stmt, 1, SQL_C_LONG, &saldo, 0, &ind_saldo);

    rc = SQLFetch(stmt);
    if (rc == SQL_NO_DATA_FOUND || rc == SQL_NO_DATA) {
        memset(saldo_saida, '0', 9);
        preencher_mensagem(mensagem, "Cliente inexistente");
        return finalizar_stmt(stmt, 100);
    }

    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -33);
    }

    formatar_9((int)saldo, saldo_saida);
    preencher_mensagem(mensagem, "Saldo encontrado");
    return finalizar_stmt(stmt, 0);
}

int BANCO_INSERIR_TRANSACAO(char *trx_id, char *cli_id, char *trx_tipo, char *trx_valor, char *mensagem) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    SQLINTEGER id_transacao = numero_de_campo(trx_id, 5);
    SQLINTEGER id_cliente = numero_de_campo(cli_id, 5);
    SQLINTEGER valor = numero_de_campo(trx_valor, 9);
    SQLINTEGER quantidade = 0;
    SQLLEN ind_quantidade = 0;
    char tipo = trx_tipo[0];
    char sql[512];

    if (verificar_conexao(mensagem) != 0) return -900;

    if (tipo == '\'') tipo = ' ';

    /*
     * Na versao manual, as transacoes podem ja estar cadastradas no DB2
     * pelo script SQL. Se o TRX_ID ja existir, nao tratamos como erro:
     * apenas seguimos com a atualizacao de saldo e historico.
     */
    rc = SQLAllocHandle(SQL_HANDLE_STMT, conexao, &stmt);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -40;
    }

    snprintf(sql, sizeof(sql),
             "SELECT COUNT(*) FROM TRANSACOES WHERE TRX_ID = %d",
             (int)id_transacao);

    rc = SQLExecDirect(stmt, (SQLCHAR *)sql, SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -41);
    }

    SQLBindCol(stmt, 1, SQL_C_LONG, &quantidade, 0, &ind_quantidade);
    rc = SQLFetch(stmt);
    if (!SQL_SUCCEEDED(rc)) {
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -42);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    stmt = SQL_NULL_HSTMT;

    if (quantidade > 0) {
        preencher_mensagem(mensagem, "Transacao ja cadastrada no DB2");
        return 0;
    }

    snprintf(sql, sizeof(sql),
             "INSERT INTO TRANSACOES (TRX_ID, CLI_ID, TRX_TIPO, TRX_VALOR, DT_PROCESSAMENTO) VALUES (%d, %d, '%c', %d, CURRENT DATE)",
             (int)id_transacao, (int)id_cliente, tipo, (int)valor);

    rc = executar_sql_direto(sql, mensagem);
    if (rc != 0) return -43;

    preencher_mensagem(mensagem, "Transacao inserida no DB2");
    return 0;
}

int BANCO_ATUALIZAR_SALDO(char *cli_id, char *saldo_novo, char *mensagem) {
    SQLRETURN rc;
    SQLINTEGER id_cliente = numero_de_campo(cli_id, 5);
    SQLINTEGER saldo = numero_de_campo(saldo_novo, 9);
    char sql[512];

    if (verificar_conexao(mensagem) != 0) return -900;

    snprintf(sql, sizeof(sql),
             "UPDATE CLIENTES SET CLI_SALDO = %d, DT_ATUALIZACAO = CURRENT DATE WHERE CLI_ID = %d",
             (int)saldo, (int)id_cliente);

    rc = executar_sql_direto(sql, mensagem);
    if (rc != 0) return -52;

    preencher_mensagem(mensagem, "Saldo atualizado no DB2");
    return 0;
}

int BANCO_REGISTRAR_ERRO(char *cli_id, char *descricao, char *mensagem) {
    SQLRETURN rc;
    SQLINTEGER id_cliente = numero_de_campo(cli_id, 5);
    char desc[101];
    char desc_sql[220];
    char sql[700];

    if (verificar_conexao(mensagem) != 0) return -900;

    copiar_trim(descricao, TAM_DESC, desc, sizeof(desc));
    escapar_sql(desc, desc_sql, sizeof(desc_sql));

    snprintf(sql, sizeof(sql),
             "INSERT INTO ERROS_PROCESSAMENTO (CLI_ID, DESCRICAO_ERRO, DT_OCORRENCIA) VALUES (%d, '%s', CURRENT TIMESTAMP)",
             (int)id_cliente, desc_sql);

    rc = executar_sql_direto(sql, mensagem);
    if (rc != 0) return -62;

    preencher_mensagem(mensagem, "Erro registrado no DB2");
    return 0;
}

int BANCO_REGISTRAR_HISTORICO(char *trx_id, char *cli_id, char *trx_tipo,
                              char *trx_valor, char *saldo_anterior,
                              char *saldo_novo, char *status, char *descricao,
                              char *mensagem) {
    SQLRETURN rc;
    SQLINTEGER id_transacao = numero_de_campo(trx_id, 5);
    SQLINTEGER id_cliente = numero_de_campo(cli_id, 5);
    SQLINTEGER valor = numero_de_campo(trx_valor, 9);
    SQLINTEGER saldo_ant = numero_de_campo(saldo_anterior, 9);
    SQLINTEGER saldo_nov = numero_de_campo(saldo_novo, 9);
    char tipo = trx_tipo[0];
    char st[31];
    char desc[101];
    char st_sql[80];
    char desc_sql[220];
    char sql[1200];

    if (verificar_conexao(mensagem) != 0) return -900;

    if (tipo == '\'') tipo = ' ';
    copiar_trim(status, 30, st, sizeof(st));
    copiar_trim(descricao, 100, desc, sizeof(desc));
    escapar_sql(st, st_sql, sizeof(st_sql));
    escapar_sql(desc, desc_sql, sizeof(desc_sql));

    snprintf(sql, sizeof(sql),
             "INSERT INTO HISTORICO_TRANSACOES (TRX_ID, CLI_ID, TRX_TIPO, TRX_VALOR, SALDO_ANTERIOR, SALDO_NOVO, STATUS_PROCESSAMENTO, DESCRICAO, DT_HISTORICO) VALUES (%d, %d, '%c', %d, %d, %d, '%s', '%s', CURRENT TIMESTAMP)",
             (int)id_transacao, (int)id_cliente, tipo, (int)valor,
             (int)saldo_ant, (int)saldo_nov, st_sql, desc_sql);

    rc = executar_sql_direto(sql, mensagem);
    if (rc != 0) return -72;

    preencher_mensagem(mensagem, "Historico registrado no DB2");
    return 0;
}

int BANCO_GERAR_RELATORIO_CLIENTES(char *caminho_arquivo, char *mensagem) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    SQLINTEGER id = 0;
    SQLINTEGER saldo = 0;
    SQLCHAR nome[64];
    SQLLEN ind_id = 0, ind_nome = 0, ind_saldo = 0;
    char caminho[TAM_CAMINHO + 1];
    FILE *arq;

    if (verificar_conexao(mensagem) != 0) return -900;
    copiar_trim(caminho_arquivo, TAM_CAMINHO, caminho, sizeof(caminho));

    arq = fopen(caminho, "w");
    if (!arq) {
        preencher_mensagem(mensagem, "Nao foi possivel abrir arquivo de relatorio final");
        return -80;
    }

    fprintf(arq, "RELATORIO FINAL - CLIENTES NO DB2\n");
    fprintf(arq, "================================\n");
    fprintf(arq, "%-8s %-30s %12s\n", "CLI_ID", "NOME", "SALDO");

    rc = SQLAllocHandle(SQL_HANDLE_STMT, conexao, &stmt);
    if (!SQL_SUCCEEDED(rc)) {
        fclose(arq);
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -81;
    }

    rc = SQLExecDirect(stmt,
        (SQLCHAR *)"SELECT CLI_ID, CLI_NOME, CLI_SALDO FROM CLIENTES ORDER BY CLI_ID",
        SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        fclose(arq);
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -82);
    }

    SQLBindCol(stmt, 1, SQL_C_LONG, &id, 0, &ind_id);
    SQLBindCol(stmt, 2, SQL_C_CHAR, nome, sizeof(nome), &ind_nome);
    SQLBindCol(stmt, 3, SQL_C_LONG, &saldo, 0, &ind_saldo);

    while ((rc = SQLFetch(stmt)) != SQL_NO_DATA && rc != SQL_NO_DATA_FOUND) {
        if (!SQL_SUCCEEDED(rc)) {
            fclose(arq);
            obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
            return finalizar_stmt(stmt, -83);
        }
        fprintf(arq, "%05d    %-30s %12d\n", (int)id, nome, (int)saldo);
    }

    fclose(arq);
    preencher_mensagem(mensagem, "Relatorio final de clientes gerado");
    return finalizar_stmt(stmt, 0);
}

int BANCO_GERAR_RELATORIO_ERROS(char *caminho_arquivo, char *mensagem) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    SQLINTEGER id_erro = 0;
    SQLINTEGER cli_id = 0;
    SQLCHAR desc[160];
    SQLCHAR data[40];
    SQLLEN ind1 = 0, ind2 = 0, ind3 = 0, ind4 = 0;
    char caminho[TAM_CAMINHO + 1];
    FILE *arq;

    if (verificar_conexao(mensagem) != 0) return -900;
    copiar_trim(caminho_arquivo, TAM_CAMINHO, caminho, sizeof(caminho));

    arq = fopen(caminho, "w");
    if (!arq) {
        preencher_mensagem(mensagem, "Nao foi possivel abrir arquivo de relatorio de erros DB2");
        return -90;
    }

    fprintf(arq, "RELATORIO FINAL - ERROS REGISTRADOS NO DB2\n");
    fprintf(arq, "==========================================\n");
    fprintf(arq, "%-8s %-8s %-35s %-25s\n", "ID_ERRO", "CLI_ID", "ERRO", "DATA");

    rc = SQLAllocHandle(SQL_HANDLE_STMT, conexao, &stmt);
    if (!SQL_SUCCEEDED(rc)) {
        fclose(arq);
        obter_diagnostico(SQL_HANDLE_DBC, conexao, mensagem);
        return -91;
    }

    rc = SQLExecDirect(stmt,
        (SQLCHAR *)"SELECT ID_ERRO, COALESCE(CLI_ID, 0), DESCRICAO_ERRO, VARCHAR_FORMAT(DT_OCORRENCIA, 'YYYY-MM-DD HH24:MI:SS') FROM ERROS_PROCESSAMENTO ORDER BY ID_ERRO",
        SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        fclose(arq);
        obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
        return finalizar_stmt(stmt, -92);
    }

    SQLBindCol(stmt, 1, SQL_C_LONG, &id_erro, 0, &ind1);
    SQLBindCol(stmt, 2, SQL_C_LONG, &cli_id, 0, &ind2);
    SQLBindCol(stmt, 3, SQL_C_CHAR, desc, sizeof(desc), &ind3);
    SQLBindCol(stmt, 4, SQL_C_CHAR, data, sizeof(data), &ind4);

    while ((rc = SQLFetch(stmt)) != SQL_NO_DATA && rc != SQL_NO_DATA_FOUND) {
        if (!SQL_SUCCEEDED(rc)) {
            fclose(arq);
            obter_diagnostico(SQL_HANDLE_STMT, stmt, mensagem);
            return finalizar_stmt(stmt, -93);
        }
        fprintf(arq, "%08d %05d    %-35s %-25s\n", (int)id_erro, (int)cli_id, desc, data);
    }

    fclose(arq);
    preencher_mensagem(mensagem, "Relatorio final de erros gerado");
    return finalizar_stmt(stmt, 0);
}
