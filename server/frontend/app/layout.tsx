'use client';

import 'bootstrap/dist/css/bootstrap.min.css';
import { useEffect, useState } from "react";
import Container from 'react-bootstrap/Container';

function useDarkMode() {
  const [
    isDarkMode,
    setIsDarkMode,
  ] = useState<boolean>();

  useEffect(() => {
    setIsDarkMode(window.matchMedia('(prefers-color-scheme: dark)').matches);

    const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
    const handleChange = (event: MediaQueryListEvent) => {
      setIsDarkMode(event.matches);
    };

    mediaQuery.addEventListener('change', handleChange);

    return () => {
      mediaQuery.removeEventListener('change', handleChange);
    };
  }, []);

  return isDarkMode;
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  const isDarkMode = useDarkMode();

  if (typeof isDarkMode === 'undefined') {
    return <html>
      <head><link rel='icon' type='image/png' href='/favicon.png' /></head>
      <body data-bs-theme={'dark'}></body>
    </html>;
  }

  return (
    <html lang="en">
      <head><link rel='icon' type='image/png' href='/icons/rocket-pad.png' /></head>
      <body data-bs-theme="dark">
        <Container fluid={true}>
          {children}
        </Container>
      </body>
    </html>
  );
}
