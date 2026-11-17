// @ts-check
// trig-seedr's website: Astro Starlight, published to https://a1o1.github.io/trig-seedr/ by .github/workflows/pages.yml.
// scripts/sync-docs.mjs runs before each build: it copies pages from the repository's docs/ (Flashing, the developer
// pages, How it is tested) and the screenshots, and writes the manual's knob reference.
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://a1o1.github.io',
  base: '/trig-seedr',
  trailingSlash: 'always',
  integrations: [
    starlight({
      title: 'Trig Seedr',
      description: 'Pattern seeds for Elektron machines: a recipe per track that makes and shapes trigs on the machine, generated and applied with an undo. Now on the Analog Rytm MKII (OS 1.74) as a beta; the Analog Four MKII and Syntakt are planned.',
      favicon: '/favicon.svg',
      customCss: ['./src/styles/custom.css'],
      components: { SiteTitle: './src/components/SiteTitle.astro' },
      social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/a1o1/trig-seedr' }],
      editLink: { baseUrl: 'https://github.com/a1o1/trig-seedr/edit/main/site/' },
      lastUpdated: false,
      sidebar: [
        { label: 'Getting started', items: [{ autogenerate: { directory: 'getting-started' } }] },
        { label: 'Beta', items: [{ autogenerate: { directory: 'beta' } }] },
        { label: 'Manual', items: [{ autogenerate: { directory: 'manual' } }] },
        { label: 'Developers', items: [{ autogenerate: { directory: 'developers' } }] },
        { label: 'About', items: [{ autogenerate: { directory: 'about' } }] },
      ],
    }),
  ],
});
