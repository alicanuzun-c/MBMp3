#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <BluetoothA2DPSource.h>
#include <WiFi.h>
#include <lvgl.h>
#include <minimp3.h>
#include <time.h>
#include "ui/ui.h"
#include "ui/screens.h"

#if __has_include("wifi_passwords.h")
#include "wifi_passwords.h"
#else
#define MBMP3_WIFI_SSID ""
#define MBMP3_WIFI_PASSWORD ""
#endif

extern "C" {
LV_FONT_DECLARE(lv_font_turkish_14);
}

namespace {
constexpr int touchIrqPin = 36;
constexpr int touchMosiPin = 32;
constexpr int touchMisoPin = 39;
constexpr int touchClockPin = 25;
constexpr int touchChipSelectPin = 33;
constexpr int touchXAtLeft = 240;
constexpr int touchXAtRight = 0;
constexpr int touchYAtTop = 320;
constexpr int touchYAtBottom = 0;
constexpr uint32_t drawBufferRows = 8;

// SD kart yazilimsal SPI pinleri; TFT ve dokunmatikten bagimsiz
constexpr int sdChipSelectPin = 26;
constexpr uint8_t sdMisoPin = 19;
constexpr uint8_t sdMosiPin = 23;
constexpr uint8_t sdClockPin = 18;
constexpr size_t mp3InputBufferSize = 4096;
constexpr size_t pcmRingBufferSize = 4096;
constexpr size_t pcmChunkFrames = 128;
constexpr uint32_t audioSampleRate = 44100;
constexpr size_t maxBluetoothDevices = 10;
constexpr size_t bluetoothNameLength = 32;

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
        // arayuz tarama baslayana kadar istegi tekrarlar.
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

struct BluetoothDevice {
    esp_bd_addr_t address;
    char name[bluetoothNameLength];
    int rssi;
};

TFT_eSPI tft;
SPIClass touchscreenSPI(SPI);
SoftSpiDriver<sdMisoPin, sdMosiPin, sdClockPin> sdSoftSpi;
SdFs sdCard;
SelectableA2DPSource a2dpSource;
BluetoothDevice bluetoothDevices[maxBluetoothDevices];
size_t bluetoothDeviceCount;
volatile uint32_t bluetoothListVersion;
portMUX_TYPE bluetoothListMux = portMUX_INITIALIZER_UNLOCKED;
char connectingDeviceName[bluetoothNameLength];
lv_obj_t *bluetoothStatusLabel;
lv_obj_t *bluetoothActionLabel;
lv_obj_t *bluetoothListContainer;
uint32_t shownBluetoothListVersion = UINT32_MAX;
bool shownBluetoothConnected;
bool bluetoothScanWanted;
uint32_t lastBluetoothScanRequest;
FsFile playbackFile;
// Ses tamponlari statik DRAM'e sigmadigi icin ilk calmada heap'ten ayrilir.
mp3dec_t *mp3Decoder;
mp3d_sample_t *decodedPcm;
uint8_t *mp3InputBuffer;
uint8_t *pcmRingBuffer;
portMUX_TYPE pcmRingMux = portMUX_INITIALIZER_UNLOCKED;
size_t pcmReadPosition;
size_t pcmWritePosition;
size_t pcmBufferedBytes;
TaskHandle_t mp3DecoderTaskHandle;
volatile uint32_t playbackGeneration;
bool a2dpStarted;
bool clockSynced;
XPT2046_Touchscreen touchscreen(touchChipSelectPin, touchIrqPin);
lv_display_t *display;
constexpr size_t drawBufferSize = 320 * drawBufferRows * sizeof(lv_color_t);
lv_color_t *drawBuffer;

SemaphoreHandle_t spiBusMutex;
bool sdReady = false;
bool ntpConfigured = false;
uint32_t lastWifiUiUpdate = 0;
String currentDirectory = "/";
String selectedTrackPath;

void sdLock();
void sdUnlock();

enum FolderEntryKind : uintptr_t {
    FolderEntryDirectory = 1,
    FolderEntryTrack = 2,
    FolderEntryParent = 3
};

uint32_t getMillis() {
    return millis();
}

bool wifiCredentialsAvailable() {
    return MBMP3_WIFI_SSID[0] != '\0';
}

void startWifiConnection() {
    if (!wifiCredentialsAvailable()) {
        WiFi.mode(WIFI_OFF);
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(MBMP3_WIFI_SSID, MBMP3_WIFI_PASSWORD);
}

void updateClock() {
    if (WiFi.status() == WL_CONNECTED && !ntpConfigured) {
        configTzTime("TRT-3", "pool.ntp.org", "time.cloudflare.com");
        ntpConfigured = true;
    }

    struct tm localTime;
    if (getLocalTime(&localTime, 10)) {
        clockSynced = true;
        lv_label_set_text_fmt(objects.time, "%02d:%02d", localTime.tm_hour, localTime.tm_min);
    }
}

void resetPcmRing() {
    portENTER_CRITICAL(&pcmRingMux);
    pcmReadPosition = 0;
    pcmWritePosition = 0;
    pcmBufferedBytes = 0;
    portEXIT_CRITICAL(&pcmRingMux);
}

size_t readPcmRing(uint8_t *destination, size_t requestedBytes) {
    if (pcmRingBuffer == nullptr) {
        return 0;
    }
    portENTER_CRITICAL(&pcmRingMux);
    const size_t bytesToRead = min(requestedBytes, pcmBufferedBytes);
    const size_t firstPart = min(bytesToRead, pcmRingBufferSize - pcmReadPosition);
    memcpy(destination, pcmRingBuffer + pcmReadPosition, firstPart);
    memcpy(destination + firstPart, pcmRingBuffer, bytesToRead - firstPart);
    pcmReadPosition = (pcmReadPosition + bytesToRead) % pcmRingBufferSize;
    pcmBufferedBytes -= bytesToRead;
    portEXIT_CRITICAL(&pcmRingMux);
    return bytesToRead;
}

bool writePcmRing(const uint8_t *source, size_t bytesToWrite, uint32_t generation) {
    size_t bytesWritten = 0;
    while (bytesWritten < bytesToWrite && generation == playbackGeneration) {
        portENTER_CRITICAL(&pcmRingMux);
        const size_t freeBytes = pcmRingBufferSize - pcmBufferedBytes;
        const size_t chunk = min(bytesToWrite - bytesWritten, freeBytes);
        const size_t firstPart = min(chunk, pcmRingBufferSize - pcmWritePosition);
        memcpy(pcmRingBuffer + pcmWritePosition, source + bytesWritten, firstPart);
        memcpy(pcmRingBuffer, source + bytesWritten + firstPart, chunk - firstPart);
        pcmWritePosition = (pcmWritePosition + chunk) % pcmRingBufferSize;
        pcmBufferedBytes += chunk;
        portEXIT_CRITICAL(&pcmRingMux);

        bytesWritten += chunk;
        if (chunk == 0) {
            vTaskDelay(1);
        }
    }
    return bytesWritten == bytesToWrite;
}

int32_t provideBluetoothAudio(uint8_t *data, int32_t byteCount) {
    const size_t requestedBytes = byteCount > 0 ? static_cast<size_t>(byteCount) : 0;
    const size_t receivedBytes = readPcmRing(data, requestedBytes);
    if (receivedBytes < requestedBytes) {
        memset(data + receivedBytes, 0, requestedBytes - receivedBytes);
    }
    return byteCount;
}

void SelectableA2DPSource::runScanWork(uint16_t, void *) {
    a2dpSource.startScan();
}

void SelectableA2DPSource::runConnectWork(uint16_t, void *param) {
    a2dpSource.connectTo(static_cast<uint8_t *>(param));
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

void startBluetoothAudio() {
    if (a2dpStarted) {
        return;
    }

    // Wi-Fi ayni radyoyu paylasir: acik kalirsa A2DP baglantisi kopar ve ~50 KB heap gider.
    // Saat senkronize olduysa RTC zamani tutmaya devam eder.
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
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
    Serial.printf("[BT] baslatildi, bos heap: %u, en buyuk blok: %u, LVGL yigit boslugu: %u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap(), uxTaskGetStackHighWaterMark(nullptr));
}

bool decodeNextMp3Frame(mp3dec_t *decoder,
                        size_t &inputBytes,
                        bool &inputEnded,
                        int16_t *pcm,
                        int &sampleCount,
                        mp3dec_frame_info_t &frameInfo,
                        uint32_t generation) {
    for (;;) {
        if (generation != playbackGeneration) {
            return false;
        }

        if (inputBytes < mp3InputBufferSize && !inputEnded) {
            sdLock();
            const int bytesRead = playbackFile.read(
                mp3InputBuffer + inputBytes,
                mp3InputBufferSize - inputBytes);
            sdUnlock();
            if (bytesRead > 0) {
                inputBytes += static_cast<size_t>(bytesRead);
            } else {
                inputEnded = true;
            }
        }

        if (inputBytes == 0 && inputEnded) {
            return false;
        }

        memset(&frameInfo, 0, sizeof(frameInfo));
        sampleCount = mp3dec_decode_frame(
            decoder,
            mp3InputBuffer,
            static_cast<int>(inputBytes),
            pcm,
            &frameInfo);

        const size_t consumedBytes = min(
            inputBytes,
            static_cast<size_t>(max(0, frameInfo.frame_bytes)));
        if (consumedBytes > 0) {
            memmove(mp3InputBuffer,
                    mp3InputBuffer + consumedBytes,
                    inputBytes - consumedBytes);
            inputBytes -= consumedBytes;
        }

        if (sampleCount > 0 && frameInfo.channels > 0 && frameInfo.hz > 0) {
            return true;
        }

        if (inputEnded && consumedBytes == 0) {
            return false;
        }

        if (consumedBytes == 0 && inputBytes == mp3InputBufferSize) {
            return false;
        }
    }
}

void mp3DecodeTask(void *) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const uint32_t generation = playbackGeneration;
        mp3dec_init(mp3Decoder);
        size_t inputBytes = 0;
        bool inputEnded = false;
        int samplesInFrame = 0;
        int outputFramesInFrame = 0;
        int outputFramePosition = 0;
        int sourceRate = 0;
        int sourceChannels = 0;
        mp3dec_frame_info_t frameInfo = {};

        while (generation == playbackGeneration) {
            if (outputFramePosition >= outputFramesInFrame) {
                if (!decodeNextMp3Frame(mp3Decoder,
                                        inputBytes,
                                        inputEnded,
                                        decodedPcm,
                                        samplesInFrame,
                                        frameInfo,
                                        generation)) {
                    break;
                }
                sourceRate = frameInfo.hz;
                sourceChannels = frameInfo.channels;
                outputFramesInFrame = static_cast<int>(
                    (static_cast<int64_t>(samplesInFrame) * audioSampleRate + sourceRate / 2) /
                    sourceRate);
                outputFramePosition = 0;
            }

            int16_t outputChunk[pcmChunkFrames * 2];
            const int frameCount = min(
                static_cast<int>(pcmChunkFrames),
                outputFramesInFrame - outputFramePosition);
            for (int index = 0; index < frameCount; ++index) {
                const int outputFrame = outputFramePosition + index;
                const int sourceFrame = min(
                    samplesInFrame - 1,
                    static_cast<int>((static_cast<int64_t>(outputFrame) * sourceRate) /
                                     audioSampleRate));
                const int sourceIndex = sourceFrame * sourceChannels;
                outputChunk[index * 2] = decodedPcm[sourceIndex];
                outputChunk[index * 2 + 1] =
                    sourceChannels == 1 ? decodedPcm[sourceIndex] : decodedPcm[sourceIndex + 1];
            }

            if (!writePcmRing(reinterpret_cast<const uint8_t *>(outputChunk),
                              frameCount * 2 * sizeof(int16_t),
                              generation)) {
                break;
            }
            outputFramePosition += frameCount;
        }
    }
}

// Bluetooth ve Wi-Fi heap'i tuketmeden once setup()'ta bir kez cagrilir.
bool startAudioDecoder() {
    decodedPcm = static_cast<mp3d_sample_t *>(
        malloc(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t)));
    mp3InputBuffer = static_cast<uint8_t *>(malloc(mp3InputBufferSize));
    pcmRingBuffer = static_cast<uint8_t *>(malloc(pcmRingBufferSize));
    mp3Decoder = static_cast<mp3dec_t *>(malloc(sizeof(mp3dec_t)));
    if (decodedPcm == nullptr || mp3InputBuffer == nullptr || pcmRingBuffer == nullptr ||
        mp3Decoder == nullptr) {
        return false;
    }

    // minimp3 cozumleme sirasinda yigitta ~16 KB scratch kullanir; mp3dec_t bu yuzden heap'te.
    if (xTaskCreatePinnedToCore(mp3DecodeTask,
                                "MP3 decode",
                                20 * 1024,
                                nullptr,
                                2,
                                &mp3DecoderTaskHandle,
                                0) != pdPASS) {
        mp3DecoderTaskHandle = nullptr;
        return false;
    }
    return true;
}

void playSelectedTrack(const String &path) {
    sdLock();
    if (playbackFile.isOpen()) {
        playbackFile.close();
    }
    const bool opened = playbackFile.open(path.c_str(), O_RDONLY);
    sdUnlock();

    if (!opened) {
        lv_label_set_text(objects.current_screen, "MP3 acilamadi");
        return;
    }

    if (mp3DecoderTaskHandle == nullptr) {
        lv_label_set_text(objects.current_screen, "MP3 bellek yetersiz");
        return;
    }

    ++playbackGeneration;
    resetPcmRing();
    selectedTrackPath = path;
    xTaskNotifyGive(mp3DecoderTaskHandle);
    startBluetoothAudio();
    if (!a2dpSource.is_connected()) {
        lv_label_set_text(objects.current_screen, "Ayarlar'dan kulaklık seç");
    }
}

void flushDisplay(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    const uint32_t width = area->x2 - area->x1 + 1;
    const uint32_t height = area->y2 - area->y1 + 1;

    xSemaphoreTakeRecursive(spiBusMutex, portMAX_DELAY);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, width, height);
    tft.pushColors(reinterpret_cast<uint16_t *>(pixels), width * height, true);
    tft.endWrite();
    xSemaphoreGiveRecursive(spiBusMutex);

