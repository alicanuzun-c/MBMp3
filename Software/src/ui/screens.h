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
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *sc_play;
    lv_obj_t *nav_top;
    lv_obj_t *charge;
    lv_obj_t *current_screen;
    lv_obj_t *time;
    lv_obj_t *nav_bottom;
    lv_obj_t *button_play;
    lv_obj_t *button_folder;
    lv_obj_t *button_settings;
    lv_obj_t *tab_view;
    lv_obj_t *tab_view_bar;
    lv_obj_t *tab_play;
    lv_obj_t *tab_folder;
    lv_obj_t *folder_container;
    lv_obj_t *tab_settings;
} objects_t;

extern objects_t objects;

void create_screen_sc_play();
void tick_screen_sc_play();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/