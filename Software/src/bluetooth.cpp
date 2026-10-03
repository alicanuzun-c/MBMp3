#include "bluetooth.h"

#include <Arduino.h>
#include <BluetoothA2DPSource.h>

#include "wifi_clock.h"

namespace {
// Kutuphane taramada bulunan ilk cihaza baglanir ve baglanti koptuktan sonra
// yeniden taramaz. Bu sinif cihaz secimini kullaniciya birakir; islemler
// kutuphanenin durum makinesiyle cakismamak icin BT gorev kuyrugunda calisir.
class SelectableA2DPSource : public BluetoothA2DPSource {
public:
    void requestScan() {
        bt_app_work_dispatch(runScanWork, 0, nullptr, 0, nullptr);
    }

    void requestConnect(const esp_bd_addr_t address) {
        esp_bd_addr_t addressCopy;
        memcpy(addressCopy, address, ESP_BD_ADDR_LEN);
        bt_app_work_dispatch(runConnectWork, 0, addressCopy, ESP_BD_ADDR_LEN, nullptr);
    }

    bool isBusyConnecting() const {
        return s_a2d_state == APP_AV_STATE_CONNECTING || s_a2d_state == APP_AV_STATE_DISCOVERED;
    }

private:
    static void runScanWork(uint16_t, void *);
    static void runConnectWork(uint16_t, void *param);

    void startScan() {
        if (is_connected() || discovery_active) {
            return;
        }
        // Elle tarama istendiginde kayitli cihaza yeniden baglanma denemelerini birak.
        is_autoreconnect_allowed = false;
        s_a2d_state = APP_AV_STATE_DISCOVERING;
        // Kayitli cihaza baglanma denemesi surerken radyo taramayi reddedebilir;
        // bluetoothServiceScan() tarama baslayana kadar istegi tekrarlar.
        const esp_err_t result = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        Serial.printf("[BT] tarama istegi: %s\n", esp_err_to_name(result));
    }

    void connectTo(uint8_t *address) {
        if (is_connected()) {
            return;
        }
        reconnect_status = AutoReconnect;
        is_autoreconnect_allowed = true;
        memcpy(peer_bd_addr, address, ESP_BD_ADDR_LEN);
        set_last_connection(peer_bd_addr);
        if (discovery_active) {
            // Tarama durunca kutuphane DISCOVERED durumunda peer_bd_addr'e baglanir.
            s_a2d_state = APP_AV_STATE_DISCOVERED;
            esp_bt_gap_cancel_discovery();
        } else {
            s_a2d_state = APP_AV_STATE_CONNECTING;
            connect_to(peer_bd_addr);
        }
    }
};

SelectableA2DPSource a2dpSource;
BluetoothPcmSource pcmSource;
bool a2dpStarted;

BluetoothDevice bluetoothDevices[maxBluetoothDevices];
size_t bluetoothDeviceCount;
volatile uint32_t bluetoothListVersion;
portMUX_TYPE bluetoothListMux = portMUX_INITIALIZER_UNLOCKED;
// Sadece arayuz gorevinden yazilir ve okunur.
char activeDeviceName[bluetoothNameLength];
bool scanWanted;
uint32_t lastScanRequest;

void SelectableA2DPSource::runScanWork(uint16_t, void *) {
    a2dpSource.startScan();
}

void SelectableA2DPSource::runConnectWork(uint16_t, void *param) {
    a2dpSource.connectTo(static_cast<uint8_t *>(param));
}

int32_t provideBluetoothAudio(uint8_t *data, int32_t byteCount) {
    const size_t requestedBytes = byteCount > 0 ? static_cast<size_t>(byteCount) : 0;
    const size_t receivedBytes = pcmSource != nullptr ? pcmSource(data, requestedBytes) : 0;
    if (receivedBytes < requestedBytes) {
        memset(data + receivedBytes, 0, requestedBytes - receivedBytes);
    }
    return byteCount;
}

// BT gorevinden cagrilir: bulunan cihazi listeye ekler, baglanmayi kullaniciya birakir.
bool recordBluetoothDevice(const char *name, esp_bd_addr_t address, int rssi) {
    portENTER_CRITICAL(&bluetoothListMux);
    size_t index = 0;
    while (index < bluetoothDeviceCount &&
           memcmp(bluetoothDevices[index].address, address, ESP_BD_ADDR_LEN) != 0) {
        ++index;
    }

    // Adini yayinlamayan cihazlarda kutuphane onceki cihazin adini tekrar verir.
    bool nameUsable = name != nullptr && name[0] != '\0';
    for (size_t other = 0; nameUsable && other < bluetoothDeviceCount; ++other) {
        if (other != index && strncmp(bluetoothDevices[other].name, name, bluetoothNameLength - 1) == 0) {
            nameUsable = false;
        }
    }

    const bool newDevice = index == bluetoothDeviceCount && index < maxBluetoothDevices;
    if (newDevice) {
        memcpy(bluetoothDevices[index].address, address, ESP_BD_ADDR_LEN);
        bluetoothDevices[index].name[0] = '\0';
        ++bluetoothDeviceCount;
        ++bluetoothListVersion;
    }

    if (index < bluetoothDeviceCount) {
        BluetoothDevice &device = bluetoothDevices[index];
        device.rssi = rssi;
        if (nameUsable && strncmp(device.name, name, bluetoothNameLength - 1) != 0) {
            strlcpy(device.name, name, sizeof(device.name));
            ++bluetoothListVersion;
        }
    }
    portEXIT_CRITICAL(&bluetoothListMux);

    if (newDevice) {
        Serial.printf("[BT] bulundu: %s (%d dBm)\n", nameUsable ? name : "(adsiz)", rssi);
    }
    return false;
}

void logBluetoothConnectionState(esp_a2d_connection_state_t state, void *) {
    static const char *const stateNames[] = {"kopuk", "baglaniyor", "bagli", "kopuyor"};
    Serial.printf("[BT] baglanti: %s, bos heap: %u\n",
                  state <= ESP_A2D_CONNECTION_STATE_DISCONNECTING ? stateNames[state] : "?",
                  ESP.getFreeHeap());
}

// Arduino'nun btStart() fonksiyonu kontrolcuyu BLE dahil (BTDM) acar. A2DP sadece klasik
// Bluetooth kullandigi icin kontrolcuyu once biz klasik modda baslatip BLE bellegini heap'e
// birakiyoruz; btStart() kontrolcu zaten acik oldugunu gorup dokunmaz.
void startClassicBluetoothController() {
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
        return;
    }

    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    esp_bt_controller_config_t config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    config.mode = ESP_BT_MODE_CLASSIC_BT;
    const esp_err_t initResult = esp_bt_controller_init(&config);
    const esp_err_t enableResult =
        initResult == ESP_OK ? esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) : initResult;
    if (enableResult != ESP_OK) {
        Serial.printf("[BT] klasik kontrolcu baslatilamadi: %s\n", esp_err_to_name(enableResult));
    }
}
}

