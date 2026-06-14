import process from 'node:process'
import { runCommand, buildDir } from './run-command.mjs'

if (process.platform === 'win32') {
  await runCommand('powershell', [
    '-ExecutionPolicy',
    'Bypass',
    '-File',
    'scripts/unit-test.ps1',
  ])
} else if (process.platform === 'linux') {
  await runCommand('cmake', ['--build', buildDir])
  await runCommand('ctest', ['--test-dir', buildDir, '--output-on-failure'])
} else {
  console.error(`unsupported platform ${process.platform}`)
  process.exit(-1)
}
