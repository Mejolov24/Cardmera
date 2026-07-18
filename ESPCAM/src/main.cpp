#include <Arduino.h>
#include <esp_camera.h>
#define FLASH_GPIO_NUM 4

camera_config_t current_config;

enum modetype{
  FLASH,
  PX_FORMAT,
  FR_SIZE,
  FB_SIZE,
  APPLY_CAM
};

enum readState{
  HEADER, // waiting for ehader
  MODE,
  VALUE,
};

readState read_buffer = HEADER;
modetype mode_buffer;
uint8_t value_buffer;

void set_flash(bool value){
  digitalWrite(FLASH_GPIO_NUM,value);
}

void modeSet(modetype mode,uint8_t value = 0){
  switch (mode)
  {
  case FLASH:
    set_flash(value);
    break;

  case PX_FORMAT:
    current_config.pixel_format = (pixformat_t)value;
    break;

  case FR_SIZE:
    current_config.frame_size = (framesize_t)value;
  break;

  case FB_SIZE:
    current_config.fb_count = value;
  break;

  case APPLY_CAM:
    esp_camera_deinit();
    esp_camera_init(&current_config);
    break;
  
  default:
    break;
  }
}

void handleModesetSerial(uint8_t data){
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
  modeSet(PX_FORMAT, PIXFORMAT_RGB565);
  modeSet(FR_SIZE, FRAMESIZE_QQVGA);
  modeSet(FB_SIZE, 2);
  modeSet(APPLY_CAM);
}

void setup() {
Serial.begin(115200);
pinMode(FLASH_GPIO_NUM, OUTPUT);
initialModeSet();
}

void loop() {
camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    // Send a simple header so Cardputer knows a frame starts
    Serial.write(0xAA); 
    Serial.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
  while (Serial.available()){handleModesetSerial(Serial.read());}
}