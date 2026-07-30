#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5CADVKeyCB.h>
#include <M5Menu.h>
#include <Menu.h>
#include <OneButton.h>
#include <SdFat.h>
#include <sprites.h>
SdFat sd;
FsFile current_file;
OneButton g0Button(0, true);
M5CADVKeyCB keyHandler;
M5Menu menu;
M5Canvas canvas(&M5.Lcd);
M5Canvas sprite_buffer(&canvas);

struct Canvas_State{
  Resolution resolution;
  float resolution_scale = 0;
  uint32_t render_x = 0;
  uint32_t render_y = 0;
};
#include <effects.h>

enum SDState{
  SD_NONE,
  SD_NORMAL,
  SD_BUSY
};
enum FlashState {
  FLASH_OFF,
  FLASH_ON,
  FLASH_TORCH,
};
enum FrameState {
  FRAME_DISABLED,
  FRAME_NORMAL,
  FRAME_AWAIT_PHOTO,
};
enum Mode{
  MODE_PHOTO,
  MODE_VIDEO
};
Canvas_State canvas_state = {};

ReceiveState rx_state = WAIT_SYNC_1;
uint32_t jpeg_length = 0;
uint32_t frame_bytes_read = 0;

#define MAX_IMG_SIZE 150000
#define HALF_BUFFER ((MAX_IMG_SIZE) /2)
#define ACTION_WRITE_SD_BUFFER 1
#define ACTION_WRITE_SD_BUFFER_A 2
#define ACTION_WRITE_SD_BUFFER_B 3
uint8_t video_buffer[MAX_IMG_SIZE];

class Buffer{
  public:
  uint8_t* memory = nullptr;
  uint32_t size = 0;
  uint32_t start = 0;
  uint32_t end = 0;
  uint32_t head = 0;
  Buffer(uint8_t* mem, uint32_t size_, int32_t start_, int32_t end_) : memory(mem), size(size_), start(start_), end(end_) {}
};
Buffer Buffer_A(video_buffer,HALF_BUFFER - 1, 0, HALF_BUFFER);
Buffer Buffer_B(video_buffer + HALF_BUFFER,HALF_BUFFER, 0, MAX_IMG_SIZE);
Buffer Buffers[2] = {Buffer_A, Buffer_B};

unsigned long lastFrameTime = 0;
const unsigned long FRAME_TIMEOUT = 2000;
uint8_t frame_await_queque = 0;

bool at_menu = false;
bool holding_shutter = false;
bool out_of_memory = false;
bool battery_charging;
volatile bool current_buffer = 0;
volatile bool DW_Done = false;
volatile SDState sd_state = SD_NONE;
volatile FrameState _frame_state = FRAME_NORMAL;
int32_t battery_level = 0;
uint16_t sd_files_amount = 0;
localCameraSettings* current_settings = &viewfinder_settings;
TaskHandle_t SDHandlerTask;
uint8_t _flash_state = FLASH_OFF;
Mode current_mode = MODE_PHOTO;
bool at_viewfinder = true;
void modeSetSend(modetype mode,int8_t value = 0){
Serial2.write(0xFF);
Serial2.write(mode);
Serial2.write(static_cast<uint8_t>(value));
}

void RequestFrame(){
  rx_state = WAIT_SYNC_1;
  frame_bytes_read = 0;
  modeSetSend(REQUEST_FRAME);
}

void SetFlash(uint8_t value = 255){
  if(value == 255) _flash_state ++;
  if (_flash_state > 2) _flash_state = 0;
  switch (_flash_state)
  {
  case FLASH_OFF:
    modeSetSend(FLASH,0);
    M5Cardputer.Speaker.tone(BEEP_1 ,100);
    break;
  case FLASH_ON:
    M5Cardputer.Speaker.tone(BEEP_2,100);
    break;
  case FLASH_TORCH:
    modeSetSend(FLASH,1);
    M5Cardputer.Speaker.tone(BEEP_3,100);
    break;
  
  default:
    break;
  }
}

void StopFrame(){
  rx_state = WAIT_SYNC_1;
  frame_bytes_read = 0;
  while (Serial2.available()) {Serial2.read();}
}

void SetFrameState(FrameState state){
  _frame_state = state;
}

void resolution_modeset(localCameraSettings* mode){
  if(mode == &viewfinder_settings){at_viewfinder = true;}
  else{at_viewfinder = false;}
  current_settings = mode;
  StopFrame();

  canvas_state.resolution = frameSizes[current_settings->FR_SIZE];
  canvas_state.resolution_scale = min((float)canvas.width() / canvas_state.resolution.w, (float)canvas.height() / canvas_state.resolution.h);
  canvas_state.render_x = (canvas.width() - (canvas_state.resolution.w * canvas_state.resolution_scale)) / 2;
  canvas_state.render_y = (canvas.height() - (canvas_state.resolution.h * canvas_state.resolution_scale)) / 2;

  modeSetSend(FR_SIZE,current_settings->FR_SIZE);
  modeSetSend(JPEG_QUALITY,current_settings->JPEG_QUALITY);
  SetFrameState(FRAME_NORMAL);
  RequestFrame();
}

