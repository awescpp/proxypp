// @ts-check
import { defineConfig } from 'astro/config'
import starlight from '@astrojs/starlight'
// import starlightVersions from 'starlight-versions'

// https://astro.build/config
export default defineConfig({
  integrations: [
    starlight({
      plugins: [
        // edit here, and version bumped automatically
        // starlightVersions({
        // versions: [{ slug: '0.1.0', label: 'v0.1.0' }],
        // current: {
        //   label: 'v0.1.0',
        // },
        // }),
      ],
      title: 'proxy++',
      defaultLocale: 'root',
      locales: {
        root: {
          label: '简体中文',
          lang: 'zh-CN',
        },
        en: { label: 'English', lang: 'en' },
      },
      social: [
        {
          icon: 'github',
          label: 'GitHub',
          href: 'https://github.com/awescpp/proxypp',
        },
      ],
      sidebar: [
        {
          label: 'Guides',
          items: [
            // Each item here is one entry in the navigation menu.
            { label: 'Example Guide', slug: 'guides/example' },
          ],
        },
        {
          label: 'Reference',
          items: [{ autogenerate: { directory: 'reference' } }],
        },
      ],
    }),
  ],
})
