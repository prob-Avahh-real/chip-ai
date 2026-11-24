import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  CAI_CMD,
  CAI_EVT,
  CAI_FLAG,
  crc8,
  encodeFrame,
  decodeFrame,
  buildSetThresholdFrame,
  parseStatusPayload,
  parseResultPayload,
} from '../protocol.js';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

test('ChipAI BLE GATT frame codec round-trips commands, thresholds, status, and results', () => {
  const ping = encodeFrame(CAI_CMD.PING);
  assert.equal(ping[0], 0x43);
  assert.equal(ping[1], 0x41);
  assert.equal(ping.length, 6);
  const decodedPing = decodeFrame(ping);
  assert.equal(decodedPing.cmd, CAI_CMD.PING);
  assert.equal(decodedPing.payload.length, 0);

  // Tampered CRC must fail decoding
  const corrupted = Uint8Array.from(ping);
  corrupted[5] ^= 0xff;
  assert.equal(decodeFrame(corrupted), null);

  const thFrame = buildSetThresholdFrame(0.75);
  const decodedTh = decodeFrame(thFrame);
  assert.equal(decodedTh.cmd, CAI_CMD.SET_THRESHOLD);
  assert.equal(new DataView(decodedTh.payload.buffer).getFloat32(0, true), 0.75);

  const statusBuf = new ArrayBuffer(15);
  const sdv = new DataView(statusBuf);
  sdv.setUint8(0, 1);
  sdv.setUint8(1, 2);
  sdv.setUint8(2, 3);
  sdv.setUint8(3, 88);
  sdv.setUint8(4, CAI_FLAG.CONNECTED | CAI_FLAG.INFER_ENABLED);
  sdv.setUint32(5, 42, true);
  sdv.setUint16(9, 64, true);
  sdv.setFloat32(11, 0.5, true);
  const statusEvt = decodeFrame(encodeFrame(CAI_EVT.STATUS, new Uint8Array(statusBuf)));
  const st = parseStatusPayload(statusEvt.payload);
  assert.equal(st.loaded, true);
  assert.equal(st.connected, true);
  assert.equal(st.inferEnabled, true);
  assert.equal(st.batteryPct, 88);
  assert.equal(st.inferCount, 42);
  assert.equal(st.arenaKb, 64);
  assert.equal(st.threshold, 0.5);

  const resBuf = new ArrayBuffer(8);
  const rdv = new DataView(resBuf);
  rdv.setUint8(0, 2);
  rdv.setUint8(1, 3);
  rdv.setUint16(2, 315, true);
  rdv.setFloat32(4, 0.875, true);
  const res = parseResultPayload(new Uint8Array(resBuf));
  assert.equal(res.topIndex, 2);
  assert.equal(res.inferUs, 315);
  assert.equal(res.topScore, 0.875);
  assert.equal(typeof crc8(new Uint8Array([1, 2, 3])), 'number');
});

test('Capacitor Android companion shell, www assets, and wearable hooks are synced', () => {
  const cap = JSON.parse(fs.readFileSync(path.join(ROOT, 'capacitor.config.json'), 'utf8'));
  assert.equal(cap.appId, 'com.skat.chipai');
  assert.equal(cap.webDir, 'www');

  for (const rel of [
    'www/index.html',
    'www/protocol.js',
    'www/wearable-input.js',
    'android/app/build.gradle',
    'android/app/src/main/AndroidManifest.xml',
    'android/app/src/main/java/com/skat/chipai/MainActivity.java',
    'android/app/src/main/assets/public/index.html',
  ]) {
    assert.ok(fs.existsSync(path.join(ROOT, rel)), `missing ${rel}`);
  }

  const html = fs.readFileSync(path.join(ROOT, 'www/index.html'), 'utf8');
  assert.match(html, /@media\s*\(max-width:\s*280px\),\s*\(max-height:\s*300px\)/);
});
