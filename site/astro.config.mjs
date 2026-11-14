// @ts-check
// trig-seedr's website: Astro Starlight, published to https://a1o1.github.io/trig-seedr/ by .github/workflows/pages.yml.
// The developer pages are copied from the repository's docs/ by scripts/sync-docs.mjs before each build.
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://a1o1.github.io',
  base: '/trig-seedr',
  trailingSlash: 'always',
  integrations: [
    starlight({
      title: 'Trig Seedr',
      description: 'Pattern seeds for Elektron machines: offline pattern algorithms (generate, groove, lock and rotate, with an audition undo). Now on the Analog Rytm MKII; the Analog Four MKII and Syntakt are planned.',
      favicon: '/favicon.svg',
      customCss: ['./src/styles/custom.css'],
      components: { SiteTitle: './src/components/SiteTitle.astro' },
      social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/a1o1/trig-seedr' }],
      editLink: { baseUrl: 'https://github.com/a1o1/trig-seedr/edit/main/site/' },
      lastUpdated: false,
      sidebar: [
        { label: 'Getting started', items: [{ autogenerate: { directory: 'getting-started' } }] },
        { label: 'Using Trig Seedr', items: [{ autogenerate: { directory: 'using' } }] },
        { label: 'Developers', items: [{ autogenerate: { directory: 'developers' } }] },
        { label: 'About', items: [{ autogenerate: { directory: 'about' } }] },
      ],
    }),
  ],
});
