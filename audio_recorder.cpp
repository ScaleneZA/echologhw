#include "audio_recorder.h"
#include "config.h"

static File recordingFile;
static bool recording = false;
static uint32_t bytesWritten = 0;
static uint32_t recordingStartTime = 0;
static int16_t audioBuffer[BUFFER_SIZE];

bool initializeAudio() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };

  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    DEBUG_PRINTF("Failed to install I2S driver: %s\n", esp_err_to_name(err));
    return false;
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    DEBUG_PRINTF("Failed to set I2S pins: %s\n", esp_err_to_name(err));
    i2s_driver_uninstall(I2S_PORT);
    return false;
  }

  DEBUG_PRINTLN("Audio system initialized successfully");
  return true;
}

wav_header_t createWAVHeader(uint32_t dataSize) {
  wav_header_t header;
  
  memcpy(header.chunkID, "RIFF", 4);
  header.chunkSize = 36 + dataSize;
  memcpy(header.format, "WAVE", 4);
  
  memcpy(header.subchunk1ID, "fmt ", 4);
  header.subchunk1Size = 16;
  header.audioFormat = 1; // PCM
  header.numChannels = CHANNELS;
  header.sampleRate = SAMPLE_RATE;
  header.byteRate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
  header.blockAlign = CHANNELS * (BITS_PER_SAMPLE / 8);
  header.bitsPerSample = BITS_PER_SAMPLE;
  
  memcpy(header.subchunk2ID, "data", 4);
  header.subchunk2Size = dataSize;
  
  return header;
}

bool startRecording(const String& filename) {
  if (recording) {
    DEBUG_PRINTLN("Already recording");
    return false;
  }

  recordingFile = SD.open(filename, FILE_WRITE);
  if (!recordingFile) {
    DEBUG_PRINTF("Failed to create recording file: %s\n", filename.c_str());
    return false;
  }

  wav_header_t header = createWAVHeader(0);
  size_t written = recordingFile.write((uint8_t*)&header, sizeof(header));
  if (written != sizeof(header)) {
    DEBUG_PRINTLN("Failed to write WAV header");
    recordingFile.close();
    return false;
  }

  esp_err_t err = i2s_start(I2S_PORT);
  if (err != ESP_OK) {
    DEBUG_PRINTF("Failed to start I2S: %s\n", esp_err_to_name(err));
    recordingFile.close();
    return false;
  }

  recording = true;
  bytesWritten = 0;
  recordingStartTime = millis();
  
  DEBUG_PRINTF("Started recording to: %s\n", filename.c_str());
  return true;
}

bool continueRecording() {
  if (!recording) {
    return false;
  }

  size_t bytesRead = 0;
  esp_err_t err = i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
  
  if (err != ESP_OK) {
    DEBUG_PRINTF("I2S read error: %s\n", esp_err_to_name(err));
    return false;
  }

  if (bytesRead > 0) {
    size_t written = recordingFile.write((uint8_t*)audioBuffer, bytesRead);
    if (written != bytesRead) {
      DEBUG_PRINTLN("Failed to write audio data to file");
      return false;
    }
    
    bytesWritten += written;
    
    if (bytesWritten % (SAMPLE_RATE * 2) == 0) {
      recordingFile.flush();
    }
  }

  return true;
}

bool stopRecording() {
  if (!recording) {
    DEBUG_PRINTLN("Not currently recording");
    return false;
  }

  esp_err_t err = i2s_stop(I2S_PORT);
  if (err != ESP_OK) {
    DEBUG_PRINTF("Failed to stop I2S: %s\n", esp_err_to_name(err));
  }

  recordingFile.seek(0);
  wav_header_t header = createWAVHeader(bytesWritten);
  size_t written = recordingFile.write((uint8_t*)&header, sizeof(header));
  
  recordingFile.close();
  recording = false;
  
  if (written != sizeof(header)) {
    DEBUG_PRINTLN("Failed to update WAV header");
    return false;
  }

  uint32_t duration = millis() - recordingStartTime;
  DEBUG_PRINTF("Recording stopped. Duration: %lu ms, Bytes: %lu\n", duration, bytesWritten);
  
  return true;
}

bool isRecording() {
  return recording;
}

uint32_t getRecordingDuration() {
  if (!recording) {
    return 0;
  }
  return millis() - recordingStartTime;
}