    lv_display_flush_ready(display);
}

void readTouch(lv_indev_t *, lv_indev_data_t *data) {
    static int16_t lastX = 0;
    static int16_t lastY = 0;

    if (touchscreen.tirqTouched() && touchscreen.touched()) {
        const TS_Point point = touchscreen.getPoint();
        const long measuredX = map(point.x, 200, 3700, 0, tft.width() - 1);
        const long measuredY = map(point.y, 240, 3800, 0, tft.height() - 1);
        lastX = constrain(map(measuredX, touchXAtLeft, touchXAtRight, 0, tft.width() - 1), 0, tft.width() - 1);
        lastY = constrain(map(measuredY, touchYAtTop, touchYAtBottom, 0, tft.height() - 1), 0, tft.height() - 1);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = lastX;
    data->point.y = lastY;
}

void refreshBluetoothSettings();

void lvglTask(void *) {
    for (;;) {
        ui_tick();
        if (millis() - lastWifiUiUpdate >= 1000) {
            lastWifiUiUpdate = millis();
            updateClock();
            if (lv_tabview_get_tab_active(objects.tab_view) == 2) {
                refreshBluetoothSettings();
            }
        }
        const uint32_t waitMs = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(waitMs < 5 ? 5 : waitMs));
    }
}

void sdLock()   { xSemaphoreTakeRecursive(spiBusMutex, portMAX_DELAY); }
void sdUnlock() { xSemaphoreGiveRecursive(spiBusMutex); }

