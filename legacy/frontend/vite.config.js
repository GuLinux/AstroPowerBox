import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  build: {
    outDir: 'build', // CRA's default build output
  },
  server: {
    proxy: {
      '/api': {
        target: `http://${process.env.ASTROPOWERBOX_HOST}`,
        changeOrigin: true,
      }
    }
}
});