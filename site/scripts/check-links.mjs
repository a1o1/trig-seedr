// Checks the built site (dist/) for broken internal links: every href and src on every page that points inside
// the site must reach a built file, and a #fragment must name an id on the target page.
// Run after a build: npm run build && npm run check-links. Exits 1 and lists each broken link.
import { readFileSync, readdirSync, statSync, existsSync } from 'node:fs';
import { join, dirname, posix } from 'node:path';
import { fileURLToPath } from 'node:url';

const dist = join(dirname(fileURLToPath(import.meta.url)), '..', 'dist');
const base = '/trig-seedr/';
const pages = [];
const walk = (d) => {
  for (const f of readdirSync(d)) {
    const p = join(d, f);
    if (statSync(p).isDirectory()) walk(p);
    else if (f.endsWith('.html')) pages.push(p);
  }
};
walk(dist);

const ids = new Map();
const idsOf = (file) => {
  if (!ids.has(file)) {
    const html = readFileSync(file, 'utf8');
    ids.set(file, new Set([...html.matchAll(/\sid="([^"]+)"/g)].map((m) => m[1])));
  }
  return ids.get(file);
};
// a site path (/trig-seedr/a/b/) -> the built file
const fileOf = (path) => {
  const rel = decodeURIComponent(path.slice(base.length));
  for (const c of [rel, join(rel, 'index.html'), rel + '.html']) {
    const f = join(dist, c);
    if (existsSync(f) && statSync(f).isFile()) return f;
  }
  return null;
};

let links = 0;
const broken = [];
for (const page of pages) {
  const html = readFileSync(page, 'utf8');
  const here = '/' + posix.join(base.slice(1), page.slice(dist.length + 1).split('\\').join('/'));
  for (const m of html.matchAll(/\s(?:href|src)="([^"]+)"/g)) {
    let url = m[1].replace(/&amp;/g, '&');
    if (/^(https?:|mailto:|data:|javascript:)/.test(url) || url.startsWith('//')) continue;
    let [path, frag] = url.split('#');
    path = path.split('?')[0];
    let file = page;
    if (path) {
      const abs = path.startsWith('/') ? path : posix.join(posix.dirname(here), path) + (path.endsWith('/') ? '/' : '');
      if (!abs.startsWith(base)) { broken.push(`${here}: ${url} (outside ${base})`); continue; }
      file = fileOf(abs);
      if (!file) { broken.push(`${here}: ${url} (no such page or file)`); continue; }
    }
    links++;
    if (frag && file.endsWith('.html') && !idsOf(file).has(decodeURIComponent(frag)))
      broken.push(`${here}: ${url} (no id "${frag}")`);
  }
}
console.log(`${pages.length} pages, ${links} internal links checked, ${broken.length} broken`);
for (const b of broken) console.log('  ' + b);
process.exit(broken.length ? 1 : 0);