void refreshFolderView();

void changeToParentDirectory() {
    if (currentDirectory == "/") {
        return;
    }

    const int lastSlash = currentDirectory.lastIndexOf('/');
    currentDirectory = lastSlash <= 0 ? "/" : currentDirectory.substring(0, lastSlash);
}

void onFolderEntryClicked(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    const uintptr_t entryKind = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    if (entryKind == FolderEntryParent) {
        changeToParentDirectory();
        refreshFolderView();
        return;
    }

    lv_obj_t *button = static_cast<lv_obj_t *>(lv_event_get_target(event));
    lv_obj_t *nameLabel = lv_obj_get_child(button, 1);
    if (nameLabel == nullptr) {
        return;
    }

    const String entryName = lv_label_get_text(nameLabel);
    String entryPath = currentDirectory;
    if (!entryPath.endsWith("/")) {
        entryPath += "/";
    }
    entryPath += entryName;

    if (entryKind == FolderEntryDirectory) {
        currentDirectory = entryPath;
        refreshFolderView();
        return;
    }

    entryPath += ".mp3";
    playSelectedTrack(entryPath);
}

void addFolderEntry(const String &name, FolderEntryKind kind, uint16_t rowIndex) {
    lv_obj_t *button = lv_button_create(objects.folder_container);
    lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 4 + rowIndex * 30);
    lv_obj_set_size(button, 224, 28);
    lv_obj_add_event_cb(
        button,
        onFolderEntryClicked,
        LV_EVENT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(kind)));

    lv_obj_t *icon = lv_label_create(button);
    lv_label_set_text(icon, kind == FolderEntryTrack ? LV_SYMBOL_AUDIO : LV_SYMBOL_DIRECTORY);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *nameLabel = lv_label_create(button);
    lv_label_set_text(nameLabel, name.c_str());
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(nameLabel, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_set_width(nameLabel, 188);
    lv_obj_align(nameLabel, LV_ALIGN_LEFT_MID, 24, 0);
}

