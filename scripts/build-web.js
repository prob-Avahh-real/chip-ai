import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const WWW = path.join(ROOT, 'www');
const ANDROID_PUBLIC = path.join(ROOT, 'android', 'app', 'src', 'main', 'assets', 'public');
const ANDROID_ASSETS = path.join(ROOT, 'android', 'app', 'src', 'main', 'assets');

const ASSETS = ['index.html', 'protocol.js', 'wearable-input.js'];

fs.mkdirSync(WWW, { recursive: true });
for (const f of ASSETS) {
  fs.copyFileSync(path.join(ROOT, f), path.join(WWW, f));
}

if (fs.existsSync(path.join(ROOT, 'android'))) {
  fs.mkdirSync(ANDROID_PUBLIC, { recursive: true });
  for (const f of ASSETS) {
    fs.copyFileSync(path.join(ROOT, f), path.join(ANDROID_PUBLIC, f));
  }
  fs.mkdirSync(ANDROID_ASSETS, { recursive: true });
  fs.copyFileSync(
    path.join(ROOT, 'capacitor.config.json'),
    path.join(ANDROID_ASSETS, 'capacitor.config.json')
  );
}

console.log(`✅ ChipAI companion web synced (${ASSETS.length} files)`);
