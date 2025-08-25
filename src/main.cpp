/*
 *  main.cpp
 *
 *  Created on: Aug 24, 2025
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include <M5Unified.h>
#include "app_config.hpp"
#include "audio.hpp"
#include "lvgl_gui.hpp"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/



/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static uint8_t current_volume = 255;
static bool audio_running = false;
static bool has_changed = false;

static int main_screen = SCREEN_HOME;
static int main_sub_mode = HOME_PLAY_MUSIC;
static uint32_t last_active_ms = 0;
static uint32_t last_time_change_smile_ms = 0;

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/

system_config_t system_config;

/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static void control_loop(void);

/******************************************************************************/

/*!
 * @brief  Load configuration from EEPROM
 */
void load_configuration(void) {
    memset(&system_config, 0, sizeof(system_config));

    /* Initialize EEPROM */
    if (!EEPROM.begin(128)) {
        Serial.println("Failed to initialize EEPROM");
        return;
    }

    /* Read the struct from EEPROM */
    EEPROM.get(0, system_config);
    if (system_config.magic == MAGIC_NUMBER) {
        current_volume = system_config.volume;
        Serial.printf("Volume %d\r\n", current_volume);
    }
}

/*!
 * @brief  Save configuration to EEPROM
 */
void save_configuration(void) {
    system_config.magic = MAGIC_NUMBER;
    system_config.volume = current_volume;
    EEPROM.put(0, system_config);
    EEPROM.commit();
    Serial.printf("Save configuration. Volume %d\r\n", current_volume);
}

/*!
 * @brief  Run splash screen
 */
void run_splash(void) {
    delay(100);    /* Wait for lvgl is initialized */
    M5.Display.drawJpgFile(SPIFFS, "/shin.jpg", 0, 0);
    M5.update();
    M5.Speaker.setVolume(255);
    audio_play_splash();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Power up!");

    SD.begin(GPIO_NUM_4);
    SPIFFS.begin(true);
    load_configuration();

    /* Initialize M5 hardware and peripherals */
    auto config = M5.config();
    M5.begin(config);
    run_splash();
    M5.Speaker.setVolume(current_volume);

    /* Initialize components */
    audio_init();
    lvgl_gui_init();
    last_active_ms = millis();
}

void loop() {
    M5.update();
    control_loop();
    M5.delay(9);  /* Small delay for stability */
}

/******************************************************************************/

/*!
 * @brief  Handle home menu process
 */
static void home_menu_loop(void) {
    /* --- Button B: Select screen --- */
    if (M5.BtnB.wasReleased()) {
        if (main_sub_mode == HOME_PLAY_MUSIC) {
            main_screen = SCREEN_PLAY_MUSIC;
            audio_set_smile_mode(false);
        }
        else {
            main_screen = SCREEN_SMILE;
            audio_set_smile_mode(true);
        }
    
        has_changed = true;
        main_sub_mode = 0;
        lvgl_set_menu_mode(main_screen, main_sub_mode);
        Serial.print("Change to screen "); Serial.println(main_screen);
    }

    if (M5.BtnC.wasReleased()) {
        last_active_ms = millis();
        main_sub_mode = (main_sub_mode + 1) % HOME_SUB_COUNT;
        lvgl_set_menu_mode(main_screen, main_sub_mode);
    }

    if (M5.BtnA.wasReleased()) {
        last_active_ms = millis();
        main_sub_mode = (main_sub_mode + HOME_SUB_COUNT - 1) % HOME_SUB_COUNT;
        lvgl_set_menu_mode(main_screen, main_sub_mode);
    }
}

/*!
 * @brief  Handle sdcard menu process
 */
