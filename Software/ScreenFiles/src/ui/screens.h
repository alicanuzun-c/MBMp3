#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_SCREEN1 = 1,
    SCREEN_ID_SCREEN2 = 2,
    _SCREEN_ID_LAST = 2
};

typedef struct _objects_t {
    lv_obj_t *screen1;
    lv_obj_t *screen2;
    lv_obj_t *goscc1;
    lv_obj_t *butonlabel1;
    lv_obj_t *goscc2;
    lv_obj_t *butonlabel2;
    lv_obj_t *goscc1_1;
    lv_obj_t *butonlabel1_1;
    lv_obj_t *goscc2_1;
    lv_obj_t *butonlabel2_1;
} objects_t;

extern objects_t objects;

void create_screen_screen1();
void tick_screen_screen1();

void create_screen_screen2();
void tick_screen_screen2();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/