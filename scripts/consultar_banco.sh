#!/usr/bin/env bash
set -euo pipefail

docker compose cp sql/02_consultas_validacao.sql db2:/tmp/02_consultas_validacao.sql >/dev/null
docker compose exec -T db2 su - db2inst1 -c "db2 -tvf /tmp/02_consultas_validacao.sql"
