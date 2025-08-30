/*
 *  audio.cpp
 *
 *  Created on: Aug 24, 2025
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <Arduino.h>
#include <SD.h>
#include <M5Unified.h>
#include <vector>
#include <string>
#include "app_config.hpp"
#include "lvgl_gui.hpp"
#include "audio.hpp"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/



/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

/* Constants for audio buffer*/
static constexpr const size_t buf_num = 3;
static constexpr const size_t buf_size = 1024;
static uint8_t wav_data[buf_num][buf_size];

/* Global flags and states */
static bool is_running = false;              /* Indicates if music is playing */
static bool next_track_requested = false;    /* Indicates if the next track is requested */

static std::vector<String> music_files;      /* List of .wav files in /music folder */
static int current_track_index = 0;          /* Current track index */
static bool playing_smile = false;
static bool normal_mode = true;

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/



/******************************************************************************/

/*!
 * @brief  Load all .wav files from /music directory
 */
static void load_music_files(void) {
    File dir = SD.open("/music");
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }

        if (!entry.isDirectory()) {
            String name = entry.name();
            if (name.endsWith(".wav") || name.endsWith(".WAV")) {
                music_files.push_back(name);
            #if 0  /* Just for debugging */
                Serial.printf("Found music file: %s\r\n", name.c_str());
            #endif
            }
        }

        entry.close();
    }

    dir.close();
}

/*!
 * @brief  Play a single WAV file from SD card
 */
static bool play_single_wav(const char* filename) {
    File file;
    file = SD.open(filename);

    if (!file) {
        Serial.println("File is NULL");
        return false;
    }

    wav_header_t wav_header;
    file.read((uint8_t*)&wav_header, sizeof(wav_header_t));

    /* Validate WAV format */
    if (memcmp(wav_header.RIFF, "RIFF", 4) ||
        memcmp(wav_header.WAVEfmt, "WAVEfmt ", 8) ||
        wav_header.audiofmt != 1 ||
        wav_header.bit_per_sample < 8 ||
        wav_header.bit_per_sample > 16 ||
        wav_header.channel == 0 || wav_header.channel > 2) {
        file.close();

        Serial.println("File is invalid WAV formwat");
        return false;
    }

    /* Seek to the data chunk */
    file.seek(offsetof(wav_header_t, audiofmt) + wav_header.fmt_chunk_size);
    sub_chunk_t sub_chunk;
    file.read((uint8_t*)&sub_chunk, 8);

    while (memcmp(sub_chunk.identifier, "data", 4)) {
        if (!file.seek(sub_chunk.chunk_size, SeekMode::SeekCur)) break;
        file.read((uint8_t*)&sub_chunk, 8);
    }

    if (memcmp(sub_chunk.identifier, "data", 4)) {
        file.close();
        Serial.println("File chunk error");
        return false;
    }

    /* Start playing audio data */
    int32_t data_len = sub_chunk.chunk_size;
    bool flg_16bit = (wav_header.bit_per_sample >> 4);

    size_t idx = 0;
    while (data_len > 0) {
        if (next_track_requested) {
            break;
        }

        if ((!normal_mode) && (!playing_smile)) {
            break;
        }

        if (!is_running) {
            M5.delay(10);
            if (playing_smile) {
                break;
            }
            continue;
        }

        size_t len = data_len < buf_size ? data_len : buf_size;
        len = file.read(wav_data[idx], len);
        data_len -= len;

        if (flg_16bit) {
            /* Play 16-bit audio */
            M5.Speaker.playRaw((const int16_t*)wav_data[idx], len >> 1, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
        }
        else {
            /* Play 8-bit audio */
            M5.Speaker.playRaw((const uint8_t*)wav_data[idx], len, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
        }

        idx = (idx + 1) % buf_num;
    }

    file.close();
    Serial.printf("Play file %s success\r\n", filename);
    return true;
}

/*!
 * @brief  Task to continuously play audio in background
 */
static void play_audio_task(void *arg) {
    while (1) {
        if (playing_smile) {
            normal_mode = false;
            play_single_wav("/smile_sound.wav");
            continue;
        }

        if (is_running && !music_files.empty()) {
            normal_mode = true;
            const String &file_to_play = music_files[current_track_index];
            Serial.printf("Now playing: %s\r\n", file_to_play.c_str());
            lvgl_set_song_name(file_to_play.c_str());
            String full_path = "/music/" + file_to_play;

            play_single_wav(full_path.c_str());

            /* If user did not request next manually, go to next automatically */
            if (!next_track_requested) {
                current_track_index = (current_track_index + 1) % music_files.size();
                system_config.play_index = current_track_index;
                save_configuration();
            }

            next_track_requested = false;
        }
        delay(10);
    }
}

/*!
 * @brief  Play splash audio
 */
void audio_play_splash(void) {
    is_running = true;
    play_single_wav("/hi_shin.wav");
    // play_single_wav("/funny.wav");
    is_running = false;
}

/*!
 * @brief  Play smile mode audio
 */
void audio_set_smile_mode(bool playing) {
    is_running = playing;
    playing_smile = playing;
}

/*!
 * @brief  Set running state
 */
void audio_set_running(bool running) {
    is_running = running;
}

/*!
 * @brief  Request play next track
 */
void audio_next_request(void) {
    if (is_running && !music_files.empty()) {
        next_track_requested = true;
        current_track_index = (current_track_index + 1) % music_files.size();
        Serial.println("Next track requested");
        system_config.play_index = current_track_index;
        save_configuration();
    }
}

/*!
 * @brief  Request play prev track
 */
void audio_prev_request(void) {
    if (is_running && !music_files.empty()) {
        next_track_requested = true;
        current_track_index = (current_track_index + music_files.size() - 1) % music_files.size();
        Serial.println("Prev track requested");
        system_config.play_index = current_track_index;
        save_configuration();
    }
}

/*!
 * @brief  Initialize audio process
 */
void audio_init(void) {
    load_music_files();  /* Load all .wav files from /music */

    /* Load the last index */
    if (system_config.magic == MAGIC_NUMBER) {
        current_track_index = system_config.play_index;
    }

    /* Create audio player task */
    xTaskCreatePinnedToCore(play_audio_task, "PLAY", 4096, NULL, 1, NULL, 0);
}
