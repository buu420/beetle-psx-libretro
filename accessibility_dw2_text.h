#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_TEXT_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_TEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum beetle_dw2_text_mode
{
   BEETLE_DW2_TEXT_DIALOG = 0,
   BEETLE_DW2_TEXT_MENU = 1
} beetle_dw2_text_mode_t;

bool beetle_accessibility_dw2_decode_text(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      beetle_dw2_text_mode_t mode, char *out, size_t out_size);

bool beetle_accessibility_dw2_decode_dialog_with_speaker(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      char *out, size_t out_size, char *speaker, size_t speaker_size);

bool beetle_accessibility_dw2_decode_formatted_dialog_with_speaker(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      const uint32_t *arguments, size_t argument_count,
      char *out, size_t out_size, char *speaker, size_t speaker_size);

bool beetle_accessibility_dw2_decode_choice(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      uint8_t choice, char *out, size_t out_size);

bool beetle_accessibility_dw2_decode_speaker_label(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      char *out, size_t out_size);

bool beetle_accessibility_dw2_decode_formatted_text(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      const uint32_t *arguments, size_t argument_count,
      beetle_dw2_text_mode_t mode, char *out, size_t out_size);

bool beetle_accessibility_dw2_decode_text_fragment(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address,
      beetle_dw2_text_mode_t mode, char *out, size_t out_size);

bool beetle_accessibility_dw2_text_is_yes_no_prompt(
      const uint8_t *main_ram, size_t ram_size, uint32_t psx_address);

#endif
