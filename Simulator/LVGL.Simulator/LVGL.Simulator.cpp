#include <stdint.h>
/*
 * PROJECT:   LVGL PC Simulator using Visual Studio
 * FILE:      LVGL.Simulator.cpp
 * PURPOSE:   Implementation for LVGL ported to Windows Desktop
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include <Windows.h>

#include "resource.h"

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
#endif

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/win32drv/win32drv.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

#include <stdio.h>

bool single_display_mode_initialization()
{
    if (!lv_win32_init(
        GetModuleHandleW(NULL),
        SW_SHOW,
        480,
        800,
        LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_LVGL))))
    {
        return false;
    }

    lv_win32_add_all_input_devices_to_group(NULL);

    return true;
}

#include <process.h>

HANDLE g_window_mutex = NULL;
bool g_initialization_status = false;

#define LVGL_SIMULATOR_MAXIMUM_DISPLAYS 16
HWND g_display_window_handles[LVGL_SIMULATOR_MAXIMUM_DISPLAYS];

unsigned int __stdcall lv_win32_window_thread_entrypoint(
    void* raw_parameter)
{
    size_t display_id = *(size_t*)(raw_parameter);

    HINSTANCE instance_handle = GetModuleHandleW(NULL);
    int show_window_mode = SW_SHOW;
    HICON icon_handle = LoadIconW(instance_handle, MAKEINTRESOURCE(IDI_LVGL));
    lv_coord_t hor_res = 800;
    lv_coord_t ver_res = 450;

    wchar_t window_title[256];
    memset(window_title, 0, sizeof(window_title));
    _snwprintf(
        window_title,
        256,
        L"LVGL Simulator for Windows Desktop (Display %d)",
        display_id);

    g_display_window_handles[display_id] = lv_win32_create_display_window(
        window_title,
        hor_res,
        ver_res,
        instance_handle,
        icon_handle,
        show_window_mode);
    if (!g_display_window_handles[display_id])
    {
        return 0;
    }

    g_initialization_status = true;

    SetEvent(g_window_mutex);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    lv_win32_quit_signal = true;

    return 0;
}

bool multiple_display_mode_initialization()
{
    if (!lv_win32_init_window_class())
    {
        return false;
    }

    for (size_t i = 0; i < LVGL_SIMULATOR_MAXIMUM_DISPLAYS; ++i)
    {
        g_initialization_status = false;

        g_window_mutex = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);

        _beginthreadex(
            NULL,
            0,
            lv_win32_window_thread_entrypoint,
            &i,
            0,
            NULL);

        WaitForSingleObjectEx(g_window_mutex, INFINITE, FALSE);

        CloseHandle(g_window_mutex);

        if (!g_initialization_status)
        {
            return false;
        }
    }

    lv_win32_window_context_t* context = (lv_win32_window_context_t*)(
        lv_win32_get_window_context(g_display_window_handles[0]));
    if (context)
    {
        lv_win32_pointer_device_object = context->mouse_device_object;
        lv_win32_keypad_device_object = context->keyboard_device_object;
        lv_win32_encoder_device_object = context->mousewheel_device_object;
    }

    lv_win32_add_all_input_devices_to_group(NULL);

    return true;
}

//static lv_obj_t* label;
//
//static void slider_event_cb(lv_event_t* e)
//{
//    lv_obj_t* slider = lv_event_get_target(e);
//
//    /*Refresh the text*/
//    lv_label_set_text_fmt(label, "%d", lv_slider_get_value(slider));
//    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
//}

void create_home_screen();
// ================= RETRODEX EXPANDABLE MENU =================

static lv_obj_t* simulator_menu_panel = nullptr;
static bool simulator_menu_open = false;


static void simulator_menu_animation(void* obj, int32_t x)
{
    lv_obj_set_x((lv_obj_t*)obj, x);

    if (x >= 480 && !simulator_menu_open)
    {
        lv_obj_add_flag(
            (lv_obj_t*)obj, LV_OBJ_FLAG_HIDDEN);
    }
}


