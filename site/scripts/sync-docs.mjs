// Copies the repository's screenshots (docs/screens) into src/assets/screens, and its Markdown docs into the site's Developers and About sections, so they have one source.
// Each gets Starlight front matter (the title from its first heading, which is then dropped) and repository
// links rewritten to GitHub.
import { readFileSync, writeFileSync, mkdirSync, readdirSync, copyFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const site = join(dirname(fileURLToPath(import.meta.url)), '..');
const repo = join(site, '..');
const gh = 'https://github.com/a1o1/trig-seedr/blob/main/';
const pages = [
  ['docs/requirements.md', 'developers/requirements.md', 'Requirements', 2],
  ['docs/firmware-notes-os174.md', 'developers/firmware-notes.md', 'Firmware notes (OS 1.74)', 3],
  ['tools/README.md', 'developers/tools-and-tests.md', 'Tools and tests', 4],
  ['CONTRIBUTING.md', 'developers/contributing.md', 'Contributing', 5],
];
for (const [src, dst, title, order] of pages) {
  let md = readFileSync(join(repo, src), 'utf8');
  md = md.replace(/^# .*\n+/, '');
  // relative links to repository files -> GitHub
  md = md.replace(/\]\((?!https?:|#|mailto:)([^)]+)\)/g, (m, p) => `](${gh}${join(dirname(src), p).replace(/\\/g, '/')})`);
  md = md.replace(/`(firmware-notes-os174\.md)`/g, '[firmware notes](../firmware-notes/)');
  const fm = `---\ntitle: ${JSON.stringify(title)}\nsidebar: { order: ${order} }\neditUrl: ${JSON.stringify(gh.replace('/blob/', '/edit/') + src)}\n---\n\n`;
  const out = join(site, 'src/content/docs', dst);
  mkdirSync(dirname(out), { recursive: true });
  writeFileSync(out, fm + `> This page is generated from [\`${src}\`](${gh}${src}).\n\n` + md);
  console.log(`${src} -> ${dst}`);
}

// the screenshots: docs/screens is their one home (the README shows them too)
const shots = join(site, 'src/assets/screens');
mkdirSync(shots, { recursive: true });
for (const f of readdirSync(join(repo, 'docs/screens')).filter((f) => f.endsWith('.png'))) {
  copyFileSync(join(repo, 'docs/screens', f), join(shots, f));
}
console.log('docs/screens -> src/assets/screens');
