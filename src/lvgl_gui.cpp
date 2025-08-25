/*
 *  lvgl_gui.cpp
 *
 *  Created on: Aug 24, 2025
 */
/* https://docs.lvgl.io/latest/en/html/widgets/tabview.html#example */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <Arduino.h>
#include <M5Unified.h>
#include <lvgl.h>
#include "app_config.hpp"
#include "lvgl_gui.hpp"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/



/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];

static lv_obj_t *ui_bar_bat;
static lv_obj_t *ui_label_bat;

static lv_obj_t *ui_home_screen;
static lv_obj_t *ui_image_menu;
static lv_obj_t *ui_label_menu;

static lv_obj_t *ui_music_screen;
static lv_obj_t *ui_image_play_music;
static lv_obj_t *ui_label_song;

static lv_obj_t *ui_smile_screen;
static lv_obj_t *ui_image_smile;

static const lv_img_dsc_t *image_src_list[32];
static int smile_image_max = 0;
static int smile_index = 0;

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/

extern const lv_img_dsc_t ui_img_left_arrow_png;
extern const lv_img_dsc_t ui_img_music_png;
extern const lv_img_dsc_t ui_img_right_arrow;
extern const lv_img_dsc_t ui_img_sun_png;

extern const lv_img_dsc_t ui_img_pause_button_png;
extern const lv_img_dsc_t ui_img_play_button_png;
extern const lv_img_dsc_t ui_img_image_radio_png;

/* Image list */
extern const lv_img_dsc_t image1;
extern const lv_img_dsc_t image2;
extern const lv_img_dsc_t image3;
extern const lv_img_dsc_t image4;
extern const lv_img_dsc_t image5;
extern const lv_img_dsc_t image6;
extern const lv_img_dsc_t image7;
extern const lv_img_dsc_t image8;
extern const lv_img_dsc_t image9;
extern const lv_img_dsc_t image10;
extern const lv_img_dsc_t image11;
extern const lv_img_dsc_t image12;
extern const lv_img_dsc_t image13;
extern const lv_img_dsc_t image14;
extern const lv_img_dsc_t image15;
extern const lv_img_dsc_t image16;
extern const lv_img_dsc_t image17;
extern const lv_img_dsc_t image18;
extern const lv_img_dsc_t image19;
extern const lv_img_dsc_t image20;
extern const lv_img_dsc_t image21;

/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static void lvgl_task(void *arg);
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void lv_tick_inc_cb(void *data);
static void lvgl_tick_init(void);

static void lvgl_top_header_init(void);
static void lvgl_ui_init(void);

/******************************************************************************/

/**
 * @brief  Task for update lvgl
 */
static void lvgl_task(void *arg) {
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX / 10);

    /* Initialize the display */
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the (dummy) input device driver */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_NONE;
    lv_indev_drv_register(&indev_drv);

    /* Create UI */
    lvgl_ui_init();
    lvgl_top_header_init();    /* Should be called last */

    while (1) {
        lv_task_handler();    /* Let the GUI do its work */
        delay(LVGL_TICK_HANDLER);
    }
}

/**
 * @brief  Display flushing
 */
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    int32_t w = (area->x2 - area->x1 + 1);
    int32_t h = (area->y2 - area->y1 + 1);

    M5.Lcd.startWrite();
    M5.Lcd.setAddrWindow(area->x1, area->y1, w, h);
    M5.Lcd.pushPixels((uint16_t *) &color_p->full, (int32_t) (w * h), true);
    M5.Lcd.endWrite();

    lv_disp_flush_ready(disp);
}

/**
 * @brief  Increase tick for lvgl task
 */
static void lv_tick_inc_cb(void *data) {
    uint32_t tick_inc_period_ms = *((uint32_t *) data);
    lv_tick_inc(tick_inc_period_ms);
}

/**
 * @brief  Create timer to increase tick
 */