void bluetoothSetPcmSource(BluetoothPcmSource source) {
    pcmSource = source;
}

void bluetoothStart() {
    if (a2dpStarted) {
        return;
    }

    // Wi-Fi ayni radyoyu paylasir: acik kalirsa A2DP baglantisi kopar ve heap yetmez.
    wifiClockShutdown();
    Serial.printf("[BT] Wi-Fi kapatildi, bos heap: %u\n", ESP.getFreeHeap());
    startClassicBluetoothController();
    Serial.printf("[BT] baslatiliyor, bos heap: %u, en buyuk blok: %u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    a2dpSource.set_on_connection_state_changed(logBluetoothConnectionState);
    a2dpSource.set_data_callback(provideBluetoothAudio);
    a2dpSource.set_ssid_callback(recordBluetoothDevice);
    a2dpSource.set_auto_reconnect(true, 3);
    a2dpSource.start();
    a2dpStarted = true;
    Serial.printf("[BT] baslatildi, bos heap: %u, en buyuk blok: %u, yigit boslugu: %u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap(), uxTaskGetStackHighWaterMark(nullptr));
}

BluetoothState bluetoothGetState() {
    if (!a2dpStarted) {
        return BluetoothState::Off;
    }
    if (a2dpSource.is_connected()) {
        return BluetoothState::Connected;
    }
    if (a2dpSource.isBusyConnecting()) {
        return BluetoothState::Connecting;
    }
    if (a2dpSource.is_discovery_active()) {
        return BluetoothState::Scanning;
    }
    return BluetoothState::Idle;
}

void bluetoothRequestScan() {
    portENTER_CRITICAL(&bluetoothListMux);
    bluetoothDeviceCount = 0;
    ++bluetoothListVersion;
    portEXIT_CRITICAL(&bluetoothListMux);
    scanWanted = true;
    lastScanRequest = millis();
    a2dpSource.requestScan();
}

void bluetoothServiceScan() {
    if (a2dpSource.is_connected()) {
        scanWanted = false;
        return;
    }
    if (!scanWanted || a2dpSource.is_discovery_active() || a2dpSource.isBusyConnecting() ||
        millis() - lastScanRequest < 4000) {
        return;
    }
    lastScanRequest = millis();
    a2dpSource.requestScan();
}

bool bluetoothConnect(size_t deviceIndex) {
    if (a2dpSource.is_connected()) {
        return false;
    }

    esp_bd_addr_t address;
    portENTER_CRITICAL(&bluetoothListMux);
    const bool valid = deviceIndex < bluetoothDeviceCount;
    if (valid) {
        memcpy(address, bluetoothDevices[deviceIndex].address, ESP_BD_ADDR_LEN);
        strlcpy(activeDeviceName, bluetoothDevices[deviceIndex].name, sizeof(activeDeviceName));
    }
    portEXIT_CRITICAL(&bluetoothListMux);
    if (!valid) {
        return false;
    }

    if (activeDeviceName[0] == '\0') {
        bluetoothFormatAddress(address, activeDeviceName, sizeof(activeDeviceName));
    }
    scanWanted = false;
    a2dpSource.requestConnect(address);
    return true;
}

void bluetoothDisconnect() {
    a2dpSource.disconnect();
}

const char *bluetoothActiveDeviceName() {
    return activeDeviceName;
}

uint32_t bluetoothDeviceListVersion() {
    return bluetoothListVersion;
}

size_t bluetoothCopyDevices(BluetoothDevice *devices, size_t maxDevices, uint32_t &version) {
    portENTER_CRITICAL(&bluetoothListMux);
    const size_t deviceCount = min(bluetoothDeviceCount, maxDevices);
    memcpy(devices, bluetoothDevices, deviceCount * sizeof(BluetoothDevice));
    version = bluetoothListVersion;
    portEXIT_CRITICAL(&bluetoothListMux);
    return deviceCount;
}

void bluetoothFormatAddress(const uint8_t *address, char *text, size_t textSize) {
    snprintf(text, textSize, "%02X:%02X:%02X:%02X:%02X:%02X",
             address[0], address[1], address[2], address[3], address[4], address[5]);
}
