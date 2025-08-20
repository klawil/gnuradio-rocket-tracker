#!/bin/bash

center="${CENTER_FREQ:-434800000}"
sample="${SAMPLE_RATE:-2500000}"
file="${INPUT_FILE}"
socket="${SOCKET_HOST:-127.0.0.1}"
port="${SOCKET_PORT:-8765}"
channels="${CHANNELS:-5}"

socket_ip=$(getent hosts $socket | awk '{ print $1 }')
echo "Socket IP: $socket_ip"

if [ "$file" == "" ]; then
  altus-tracker --center_freq $center --sample_rate $sample --socket $socket_ip --port $port --channels $channels
else
  altus-tracker --center_freq $center --sample_rate $sample --file "$file" --socket $socket_ip --port $port --channels $channels
fi
