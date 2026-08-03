import { defineConfig } from 'vitest/config'
import { fileURLToPath } from 'node:url'

const projectRoot = fileURLToPath(new URL('.', import.meta.url))

export default defineConfig({
  root: projectRoot,
  resolve: {
    alias: {
      '@': projectRoot,
    },
  },
  test: {
    globals: true,
    environment: 'node',
    include: ['e2e/**/*.e2e.test.ts'],
    testTimeout: 10_000,
    hookTimeout: 10_000,
    fileParallelism: false,
  },
})
