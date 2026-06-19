FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV IBM_DB_HOME=/opt/ibm/clidriver
ENV LD_LIBRARY_PATH=/opt/ibm/clidriver/lib
ENV PATH=/opt/ibm/clidriver/bin:$PATH

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       ca-certificates curl tar gzip make gcc gnucobol libaio1 libxml2 \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/ibm \
    && curl -fsSL "https://public.dhe.ibm.com/ibmdl/export/pub/software/data/db2/drivers/odbc_cli/linuxx64_odbc_cli.tar.gz" \
       | tar -xz -C /opt/ibm

WORKDIR /app
CMD ["bash"]
