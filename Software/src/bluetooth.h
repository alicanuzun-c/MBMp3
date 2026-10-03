#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr size_t maxBluetoothDevices = 10;
constexpr size_t bluetoothNameLength = 32;
constexpr size_t bluetoothAddressLength = 6;

struct BluetoothDevice {
    uint8_t address[bluetoothAddressLength];
    char name[bluetoothNameLength];  // Adini yayinlamayan cihazlarda bos
    int rssi;
};

enum class BluetoothState {
    Off,
    Idle,
    Scanning,
    Connecting,
    Connected
};

// A2DP kulakliga gonderilecek PCM'i saglayan fonksiyon (44.1 kHz stereo 16 bit).
using BluetoothPcmSource = size_t (*)(uint8_t *destination, size_t requestedBytes);

void bluetoothSetPcmSource(BluetoothPcmSource source);

// Wi-Fi'yi kapatir ve A2DP kaynagini baslatir; ikinci cagri bir sey yapmaz.
void bluetoothStart();

BluetoothState bluetoothGetState();

// Cihaz listesini temizler ve taramayi baslatir.
void bluetoothRequestScan();

// Tarama istendiyse ve radyo reddettiyse istegi yineler; arayuzden periyodik cagrilir.
void bluetoothServiceScan();

// Listedeki cihaza baglanir; secilen cihaz otomatik yeniden baglanma icin kaydedilir.
bool bluetoothConnect(size_t deviceIndex);
void bluetoothDisconnect();

// Baglanilan ya da baglanilmaya calisilan cihazin adi; bilinmiyorsa bos.
const char *bluetoothActiveDeviceName();

// Liste her degistiginde artar; arayuz yeniden cizmek icin karsilastirir.
uint32_t bluetoothDeviceListVersion();
// Listeyi ve o anki surumunu birlikte kopyalar; kopyalanan cihaz sayisini doner.
size_t bluetoothCopyDevices(BluetoothDevice *devices, size_t maxDevices, uint32_t &version);

void bluetoothFormatAddress(const uint8_t *address, char *text, size_t textSize);
