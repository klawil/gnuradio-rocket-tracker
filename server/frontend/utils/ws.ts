import { AltusPacket } from "@/types/Altus";
import { useCallback, useEffect, useRef, useState } from "react";

function useQueue<T>(initialValue: T[] = []) {
  const queueRef = useRef<T[]>(initialValue);
  const [queue, setQueueMirror] = useState<T[]>(initialValue);

  if (queueRef.current.length !== queue.length) {
    setQueueMirror(queueRef.current);
  }

  const pop = useCallback(() => {
    if (queueRef.current.length === 0) return;

    const [
      item,
      ...rest
    ] = queueRef.current;

    queueRef.current = rest;
    setQueueMirror([ ...rest ]);

    return item;
  }, [queue]); // eslint-disable-line react-hooks/exhaustive-deps

  const add = useCallback((item: T) => {
    queueRef.current.push(item);
    setQueueMirror([ ...queueRef.current ]);
  }, []);

  return {
    add,
    pop,
  };
}

const startRetryDelay = 5; // in seconds

export function useMessaging(
  Type: 'LOCATION' | 'ROCKET',
  ID: number = 0
) {
  const ref = useRef<WebSocket>(null);
  const {
    add: addMessage,
    pop: popMessage,
  } = useQueue<AltusPacket>();
  
  const [ isConnected, setIsConnected ] = useState(false);
  const [ lastConnEvent, setLastConnEvent ] = useState(0);

  const connectToWs = useCallback(() => {
    if (ref.current) return;
    const socket = new WebSocket(`ws${window.location.protocol === 'https:' ? 's' : ''}://${window.location.host}/ws`);
    ref.current = socket;

    const controller = new AbortController();

    socket.addEventListener(
      'open',
      () => {
        setLastConnEvent(Date.now());
        setIsConnected(true);
        socket.send(JSON.stringify({
          Type,
          ID,
        }));
      },
      controller,
    );

    socket.addEventListener(
      'message',
      async (event) => {
        const payload =
          typeof event.data === 'string' ? event.data : await event.data.text();
        if (payload === 'PING') return;
        const message = JSON.parse(payload) as AltusPacket;
        addMessage(message);
      },
      controller,
    );

    socket.addEventListener(
      'error',
      (event) => {
        console.error(`error`, event);
      },
      controller,
    );

    socket.addEventListener(
      'close',
      (event) => {
        setLastConnEvent(Date.now());
        setIsConnected(false);
        ref.current = null;
        if (event.wasClean) return;
        console.error(`close`, event);
      },
      controller,
    );

    return () => {
      console.log('ABORT');
      controller.abort();
      ref.current = null;
    };
  }, [addMessage, ID, Type]);

  useEffect(() => {
    if (isConnected) {
      return;
    }

    let timeout = setTimeout(() => {});
    if (Date.now() - lastConnEvent < startRetryDelay * 1000) {
      timeout = setTimeout(connectToWs, Date.now() - lastConnEvent + (startRetryDelay * 1000));
    } else {
      connectToWs();
    }

    return () => clearTimeout(timeout);
  }, [ isConnected, lastConnEvent, connectToWs ]);

  return [
    popMessage,
    isConnected,
  ] as const;
}
