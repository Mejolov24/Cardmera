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

void invert_rgb() {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_x; y < canvas_state.render_y + canvas_state.resolution.h; y++) {
    for (int x = canvas_state.render_x; x < canvas_state.render_y + canvas_state.resolution.w; x++) {

      int i = y * canvas.width() + x;

      uint16_t p = pixels[i];

      uint16_t r = (p >> 11) & 0x1F;
      uint16_t g = (p >> 5)  & 0x3F;
      uint16_t b = p & 0x1F;

      pixels[i] = (b << 11) | (g << 5) | r;
    }
  }
}


void Static() {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_x; y < canvas_state.render_y + canvas_state.resolution.h; y++) {
    for (int x = canvas_state.render_x; x < canvas_state.render_y + canvas_state.resolution.w; x++) {
      uint16_t color = (fast_noise() & 0x1) ? 0xFFFF : 0x0000;
      int offset = y * canvas.width() + x;
      pixels[offset] = color;
    }
  }
}

void invert_endians() {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();
  for (int y = canvas_state.render_x; y < canvas_state.render_y + canvas_state.resolution.h; y++) {
    for (int x = canvas_state.render_x; x < canvas_state.render_y + canvas_state.resolution.w; x++) {

      int offset = y * canvas.width() + x;

      uint16_t pixel = pixels[offset];

      pixels[offset] = (pixel >> 8) | (pixel << 8);
    }
  }
}
#endif