void force_modeset(){
  // discard old camera data
  StopFrame();
  modeSetSend(FR_SIZE,current_settings->FR_SIZE);
  modeSetSend(JPEG_QUALITY,current_settings->JPEG_QUALITY);
  modeSetSend(BRIGHTNESS,global_settings.BRIGHTNESS);
  modeSetSend(CONTRAST,global_settings.CONTRAST);
  modeSetSend(CONTRAST,global_settings.CONTRAST);
  modeSetSend(SHARPNESS,global_settings.SHARPNESS);
  modeSetSend(WB,global_settings.WB);
  modeSetSend(WB_MODE,global_settings.WB_MODE);
  modeSetSend(AWB_GAIN,global_settings.AWB_GAIN);
  modeSetSend(EXP_CTRL,global_settings.EXP_CTRL);
  modeSetSend(AE_LEVEL,global_settings.AE_LEVEL);
  modeSetSend(AEC_VALUE,static_cast<int8_t>(aec_map[global_settings.AEC_VALUE]));
  modeSetSend(AEC2,global_settings.AEC2);
  modeSetSend(GAIN_CTRL,global_settings.GAIN_CTRL);
  modeSetSend(AGC_GAIN,global_settings.AGC_GAIN);
  if(global_settings.GAIN_CEILING != 0) modeSetSend(GAIN_CEILING,global_settings.GAIN_CEILING);
  modeSetSend(AGC_GAIN,global_settings.AGC_GAIN);
  modeSetSend(DENOISE,global_settings.DENOISE);
  modeSetSend(RAW_GMA,global_settings.RAW_GMA);
  modeSetSend(BPC,global_settings.BPC);
  modeSetSend(WPC,global_settings.WPC);
  modeSetSend(DCW,global_settings.DCW);
  modeSetSend(LENS_CORR,global_settings.LENS_CORR);
  modeSetSend(MIRROR,global_settings.MIRROR);
  modeSetSend(FLIP,global_settings.FLIP);
  modeSetSend(SPECIAL,global_settings.SPECIAL);
  SetFrameState(FRAME_NORMAL);
  RequestFrame();

  canvas_state.resolution = frameSizes[current_settings->FR_SIZE];
  canvas_state.resolution_scale = min((float)canvas.width() / canvas_state.resolution.w, (float)canvas.height() / canvas_state.resolution.h);
  canvas_state.render_x = (canvas.width() - (canvas_state.resolution.w * canvas_state.resolution_scale)) / 2;
  canvas_state.render_y = (canvas.height() - (canvas_state.resolution.h * canvas_state.resolution_scale)) / 2;

}

void OnUsage(M5Menu::MenuItem* item, M5Menu::Menu* menu){
    }

void render_sprite(uint8_t sprite_id,uint16_t pos_X,uint16_t pos_Y){
  uint16_t sprite_w = 16;
  uint16_t sprite_h = 16;
  uint16_t sheet_w = 288;
  uint16_t sheet_h = 16;
  uint16_t offset = (sprite_h) * sprite_id;
  sprite_buffer.clear();
  sprite_buffer.setSwapBytes(true);
  sprite_buffer.pushImage(-offset,0,sheet_w,sheet_h,epd_bitmap_sprites,(uint16_t)0xF81F);
  sprite_buffer.setSwapBytes(false);
  sprite_buffer.pushSprite(pos_X,pos_Y);
}

void render(){
  if(at_menu){
    canvas.pushSprite(0,0);
    canvas.clear();
    return;
  }
  render_sprite(SPRITE_FLASH + _flash_state,0,0);

  switch (sd_state)
  {
  case SD_NONE: render_sprite(SPRITE_SD_NONE,224,0); break;
  case SD_BUSY: render_sprite(SPRITE_SD_BUSY,224,0); break;
  case SD_NORMAL: render_sprite(SPRITE_SD_NORMAL,224,0); break;
  default: break;
  }
  if(battery_level < 100 and battery_level > 80){render_sprite(SPRITE_BATTERY + 1,224,16);}
  if(battery_level < 80 and battery_level > 60){render_sprite(SPRITE_BATTERY + 2,224,16);}
  if(battery_level < 60 and battery_level > 40){render_sprite(SPRITE_BATTERY + 3,224,16);}
  if(battery_level < 40 and battery_level > 20){render_sprite(SPRITE_BATTERY + 4,224,16);}
  if(battery_level < 20){render_sprite(SPRITE_BATTERY + 5,224,16);}
  if(global_settings.direct_write){render_sprite(SPRITE_DIRECT_WRITE,224,119);}

  canvas.pushSprite(0,0);
  canvas.clear();
}

