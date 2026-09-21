#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_MENU_PROFILE_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_MENU_PROFILE_H

#include <stddef.h>
#include <stdint.h>

#define BEETLE_DW2_MENU_PROFILE_HASH "5c85b974838512f18848bcf34722dab4305e3914a1d2e283e4eb2bfd9b4b94e2"
#define BEETLE_DW2_MENU_GPR_NONE 255u
#define BEETLE_DW2_OVERLAY_SIGNATURE_WORDS 4u
#define BEETLE_DW2_MENU_PROBE_RULE_COUNT 4u
#define BEETLE_DW2_OVERLAY_SIGNATURE_COUNT 7u
#define BEETLE_DW2_MENU_RULE_TASK_POINTER 0x00000001u
#define BEETLE_DW2_MENU_RULE_FOCUS_VALUE 0x00000002u
#define BEETLE_DW2_MENU_RULE_LABEL_TEXT_POINTER 0x00000004u
#define BEETLE_DW2_MENU_RULE_DETAIL_TEXT_POINTER 0x00000008u
#define BEETLE_DW2_MENU_RULE_VISIBLE_ONLY 0x00000010u
#define BEETLE_DW2_MENU_RULE_EXISTING_HOOK 0x00000020u

typedef enum beetle_dw2_menu_profile_context
{
   BEETLE_DW2_PROFILE_CONTEXT_NONE = 0,
   BEETLE_DW2_PROFILE_CONTEXT_SYSTEM = 1,
   BEETLE_DW2_PROFILE_CONTEXT_DOMAIN_SELECT = 2,
   BEETLE_DW2_PROFILE_CONTEXT_DOMAIN_CIRCLE = 3,
   BEETLE_DW2_PROFILE_CONTEXT_LIST = 4,
   BEETLE_DW2_PROFILE_CONTEXT_PROMPT = 5,
   BEETLE_DW2_PROFILE_CONTEXT_BATTLE = 6,
   BEETLE_DW2_PROFILE_CONTEXT_NAME_ENTRY = 7,
   BEETLE_DW2_PROFILE_CONTEXT_DIALOGUE_CHOICE = 8,
} beetle_dw2_menu_profile_context_t;

typedef enum beetle_dw2_menu_profile_adapter
{
   BEETLE_DW2_PROFILE_ADAPTER_INDEXED = 0,
   BEETLE_DW2_PROFILE_ADAPTER_DOMAIN_MAP = 1,
   BEETLE_DW2_PROFILE_ADAPTER_DOMAIN_CIRCLE = 2,
   BEETLE_DW2_PROFILE_ADAPTER_GRID = 3,
   BEETLE_DW2_PROFILE_ADAPTER_BATTLE = 4,
   BEETLE_DW2_PROFILE_ADAPTER_NAME_ENTRY = 5,
   BEETLE_DW2_PROFILE_ADAPTER_DIALOGUE_CHOICE = 6,
} beetle_dw2_menu_profile_adapter_t;

typedef struct beetle_dw2_menu_probe_rule
{
   uint32_t pc;
   uint32_t overlay_tag;
   uint8_t context;
   uint8_t adapter;
   uint8_t task_reg;
   uint8_t focus_reg;
   uint8_t label_reg;
   uint8_t detail_reg;
   uint8_t focus_min;
   uint8_t focus_max;
   uint32_t flags;
} beetle_dw2_menu_probe_rule_t;

typedef struct beetle_dw2_overlay_signature
{
   uint32_t tag;
   uint32_t load_base;
   uint32_t size;
   uint32_t signature_address;
   uint32_t signature_words[BEETLE_DW2_OVERLAY_SIGNATURE_WORDS];
} beetle_dw2_overlay_signature_t;

extern const char beetle_accessibility_dw2_menu_profile_hash[];

const beetle_dw2_menu_probe_rule_t *
beetle_accessibility_dw2_menu_rule_lookup(uint32_t pc,
      uint32_t overlay_tag);
const beetle_dw2_overlay_signature_t *
beetle_accessibility_dw2_overlay_signatures(size_t *count);
const char *beetle_accessibility_dw2_icon_label(uint32_t resource_id);
const char *beetle_accessibility_dw2_context_name(uint8_t context);
size_t beetle_accessibility_dw2_menu_rule_count(void);

#endif
