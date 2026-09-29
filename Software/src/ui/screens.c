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

static const char *screen_names[] = { "sc_play", "sc_folder", "sc_settings" };
static const char *object_names[] = { "sc_play", "sc_folder", "sc_settings", "main_settings", "main_folder", "main_play", "main_settings_3", "main_folder_3", "main_play_3", "main_settings_4", "main_folder_4", "main_play_4" };

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

static void event_handler_cb_sc_play_main_settings(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_sc_play_main_folder(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

static void event_handler_cb_sc_play_main_play(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 3, 0, e);
    }
}

static void event_handler_cb_sc_folder_main_settings_3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_sc_folder_main_folder_3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

static void event_handler_cb_sc_folder_main_play_3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 3, 0, e);
    }
}

static void event_handler_cb_sc_settings_main_settings_4(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_sc_settings_main_folder_4(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

static void event_handler_cb_sc_settings_main_play_4(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 3, 0, e);
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
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x227955), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // main_settings
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_settings = obj;
            lv_obj_set_pos(obj, 190, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_settings, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_play_main_settings, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // main_folder
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_folder = obj;
            lv_obj_set_pos(obj, 104, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_folder, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_play_main_folder, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // main_play
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_play = obj;
            lv_obj_set_pos(obj, 23, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_play, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_play_main_play, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 80, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "PLAY PAGE");
        }
    }
    
    tick_screen_sc_play();
}

void tick_screen_sc_play() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
}

void create_screen_sc_folder() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.sc_folder = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xa31818), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // main_settings_3
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_settings_3 = obj;
            lv_obj_set_pos(obj, 190, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_settings, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_folder_main_settings_3, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // main_folder_3
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_folder_3 = obj;
            lv_obj_set_pos(obj, 104, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_folder, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_folder_main_folder_3, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // main_play_3
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_play_3 = obj;
            lv_obj_set_pos(obj, 23, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_play, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_folder_main_play_3, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 80, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "FOLDER PAGE");
        }
    }
    
    tick_screen_sc_folder();
}

void tick_screen_sc_folder() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
}

void create_screen_sc_settings() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.sc_settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x6e0a5a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // main_settings_4
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_settings_4 = obj;
            lv_obj_set_pos(obj, 190, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_settings, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_settings_main_settings_4, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // main_folder_4
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_folder_4 = obj;
            lv_obj_set_pos(obj, 104, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_folder, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_settings_main_folder_4, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // main_play_4
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.main_play_4 = obj;
            lv_obj_set_pos(obj, 23, 278);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 32);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_button_play, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_sc_settings_main_play_4, LV_EVENT_ALL, flowState);
            lv_obj_set_style_transform_scale_x(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 300, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 80, 144);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "SETTINGS PAGE");
        }
    }
    
    tick_screen_sc_settings();
}

void tick_screen_sc_settings() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_sc_play,
    tick_screen_sc_folder,
    tick_screen_sc_settings,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 3) {
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
    create_screen_sc_folder();
    create_screen_sc_settings();
}