#include "ble_gatt.h"
#include "cai_protocol.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <string.h>

static cai_app_t *s_app;
static bool s_connected;
static struct bt_conn *s_conn;

static struct bt_uuid_128 uuid_svc = BT_UUID_INIT_128(
    0x01, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static struct bt_uuid_128 uuid_control = BT_UUID_INIT_128(
    0x03, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static struct bt_uuid_128 uuid_status = BT_UUID_INIT_128(
    0x01, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static struct bt_uuid_128 uuid_result = BT_UUID_INIT_128(
    0x02, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static struct bt_uuid_128 uuid_info = BT_UUID_INIT_128(
    0x04, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);

static uint8_t status_ccc_cfg_changed;
static uint8_t result_ccc_cfg_changed;
static char s_device_name[24] = "ChipAI-0000";

#define STATUS_ATTR_INDEX 4
#define RESULT_ATTR_INDEX 7

static ssize_t on_control_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(offset);
    ARG_UNUSED(flags);

    if (s_app != NULL) {
        cai_app_on_control_write(s_app, buf, len);
    }
    return len;
}

static ssize_t on_status_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                              void *buf, uint16_t len, uint16_t offset)
{
    ARG_UNUSED(attr);

    if (s_app == NULL) {
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    const cai_status_t *st = cai_app_status(s_app);
    uint8_t pl[CAI_MAX_PAYLOAD];
    size_t plen = cai_build_status_payload(st, pl, sizeof(pl));
    uint8_t frame[32];
    size_t fn = cai_encode_frame(CAI_EVT_STATUS, pl, (uint8_t)plen, frame, sizeof(frame));
    return bt_gatt_attr_read(conn, attr, buf, len, offset, frame, fn);
}

static ssize_t on_info_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    ARG_UNUSED(attr);
    static const char info[] = "ChipAI-nRF52-v1.0.0";
    return bt_gatt_attr_read(conn, attr, buf, len, offset, info, sizeof(info) - 1);
}

BT_GATT_SERVICE_DEFINE(cai_svc,
    BT_GATT_PRIMARY_SERVICE(&uuid_svc),
    BT_GATT_CHARACTERISTIC(&uuid_control.uuid,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE,
                           NULL, on_control_write, NULL),
    BT_GATT_CHARACTERISTIC(&uuid_status.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           on_status_read, NULL, NULL),
    BT_GATT_CCC(&status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&uuid_result.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(&result_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&uuid_info.uuid,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           on_info_read, NULL, NULL)
);

static int notify_frame(uint8_t attr_index, uint8_t evt_cmd,
                        const uint8_t *payload, uint8_t payload_len)
{
    if (!s_connected || s_conn == NULL) {
        return 0;
    }

    uint8_t frame[6 + CAI_MAX_PAYLOAD];
    size_t n = cai_encode_frame(evt_cmd, payload, payload_len, frame, sizeof(frame));
    if (n == 0) {
        return -EINVAL;
    }

    return bt_gatt_notify(s_conn, &cai_svc.attrs[attr_index], frame, n);
}

void ble_gatt_notify_status(uint8_t evt_cmd, const uint8_t *payload, uint8_t len, void *ctx)
{
    ARG_UNUSED(ctx);
    notify_frame(STATUS_ATTR_INDEX, evt_cmd, payload, len);
}

void ble_gatt_notify_result(uint8_t evt_cmd, const uint8_t *payload, uint8_t len, void *ctx)
{
    ARG_UNUSED(ctx);
    notify_frame(RESULT_ATTR_INDEX, evt_cmd, payload, len);
}

void ble_gatt_notify_error(uint8_t code, void *ctx)
{
    ARG_UNUSED(ctx);
    notify_frame(STATUS_ATTR_INDEX, CAI_EVT_ERROR, &code, 1);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        return;
    }
    s_conn = bt_conn_ref(conn);
    s_connected = true;
    if (s_app) {
        cai_app_set_connected(s_app, true);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(reason);
    if (s_conn) {
        bt_conn_unref(s_conn);
        s_conn = NULL;
    }
    s_connected = false;
    if (s_app) {
        cai_app_set_connected(s_app, false);
    }
}

BT_CONN_CB_DEFINE(conn_cb) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void set_adv_name(void)
{
    bt_addr_le_t addrs[1];
    size_t count = 1;

    bt_id_get(addrs, &count);
    snprintk(s_device_name, sizeof(s_device_name), "ChipAI-%02X%02X",
             addrs[0].a.val[4], addrs[0].a.val[5]);
    bt_set_name(s_device_name);
}

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_UUID128_ALL, uuid_svc.val, sizeof(uuid_svc.val)),
};

static struct bt_data sd;

static void advertise(void)
{
    sd = (struct bt_data)BT_DATA(BT_DATA_NAME_COMPLETE,
                                 s_device_name,
                                 strlen(s_device_name));
    bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), &sd, 1);
}

int ble_gatt_init(cai_app_t *app)
{
    s_app = app;
    int err = bt_enable(NULL);
    if (err) {
        return err;
    }
    set_adv_name();
    advertise();
    return 0;
}