static void simulator_menu_toggle(lv_event_t* e)
{
    if (simulator_menu_panel == nullptr) return;

    simulator_menu_open = !simulator_menu_open;

    if (simulator_menu_open)
    {
        lv_obj_clear_flag(
            simulator_menu_panel, LV_OBJ_FLAG_HIDDEN);
    }

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, simulator_menu_panel);
    lv_anim_set_exec_cb(&anim, simulator_menu_animation);

    lv_anim_set_values(
        &anim,
        simulator_menu_open ? 480 : 250,
        simulator_menu_open ? 250 : 480
    );

    lv_anim_set_time(&anim, 220);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}


static void simulator_menu_back(lv_event_t* e)
{
    lv_obj_clean(lv_scr_act());
    create_home_screen();
}

static void simulator_menu_option(lv_event_t* e)
{
    const char* option =
        (const char*)lv_event_get_user_data(e);

    lv_obj_clean(lv_scr_act());

    lv_obj_t* screen = lv_scr_act();

    lv_obj_set_style_bg_color(
        screen, lv_color_hex(0x212121), 0);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, option);
    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(
        title, &lv_font_montserrat_30, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 100);

    lv_obj_t* description = lv_label_create(screen);

    if (strcmp(option, "PLAY") == 0)
        lv_label_set_text(description, "GAMEPLAY FEATURE - IN DEVELOPMENT");
    else if (strcmp(option, "EXCHANGE") == 0)
        lv_label_set_text(description, "EXCHANGE CARDS WITH NEARBY DEVICES");
    else
        lv_label_set_text(description, "SD CARD VIDEO PLAYER - EXPERIMENTAL");

    lv_obj_set_style_text_color(
        description, lv_color_hex(0xB4B4B4), 0);
    lv_obj_align(description, LV_ALIGN_TOP_MID, 0, 160);

    lv_obj_t* back = lv_btn_create(screen);
    lv_obj_set_size(back, 160, 50);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -45);

    lv_obj_t* back_label = lv_label_create(back);
    lv_label_set_text(back_label, "BACK");
    lv_obj_center(back_label);

    lv_obj_add_event_cb(
        back, simulator_menu_back, LV_EVENT_CLICKED, NULL);
}