static void lvgl_tick_init(void) {
    static const uint32_t tick_inc_period_ms = LVGL_TICK_HANDLER;
    static esp_timer_create_args_t periodic_timer_args;

    periodic_timer_args.callback = lv_tick_inc_cb;
    periodic_timer_args.name = "tick";     /* name is optional, but may help identify the timer when debugging */
    periodic_timer_args.arg = (void *) &tick_inc_period_ms;
    periodic_timer_args.dispatch_method = ESP_TIMER_TASK;
    periodic_timer_args.skip_unhandled_events = true;

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    /* The timer has been created but is not running yet. Start the timer now */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, tick_inc_period_ms * 1000));
}

/******************************************************************************/

/**
 * @brief  Create all components for UI
 */
static void lvgl_ui_init(void) {
    lv_obj_t *label;
    lv_obj_t *image;
    lv_obj_t *main_scr = lv_scr_act();
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    lv_obj_clear_flag(main_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(main_scr, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(main_scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Home screen */
    ui_home_screen = lv_obj_create(main_scr);
    lv_obj_remove_style_all(ui_home_screen);
    lv_obj_set_width(ui_home_screen, 320);
    lv_obj_set_height(ui_home_screen, 212);
    lv_obj_set_x(ui_home_screen, 0);
    lv_obj_set_y(ui_home_screen, 28);
    lv_obj_clear_flag(ui_home_screen, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_home_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_home_screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_image_menu = lv_img_create(ui_home_screen);
    lv_img_set_src(ui_image_menu, &ui_img_music_png);
    lv_obj_set_width(ui_image_menu, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_image_menu, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_image_menu, 0);
    lv_obj_set_y(ui_image_menu, 50);
    lv_obj_set_align(ui_image_menu, LV_ALIGN_TOP_MID);
    lv_obj_add_flag(ui_image_menu, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_image_menu, LV_OBJ_FLAG_SCROLLABLE);

    ui_label_menu = lv_label_create(ui_home_screen);
    lv_obj_set_width(ui_label_menu, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_label_menu, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_label_menu, 0);
    lv_obj_set_y(ui_label_menu, 10);
    lv_obj_set_align(ui_label_menu, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_label_menu, "Play Music");
    lv_obj_set_style_text_color(ui_label_menu, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label_menu, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label_menu, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    image = lv_img_create(ui_home_screen);
    lv_img_set_src(image, &ui_img_left_arrow_png);
    lv_obj_set_width(image, LV_SIZE_CONTENT);
    lv_obj_set_height(image, LV_SIZE_CONTENT);
    lv_obj_set_x(image, 30);
    lv_obj_set_y(image, 100);
    lv_obj_add_flag(image, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    image = lv_img_create(ui_home_screen);
    lv_img_set_src(image, &ui_img_right_arrow);
    lv_obj_set_width(image, LV_SIZE_CONTENT);
    lv_obj_set_height(image, LV_SIZE_CONTENT);
    lv_obj_set_x(image, 258);
    lv_obj_set_y(image, 100);
    lv_obj_add_flag(image, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    /* Play Music */
    ui_music_screen = lv_obj_create(main_scr);
    lv_obj_remove_style_all(ui_music_screen);
    lv_obj_set_width(ui_music_screen, 320);
    lv_obj_set_height(ui_music_screen, 212);
    lv_obj_set_x(ui_music_screen, 0);
    lv_obj_set_y(ui_music_screen, 28);
    lv_obj_clear_flag(ui_music_screen, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_music_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_music_screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(ui_music_screen, LV_OBJ_FLAG_HIDDEN);

    image = lv_img_create(ui_music_screen);
    lv_img_set_src(image, &ui_img_image_radio_png);
    lv_obj_set_width(image, LV_SIZE_CONTENT);
    lv_obj_set_height(image, LV_SIZE_CONTENT);
    lv_obj_set_x(image, 0);
    lv_obj_set_y(image, 74);
    lv_obj_set_align(image, LV_ALIGN_TOP_MID);
    lv_obj_add_flag(image, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    image = lv_img_create(ui_music_screen);
    lv_img_set_src(image, &ui_img_left_arrow_png);
    lv_obj_set_width(image, LV_SIZE_CONTENT);
    lv_obj_set_height(image, LV_SIZE_CONTENT);
    lv_obj_set_x(image, 30);
    lv_obj_set_y(image, 120);
    lv_obj_add_flag(image, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    image = lv_img_create(ui_music_screen);
    lv_img_set_src(image, &ui_img_right_arrow);
    lv_obj_set_width(image, LV_SIZE_CONTENT);
    lv_obj_set_height(image, LV_SIZE_CONTENT);
    lv_obj_set_x(image, 258);
    lv_obj_set_y(image, 120);
    lv_obj_add_flag(image, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    ui_label_song = lv_label_create(ui_music_screen);
    lv_obj_set_width(ui_label_song, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_label_song, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_label_song, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_label_song, "Play Music");
    lv_obj_set_style_text_color(ui_label_song, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label_song, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label_song, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_image_play_music = lv_img_create(ui_music_screen);
    lv_img_set_src(ui_image_play_music, &ui_img_pause_button_png);
    lv_obj_set_width(ui_image_play_music, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_image_play_music, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_image_play_music, 0);
    lv_obj_set_y(ui_image_play_music, 30);
    lv_obj_set_align(ui_image_play_music, LV_ALIGN_TOP_MID);
    lv_obj_add_flag(ui_image_play_music, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_image_play_music, LV_OBJ_FLAG_SCROLLABLE);

    /* Smiles screen */
    ui_smile_screen = lv_obj_create(main_scr);
    lv_obj_remove_style_all(ui_smile_screen);
    lv_obj_set_width(ui_smile_screen, 320);
    lv_obj_set_height(ui_smile_screen, 212);
    lv_obj_set_x(ui_smile_screen, 0);
    lv_obj_set_y(ui_smile_screen, 28);
    lv_obj_clear_flag(ui_smile_screen, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_smile_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_smile_screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(ui_smile_screen, LV_OBJ_FLAG_HIDDEN);

    ui_image_smile = lv_img_create(ui_smile_screen);
    lv_obj_set_width(ui_image_smile, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_image_smile, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_image_smile, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_flag(ui_image_smile, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_image_smile, LV_OBJ_FLAG_SCROLLABLE);

    smile_index = 0;
    smile_image_max = 0;
    image_src_list[smile_image_max++] = &image1;
    image_src_list[smile_image_max++] = &image2;
    image_src_list[smile_image_max++] = &image3;
    image_src_list[smile_image_max++] = &image4;
    image_src_list[smile_image_max++] = &image5;
    image_src_list[smile_image_max++] = &image6;
    image_src_list[smile_image_max++] = &image7;
    image_src_list[smile_image_max++] = &image8;
    image_src_list[smile_image_max++] = &image9;
    image_src_list[smile_image_max++] = &image10;
    image_src_list[smile_image_max++] = &image11;
    image_src_list[smile_image_max++] = &image12;
    image_src_list[smile_image_max++] = &image13;
    image_src_list[smile_image_max++] = &image14;
    image_src_list[smile_image_max++] = &image15;
    image_src_list[smile_image_max++] = &image16;
    image_src_list[smile_image_max++] = &image17;
    image_src_list[smile_image_max++] = &image18;
    image_src_list[smile_image_max++] = &image19;
    image_src_list[smile_image_max++] = &image20;
    image_src_list[smile_image_max++] = &image21;
    lv_img_set_src(ui_image_smile, image_src_list[0]);
}

/**
 * @brief  Initialize header of screen
 */
static void lvgl_top_header_init(void) {
    lv_obj_t *main_scr = lv_scr_act();

    ui_bar_bat = lv_bar_create(main_scr);
    lv_bar_set_value(ui_bar_bat, 100, LV_ANIM_OFF);
    lv_obj_set_width(ui_bar_bat, 40);
    lv_obj_set_height(ui_bar_bat, 16);
    lv_obj_set_x(ui_bar_bat, -5);
    lv_obj_set_y(ui_bar_bat, 6);
    lv_obj_set_style_radius(ui_bar_bat, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_align(ui_bar_bat, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_style_bg_color(ui_bar_bat, lv_color_hex(0x72E086), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_bar_bat, 255, LV_PART_INDICATOR| LV_STATE_DEFAULT);

    ui_label_bat = lv_label_create(main_scr);
    lv_obj_set_width(ui_label_bat, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_label_bat, LV_SIZE_CONTENT);
    lv_label_set_text(ui_label_bat, "50 %");
    lv_obj_align_to(ui_label_bat, ui_bar_bat, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_set_style_text_color(ui_label_bat, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label_bat, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/******************************************************************************/

/**
 * @brief  Set battery percentage
 */
void lvgl_set_battery(uint8_t percentage) {
    char buff[16];
    sprintf(buff, "%d %%", percentage);
    if (percentage <= 25) {
        lv_obj_set_style_bg_color(ui_bar_bat, lv_color_hex(0xF60B0B), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    else {
        lv_obj_set_style_bg_color(ui_bar_bat, lv_color_hex(0x72E086), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }

    lv_label_set_text(ui_label_bat, buff);
    lv_bar_set_value(ui_bar_bat, percentage, LV_ANIM_OFF);
}

/**
 * @brief  Set battery percentage
 */
void lvgl_set_menu_mode(uint8_t mode, uint8_t sub_mode) {
    lv_obj_add_flag(ui_home_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_smile_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_music_screen, LV_OBJ_FLAG_HIDDEN);
    
    switch (mode) {
        case SCREEN_PLAY_MUSIC:
            lv_obj_clear_flag(ui_music_screen, LV_OBJ_FLAG_HIDDEN);
            break;

        case SCREEN_SMILE:
            lv_obj_clear_flag(ui_smile_screen, LV_OBJ_FLAG_HIDDEN);
            smile_index = 0;
            break;

        case SCREEN_HOME:
        default:
            lv_obj_clear_flag(ui_home_screen, LV_OBJ_FLAG_HIDDEN);
            if (sub_mode == HOME_PLAY_MUSIC) {
                lv_img_set_src(ui_image_menu, &ui_img_music_png);
                lv_label_set_text(ui_label_menu, "Play Music");
            }
            else {
                lv_img_set_src(ui_image_menu, &ui_img_sun_png);
                lv_label_set_text(ui_label_menu, "Smiles");
            }
            break;
    }
}

/**
 * @brief  Change image resource
 */
void lvgl_change_next_smile(void) {
    smile_index = (smile_index + 1) % smile_image_max;  /* smile_image_max is always > 0 */
    lv_img_set_src(ui_image_smile, image_src_list[smile_index]);
}

/**
 * @brief  Change image resource
 */
void lvgl_change_prev_smile(void) {
    smile_index = (smile_index + smile_image_max - 1) % smile_image_max;  /* smile_image_max is always > 0 */
    lv_img_set_src(ui_image_smile, image_src_list[smile_index]);
}

/**
 * @brief  Set display playing sate
 */
void lvgl_set_play_state(bool playing) {
    if (!playing) {
        lv_img_set_src(ui_image_play_music, &ui_img_play_button_png);
    }
    else {
        lv_img_set_src(ui_image_play_music, &ui_img_pause_button_png);
    }
}

/**
 * @brief  Set song name
 */
void lvgl_set_song_name(const char *name) {
    lv_label_set_text(ui_label_song, name);
}

/**
 * @brief  Initialize LVGL for gui
 */
void lvgl_gui_init(void) {
    lv_init();
    lvgl_tick_init();

    /* Create lvgl task */
    xTaskCreatePinnedToCore(lvgl_task, "LVGL", 10240, NULL, 4, NULL, 1);
    delay(200);    /* Wait for lvgl is initialized */
}
