#include <Arduino.h>
#include <stdint.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

Board *board = nullptr;

static lv_obj_t *menu_panel = nullptr;
static bool menu_open = false;

void create_home_screen();
void create_card_detail(int card_number);
void create_feature_screen(const char *feature);

static void menu_anim_exec(void *var, int32_t value) {
    lv_obj_set_x((lv_obj_t *)var, value);
}

static void toggle_menu(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || menu_panel == nullptr) {
        return;
    }

    menu_open = !menu_open;

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, menu_panel);
    lv_anim_set_exec_cb(&anim, menu_anim_exec);
    lv_anim_set_values(&anim, lv_obj_get_x(menu_panel),
                       menu_open ? 250 : 480);
    lv_anim_set_time(&anim, 220);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void card_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int number = (int)(intptr_t)lv_event_get_user_data(e);
        create_card_detail(number);
    }
}

static void back_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        create_home_screen();
    }
}

static void feature_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        const char *feature = (const char *)lv_event_get_user_data(e);
        create_feature_screen(feature);
    }
}

static void add_menu_option(lv_obj_t *parent, const char *name,
                            int y, const char *feature) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 180, 48);
    lv_obj_set_pos(button, 15, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x303030), 0);
    lv_obj_set_style_radius(button, 8, 0);

    lv_obj_add_event_cb(button, feature_event_cb,
                        LV_EVENT_CLICKED, (void *)feature);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
}

void create_home_screen() {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    menu_panel = nullptr;
    menu_open = false;

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "RETRODEX");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 18);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "YOUR DIGITAL CARD COLLECTION");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x999999), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_LEFT, 22, 57);

    for (int i = 0; i < 6; i++) {
        int column = i % 2;
        int row = i / 2;

        lv_obj_t *card = lv_obj_create(screen);
        lv_obj_set_size(card, 205, 165);
        lv_obj_set_pos(card, 20 + column * 225, 105 + row * 190);

        lv_obj_set_style_bg_color(card, lv_color_hex(0x202020), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(card, card_event_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)(i + 1));

        lv_obj_t *name = lv_label_create(card);
        lv_label_set_text_fmt(name, "CARD %03d", i + 1);
        lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 8, 8);

        lv_obj_t *rarity = lv_label_create(card);
        lv_label_set_text(rarity, "COMMON");
        lv_obj_set_style_text_color(rarity, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(rarity, &lv_font_montserrat_14, 0);
        lv_obj_align(rarity, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    }

    // Expandable menu panel, initially outside the right screen edge
    menu_panel = lv_obj_create(screen);
    lv_obj_set_size(menu_panel, 220, 260);
    lv_obj_set_pos(menu_panel, 480, 510);
    lv_obj_set_style_bg_color(menu_panel, lv_color_hex(0x181818), 0);
    lv_obj_set_style_bg_opa(menu_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(menu_panel, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(menu_panel, 1, 0);
    lv_obj_set_style_radius(menu_panel, 12, 0);
    lv_obj_clear_flag(menu_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *menu_title = lv_label_create(menu_panel);
    lv_label_set_text(menu_title, "MENU");
    lv_obj_set_style_text_color(menu_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(menu_title, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(menu_title, 15, 12);

    add_menu_option(menu_panel, "PLAY", 48, "PLAY");
    add_menu_option(menu_panel, "EXCHANGE", 108, "EXCHANGE");
    add_menu_option(menu_panel, "VIDEO PLAY", 168, "VIDEO PLAY");

    // Always-visible bottom-right menu handle
    lv_obj_t *handle = lv_btn_create(screen);
    lv_obj_set_size(handle, 65, 65);
    lv_obj_set_pos(handle, 415, 720);
    lv_obj_set_style_bg_color(handle, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_radius(handle, 10, 0);
    lv_obj_add_event_cb(handle, toggle_menu, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *handle_label = lv_label_create(handle);
    lv_label_set_text(handle_label, "MENU");
    lv_obj_set_style_text_color(handle_label, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(handle_label, &lv_font_montserrat_14, 0);
    lv_obj_center(handle_label);
}

void create_card_detail(int card_number) {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    menu_panel = nullptr;

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101010), 0);

    lv_obj_t *heading = lv_label_create(screen);
    lv_label_set_text(heading, "CARD DETAILS");
    lv_obj_set_style_text_font(heading, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(heading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 20, 25);

    lv_obj_t *name = lv_label_create(screen);
    lv_label_set_text_fmt(name, "CARD %03d", card_number);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 25, 130);

    lv_obj_t *rarity = lv_label_create(screen);
    lv_label_set_text(rarity, "RARITY: COMMON");
    lv_obj_set_style_text_font(rarity, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(rarity, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(rarity, LV_ALIGN_TOP_LEFT, 25, 185);

    lv_obj_t *back = lv_btn_create(screen);
    lv_obj_set_size(back, 150, 50);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_add_event_cb(back, back_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, "BACK");
    lv_obj_center(back_label);
}

void create_feature_screen(const char *feature) {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    menu_panel = nullptr;

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101010), 0);

    lv_obj_t *heading = lv_label_create(screen);
    lv_label_set_text(heading, feature);
    lv_obj_set_style_text_font(heading, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(heading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 20, 30);

    const char *message = "Feature screen ready.\nImplementation coming next.";
    if (strcmp(feature, "VIDEO PLAY") == 0) {
        message = "SD card video player.\nPlayback support is planned.";
    }

    lv_obj_t *description = lv_label_create(screen);
    lv_label_set_text(description, message);
    lv_obj_set_style_text_font(description, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(description, lv_color_hex(0xBBBBBB), 0);
    lv_obj_align(description, LV_ALIGN_TOP_LEFT, 25, 130);

    lv_obj_t *back = lv_btn_create(screen);
    lv_obj_set_size(back, 150, 50);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_add_event_cb(back, back_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, "BACK");
    lv_obj_center(back_label);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("RetroDex starting...");

    board = new Board();
    board->init();

    if (!board->begin()) {
        Serial.println("Board initialization failed!");
        return;
    }

    lvgl_port_init(board->getLCD(), board->getTouch());

    lvgl_port_lock(-1);
    create_home_screen();
    lvgl_port_unlock();

    Serial.println("RetroDex LVGL initialized!");
}

void loop() {
    delay(1000);
}
