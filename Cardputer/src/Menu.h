#ifndef MENU_H
#define MENU_H
#include <M5Menu.h>

String aec_options[] = {
    "Auto",
    "Very Fast (200)",
    "Fast (400)",
    "Normal (600)",
    "Slow (800)",
    "Night (1000)",
};
uint8_t aec_map[] = {0,42,85,127,170,212};

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

extern void apply_viewfinder_settings();
extern void apply_photo_settings();
extern void apply_recording_settings();
extern void apply_modeset();

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

struct localCameraSettings{
  uint8_t PX_FORMAT;
  uint8_t FR_SIZE;
  uint8_t FB_SIZE;
  uint8_t JPEG_QUALITY; //0 to 63
};

struct GlobalCameraSettings {
    int8_t BRIGHTNESS = 0;
    int8_t CONTRAST = 0;
    int8_t SATURATION = 0;
    int8_t SHARPNESS = 0;
    bool WB = true;
    int8_t WB_MODE = 0;
    bool AWB_GAIN = true;
    bool EXP_CTRL = true;
    int8_t AE_LEVEL = 0;
    uint8_t AEC_VALUE = static_cast<int8_t>(aec_map[3]);
    bool AEC2 = false;
    bool GAIN_CTRL = true;
    int8_t GAIN_CEILING = 0;
    bool LENS_CORR = true;
    bool MIRROR = false;
    bool FLIP = false;
    int8_t SPECIAL = 0;
};
GlobalCameraSettings global_settings = {};
localCameraSettings viewfinder_settings = {};
localCameraSettings photo_settings = {};
localCameraSettings recording_settings = {};

M5Menu::MenuItem global_settings_items[] = {
    {"Apply settings",apply_modeset},
    {"brightness",&global_settings.BRIGHTNESS,1,-2,2},
    {"contrast",&global_settings.CONTRAST,1,-2,2},
    {"saturation",&global_settings.SATURATION,1,-2,2},
    {"sharpness",&global_settings.SHARPNESS,1,-2,2},
    {"wb",&global_settings.WB},
    {"wb_mode",&global_settings.WB_MODE,1,0,4},
    {"awb_gain",&global_settings.AWB_GAIN},
    {"exp_ctrl",&global_settings.EXP_CTRL},
    {"ae_level",&global_settings.AE_LEVEL,1,-2,2},
    {"aec_value",&global_settings.AEC_VALUE,aec_options},
    {"aec2",&global_settings.AEC2},
    {"gain_ctrl",&global_settings.GAIN_CTRL},
    {"gain_ceiling",&global_settings.GAIN_CEILING},
    {"lens_corr",&global_settings.LENS_CORR},
    {"mirror",&global_settings.MIRROR},
    {"flip",&global_settings.FLIP},
    {"special",&global_settings.SPECIAL,1,0,6}
};
M5Menu::Menu global_menu_settings(4,global_settings_items);


M5Menu::MenuItem recording_settings_items[] = {
    {
        "Apply Settings",
        apply_recording_settings
    },
    {
        "Resolution",
        &recording_settings.FR_SIZE,
        resolutions
    },
    {
        "JPEG Quality",
        &recording_settings.JPEG_QUALITY,
        1,
        0,
        63
    },
    
};
M5Menu::Menu recording_menu_settings(3,recording_settings_items);


M5Menu::MenuItem photo_settings_items[] = {
    {
        "Apply Settings",
        apply_photo_settings
    },
    {
        "Resolution",
        &photo_settings.FR_SIZE,
        resolutions
    },
    {
        "JPEG Quality",
        &photo_settings.JPEG_QUALITY,
        1,
        0,
        63
    },
    
};
M5Menu::Menu photo_menu_settings(2,photo_settings_items);


M5Menu::MenuItem viewfinder_settings_items[] = {
    {
        "Apply Settings",
        apply_viewfinder_settings
    },
    {
        "Resolution",
        &viewfinder_settings.FR_SIZE,
        resolutions
    },
    {
        "JPEG Quality",
        &viewfinder_settings.JPEG_QUALITY,
        1,
        0,
        63
    },
    
};
M5Menu::Menu viewfinder_menu_settings(1,viewfinder_settings_items);
M5Menu::MenuItem main_menu_items[] = {
    {
        "Viewfinder settings",
        &viewfinder_menu_settings
    },
    {
        "Photo Settings",
        &photo_menu_settings
    },
    {
        "Recording Settings",
        &recording_menu_settings
    },
    {
        "Global Settings",
        &global_menu_settings
    }
    
};
M5Menu::Menu main_menu(0,main_menu_items);
#endif