static void simulator_add_menu_option(
    lv_obj_t* parent,
    const char* text,
    int y)
{
    lv_obj_t* button = lv_btn_create(parent);

    lv_obj_set_size(button, 180, 55);
    lv_obj_set_pos(button, 20, y);

    lv_obj_set_style_bg_color(
        button, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_radius(button, 4, 0);

    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    lv_obj_add_event_cb(
        button,
        simulator_menu_option,
        LV_EVENT_CLICKED,
        (void*)text);
}

static void simulator_create_menu(lv_obj_t* screen)
{
    simulator_menu_open = false;

    // Sliding panel
    simulator_menu_panel = lv_obj_create(screen);
    lv_obj_set_size(simulator_menu_panel, 220, 260);
    lv_obj_set_pos(simulator_menu_panel, 480, 510);

lv_obj_add_flag(simulator_menu_panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_color(
        simulator_menu_panel, lv_color_hex(0x181818), 0);
    lv_obj_set_style_bg_opa(
        simulator_menu_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(
        simulator_menu_panel, 0, 0);
    lv_obj_set_style_radius(
        simulator_menu_panel, 8, 0);

    simulator_add_menu_option(simulator_menu_panel, "PLAY", 15);
    simulator_add_menu_option(simulator_menu_panel, "EXCHANGE", 90);
    simulator_add_menu_option(simulator_menu_panel, "VIDEO PLAY", 165);

    // Menu handle
    lv_obj_t* handle = lv_btn_create(screen);
    lv_obj_set_size(handle, 65, 65);
    lv_obj_set_pos(handle, 415, 720);

    lv_obj_set_style_bg_color(
        handle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(handle, 4, 0);

    lv_obj_t* handle_label = lv_label_create(handle);
    lv_label_set_text(handle_label, "MENU");
    lv_obj_set_style_text_color(
        handle_label, lv_color_hex(0x111111), 0);
    lv_obj_center(handle_label);

    lv_obj_add_event_cb(
        handle,
        simulator_menu_toggle,
        LV_EVENT_CLICKED,
        NULL);
}

// ================= END EXPANDABLE MENU =================

static int selected_card_id = 1;

void create_card_details_screen();

void card_click_event(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        selected_card_id =
            (int)(intptr_t)lv_event_get_user_data(e);

        lv_obj_clean(lv_scr_act());
        create_card_details_screen();
    }
}

void create_home_screen()
{
    lv_obj_t* screen = lv_scr_act();

    // Background
    lv_obj_set_style_bg_color(
        screen, lv_color_hex(0x212121), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "RETRODEX");
    lv_obj_set_style_text_font(
        title, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 20);

    // Subtitle
    lv_obj_t* subtitle = lv_label_create(screen);
    lv_label_set_text(
        subtitle, "YOUR DIGITAL CARD COLLECTION");
    lv_obj_set_style_text_font(
        subtitle, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(
        subtitle, lv_color_hex(0xB4B4B4), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_LEFT, 22, 62);

    // Scrollable gallery
    lv_obj_t* gallery = lv_obj_create(screen);
    lv_obj_set_size(gallery, 440, 620);
    lv_obj_set_pos(gallery, 20, 110);

    lv_obj_set_style_bg_opa(gallery, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gallery, 0, 0);
    lv_obj_set_style_pad_all(gallery, 0, 0);

    lv_obj_set_scroll_dir(gallery, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(gallery, LV_SCROLLBAR_MODE_AUTO);

    // Card placeholders
    for (int i = 0; i < 6; i++)
    {
        int column = i % 2;
        int row = i / 2;

        lv_obj_t* card = lv_obj_create(gallery);

        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

        // Normal size: 100%
        lv_obj_set_style_transform_zoom(
            card, 256, LV_STATE_DEFAULT);

        // Pressed size: approximately 109%
        lv_obj_set_style_transform_zoom(
            card, 280, LV_STATE_PRESSED);

        // Open the selected card
        lv_obj_add_event_cb(
            card,
            card_click_event,
            LV_EVENT_CLICKED,
            (void*)(intptr_t)(i + 1));

        lv_obj_set_size(card, 190, 270);
        lv_obj_set_pos(
            card, 20 + column * 210, 10 + row * 290);

        lv_obj_set_style_bg_color(
            card, lv_color_hex(0x303030), 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_radius(card, 6, 0);

        lv_obj_t* card_name = lv_label_create(card);
        lv_label_set_text_fmt(
            card_name, "CARD %03d", i + 1);
        lv_obj_set_style_text_color(
            card_name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(
            card_name, &lv_font_montserrat_16, 0);
        lv_obj_align(
            card_name, LV_ALIGN_TOP_LEFT, 10, 10);

        lv_obj_t* rarity = lv_label_create(card);
        lv_label_set_text(rarity, "COMMON");
        lv_obj_set_style_text_color(
            rarity, lv_color_hex(0xB4B4B4), 0);
        lv_obj_align(
            rarity, LV_ALIGN_BOTTOM_LEFT, 10, -10);

                lv_obj_add_flag(card_name, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_flag(rarity, LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    simulator_create_menu(screen);
}

void back_button_event(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        lv_obj_clean(lv_scr_act());
        create_home_screen();
    }
}

void close_popup_event(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        lv_obj_t* overlay =
            (lv_obj_t*)lv_event_get_user_data(e);

        lv_obj_del(overlay);
    }
}

void show_device_discovery()
{
    // Dark overlay
    lv_obj_t* overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, 480, 800);
    lv_obj_set_pos(overlay, 0, 0);

    lv_obj_set_style_bg_color(
        overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(
        overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);

    // Discovery popup
    lv_obj_t* popup = lv_obj_create(overlay);
    lv_obj_set_size(popup, 400, 300);
    lv_obj_center(popup);

    lv_obj_set_style_bg_color(
        popup, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_width(popup, 0, 0);
    lv_obj_set_style_radius(popup, 6, 0);

    // Heading
    lv_obj_t* heading = lv_label_create(popup);
    lv_label_set_text(heading, "DEVICE DISCOVERY");
    lv_obj_set_style_text_color(
        heading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(
        heading, &lv_font_montserrat_20, 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 25);

    // Discovery status
    lv_obj_t* status = lv_label_create(popup);
    lv_label_set_text(
        status, "Searching for nearby devices...");
    lv_obj_set_style_text_color(
        status, lv_color_hex(0xB4B4B4), 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 0);

    // Close button
    lv_obj_t* close_btn = lv_btn_create(popup);
    lv_obj_set_size(close_btn, 160, 45);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "CLOSE");
    lv_obj_center(close_label);

    lv_obj_add_event_cb(
        close_btn,
        close_popup_event,
        LV_EVENT_CLICKED,
        overlay);
}

void exchange_button_event(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        show_device_discovery();
    }
}

void create_card_details_screen()
{
    lv_obj_t* screen = lv_scr_act();

    // Background
    lv_obj_set_style_bg_color(
        screen, lv_color_hex(0x212121), 0);

    // Card artwork placeholder
    lv_obj_t* artwork = lv_obj_create(screen);
    lv_obj_set_size(artwork, 260, 300);
    lv_obj_align(artwork, LV_ALIGN_TOP_MID, 0, 35);

    lv_obj_set_style_bg_color(
        artwork, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_width(artwork, 0, 0);
    lv_obj_set_style_radius(artwork, 4, 0);

    // Card name
    lv_obj_t* name = lv_label_create(screen);
    lv_label_set_text_fmt(
        name, "CARD %03d", selected_card_id);
    lv_obj_set_style_text_color(
        name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(
        name, &lv_font_montserrat_24, 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 355);

    // Card ID
    lv_obj_t* id = lv_label_create(screen);
    lv_label_set_text_fmt(
        id, "ID: RTX-%03d", selected_card_id);
    lv_obj_set_style_text_color(
        id, lv_color_hex(0xB4B4B4), 0);
    lv_obj_align(id, LV_ALIGN_TOP_MID, 0, 395);

    // Rarity
    lv_obj_t* rarity = lv_label_create(screen);
    lv_label_set_text(
        rarity,
        selected_card_id == 1 ? "RARITY: RARE" : "RARITY: COMMON");
    lv_obj_set_style_text_color(
        rarity, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(rarity, LV_ALIGN_TOP_MID, 0, 430);

    // Description
    lv_obj_t* description = lv_label_create(screen);
    lv_label_set_text(
        description,
        "A rare collectible card from RetroDex.");
    lv_obj_set_style_text_color(
        description, lv_color_hex(0xB4B4B4), 0);
    lv_obj_set_width(description, 400);
    lv_obj_set_style_text_align(
        description, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(
        description, LV_LABEL_LONG_WRAP);
    lv_obj_align(description, LV_ALIGN_TOP_MID, 0, 475);


    // Back button - directly below description
    lv_obj_t* back = lv_btn_create(screen);
    lv_obj_set_size(back, 180, 50);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 25, 555);

    lv_obj_t* back_label = lv_label_create(back);
    lv_label_set_text(back_label, "BACK");
    lv_obj_center(back_label);

    lv_obj_add_event_cb(
        back, back_button_event, LV_EVENT_CLICKED, NULL);


    // Exchange button - beside Back
    lv_obj_t* exchange = lv_btn_create(screen);
    lv_obj_set_size(exchange, 180, 50);
    lv_obj_align(exchange, LV_ALIGN_TOP_RIGHT, -25, 555);

    lv_obj_t* exchange_label = lv_label_create(exchange);
    lv_label_set_text(exchange_label, "EXCHANGE");
    lv_obj_center(exchange_label);

    lv_obj_add_event_cb(
        exchange, exchange_button_event, LV_EVENT_CLICKED, NULL);
}
int main()
{
    lv_init();

    if (!single_display_mode_initialization())
    {
        return -1;
    }

    /*if (!multiple_display_mode_initialization())
    {
        return -1;
    }
    else
    {
        for (size_t i = 0; i < LVGL_SIMULATOR_MAXIMUM_DISPLAYS; ++i)
        {
            lv_win32_window_context_t* context = (lv_win32_window_context_t*)(
                lv_win32_get_window_context(g_display_window_handles[i]));
            if (context)
            {
                lv_disp_set_default(context->display_device_object);
                switch (i)
                {
                case 0:
                    create_card_details_screen();
                    break;
                case 1:
                    lv_demo_benchmark();
                    break;
                case 2:
                    lv_example_style_1();
                    break;
                case 3:
                    lv_example_get_started_1();
                    break;
                case 4:
                    lv_example_anim_1();
                    break;
                case 5:
                    lv_example_style_2();
                    break;
                case 6:
                    lv_example_get_started_2();
                    break;
                case 7:
                    lv_example_anim_2();
                    break;
                case 8:
                    lv_example_style_3();
                    break;
                case 9:
                    lv_example_get_started_3();
                    break;
                case 10:
                    lv_example_anim_3();
                    break;
                case 11:
                    lv_example_style_4();
                    break;
                case 12:
                    lv_example_style_5();
                    break;
                case 13:
                    lv_example_style_6();
                    break;
                case 14:
                    lv_example_imgfont_1();
                    break;
                case 15:
                    lv_example_style_7();
                    break;
                default:
                    break;
                }
            }
        }
    }*/

    //lv_win32_window_context_t* context = (lv_win32_window_context_t*)(
    //    lv_win32_get_window_context(g_display_window_handles[1]));
    //if (context)
    //{
    //    lv_obj_t* scr = lv_disp_get_scr_act(context->display_device_object);

    //    /*Create a slider in the center of the display*/
    //    lv_obj_t* slider = lv_slider_create(scr);
    //    lv_obj_set_width(slider, 200);                          /*Set the width*/
    //    lv_obj_center(slider);                                  /*Align to the center of the parent (screen)*/
    //    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);     /*Assign an event function*/

    //    /*Create a label above the slider*/
    //    label = lv_label_create(scr);
    //    lv_label_set_text(label, "0");
    //    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
    //}

    /*
     * Demos, benchmarks, and tests.
     *
     * Uncomment any one (and only one) of the functions below to run that
     * item.
     */

    // ----------------------------------
    // my freetype application
    // ----------------------------------

    ///*Init freetype library
    // *Cache max 64 faces and 1 size*/
    //lv_freetype_init(64, 1, 0);

    ///*Create a font*/
    //static lv_ft_info_t info;
    //info.name = "./lvgl/src/extra/libs/freetype/arial.ttf";
    //info.weight = 36;
    //info.style = FT_FONT_STYLE_NORMAL;
    //lv_ft_font_init(&info);

    ///*Create style with the new font*/
    //static lv_style_t style;
    //lv_style_init(&style);
    //lv_style_set_text_font(&style, info.font);

    ///*Create a label with the new style*/
    //lv_obj_t* label = lv_label_create(lv_scr_act());
    //lv_obj_add_style(label, &style, 0);
    //lv_label_set_text(label, "FreeType Arial Test");

    // ----------------------------------
    // my Win32 filesystem driver application
    // ----------------------------------

    /*::lv_fs_win32_init();

    lv_fs_dir_t d;
    if (lv_fs_dir_open(&d, "/") == LV_FS_RES_OK)
    {
        char b[MAX_PATH];
        memset(b, 0, MAX_PATH);
        while (lv_fs_dir_read(&d, b) == LV_FS_RES_OK)
        {
            printf("%s\n", b);
        }

        lv_fs_dir_close(&d);
    }*/

    // ----------------------------------
    // Demos from lv_examples
    // ----------------------------------

create_card_details_screen();           // ok
    //lv_demo_benchmark();
    // lv_demo_keypad_encoder();    // ok
    // lv_demo_music();             // removed from repository
    // lv_demo_printer();           // removed from repository
    // lv_demo_stress();            // ok

    // ----------------------------------
    // LVGL examples
    // ----------------------------------

    /*
     * There are many examples of individual widgets found under the
     * lvgl\exampless directory.  Here are a few sample test functions.
     * Look in that directory to find all the rest.
     */

    // lv_ex_get_started_1();
    // lv_ex_get_started_2();
    // lv_ex_get_started_3();

    // lv_example_flex_1();
    // lv_example_flex_2();
    // lv_example_flex_3();
    // lv_example_flex_4();
    // lv_example_flex_5();
    // lv_example_flex_6();        // ok

    // lv_example_grid_1();
    // lv_example_grid_2();
    // lv_example_grid_3();
    // lv_example_grid_4();
    // lv_example_grid_5();
    // lv_example_grid_6();

    // lv_port_disp_template();
    // lv_port_fs_template();
    // lv_port_indev_template();

    // lv_example_scroll_1();
    // lv_example_scroll_2();
    // lv_example_scroll_3();

    // lv_example_style_1();
    // lv_example_style_2();
    // lv_example_style_3();
    // lv_example_style_4();        // ok
    // lv_example_style_6();        // file has no source code
    // lv_example_style_7();
    // lv_example_style_8();
    // lv_example_style_9();
    // lv_example_style_10();
    // lv_example_style_11();       // ok

    // ----------------------------------
    // LVGL widgets examples
    // ----------------------------------

    // lv_example_arc_1();
    // lv_example_arc_2();

    // lv_example_bar_1();          // ok
    // lv_example_bar_2();
    // lv_example_bar_3();
    // lv_example_bar_4();
    // lv_example_bar_5();
    // lv_example_bar_6();          // issues

    // lv_example_btn_1();
    // lv_example_btn_2();
    // lv_example_btn_3();

    // lv_example_btnmatrix_1();
    // lv_example_btnmatrix_2();
    // lv_example_btnmatrix_3();

    // lv_example_calendar_1();

    // lv_example_canvas_1();
    // lv_example_canvas_2();

    // lv_example_chart_1();        // ok
    // lv_example_chart_2();        // ok
    // lv_example_chart_3();        // ok
    // lv_example_chart_4();        // ok
    // lv_example_chart_5();        // ok
    // lv_example_chart_6();        // ok

    // lv_example_checkbox_1();

    // lv_example_colorwheel_1();   // ok

    // lv_example_dropdown_1();
    // lv_example_dropdown_2();
    // lv_example_dropdown_3();

    // lv_example_img_1();
    // lv_example_img_2();
    // lv_example_img_3();
    // lv_example_img_4();         // ok

    // lv_example_imgbtn_1();

    // lv_example_keyboard_1();    // ok

    // lv_example_label_1();
    // lv_example_label_2();       // ok

    // lv_example_led_1();

    // lv_example_line_1();

    // lv_example_list_1();

    // lv_example_meter_1();
    // lv_example_meter_2();
    // lv_example_meter_3();
    // lv_example_meter_4();       // ok

    // lv_example_msgbox_1();

    // lv_example_obj_1();         // ok

    // lv_example_roller_1();
    // lv_example_roller_2();      // ok

    // lv_example_slider_1();      // ok
    // lv_example_slider_2();      // issues
    // lv_example_slider_3();      // issues

    // lv_example_spinbox_1();

    // lv_example_spinner_1();     // ok

    // lv_example_switch_1();      // ok

    // lv_example_table_1();
    // lv_example_table_2();       // ok

    // lv_example_tabview_1();

    // lv_example_textarea_1();    // ok
    // lv_example_textarea_2();
    // lv_example_textarea_3();    // ok, but not all button have functions

    // lv_example_tileview_1();    // ok

    // lv_example_win_1();         // ok

    // ----------------------------------
    // Task handler loop
    // ----------------------------------

    while (!lv_win32_quit_signal)
    {
        lv_task_handler();
        Sleep(1);
    }

    return 0;
}
