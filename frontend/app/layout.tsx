import type { Metadata } from "next";
import 'bootstrap/dist/css/bootstrap.min.css';

export const metadata: Metadata = {
  title: "Altus Rocket Tracker",
  description: "",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <head><link rel='icon' type='image/png' href='/icons/rocket-pad.png' /></head>
      <body data-bs-theme="dark">{children}</body>
    </html>
  );
}
