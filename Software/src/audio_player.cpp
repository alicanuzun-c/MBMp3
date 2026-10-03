#include "audio_player.h"

#include <Arduino.h>
#include <minimp3.h>

#include "storage.h"

namespace {
constexpr size_t mp3InputBufferSize = 4096;
// Bir MP3 karesi 4.6 KB PCM uretir; tampon birkac kare tutmazsa Bluetooth tamponu bosaltir
// ve araya sessizlik girer (ses cizirdar). 12 KB ~70 ms ses demek.
constexpr size_t pcmRingBufferSize = 12 * 1024;
constexpr size_t pcmChunkFrames = 128;
constexpr uint32_t audioSampleRate = 44100;

FsFile playbackFile;
// Ses tamponlari statik DRAM'e sigmadigi icin heap'ten ayrilir.
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
// Teshis: Bluetooth'un istedigi ve tampondan verilebilen PCM miktari (pcmRingMux ile korunur).
uint32_t statsRequestedBytes;
uint32_t statsDeliveredBytes;

void resetPcmRing() {
    portENTER_CRITICAL(&pcmRingMux);
    pcmReadPosition = 0;
    pcmWritePosition = 0;
    pcmBufferedBytes = 0;
    portEXIT_CRITICAL(&pcmRingMux);
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
            const int bytesRead = storageReadFile(
                playbackFile,
                mp3InputBuffer + inputBytes,
                mp3InputBufferSize - inputBytes);
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
                    Serial.printf("[MP3] durdu: %s\n",
                                  generation != playbackGeneration ? "yeni parca secildi"
                                  : inputEnded                     ? "dosya sonu"
                                                                   : "MP3 cozulemedi");
                    break;
                }
                if (sourceRate == 0) {
                    Serial.printf("[MP3] format: %d Hz, %d kanal, %d kbps\n",
                                  frameInfo.hz, frameInfo.channels, frameInfo.bitrate_kbps);
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
                // 44.1 kHz olmayan parcalarda iki kaynak ornegi arasinda dogrusal ara deger
                // alinir; ornek tekrarlamak metalik bir tini birakir. 44.1 kHz'de kesir 0'dir.
                const int64_t sourcePosition =
                    (static_cast<int64_t>(outputFramePosition + index) * sourceRate << 16) /
                    audioSampleRate;
                const int sourceFrame = min(samplesInFrame - 1, static_cast<int>(sourcePosition >> 16));
                const int nextFrame = min(samplesInFrame - 1, sourceFrame + 1);
                const int64_t fraction = sourcePosition & 0xFFFF;
                for (int channel = 0; channel < 2; ++channel) {
                    const int sourceChannel = sourceChannels == 1 ? 0 : channel;
                    const int32_t current = decodedPcm[sourceFrame * sourceChannels + sourceChannel];
                    const int32_t next = decodedPcm[nextFrame * sourceChannels + sourceChannel];
                    outputChunk[index * 2 + channel] =
                        static_cast<int16_t>(current + (((next - current) * fraction) >> 16));
                }
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
}

bool audioPlayerBegin() {
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
    // Oncelik LVGL'den (2) yuksek: cozucu geride kalirsa tampon bosalir ve ses cizirdar.
    if (xTaskCreatePinnedToCore(mp3DecodeTask,
                                "MP3 decode",
                                20 * 1024,
                                nullptr,
                                5,
                                &mp3DecoderTaskHandle,
                                0) != pdPASS) {
        mp3DecoderTaskHandle = nullptr;
        return false;
    }
    return true;
}

AudioPlayResult audioPlayerPlay(const char *path) {
    if (!storageOpenFile(playbackFile, path)) {
        return AudioPlayResult::OpenFailed;
    }

    if (mp3DecoderTaskHandle == nullptr) {
        return AudioPlayResult::NoMemory;
    }

    ++playbackGeneration;
    resetPcmRing();
    xTaskNotifyGive(mp3DecoderTaskHandle);
    Serial.printf("[MP3] calma: %s\n", path);
    return AudioPlayResult::Started;
}

void audioPlayerTakeStats(uint32_t &requestedBytes, uint32_t &deliveredBytes) {
    portENTER_CRITICAL(&pcmRingMux);
    requestedBytes = statsRequestedBytes;
    deliveredBytes = statsDeliveredBytes;
    statsRequestedBytes = 0;
    statsDeliveredBytes = 0;
    portEXIT_CRITICAL(&pcmRingMux);
}

size_t audioPlayerReadPcm(uint8_t *destination, size_t requestedBytes) {
    if (pcmRingBuffer == nullptr) {
        return 0;
    }
    portENTER_CRITICAL(&pcmRingMux);
    const size_t bytesToRead = min(requestedBytes, pcmBufferedBytes);
    statsRequestedBytes += requestedBytes;
    statsDeliveredBytes += bytesToRead;
    const size_t firstPart = min(bytesToRead, pcmRingBufferSize - pcmReadPosition);
    memcpy(destination, pcmRingBuffer + pcmReadPosition, firstPart);
    memcpy(destination + firstPart, pcmRingBuffer, bytesToRead - firstPart);
    pcmReadPosition = (pcmReadPosition + bytesToRead) % pcmRingBufferSize;
    pcmBufferedBytes -= bytesToRead;
    portEXIT_CRITICAL(&pcmRingMux);
    return bytesToRead;
}
