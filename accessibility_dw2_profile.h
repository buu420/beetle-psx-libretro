#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_PROFILE_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_PROFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct beetle_accessibility_dw2_message
{
   uint16_t file_index;
   uint32_t offset;
   const char *text;
};

extern const char beetle_accessibility_dw2_serial[];

const char *beetle_accessibility_dw2_title_lookup(uint16_t signature,
      bool start_pressed);
const char *beetle_accessibility_dw2_title_selection_lookup(uint8_t selection);
const char *beetle_accessibility_dw2_message_lookup(uint16_t file_index,
      uint32_t offset);
const char *beetle_accessibility_dw2_movie_cue_lookup(uint32_t disc_sector);
size_t beetle_accessibility_dw2_message_count(void);

#endif
