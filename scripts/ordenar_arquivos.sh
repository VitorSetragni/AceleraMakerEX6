#!/usr/bin/env bash
set -euo pipefail

mkdir -p entrada temporario saida
sort -k1.1,1.5 -k1.6,1.10 entrada/TRANSACOES.TXT > entrada/TRANSACOES_ORD.TXT