void refreshFolderView() {
    lv_obj_clean(objects.folder_container);
    lv_obj_set_style_pad_all(objects.folder_container, 0, LV_PART_MAIN);
    const bool folderTabActive = lv_tabview_get_tab_active(objects.tab_view) == 1;
    if (folderTabActive) {
        lv_label_set_text(objects.current_screen, currentDirectory.c_str());
    }

    if (!sdReady) {
        if (folderTabActive) {
            lv_label_set_text(objects.current_screen, "SD kart hazir degil");
        }
        return;
    }

    sdLock();
    FsFile directory;
    if (!directory.open(currentDirectory.c_str(), O_RDONLY) || !directory.isDir()) {
        sdUnlock();
        if (folderTabActive) {
            lv_label_set_text(objects.current_screen, "Klasor acilamadi");
        }
        if (directory.isOpen()) {
            directory.close();
        }
        return;
    }

    uint16_t rowIndex = 0;
    if (currentDirectory != "/") {
        addFolderEntry("geri", FolderEntryParent, rowIndex++);
    }

    FsFile entry;
    char entryName[256];
    while (entry.openNext(&directory, O_RDONLY)) {
        entry.getName(entryName, sizeof(entryName));
        String name(entryName);
        if (entry.isDir()) {
            String lowercaseName = name;
            lowercaseName.toLowerCase();
            if (lowercaseName == "system volume information") {
                entry.close();
                continue;
            }
            addFolderEntry(name, FolderEntryDirectory, rowIndex++);
        } else {
            String lowercaseName = name;
            lowercaseName.toLowerCase();
            if (lowercaseName.endsWith(".mp3")) {
                name.remove(name.length() - 4);
                addFolderEntry(name, FolderEntryTrack, rowIndex++);
            }
        }
        entry.close();
    }
    directory.close();
    sdUnlock();

    if (rowIndex == 0 && folderTabActive) {
        lv_label_set_text(objects.current_screen, "Klasor bos veya MP3 yok");
    }
}

