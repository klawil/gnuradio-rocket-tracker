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
          destination: 'https://rocket-public.klawil.net/api/:path*',
        },
        {
          source: '/tiles/:path*',
          destination: 'https://rocket-public.klawil.net/tiles/:path*',
        },
        {
          source: '/ws',
          destination: 'https://rocket-public.klawil.net/ws',
        },
      ],
    };
  }
} else {
  nextConfig.distDir = '../build';
}

export default nextConfig;
