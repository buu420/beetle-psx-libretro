#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_STORY_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_STORY_H
#include <stddef.h>
#include <stdint.h>
#define BEETLE_DW2_STORY_MAX_OBJECTIVES 4
typedef struct beetle_dw2_story_objective
{
   uint32_t id;
   int16_t scene;
   int16_t domain;
   char npc[48];
   char label[192];
} beetle_dw2_story_objective_t;
size_t beetle_accessibility_dw2_story_objectives(const uint8_t *ram,
      size_t ram_size, beetle_dw2_story_objective_t *out, size_t capacity);
void beetle_accessibility_dw2_story_reset(void);
void beetle_accessibility_dw2_story_observe_dialogue(const uint8_t *ram,
      size_t ram_size, const char *visible_text);
const char *beetle_accessibility_dw2_story_scene_name(unsigned scene);
const char *beetle_accessibility_dw2_story_domain_name(unsigned domain);
#endif
