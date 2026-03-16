import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'
import { watch } from 'fs'

function watchPublicPlugin() {
  let reloadTimeout = null
  return {
    name: 'watch-public',
    configureServer(server) {
      const publicDir = resolve(__dirname, 'public')
      watch(publicDir, { recursive: true }, (event, filename) => {
        if (filename && filename.endsWith('.js')) {
          clearTimeout(reloadTimeout)
          reloadTimeout = setTimeout(() => {
            server.ws.send({ type: 'full-reload', path: '*' })
          }, 1000)
        }
      })
    }
  }
}

export default defineConfig({
  base: '/wood_test/',
  plugins: [vue(), watchPublicPlugin()],
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    rollupOptions: {
      input: {
        main: './index.html'
      }
    }
  },
  server: {
    port: 8770,
    strictPort: true,
    open: false
  },
  publicDir: 'public'
})
