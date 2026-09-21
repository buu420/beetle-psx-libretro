#ifndef BEETLE_PSX_ACCESSIBILITY_TRACE_H
#define BEETLE_PSX_ACCESSIBILITY_TRACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libretro.h"

void beetle_accessibility_trace_configure(const char *base_dir);
void beetle_accessibility_trace_set_serial(const char *serial);
void beetle_accessibility_trace_reset(const char *reason);
void beetle_accessibility_trace_input(retro_input_state_t input_state_cb);
void beetle_accessibility_trace_frame(const uint8_t *main_ram, size_t ram_size);
void beetle_accessibility_trace_video(const void *pixels, unsigned width,
      unsigned height, unsigned pitch, bool changed);
void beetle_accessibility_trace_audio(const int16_t *samples, size_t frames);
void beetle_accessibility_trace_speech(const char *text, int priority,
      const char *channel, const char *result);
void beetle_accessibility_trace_navigation_key(unsigned keycode, bool down,
      uint32_t overlay_tag);
void beetle_accessibility_trace_ram_write(uint32_t pc, uint32_t address,
      uint32_t value, unsigned width, const char *source);
void beetle_accessibility_trace_cpu_dialog_probe(uint32_t pc, uint32_t s1,
      uint32_t s2, uint32_t s6, uint32_t sp, uint32_t fp,
      const uint8_t *main_ram, size_t ram_size);
void beetle_accessibility_trace_cpu_name_entry_probe(uint32_t pc,
      uint32_t state, uint32_t selected,
      const uint8_t *main_ram, size_t ram_size);
void beetle_accessibility_trace_cpu_menu_probe(uint32_t pc,
      uint32_t overlay_tag, uint32_t task, uint32_t focus,
      uint32_t label_ref, uint32_t detail_ref, uint32_t flags);
void beetle_accessibility_trace_cpu_menu_reject(uint32_t pc,
      uint32_t overlay_tag, const char *reason);
void beetle_accessibility_trace_close(void);

#endif
