#include "ble_gatt.h"
#include "cai_protocol.h"

#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <string.h>

static const char *TAG = "cai_ble";
static cai_app_t *s_app;

static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_status_val_handle;
static uint16_t s_result_val_handle;
static uint16_t s_device_info_val_handle;

static uint8_t s_own_addr_type;
static char s_device_name[24] = "ChipAI-0000";

/* CA10A100-0001-4000-8000-000000CA1001 */
static const ble_uuid128_t UUID_SVC =
    BLE_UUID128_INIT(0x01, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
                     0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static const ble_uuid128_t UUID_CONTROL =
    BLE_UUID128_INIT(0x03, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
                     0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static const ble_uuid128_t UUID_STATUS =
    BLE_UUID128_INIT(0x01, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
                     0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static const ble_uuid128_t UUID_RESULT =
    BLE_UUID128_INIT(0x02, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
                     0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);
static const ble_uuid128_t UUID_DEVICE_INFO =
    BLE_UUID128_INIT(0x04, 0x10, 0xca, 0x00, 0x00, 0x00, 0x00, 0x80,
                     0x00, 0x40, 0x01, 0x00, 0x00, 0xa1, 0x10, 0xca);

static int gap_event(struct ble_gap_event *event, void *arg);
static void on_sync(void);
static void on_reset(int reason);
static void advertise(void);
static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg);

static int notify_frame(uint16_t val_handle, uint8_t evt_cmd,
                        const uint8_t *payload, uint8_t payload_len)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return 0;
    }

    uint8_t frame[6 + CAI_MAX_PAYLOAD];
    size_t n = cai_encode_frame(evt_cmd, payload, payload_len, frame, sizeof(frame));
    if (n == 0) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(frame, n);
    if (om == NULL) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return ble_gatts_notify_custom(s_conn_handle, val_handle, om);
}

void ble_gatt_notify_status(uint8_t evt_cmd, const uint8_t *payload, uint8_t len, void *ctx)
{
    (void)ctx;
    notify_frame(s_status_val_handle, evt_cmd, payload, len);
}

void ble_gatt_notify_result(uint8_t evt_cmd, const uint8_t *payload, uint8_t len, void *ctx)
{
    (void)ctx;
    notify_frame(s_result_val_handle, evt_cmd, payload, len);
}

void ble_gatt_notify_error(uint8_t code, void *ctx)
{
    (void)ctx;
    notify_frame(s_status_val_handle, CAI_EVT_ERROR, &code, 1);
}

int ble_gatt_device_info_read(uint8_t *out, size_t cap)
{
    const char *info = "ChipAI-ESP32-v1.0.0";
    size_t n = strlen(info);
    if (n >= cap) {
        return -1;
    }
    memcpy(out, info, n);
    return (int)n;
}

static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    uint16_t chr = (uint16_t)(uintptr_t)arg;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (chr == 0x0003 && s_app != NULL) {
            uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
            uint8_t buf[64];
            if (om_len > sizeof(buf)) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            ble_hs_mbuf_to_flat(ctxt->om, buf, om_len, NULL);
            cai_app_on_control_write(s_app, buf, om_len);
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;

    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (chr == 0x0005) {
            uint8_t info[32];
            int n = ble_gatt_device_info_read(info, sizeof(info));
            if (n < 0) {
                return BLE_ATT_ERR_UNLIKELY;
            }
            return os_mbuf_append(ctxt->om, info, n) ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
        }
        if (chr == 0x0001 && s_app != NULL) {
            const cai_status_t *st = cai_app_status(s_app);
            uint8_t pl[CAI_MAX_PAYLOAD];
            size_t plen = cai_build_status_payload(st, pl, sizeof(pl));
            uint8_t frame[32];
            size_t fn = cai_encode_frame(CAI_EVT_STATUS, pl, (uint8_t)plen, frame, sizeof(frame));
            return os_mbuf_append(ctxt->om, frame, fn) ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
        }
        return BLE_ATT_ERR_UNLIKELY;

    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &UUID_SVC.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &UUID_CONTROL.u,
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .arg = (void *)(uintptr_t)0x0003,
            },
            {
                .uuid = &UUID_STATUS.u,
                .access_cb = gatt_access,
                .val_handle = &s_status_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .arg = (void *)(uintptr_t)0x0001,
            },
            {
                .uuid = &UUID_RESULT.u,
                .access_cb = gatt_access,
                .val_handle = &s_result_val_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .arg = (void *)(uintptr_t)0x0002,
            },
            {
                .uuid = &UUID_DEVICE_INFO.u,
                .access_cb = gatt_access,
                .val_handle = &s_device_info_val_handle,
                .flags = BLE_GATT_CHR_F_READ,
                .arg = (void *)(uintptr_t)0x0005,
            },
            { 0 }
        },
    },
    { 0 }
};

static void advertise(void)
{
    struct ble_gap_adv_params adv_params = { 0 };
    struct ble_hs_adv_fields fields = { 0 };
    struct ble_hs_adv_fields rsp = { 0 };

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)s_device_name;
    fields.name_len = strlen(s_device_name);
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    static ble_uuid128_t adv_uuid = UUID_SVC;
    rsp.uuids128 = &adv_uuid;
    rsp.num_uuids128 = 1;
    rsp.uuids128_is_complete = 1;
    ble_gap_adv_rsp_set_fields(&rsp);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, gap_event, NULL);
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            if (s_app) {
                cai_app_set_connected(s_app, true);
            }
            ESP_LOGI(TAG, "connected");
        } else {
            advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        if (s_app) {
            cai_app_set_connected(s_app, false);
        }
        ESP_LOGI(TAG, "disconnected");
        advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        return 0;

    default:
        return 0;
    }
}

static void on_sync(void)
{
    uint8_t addr[6];
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    ble_hs_id_copy_addr(s_own_addr_type, addr, NULL);
    snprintf(s_device_name, sizeof(s_device_name), "ChipAI-%02X%02X", addr[4], addr[5]);
    ble_svc_gap_device_name_set(s_device_name);
    advertise();
}

static void on_reset(int reason)
{
    ESP_LOGE(TAG, "reset reason=%d", reason);
}

static void host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

int ble_gatt_init(cai_app_t *app)
{
    s_app = app;

    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        return rc;
    }
    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        return rc;
    }

    nimble_port_freertos_init(host_task);
    return 0;
}
