#ifndef MENU_H
#define MENU_H
#include <M5Menu.h>

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

#define BEEP_1 1318.51
#define BEEP_2 1046.50
#define CAMERA_PRESS 4000
#define CAMERA_RELEASE 4000

#define UI_FONT &fonts::FreeSerifBold9pt7b
#define NO_SIGNAL_FONT &fonts::FreeSerifBold12pt7b

struct Resolution {
    uint16_t w;
    uint16_t h;
};

const Resolution frameSizes[] = {
    {96,96},
    {160,120},
    {176,144},
    {240,176},
    {240,240},
    {320,240},
    {400,296},
    {480,320},
    {640,480},
    {800,600},
    {1024,768},
    {1280,720},
    {1280,1024},
    {1600,1200},
    {1920,1080},
    {720,1280},
    {864,1536},
    {2048,1536},
    {2560,1440},
    {2560,1600},
    {1080,1920},
    {2560,1920},
};

enum ReceiveState {
  WAIT_SYNC_1,
  WAIT_SYNC_2,
  READ_LENGTH,
  READ_PAYLOAD
};

enum modetype{
  FLASH,
  FR_SIZE,
  JPEG_QUALITY,
  BRIGHTNESS,
  CONTRAST,
  SATURATION,
  SHARPNESS,
  WB,
  WB_MODE,
  AWB_GAIN,
  EXP_CTRL,
  AE_LEVEL,
  AEC_VALUE,
  AEC2,
  GAIN_CTRL,
  GAIN_CEILING,
  LENS_CORR,
  MIRROR,
  FLIP,
  SPECIAL,
  REQUEST_FRAME
};

String aec_options[] = {
    "Fast (200)",
    "Short (400)",
    "Normal (600)",
    "Long (800)",
    "Longer (1000)",
};
uint8_t aec_map[] = {42,85,127,170,212};

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

M5Menu::MenuTheme theme{
.background_color = 0x211a,
.border_color = 0x2c9f,
.selection_color = 0x06e0,
.value_color = WHITE,
.item_height = 23,
.item_window = 5,
.bool_true_color = GREEN,
.bool_false_color = RED,
.bool_true_string = "On",
.bool_false_string = "Off",
.font = &fonts::FreeSans9pt7b};

struct localCameraSettings{
  uint8_t PX_FORMAT;
  uint8_t FR_SIZE = 2;
  uint8_t FB_SIZE;
  uint8_t JPEG_QUALITY = 6; //0 to 63
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
    uint8_t AEC_VALUE = static_cast<int8_t>(aec_map[2]);
    bool AEC2 = false;
    bool GAIN_CTRL = true;
    int8_t GAIN_CEILING = 0;
    bool LENS_CORR = true;
    bool MIRROR = false;
    bool FLIP = false;
    int8_t SPECIAL = 0;
    uint8_t frame_await = 0;
    bool direct_write = false;

    bool invert_rgb = false;
    bool invert_endians = false;
};
GlobalCameraSettings global_settings = {};
localCameraSettings viewfinder_settings = {};
localCameraSettings photo_settings = {};
localCameraSettings recording_settings = {};

M5Menu::MenuItem circuit_bending_items[] = {
    {
        "Invert RGB",
        &global_settings.invert_rgb
    },
    {
        "Invert Endians",
        &global_settings.invert_endians
    }
};
M5Menu::Menu circuit_bending_menu(5,circuit_bending_items);
M5Menu::MenuItem global_settings_items[] = {
    {"Brightness", &global_settings.BRIGHTNESS, 1, -2, 2},
    {"Contrast", &global_settings.CONTRAST, 1, -2, 2},
    {"Saturation", &global_settings.SATURATION, 1, -2, 2},
    {"Sharpness", &global_settings.SHARPNESS, 1, -2, 2},
    {"Auto White Balance", &global_settings.WB},
    {"White Balance Mode", &global_settings.WB_MODE, 1, 0, 4},
    {"Auto WB Gain", &global_settings.AWB_GAIN},
    {"Auto Exposure", &global_settings.EXP_CTRL},
    {"Exposure Bias", &global_settings.AE_LEVEL, 1, -2, 2},
    {"Exposure Speed", &global_settings.AEC_VALUE, aec_options},
    {"Enhanced Exposure", &global_settings.AEC2},
    {"Auto Gain", &global_settings.GAIN_CTRL},
    {"Max Gain", &global_settings.GAIN_CEILING},
    {"Lens Correction", &global_settings.LENS_CORR},
    {"Mirror Image", &global_settings.MIRROR},
    {"Flip Image", &global_settings.FLIP},
    {"Color Effect", &global_settings.SPECIAL, 1, 0, 6},
    {"Circuit Bending",&circuit_bending_menu}
};
M5Menu::Menu global_menu_settings(4,global_settings_items);


M5Menu::MenuItem recording_settings_items[] = {
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
    {
        "Frame await",
        &global_settings.frame_await
    },
    {
        "Direct Write",
        &global_settings.direct_write
    }
    
};
M5Menu::Menu photo_menu_settings(2,photo_settings_items);


M5Menu::MenuItem viewfinder_settings_items[] = {
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