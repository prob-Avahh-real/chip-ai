/**
 * ChipAI BLE GATT Protocol v1 — JS companion codec
 * Mirrors firmware/shared/cai_protocol.{h,c} and mobile/flutter/lib/ble/protocol.dart
 */

export const CAI_PROTO_VER = 1;
export const CAI_SOF0 = 0x43; // 'C'
export const CAI_SOF1 = 0x41; // 'A'
export const CAI_MAX_PAYLOAD = 32;

export const CAI_UUIDS = {
  service: 'ca10a100-0001-4000-8000-000000ca1001',
  status: 'ca10a101-0001-4000-8000-000000ca1001',
  result: 'ca10a102-0001-4000-8000-000000ca1001',
  control: 'ca10a103-0001-4000-8000-000000ca1001',
  deviceInfo: 'ca10a104-0001-4000-8000-000000ca1001',
  broadcastPrefix: 'ChipAI',
};

export const CAI_CMD = {
  REQUEST_STATUS: 0x01,
  START_INFER: 0x02,
  STOP_INFER: 0x03,
  SET_THRESHOLD: 0x04,
  PING: 0x05,
};

export const CAI_EVT = {
  STATUS: 0x80,
  RESULT: 0x81,
  PONG: 0x85,
  ERROR: 0x8f,
};

export const CAI_FLAG = {
  CONNECTED: 1 << 0,
  INFER_ENABLED: 1 << 1,
};

export function crc8(bytes) {
  let crc = 0;
  for (let i = 0; i < bytes.length; i++) {
    crc ^= bytes[i] & 0xff;
    for (let b = 0; b < 8; b++) {
      crc = (crc & 0x80) !== 0 ? ((crc << 1) ^ 0x07) & 0xff : (crc << 1) & 0xff;
    }
  }
  return crc;
}

export function encodeFrame(cmd, payload = new Uint8Array(0)) {
  const pl = payload instanceof Uint8Array ? payload : Uint8Array.from(payload);
  if (pl.length > CAI_MAX_PAYLOAD) {
    throw new RangeError(`Payload exceeds CAI_MAX_PAYLOAD (${CAI_MAX_PAYLOAD})`);
  }
  const body = new Uint8Array(3 + pl.length);
  body[0] = CAI_PROTO_VER;
  body[1] = cmd & 0xff;
  body[2] = pl.length;
  body.set(pl, 3);

  const frame = new Uint8Array(6 + pl.length);
  frame[0] = CAI_SOF0;
  frame[1] = CAI_SOF1;
  frame.set(body, 2);
  frame[5 + pl.length] = crc8(body);
  return frame;
}

export function decodeFrame(bytes) {
  const data = bytes instanceof Uint8Array ? bytes : Uint8Array.from(bytes);
  if (data.length < 6 || data[0] !== CAI_SOF0 || data[1] !== CAI_SOF1) return null;
  if (data[2] !== CAI_PROTO_VER) return null;
  const cmd = data[3];
  const len = data[4];
  if (len > CAI_MAX_PAYLOAD || data.length < 6 + len) return null;
  const body = data.subarray(2, 5 + len);
  if (crc8(body) !== data[5 + len]) return null;
  return { cmd, payload: data.slice(5, 5 + len) };
}

export function buildSetThresholdFrame(threshold) {
  const buf = new ArrayBuffer(4);
  new DataView(buf).setFloat32(0, threshold, true);
  return encodeFrame(CAI_CMD.SET_THRESHOLD, new Uint8Array(buf));
}

export function parseStatusPayload(payload) {
  const pl = payload instanceof Uint8Array ? payload : Uint8Array.from(payload);
  if (pl.length < 15) return null;
  const view = new DataView(pl.buffer, pl.byteOffset, pl.byteLength);
  const flags = pl[4];
  return {
    loaded: pl[0] === 1,
    backendKind: pl[1],
    labelCount: pl[2],
    batteryPct: pl[3],
    flags,
    connected: (flags & CAI_FLAG.CONNECTED) !== 0,
    inferEnabled: (flags & CAI_FLAG.INFER_ENABLED) !== 0,
    inferCount: view.getUint32(5, true),
    arenaKb: pl[9] | (pl[10] << 8),
    threshold: view.getFloat32(11, true),
  };
}

export function parseResultPayload(payload) {
  const pl = payload instanceof Uint8Array ? payload : Uint8Array.from(payload);
  if (pl.length < 8) return null;
  const view = new DataView(pl.buffer, pl.byteOffset, pl.byteLength);
  return {
    topIndex: pl[0],
    scoreCount: pl[1],
    inferUs: pl[2] | (pl[3] << 8),
    topScore: view.getFloat32(4, true),
  };
}
