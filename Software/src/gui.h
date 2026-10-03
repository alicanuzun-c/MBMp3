#pragma once

#include <lvgl.h>

extern "C" {
LV_FONT_DECLARE(lv_font_turkish_14);
}

enum GuiTab : uint32_t {
    GuiTabPlay = 0,
    GuiTabFolder = 1,
    GuiTabSettings = 2
};

// Ekrani, dokunmatigi ve LVGL'yi baslatir, EEZ arayuzunu ve sekmeleri olusturur.
void guiBegin();

// LVGL gorevini baslatir; bundan sonra LVGL'ye sadece bu gorevden dokunulur.
void guiStartTask();

GuiTab guiActiveTab();

// Ust bardaki ortadaki yazi.
void guiSetStatus(const char *text);

// Ayni yaziyi tekrar atayip gereksiz yeniden cizim yapmamak icin.
void guiSetLabelTextIfChanged(lv_obj_t *label, const char *text);
