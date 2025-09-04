# Usando imagem base de Linux
FROM ubuntu:22.04

# Instalando build essentials
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    valgrind \
    siege \
    && rm -rf /var/lib/apt/lists/*

# Diretório de trabalho
WORKDIR /app

# Montaremos o src do host como volume
# COPY opcional apenas para inicializar (não obrigatório)
COPY src/ ./src
COPY include/ ./include
COPY tests/ ./tests
COPY var/ ./var
COPY errors/ ./errors

# Script para build (você pode usar make, cmake, etc.)
CMD ["bash"]