void OnKey(uint8_t key, bool pressed){
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
  if (status.opt){
    at_menu = !at_menu;
    at_menu ? menu.open() : menu.close();
  }
  if (status.del){menu.process_input(M5Menu::Input::BACK);}
  if (status.enter) menu.process_input(M5Menu::Input::SELECT);
  if(!pressed) return;
  switch (key){

    case 53: // escape
    SetFlash();
    break;
    
    case 51:
        menu.process_input(M5Menu::Input::UP);
        break;
    case 55:
        menu.process_input(M5Menu::Input::DOWN);
        break;

    case 54:
    menu.process_input(M5Menu::Input::LEFT);
    break;

    case 56:
    menu.process_input(M5Menu::Input::RIGHT);
    break;

    default:
        break;
  }
  render();
}

void update_SD_Count(){
  if(sd_state == SD_NONE) return;
  sd_files_amount = 0;
  FsFile root;
  FsFile file;

  if (!root.open("/DCMI")) return;

  while (file.openNext(&root, O_RDONLY)) {
    if (!file.isDir()) {
        sd_files_amount++;
    }
    file.close();
  }
  root.close();
}

void SDTask(void *pvParameters){
  uint32_t notificationValue = 0;
  char filename[256];
  for(;;){
      if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notificationValue, portMAX_DELAY) == pdPASS) {
        switch (current_mode)
        {
        case MODE_PHOTO:
        snprintf(filename, sizeof(filename), "/DCMI/Photo_%u.jpeg", sd_files_amount);
          break;
        
        default: break;
        }

        
        if(global_settings.direct_write){
          if (!current_file.isOpen()) {current_file = sd.open(filename, O_WRITE | O_CREAT | O_TRUNC);}
            current_file.write(Buffers[!current_buffer].memory, Buffers[!current_buffer].head);
            Buffers[!current_buffer].head = 0;
            Serial.println("Write");
            if(DW_Done){
              current_file.close();
              Serial.println("Finished from SD");
              sd_state = SD_NORMAL;
              DW_Done = false;
              Buffers[0].head = 0;
              Buffers[1].head = 0;
              update_SD_Count();
              resolution_modeset(&viewfinder_settings);
            }
        }
        else{
            current_file = sd.open(filename, O_WRITE | O_CREAT | O_TRUNC);
            current_file.write(video_buffer,jpeg_length);
            current_file.close();
            sd_state = SD_NORMAL;
            update_SD_Count();
            resolution_modeset(&viewfinder_settings);
        }
      }
  }
}

void SDWrite(){
  sd_state = SD_BUSY;
  if(!global_settings.direct_write){StopFrame();}
  xTaskNotifyGive(SDHandlerTask);
}

void HandleBuffering(){
  int bytes_available = Serial2.available();
  int bytes_to_read = min(bytes_available, (int)(jpeg_length - frame_bytes_read));
  if (global_settings.direct_write and _frame_state == FRAME_DISABLED){
    Serial2.readBytes(&Buffers[current_buffer].memory[Buffers[current_buffer].head], bytes_to_read);
    Buffers[current_buffer].head += bytes_to_read;
    if (frame_bytes_read + bytes_to_read >= jpeg_length){
      Serial.println("read all bytes, last write in progress.");
      DW_Done = true;
      current_buffer = !current_buffer;
      SDWrite();
    }
    if (Buffers[current_buffer].head >= Buffers[current_buffer].size){ // this
      Serial.println("Swapper bufffer!");
      current_buffer = !current_buffer;
      SDWrite();
      return;
    }
  }
  else{
   Serial2.readBytes(&video_buffer[frame_bytes_read], bytes_to_read);
  }
  frame_bytes_read += bytes_to_read;
}

void take_photo(){
  if(_flash_state == FLASH_ON) modeSetSend(FLASH,0);
  SetFrameState(FRAME_DISABLED);
  frame_await_queque = 0;
  M5Cardputer.Speaker.tone(CAMERA_RELEASE,30);
  if(sd_state == SD_NONE) {resolution_modeset(&viewfinder_settings); return;}
  SDWrite();
}

