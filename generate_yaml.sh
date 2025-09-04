#!/bin/bash

CONFIG_FILE="./config/one-server.conf"
COMPOSE_FILE="./docker-compose.yaml"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Arquivo de configuração de servidores não encontrado!"
    exit 1
fi

# Começo do docker-compose.yaml
cat > $COMPOSE_FILE <<EOL
version: '3.8'
services:
  webserver:
    build: .
    volumes:
      - .:/app
    ports:
EOL

# Extrair todas as portas da diretiva 'listen' (uma por server)
grep -E '^\s*listen\s+[0-9]+;' "$CONFIG_FILE" | while read -r line; do
    port=$(echo "$line" | sed -E 's/[^0-9]*([0-9]+);/\1/')
    echo "      - \"$port:$port\"" >> $COMPOSE_FILE
done

# Comando opcional para compilar e rodar o webserver (você pode comentar se quiser rodar manual)
echo "    command: bash" >> $COMPOSE_FILE

# Mantém o container aberto para Valgrind ou testes manuais
echo "    stdin_open: true" >> $COMPOSE_FILE
echo "    tty: true" >> $COMPOSE_FILE

echo "docker-compose.yaml gerado com sucesso!"
