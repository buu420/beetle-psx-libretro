#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_MENU_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_MENU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BEETLE_DW2_MENU_TEXT_MAX 640
#define BEETLE_DW2_MENU_SPEECH_MAX 2048
#define BEETLE_DW2_MENU_STABLE_FRAMES 3

typedef enum beetle_dw2_menu_context
{
   BEETLE_DW2_MENU_NONE = 0,
   BEETLE_DW2_MENU_SYSTEM,
   BEETLE_DW2_MENU_DOMAIN_SELECT,
   BEETLE_DW2_MENU_DOMAIN_CIRCLE,
   BEETLE_DW2_MENU_LIST,
   BEETLE_DW2_MENU_PROMPT,
   BEETLE_DW2_MENU_BATTLE,
   BEETLE_DW2_MENU_NAME_ENTRY,
   BEETLE_DW2_MENU_DIALOGUE_CHOICE,
   BEETLE_DW2_MENU_INSPECTION
} beetle_dw2_menu_context_t;

typedef struct beetle_dw2_menu_snapshot
{
   bool active;
   bool enabled;
   uint8_t layer;
   beetle_dw2_menu_context_t context;
   uint32_t task;
   uint32_t focus_id;
   char title[BEETLE_DW2_MENU_TEXT_MAX];
   char label[BEETLE_DW2_MENU_TEXT_MAX];
   char details[BEETLE_DW2_MENU_TEXT_MAX];
} beetle_dw2_menu_snapshot_t;

typedef struct beetle_dw2_menu_probe
{
   uint32_t pc;
   uint32_t overlay_tag;
   uint32_t task;
   uint32_t focus;
   uint32_t label_ref;
   uint32_t detail_ref;
   uint32_t flags;
   uint8_t context;
   uint8_t adapter;
} beetle_dw2_menu_probe_t;

void beetle_accessibility_dw2_menu_reset(void);
void beetle_accessibility_dw2_menu_begin_frame(void);
void beetle_accessibility_dw2_menu_poll_native(const uint8_t *main_ram,
      size_t ram_size, uint32_t overlay_tag);
void beetle_accessibility_dw2_battle_frame(const uint8_t *main_ram,
      size_t ram_size, uint32_t overlay_tag, bool menu_active);
bool beetle_accessibility_dw2_menu_active(void);
bool beetle_accessibility_dw2_menu_blocks_dialog(void);
bool beetle_accessibility_dw2_menu_battle_active(void);
bool beetle_accessibility_dw2_menu_observe_probe(const uint8_t *main_ram,
      size_t ram_size, const beetle_dw2_menu_probe_t *probe);
bool beetle_accessibility_dw2_menu_submit(
      const beetle_dw2_menu_snapshot_t *snapshot);
void beetle_accessibility_dw2_menu_end_frame(void);

#endif
