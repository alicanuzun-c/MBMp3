#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_SC_PLAY = 1,
    SCREEN_ID_SC_FOLDER = 2,
    SCREEN_ID_SC_SETTINGS = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *sc_play;
    lv_obj_t *sc_folder;
    lv_obj_t *sc_settings;
    lv_obj_t *button_play;
    lv_obj_t *button_folder;
    lv_obj_t *button_settings;
    lv_obj_t *button_play_1;
    lv_obj_t *button_folder_1;
    lv_obj_t *button_settings_1;
    lv_obj_t *folder_path_label;
    lv_obj_t *folder_list;
    lv_obj_t *button_play_2;
    lv_obj_t *button_folder_2;
    lv_obj_t *button_settings_2;
} objects_t;

extern objects_t objects;

void create_screen_sc_play();
void tick_screen_sc_play();

void create_screen_sc_folder();
void tick_screen_sc_folder();

void create_screen_sc_settings();
void tick_screen_sc_settings();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/