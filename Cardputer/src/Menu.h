#ifndef MENU_H
#define MENU_H
#include <M5Menu.h>
M5Menu::MenuTheme theme{
.background_color = 0x211a,
.border_color = 0x2c9f,
.selection_color = 0x06e0,
.value_color = WHITE,
.item_height = 23,
.item_window = 5,
.bool_true_color = GREEN,
.bool_false_color = RED,
.bool_true_string = "True",
.bool_false_string = "False",
.font = &fonts::FreeSans12pt7b};

uint8_t special = 0;
uint8_t current_resolution = 2;
uint8_t jpeg_quality = 12;
String resolutions[] = {
"96x96",
"160x120",
"176x144",
"240x176",
"240x240",
"320x240",
"400x296",
"480x320",
"640x480",
"800x600",
"1024x768",
"1280x720",
"1280x1024",
"1600x1200",
"1920x1080",
"720x1280",
"864x1536",
"2048x1536",
"2560x1440",
"2560x1600",
"1080x1920",
"2560x1920",
};

extern void apply_settings();

M5Menu::MenuItem main_menu_items[] = {
    {
        "Apply Settings",
        apply_settings
    },
    {
        "Resolution",
        &current_resolution,
        resolutions
    },
    {
        "JPEG Quality",
        &jpeg_quality,
        1,
        0,
        63
    },
    {
        "Special",
        &special,
        1,
        0,
        10
    }
};
M5Menu::Menu main_menu(0,main_menu_items);
#endif