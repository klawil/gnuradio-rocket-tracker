import { useEffect, useState } from "react";

export function useTime() {
  const [ time, setTime ] = useState(0);
  useEffect(() => {
    setTime(Date.now());
    const interval = setInterval(() => {
      setTime(Date.now());
    }, 500);

    return () => clearInterval(interval);
  }, []);

  return time;
}

export function timeToDelta(now: number, event: number, max: number = 60) {
  if (event > now) {
    return '0s';
  }

  const seconds = Math.ceil((now - event) / 1000);
  const minutes = seconds / 60;
  if (minutes >= max) {
    return `>${max} m`;
  }

  if (seconds <= 120) {
    return `${seconds}s`;
  }

  return `${Math.ceil(minutes)}m`;
}