void OOM(){
  if(global_settings.direct_write) return;
  if(_flash_state == FLASH_ON) modeSetSend(FLASH,0);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(RED,BLACK);
  canvas.setFont(NO_SIGNAL_FONT);
  canvas.drawString("Out of Memory!",M5.Display.width()/2,M5.Display.height()/2 - 14);
  canvas.drawString(at_viewfinder ? "Lower resolution!" : "Enable DW!",M5.Display.width()/2,M5.Display.height()/2 + 14);
  render();
  M5Cardputer.Speaker.tone(BEEP_1,100);
  delay(110);
  M5Cardputer.Speaker.tone(BEEP_1,100);
  delay(110);
  M5Cardputer.Speaker.tone(BEEP_1,200);
  resolution_modeset(&viewfinder_settings);
}

void CameraTick(){
  if (at_menu) return;
  if (_frame_state == FRAME_DISABLED and !global_settings.direct_write) return;
  battery_level = M5Cardputer.Power.getBatteryLevel();
  if(at_menu) return;
  if (millis() - lastFrameTime > FRAME_TIMEOUT){
    RequestFrame();
    Static();
    canvas.setFont(NO_SIGNAL_FONT);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(ORANGE,BLACK);
    canvas.drawString("No video",M5.Display.width()/2,M5.Display.height()/2);
    if (at_menu) return;
    render();
  }

  while (Serial2.available() > 0) {
  switch (rx_state)
    {
    case WAIT_SYNC_1:{if (Serial2.read() == 0xAA){ rx_state = WAIT_SYNC_2;}break;}
    case WAIT_SYNC_2:{if (Serial2.read() == 0xBB){ rx_state = READ_LENGTH;} else{ rx_state = WAIT_SYNC_1;} break;}
    case READ_LENGTH:{
      if (!(Serial2.available() >= 4)) break;
      Serial2.readBytes((uint8_t*)&jpeg_length, 4);
      if (jpeg_length == 0) rx_state = WAIT_SYNC_1; // Corrupted header, restart
      if(jpeg_length > MAX_IMG_SIZE) OOM();
      else {
        rx_state = READ_PAYLOAD;
        frame_bytes_read = 0;
      }
      break;
    }
    case READ_PAYLOAD:{
        HandleBuffering();
        lastFrameTime = millis();
        if (frame_bytes_read < jpeg_length) return;

        canvas.drawJpg(video_buffer, jpeg_length, canvas_state.render_x, canvas_state.render_y,0,0,0, canvas_state.resolution_scale, canvas_state.resolution_scale);
        invert_endians();
        // Reset state machine and request the next frame
        if (_frame_state == FRAME_AWAIT_PHOTO){
          frame_await_queque ++;
          if(frame_await_queque >= global_settings.frame_await) take_photo();
          break;
        }
        RequestFrame();
        render();
        break;
    }
    default:
      break;
    }
  }
}

void button_press(){
  M5Cardputer.Speaker.tone(CAMERA_PRESS,30);
  resolution_modeset(&photo_settings);
  if(_flash_state == FLASH_ON) modeSetSend(FLASH,1);
  frame_await_queque = 0;
  holding_shutter = true;
}
void button_release(){
  SetFrameState(FRAME_AWAIT_PHOTO);
  holding_shutter = false;
}

// TODO: make ticker start and stop during disconects, and make it faster for a cooler effect.
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  M5Cardputer.Power.getBatteryLevel();
  xTaskCreatePinnedToCore(SDTask ,"SDHandlerTask",4096,NULL,0,&SDHandlerTask,0);

  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  if(sd.begin(SD_SPI_CS_PIN, SD_SCK_MHZ(25))){sd_state = SD_NORMAL;}
  sd.mkdir("/AppData");
  sd.mkdir("/AppData/Cardmera");
  sd.mkdir("/DCMI");
  update_SD_Count();
  Serial.begin(9600);
  battery_level = M5Cardputer.Power.getBatteryLevel();
  canvas.createSprite(240, 135);
  sprite_buffer.createSprite(16,16);
  canvas.setBaseColor(BLACK);
  canvas.setColorDepth(16);;
  M5Cardputer.Display.setFont(UI_FONT);
  Serial2.setRxBufferSize(40000);
  Serial2.begin(2000000,SERIAL_8N1,13,15);
  g0Button.setPressMs(0);
  g0Button.attachLongPressStart(button_press);
  g0Button.attachLongPressStop(button_release);
  keyHandler.SetupKeyboardCallback(OnKey);

  menu.begin(&canvas,render,OnUsage);
  menu.setTheme(&theme);
  menu.goToMenu(&main_menu);
  set_volume();
  M5Cardputer.Speaker.tone(BEEP_2,100);
  delay(100);
  M5Cardputer.Speaker.tone(BEEP_1,100);
  force_modeset();

}

void loop() {
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();
  CameraTick();
  g0Button.tick();
}