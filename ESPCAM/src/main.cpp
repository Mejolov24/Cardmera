#include <Arduino.h>
#include <esp_camera.h>
#define FLASH_GPIO_NUM 4

camera_config_t current_config = {};

enum modetype{
  FLASH,
  PX_FORMAT,
  FR_SIZE,
  FB_SIZE,
  JPEG_QUALITY,
  // setting above this need re-init (APPLY_CAM)
  BRIGHTNESS,
  CONTRAST,
  SATURATION,
  SHARPNESS,
  WB,
  WB_MODE,
  AWB_GAIN,
  EXP_CTRL,
  AE_LEVEL,
  AEC2,
  GAIN_CTRL,
  GAIN_CEILING,
  LENS_CORR,
  MIRROR,
  FLIP,
  SPECIAL,
  APPLY_CAM,
  REQUEST_FRAME
};

enum readState{
  HEADER, // waiting for ehader
  MODE,
  VALUE,
};

readState read_buffer = HEADER;
modetype mode_buffer;
int8_t value_buffer;
bool frame_requested = false;



void set_flash(bool value){
  digitalWrite(FLASH_GPIO_NUM,value);
}

void modeSet(modetype mode,int8_t value = 0){
  sensor_t *s = esp_camera_sensor_get();
  switch (mode)
  {
  case FLASH: set_flash(value); break;
  case PX_FORMAT: current_config.pixel_format = (pixformat_t)value; break;
  case FR_SIZE: current_config.frame_size = (framesize_t)value; break;
  case FB_SIZE: current_config.fb_count = value; break;
  case JPEG_QUALITY: current_config.jpeg_quality = value; s->set_quality(s,value); break;
  case BRIGHTNESS: s->set_brightness(s,value); break;
  case CONTRAST: s->set_contrast(s,value); break;
  case SATURATION: s->set_saturation(s,value); break;
  case SHARPNESS: s->set_sharpness(s,value); break;
  case WB: s->set_whitebal(s,value); break;
  case WB_MODE: s->set_wb_mode(s,value); break;
  case AWB_GAIN: s->set_awb_gain(s,value); break;
  case EXP_CTRL: s->set_exposure_ctrl(s,value); break;
  case AE_LEVEL: s->set_ae_level(s,value); break;
  case AEC2: s->set_aec2(s,value); break;
  case GAIN_CTRL: s->set_gain_ctrl(s,value); break;
  case GAIN_CEILING: s->set_gainceiling(s,(gainceiling_t)value); break;
  case LENS_CORR: s->set_lenc(s,value); break;
  case MIRROR: s->set_hmirror(s,value); break;
  case FLIP: s->set_vflip(s,value); break;
  case SPECIAL: s->set_special_effect(s,value); break;

  case APPLY_CAM:
    esp_camera_deinit();
    esp_camera_init(&current_config);
    break;
  
    case REQUEST_FRAME:
    frame_requested = true;
    break;

  default:
    break;
  }
}

void handleModesetSerial(int8_t data){
  switch (read_buffer)
  {
  case HEADER:
    if(data == 0xFF) read_buffer = MODE;
    else return;
    break;
  case MODE:
    mode_buffer = (modetype)data;
    read_buffer = VALUE;
    break;
  case VALUE:
    value_buffer = data;
    read_buffer = HEADER;
    modeSet(mode_buffer,value_buffer);
    break;
  default:
  return; break;
  }
}

void initialModeSet(){
current_config.pin_d0 = 5;
  current_config.pin_d1 = 18;
  current_config.pin_d2 = 19;
  current_config.pin_d3 = 21;
  current_config.pin_d4 = 36;
  current_config.pin_d5 = 39;
  current_config.pin_d6 = 34;
  current_config.pin_d7 = 35;
  current_config.pin_xclk = 0;
  current_config.pin_pclk = 22;
  current_config.pin_vsync = 25;
  current_config.pin_href = 23;
  current_config.pin_sccb_sda = 26;
  current_config.pin_sccb_scl = 27;
  current_config.pin_pwdn = 32;
  current_config.pin_reset = -1; // Not used
  current_config.xclk_freq_hz = 20000000;
  current_config.ledc_channel = LEDC_CHANNEL_0;
  current_config.ledc_timer = LEDC_TIMER_0;
  current_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  current_config.fb_location = CAMERA_FB_IN_PSRAM;
  current_config.jpeg_quality = 12; 
  modeSet(PX_FORMAT, PIXFORMAT_JPEG); 
  modeSet(FR_SIZE, FRAMESIZE_QQVGA);
  modeSet(FB_SIZE, 1);
  modeSet(APPLY_CAM);
  
}

void setup() {
Serial.begin(2000000);
pinMode(FLASH_GPIO_NUM, OUTPUT);
initialModeSet();

}

void loop() {
  while (Serial.available()){handleModesetSerial(static_cast<uint8_t>(Serial.read()));}
  if (frame_requested){
    frame_requested = false;
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb and !frame_requested and Serial.available()) return;
    Serial.write(0xAA); // Sync byte 1
    Serial.write(0xBB); // Sync byte 2
    Serial.write((uint8_t*)&fb->len, 4); // 4-byte length of the JPEG
    Serial.write(fb->buf, fb->len);      // JPEG data
    
    esp_camera_fb_return(fb);
  }
}