void formatBluetoothAddress(const esp_bd_addr_t address, char *text, size_t textSize) {
    snprintf(text, textSize, "%02X:%02X:%02X:%02X:%02X:%02X",
             address[0], address[1], address[2], address[3], address[4], address[5]);
}

void setLabelTextIfChanged(lv_obj_t *label, const char *text) {
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

void onBluetoothDeviceClicked(lv_event_t *event) {
    if (a2dpSource.is_connected()) {
        return;
    }

    const size_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    esp_bd_addr_t address;
    portENTER_CRITICAL(&bluetoothListMux);
    const bool valid = index < bluetoothDeviceCount;
    if (valid) {
        memcpy(address, bluetoothDevices[index].address, ESP_BD_ADDR_LEN);
        strlcpy(connectingDeviceName, bluetoothDevices[index].name, sizeof(connectingDeviceName));
    }
    portEXIT_CRITICAL(&bluetoothListMux);
    if (!valid) {
        return;
    }

    if (connectingDeviceName[0] == '\0') {
        formatBluetoothAddress(address, connectingDeviceName, sizeof(connectingDeviceName));
    }
    bluetoothScanWanted = false;
    a2dpSource.requestConnect(address);
    lv_label_set_text_fmt(bluetoothStatusLabel, "Bağlanıyor: %s", connectingDeviceName);
}

void onBluetoothActionClicked(lv_event_t *) {
    if (!a2dpStarted) {
        startBluetoothAudio();
    }

    if (a2dpSource.is_connected()) {
        a2dpSource.disconnect();
        lv_label_set_text(bluetoothStatusLabel, "Bağlantı kesiliyor");
        return;
    }

    portENTER_CRITICAL(&bluetoothListMux);
    bluetoothDeviceCount = 0;
    ++bluetoothListVersion;
    portEXIT_CRITICAL(&bluetoothListMux);
    bluetoothScanWanted = true;
    lastBluetoothScanRequest = millis();
    a2dpSource.requestScan();
}

// Tara'ya basildiysa tarama gercekten baslayana kadar istegi birkac saniyede bir yinele.
void retryBluetoothScanIfNeeded(bool connected) {
    if (connected) {
        bluetoothScanWanted = false;
        return;
    }
    if (!bluetoothScanWanted || a2dpSource.is_discovery_active() || a2dpSource.isBusyConnecting() ||
        millis() - lastBluetoothScanRequest < 4000) {
        return;
    }
    lastBluetoothScanRequest = millis();
    a2dpSource.requestScan();
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
    formatBluetoothAddress(device.address, addressText, sizeof(addressText));
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

void refreshBluetoothSettings() {
    const bool connected = a2dpSource.is_connected();
    retryBluetoothScanIfNeeded(connected);
    setLabelTextIfChanged(bluetoothActionLabel, connected ? "Kes" : "Tara");

    const char *deviceName = connectingDeviceName[0] != '\0' ? connectingDeviceName : "kayıtlı cihaz";
    char statusText[64];
    if (!a2dpStarted) {
        strlcpy(statusText, "Bluetooth kapalı", sizeof(statusText));
    } else if (connected) {
        snprintf(statusText, sizeof(statusText), "Bağlı: %s", deviceName);
    } else if (a2dpSource.isBusyConnecting()) {
        snprintf(statusText, sizeof(statusText), "Bağlanıyor: %s", deviceName);
    } else if (a2dpSource.is_discovery_active()) {
        strlcpy(statusText, "Taranıyor...", sizeof(statusText));
    } else {
        strlcpy(statusText, "Bağlı değil", sizeof(statusText));
    }
    setLabelTextIfChanged(bluetoothStatusLabel, statusText);

    if (bluetoothListVersion == shownBluetoothListVersion && connected == shownBluetoothConnected) {
        return;
    }

    BluetoothDevice devices[maxBluetoothDevices];
    portENTER_CRITICAL(&bluetoothListMux);
    const size_t deviceCount = bluetoothDeviceCount;
    memcpy(devices, bluetoothDevices, sizeof(devices));
    shownBluetoothListVersion = bluetoothListVersion;
    portEXIT_CRITICAL(&bluetoothListMux);
    shownBluetoothConnected = connected;

    lv_obj_clean(bluetoothListContainer);
    if (connected) {
        return;
    }
    for (size_t index = 0; index < deviceCount; ++index) {
        addBluetoothDeviceRow(devices[index], index);
    }
}

void createBluetoothSettings() {
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

    refreshBluetoothSettings();
}

void onTabChanged(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    switch (lv_tabview_get_tab_active(objects.tab_view)) {
        case 0:
            lv_label_set_text(objects.current_screen, "Play");
            break;
        case 1:
            lv_label_set_text(objects.current_screen, currentDirectory.c_str());
            break;
        case 2:
            lv_label_set_text(objects.current_screen, "Settings");
            break;
    }
}
}

void setup() {
    Serial.begin(115200);
    static const char *const resetReasons[] = {
        "bilinmiyor", "guc acildi", "harici pin", "yazilim", "panic (cokme)",
        "kesme watchdog", "gorev watchdog", "watchdog", "uyku", "brownout (voltaj dustu)", "SDIO"};
    const esp_reset_reason_t resetReason = esp_reset_reason();
    Serial.printf("\n[BOOT] reset nedeni: %s\n",
                  resetReason <= ESP_RST_SDIO ? resetReasons[resetReason] : "?");
    spiBusMutex = xSemaphoreCreateRecursiveMutex();

    // SD CS'i SD baslatilmadan once pasif tut.
    pinMode(sdChipSelectPin, OUTPUT);
    digitalWrite(sdChipSelectPin, HIGH);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    touchscreenSPI.begin(touchClockPin, touchMisoPin, touchMosiPin, touchChipSelectPin);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    sdLock();
    sdReady = sdCard.begin(SdSpiConfig(
        sdChipSelectPin,
        DEDICATED_SPI,
        SD_SCK_MHZ(0),
        &sdSoftSpi));
    sdUnlock();

    lv_init();
    lv_tick_set_cb(getMillis);

    drawBuffer = static_cast<lv_color_t *>(heap_caps_malloc(drawBufferSize, MALLOC_CAP_DMA));

    display = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(display, flushDisplay);
    lv_display_set_buffers(
        display,
        drawBuffer,
        nullptr,
        drawBufferSize,
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touchInput = lv_indev_create();
    lv_indev_set_type(touchInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(touchInput, display);
    lv_indev_set_read_cb(touchInput, readTouch);

    ui_init();
    lv_label_set_text(objects.time, "--:--");
    if (!startAudioDecoder()) {
        Serial.println("[MP3] cozucu icin bellek ayrilamadi");
    }
    Serial.printf("[MP3] cozucu hazir, bos heap: %u\n", ESP.getFreeHeap());
    startWifiConnection();
    lv_label_set_text(objects.current_screen, "Play");
    lv_obj_set_pos(objects.current_screen, 52, 12);
    lv_obj_set_size(objects.current_screen, 136, 18);
    lv_label_set_long_mode(objects.current_screen, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(objects.current_screen, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(objects.current_screen, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_add_event_cb(objects.tab_view, onTabChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    refreshFolderView();
    createBluetoothSettings();
    xTaskCreatePinnedToCore(lvglTask, "LVGL", 10 * 1024, nullptr, 2, nullptr, 1);
    Serial.printf("[BOOT] arayuz ve Wi-Fi hazir, bos heap: %u\n", ESP.getFreeHeap());
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}