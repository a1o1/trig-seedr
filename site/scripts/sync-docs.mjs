// Runs before every build (npm run sync). Three jobs, so each text has one source:
// 1. copies the repository's Markdown docs into the site (Getting started, Developers, About), each with Starlight
//    front matter (the title from its first heading, which is then dropped) and repository links rewritten to GitHub;
// 2. copies the screenshots (docs/screens/*.png, made by tools/screens.sh) into src/assets/screens;
// 3. writes the manual's knob reference (manual/knobs.md) from the knob tables on the manual's page files.
// Every file it writes is git-ignored (.gitignore).
import { readFileSync, writeFileSync, mkdirSync, readdirSync, copyFileSync, rmSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const site = join(dirname(fileURLToPath(import.meta.url)), '..');
const repo = join(site, '..');
const docs = join(site, 'src/content/docs');
const gh = 'https://github.com/a1o1/trig-seedr/blob/main/';

// the Features pages this script copied from docs/v1/ until 0.7.1: gone from a checkout that built them before
rmSync(join(docs, 'features'), { recursive: true, force: true });

// 1. repository docs
const pages = [
  ['docs/flashing.md', 'getting-started/flashing.md', 'Flashing a Rytm MKII', 3],
  ['docs/flashing-st.md', 'getting-started/flashing-syntakt.md', 'Flashing a Syntakt', 4],
  ['docs/requirements.md', 'developers/requirements.md', 'Requirements', 1],
  ['docs/firmware-notes-os174.md', 'developers/firmware-notes.md', 'Firmware notes (Rytm OS 1.74)', 2],
  ['docs/firmware-notes-st142.md', 'developers/firmware-notes-syntakt.md', 'Firmware notes (Syntakt OS 1.42)', 3],
  ['docs/adding-a-device.md', 'developers/adding-a-device.md', 'Adding a device', 5],
  ['tools/README.md', 'developers/tools-and-tests.md', 'Tools and tests', 4],
  ['CONTRIBUTING.md', 'developers/contributing.md', 'Contributing', 6],
  ['docs/testing.md', 'about/testing.md', 'How it is tested', 1],
];
for (const [src, dst, title, order] of pages) {
  let md = readFileSync(join(repo, src), 'utf8');
  md = md.replace(/^# .*\n+/, '');
  // relative links to repository files -> GitHub
  md = md.replace(/\]\((?!https?:|#|mailto:)([^)]+)\)/g, (m, p) => `](${gh}${join(dirname(src), p).replace(/\\/g, '/')})`);
  md = md.replace(/`(firmware-notes-os174\.md)`/g, '[firmware notes](../firmware-notes/)');
  const fm = `---\ntitle: ${JSON.stringify(title)}\nsidebar: { order: ${order} }\neditUrl: ${JSON.stringify(gh.replace('/blob/', '/edit/') + src)}\n---\n\n`;
  const out = join(docs, dst);
  mkdirSync(dirname(out), { recursive: true });
  writeFileSync(out, fm + `> This page is generated from [\`${src}\`](${gh}${src}).\n\n` + md);
  console.log(`${src} -> ${dst}`);
}

// 2. the screenshots: docs/screens is their one home (the README shows them too)
const shots = join(site, 'src/assets/screens');
mkdirSync(shots, { recursive: true });
for (const f of readdirSync(join(repo, 'docs/screens')).filter((f) => f.endsWith('.png'))) {
  copyFileSync(join(repo, 'docs/screens', f), join(shots, f));
}
console.log('docs/screens -> src/assets/screens');

// 3. the knob reference: every row of every table headed "| Knob | Name | Values | Default | Shown |" on the
// manual's pages, in on-screen order, so each knob's values, default and condition are written once (on its page)
const manual = ['take', 'rhythm', 'place', 'dynamics', 'note', 'lock', 'trig', 'time', 'view', 'tools', 'turing'];
const head = /^\|\s*Knob\s*\|\s*Name\s*\|\s*Values\s*\|\s*Default\s*\|\s*Shown\s*\|/;
let ref = `---
title: Knob reference
description: Every knob of every page, with its values, its default and when it is shown.
sidebar: { order: 13 }
editUrl: false
---

Every knob of every page, in on-screen order. A knob that is not shown keeps its value
([Hidden knobs](../concepts/#hidden-knobs)). Ranges: the knob sets the high end, FUNC + knob the low end
([Range knobs](../concepts/#range-knobs)).

> This page is made at build time from the knob tables on each page.
`;
for (const name of manual) {
  const lines = readFileSync(join(docs, 'manual', `${name}.md`), 'utf8').split('\n');
  const title = (lines.find((l) => l.startsWith('title:')) || name).replace(/^title:\s*/, '').replace(/"/g, '');
  const rows = [];
  for (let i = 0; i < lines.length; i++) {
    if (!head.test(lines[i])) continue;
    for (i += 2; i < lines.length && lines[i].startsWith('|'); i++) {
      const cells = lines[i].split('|').slice(1, -1).map((c) => c.trim());
      rows.push(`| ${cells.slice(0, 5).join(' | ')} |`);
    }
  }
  if (!rows.length) throw new Error(`manual/${name}.md: no knob table`);
  ref += `\n## [${title}](../${name}/)\n\n| Knob | Name | Values | Default | Shown |\n|---|---|---|---|---|\n${rows.join('\n')}\n`;
}
writeFileSync(join(docs, 'manual/knobs.md'), ref);
console.log('manual/*.md knob tables -> manual/knobs.md');
