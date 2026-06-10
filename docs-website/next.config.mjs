const isProd = process.env.NODE_ENV === 'production';

/** @type {import('next').NextConfig} */
const nextConfig = {
  output: 'export',
  basePath: isProd ? '/100-Projects-with-Arduino' : '',
  images: {
    unoptimized: true,
  },
};

export default nextConfig;