static void sdcard_menu_loop(void) {
    static bool is_save_A = false;
    static bool is_save_C = false;
    static bool is_holding_A = false;
    static bool is_holding_C = false;
    static uint32_t hold_start_time_A = 0;
    static uint32_t hold_start_time_C = 0;

    uint32_t now = millis();

    /* --- Hold A and C: Back to Home --- */
    if (M5.BtnC.pressedFor(HOLDING_BACK_TIME_MS) && M5.BtnA.pressedFor(HOLDING_BACK_TIME_MS)) {
        main_screen = SCREEN_HOME;
        audio_running = false;
        audio_set_running(audio_running);
        lvgl_set_menu_mode(main_screen, main_sub_mode);
        has_changed = true;
        while ((M5.BtnC.pressedFor(HOLDING_BACK_TIME_MS) || M5.BtnA.pressedFor(HOLDING_BACK_TIME_MS))) {
            M5.update(); M5.delay(10);
        }
        return;
    }

    /* --- Button B: Toggle Play/Pause --- */
    if (M5.BtnB.wasPressed()) {
        audio_running = !audio_running;
        audio_set_running(audio_running);
        has_changed = true;
        lvgl_set_play_state(audio_running);
    }

    /* --- Button C: Skip to Next Track OR Increase Volume on Hold --- */
    if (M5.BtnC.pressedFor(HOLDING_TIME_MS) && !M5.BtnA.isPressed()) {
        is_holding_C = true;
    }

    if (M5.BtnC.wasReleased()) {
        if (!is_holding_C) {
            audio_next_request();
        }
        is_holding_C = false;
        has_changed = true;

        if (is_save_C) {
            is_save_C = false;
            save_configuration();
        }
    }

    if (is_holding_C && (now - hold_start_time_C >= CHANGE_VOL_INTERVAL_MS)) {
        hold_start_time_C = now;

        if (current_volume <= 250) {
            current_volume += 5;
            M5.Speaker.setVolume(current_volume);
            is_save_C = true;
        }
    }

    if (M5.BtnC.wasPressed()) {
        hold_start_time_C = now;
    }

    /* --- Button A: Hold to Decrease Volume --- */
    if (M5.BtnA.pressedFor(HOLDING_TIME_MS) && !M5.BtnC.isPressed()) {
        is_holding_A = true;
    }

    if (M5.BtnA.wasReleased()) {
        if (!is_holding_A) {
            audio_prev_request();
        }
        is_holding_A = false;
        has_changed = true;

        if (is_save_A) {
            is_save_A = false;
            save_configuration();
        }
    }

    if (is_holding_A && (now - hold_start_time_A >= CHANGE_VOL_INTERVAL_MS)) {
        hold_start_time_A = now;

        if (current_volume >= 5) {
            current_volume -= 5;
            M5.Speaker.setVolume(current_volume);
            is_save_A = true;
        }
    }

    if (M5.BtnA.wasPressed()) {
        hold_start_time_A = now;
    }
}

/*!
 * @brief  Handle smile menu process
 */
static void smile_menu_loop(void) {
    static bool auto_change = true;

    /* --- Hold A and C: Back to Home --- */
    if (M5.BtnC.pressedFor(HOLDING_BACK_TIME_MS) && M5.BtnA.pressedFor(HOLDING_BACK_TIME_MS)) {
        main_screen = SCREEN_HOME;
        lvgl_set_menu_mode(main_screen, main_sub_mode);
        has_changed = true;
        audio_set_smile_mode(false);
        while ((M5.BtnC.pressedFor(HOLDING_BACK_TIME_MS) || M5.BtnA.pressedFor(HOLDING_BACK_TIME_MS))) {
            M5.update(); M5.delay(10);
        }
        return;
    }

    /* --- Button A: Prev image --- */
    if (M5.BtnA.wasReleased()) {
        lvgl_change_prev_smile();
        last_time_change_smile_ms = millis();
    }

    /* --- Button B: Toggle auto change --- */
    if (M5.BtnB.wasPressed()) {
        auto_change = !auto_change;
    }

    /* --- Button C: Next image --- */
    if (M5.BtnC.wasReleased()) {
        lvgl_change_next_smile();
        last_time_change_smile_ms = millis();
    }

    if (!auto_change) {
        return;
    }

    /* Change images automatically */
    uint32_t elapsed = millis() - last_time_change_smile_ms;
    if (elapsed >= 10000) {
        last_time_change_smile_ms = millis();
        lvgl_change_next_smile();
    }
}

/*!
 * @brief  Control menu process
 */
static void control_loop(void) {
    static uint32_t last_time_update_ms;
    uint8_t battery;
    uint32_t elapsed;

    switch (main_screen) {
        case SCREEN_PLAY_MUSIC:
            sdcard_menu_loop();
            last_active_ms = millis();
            break;

        case SCREEN_SMILE:
            smile_menu_loop();
            last_active_ms = millis();
            break;

        case SCREEN_HOME:
        default:
            home_menu_loop();
            last_time_change_smile_ms = millis();
            break;
    }

    elapsed = millis() - last_active_ms;
    if (elapsed > 10000) {
        last_active_ms = millis();
        M5.Power.powerOff();
    }

    /* Update system informations */
    elapsed = millis() - last_time_update_ms;
    if ((elapsed < 10000) && (!has_changed)) {
        return;
    }

    has_changed = false;
    last_time_update_ms = millis();
    lvgl_set_battery(M5.Power.getBatteryLevel());
}
