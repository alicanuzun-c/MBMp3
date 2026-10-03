#include "settings_view.h"

#include <Arduino.h>

#include "bluetooth.h"
#include "gui.h"
#include "ui/screens.h"

namespace {
lv_obj_t *bluetoothStatusLabel;
lv_obj_t *bluetoothActionLabel;
lv_obj_t *bluetoothListContainer;
uint32_t shownBluetoothListVersion = UINT32_MAX;
bool shownBluetoothConnected;

void onBluetoothDeviceClicked(lv_event_t *event) {
    const size_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    if (bluetoothConnect(index)) {
        lv_label_set_text_fmt(bluetoothStatusLabel, "Bağlanıyor: %s", bluetoothActiveDeviceName());
    }
}

void onBluetoothActionClicked(lv_event_t *) {
    bluetoothStart();

    if (bluetoothGetState() == BluetoothState::Connected) {
        bluetoothDisconnect();
        lv_label_set_text(bluetoothStatusLabel, "Bağlantı kesiliyor");
        return;
    }

    bluetoothRequestScan();
}

void addBluetoothDeviceRow(const BluetoothDevice &device, size_t index) {
    lv_obj_t *button = lv_button_create(bluetoothListContainer);
    lv_obj_set_size(button, LV_PCT(100), 28);
    lv_obj_add_event_cb(button,
                        onBluetoothDeviceClicked,
                        LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<uintptr_t>(index)));

    lv_obj_t *icon = lv_label_create(button);
    lv_label_set_text(icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    char addressText[18];
    bluetoothFormatAddress(device.address, addressText, sizeof(addressText));
    lv_obj_t *nameLabel = lv_label_create(button);
    lv_label_set_text(nameLabel, device.name[0] != '\0' ? device.name : addressText);
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(nameLabel, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_set_width(nameLabel, 140);
    lv_obj_align(nameLabel, LV_ALIGN_LEFT_MID, 18, 0);

    lv_obj_t *rssiLabel = lv_label_create(button);
    lv_label_set_text_fmt(rssiLabel, "%d", device.rssi);
    lv_obj_align(rssiLabel, LV_ALIGN_RIGHT_MID, 0, 0);
}
}

void settingsViewCreate() {
    // EEZ'nin olusturdugu bos yer tutucuyu kaldir, Bluetooth listesini buraya kur.
    lv_obj_clean(objects.tab_settings);
    lv_obj_set_style_pad_all(objects.tab_settings, 6, LV_PART_MAIN);
    lv_obj_remove_flag(objects.tab_settings, LV_OBJ_FLAG_SCROLLABLE);

    bluetoothStatusLabel = lv_label_create(objects.tab_settings);
    lv_obj_set_style_text_font(bluetoothStatusLabel, &lv_font_turkish_14, LV_PART_MAIN);
    lv_label_set_long_mode(bluetoothStatusLabel, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(bluetoothStatusLabel, 150);
    lv_obj_align(bluetoothStatusLabel, LV_ALIGN_TOP_LEFT, 0, 6);
    lv_label_set_text(bluetoothStatusLabel, "");

    lv_obj_t *actionButton = lv_button_create(objects.tab_settings);
    lv_obj_set_size(actionButton, 64, 28);
    lv_obj_align(actionButton, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(actionButton, onBluetoothActionClicked, LV_EVENT_CLICKED, nullptr);
    bluetoothActionLabel = lv_label_create(actionButton);
    lv_obj_set_style_text_font(bluetoothActionLabel, &lv_font_turkish_14, LV_PART_MAIN);
    lv_label_set_text(bluetoothActionLabel, "Tara");
    lv_obj_center(bluetoothActionLabel);

    bluetoothListContainer = lv_obj_create(objects.tab_settings);
    lv_obj_set_pos(bluetoothListContainer, 0, 34);
    lv_obj_set_size(bluetoothListContainer, LV_PCT(100), 221 - 12 - 34);
    lv_obj_set_style_pad_all(bluetoothListContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(bluetoothListContainer, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bluetoothListContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(bluetoothListContainer, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(bluetoothListContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(bluetoothListContainer, LV_DIR_VER);

    settingsViewRefresh();
}

void settingsViewRefresh() {
    bluetoothServiceScan();
    const BluetoothState state = bluetoothGetState();
    const bool connected = state == BluetoothState::Connected;
    guiSetLabelTextIfChanged(bluetoothActionLabel, connected ? "Kes" : "Tara");

    const char *deviceName =
        bluetoothActiveDeviceName()[0] != '\0' ? bluetoothActiveDeviceName() : "kayıtlı cihaz";
    char statusText[64];
    switch (state) {
        case BluetoothState::Off:
            strlcpy(statusText, "Bluetooth kapalı", sizeof(statusText));
            break;
        case BluetoothState::Connected:
            snprintf(statusText, sizeof(statusText), "Bağlı: %s", deviceName);
            break;
        case BluetoothState::Connecting:
            snprintf(statusText, sizeof(statusText), "Bağlanıyor: %s", deviceName);
            break;
        case BluetoothState::Scanning:
            strlcpy(statusText, "Taranıyor...", sizeof(statusText));
            break;
        case BluetoothState::Idle:
            strlcpy(statusText, "Bağlı değil", sizeof(statusText));
            break;
    }
    guiSetLabelTextIfChanged(bluetoothStatusLabel, statusText);

    if (bluetoothDeviceListVersion() == shownBluetoothListVersion &&
        connected == shownBluetoothConnected) {
        return;
    }

    BluetoothDevice devices[maxBluetoothDevices];
    const size_t deviceCount = bluetoothCopyDevices(devices, maxBluetoothDevices, shownBluetoothListVersion);
    shownBluetoothConnected = connected;

    lv_obj_clean(bluetoothListContainer);
    if (connected) {
        return;
    }
    for (size_t index = 0; index < deviceCount; ++index) {
        addBluetoothDeviceRow(devices[index], index);
    }
}
