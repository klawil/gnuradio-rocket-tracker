import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  output: 'export',
  trailingSlash: true,
  productionBrowserSourceMaps: true,
};

if (!process.env.PROD_BUILD) {
  nextConfig.rewrites = async () => {
    return {
      fallback: [
        {
          source: '/api/:path*',
          destination: 'http://127.0.0.1:8080/api/:path*',
        },
        {
          source: '/tiles/:path*',
          destination: 'http://127.0.0.1:8080/tiles/:path*',
        },
        {
          source: '/ws',
          destination: 'http://127.0.0.1:8080/ws',
        },
      ],
    };
  }
} else {
  nextConfig.distDir = '../build';
}

export default nextConfig;
