#!/bin/bash

cmd="altus-tracker"
if [ "$CENTER_FREQ" != "" ]; then
  cmd+=" --center_freq $CENTER_FREQ"
fi
if [ "$SAMPLE_RATE" != "" ]; then
  cmd+=" --sample_rate $SAMPLE_RATE"
fi
if [ "$SOCKET_HOST" != "" ]; then
  socket_ip=$(getent hosts $socket | awk '{ print $1 }' | head -n 1)
  if [ "$socket_ip" != "" ]; then
    cmd+=" --socket \"$socket_ip\""
  fi
elif [ "$SOCKET_IP" != "" ]; then
  cmd+=" --socket \"$SOCKET_IP\""
fi
if [ "$SOCKET_PORT" != "" ]; then
  cmd+=" --port $SOCKET_PORT"
fi
if [ "$INPUT_FILE" != "" ]; then
  cmd+=" --file \"$INPUT_FILE\""
fi
if [ "$CHANNELS" != "" ]; then
  cmd+=" --channels $CHANNELS"
fi

echo "Command: $cmd"
eval $cmd
