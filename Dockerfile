# Usando imagem base de Linux
FROM ubuntu:22.04

# Instalando build essentials
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    valgrind \
    siege \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Diretório de trabalho
WORKDIR /var/www

# Copy site content to /var/www
COPY var/www ./var/www

# Diretório de trabalho
WORKDIR /app

# Montaremos o src do host como volume
# COPY opcional apenas para inicializar (não obrigatório)
COPY . .

RUN make re
RUN chmod +x ./var/www/site1/cgi/cgi-bin/*.py

# Script para build (você pode usar make, cmake, etc.)
CMD ["./webserv"]