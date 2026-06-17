/** @type {import('@commitlint/types').UserConfig} */
module.exports = {
  extends: ['@commitlint/config-conventional'],

  rules: {
    'type-enum': [
      2,
      'always',
      [
        'feat', // 新功能
        'fix', // 修复 bug
        'perf', // 性能优化
        'refactor', // 重构
        'test', // 测试
        'docs', // 文档
        'style', // 格式、命名、空白等
        'build', // CMake、依赖、构建脚本
        'ci', // CI/CD
        'chore', // 杂项维护
        'revert',
      ],
    ],

    'scope-enum': [
      2,
      'always',
      [
        'core',
        'app',
        'server',
        'session',
        'http',
        'rule',
        'script',
        'qjs',
        'net',
        'config',
        'platform',
        'cmake',
        'deps',
        'test',
        'docs',
        'version',
        'scripts',
        'log',
        'format',
        'e2e',
        'ci',
      ],
    ],
    'subject-case': [0],
    'subject-empty': [2, 'never'],
    'subject-full-stop': [2, 'never', '.'],
    'header-max-length': [2, 'always', 100],
  },

  prompt: {
    messages: {
      type: '选择提交类型:',
      scope: '选择影响范围:',
      subject: '填写简短描述:',
      body: '填写详细描述，可跳过:',
      breaking: '列出破坏性变更，可跳过:',
      footerPrefixsSelect: '选择关联 issue 类型，可跳过:',
      customFooterPrefixs: '输入自定义 footer，可跳过:',
      confirmCommit: '确认提交?',
    },
    types: [
      { value: 'feat', name: 'feat:     新功能' },
      { value: 'fix', name: 'fix:      修复 bug' },
      { value: 'perf', name: 'perf:     性能优化' },
      { value: 'refactor', name: 'refactor: 重构' },
      { value: 'test', name: 'test:     测试' },
      { value: 'docs', name: 'docs:     文档' },
      { value: 'style', name: 'style:    代码格式/命名调整' },
      { value: 'build', name: 'build:    构建系统/CMake/依赖' },
      { value: 'ci', name: 'ci:       CI/CD 配置' },
      { value: 'chore', name: 'chore:    杂项维护' },
    ],
    scopes: [
      'core',
      'net',
      'log',
      'http',
      'script',
      'config',
      'platform',
      'cmake',
      'scripts',
      'test',
      'version',
      'format',
      'e2e',
      'docs',
      'rule',
      'qjs',
    ],
    allowCustomScopes: true,
    allowEmptyScopes: true,
  },
}
