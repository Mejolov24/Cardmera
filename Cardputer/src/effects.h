#ifndef EFFECTS_H
    #define EFFECTS_H
#include <stdint.h>
#include <M5GFX.h>
extern M5Canvas canvas;
extern Canvas_State canvas_state;

uint32_t fast_noise() {
    static uint32_t state = 0xACE1u; // Seed
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void bleeding(uint8_t strength) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_y; y < canvas_state.render_y + canvas_state.render_h; y++) {
      for (int x = canvas_state.render_x; x < canvas_state.render_x + canvas_state.render_w; x++) {

      int offset = y * canvas.width() + x;
      uint16_t pixel = pixels[offset];
      uint16_t p = __builtin_bswap16(pixels[offset]);

      // 2. Extract and swap channels
      uint16_t r = (p >> 11) & 0x1F;
      uint16_t g = (p >> 5)  & 0x3F;
      uint16_t b = p & 0x1F;
      uint8_t scale = 5 - ((strength * 5) / 255);
      g = (g + (r >> scale)) & 0x3F;
      b = (b + (g >> scale)) & 0x1F;
      uint16_t result = (r << 11) | (g << 5) | b;
      pixels[offset] = __builtin_bswap16(result);
    }
  }
}

void rb_mask(uint8_t strength) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_y; y < canvas_state.render_y + canvas_state.render_h; y++) {
      for (int x = canvas_state.render_x; x < canvas_state.render_x + canvas_state.render_w; x++) {

      int offset = y * canvas.width() + x;
      uint16_t pixel = pixels[offset];
      uint16_t p = __builtin_bswap16(pixels[offset]);

      // 2. Extract and swap channels
      uint16_t r = (p >> 11) & 0x1F;
      uint16_t g = (p >> 5)  & 0x3F;
      uint16_t b = p & 0x1F;
      uint8_t scale = 5 - ((strength * 5) / 255);
      g = g & (b >> scale);
      b = b & (g >> scale);
      g = g & 0x3F;
      b = b & 0x1F;
      uint16_t result = (r << 11) | (g << 5) | b;
      pixels[offset] = __builtin_bswap16(result);
    }
  }
}

void invert_rgb(uint8_t strength) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_y; y < canvas_state.render_y + canvas_state.render_h; y++) {
      for (int x = canvas_state.render_x; x < canvas_state.render_x + canvas_state.render_w; x++) {

      int offset = y * canvas.width() + x;
      uint16_t pixel = pixels[offset];
      uint16_t p = __builtin_bswap16(pixels[offset]);

      // 2. Extract and swap channels
      uint16_t r = (p >> 11) & 0x1F;
      uint16_t g = (p >> 5)  & 0x3F;
      uint16_t b = p & 0x1F;
      uint16_t swapped = (b << 11) | (g << 5) | r;

      // 3. Swap bytes back to match the canvas buffer's expected format and save
      pixels[offset] = __builtin_bswap16(swapped);
    }
  }
}



void Static() {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_y; y < canvas_state.render_y + canvas_state.render_h; y++) {
      for (int x = canvas_state.render_x; x < canvas_state.render_x + canvas_state.render_w; x++) {
      uint16_t color = (fast_noise() & 0x1) ? 0xFFFF : 0x0000;
      int offset = y * canvas.width() + x;
      pixels[offset] = color;
    }
  }
}

void invert_endians(uint8_t strength) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_y; y < canvas_state.render_y + canvas_state.render_h; y++) {
      for (int x = canvas_state.render_x; x < canvas_state.render_x + canvas_state.render_w; x++) {

      int offset = y * canvas.width() + x;

      uint16_t pixel = pixels[offset];

      pixels[offset] = __builtin_bswap16(pixels[offset]);
    }
  }
}



#endif