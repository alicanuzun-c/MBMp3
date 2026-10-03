#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

static const char *screen_names[] = { "sc_play" };
static const char *object_names[] = { "sc_play", "nav_top", "charge", "current_screen", "time", "nav_bottom", "button_play", "button_folder", "button_settings", "tab_view", "tab_view_bar", "tab_play", "tab_folder", "folder_container", "tab_settings" };

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

static void event_handler_cb_sc_play_button_play(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 6, 0, e);
    }
}

static void event_handler_cb_sc_play_button_folder(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 7, 0, e);
    }
}

static void event_handler_cb_sc_play_button_settings(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 8, 0, e);
    }
}

//
// Screens
//

void create_screen_sc_play() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.sc_play = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x992ec0), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // nav_top
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.nav_top = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 40);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // charge
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.charge = obj;
                    lv_obj_set_pos(obj, 8, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text_static(obj, "%98");
                }
                {
                    // current_screen
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.current_screen = obj;
                    lv_obj_set_pos(obj, 85, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text_static(obj, "Play Page");
                }
                {
                    // time
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.time = obj;
                    lv_obj_set_pos(obj, 196, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text_static(obj, "19:23");
                }
            }
        }
        {
            // nav_bottom
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.nav_bottom = obj;
            lv_obj_set_pos(obj, 0, 250);
            lv_obj_set_size(obj, 240, 70);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // button_play
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.button_play = obj;
                    lv_obj_set_pos(obj, 8, 11);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_play, NULL);
                    lv_obj_add_event_cb(obj, event_handler_cb_sc_play_button_play, LV_EVENT_ALL, flowState);
                }
                {
                    // button_folder
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.button_folder = obj;
                    lv_obj_set_pos(obj, 96, 11);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_folder, NULL);
                    lv_obj_add_event_cb(obj, event_handler_cb_sc_play_button_folder, LV_EVENT_ALL, flowState);
                }
                {
                    // button_settings
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.button_settings = obj;
                    lv_obj_set_pos(obj, 181, 11);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_settings, NULL);
                    lv_obj_add_event_cb(obj, event_handler_cb_sc_play_button_settings, LV_EVENT_ALL, flowState);
                }
            }
        }
        {
            // tab_view
            lv_obj_t *obj = lv_tabview_create(parent_obj);
            objects.tab_view = obj;
            lv_obj_set_pos(obj, 0, 40);
            lv_obj_set_size(obj, 240, 221);
            lv_tabview_set_tab_bar_position(obj, LV_DIR_BOTTOM);
            lv_tabview_set_tab_bar_size(obj, 0);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_NONE);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_NONE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xdb3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // tab_view_bar
                    lv_obj_t *obj = lv_tabview_get_tab_bar(parent_obj);
                    objects.tab_view_bar = obj;
                }
                {
                    lv_obj_t *obj = lv_tabview_get_content(parent_obj);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // tab_play
                            lv_obj_t *obj = lv_tabview_add_tab(lv_obj_get_parent(parent_obj), "TAB 1");
                            objects.tab_play = obj;
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x992ec0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(obj, -6, -8);
                                    lv_obj_set_size(obj, 208, 200);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                }
                            }
                        }
                        {
                            // tab_folder
                            lv_obj_t *obj = lv_tabview_add_tab(lv_obj_get_parent(parent_obj), "TAB 2");
                            objects.tab_folder = obj;
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x992ec0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // folder_container
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.folder_container = obj;
                                    lv_obj_set_pos(obj, 3, -3);
                                    lv_obj_set_size(obj, 208, 182);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                }
                            }
                        }
                        {
                            // tab_settings
                            lv_obj_t *obj = lv_tabview_add_tab(lv_obj_get_parent(parent_obj), "TAB 3");
                            objects.tab_settings = obj;
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0x992ec0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(obj, 3, 13);
                                    lv_obj_set_size(obj, 181, 159);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_sc_play();
}

void tick_screen_sc_play() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_sc_play,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 1) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
//
//

void create_screens() {
    
    eez_flow_init_fonts(fonts, sizeof(fonts) / sizeof(ext_font_desc_t));

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    
    // Create screens
    create_screen_sc_play();
}