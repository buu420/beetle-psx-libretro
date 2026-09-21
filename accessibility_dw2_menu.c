#include "accessibility_dw2_menu.h"

#include "accessibility_dw2_menu_profile.h"
#include "accessibility_dw2_text.h"
#include "accessibility_speech.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define DW2_RAM_MASK 0x001fffffu
#define DW2_TASK_COUNT_OFFSET 0x00050798u
#define DW2_TASK_LIST_OFFSET 0x0005079cu
#define DW2_TASK_LIST_LIMIT 128u
#define DW2_TASK_TYPE_OFFSET 0x00u
#define DW2_TASK_STATE_OFFSET 0x10u
#define DW2_TASK_SUBSTATE_OFFSET 0x14u
#define DW2_TASK_DATA_OFFSET 0x2cu
#define DW2_DOMAIN_TASK_TYPE 0x0306u
#define DW2_CIRCLE_TASK_TYPE 0x000bu
#define DW2_COMMON_CHOICE_TASK_TYPE 0x000cu
#define DW2_STATUS_OVERVIEW_TASK_TYPE 0x000du
#define DW2_TARGET_GRID_TASK_TYPE 0x000eu
#define DW2_ITEM_LIST_TASK_TYPE 0x000fu
#define DW2_DIGIMON_LIST_TASK_TYPE 0x0010u
#define DW2_DIGIMON_STATUS_TASK_TYPE 0x0011u
#define DW2_CATEGORY_LIST_TASK_TYPE 0x0012u
#define DW2_DIGIMON_STATUS_ALT1_TASK_TYPE 0x0013u
#define DW2_DIGIMON_STATUS_ALT2_TASK_TYPE 0x0014u
#define DW2_DIGIMON_STATUS_ALT3_TASK_TYPE 0x0015u
#define DW2_SAVE_ROOT_TASK_TYPE 0x0600u
#define DW2_MEMORY_CARD_SLOT_TASK_TYPE 0x0603u
#define DW2_MEMORY_CARD_FILE_TASK_TYPE 0x0604u
#define DW2_MEMORY_CARD_DIGIMON_TASK_TYPE 0x0605u
#define DW2_MEMORY_CARD_SLOT_ALT_TASK_TYPE 0x0606u
#define DW2_SAVE_TASK_TYPE_LAST DW2_MEMORY_CARD_SLOT_ALT_TASK_TYPE
#define DW2_DIGIVOLVE_MESSAGE_TASK_TYPE 0x030du
#define DW2_DIGIVOLVE_MODE_TASK_TYPE 0x030bu
#define DW2_DIGIVOLVE_ROSTER_TASK_TYPE 0x030cu
#define DW2_DIGIVOLVE_PANEL_TASK_TYPE 0x030fu
#define DW2_ITEM_SHOP_BITS_TASK_TYPE 0x0315u
#define DW2_ITEM_SHOP_ROOT_TASK_TYPE 0x0316u
#define DW2_ITEM_SHOP_LIST_TASK_TYPE 0x0317u
#define DW2_STAG2000_TAG 0x00000192u
#define DW2_STAG3000_TAG 0x00000193u
#define DW2_STAG4000_TAG 0x0000019au
#define DW2_DOMAIN_SELECTED_OFFSET 0x08u
#define DW2_DOMAIN_RECORDS_OFFSET 0x0cu
#define DW2_DOMAIN_RECORD_SIZE 0x18u
#define DW2_DOMAIN_RECORD_LIMIT 64u
#define DW2_DOMAIN_RECORD_ID_OFFSET 0x00u
#define DW2_DOMAIN_RECORD_LABEL_OFFSET 0x04u
#define DW2_CIRCLE_CURSOR_FIRST_OFFSET 0x20u
#define DW2_CIRCLE_CURSOR_SECOND_OFFSET 0x22u
#define DW2_CIRCLE_BOUND_FIRST_OFFSET 0x24u
#define DW2_CIRCLE_BOUND_SECOND_OFFSET 0x26u
#define DW2_CIRCLE_STATE_POINTER_OFFSET 0x00050768u
#define DW2_CIRCLE_DOMAIN_FLAG 0x00000001u
#define DW2_CIRCLE_TRANSFER_FLAG 0x00000002u
#define DW2_CIRCLE_SAVE_FLAG 0x00000004u
#define DW2_CIRCLE_AUTOPILOT_FLAG 0x00000008u
#define DW2_CIRCLE_COMMON_DISABLED_FLAG 0x00000010u
#define DW2_TASK_BANKS_OFFSET 0x00040d50u
#define DW2_TASK_BANK_LIMIT 8u
#define DW2_TASK_DESCRIPTOR_CALLBACK_OFFSET 0x04u
#define DW2_TASK_DESCRIPTOR_DATA_SIZE_OFFSET 0x10u
#define DW2_RENDER_TASK_TYPE 0x0009u
#define DW2_RENDER_SLOT_SIZE 0x34u
#define DW2_RENDER_SLOT_LIMIT 50u
#define DW2_RENDER_SLOT_ACTIVE_OFFSET 0x00u
#define DW2_RENDER_SLOT_DISABLED_OFFSET 0x02u
#define DW2_RENDER_SLOT_TEXT_OFFSET 0x08u
#define DW2_RENDER_SLOT_ARGUMENTS_OFFSET 0x0cu
#define DW2_RENDER_SLOT_ARGUMENT_COUNT 4u
#define DW2_RENDER_SLOT_CHOICE_OFFSET 0x29u
#define DW2_RENDER_HANDLE_EMPTY 0xffffffffu
#define DW2_EMPTY_MARKER_GLYPH 0x49u
#define DW2_EMPTY_MARKER_LENGTH 8u
#define DW2_ITEM_RECORDS_OFFSET 0x72u
#define DW2_ITEM_RECORD_SIZE 6u
#define DW2_ITEM_RECORD_ID_OFFSET 0x00u
#define DW2_DIGIMON_RECORDS_OFFSET 0x6cu
#define DW2_DIGIMON_RECORD_SIZE 8u
#define DW2_DIGIMON_RECORD_TYPE_OFFSET 0x00u
#define DW2_DIGIMON_RECORD_POINTER_OFFSET 0x04u
#define DW2_DIGIMON_NAME_OFFSET 0x4cu
#define DW2_PLAYER_STATE_POINTER_OFFSET 0x00050720u
#define DW2_PLAYER_STATE_SIZE 0x2cu
#define DW2_PLAYER_BITS_OFFSET 0x08u
#define DW2_PLAYER_BEETLE_CURRENT_HP_OFFSET 0x24u
#define DW2_PLAYER_BEETLE_MAXIMUM_HP_OFFSET 0x26u
#define DW2_PLAYER_BEETLE_CURRENT_EP_OFFSET 0x28u
#define DW2_PLAYER_BEETLE_MAXIMUM_EP_OFFSET 0x2au
#define DW2_STATUS_BUG_HANDLES_OFFSET 0x58u
#define DW2_STATUS_BUG_HANDLE_COUNT 4u
#define DW2_STATUS_TRAINER_NAME_OFFSET 0x74u
#define DW2_STATUS_RANK_OFFSET 0x78u
#define DW2_STATUS_BEETLE_NAME_OFFSET 0x7cu
#define DW2_STATUS_PARTY_NAMES_OFFSET 0x80u
#define DW2_STATUS_PARTY_RECORDS_OFFSET 0xa0u
#define DW2_STATUS_PARTY_COUNT_OFFSET 0xacu
#define DW2_STATUS_PARTY_LIMIT 3u
#define DW2_DETAIL_RECORD_OFFSET 0x84u
#define DW2_DETAIL_SPECIES_OFFSET 0x88u
#define DW2_DETAIL_TYPE_OFFSET 0x8cu
#define DW2_DETAIL_LEVEL_OFFSET 0x90u
#define DW2_DETAIL_SPECIALTY_OFFSET 0x94u
#define DW2_DETAIL_PARENTS_OFFSET 0x98u
#define DW2_DETAIL_PARENT_LIMIT 2u
#define DW2_DETAIL_PARENT_TERMINATOR_OFFSET 0xa0u
#define DW2_DIGIMON_EL_OFFSET 0x0du
#define DW2_DIGIMON_DP_OFFSET 0x0eu
#define DW2_DIGIMON_MAXIMUM_EL_OFFSET 0x0fu
#define DW2_DIGIMON_EXP_OFFSET 0x10u
#define DW2_DIGIMON_MAXIMUM_HP_OFFSET 0x14u
#define DW2_DIGIMON_CURRENT_HP_OFFSET 0x16u
#define DW2_DIGIMON_MAXIMUM_MP_OFFSET 0x18u
#define DW2_DIGIMON_CURRENT_MP_OFFSET 0x1au
#define DW2_DIGIMON_ATTACK_OFFSET 0x1cu
#define DW2_DIGIMON_DEFENSE_OFFSET 0x1eu
#define DW2_DIGIMON_SPEED_OFFSET 0x20u
#define DW2_DIGIMON_SCALAR_SIZE 0x22u
#define DW2_DIGIMON_DETAIL_SIZE (DW2_DIGIMON_NAME_OFFSET + 1u)
#define DW2_MEMORY_CARD_DIGIMON_RECORD_LIMIT 36u
#define DW2_SAVE_PLAYER_NAME_OFFSET 0x14u
#define DW2_SAVE_PLAYER_NAME_SIZE 6u
#define DW2_ITEM_SHOP_FOCUS_OFFSET 0x00070a00u
#define DW2_ITEM_SHOP_MODE_OFFSET 0x00070a04u
#define DW2_ITEM_SHOP_IDS_OFFSET 0x00070a08u
#define DW2_ITEM_SHOP_ROW_LIMIT 8u
#define DW2_ITEM_SHOP_ITEM_LIMIT 50u
#define DW2_PLAYER_BITS_NATIVE_OFFSET 0x0005e628u
#define DW2_RESOURCE_SLOTS_OFFSET 0x0005f8c8u
#define DW2_RESOURCE_SLOT_STRIDE 0x10u
#define DW2_RESOURCE_SLOT_LIMIT 0x50u
#define DW2_ITEM_RESOURCE_GROUP 0x045eu
#define DW2_ITEM_RESOURCE_RECORD_SIZE 0x10u
#define DW2_ITEM_RESOURCE_RECORD_LIMIT 256u
#define DW2_DIGIVOLVE_PHASE_OFFSET 0x000709d4u
#define DW2_DIGIVOLVE_FOCUS_OFFSET 0x000709d8u
#define DW2_DIGIVOLVE_PAGE_OFFSET 0x000709c4u
#define DW2_DIGIVOLVE_ROW_OFFSET 0x000709c8u
#define DW2_DIGIVOLVE_MODE_OFFSET 0x000709d0u
#define DW2_DIGIVOLVE_PARTY_LIMIT 3u
#define DW2_DIGIVOLVE_ROSTER_LIMIT 36u
#define DW2_BATTLE_COMMAND_CALLBACK 0x80064b30u
#define DW2_BATTLE_CANNON_CALLBACK 0x80065594u
#define DW2_BATTLE_CANNON_TASK_DATA_SIZE 0xf8u
#define DW2_BATTLE_CANNON_CATEGORY_OFFSET 0x000737e8u
#define DW2_BATTLE_CANNON_CURSOR_OFFSET 0x000737f0u
#define DW2_BATTLE_CANNON_SCROLL_OFFSET 0x000737f8u
#define DW2_BATTLE_CANNON_CATEGORY_COUNT 3u
#define DW2_BATTLE_CANNON_ITEM_LIMIT 48u
#define DW2_BATTLE_TECHNIQUE_CALLBACK 0x80066698u
#define DW2_BATTLE_TARGET_CALLBACK 0x80066db0u
#define DW2_BATTLE_RESULTS_CALLBACK 0x800706bcu
#define DW2_BATTLE_LEARNED_TECHNIQUE_CALLBACK 0x800720e4u
#define DW2_BATTLE_COMMAND_TASK_DATA_SIZE 0x14u
#define DW2_BATTLE_TECHNIQUE_TASK_DATA_SIZE 0x54u
#define DW2_BATTLE_TARGET_TASK_DATA_SIZE 0x20u
#define DW2_BATTLE_RESULTS_TASK_DATA_SIZE 0x84u
#define DW2_BATTLE_LEARNED_TECHNIQUE_TASK_DATA_SIZE 0xccu
#define DW2_BATTLE_COMMAND_CURSOR_OFFSET 0x000737e0u
#define DW2_BATTLE_COMMAND_OWNER_OFFSET 0x00073cc8u
#define DW2_BATTLE_COMMAND_RESTRICTED_OFFSET 0x00073cc0u
#define DW2_BATTLE_COMMAND_OPEN_OFFSET 0x18u
#define DW2_BATTLE_TAMER_OWNER 6u
#define DW2_BATTLE_CATEGORY_OFFSET 0x00073800u
#define DW2_BATTLE_CATEGORY_CURSOR_OFFSET 0x00073808u
#define DW2_BATTLE_CATEGORY_SCROLL_OFFSET 0x00073810u
#define DW2_BATTLE_CATEGORY_RECORDS_OFFSET 0x00073820u
#define DW2_BATTLE_CATEGORY_COUNT 4u
#define DW2_BATTLE_CATEGORY_RECORD_SIZE 0x1bu
#define DW2_BATTLE_CATEGORY_TECHNIQUE_LIMIT 13u
#define DW2_BATTLE_CATEGORY_TECHNIQUE_ID_OFFSET 0x0du
#define DW2_BATTLE_CATEGORY_COUNT_OFFSET 0x1au
#define DW2_BATTLE_TECHNIQUE_RESOURCE_GROUP 0x025bu
#define DW2_BATTLE_TECHNIQUE_RECORD_SIZE 0x44u
#define DW2_BATTLE_TECHNIQUE_RECORD_LIMIT 256u
#define DW2_BATTLE_TECHNIQUE_MP_OFFSET 0x04u
#define DW2_BATTLE_TECHNIQUE_NAME_OFFSET 0x24u
#define DW2_BATTLE_TECHNIQUE_DESCRIPTION_OFFSET 0x28u
#define DW2_BATTLE_TARGET_SELECTED_OFFSET 0x04u
#define DW2_BATTLE_TARGET_LIMIT 6u
#define DW2_BATTLE_PARTICIPANTS_OFFSET 0x00073cd8u
#define DW2_BATTLE_PARTICIPANT_SIZE 0x5cu
#define DW2_BATTLE_PARTICIPANT_PRESENT_OFFSET 0x01u
#define DW2_BATTLE_PARTICIPANT_NAME_OFFSET 0x4cu
#define DW2_BATTLE_PARTICIPANT_NAME_SIZE 0x10u
#define DW2_BATTLE_ALLY_LIMIT 3u
#define DW2_BATTLE_PARTICIPANT_LIMIT 6u
#define DW2_BATTLE_SELECTED_TECHNIQUE_OFFSET 0x00073f72u
#define DW2_BATTLE_SELECTED_TECHNIQUE_STRIDE 0x10u
#define DW2_BATTLE_VALUE_STABLE_FRAMES 3u
#define DW2_BATTLE_NUMBER_TASK_TYPE 0x050du
#define DW2_BATTLE_NUMBER_CALLBACK 0x8006f8ecu
#define DW2_BATTLE_NUMBER_DATA_SIZE 0x14u
#define DW2_BATTLE_MODEL_TASK_TYPE 0x0509u
#define DW2_BATTLE_MODEL_CALLBACK 0x8006ef50u
#define DW2_BATTLE_MODEL_DATA_SIZE 0x3cu
#define DW2_BATTLE_EVENT_QUEUE_CAPACITY 32u
#define DW2_BATTLE_EVENT_TEXT_MAX 256u
#define DW2_BATTLE_LEARNED_PARTICIPANT_OFFSET 0x00u
#define DW2_BATTLE_LEARNED_TECHNIQUES_OFFSET 0x74u
#define DW2_BATTLE_LEARNED_PANE_OFFSET 0xa4u
#define DW2_BATTLE_LEARNED_CURSOR_OFFSET 0xa8u
#define DW2_BATTLE_LEARNED_SCROLL_OFFSET 0xb0u
#define DW2_BATTLE_LEARNED_COUNT_OFFSET 0xb8u
#define DW2_BATTLE_LEARNED_PANE_STRIDE 0x18u
#define DW2_BATTLE_LEARNED_LIST_LIMIT 12u

typedef struct dw2_grid_owner
{
   uint32_t task_address;
   uint32_t task_type;
   size_t data_offset;
   size_t data_size;
   size_t cursor_relative;
   size_t bounds_relative;
} dw2_grid_owner_t;

typedef struct dw2_battle_value_observation
{
   bool present;
   uint16_t observed_mp;
   uint16_t announced_mp;
   unsigned mp_stable_frames;
} dw2_battle_value_observation_t;

typedef struct dw2_battle_number_observation
{
   uint32_t task;
   size_t data;
   uint32_t kind;
   uint32_t amount;
   uint32_t icon;
   bool announced;
} dw2_battle_number_observation_t;

static bool dw2_best_valid;
static bool dw2_pending_valid;
static bool dw2_spoken_valid;
static unsigned dw2_pending_frames;
static beetle_dw2_menu_snapshot_t dw2_best;
static beetle_dw2_menu_snapshot_t dw2_pending;
static beetle_dw2_menu_snapshot_t dw2_spoken;
static bool dw2_battle_event_overlay_active;
static bool dw2_battle_resolution_active;
static bool dw2_battle_command_baseline_valid;
static dw2_battle_value_observation_t
   dw2_battle_value_observations[DW2_BATTLE_PARTICIPANT_LIMIT];
static char dw2_battle_event_queue[DW2_BATTLE_EVENT_QUEUE_CAPACITY]
   [DW2_BATTLE_EVENT_TEXT_MAX];
static size_t dw2_battle_event_queue_head;
static size_t dw2_battle_event_queue_count;
static dw2_battle_number_observation_t dw2_battle_numbers[DW2_TASK_LIST_LIMIT];
static size_t dw2_battle_number_count;

static void dw2_append_sentence(char *output, size_t output_size,
      const char *text);
static bool dw2_item_fields(const uint8_t *ram, size_t ram_size,
      uint16_t item_id, uint32_t *price, uint32_t *name_address,
      uint32_t *description_address);

static uint16_t dw2_read_u16(const uint8_t *ram, size_t offset)
{
   return (uint16_t)ram[offset] | ((uint16_t)ram[offset + 1] << 8);
}

static int16_t dw2_read_s16(const uint8_t *ram, size_t offset)
{
   return (int16_t)dw2_read_u16(ram, offset);
}

static uint32_t dw2_read_u32(const uint8_t *ram, size_t offset)
{
   return (uint32_t)ram[offset]
      | ((uint32_t)ram[offset + 1] << 8)
      | ((uint32_t)ram[offset + 2] << 16)
      | ((uint32_t)ram[offset + 3] << 24);
}

static bool dw2_ram_range(size_t ram_size, size_t offset, size_t length)
{
   return length && offset < ram_size && length <= ram_size - offset;
}

static bool dw2_address_to_offset(uint32_t address, size_t ram_size,
      size_t length, size_t *offset)
{
   uint32_t region = address & 0xffe00000u;
   size_t candidate;

   if (!offset || (region != 0x80000000u && region != 0xa0000000u))
      return false;
   candidate = address & DW2_RAM_MASK;
   if (!dw2_ram_range(ram_size, candidate, length))
      return false;
   *offset = candidate;
   return true;
}

static bool dw2_active_task(const uint8_t *ram, size_t ram_size,
      size_t index, uint32_t *task_address, size_t *task_offset)
{
   size_t entry_offset = DW2_TASK_LIST_OFFSET + index * 4u;
   uint32_t address;
   size_t offset;

   if (!dw2_ram_range(ram_size, entry_offset, 4))
      return false;
   address = dw2_read_u32(ram, entry_offset);
   if (!dw2_address_to_offset(address, ram_size, 0x38, &offset))
      return false;
   if (task_address)
      *task_address = address;
   if (task_offset)
      *task_offset = offset;
   return true;
}

static size_t dw2_active_task_count(const uint8_t *ram, size_t ram_size)
{
   uint32_t count;

   if (!dw2_ram_range(ram_size, DW2_TASK_COUNT_OFFSET, 4))
      return 0;
   count = dw2_read_u32(ram, DW2_TASK_COUNT_OFFSET);
   if (!count || count > DW2_TASK_LIST_LIMIT
         || !dw2_ram_range(ram_size, DW2_TASK_LIST_OFFSET,
               (size_t)count * 4u))
      return 0;
   return (size_t)count;
}

static bool dw2_find_first_active_task_type(const uint8_t *ram,
      size_t ram_size,
      uint32_t task_type, uint32_t *task_address, size_t *task_offset)
{
   size_t count = dw2_active_task_count(ram, ram_size);
   size_t index;

   for (index = 0; index < count; index++)
   {
      uint32_t candidate_address;
      size_t candidate_offset;

      if (!dw2_active_task(ram, ram_size, index, &candidate_address,
               &candidate_offset)
            || dw2_read_u32(ram, candidate_offset + DW2_TASK_TYPE_OFFSET)
               != task_type
            || dw2_read_u32(ram, candidate_offset + DW2_TASK_STATE_OFFSET)
               != 1u)
         continue;
      if (task_address)
         *task_address = candidate_address;
      if (task_offset)
         *task_offset = candidate_offset;
      return true;
   }
   return false;
}

static bool dw2_task_descriptor(const uint8_t *ram, size_t ram_size,
      uint32_t task_type, uint32_t *callback, size_t *data_size)
{
   uint32_t bank_index = task_type >> 8;
   uint32_t entry_index = task_type & 0xffu;
   uint32_t bank_address;
   uint32_t descriptor_address;
   uint32_t native_size;
   size_t bank_offset;
   size_t descriptor_offset;

   if ((!callback && !data_size) || bank_index >= DW2_TASK_BANK_LIMIT
         || !dw2_ram_range(ram_size,
               DW2_TASK_BANKS_OFFSET + bank_index * 4u, 4))
      return false;
   bank_address = dw2_read_u32(ram,
         DW2_TASK_BANKS_OFFSET + bank_index * 4u);
   if (!dw2_address_to_offset(bank_address, ram_size,
            ((size_t)entry_index + 1u) * 4u, &bank_offset))
      return false;
   descriptor_address = dw2_read_u32(ram,
         bank_offset + (size_t)entry_index * 4u);
   if (!dw2_address_to_offset(descriptor_address, ram_size,
            DW2_TASK_DESCRIPTOR_DATA_SIZE_OFFSET + 4u, &descriptor_offset))
      return false;
   native_size = dw2_read_u32(ram,
         descriptor_offset + DW2_TASK_DESCRIPTOR_DATA_SIZE_OFFSET);
   if (!native_size || native_size > 0x10000u)
      return false;
   if (callback)
      *callback = dw2_read_u32(ram,
            descriptor_offset + DW2_TASK_DESCRIPTOR_CALLBACK_OFFSET);
   if (data_size)
      *data_size = (size_t)native_size;
   return true;
}

static bool dw2_task_data_size(const uint8_t *ram, size_t ram_size,
      uint32_t task_type, size_t *data_size)
{
   return data_size
      && dw2_task_descriptor(ram, ram_size, task_type, NULL, data_size);
}

static bool dw2_find_renderer_data(const uint8_t *ram, size_t ram_size,
      size_t *renderer_data_offset)
{
   size_t count = dw2_active_task_count(ram, ram_size);
   size_t index;

   for (index = 0; index < count; index++)
   {
      size_t task_offset;
      uint32_t data_address;

      if (!dw2_active_task(ram, ram_size, index, NULL, &task_offset)
            || dw2_read_u32(ram, task_offset + DW2_TASK_TYPE_OFFSET)
               != DW2_RENDER_TASK_TYPE
            || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1)
         continue;
      data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
      if (dw2_address_to_offset(data_address, ram_size,
               DW2_RENDER_SLOT_SIZE * DW2_RENDER_SLOT_LIMIT,
               renderer_data_offset))
         return true;
   }
   return false;
}

static bool dw2_find_grid_owner(const uint8_t *ram, size_t ram_size,
      size_t cursor_offset, size_t bounds_offset, dw2_grid_owner_t *owner)
{
   size_t count = dw2_active_task_count(ram, ram_size);
   size_t index;

   if (!owner)
      return false;
   for (index = 0; index < count; index++)
   {
      uint32_t candidate_address;
      uint32_t type;
      uint32_t candidate_data_address;
      size_t task_offset;
      size_t candidate_data_offset;
      size_t candidate_data_size;

      if (!dw2_active_task(ram, ram_size, index,
               &candidate_address, &task_offset)
            || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1)
         continue;
      type = dw2_read_u32(ram, task_offset + DW2_TASK_TYPE_OFFSET);
      if (type == DW2_RENDER_TASK_TYPE || type == DW2_CIRCLE_TASK_TYPE
            || !dw2_task_data_size(ram, ram_size, type,
                  &candidate_data_size))
         continue;
      candidate_data_address = dw2_read_u32(ram,
            task_offset + DW2_TASK_DATA_OFFSET);
      if (!dw2_address_to_offset(candidate_data_address, ram_size,
               candidate_data_size, &candidate_data_offset)
            || candidate_data_size < 4u
            || cursor_offset < candidate_data_offset
            || cursor_offset > candidate_data_offset
               + candidate_data_size - 4u
            || bounds_offset < candidate_data_offset
            || bounds_offset > candidate_data_offset
               + candidate_data_size - 4u)
         continue;
      owner->task_address = candidate_address;
      owner->task_type = type;
      owner->data_offset = candidate_data_offset;
      owner->data_size = candidate_data_size;
      owner->cursor_relative = cursor_offset - candidate_data_offset;
      owner->bounds_relative = bounds_offset - candidate_data_offset;
      return true;
   }
   return false;
}

static bool dw2_submit_domain_selection(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   uint32_t data_address;
   uint32_t selected;
   size_t data_offset;
   size_t record_index;
   size_t record_offset = 0;
   uint32_t label_address;

   if (dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   if (!dw2_address_to_offset(data_address, ram_size,
            DW2_DOMAIN_RECORDS_OFFSET + DW2_DOMAIN_RECORD_SIZE, &data_offset))
      return false;
   selected = dw2_read_u32(ram, data_offset + DW2_DOMAIN_SELECTED_OFFSET);
   if (selected >= DW2_DOMAIN_RECORD_LIMIT)
      return false;

   for (record_index = 0; record_index < DW2_DOMAIN_RECORD_LIMIT;
         record_index++)
   {
      record_offset = data_offset + DW2_DOMAIN_RECORDS_OFFSET
         + record_index * DW2_DOMAIN_RECORD_SIZE;
      if (!dw2_ram_range(ram_size, record_offset, DW2_DOMAIN_RECORD_SIZE))
         return false;
      if (dw2_read_s16(ram, record_offset + DW2_DOMAIN_RECORD_ID_OFFSET) < 0)
         return false;
      if (record_index == selected)
         break;
   }
   if (record_index != selected)
      return false;

   label_address = dw2_read_u32(ram,
         record_offset + DW2_DOMAIN_RECORD_LABEL_OFFSET);
   memset(&snapshot, 0, sizeof(snapshot));
   if (!beetle_accessibility_dw2_decode_text(ram, ram_size, label_address,
            BEETLE_DW2_TEXT_MENU, snapshot.label, sizeof(snapshot.label)))
      return false;
   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 1;
   snapshot.context = BEETLE_DW2_MENU_DOMAIN_SELECT;
   snapshot.task = task_address;
   snapshot.focus_id = selected;
   strcpy(snapshot.title, "Domain Selection");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_circle_enabled(uint32_t focus, uint32_t flags)
{
   switch (focus)
   {
      case 0:
         return true;
      case 1:
      case 2:
      case 3:
         return (flags & DW2_CIRCLE_COMMON_DISABLED_FLAG) == 0;
      case 4:
         return (flags & DW2_CIRCLE_TRANSFER_FLAG) != 0;
      case 5:
         return (flags & DW2_CIRCLE_DOMAIN_FLAG)
            ? (flags & DW2_CIRCLE_AUTOPILOT_FLAG) != 0
            : (flags & DW2_CIRCLE_SAVE_FLAG) != 0;
      default:
         return false;
   }
}

static bool dw2_submit_domain_circle(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   uint32_t data_address;
   uint32_t state_address;
   uint32_t flags;
   uint32_t resource_id;
   uint32_t focus;
   size_t data_offset;
   size_t state_offset;
   int16_t first;
   int16_t second;
   int16_t first_bound;
   int16_t second_bound;
   const char *label;

   if (dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1
         || dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET) != 1)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   if (!dw2_address_to_offset(data_address, ram_size,
            DW2_CIRCLE_BOUND_SECOND_OFFSET + 2u, &data_offset))
      return false;
   first = dw2_read_s16(ram, data_offset + DW2_CIRCLE_CURSOR_FIRST_OFFSET);
   second = dw2_read_s16(ram, data_offset + DW2_CIRCLE_CURSOR_SECOND_OFFSET);
   first_bound = dw2_read_s16(ram,
         data_offset + DW2_CIRCLE_BOUND_FIRST_OFFSET);
   second_bound = dw2_read_s16(ram,
         data_offset + DW2_CIRCLE_BOUND_SECOND_OFFSET);
   if (first_bound != 2 || second_bound != 3 || first < 0 || second < 0
         || first >= first_bound || second >= second_bound)
      return false;
   focus = (uint32_t)(second_bound * first + second);

   if (!dw2_ram_range(ram_size, DW2_CIRCLE_STATE_POINTER_OFFSET, 4))
      return false;
   state_address = dw2_read_u32(ram, DW2_CIRCLE_STATE_POINTER_OFFSET);
   if (!dw2_address_to_offset(state_address, ram_size, 4, &state_offset))
      return false;
   flags = dw2_read_u32(ram, state_offset);
   resource_id = 0x01fd0061u + focus;
   if (focus == 5 && (flags & DW2_CIRCLE_DOMAIN_FLAG))
      resource_id = 0x01fd0067u;
   label = beetle_accessibility_dw2_icon_label(resource_id);
   if (!label)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   snapshot.active = true;
   snapshot.enabled = dw2_circle_enabled(focus, flags);
   snapshot.layer = 2;
   snapshot.context = (flags & DW2_CIRCLE_DOMAIN_FLAG)
      ? BEETLE_DW2_MENU_DOMAIN_CIRCLE : BEETLE_DW2_MENU_SYSTEM;
   snapshot.task = task_address;
   snapshot.focus_id = focus;
   strcpy(snapshot.title, (flags & DW2_CIRCLE_DOMAIN_FLAG)
         ? "Domain Menu" : "Menu");
   strcpy(snapshot.label, label);
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_owner_range(const dw2_grid_owner_t *owner,
      size_t relative, size_t length)
{
   return owner && length && relative < owner->data_size
      && length <= owner->data_size - relative;
}

static bool dw2_interactive_task_layout(uint32_t task_type,
      size_t *cursor_relative, size_t *bounds_relative,
      uint32_t *active_substate)
{
   size_t cursor;
   size_t bounds;
   uint32_t substate;

   switch (task_type)
   {
      case DW2_COMMON_CHOICE_TASK_TYPE:
         cursor = 0x28u;
         bounds = 0x2cu;
         substate = 1u;
         break;
      case DW2_TARGET_GRID_TASK_TYPE:
         cursor = 0x88u;
         bounds = 0x8cu;
         substate = 2u;
         break;
      case DW2_ITEM_LIST_TASK_TYPE:
         cursor = 0x54u;
         bounds = 0x58u;
         substate = 2u;
         break;
      case DW2_DIGIMON_LIST_TASK_TYPE:
         cursor = 0x50u;
         bounds = 0x54u;
         substate = 2u;
         break;
      case DW2_CATEGORY_LIST_TASK_TYPE:
         cursor = 0x54u;
         bounds = 0x64u;
         substate = 2u;
         break;
      default:
         return false;
   }

   if (cursor_relative)
      *cursor_relative = cursor;
   if (bounds_relative)
      *bounds_relative = bounds;
   if (active_substate)
      *active_substate = substate;
   return true;
}

static bool dw2_grid_handle_relative(const uint8_t *ram,
      const dw2_grid_owner_t *owner, uint32_t focus, uint32_t total,
      size_t *handle_relative)
{
   uint32_t category;
   uint32_t origin;
   uint32_t visible;
   int16_t flavor;
   int16_t scroll;
   size_t relative;

   if (!ram || !owner || !handle_relative || !total)
      return false;

   switch (owner->task_type)
   {
      case DW2_COMMON_CHOICE_TASK_TYPE:
         if (owner->cursor_relative != 0x28u
               || owner->bounds_relative != 0x2cu || focus >= 9u
               || !dw2_owner_range(owner, 0x38u, 2u))
            return false;
         /* FUN_80013558 writes the screen heading to handle zero. Transfer
          * flavors 7/8 then write a disabled Digimon/Items subtype before
          * their focusable Digi-Beetle and Server handles. */
         flavor = dw2_read_s16(ram, owner->data_offset + 0x38u);
         relative = ((size_t)focus
               + ((flavor == 7 || flavor == 8) ? 2u : 1u)) * 4u;
         break;

      case DW2_TARGET_GRID_TASK_TYPE:
         if (owner->cursor_relative != 0x88u
               || owner->bounds_relative != 0x8cu || focus >= 20u)
            return false;
         relative = (size_t)focus * 4u;
         break;

      case DW2_ITEM_LIST_TASK_TYPE:
         if (owner->cursor_relative != 0x54u
               || owner->bounds_relative != 0x58u
               || !dw2_owner_range(owner, 0x6eu, 2u))
            return false;
         scroll = dw2_read_s16(ram, owner->data_offset + 0x6eu);
         if (scroll < 0)
            return false;
         origin = (uint32_t)scroll * 8u;
         if (focus < origin || focus - origin >= 16u)
            return false;
         relative = (size_t)(focus - origin) * 4u;
         break;

      case DW2_DIGIMON_LIST_TASK_TYPE:
         if (owner->cursor_relative != 0x50u
               || owner->bounds_relative != 0x54u
               || !dw2_owner_range(owner, 0x68u, 2u))
            return false;
         scroll = dw2_read_s16(ram, owner->data_offset + 0x68u);
         if (scroll < 0 || focus < (uint32_t)scroll)
            return false;
         visible = focus - (uint32_t)scroll;
         if (visible >= 4u)
            return false;
         relative = 0x08u + (size_t)visible * 0x10u;
         break;

      case DW2_CATEGORY_LIST_TASK_TYPE:
         if (!dw2_owner_range(owner, 0x114u, 4u))
            return false;
         category = dw2_read_u32(ram, owner->data_offset + 0x114u);
         if (category >= 4u
               || owner->cursor_relative != 0x54u + (size_t)category * 4u
               || owner->bounds_relative != 0x64u
                  + (size_t)category * 0x0cu
               || !dw2_owner_range(owner,
                  0x94u + (size_t)category * 4u, 4u))
            return false;
         origin = dw2_read_u32(ram, owner->data_offset + 0x94u
               + (size_t)category * 4u);
         if (focus < origin || focus - origin >= 3u)
            return false;
         visible = focus - origin;
         relative = 0x18u + (size_t)category * 0x0cu
            + (size_t)visible * 4u;
         break;

      default:
         if (owner->bounds_relative != owner->cursor_relative + 4u
               || total > DW2_RENDER_SLOT_LIMIT
               || owner->cursor_relative < (size_t)total * 4u)
            return false;
         relative = owner->cursor_relative - (size_t)total * 4u
            + (size_t)focus * 4u;
         break;
   }

   if (!dw2_owner_range(owner, relative, 4u))
      return false;
   *handle_relative = relative;
   return true;
}

static bool dw2_renderer_slot_offset(const uint8_t *ram, size_t ram_size,
      size_t renderer_data_offset, uint32_t handle, size_t *slot_offset)
{
   size_t candidate;

   if (!slot_offset || handle >= DW2_RENDER_SLOT_LIMIT)
      return false;
   candidate = renderer_data_offset + (size_t)handle * DW2_RENDER_SLOT_SIZE;
   if (!dw2_ram_range(ram_size, candidate, DW2_RENDER_SLOT_SIZE)
         || !ram[candidate + DW2_RENDER_SLOT_ACTIVE_OFFSET])
      return false;
   *slot_offset = candidate;
   return true;
}

static void dw2_restore_x_button_text(char *text, size_t text_size)
{
   static const char *const needles[] = { "Press Button", "press Button" };
   static const char *const replacements[] = {
      "Press X Button", "press X Button"
   };
   size_t index;

   if (!text || !text_size)
      return;
   for (index = 0; index < sizeof(needles) / sizeof(needles[0]); index++)
   {
      char *match = strstr(text, needles[index]);
      size_t needle_length;
      size_t replacement_length;
      size_t text_length;

      if (!match)
         continue;
      needle_length = strlen(needles[index]);
      replacement_length = strlen(replacements[index]);
      text_length = strlen(text);
      if (replacement_length < needle_length
            || replacement_length - needle_length
               > text_size - text_length - 1u)
         return;
      memmove(match + replacement_length, match + needle_length,
            strlen(match + needle_length) + 1u);
      memcpy(match, replacements[index], replacement_length);
      return;
   }
}

static bool dw2_decode_renderer_handle(const uint8_t *ram, size_t ram_size,
      size_t renderer_data_offset, uint32_t handle, char *out,
      size_t out_size, bool *enabled)
{
   uint32_t arguments[DW2_RENDER_SLOT_ARGUMENT_COUNT];
   uint32_t text_address;
   size_t render_slot_offset;
   size_t index;

   if (!dw2_renderer_slot_offset(ram, ram_size, renderer_data_offset,
            handle, &render_slot_offset))
      return false;

   text_address = dw2_read_u32(ram,
         render_slot_offset + DW2_RENDER_SLOT_TEXT_OFFSET);
   for (index = 0; index < DW2_RENDER_SLOT_ARGUMENT_COUNT; index++)
      arguments[index] = dw2_read_u32(ram, render_slot_offset
            + DW2_RENDER_SLOT_ARGUMENTS_OFFSET + index * 4u);
   if (!beetle_accessibility_dw2_decode_formatted_text(ram, ram_size,
            text_address, arguments, DW2_RENDER_SLOT_ARGUMENT_COUNT,
            BEETLE_DW2_TEXT_MENU, out, out_size))
      return false;
   if (enabled)
      *enabled = ram[render_slot_offset
            + DW2_RENDER_SLOT_DISABLED_OFFSET] == 0;
   return true;
}

static bool dw2_decode_empty_renderer_handle(const uint8_t *ram,
      size_t ram_size, size_t renderer_data_offset, uint32_t handle,
      char *out, size_t out_size, bool *enabled)
{
   uint32_t text_address;
   size_t render_slot_offset;
   size_t text_offset;
   size_t index;

   if (!out || sizeof("Empty") > out_size
         || !dw2_renderer_slot_offset(ram, ram_size, renderer_data_offset,
               handle, &render_slot_offset))
      return false;
   text_address = dw2_read_u32(ram,
         render_slot_offset + DW2_RENDER_SLOT_TEXT_OFFSET);
   if (!dw2_address_to_offset(text_address, ram_size,
            DW2_EMPTY_MARKER_LENGTH + 1u, &text_offset))
      return false;
   for (index = 0; index < DW2_EMPTY_MARKER_LENGTH; index++)
      if (ram[text_offset + index] != DW2_EMPTY_MARKER_GLYPH)
         return false;
   if (ram[text_offset + DW2_EMPTY_MARKER_LENGTH] != 0xffu)
      return false;

   /* Resource 0x1FD0098 is the native visual empty marker "--------".
    * FUN_80015298 assigns it to every unoccupied target-grid position. */
   strcpy(out, "Empty");
   if (enabled)
      *enabled = ram[render_slot_offset
            + DW2_RENDER_SLOT_DISABLED_OFFSET] == 0;
   return true;
}

static const char *dw2_common_choice_graphic_label(const uint8_t *ram,
      const dw2_grid_owner_t *owner, uint32_t focus)
{
   int16_t flavor;

   if (!dw2_owner_range(owner, 0x38u, 2u))
      return NULL;
   flavor = dw2_read_s16(ram, owner->data_offset + 0x38u);

   /* Common-choice flavor 1 is the Status strip. FUN_80014870 draws these
    * three labels from native graphic resources rather than text handles. */
   if (flavor == 1)
   {
      switch (focus)
      {
         case 0: return "Conditions";
         case 1: return "Digi-Beetle";
         case 2: return "Important";
         default: return NULL;
      }
   }
   /* The native 0x513 resource table used by FUN_80014400 identifies the
    * graphic labels and exact focus order for these common-choice flavors.
    * Flavors 7/8 contain an extra non-focusable subtype handle. */
   if (flavor == 3 || flavor == 7 || flavor == 8)
   {
      if (focus == 0)
         return "Digi-Beetle";
      if (focus == 1)
         return "Server";
   }
   if (flavor == 4)
   {
      if (focus == 0)
         return "Digimon";
      if (focus == 1)
         return "Items";
   }
   return NULL;
}

static bool dw2_decode_item_record_label(const uint8_t *ram,
      const dw2_grid_owner_t *owner, uint32_t focus, char *out,
      size_t out_size)
{
   size_t record_relative = DW2_ITEM_RECORDS_OFFSET
      + (size_t)focus * DW2_ITEM_RECORD_SIZE;

   if (!dw2_owner_range(owner, record_relative, DW2_ITEM_RECORD_SIZE)
         || dw2_read_u16(ram, owner->data_offset + record_relative
            + DW2_ITEM_RECORD_ID_OFFSET) != 0
         || sizeof("Empty") > out_size)
      return false;
   /* FUN_80016198 renders item-id zero with resource 0x1FD0098, the
    * native visual empty marker "--------". */
   strcpy(out, "Empty");
   return true;
}

static bool dw2_decode_digimon_record_label(const uint8_t *ram,
      size_t ram_size, const dw2_grid_owner_t *owner, uint32_t focus,
      char *out, size_t out_size)
{
   uint32_t digimon_address;
   size_t record_relative;
   size_t record_offset;
   uint8_t record_type;

   record_relative = DW2_DIGIMON_RECORDS_OFFSET
      + (size_t)focus * DW2_DIGIMON_RECORD_SIZE;
   if (!dw2_owner_range(owner, record_relative, DW2_DIGIMON_RECORD_SIZE))
      return false;
   record_offset = owner->data_offset + record_relative;
   record_type = ram[record_offset + DW2_DIGIMON_RECORD_TYPE_OFFSET];
   /* FUN_80017D84 allocates no renderer text for a native type-0 row, while
    * FUN_800188BC keeps the visibly empty row in the focusable list. */
   if (record_type == 0)
   {
      if (sizeof("Empty") > out_size)
         return false;
      strcpy(out, "Empty");
      return true;
   }
   if (record_type != 1)
      return false;
   digimon_address = dw2_read_u32(ram,
         record_offset + DW2_DIGIMON_RECORD_POINTER_OFFSET);
   if (digimon_address > UINT32_MAX - DW2_DIGIMON_NAME_OFFSET)
      return false;
   return beetle_accessibility_dw2_decode_text(ram, ram_size,
         digimon_address + DW2_DIGIMON_NAME_OFFSET, BEETLE_DW2_TEXT_MENU,
         out, out_size);
}

static bool dw2_decode_grid_native_fallback(const uint8_t *ram,
      size_t ram_size, const dw2_grid_owner_t *owner, uint32_t focus,
      char *out, size_t out_size, bool *enabled)
{
   const char *label;

   if (!owner || !out || !out_size)
      return false;
   if (owner->task_type == DW2_COMMON_CHOICE_TASK_TYPE)
   {
      label = dw2_common_choice_graphic_label(ram, owner, focus);
      if (!label || strlen(label) + 1u > out_size)
         return false;
      strcpy(out, label);
   }
   else if (owner->task_type == DW2_ITEM_LIST_TASK_TYPE)
   {
      if (!dw2_decode_item_record_label(ram, owner, focus, out, out_size))
         return false;
   }
   else if (owner->task_type == DW2_DIGIMON_LIST_TASK_TYPE)
   {
      if (!dw2_decode_digimon_record_label(ram, ram_size, owner, focus,
               out, out_size))
         return false;
   }
   else
      return false;

   if (enabled)
      *enabled = true;
   return true;
}

static size_t dw2_grid_detail_relatives(uint32_t task_type,
      size_t relatives[3])
{
   switch (task_type)
   {
      case DW2_TARGET_GRID_TASK_TYPE:
         relatives[0] = 0x50u;
         relatives[1] = 0x54u;
         relatives[2] = 0x58u;
         return 3u;
      case DW2_ITEM_LIST_TASK_TYPE:
         relatives[0] = 0x44u;
         return 1u;
      case DW2_DIGIMON_LIST_TASK_TYPE:
         relatives[0] = 0x40u;
         relatives[1] = 0x44u;
         relatives[2] = 0x48u;
         return 3u;
      case DW2_CATEGORY_LIST_TASK_TYPE:
         relatives[0] = 0x48u;
         relatives[1] = 0x4cu;
         return 2u;
      default:
         return 0;
   }
}

static bool dw2_display_task_layout(uint32_t task_type,
      size_t *handle_count, size_t *ready_relative,
      uint32_t *active_substate)
{
   size_t count;
   size_t ready;
   uint32_t substate;

   switch (task_type)
   {
      case DW2_STATUS_OVERVIEW_TASK_TYPE:
         count = 26u;
         ready = 0x70u;
         substate = 1u;
         break;
      case DW2_DIGIMON_STATUS_TASK_TYPE:
      case DW2_DIGIMON_STATUS_ALT1_TASK_TYPE:
      case DW2_DIGIMON_STATUS_ALT2_TASK_TYPE:
      case DW2_DIGIMON_STATUS_ALT3_TASK_TYPE:
         count = 27u;
         ready = 0x80u;
         substate = 2u;
         break;
      default:
         return false;
   }

   if (handle_count)
      *handle_count = count;
   if (ready_relative)
      *ready_relative = ready;
   if (active_substate)
      *active_substate = substate;
   return true;
}

static bool dw2_decode_native_text_pointer(const uint8_t *ram,
      size_t ram_size, uint32_t address, char *out, size_t out_size)
{
   size_t offset;

   if (!out || !out_size
         || !dw2_address_to_offset(address, ram_size, 1u, &offset))
      return false;
   return beetle_accessibility_dw2_decode_text(ram, ram_size, address,
         BEETLE_DW2_TEXT_MENU, out, out_size);
}

static bool dw2_compose_sentence(char *output, size_t output_size,
      const char *text)
{
   size_t output_length;
   size_t text_length;
   size_t required;
   bool punctuation;
   unsigned char last;

   if (!output || !output_size || !text || !text[0])
      return false;
   output_length = strlen(output);
   text_length = strlen(text);
   last = (unsigned char)text[text_length - 1u];
   punctuation = last == '.' || last == '!' || last == '?';
   required = (output_length ? 1u : 0u) + text_length
      + (punctuation ? 0u : 1u);
   if (output_length >= output_size
         || required > output_size - output_length - 1u)
      return false;
   if (output_length)
      output[output_length++] = ' ';
   memcpy(output + output_length, text, text_length);
   output_length += text_length;
   if (!punctuation)
      output[output_length++] = '.';
   output[output_length] = '\0';
   return true;
}

static bool dw2_append_labeled_text(char *output, size_t output_size,
      const char *label, const char *value)
{
   char sentence[BEETLE_DW2_MENU_TEXT_MAX];
   int written;

   if (!label || !label[0] || !value || !value[0])
      return false;
   written = snprintf(sentence, sizeof(sentence), "%s %s", label, value);
   return written > 0 && (size_t)written < sizeof(sentence)
      && dw2_compose_sentence(output, output_size, sentence);
}

static bool dw2_append_labeled_unsigned(char *output, size_t output_size,
      const char *label, uint32_t value)
{
   char sentence[64];
   int written;

   if (!label || !label[0])
      return false;
   written = snprintf(sentence, sizeof(sentence), "%s %u", label,
         (unsigned)value);
   return written > 0 && (size_t)written < sizeof(sentence)
      && dw2_compose_sentence(output, output_size, sentence);
}

static bool dw2_append_current_maximum(char *output, size_t output_size,
      const char *label, uint32_t current, uint32_t maximum)
{
   char sentence[96];
   int written;

   if (!label || !label[0])
      return false;
   written = snprintf(sentence, sizeof(sentence), "%s %u of %u", label,
         (unsigned)current, (unsigned)maximum);
   return written > 0 && (size_t)written < sizeof(sentence)
      && dw2_compose_sentence(output, output_size, sentence);
}

static bool dw2_append_unique_bug_text(char *output, size_t output_size,
      const char *text, size_t offsets[DW2_STATUS_BUG_HANDLE_COUNT],
      size_t lengths[DW2_STATUS_BUG_HANDLE_COUNT], size_t *count)
{
   size_t output_length;
   size_t text_length;
   size_t index;

   if (!output || !output_size || !text || !text[0] || !offsets
         || !lengths || !count || *count > DW2_STATUS_BUG_HANDLE_COUNT)
      return false;
   text_length = strlen(text);
   for (index = 0; index < *count; index++)
      if (lengths[index] == text_length
            && !memcmp(output + offsets[index], text, text_length))
         return true;
   if (*count == DW2_STATUS_BUG_HANDLE_COUNT)
      return false;
   output_length = strlen(output);
   if (output_length >= output_size
         || text_length + (output_length ? 1u : 0u)
         > output_size - output_length - 1u)
      return false;
   if (output_length)
      output[output_length++] = ' ';
   offsets[*count] = output_length;
   lengths[*count] = text_length;
   (*count)++;
   memcpy(output + output_length, text, text_length + 1u);
   return true;
}

static uint32_t dw2_next_level(uint8_t el, uint8_t maximum_el,
      uint32_t experience)
{
   uint64_t level = el;
   uint64_t threshold;
   uint64_t step;

   /* FUN_8001E984 supplies the exact value drawn by FUN_80019214. */
   if (maximum_el <= el)
      return 99999999u;
   if (el > 61u)
      threshold = 0xc9cacu + (level - 61u) * 0xffffu;
   else if (el >= 31u)
   {
      step = level - 30u;
      threshold = step * 0x11e4u
         + (step * step * 0x0fu + step * step * step * 5u) * 4u
         + 0x7968u;
   }
   else if (el >= 21u)
   {
      step = level - 20u;
      threshold = step * step * 0x1eu + step * step * step * 10u
         + step * 0x4c4u + 0x16f8u;
   }
   else if (el >= 11u)
   {
      step = level - 10u;
      threshold = step * step * step * 10u / 3u
         + step * step * 10u + step * 0x6bu + 0x1e0u;
   }
   else
      threshold = level * level * level / 3u + level * level
         + level * 5u;
   if ((uint64_t)experience > threshold)
      return 0;
   return (uint32_t)(threshold - experience);
}

static bool dw2_submit_native_status_overview(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t data_offset,
      size_t data_size)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char bug_text[BEETLE_DW2_MENU_TEXT_MAX];
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   char row[BEETLE_DW2_MENU_TEXT_MAX];
   size_t bug_offsets[DW2_STATUS_BUG_HANDLE_COUNT];
   size_t bug_lengths[DW2_STATUS_BUG_HANDLE_COUNT];
   size_t bug_count = 0;
   size_t player_offset;
   size_t record_offset;
   size_t renderer_data_offset;
   uint32_t player_address;
   uint32_t record_address;
   int16_t party_count;
   size_t index;
   int written;

   if (data_size < DW2_STATUS_PARTY_COUNT_OFFSET + 2u
         || !dw2_ram_range(ram_size, DW2_PLAYER_STATE_POINTER_OFFSET, 4u))
      return false;
   player_address = dw2_read_u32(ram, DW2_PLAYER_STATE_POINTER_OFFSET);
   if (!dw2_address_to_offset(player_address, ram_size,
            DW2_PLAYER_STATE_SIZE, &player_offset))
      return false;
   party_count = dw2_read_s16(ram,
         data_offset + DW2_STATUS_PARTY_COUNT_OFFSET);
   if (party_count < 0 || party_count > (int16_t)DW2_STATUS_PARTY_LIMIT)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   /* FUN_80014CBC pairs these task strings with player and party scalars. */
   if (!dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_STATUS_TRAINER_NAME_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Name", text)
         || !dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_STATUS_RANK_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Rank", text)
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "BITs",
            dw2_read_u32(ram, player_offset + DW2_PLAYER_BITS_OFFSET))
         || !dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_STATUS_BEETLE_NAME_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Digi-Beetle", text)
         || !dw2_append_current_maximum(snapshot.details,
            sizeof(snapshot.details), "HP",
            dw2_read_u16(ram, player_offset
               + DW2_PLAYER_BEETLE_CURRENT_HP_OFFSET),
            dw2_read_u16(ram, player_offset
               + DW2_PLAYER_BEETLE_MAXIMUM_HP_OFFSET))
         || !dw2_append_current_maximum(snapshot.details,
            sizeof(snapshot.details), "EP",
            dw2_read_u16(ram, player_offset
               + DW2_PLAYER_BEETLE_CURRENT_EP_OFFSET),
            dw2_read_u16(ram, player_offset
               + DW2_PLAYER_BEETLE_MAXIMUM_EP_OFFSET)))
      return false;

   /* FUN_8001BC24 leaves every -1 sentinel unchanged if the renderer task is
    * missing, so renderer existence must be established before None. */
   if (!dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   bug_text[0] = '\0';
   for (index = 0; index < DW2_STATUS_BUG_HANDLE_COUNT; index++)
   {
      uint32_t handle = dw2_read_u32(ram, data_offset
            + DW2_STATUS_BUG_HANDLES_OFFSET + index * 4u);

      /* FUN_8001C088 initializes unallocated renderer handles to -1;
       * FUN_80014984 leaves that sentinel only for an empty Bug field. */
      if (handle == DW2_RENDER_HANDLE_EMPTY)
         continue;
      if (!dw2_decode_renderer_handle(ram, ram_size,
               renderer_data_offset, handle, text, sizeof(text), NULL)
            || !dw2_append_unique_bug_text(bug_text, sizeof(bug_text),
               text, bug_offsets, bug_lengths, &bug_count))
         return false;
   }
   if (!bug_count)
      strcpy(bug_text, "None");
   if (!dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Bug", bug_text))
      return false;

   for (index = 0; index < (size_t)party_count; index++)
   {
      if (!dw2_decode_native_text_pointer(ram, ram_size,
               dw2_read_u32(ram, data_offset
                  + DW2_STATUS_PARTY_NAMES_OFFSET + index * 4u),
               text, sizeof(text)))
         return false;
      record_address = dw2_read_u32(ram, data_offset
            + DW2_STATUS_PARTY_RECORDS_OFFSET + index * 4u);
      if (!dw2_address_to_offset(record_address, ram_size,
               DW2_DIGIMON_SCALAR_SIZE, &record_offset))
         return false;
      written = snprintf(row, sizeof(row),
            "Digimon %u %s, EL %u, HP %u of %u, MP %u of %u",
            (unsigned)index + 1u, text,
            (unsigned)ram[record_offset + DW2_DIGIMON_EL_OFFSET],
            (unsigned)dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_CURRENT_HP_OFFSET),
            (unsigned)dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_MAXIMUM_HP_OFFSET),
            (unsigned)dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_CURRENT_MP_OFFSET),
            (unsigned)dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_MAXIMUM_MP_OFFSET));
      if (written <= 0 || (size_t)written >= sizeof(row)
            || !dw2_compose_sentence(snapshot.details,
               sizeof(snapshot.details), row))
         return false;
   }

   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 4;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = DW2_STATUS_OVERVIEW_TASK_TYPE;
   strcpy(snapshot.label, "Status");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_native_digimon_status(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, uint32_t task_type,
      size_t data_offset, size_t data_size)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char first_parent[BEETLE_DW2_MENU_TEXT_MAX];
   char second_parent[BEETLE_DW2_MENU_TEXT_MAX];
   char parent_value[BEETLE_DW2_MENU_TEXT_MAX];
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   uint32_t record_address;
   uint32_t first_parent_address;
   uint32_t second_parent_address;
   uint32_t experience;
   size_t record_offset;
   uint8_t el;
   uint8_t maximum_el;
   int written;

   if (data_size < DW2_DETAIL_PARENT_TERMINATOR_OFFSET + 4u)
      return false;
   record_address = dw2_read_u32(ram,
         data_offset + DW2_DETAIL_RECORD_OFFSET);
   if (record_address > UINT32_MAX - DW2_DIGIMON_NAME_OFFSET
         || !dw2_address_to_offset(record_address, ram_size,
            DW2_DIGIMON_DETAIL_SIZE, &record_offset)
         || dw2_read_u32(ram,
            data_offset + DW2_DETAIL_PARENT_TERMINATOR_OFFSET) != 0u)
      return false;
   first_parent_address = dw2_read_u32(ram,
         data_offset + DW2_DETAIL_PARENTS_OFFSET);
   second_parent_address = dw2_read_u32(ram,
         data_offset + DW2_DETAIL_PARENTS_OFFSET + 4u);
   if (!first_parent_address && second_parent_address)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   /* FUN_80018D78 resolves the classification and compact parent pointers;
    * FUN_80019214 pairs them with these selected-record scalar values. */
   if (!dw2_decode_native_text_pointer(ram, ram_size,
            record_address + DW2_DIGIMON_NAME_OFFSET, text, sizeof(text))
         || !dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), text)
         || !dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_DETAIL_SPECIES_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Species", text)
         || !dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_DETAIL_TYPE_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Type", text)
         || !dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_DETAIL_LEVEL_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Level", text)
         || !dw2_decode_native_text_pointer(ram, ram_size,
            dw2_read_u32(ram, data_offset + DW2_DETAIL_SPECIALTY_OFFSET),
            text, sizeof(text))
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Specialty", text))
      return false;

   el = ram[record_offset + DW2_DIGIMON_EL_OFFSET];
   maximum_el = ram[record_offset + DW2_DIGIMON_MAXIMUM_EL_OFFSET];
   experience = dw2_read_u32(ram, record_offset + DW2_DIGIMON_EXP_OFFSET);
   if (!dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "EL", el)
         || !dw2_append_current_maximum(snapshot.details,
            sizeof(snapshot.details), "HP",
            dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_CURRENT_HP_OFFSET),
            dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_MAXIMUM_HP_OFFSET))
         || !dw2_append_current_maximum(snapshot.details,
            sizeof(snapshot.details), "MP",
            dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_CURRENT_MP_OFFSET),
            dw2_read_u16(ram, record_offset
               + DW2_DIGIMON_MAXIMUM_MP_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Attack",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_ATTACK_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Defense",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_DEFENSE_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Speed",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_SPEED_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "EXP", experience)
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Next Level",
            dw2_next_level(el, maximum_el, experience)))
      return false;

   if (!first_parent_address)
      strcpy(parent_value, "None");
   else
   {
      if (!dw2_decode_native_text_pointer(ram, ram_size,
               first_parent_address, first_parent, sizeof(first_parent)))
         return false;
      if (!second_parent_address)
         strcpy(parent_value, first_parent);
      else
      {
         if (!dw2_decode_native_text_pointer(ram, ram_size,
                  second_parent_address, second_parent,
                  sizeof(second_parent)))
            return false;
         written = snprintf(parent_value, sizeof(parent_value), "%s and %s",
               first_parent, second_parent);
         if (written <= 0 || (size_t)written >= sizeof(parent_value))
            return false;
      }
   }
   if (!dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Parents", parent_value))
      return false;

   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 4;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = task_type;
   strcpy(snapshot.label, "Digimon");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_native_display_task(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, uint32_t task_type,
      size_t data_offset, size_t data_size)
{
   if (task_type == DW2_STATUS_OVERVIEW_TASK_TYPE)
      return dw2_submit_native_status_overview(ram, ram_size, task_address,
            data_offset, data_size);
   if (task_type == DW2_DIGIMON_STATUS_TASK_TYPE
         || task_type == DW2_DIGIMON_STATUS_ALT1_TASK_TYPE
         || task_type == DW2_DIGIMON_STATUS_ALT2_TASK_TYPE
         || task_type == DW2_DIGIMON_STATUS_ALT3_TASK_TYPE)
      return dw2_submit_native_digimon_status(ram, ram_size, task_address,
            task_type, data_offset, data_size);
   return false;
}

static bool dw2_submit_display_task(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset, uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   size_t detail_offsets[27];
   size_t detail_lengths[27];
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   uint32_t data_address;
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   size_t handle_count;
   size_t ready_relative;
   uint32_t active_substate;
   size_t index;
   size_t detail_count = 0;
   bool have_label = false;

   if (!dw2_display_task_layout(task_type, &handle_count, &ready_relative,
            &active_substate)
         || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1
         || dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET)
            != active_substate
         || !dw2_task_data_size(ram, ram_size, task_type, &data_size)
         || handle_count * 4u > data_size
         || ready_relative > data_size - 4u)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   /* FUN_80014984/FUN_80018D78 use ready_relative as a native window
    * handle. Reaching the task's active substate is the readiness signal;
    * the handle is not a 0x1000 tween counter. */
   if (!dw2_address_to_offset(data_address, ram_size, data_size, &data_offset))
      return false;
   if (dw2_submit_native_display_task(ram, ram_size, task_address,
            task_type, data_offset, data_size))
      return true;
   if (!dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   for (index = 0; index < handle_count; index++)
   {
      uint32_t handle = dw2_read_u32(ram, data_offset + index * 4u);
      if (!dw2_decode_renderer_handle(ram, ram_size, renderer_data_offset,
               handle, text, sizeof(text), NULL))
         continue;
      if (!have_label)
      {
         strcpy(snapshot.label, text);
         have_label = true;
      }
      else
      {
         size_t text_length = strlen(text);
         size_t current_length = strlen(snapshot.details);
         size_t detail_start = current_length ? current_length + 1u : 0u;
         size_t required = detail_start + text_length + 1u;
         size_t detail_index;
         unsigned char last = (unsigned char)text[text_length - 1u];
         bool duplicate = !strcmp(snapshot.label, text);

         if (last != '.' && last != '!' && last != '?')
            required++;
         for (detail_index = 0; !duplicate && detail_index < detail_count;
               detail_index++)
            if (detail_lengths[detail_index] == text_length
                  && !memcmp(snapshot.details
                     + detail_offsets[detail_index], text, text_length))
               duplicate = true;
         if (duplicate)
            continue;
         if (required > sizeof(snapshot.details))
            break;
         dw2_append_sentence(snapshot.details, sizeof(snapshot.details), text);
         detail_offsets[detail_count] = detail_start;
         detail_lengths[detail_count++] = text_length;
      }
   }
   if (!have_label)
      return false;

   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 4;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = task_type;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_grid_owner(const uint8_t *ram, size_t ram_size,
      const dw2_grid_owner_t *owner, size_t cursor_offset,
      size_t bounds_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   uint32_t handle;
   uint32_t focus;
   size_t detail_relatives[3];
   char seen_details[3][BEETLE_DW2_MENU_TEXT_MAX];
   char detail[BEETLE_DW2_MENU_TEXT_MAX];
   size_t detail_count;
   size_t detail_index;
   size_t seen_count = 0;
   size_t seen_index;
   size_t handle_relative;
   size_t handle_offset;
   size_t renderer_data_offset;
   size_t renderer_slot_offset;
   int16_t first;
   int16_t second;
   int16_t first_bound;
   int16_t second_bound;
   uint32_t total;
   bool native_label = false;

   if (!owner || !dw2_ram_range(ram_size, cursor_offset, 4u)
         || !dw2_ram_range(ram_size, bounds_offset, 4u))
      return false;
   first = dw2_read_s16(ram, cursor_offset);
   second = dw2_read_s16(ram, cursor_offset + 2u);
   first_bound = dw2_read_s16(ram, bounds_offset);
   second_bound = dw2_read_s16(ram, bounds_offset + 2u);
   if (first < 0 || second < 0 || first_bound <= 0 || second_bound <= 0
         || first >= first_bound || second >= second_bound)
      return false;
   total = (uint32_t)first_bound * (uint32_t)second_bound;
   if (!total)
      return false;
   focus = (uint32_t)second_bound * (uint32_t)first + (uint32_t)second;
   if (!dw2_grid_handle_relative(ram, owner, focus, total,
             &handle_relative))
      return false;
   handle_offset = owner->data_offset + handle_relative;
   if (!dw2_ram_range(ram_size, handle_offset, 4))
      return false;
   handle = dw2_read_u32(ram, handle_offset);
   if (!dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   if (owner->task_type == DW2_COMMON_CHOICE_TASK_TYPE)
      native_label = dw2_decode_grid_native_fallback(ram, ram_size, owner,
            focus, snapshot.label, sizeof(snapshot.label), &snapshot.enabled);
   if (native_label)
   {
      /* Graphic labels still use the selected native renderer slot's
       * disabled flag when that slot is live. */
      if (dw2_renderer_slot_offset(ram, ram_size, renderer_data_offset,
               handle, &renderer_slot_offset))
         snapshot.enabled = ram[renderer_slot_offset
               + DW2_RENDER_SLOT_DISABLED_OFFSET] == 0;
   }
   else if (!dw2_decode_renderer_handle(ram, ram_size, renderer_data_offset,
               handle, snapshot.label, sizeof(snapshot.label),
               &snapshot.enabled)
         && !(owner->task_type == DW2_TARGET_GRID_TASK_TYPE
            && dw2_decode_empty_renderer_handle(ram, ram_size,
               renderer_data_offset, handle, snapshot.label,
               sizeof(snapshot.label), &snapshot.enabled))
         && !dw2_decode_grid_native_fallback(ram, ram_size, owner, focus,
            snapshot.label, sizeof(snapshot.label), &snapshot.enabled))
      return false;

   detail_count = dw2_grid_detail_relatives(owner->task_type,
         detail_relatives);
   for (detail_index = 0; detail_index < detail_count; detail_index++)
   {
      size_t relative = detail_relatives[detail_index];
      uint32_t detail_handle;
      bool duplicate = false;

      if (!dw2_owner_range(owner, relative, 4u))
         continue;
      detail_handle = dw2_read_u32(ram, owner->data_offset + relative);
      if (!dw2_decode_renderer_handle(ram, ram_size, renderer_data_offset,
               detail_handle, detail, sizeof(detail), NULL)
            || !strcmp(detail, snapshot.label))
         continue;
      for (seen_index = 0; seen_index < seen_count; seen_index++)
         if (!strcmp(detail, seen_details[seen_index]))
         {
            duplicate = true;
            break;
         }
      if (duplicate)
         continue;
      strcpy(seen_details[seen_count++], detail);
      dw2_append_sentence(snapshot.details, sizeof(snapshot.details), detail);
   }

   snapshot.active = true;
   snapshot.layer = 3;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = owner->task_address;
   snapshot.focus_id = ((uint32_t)owner->cursor_relative << 16) | focus;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_common_grid(const uint8_t *ram, size_t ram_size,
      const beetle_dw2_menu_probe_t *probe)
{
   dw2_grid_owner_t owner;
   size_t cursor_offset;
   size_t bounds_offset;

   if (!probe || probe->adapter != BEETLE_DW2_PROFILE_ADAPTER_GRID
         || !dw2_address_to_offset(probe->task, ram_size, 4,
               &cursor_offset)
         || !dw2_address_to_offset(probe->detail_ref, ram_size, 4,
               &bounds_offset)
         || !dw2_find_grid_owner(ram, ram_size, cursor_offset,
               bounds_offset, &owner))
      return false;
   return dw2_submit_grid_owner(ram, ram_size, &owner, cursor_offset,
         bounds_offset);
}

static bool dw2_submit_interactive_task(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   dw2_grid_owner_t owner;
   uint32_t active_substate;
   uint32_t data_address;
   size_t data_offset;
   size_t data_size;
   size_t cursor_relative;
   size_t bounds_relative;
   uint32_t category;

   if (!dw2_interactive_task_layout(task_type, &cursor_relative,
            &bounds_relative, &active_substate)
         || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1
         || dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET)
            != active_substate
         || !dw2_task_data_size(ram, ram_size, task_type, &data_size)
         || data_size < 4u)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   if (!dw2_address_to_offset(data_address, ram_size, data_size,
            &data_offset))
      return false;
   if (task_type == DW2_CATEGORY_LIST_TASK_TYPE)
   {
      if (0x114u > data_size - 4u)
         return false;
      category = dw2_read_u32(ram, data_offset + 0x114u);
      if (category >= 4u)
         return false;
      cursor_relative += (size_t)category * 4u;
      bounds_relative += (size_t)category * 0x0cu;
   }
   if (cursor_relative > data_size - 4u
         || bounds_relative > data_size - 4u)
      return false;

   owner.task_address = task_address;
   owner.task_type = task_type;
   owner.data_offset = data_offset;
   owner.data_size = data_size;
   owner.cursor_relative = cursor_relative;
   owner.bounds_relative = bounds_relative;
   return dw2_submit_grid_owner(ram, ram_size, &owner,
         data_offset + cursor_relative, data_offset + bounds_relative);
}

static bool dw2_renderer_yes_no_choice(const uint8_t *ram, size_t ram_size,
      size_t renderer_data_offset, uint32_t handle, uint8_t *choice)
{
   uint32_t text_address;
   size_t slot_offset;
   size_t text_offset;
   size_t index;
   bool saw_yes = false;
   bool saw_no = false;
   bool saw_end = false;
   bool terminated = false;

   if (!choice || !dw2_renderer_slot_offset(ram, ram_size,
            renderer_data_offset, handle, &slot_offset))
      return false;
   text_address = dw2_read_u32(ram,
         slot_offset + DW2_RENDER_SLOT_TEXT_OFFSET);
   if (!dw2_address_to_offset(text_address, ram_size, 1u, &text_offset))
      return false;

   /* Renderer command F8 stores the native yes/no highlight markers as
    * 1 (Yes), 2 (No), and 0 (input terminator). */
   for (index = 0; index < 160u && text_offset + index < ram_size;)
   {
      uint8_t value = ram[text_offset + index];
      size_t command_size;

      if (value == 0xffu)
      {
         terminated = true;
         break;
      }
      if (value < 0xefu || value == 0xfbu || value == 0xfcu
            || value == 0xfdu || value == 0xfeu)
      {
         index++;
         continue;
      }

      switch (value)
      {
         case 0xefu:
         case 0xf0u:
         case 0xf3u:
         case 0xfau:
            command_size = 2u;
            break;
         case 0xf1u:
         case 0xf2u:
            command_size = 4u;
            break;
         case 0xf4u:
            if (index + 1u >= 160u
                  || text_offset + index + 1u >= ram_size)
               return false;
            command_size = ram[text_offset + index + 1u] < 0x30u
               ? 5u : 2u;
            break;
         case 0xf5u:
            command_size = 1u;
            break;
         case 0xf6u:
         case 0xf7u:
            command_size = 10u;
            break;
         case 0xf8u:
            command_size = 2u;
            if (index + 1u >= 160u
                  || text_offset + index + 1u >= ram_size)
               return false;
            value = ram[text_offset + index + 1u];
            if (value == 1u)
               saw_yes = true;
            else if (value == 2u)
               saw_no = true;
            else if (value == 0u)
               saw_end = true;
            break;
         case 0xf9u:
            if (index + 1u >= 160u
                  || text_offset + index + 1u >= ram_size)
               return false;
            command_size = (ram[text_offset + index + 1u] & 1u)
               ? 2u : 5u;
            break;
         default:
            return false;
      }
      if (command_size > 160u - index
            || command_size > ram_size - (text_offset + index))
         return false;
      index += command_size;
   }
   if (!terminated || !saw_yes || !saw_no || !saw_end
         || ram[slot_offset + DW2_RENDER_SLOT_CHOICE_OFFSET] > 1u)
      return false;
   *choice = ram[slot_offset + DW2_RENDER_SLOT_CHOICE_OFFSET];
   return true;
}

static bool dw2_memory_card_task_data(const uint8_t *ram, size_t ram_size,
      size_t task_offset, uint32_t task_type, size_t minimum_size,
      size_t *data_offset, size_t *data_size)
{
   uint32_t data_address;
   size_t native_size;

   if (!data_offset
         || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1
         || !dw2_task_data_size(ram, ram_size, task_type, &native_size)
         || native_size < minimum_size)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   if (!dw2_address_to_offset(data_address, ram_size, native_size,
            data_offset))
      return false;
   if (data_size)
      *data_size = native_size;
   return true;
}

static bool dw2_memory_card_grid_focus(const uint8_t *ram,
      size_t data_offset, size_t data_size, size_t cursor_relative,
      size_t bounds_relative, uint32_t maximum, uint32_t *focus)
{
   int16_t first;
   int16_t second;
   int16_t first_bound;
   int16_t second_bound;
   uint32_t total;

   if (!focus || cursor_relative > data_size - 4u
         || bounds_relative > data_size - 4u)
      return false;
   first = dw2_read_s16(ram, data_offset + cursor_relative);
   second = dw2_read_s16(ram, data_offset + cursor_relative + 2u);
   first_bound = dw2_read_s16(ram, data_offset + bounds_relative);
   second_bound = dw2_read_s16(ram, data_offset + bounds_relative + 2u);
   if (first < 0 || second < 0 || first_bound <= 0 || second_bound <= 0
         || first >= first_bound || second >= second_bound)
      return false;
   total = (uint32_t)first_bound * (uint32_t)second_bound;
   *focus = (uint32_t)second_bound * (uint32_t)first
      + (uint32_t)second;
   return total && total <= maximum && *focus < maximum;
}

static bool dw2_memory_card_decode_handle(const uint8_t *ram,
      size_t ram_size, size_t renderer_data_offset, size_t data_offset,
      size_t data_size, size_t relative, char *out, size_t out_size,
      bool *enabled, uint32_t *handle_out)
{
   uint32_t handle;

   if (data_size < 4u || relative > data_size - 4u)
      return false;
   handle = dw2_read_u32(ram, data_offset + relative);
   if (!dw2_decode_renderer_handle(ram, ram_size, renderer_data_offset,
            handle, out, out_size, enabled))
      return false;
   if (handle_out)
      *handle_out = handle;
   return true;
}

static bool dw2_memory_card_handle_visible(const uint8_t *ram,
      size_t ram_size, size_t renderer_data_offset, size_t data_offset,
      size_t data_size, size_t relative, bool *enabled,
      uint32_t *handle_out)
{
   uint32_t handle;
   size_t slot_offset;

   if (data_size < 4u || relative > data_size - 4u)
      return false;
   handle = dw2_read_u32(ram, data_offset + relative);
   if (!dw2_renderer_slot_offset(ram, ram_size, renderer_data_offset,
            handle, &slot_offset))
      return false;
   if (enabled)
      *enabled = ram[slot_offset + DW2_RENDER_SLOT_DISABLED_OFFSET] == 0;
   if (handle_out)
      *handle_out = handle;
   return true;
}

static bool dw2_resource_group_base(const uint8_t *ram, size_t ram_size,
      uint32_t group, size_t *base_offset)
{
   size_t slot_index;

   if (!base_offset)
      return false;
   for (slot_index = 0; slot_index < DW2_RESOURCE_SLOT_LIMIT; slot_index++)
   {
      size_t slot_offset = DW2_RESOURCE_SLOTS_OFFSET
         + slot_index * DW2_RESOURCE_SLOT_STRIDE;
      uint32_t base_address;

      if (!dw2_ram_range(ram_size, slot_offset, DW2_RESOURCE_SLOT_STRIDE))
         return false;
      if (dw2_read_u32(ram, slot_offset + 4u) != group)
         continue;
      base_address = dw2_read_u32(ram, slot_offset + 0x0cu);
      if (dw2_address_to_offset(base_address, ram_size, 4u, base_offset))
         return true;
   }
   return false;
}

static bool dw2_battle_task_data(const uint8_t *ram, size_t ram_size,
      size_t task_offset, uint32_t expected_callback,
      size_t expected_data_size, bool require_visible_substate,
      size_t *data_offset)
{
   uint32_t task_type;
   uint32_t callback;
   uint32_t data_address;
   size_t data_size;

   if (!data_offset
         || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1u
         || (require_visible_substate
            && dw2_read_u32(ram,
               task_offset + DW2_TASK_SUBSTATE_OFFSET) != 1u))
      return false;
   task_type = dw2_read_u32(ram, task_offset + DW2_TASK_TYPE_OFFSET);
   if (!dw2_task_descriptor(ram, ram_size, task_type, &callback, &data_size)
         || callback != expected_callback
         || data_size != expected_data_size)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   return dw2_address_to_offset(data_address, ram_size, data_size,
         data_offset);
}

static bool dw2_battle_technique_fields(const uint8_t *ram,
      size_t ram_size, uint8_t technique_id, uint8_t *mp,
      char *name, size_t name_size, char *description,
      size_t description_size)
{
   size_t base_offset;
   size_t table_offset;
   size_t record_index;
   uint32_t table_relative;

   if (!technique_id || !mp || !name || !name_size
         || !dw2_resource_group_base(ram, ram_size,
            DW2_BATTLE_TECHNIQUE_RESOURCE_GROUP, &base_offset))
      return false;
   table_relative = dw2_read_u32(ram, base_offset);
   if (table_relative >= ram_size - base_offset)
      return false;
   table_offset = base_offset + table_relative;

   for (record_index = 0;
         record_index < DW2_BATTLE_TECHNIQUE_RECORD_LIMIT;
         record_index++)
   {
      size_t record_offset = table_offset
         + record_index * DW2_BATTLE_TECHNIQUE_RECORD_SIZE;
      int16_t candidate_id;

      if (!dw2_ram_range(ram_size, record_offset,
               DW2_BATTLE_TECHNIQUE_RECORD_SIZE))
         return false;
      candidate_id = dw2_read_s16(ram, record_offset);
      if (!candidate_id)
         return false;
      if (candidate_id < 0)
         return false;
      if ((uint16_t)candidate_id == technique_id)
      {
         uint32_t name_relative = dw2_read_u32(ram, record_offset
            + DW2_BATTLE_TECHNIQUE_NAME_OFFSET);
         size_t name_offset;

         if (!name_relative || name_relative >= ram_size - base_offset)
            return false;
         name_offset = base_offset + name_relative;
         if (!dw2_ram_range(ram_size, name_offset, 1u)
               || !beetle_accessibility_dw2_decode_text(ram, ram_size,
                  0x80000000u | (uint32_t)name_offset,
                  BEETLE_DW2_TEXT_MENU, name, name_size))
            return false;
         *mp = ram[record_offset + DW2_BATTLE_TECHNIQUE_MP_OFFSET];
         if (description && description_size)
         {
            uint32_t relative = dw2_read_u32(ram, record_offset
               + DW2_BATTLE_TECHNIQUE_DESCRIPTION_OFFSET);

            /* 80066484 renders the selected move's help through 800663f8
             * and 8001edd4: group 025b base + record's relative +28. */
            if (!relative || relative >= ram_size - base_offset
                  || !beetle_accessibility_dw2_decode_text(ram, ram_size,
                     0x80000000u | (uint32_t)(base_offset + relative),
                     BEETLE_DW2_TEXT_MENU, description, description_size))
               snprintf(description, description_size,
                     "Description unavailable.");
         }
         return true;
      }
   }
   return false;
}

static bool dw2_battle_participant_name(const uint8_t *ram,
      size_t ram_size, uint32_t participant, char *name,
      size_t name_size, size_t *participant_offset)
{
   size_t offset;
   size_t name_index;
   bool terminated = false;

   if (!ram || !name || !name_size
         || participant >= DW2_BATTLE_PARTICIPANT_LIMIT)
      return false;
   offset = DW2_BATTLE_PARTICIPANTS_OFFSET
      + (size_t)participant * DW2_BATTLE_PARTICIPANT_SIZE;
   if (!dw2_ram_range(ram_size, offset, DW2_BATTLE_PARTICIPANT_SIZE)
         || ram[offset + DW2_BATTLE_PARTICIPANT_PRESENT_OFFSET] == 0)
      return false;
   for (name_index = 0;
         name_index < DW2_BATTLE_PARTICIPANT_NAME_SIZE; name_index++)
      if (ram[offset + DW2_BATTLE_PARTICIPANT_NAME_OFFSET
               + name_index] == 0xffu)
      {
         terminated = true;
         break;
      }
   if (!terminated
         || !beetle_accessibility_dw2_decode_text(ram, ram_size,
            0x80000000u | (uint32_t)(offset
               + DW2_BATTLE_PARTICIPANT_NAME_OFFSET),
            BEETLE_DW2_TEXT_MENU, name, name_size))
      return false;
   if (participant_offset)
      *participant_offset = offset;
   return true;
}

static bool dw2_submit_battle_command(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset)
{
   static const char *const tamer_commands[] = {
      "Give Orders", "Cannon", "Run Away"
   };
   static const char *const digimon_commands[] = { "Battle", "Guard" };
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   uint32_t owner;
   uint32_t cursor;
   unsigned count;
   int written;

   if (!dw2_battle_task_data(ram, ram_size, task_offset,
            DW2_BATTLE_COMMAND_CALLBACK, DW2_BATTLE_COMMAND_TASK_DATA_SIZE,
            false, &data_offset)
         || dw2_read_u32(ram,
            task_offset + DW2_BATTLE_COMMAND_OPEN_OFFSET) != 1u
         || !dw2_ram_range(ram_size, DW2_BATTLE_COMMAND_OWNER_OFFSET, 4u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_COMMAND_CURSOR_OFFSET, 4u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_COMMAND_RESTRICTED_OFFSET,
            4u))
      return false;
   (void)data_offset;

   /* The native command controller uses the owner, not 73cd4's child
    * return flag, to choose the three tamer or two Digimon commands.
    * 737e8 belongs to the separate cannon inventory controller. */
   owner = dw2_read_u32(ram, DW2_BATTLE_COMMAND_OWNER_OFFSET);
   cursor = dw2_read_u32(ram, DW2_BATTLE_COMMAND_CURSOR_OFFSET);
   if (owner > DW2_BATTLE_TAMER_OWNER)
      return false;
   count = owner == DW2_BATTLE_TAMER_OWNER ? 3u : 2u;
   if (cursor >= count)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   snapshot.enabled = true;
   if (owner == DW2_BATTLE_TAMER_OWNER)
   {
      strcpy(snapshot.label, tamer_commands[cursor]);
      snapshot.enabled = cursor == 0u
         || dw2_read_u32(ram, DW2_BATTLE_COMMAND_RESTRICTED_OFFSET) == 0u;
   }
   else
   {
      char name[128];

      if (!dw2_battle_participant_name(ram, ram_size, owner,
               name, sizeof(name), NULL))
         snprintf(name, sizeof(name), "Digimon %u", (unsigned)(owner + 1u));
      written = snprintf(snapshot.label, sizeof(snapshot.label),
            "%s. %s", name, digimon_commands[cursor]);
      if (written < 0 || (size_t)written >= sizeof(snapshot.label))
         return false;
   }
   written = snprintf(snapshot.details, sizeof(snapshot.details),
         "%u of %u.", (unsigned)(cursor + 1u), count);
   if (written < 0 || (size_t)written >= sizeof(snapshot.details))
      return false;

   snapshot.active = true;
   snapshot.layer = 6;
   snapshot.context = BEETLE_DW2_MENU_BATTLE;
   snapshot.task = task_address;
   snapshot.focus_id = 0x43000000u | (owner << 16) | cursor;
   strcpy(snapshot.title, "Battle Commands");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_battle_results(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char experience_reward[BEETLE_DW2_MENU_TEXT_MAX];
   char bits_reward[BEETLE_DW2_MENU_TEXT_MAX];
   char participant_name[BEETLE_DW2_MENU_TEXT_MAX];
   char row[BEETLE_DW2_MENU_TEXT_MAX];
   size_t data_offset;
   size_t renderer_data_offset;
   uint32_t experience_handle = DW2_RENDER_HANDLE_EMPTY;
   uint32_t bits_handle = DW2_RENDER_HANDLE_EMPTY;
   uint32_t participant;
   uint32_t handle;

   if (!dw2_battle_task_data(ram, ram_size, task_offset,
            DW2_BATTLE_RESULTS_CALLBACK, DW2_BATTLE_RESULTS_TASK_DATA_SIZE,
            false, &data_offset)
         || !dw2_find_renderer_data(ram, ram_size,
            &renderer_data_offset)
         || !dw2_ram_range(ram_size, DW2_PLAYER_BITS_NATIVE_OFFSET, 4u))
      return false;

   /* FUN_8007040C uses the result task's data block for animation state.
    * The values a sighted player sees are the already-formatted native text
    * renderer slots, including each reward's current numeric argument. */
   (void)data_offset;
   experience_reward[0] = '\0';
   bits_reward[0] = '\0';
   memset(&snapshot, 0, sizeof(snapshot));
   for (handle = 0; handle < DW2_RENDER_SLOT_LIMIT; handle++)
   {
      char text[BEETLE_DW2_MENU_TEXT_MAX];
      bool enabled;

      if (!dw2_decode_renderer_handle(ram, ram_size,
               renderer_data_offset, handle, text, sizeof(text), &enabled)
            || !enabled || !strstr(text, "earned"))
         continue;
      if (experience_handle == DW2_RENDER_HANDLE_EMPTY
            && strstr(text, "EXP"))
      {
         experience_handle = handle;
         strcpy(experience_reward, text);
      }
      else if (bits_handle == DW2_RENDER_HANDLE_EMPTY
            && strstr(text, "BIT"))
      {
         bits_handle = handle;
         strcpy(bits_reward, text);
      }
   }
   if (experience_handle == DW2_RENDER_HANDLE_EMPTY
         || bits_handle == DW2_RENDER_HANDLE_EMPTY)
      return false;

   strcpy(snapshot.label, experience_reward);
   for (participant = 0; participant < DW2_BATTLE_ALLY_LIMIT;
         participant++)
   {
      size_t participant_offset = DW2_BATTLE_PARTICIPANTS_OFFSET
         + (size_t)participant * DW2_BATTLE_PARTICIPANT_SIZE;
      uint8_t level;
      uint8_t maximum_level;
      uint32_t experience;
      uint32_t next_level;
      int written;

      if (!dw2_ram_range(ram_size, participant_offset,
               DW2_BATTLE_PARTICIPANT_SIZE)
            || ram[participant_offset
               + DW2_BATTLE_PARTICIPANT_PRESENT_OFFSET] == 0)
         continue;
      if (!dw2_battle_participant_name(ram, ram_size, participant,
               participant_name, sizeof(participant_name), NULL))
         return false;
      level = ram[participant_offset + DW2_DIGIMON_EL_OFFSET];
      maximum_level = ram[participant_offset
         + DW2_DIGIMON_MAXIMUM_EL_OFFSET];
      experience = dw2_read_u32(ram,
            participant_offset + DW2_DIGIMON_EXP_OFFSET);
      next_level = dw2_next_level(level, maximum_level, experience);
      if (!next_level)
         written = snprintf(row, sizeof(row),
               "%s. Level %u. EXP %u. Level up.", participant_name,
               (unsigned)level, (unsigned)experience);
      else
         written = snprintf(row, sizeof(row),
               "%s. Level %u. EXP %u. Next Level %u.", participant_name,
               (unsigned)level, (unsigned)experience,
               (unsigned)next_level);
      if (written < 0 || (size_t)written >= sizeof(row)
            || !dw2_compose_sentence(snapshot.details,
               sizeof(snapshot.details), row))
         return false;
   }
   if (!dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), bits_reward)
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Total BITs",
            dw2_read_u32(ram, DW2_PLAYER_BITS_NATIVE_OFFSET)))
      return false;

   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 9;
   snapshot.context = BEETLE_DW2_MENU_BATTLE;
   snapshot.task = task_address;
   snapshot.focus_id = 0x52000000u
      ^ (experience_handle << 8) ^ bits_handle
      ^ dw2_read_u32(ram, DW2_PLAYER_BITS_NATIVE_OFFSET);
   strcpy(snapshot.title, "Battle Results");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_battle_learned_technique(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset)
{
   static const char *const pane_names[] = {
      "Acquired Tech", "Original Tech"
   };
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   uint32_t participant;
   uint32_t pane;
   uint32_t cursor;
   uint32_t scroll;
   uint32_t selected;
   uint32_t count;
   uint16_t technique_id;
   uint8_t mp;
   char participant_name[BEETLE_DW2_MENU_TEXT_MAX];
   int written;

   if (!dw2_battle_task_data(ram, ram_size, task_offset,
            DW2_BATTLE_LEARNED_TECHNIQUE_CALLBACK,
            DW2_BATTLE_LEARNED_TECHNIQUE_TASK_DATA_SIZE, false,
            &data_offset))
      return false;
   participant = dw2_read_u32(ram,
         data_offset + DW2_BATTLE_LEARNED_PARTICIPANT_OFFSET);
   pane = dw2_read_u32(ram,
         data_offset + DW2_BATTLE_LEARNED_PANE_OFFSET);
   if (participant >= DW2_BATTLE_ALLY_LIMIT || pane > 1u)
      return false;
   cursor = dw2_read_u32(ram,
         data_offset + DW2_BATTLE_LEARNED_CURSOR_OFFSET + pane * 4u);
   scroll = dw2_read_u32(ram,
         data_offset + DW2_BATTLE_LEARNED_SCROLL_OFFSET + pane * 4u);
   count = dw2_read_u32(ram,
         data_offset + DW2_BATTLE_LEARNED_COUNT_OFFSET + pane * 4u);
   selected = cursor + scroll;
   if (cursor >= DW2_BATTLE_LEARNED_LIST_LIMIT
         || scroll >= DW2_BATTLE_LEARNED_LIST_LIMIT
         || selected >= DW2_BATTLE_LEARNED_LIST_LIMIT
         || count > DW2_BATTLE_LEARNED_LIST_LIMIT
         || !dw2_battle_participant_name(ram, ram_size, participant,
            participant_name, sizeof(participant_name), NULL))
      return false;
   technique_id = selected < count
      ? dw2_read_u16(ram,
         data_offset + DW2_BATTLE_LEARNED_TECHNIQUES_OFFSET
            + pane * DW2_BATTLE_LEARNED_PANE_STRIDE + selected * 2u)
      : 0u;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!technique_id)
      strcpy(snapshot.label, "Empty slot");
   else if (!dw2_battle_technique_fields(ram, ram_size,
            (uint8_t)technique_id, &mp, snapshot.label,
            sizeof(snapshot.label), NULL, 0))
      return false;
   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 10;
   snapshot.context = BEETLE_DW2_MENU_BATTLE;
   snapshot.task = task_address;
   snapshot.focus_id = 0x4c000000u
      | (pane << 20) | (selected << 12) | technique_id;
   strcpy(snapshot.title, "Learned a new technique");
   if (count)
      written = snprintf(snapshot.details, sizeof(snapshot.details),
            "%s. %s. %u of %u.", participant_name, pane_names[pane],
            (unsigned)(selected + 1u), (unsigned)count);
   else
      written = snprintf(snapshot.details, sizeof(snapshot.details),
            "%s. %s.", participant_name, pane_names[pane]);
   if (written < 0 || (size_t)written >= sizeof(snapshot.details))
      return false;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_battle_technique(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset)
{
   static const char *const category_names[] = {
      "Attack", "Counter", "Interrupt", "Assist"
   };
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   size_t category_record;
   uint16_t category;
   uint16_t cursor;
   uint16_t scroll;
   uint32_t selected;
   uint32_t owner;
   uint8_t count;
   uint8_t technique_id;
   uint8_t mp;
   char owner_name[128];
   char technique_name[256];
   char description[512];
   int written;

   if (!dw2_battle_task_data(ram, ram_size, task_offset,
            DW2_BATTLE_TECHNIQUE_CALLBACK,
            DW2_BATTLE_TECHNIQUE_TASK_DATA_SIZE, true, &data_offset)
         || !dw2_ram_range(ram_size, DW2_BATTLE_COMMAND_OWNER_OFFSET, 4u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_CATEGORY_OFFSET, 2u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_CATEGORY_CURSOR_OFFSET,
            DW2_BATTLE_CATEGORY_COUNT * 2u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_CATEGORY_SCROLL_OFFSET,
            DW2_BATTLE_CATEGORY_COUNT * 2u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_CATEGORY_RECORDS_OFFSET,
            DW2_BATTLE_CATEGORY_COUNT * DW2_BATTLE_CATEGORY_RECORD_SIZE))
      return false;
   (void)data_offset;

   owner = dw2_read_u32(ram, DW2_BATTLE_COMMAND_OWNER_OFFSET);
   if (owner >= DW2_BATTLE_PARTICIPANT_LIMIT)
      return false;
   if (!dw2_battle_participant_name(ram, ram_size, owner,
            owner_name, sizeof(owner_name), NULL))
      snprintf(owner_name, sizeof(owner_name), "Digimon %u",
            (unsigned)(owner + 1u));
   category = dw2_read_u16(ram, DW2_BATTLE_CATEGORY_OFFSET);
   if (category >= DW2_BATTLE_CATEGORY_COUNT)
      return false;
   cursor = dw2_read_u16(ram,
         DW2_BATTLE_CATEGORY_CURSOR_OFFSET + (size_t)category * 2u);
   scroll = dw2_read_u16(ram,
         DW2_BATTLE_CATEGORY_SCROLL_OFFSET + (size_t)category * 2u);
   category_record = DW2_BATTLE_CATEGORY_RECORDS_OFFSET
      + (size_t)category * DW2_BATTLE_CATEGORY_RECORD_SIZE;
   count = ram[category_record + DW2_BATTLE_CATEGORY_COUNT_OFFSET];
   if (count > DW2_BATTLE_CATEGORY_TECHNIQUE_LIMIT
         || cursor >= 3u
         || scroll > DW2_BATTLE_CATEGORY_TECHNIQUE_LIMIT)
      return false;
   selected = (uint32_t)cursor + (uint32_t)scroll;
   if (selected >= DW2_BATTLE_CATEGORY_TECHNIQUE_LIMIT)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   technique_id = selected < count
      ? ram[category_record + DW2_BATTLE_CATEGORY_TECHNIQUE_ID_OFFSET
         + selected]
      : 0u;
   if (!technique_id)
   {
      snprintf(snapshot.label, sizeof(snapshot.label),
            "%s. Empty slot", owner_name);
      if (!count)
         written = snprintf(snapshot.details, sizeof(snapshot.details),
               "%s.", category_names[category]);
      else
         written = snprintf(snapshot.details, sizeof(snapshot.details),
               "%s. %u of 3.", category_names[category],
               (unsigned)(cursor + 1u));
      if (written < 0 || (size_t)written >= sizeof(snapshot.details))
         return false;
      snapshot.active = true;
      snapshot.enabled = true;
      snapshot.layer = 7;
      snapshot.context = BEETLE_DW2_MENU_BATTLE;
      snapshot.task = task_address;
      snapshot.focus_id = 0x54800000u
         | ((uint32_t)category << 20) | (owner << 16) | (selected << 12);
      strcpy(snapshot.title, "Give Orders");
      return beetle_accessibility_dw2_menu_submit(&snapshot);
   }
   if (!dw2_battle_technique_fields(ram, ram_size, technique_id, &mp,
            technique_name, sizeof(technique_name),
            description, sizeof(description)))
      return false;
   written = snprintf(snapshot.label, sizeof(snapshot.label),
         "%s. %s", owner_name, technique_name);
   if (written < 0 || (size_t)written >= sizeof(snapshot.label))
      return false;
   snapshot.active = true;
   snapshot.enabled = ram[category_record + selected] == 0;
   snapshot.layer = 7;
   snapshot.context = BEETLE_DW2_MENU_BATTLE;
   snapshot.task = task_address;
   snapshot.focus_id = 0x54000000u
      | ((uint32_t)category << 20)
      | (owner << 16)
      | (selected << 12)
      | ((uint32_t)technique_id << 1)
      | (snapshot.enabled ? 0u : 1u);
   strcpy(snapshot.title, "Give Orders");
   written = snprintf(snapshot.details, sizeof(snapshot.details),
         "%s. MP %u. %u of %u.", category_names[category],
         (unsigned)mp, (unsigned)(selected + 1u), (unsigned)count);
   if (written < 0 || (size_t)written >= sizeof(snapshot.details))
      return false;
   if (!dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), description))
      return false;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_battle_target(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   size_t participant_offset;
   uint32_t selected;
   int written;

   if (!dw2_battle_task_data(ram, ram_size, task_offset,
            DW2_BATTLE_TARGET_CALLBACK, DW2_BATTLE_TARGET_TASK_DATA_SIZE,
            false, &data_offset))
      return false;
   /* 80066db0 handles input in state +10 == 1. Its initializer calls
    * 80011544, which clears +14; unlike the technique list, this controller
    * never changes or tests that substate. */
   selected = dw2_read_u32(ram,
         data_offset + DW2_BATTLE_TARGET_SELECTED_OFFSET);
   if (selected >= DW2_BATTLE_TARGET_LIMIT)
      return false;
   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_battle_participant_name(ram, ram_size, selected,
            snapshot.label, sizeof(snapshot.label), &participant_offset))
      return false;
   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 8;
   snapshot.context = BEETLE_DW2_MENU_BATTLE;
   snapshot.task = task_address;
   snapshot.focus_id = 0x74000000u | selected;
   strcpy(snapshot.title, "Select Target");
   if (selected < DW2_BATTLE_ALLY_LIMIT)
   {
      uint16_t maximum_hp = dw2_read_u16(ram, participant_offset
         + DW2_DIGIMON_MAXIMUM_HP_OFFSET);
      uint16_t current_hp = dw2_read_u16(ram, participant_offset
         + DW2_DIGIMON_CURRENT_HP_OFFSET);
      uint16_t maximum_mp = dw2_read_u16(ram, participant_offset
         + DW2_DIGIMON_MAXIMUM_MP_OFFSET);
      uint16_t current_mp = dw2_read_u16(ram, participant_offset
         + DW2_DIGIMON_CURRENT_MP_OFFSET);

      written = snprintf(snapshot.details, sizeof(snapshot.details),
            "Ally. HP %u of %u. MP %u of %u.", (unsigned)current_hp,
            (unsigned)maximum_hp, (unsigned)current_mp,
            (unsigned)maximum_mp);
   }
   else
      written = snprintf(snapshot.details, sizeof(snapshot.details),
            "Enemy.");
   if (written < 0 || (size_t)written >= sizeof(snapshot.details))
      return false;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static void dw2_battle_event_reset(void)
{
   dw2_battle_event_overlay_active = false;
   dw2_battle_resolution_active = false;
   dw2_battle_command_baseline_valid = false;
   memset(dw2_battle_value_observations, 0,
         sizeof(dw2_battle_value_observations));
   memset(dw2_battle_event_queue, 0, sizeof(dw2_battle_event_queue));
   dw2_battle_event_queue_head = 0;
   dw2_battle_event_queue_count = 0;
   dw2_battle_number_count = 0;
}

static bool dw2_battle_queue_event(const char *text)
{
   size_t tail;

   if (!text || !*text)
      return false;
   if (dw2_battle_event_queue_count)
   {
      size_t previous = (dw2_battle_event_queue_head
         + dw2_battle_event_queue_count - 1u)
         % DW2_BATTLE_EVENT_QUEUE_CAPACITY;
      if (!strcmp(dw2_battle_event_queue[previous], text))
         return true;
   }
   if (dw2_battle_event_queue_count >= DW2_BATTLE_EVENT_QUEUE_CAPACITY)
      return false;
   tail = (dw2_battle_event_queue_head + dw2_battle_event_queue_count)
      % DW2_BATTLE_EVENT_QUEUE_CAPACITY;
   strncpy(dw2_battle_event_queue[tail], text,
         DW2_BATTLE_EVENT_TEXT_MAX - 1u);
   dw2_battle_event_queue[tail][DW2_BATTLE_EVENT_TEXT_MAX - 1u] = '\0';
   dw2_battle_event_queue_count++;
   return true;
}

static void dw2_battle_dispatch_event(void)
{
   char speech[DW2_BATTLE_EVENT_QUEUE_CAPACITY
      * (DW2_BATTLE_EVENT_TEXT_MAX + 1u)];

   if (!dw2_battle_event_queue_count)
      return;
   speech[0] = '\0';
   /* Simultaneous HP/MP changes form one utterance. NVDA cancels the
    * previous utterance on each call, so one event per frame loses speech. */
   while (dw2_battle_event_queue_count)
   {
      if (!dw2_compose_sentence(speech, sizeof(speech),
               dw2_battle_event_queue[dw2_battle_event_queue_head]))
         break;
      dw2_battle_event_queue[dw2_battle_event_queue_head][0] = '\0';
      dw2_battle_event_queue_head = (dw2_battle_event_queue_head + 1u)
         % DW2_BATTLE_EVENT_QUEUE_CAPACITY;
      dw2_battle_event_queue_count--;
   }
   if (speech[0])
      beetle_accessibility_speak(speech, 10, "battle");
}

static bool dw2_battle_participant_label(const uint8_t *ram,
      size_t ram_size, uint32_t participant, char *label,
      size_t label_size, size_t *participant_offset)
{
   int written;

   if (dw2_battle_participant_name(ram, ram_size, participant, label,
            label_size, participant_offset))
      return true;
   if (!label || !label_size || participant >= DW2_BATTLE_PARTICIPANT_LIMIT)
      return false;
   written = snprintf(label, label_size, "%s %u",
         participant < DW2_BATTLE_ALLY_LIMIT ? "Ally" : "Enemy",
         (unsigned)(participant < DW2_BATTLE_ALLY_LIMIT
            ? participant + 1u
            : participant - DW2_BATTLE_ALLY_LIMIT + 1u));
   if (written < 0 || (size_t)written >= label_size)
      return false;
   if (participant_offset)
      *participant_offset = DW2_BATTLE_PARTICIPANTS_OFFSET
         + (size_t)participant * DW2_BATTLE_PARTICIPANT_SIZE;
   return true;
}

static uint32_t dw2_battle_number_target(const uint8_t *ram, size_t ram_size)
{
   size_t count = dw2_active_task_count(ram, ram_size);
   size_t index;
   uint32_t selected = UINT32_MAX;

   /* Interpreter 8006cb8c, opcode 3, marks the affected model through
    * 8006f640: data +28 = 1, task +30 = 5. Task +08 is its participant ID.
    * A model can be in state 2 while playing its hit/defeat animation. */
   for (index = 0; index < count; index++)
   {
      size_t task_offset, data_offset, data_size;
      uint32_t callback, state, participant;
      if (!dw2_active_task(ram, ram_size, index, NULL, &task_offset)
            || dw2_read_u32(ram, task_offset) != DW2_BATTLE_MODEL_TASK_TYPE)
         continue;
      state = dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET);
      if ((state != 1u && state != 2u)
            || dw2_read_u32(ram, task_offset + 0x30u) != 5u
            || !dw2_task_descriptor(ram, ram_size, DW2_BATTLE_MODEL_TASK_TYPE,
               &callback, &data_size)
            || callback != DW2_BATTLE_MODEL_CALLBACK
            || data_size != DW2_BATTLE_MODEL_DATA_SIZE
            || !dw2_address_to_offset(dw2_read_u32(ram,
                  task_offset + DW2_TASK_DATA_OFFSET), ram_size,
               data_size, &data_offset)
            || dw2_read_u32(ram, data_offset + 0x28u) != 1u)
         continue;
      participant = dw2_read_u32(ram, task_offset + 0x08u);
      if (participant >= DW2_BATTLE_PARTICIPANT_LIMIT)
         continue;
      if (selected != UINT32_MAX && selected != participant)
         return UINT32_MAX;
      selected = participant;
   }
   return selected;
}

static void dw2_battle_observe_numbers(const uint8_t *ram, size_t ram_size,
      bool announce)
{
   dw2_battle_number_observation_t current[DW2_TASK_LIST_LIMIT];
   size_t current_count = 0;
   size_t count = dw2_active_task_count(ram, ram_size);
   size_t index;

   for (index = 0; index < count; index++)
   {
      dw2_battle_number_observation_t observation;
      size_t task_offset, old_index, participant_offset;
      uint32_t participant, substate;
      bool holding;
      unsigned displayed;
      char label[96];
      char event[DW2_BATTLE_EVENT_TEXT_MAX];
      int written;
      if (!dw2_active_task(ram, ram_size, index, &observation.task, &task_offset))
         continue;
      if (dw2_read_u32(ram, task_offset) == DW2_BATTLE_NUMBER_TASK_TYPE
            && dw2_battle_task_data(ram, ram_size, task_offset,
               DW2_BATTLE_NUMBER_CALLBACK, DW2_BATTLE_NUMBER_DATA_SIZE,
               false, &observation.data))
      {
         observation.kind = dw2_read_u32(ram, observation.data);
         observation.amount = dw2_read_u32(ram, observation.data + 4u);
         observation.icon = dw2_read_u32(ram, observation.data + 8u);
         if (observation.kind == 7u)
            /* This popup's second constructor argument is unused and may
             * be uninitialized. Track the actual fired item instead. */
            observation.amount = dw2_ram_range(ram_size, 0x7406cu, 2u)
               ? dw2_read_u16(ram, 0x7406cu) : 0u;
         substate = dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET);
         holding = substate == 1u
            && dw2_read_u32(ram, observation.data + 0x0cu) == 0x1000u;
         if (observation.kind > 7u || (observation.kind >= 4u
                  && observation.kind <= 6u))
            continue;
      }
      else if (dw2_read_u32(ram, task_offset) == 0x50cu
            && dw2_battle_task_data(ram, ram_size, task_offset,
               0x8006f69cu, 8u, false, &observation.data))
      {
         /* 8006f674 stores the action category at +8 and converts the
          * script's success argument into the FIGHT000 row selector at +4:
          * 2 is the normal banner, 4 is its failure variant (8006f820).
          * Category 0 alone means Attack, including every normal attack.
          * The native failure producer uses (0,4); Guard uses (4,2).
          * Other successful banners are followed by the technique popup. */
         observation.kind = 0x100u + dw2_read_u32(ram, task_offset + 8u);
         observation.amount = dw2_read_u32(ram, task_offset + 4u);
         observation.icon = 0;
         substate = dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET);
         holding = substate == 1u && dw2_read_u32(ram, observation.data) == 0x1000u;
         if (!((observation.kind == 0x100u && observation.amount == 4u)
                  || (observation.kind == 0x104u && observation.amount == 2u)))
            continue;
      }
      else
         continue;
      observation.announced = !announce && holding;
      if (observation.amount > INT32_MAX)
         continue;
      for (old_index = 0; old_index < dw2_battle_number_count; old_index++)
      {
         const dw2_battle_number_observation_t *old =
            &dw2_battle_numbers[old_index];
         /* A fresh popup always rises through substate 0 for seven native
          * updates. That rearms a reused task even for an identical number;
          * retirement (substate 2) must not rearm the existing popup. */
         if (substate != 0u
               && old->task == observation.task && old->data == observation.data
               && old->kind == observation.kind && old->amount == observation.amount
               && old->icon == observation.icon)
         {
            observation.announced = observation.announced || old->announced;
            break;
         }
      }
      /* 8006fa28 draws kinds 1..3 through the three-digit formatter
       * 8001d5b4. Wait for 8006f8ec's holding state; the queued script and
       * early HP writes do not say when the number reaches the screen. */
      if (announce && !observation.announced && holding)
      {
         displayed = (unsigned)(observation.amount % 1000u);
         participant = dw2_battle_number_target(ram, ram_size);
         label[0] = '\0';
         participant_offset = 0;
         if (participant < DW2_BATTLE_PARTICIPANT_LIMIT)
            dw2_battle_participant_label(ram, ram_size, participant,
                  label, sizeof(label), &participant_offset);
         written = 0;
         if (observation.kind == 0u)
         {
            char technique[BEETLE_DW2_MENU_TEXT_MAX];
            uint8_t mp;
            if (observation.amount <= UINT16_MAX
                  && dw2_battle_technique_fields(ram, ram_size,
                     (uint16_t)observation.amount, &mp, technique,
                     sizeof(technique), NULL, 0))
               written = label[0]
                  ? snprintf(event, sizeof(event), "%s uses %s.", label, technique)
                  : snprintf(event, sizeof(event), "%s.", technique);
         }
         else if (observation.kind == 7u)
         {
            uint32_t price, name, description;
            char item[128];
            /* The native cannon removes 7406c from inventory and shows its
             * firing animation through popup 7. It has no Digimon actor. */
            if (dw2_ram_range(ram_size, 0x7406cu, 2u)
                  && dw2_item_fields(ram, ram_size, dw2_read_u16(ram, 0x7406cu),
                     &price, &name, &description)
                  && beetle_accessibility_dw2_decode_text(ram, ram_size, name,
                     BEETLE_DW2_TEXT_MENU, item, sizeof(item)))
               written = snprintf(event, sizeof(event), "Used %s.", item);
            else
               written = snprintf(event, sizeof(event), "Cannon fired.");
         }
         else if (observation.kind == 0x100u)
            written = label[0]
               ? snprintf(event, sizeof(event), "%s's action failed.", label)
               : snprintf(event, sizeof(event), "Action failed.");
         else if (observation.kind == 0x104u)
            written = label[0]
               ? snprintf(event, sizeof(event), "%s guards.", label)
               : snprintf(event, sizeof(event), "Guard.");
         else if (observation.kind == 1u)
            written = label[0]
               ? snprintf(event, sizeof(event), "%s takes %u damage.", label, displayed)
               : snprintf(event, sizeof(event), "Damage %u.", displayed);
         else if (observation.kind == 2u)
            written = label[0]
               ? snprintf(event, sizeof(event), "%s recovers %u HP.", label, displayed)
               : snprintf(event, sizeof(event), "HP recovery %u.", displayed);
         else if (observation.kind == 3u)
            /* Kind 3 also displays a number, but its unit depends on the
             * effect. Do not turn a support effect into an invented MP heal. */
            written = label[0]
               ? snprintf(event, sizeof(event), "%s. %u.", label, displayed)
               : snprintf(event, sizeof(event), "%u.", displayed);
         if (written > 0 && (size_t)written < sizeof(event)
               && participant_offset
               && observation.kind >= 1u && observation.kind <= 3u
               && dw2_ram_range(ram_size, participant_offset,
                  DW2_BATTLE_PARTICIPANT_SIZE)
               && ram[participant_offset + DW2_BATTLE_PARTICIPANT_PRESENT_OFFSET])
         {
            char details[96];
            uint16_t hp = dw2_read_u16(ram,
                  participant_offset + DW2_DIGIMON_CURRENT_HP_OFFSET);
            if (participant < DW2_BATTLE_ALLY_LIMIT && observation.kind != 3u)
            {
               snprintf(details, sizeof(details), "HP %u of %u.",
                     (unsigned)hp, (unsigned)dw2_read_u16(ram,
                        participant_offset + DW2_DIGIMON_MAXIMUM_HP_OFFSET));
               dw2_compose_sentence(event, sizeof(event), details);
            }
            if (observation.kind == 1u && observation.amount && !hp)
               dw2_compose_sentence(event, sizeof(event), "Defeated.");
         }
         if (written > 0 && (size_t)written < sizeof(event)
               && dw2_battle_queue_event(event))
         {
            observation.announced = true;
            dw2_battle_resolution_active = true;
         }
      }
      current[current_count++] = observation;
   }
   memcpy(dw2_battle_numbers, current, current_count * sizeof(current[0]));
   dw2_battle_number_count = current_count;
}

static void dw2_battle_sync_observations(const uint8_t *ram,
      size_t ram_size)
{
   uint32_t participant;

   for (participant = 0; participant < DW2_BATTLE_PARTICIPANT_LIMIT;
         participant++)
   {
      size_t offset = DW2_BATTLE_PARTICIPANTS_OFFSET
         + (size_t)participant * DW2_BATTLE_PARTICIPANT_SIZE;
      dw2_battle_value_observation_t *observation =
         &dw2_battle_value_observations[participant];

      if (!dw2_ram_range(ram_size, offset, DW2_BATTLE_PARTICIPANT_SIZE)
            || ram[offset + DW2_BATTLE_PARTICIPANT_PRESENT_OFFSET] == 0)
      {
         memset(observation, 0, sizeof(*observation));
         continue;
      }
      observation->present = true;
      observation->observed_mp = dw2_read_u16(ram,
            offset + DW2_DIGIMON_CURRENT_MP_OFFSET);
      observation->announced_mp = observation->observed_mp;
      observation->mp_stable_frames = 0;
   }
}

static void dw2_battle_observe_values(const uint8_t *ram,
      size_t ram_size, bool allow_events)
{
   uint32_t participant;

   for (participant = 0; participant < DW2_BATTLE_PARTICIPANT_LIMIT;
         participant++)
   {
      size_t offset = DW2_BATTLE_PARTICIPANTS_OFFSET
         + (size_t)participant * DW2_BATTLE_PARTICIPANT_SIZE;
      dw2_battle_value_observation_t *observation =
         &dw2_battle_value_observations[participant];
      uint16_t current_mp;
      uint16_t maximum_mp;
      char label[BEETLE_DW2_MENU_TEXT_MAX];
      char event[DW2_BATTLE_EVENT_TEXT_MAX];
      int written;

      if (!dw2_ram_range(ram_size, offset, DW2_BATTLE_PARTICIPANT_SIZE)
            || ram[offset + DW2_BATTLE_PARTICIPANT_PRESENT_OFFSET] == 0)
      {
         memset(observation, 0, sizeof(*observation));
         continue;
      }
      current_mp = dw2_read_u16(ram,
            offset + DW2_DIGIMON_CURRENT_MP_OFFSET);
      maximum_mp = dw2_read_u16(ram,
            offset + DW2_DIGIMON_MAXIMUM_MP_OFFSET);
      if (!observation->present)
      {
         observation->present = true;
         observation->observed_mp = current_mp;
         observation->announced_mp = current_mp;
         continue;
      }
      if (current_mp != observation->observed_mp)
      {
         observation->observed_mp = current_mp;
         observation->mp_stable_frames = 1;
      }
      else if (current_mp != observation->announced_mp
            && observation->mp_stable_frames
               < DW2_BATTLE_VALUE_STABLE_FRAMES)
         observation->mp_stable_frames++;

      if (allow_events && participant < DW2_BATTLE_ALLY_LIMIT
            && observation->announced_mp != observation->observed_mp
            && observation->mp_stable_frames
               >= DW2_BATTLE_VALUE_STABLE_FRAMES
            && dw2_battle_participant_label(ram, ram_size, participant,
               label, sizeof(label), NULL))
      {
         if (observation->observed_mp < observation->announced_mp)
            written = snprintf(event, sizeof(event),
                  "%s uses %u MP. MP %u of %u.", label,
                  (unsigned)(observation->announced_mp
                     - observation->observed_mp),
                  (unsigned)observation->observed_mp,
                  (unsigned)maximum_mp);
         else
            written = snprintf(event, sizeof(event),
                  "%s recovers %u MP. MP %u of %u.", label,
                  (unsigned)(observation->observed_mp
                     - observation->announced_mp),
                  (unsigned)observation->observed_mp,
                  (unsigned)maximum_mp);
         if (written > 0 && (size_t)written < sizeof(event)
               && dw2_battle_queue_event(event))
         {
            observation->announced_mp = observation->observed_mp;
            observation->mp_stable_frames = 0;
         }
      }
   }
}

void beetle_accessibility_dw2_battle_frame(const uint8_t *ram,
      size_t ram_size, uint32_t overlay_tag, bool menu_active)
{
   if (!ram || overlay_tag != DW2_STAG3000_TAG)
   {
      dw2_battle_event_reset();
      return;
   }
   if (!dw2_battle_event_overlay_active)
   {
      dw2_battle_event_reset();
      dw2_battle_event_overlay_active = true;
      dw2_battle_sync_observations(ram, ram_size);
   }
   if (menu_active)
   {
      dw2_battle_resolution_active = false;
      dw2_battle_command_baseline_valid = true;
      dw2_battle_event_queue_head = 0;
      dw2_battle_event_queue_count = 0;
      dw2_battle_sync_observations(ram, ram_size);
      dw2_battle_observe_numbers(ram, ram_size, false);
      return;
   }
   /* 73890 is a generated instruction buffer, not current battle state.
    * Announce actions only when their native display task reaches its hold. */
   /* A real command menu establishes MP before the native script builder
    * deducts technique cost / restores Guard MP. Retain that baseline while
    * awaiting the popup; loading without a command still synchronizes. */
   if (!dw2_battle_resolution_active && !dw2_battle_command_baseline_valid)
      dw2_battle_sync_observations(ram, ram_size);
   dw2_battle_observe_numbers(ram, ram_size, true);
   /* Let pre-popup MP stabilize without speech. Once the popup holds, its
    * name and the stable MP update share a single NVDA utterance. */
   if (dw2_battle_resolution_active || dw2_battle_command_baseline_valid)
      dw2_battle_observe_values(ram, ram_size, dw2_battle_resolution_active);
   dw2_battle_dispatch_event();
}

static bool dw2_item_fields(const uint8_t *ram, size_t ram_size,
      uint16_t item_id, uint32_t *price, uint32_t *name_address,
      uint32_t *description_address)
{
   size_t base_offset;
   size_t table_offset;
   size_t record_index;
   uint32_t table_relative;

   if (!item_id || !price || !name_address || !description_address
         || !dw2_resource_group_base(ram, ram_size,
            DW2_ITEM_RESOURCE_GROUP, &base_offset))
      return false;
   table_relative = dw2_read_u32(ram, base_offset);
   if (table_relative >= ram_size - base_offset)
      return false;
   table_offset = base_offset + table_relative;
   for (record_index = 0; record_index < DW2_ITEM_RESOURCE_RECORD_LIMIT;
         record_index++)
   {
      size_t record_offset = table_offset
         + record_index * DW2_ITEM_RESOURCE_RECORD_SIZE;
      uint16_t candidate_id;

      if (!dw2_ram_range(ram_size, record_offset,
               DW2_ITEM_RESOURCE_RECORD_SIZE))
         return false;
      candidate_id = dw2_read_u16(ram, record_offset);
      if (!candidate_id)
         return false;
      if (candidate_id == item_id)
      {
         uint32_t name_relative = dw2_read_u32(ram, record_offset + 8u);
         uint32_t description_relative = dw2_read_u32(ram,
               record_offset + 0x0cu);

         if (name_relative >= ram_size - base_offset
               || description_relative >= ram_size - base_offset
               || !dw2_ram_range(ram_size, base_offset + name_relative, 1u)
               || !dw2_ram_range(ram_size,
                  base_offset + description_relative, 1u))
            return false;
         /* FUN_8001E180 returns the visible 24-bit shop price at +4. */
         *price = dw2_read_u32(ram, record_offset + 4u) & 0x00ffffffu;
         *name_address = 0x80000000u
            | (uint32_t)(base_offset + name_relative);
         *description_address = 0x80000000u
            | (uint32_t)(base_offset + description_relative);
         return true;
      }
   }
   return false;
}

static bool dw2_submit_battle_cannon(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset)
{
   /* SYS_MESS 1fd0007..9, drawn by 80065594. The cannon has its own
    * controller; it is neither the common inventory nor Give Orders. */
   static const char *const categories[] = {
      "ShooterGun", "RCannon", "ZCannon"
   };
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   uint16_t category;
   uint16_t cursor;
   uint16_t scroll;
   uint32_t count;
   uint32_t selected;
   uint32_t total;
   uint8_t item_id;
   char name[256];
   char description[512];
   int written;

   if (!dw2_battle_task_data(ram, ram_size, task_offset,
            DW2_BATTLE_CANNON_CALLBACK, DW2_BATTLE_CANNON_TASK_DATA_SIZE,
            true, &data_offset)
         || dw2_read_u32(ram, data_offset + 0x3cu) != 0x1000u
         || !dw2_ram_range(ram_size, DW2_BATTLE_CANNON_CATEGORY_OFFSET, 2u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_CANNON_CURSOR_OFFSET,
            DW2_BATTLE_CANNON_CATEGORY_COUNT * 2u)
         || !dw2_ram_range(ram_size, DW2_BATTLE_CANNON_SCROLL_OFFSET,
            DW2_BATTLE_CANNON_CATEGORY_COUNT * 2u))
      return false;
   category = dw2_read_u16(ram, DW2_BATTLE_CANNON_CATEGORY_OFFSET);
   if (category >= DW2_BATTLE_CANNON_CATEGORY_COUNT)
      return false;
   cursor = dw2_read_u16(ram,
         DW2_BATTLE_CANNON_CURSOR_OFFSET + (size_t)category * 2u);
   scroll = dw2_read_u16(ram,
         DW2_BATTLE_CANNON_SCROLL_OFFSET + (size_t)category * 2u);
   count = dw2_read_u32(ram, data_offset + 0xe8u + (size_t)category * 4u);
   total = count < 3u ? 3u : count;
   if (count > DW2_BATTLE_CANNON_ITEM_LIMIT || cursor >= 3u
         || (uint32_t)scroll + 3u > total)
      return false;
   selected = (uint32_t)scroll + (uint32_t)cursor;

   memset(&snapshot, 0, sizeof(snapshot));
   /* 80065100 packs up to 48 item IDs per cannon. 80065354 deliberately
    * allocates no text handle for zero IDs, but 80065594 allows the cursor
    * onto all three rows. Keep those empty selections audible. */
   item_id = selected < count
      ? ram[data_offset + 0x58u
         + (size_t)category * DW2_BATTLE_CANNON_ITEM_LIMIT + selected] : 0u;
   description[0] = '\0';
   if (item_id)
   {
      uint32_t price;
      uint32_t name_address;
      uint32_t description_address;
      if (!dw2_item_fields(ram, ram_size, item_id, &price, &name_address,
               &description_address)
            || !beetle_accessibility_dw2_decode_text(ram, ram_size,
               name_address, BEETLE_DW2_TEXT_MENU, name, sizeof(name)))
         return false;
      /* 800652c8 uses the item's native +0xc description for the selected
       * row. Do not retain the preceding item's text on an empty row. */
      beetle_accessibility_dw2_decode_text_fragment(ram, ram_size,
            description_address, BEETLE_DW2_TEXT_MENU, description,
            sizeof(description));
   }
   else
      strcpy(name, "Empty slot");
   written = snprintf(snapshot.label, sizeof(snapshot.label), "%s. %s",
         categories[category], name);
   if (written < 0 || (size_t)written >= sizeof(snapshot.label))
      return false;
   written = snprintf(snapshot.details, sizeof(snapshot.details),
         "%u of %u.", (unsigned)(selected + 1u), (unsigned)total);
   if (written < 0 || (size_t)written >= sizeof(snapshot.details)
         || (description[0]
            && !dw2_compose_sentence(snapshot.details, sizeof(snapshot.details),
               description)))
      return false;
   snapshot.active = true;
   /* Equipment and damaged-state flags gate both input (80065594) and
    * the visible selection cursor (80065a98). */
   snapshot.enabled = dw2_read_u32(ram,
         data_offset + 0x40u + (size_t)category * 4u) != 0u
      && dw2_read_u32(ram,
         data_offset + 0x4cu + (size_t)category * 4u) == 0u;
   snapshot.layer = 7;
   snapshot.context = BEETLE_DW2_MENU_BATTLE;
   snapshot.task = task_address;
   snapshot.focus_id = 0x43000000u | ((uint32_t)category << 20)
      | (selected << 12) | ((uint32_t)item_id << 1)
      | (snapshot.enabled ? 0u : 1u);
   strcpy(snapshot.title, "Cannon");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_append_shop_price(char *output, size_t output_size,
      uint32_t price)
{
   char sentence[64];
   int written = snprintf(sentence, sizeof(sentence), "Price %u BITs",
         (unsigned)price);

   return written > 0 && (size_t)written < sizeof(sentence)
      && dw2_compose_sentence(output, output_size, sentence);
}

static bool dw2_submit_stag2000_message(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   uint32_t first_task_address;
   uint32_t handle;
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   uint8_t choice;

   /* FUN_80068D84 and FUN_80068DD8 update the first active 0x30D task.
    * Mirror that ownership rule so a retained secondary task cannot replace
    * the native foreground message. */
   if (task_type != DW2_DIGIVOLVE_MESSAGE_TASK_TYPE
         || !dw2_find_first_active_task_type(ram, ram_size, task_type,
            &first_task_address, NULL)
         || first_task_address != task_address
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x20u, &data_offset, &data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 0u,
            snapshot.label, sizeof(snapshot.label), &snapshot.enabled,
            &handle))
      return false;

   snapshot.active = true;
   snapshot.layer = 5;
   snapshot.context = BEETLE_DW2_MENU_PROMPT;
   snapshot.task = task_address;
   snapshot.focus_id = handle;
   if (dw2_renderer_yes_no_choice(ram, ram_size, renderer_data_offset,
            handle, &choice))
   {
      strcpy(snapshot.details, snapshot.label);
      strcpy(snapshot.label, choice ? "Selected No" : "Selected Yes");
      snapshot.context = BEETLE_DW2_MENU_DIALOGUE_CHOICE;
      snapshot.focus_id = (handle << 1) | choice;
   }
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_item_shop_root(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset, uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   size_t data_size;
   size_t bits_task_offset;
   size_t bits_data_offset;
   size_t bits_data_size;
   size_t message_task_offset;
   size_t message_data_offset;
   size_t message_data_size;
   size_t renderer_data_offset;
   uint32_t focus;
   char options[2][BEETLE_DW2_MENU_TEXT_MAX];
   bool option_enabled[2];
   int written;

   if (task_type != DW2_ITEM_SHOP_ROOT_TASK_TYPE
         || !dw2_ram_range(ram_size, DW2_ITEM_SHOP_FOCUS_OFFSET, 4u)
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 8u, &data_offset, &data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   /* Task 0x315 owns the native BITs field and is the shop window's
    * readiness signal. Task 0x316 can exist before that field is visible. */
   if (!dw2_find_first_active_task_type(ram, ram_size,
            DW2_ITEM_SHOP_BITS_TASK_TYPE, NULL, &bits_task_offset)
         || !dw2_memory_card_task_data(ram, ram_size, bits_task_offset,
            DW2_ITEM_SHOP_BITS_TASK_TYPE, 4u,
            &bits_data_offset, &bits_data_size)
         || !dw2_memory_card_handle_visible(ram, ram_size,
            renderer_data_offset, bits_data_offset, bits_data_size,
            0u, NULL, NULL))
      return false;
   /* The shop controller creates 0x316 while its preceding shared message
    * can still be visible. Do not announce Buy/Sell behind that message. */
   if (dw2_find_first_active_task_type(ram, ram_size,
            DW2_DIGIVOLVE_MESSAGE_TASK_TYPE, NULL, &message_task_offset)
         && dw2_memory_card_task_data(ram, ram_size, message_task_offset,
            DW2_DIGIVOLVE_MESSAGE_TASK_TYPE, 0x20u,
            &message_data_offset, &message_data_size)
         && dw2_memory_card_handle_visible(ram, ram_size,
            renderer_data_offset, message_data_offset, message_data_size,
            0u, NULL, NULL))
      return false;
   focus = dw2_read_u32(ram, DW2_ITEM_SHOP_FOCUS_OFFSET);
   if (focus > 1u)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 0u,
            options[0], sizeof(options[0]), &option_enabled[0], NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 4u,
            options[1], sizeof(options[1]), &option_enabled[1], NULL))
      return false;
   written = snprintf(snapshot.label, sizeof(snapshot.label), "Selected %s",
         options[focus]);
   if (written <= 0 || (size_t)written >= sizeof(snapshot.label))
      return false;
   written = snprintf(snapshot.details, sizeof(snapshot.details),
         "Options: %s, %s", options[0], options[1]);
   if (written <= 0 || (size_t)written >= sizeof(snapshot.details))
      return false;
   snapshot.enabled = option_enabled[focus];
   snapshot.active = true;
   snapshot.layer = 5;
   snapshot.context = BEETLE_DW2_MENU_DIALOGUE_CHOICE;
   snapshot.task = task_address;
   snapshot.focus_id = focus;
   strcpy(snapshot.title, "Item Shop");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_digivolve_mode(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset, uint32_t task_type)
{
   static const char *const options[] = {
      "Digivolve", "DNA Digivolve"
   };
   beetle_dw2_menu_snapshot_t snapshot;
   bool option_enabled[2];
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   uint32_t focus;
   int written;

   if (task_type != DW2_DIGIVOLVE_MODE_TASK_TYPE
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x0cu, &data_offset, &data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   focus = dw2_read_u32(ram, data_offset);
   if (focus > 1u
         || !dw2_memory_card_handle_visible(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 4u,
            &option_enabled[0], NULL)
         || !dw2_memory_card_handle_visible(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 8u,
            &option_enabled[1], NULL))
      return false;

   /* FUN_800681A0 creates native graphical resources 0x102 and 0x103 for
    * these two fixed choices; FUN_80068364 moves their live highlight from
    * the task-owned cursor. They are not text-renderer resources, so use the
    * verified native labels while retaining both game-owned visibility gates. */
   memset(&snapshot, 0, sizeof(snapshot));
   written = snprintf(snapshot.label, sizeof(snapshot.label), "Selected %s",
         options[focus]);
   if (written <= 0 || (size_t)written >= sizeof(snapshot.label))
      return false;
   written = snprintf(snapshot.details, sizeof(snapshot.details),
         "Options: %s, %s", options[0], options[1]);
   if (written <= 0 || (size_t)written >= sizeof(snapshot.details))
      return false;
   snapshot.active = true;
   snapshot.enabled = option_enabled[focus];
   snapshot.layer = 6;
   snapshot.context = BEETLE_DW2_MENU_DIALOGUE_CHOICE;
   snapshot.task = task_address;
   snapshot.focus_id = focus;
   strcpy(snapshot.title, "Digivolution");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_digivolve_roster(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset, uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char species[BEETLE_DW2_MENU_TEXT_MAX];
   char generation[BEETLE_DW2_MENU_TEXT_MAX];
   char position[48];
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   size_t row_data;
   size_t row_state;
   uint32_t count;
   uint32_t page;
   uint32_t row;
   uint32_t focus;
   uint32_t mode;
   int written;

   if (task_type != DW2_DIGIVOLVE_ROSTER_TASK_TYPE
         || !dw2_ram_range(ram_size, DW2_DIGIVOLVE_PAGE_OFFSET, 0x10u)
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x78u, &data_offset, &data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   count = dw2_read_u32(ram, data_offset);
   page = dw2_read_u32(ram, DW2_DIGIVOLVE_PAGE_OFFSET);
   row = dw2_read_u32(ram, DW2_DIGIVOLVE_ROW_OFFSET);
   mode = dw2_read_u32(ram, DW2_DIGIVOLVE_MODE_OFFSET);
   if (!count || count > DW2_DIGIVOLVE_ROSTER_LIMIT || row >= 4u
         || page >= count || row > UINT32_MAX - page || mode > 2u)
      return false;
   focus = page + row;
   if (focus >= count)
      return false;
   row_data = 8u + (size_t)row * 0x10u;
   row_state = 0x48u + (size_t)row * 0x0cu;
   if (dw2_read_u32(ram, data_offset + row_state) != 1u
         || dw2_read_u32(ram, data_offset + row_state + 8u) != focus)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, row_data + 4u,
            snapshot.label, sizeof(snapshot.label), &snapshot.enabled, NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, row_data + 8u,
            species, sizeof(species), NULL, NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, row_data + 0x0cu,
            generation, sizeof(generation), NULL, NULL)
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Species", species)
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Generation", generation))
      return false;
   written = snprintf(position, sizeof(position), "%u of %u",
         (unsigned)focus + 1u, (unsigned)count);
   if (written <= 0 || (size_t)written >= sizeof(position)
         || !dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), position))
      return false;

   snapshot.active = true;
   snapshot.layer = 6;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = (mode << 16) | focus;
   strcpy(snapshot.title, mode ? "DNA Digivolve" : "Digivolve");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_item_shop_list(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset, uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   char position[64];
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   size_t prompt_relative;
   uint32_t substate;
   uint32_t cursor;
   uint32_t page;
   uint32_t item_count;
   uint32_t maximum_page;
   uint32_t absolute;
   uint32_t mode;
   uint32_t price;
   uint32_t name_address;
   uint32_t description_address;
   uint32_t units;
   uint32_t bits;
   uint32_t prompt_handle;
   uint32_t dirty;
   uint16_t item_id;
   uint8_t choice;
   bool have_prompt = false;
   bool prompt_enabled = true;
   int written;

   if (task_type != DW2_ITEM_SHOP_LIST_TASK_TYPE
         || !dw2_ram_range(ram_size, DW2_ITEM_SHOP_MODE_OFFSET, 4u)
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x64u, &data_offset, &data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   substate = dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET);
   cursor = dw2_read_u32(ram, data_offset + 0x40u);
   page = dw2_read_u32(ram, data_offset + 0x44u);
   item_count = dw2_read_u32(ram, data_offset + 0x48u);
   maximum_page = dw2_read_u32(ram, data_offset + 0x4cu);
   mode = dw2_read_u32(ram, DW2_ITEM_SHOP_MODE_OFFSET);
   dirty = dw2_read_u32(ram, data_offset + 0x3cu);
   if (substate > 2u || cursor >= DW2_ITEM_SHOP_ROW_LIMIT
          || !item_count || item_count > DW2_ITEM_SHOP_ITEM_LIMIT
          || mode > 1u || maximum_page >= DW2_ITEM_SHOP_ITEM_LIMIT
          || page > maximum_page
          || page > (UINT32_MAX - cursor) / DW2_ITEM_SHOP_ROW_LIMIT)
      return false;
   /* FUN_8006C3B8 uses list substate zero, Buy confirmation at
    * (mode zero, substate one, +0x18), and Sell confirmation at
    * (mode one, substate two, +0x14). Other combinations are stale. */
   if ((substate == 1u && mode != 0u)
         || (substate == 2u && mode != 1u))
      return false;
   absolute = page * DW2_ITEM_SHOP_ROW_LIMIT + cursor;
   if (absolute >= item_count
         || !dw2_ram_range(ram_size,
            DW2_ITEM_SHOP_IDS_OFFSET + (size_t)absolute * 2u, 2u)
         || !dw2_ram_range(ram_size, DW2_PLAYER_BITS_NATIVE_OFFSET, 4u))
      return false;
   item_id = dw2_read_u16(ram,
         DW2_ITEM_SHOP_IDS_OFFSET + (size_t)absolute * 2u);
   units = dw2_read_u32(ram, data_offset + 0x58u);
   bits = dw2_read_u32(ram, DW2_PLAYER_BITS_NATIVE_OFFSET);
   if (!dw2_item_fields(ram, ram_size, item_id, &price, &name_address,
            &description_address) || (mode == 0u && units > 99u))
      return false;
   if (mode == 1u)
      price /= 2u;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_memory_card_handle_visible(ram, ram_size,
             renderer_data_offset, data_offset, data_size,
             0x1cu + cursor * 4u, &snapshot.enabled, NULL)
         || !beetle_accessibility_dw2_decode_text_fragment(ram, ram_size,
            name_address, BEETLE_DW2_TEXT_MENU, snapshot.label,
             sizeof(snapshot.label)))
      return false;
   /* The renderer row combines this native name with a numeric price. Read
    * both fields from the same 0x045E item record so neither is filtered. */
   if (substate == 0u
         && (dirty != 0u
            || !dw2_memory_card_handle_visible(ram, ram_size,
               renderer_data_offset, data_offset, data_size, 4u,
               NULL, NULL)
            || !dw2_memory_card_handle_visible(ram, ram_size,
                renderer_data_offset, data_offset, data_size, 8u,
                NULL, NULL)
            || (mode == 0u
               && !dw2_memory_card_handle_visible(ram, ram_size,
                  renderer_data_offset, data_offset, data_size, 0x0cu,
                  NULL, NULL))
            || !dw2_memory_card_handle_visible(ram, ram_size,
               renderer_data_offset, data_offset, data_size, 0x10u,
               NULL, NULL)))
      return false;
   /* BITs, Units, and description are announced from canonical native
    * values, but only after each corresponding renderer field is visible. */
   if (substate != 0u)
   {
      prompt_relative = mode == 0u ? 0x18u : 0x14u;
      if (!dw2_memory_card_decode_handle(ram, ram_size,
               renderer_data_offset, data_offset, data_size,
               prompt_relative, text, sizeof(text), &prompt_enabled,
               &prompt_handle)
            || !dw2_renderer_yes_no_choice(ram, ram_size,
               renderer_data_offset, prompt_handle, &choice))
         return false;
      have_prompt = true;
   }

   snapshot.active = true;
   snapshot.layer = 6;
   snapshot.task = task_address;
   snapshot.focus_id = (substate << 24) | (mode << 23)
      | (absolute << 1) | (have_prompt ? choice : 0u);
   strcpy(snapshot.title, "Item Shop");
   if (have_prompt)
   {
      char selected_item[BEETLE_DW2_MENU_TEXT_MAX];

      strcpy(selected_item, snapshot.label);
      strcpy(snapshot.label, text);
      snapshot.enabled = prompt_enabled;
      snapshot.context = BEETLE_DW2_MENU_PROMPT;
      if (!dw2_compose_sentence(snapshot.details, sizeof(snapshot.details),
               choice ? "Selected No" : "Selected Yes")
            || !dw2_compose_sentence(snapshot.details,
               sizeof(snapshot.details), selected_item)
            || !dw2_append_shop_price(snapshot.details,
               sizeof(snapshot.details), price))
         return false;
      return beetle_accessibility_dw2_menu_submit(&snapshot);
   }

   snapshot.context = BEETLE_DW2_MENU_LIST;
   if (!dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), mode == 0u ? "Buy" : "Sell")
         || !dw2_append_shop_price(snapshot.details,
            sizeof(snapshot.details), price))
      return false;
   if (mode == 0u
         && !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Units", units))
      return false;
   if (!dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "BITs", bits))
      return false;
   written = snprintf(position, sizeof(position), "%u of %u",
         (unsigned)absolute + 1u, (unsigned)item_count);
   if (written <= 0 || (size_t)written >= sizeof(position)
         || !dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), position))
      return false;
   if (!beetle_accessibility_dw2_decode_text_fragment(ram, ram_size,
            description_address, BEETLE_DW2_TEXT_MENU, text, sizeof(text))
         || !strcmp(text, snapshot.label)
         || !dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), text))
      return false;
   if (dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 0x14u, text,
            sizeof(text), NULL, NULL)
         && strcmp(text, snapshot.label))
      dw2_compose_sentence(snapshot.details, sizeof(snapshot.details), text);
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_digivolution_panel(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char species[BEETLE_DW2_MENU_TEXT_MAX];
   char type[BEETLE_DW2_MENU_TEXT_MAX];
   char generation[BEETLE_DW2_MENU_TEXT_MAX];
   char specialty[BEETLE_DW2_MENU_TEXT_MAX];
   char first_parent[BEETLE_DW2_MENU_TEXT_MAX];
   char second_parent[BEETLE_DW2_MENU_TEXT_MAX];
   char parents[BEETLE_DW2_MENU_TEXT_MAX];
   char prompt[BEETLE_DW2_MENU_TEXT_MAX];
   uint32_t selector_task_address;
   uint32_t record_address;
   uint32_t first_parent_handle;
   uint32_t second_parent_handle;
   uint32_t prompt_handle;
   uint32_t phase;
   uint32_t focus;
   uint32_t experience;
   size_t selector_task_offset;
   size_t selector_data_offset;
   size_t selector_data_size;
   size_t panel_data_offset;
   size_t panel_data_size;
   size_t renderer_data_offset;
   size_t record_offset;
   uint8_t el;
   uint8_t maximum_el;
   uint8_t choice;
   bool name_enabled;
   bool prompt_enabled;
   int written;

   if (task_type != DW2_DIGIVOLVE_PANEL_TASK_TYPE
         || !dw2_ram_range(ram_size, DW2_DIGIVOLVE_PHASE_OFFSET, 8u)
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x60u, &panel_data_offset, &panel_data_size)
         || !dw2_find_first_active_task_type(ram, ram_size,
            DW2_DIGIVOLVE_MESSAGE_TASK_TYPE, &selector_task_address,
            &selector_task_offset)
         || !dw2_memory_card_task_data(ram, ram_size, selector_task_offset,
            DW2_DIGIVOLVE_MESSAGE_TASK_TYPE, 0x20u,
            &selector_data_offset, &selector_data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   (void)selector_task_address;
   phase = dw2_read_u32(ram, DW2_DIGIVOLVE_PHASE_OFFSET);
   focus = dw2_read_u32(ram, DW2_DIGIVOLVE_FOCUS_OFFSET);
   if (phase > 4u || focus >= DW2_DIGIVOLVE_PARTY_LIMIT)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, panel_data_offset, panel_data_size, 0u,
            snapshot.label, sizeof(snapshot.label), &name_enabled, NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, selector_data_offset, selector_data_size,
            0u, prompt, sizeof(prompt), &prompt_enabled, &prompt_handle))
      return false;
   /* STAG2000 resources 0x116/0x117 place native X glyph 0x3F directly
    * before "Button". The general decoder ignores 0x3F because it is also
    * used as layout data, so restore it only for this verified prompt. */
   dw2_restore_x_button_text(prompt, sizeof(prompt));
   snapshot.active = true;
   snapshot.layer = 6;
   snapshot.task = task_address;
   snapshot.focus_id = (phase << 16) | (focus << 8);
   strcpy(snapshot.title, phase >= 2u ? "DNA Digivolve" : "Digivolve");
   if (dw2_renderer_yes_no_choice(ram, ram_size, renderer_data_offset,
            prompt_handle, &choice))
   {
      char selected_name[BEETLE_DW2_MENU_TEXT_MAX];

      strcpy(selected_name, snapshot.label);
      strcpy(snapshot.label, prompt);
      snapshot.enabled = prompt_enabled;
      snapshot.context = BEETLE_DW2_MENU_PROMPT;
      snapshot.focus_id |= choice;
      if (!dw2_compose_sentence(snapshot.details, sizeof(snapshot.details),
               choice ? "Selected No" : "Selected Yes")
            || !dw2_compose_sentence(snapshot.details,
               sizeof(snapshot.details), selected_name))
         return false;
      return beetle_accessibility_dw2_menu_submit(&snapshot);
   }

   if (!dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, panel_data_offset, panel_data_size, 4u,
            species, sizeof(species), NULL, NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, panel_data_offset, panel_data_size, 8u,
            type, sizeof(type), NULL, NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, panel_data_offset, panel_data_size, 0x0cu,
            generation, sizeof(generation), NULL, NULL)
         || !dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, panel_data_offset, panel_data_size, 0x10u,
            specialty, sizeof(specialty), NULL, NULL))
      return false;
   first_parent_handle = dw2_read_u32(ram, panel_data_offset + 0x14u);
   second_parent_handle = dw2_read_u32(ram, panel_data_offset + 0x18u);
   if (first_parent_handle == DW2_RENDER_HANDLE_EMPTY)
   {
      if (second_parent_handle != DW2_RENDER_HANDLE_EMPTY)
         return false;
      strcpy(parents, "None");
   }
   else
   {
      if (!dw2_decode_renderer_handle(ram, ram_size, renderer_data_offset,
               first_parent_handle, first_parent, sizeof(first_parent), NULL))
         return false;
      if (second_parent_handle == DW2_RENDER_HANDLE_EMPTY)
         strcpy(parents, first_parent);
      else
      {
         if (!dw2_decode_renderer_handle(ram, ram_size,
                  renderer_data_offset, second_parent_handle, second_parent,
                  sizeof(second_parent), NULL))
            return false;
         written = snprintf(parents, sizeof(parents), "%s and %s",
               first_parent, second_parent);
         if (written <= 0 || (size_t)written >= sizeof(parents))
            return false;
      }
   }
   record_address = dw2_read_u32(ram, panel_data_offset + 0x5cu);
   if (!dw2_address_to_offset(record_address, ram_size,
            DW2_DIGIMON_DETAIL_SIZE, &record_offset))
      return false;
   el = ram[record_offset + DW2_DIGIMON_EL_OFFSET];
   maximum_el = ram[record_offset + DW2_DIGIMON_MAXIMUM_EL_OFFSET];
   experience = dw2_read_u32(ram, record_offset + DW2_DIGIMON_EXP_OFFSET);

   snapshot.enabled = name_enabled;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   if (!dw2_append_labeled_text(snapshot.details, sizeof(snapshot.details),
            "Species", species)
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Type", type)
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Generation", generation)
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Specialty", specialty)
         || !dw2_append_labeled_text(snapshot.details,
            sizeof(snapshot.details), "Parents", parents)
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "EL", el)
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "DP",
            ram[record_offset + DW2_DIGIMON_DP_OFFSET])
         || !dw2_append_current_maximum(snapshot.details,
            sizeof(snapshot.details), "HP",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_CURRENT_HP_OFFSET),
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_MAXIMUM_HP_OFFSET))
         || !dw2_append_current_maximum(snapshot.details,
            sizeof(snapshot.details), "MP",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_CURRENT_MP_OFFSET),
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_MAXIMUM_MP_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Attack",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_ATTACK_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Defense",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_DEFENSE_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Speed",
            dw2_read_u16(ram, record_offset + DW2_DIGIMON_SPEED_OFFSET))
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "EXP", experience)
         || !dw2_append_labeled_unsigned(snapshot.details,
            sizeof(snapshot.details), "Next Level",
            dw2_next_level(el, maximum_el, experience))
         || !dw2_compose_sentence(snapshot.details,
            sizeof(snapshot.details), prompt))
      return false;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static const char *dw2_memory_card_title(int16_t operation,
      bool slot_screen)
{
   if (slot_screen)
   {
      switch (operation)
      {
         case 1: return "Save";
         case 2: return "Continue";
         case 3:
         case 4: return "Battle Mode";
         case 5: return "Data Conversion";
         default: return NULL;
      }
   }
   if (operation >= 1 && operation <= 2)
      return "Save";
   if (operation >= 3 && operation <= 4)
      return "Continue";
   if (operation >= 5 && operation <= 8)
      return "Battle Mode";
   if (operation >= 9 && operation <= 10)
      return "Data Conversion";
   return NULL;
}

static bool dw2_submit_memory_card_slot_task(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   const char *title;
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   uint32_t focus;
   int16_t operation;

   if ((task_type != DW2_MEMORY_CARD_SLOT_TASK_TYPE
            && task_type != DW2_MEMORY_CARD_SLOT_ALT_TASK_TYPE)
         || dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET) != 1
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x2cu, &data_offset, &data_size)
         || !dw2_memory_card_grid_focus(ram, data_offset, data_size,
            0x10u, 0x14u, 2u, &focus))
      return false;
   operation = dw2_read_s16(ram, data_offset + 0x20u);
   title = dw2_memory_card_title(operation, true);
   if (!title)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   snprintf(snapshot.label, sizeof(snapshot.label), "Slot %u",
         (unsigned)focus + 1u);
   if (dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      dw2_memory_card_decode_handle(ram, ram_size, renderer_data_offset,
            data_offset, data_size, 0x0cu, snapshot.details,
            sizeof(snapshot.details), NULL, NULL);
   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 5;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = ((uint32_t)(uint16_t)operation << 16) | focus;
   strcpy(snapshot.title, title);
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static void dw2_append_save_record_details(const uint8_t *ram,
      size_t ram_size, uint32_t records_address, uint32_t focus,
      size_t renderer_data_offset, size_t data_offset, size_t data_size,
      char *details, size_t details_size)
{
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   char value[96];
   size_t row_handle_relative;
   size_t records_offset;
   size_t header_offset;
   size_t record_offset;
   uint32_t bits;
   uint32_t play_frames;
   uint32_t total_minutes;
   uint32_t hours;
   uint32_t minutes;
   bool player_name_terminated = false;
   size_t name_index;

   if (!dw2_address_to_offset(records_address, ram_size, 0x0cu,
            &records_offset))
      return;
   header_offset = records_offset + (size_t)focus * 4u;
   if (!dw2_ram_range(ram_size, header_offset, 4u))
      return;
   if (!dw2_read_u32(ram, header_offset))
   {
      dw2_append_sentence(details, details_size, "Empty");
      return;
   }

   record_offset = records_offset + 0x0cu
      + (size_t)focus * 0x1058u;
   if (!dw2_ram_range(ram_size, record_offset, 0x1au))
      return;
   for (name_index = 0; name_index < DW2_SAVE_PLAYER_NAME_SIZE;
         name_index++)
   {
      if (ram[record_offset + DW2_SAVE_PLAYER_NAME_OFFSET + name_index]
            == 0xffu)
      {
         player_name_terminated = true;
         break;
      }
   }
   /* FUN_800646C0 builds each visible file row's native player-name and
    * Tamer-rank handles at +0x10/+0x14 with a 0x0C row stride. */
   row_handle_relative = 0x10u + (size_t)focus * 0x0cu;
   if (player_name_terminated
         && (dw2_memory_card_decode_handle(ram, ram_size,
               renderer_data_offset, data_offset, data_size,
               row_handle_relative, text, sizeof(text), NULL, NULL)
            || beetle_accessibility_dw2_decode_text(ram, ram_size,
               0x80000000u | (uint32_t)(record_offset
                  + DW2_SAVE_PLAYER_NAME_OFFSET),
               BEETLE_DW2_TEXT_MENU, text, sizeof(text))))
      dw2_append_sentence(details, details_size, text);
   if (dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size,
            row_handle_relative + 4u, text, sizeof(text), NULL, NULL))
      dw2_append_sentence(details, details_size, text);

   bits = dw2_read_u32(ram, record_offset + 8u);
   snprintf(value, sizeof(value), "%u BITS", (unsigned)bits);
   dw2_append_sentence(details, details_size, value);

   /* FUN_8006637C caps the native 60 Hz play-frame counter, divides it
    * by 3600 for whole minutes, then converts each block of 60 minutes
    * for the save row's hour/minute display. */
   play_frames = dw2_read_u32(ram, record_offset + 4u);
   if (play_frames > 21599999u)
      play_frames = 21599999u;
   total_minutes = play_frames / 3600u;
   hours = total_minutes / 60u;
   minutes = total_minutes % 60u;
   if (hours)
      snprintf(value, sizeof(value), "Play time %u hour%s %u minute%s",
            (unsigned)hours, hours == 1u ? "" : "s",
            (unsigned)minutes, minutes == 1u ? "" : "s");
   else
      snprintf(value, sizeof(value), "Play time %u minute%s",
            (unsigned)minutes, minutes == 1u ? "" : "s");
   dw2_append_sentence(details, details_size, value);
}

static bool dw2_submit_memory_card_file_list(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      size_t data_offset, size_t data_size, size_t renderer_data_offset,
      int16_t operation, const char *title)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char instruction[BEETLE_DW2_MENU_TEXT_MAX];
   uint32_t focus;
   uint32_t records_address;
   uint32_t prompt_handle;
   uint8_t choice;

   if (dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET) != 7
         || dw2_read_s16(ram, data_offset + 0x86u) != 1
         || !dw2_memory_card_grid_focus(ram, data_offset, data_size,
            0x68u, 0x6cu, 3u, &focus))
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   snprintf(snapshot.label, sizeof(snapshot.label), "File %u",
         (unsigned)focus + 1u);
   records_address = dw2_read_u32(ram, data_offset + 0x90u);
   dw2_append_save_record_details(ram, ram_size, records_address, focus,
         renderer_data_offset, data_offset, data_size, snapshot.details,
         sizeof(snapshot.details));
   if (dw2_memory_card_decode_handle(ram, ram_size, renderer_data_offset,
            data_offset, data_size, 4u, instruction, sizeof(instruction),
            NULL, &prompt_handle))
   {
      dw2_append_sentence(snapshot.details, sizeof(snapshot.details),
            instruction);
      /* The overwrite question retains the file-list controller and grid.
       * Its native F8 answer, rather than that retained grid, has focus. */
      if (dw2_renderer_yes_no_choice(ram, ram_size, renderer_data_offset,
               prompt_handle, &choice))
      {
         dw2_append_sentence(snapshot.details, sizeof(snapshot.details),
               choice ? "Selected No" : "Selected Yes");
         snapshot.focus_id = 0x80000000u | ((uint32_t)choice << 24);
      }
   }
   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 5;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id |= ((uint32_t)(uint16_t)operation << 16) | focus;
   strcpy(snapshot.title, title);
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_memory_card_conversion_list(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      size_t data_offset, size_t data_size, size_t renderer_data_offset,
      int16_t operation, const char *title)
{
   beetle_dw2_menu_snapshot_t snapshot;
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   uint32_t focus;
   size_t handle_relative;

   if (dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET) != 12
         || dw2_read_s16(ram, data_offset + 0x86u) != 2
         || !dw2_memory_card_grid_focus(ram, data_offset, data_size,
            0x68u, 0x6cu, 5u, &focus))
      return false;
   handle_relative = 0x38u + (size_t)focus * 8u;

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, handle_relative,
            snapshot.label, sizeof(snapshot.label), &snapshot.enabled, NULL))
      return false;
   if (dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size,
            handle_relative + 4u, text, sizeof(text), NULL, NULL)
         && strcmp(text, snapshot.label))
      dw2_append_sentence(snapshot.details, sizeof(snapshot.details), text);
   if (dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 4u, text,
            sizeof(text), NULL, NULL) && strcmp(text, snapshot.label))
      dw2_append_sentence(snapshot.details, sizeof(snapshot.details), text);
   snapshot.active = true;
   snapshot.layer = 5;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = ((uint32_t)(uint16_t)operation << 16) | focus;
   strcpy(snapshot.title, title);
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_memory_card_file_task(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   const char *title;
   char status[BEETLE_DW2_MENU_TEXT_MAX];
   char prompt[BEETLE_DW2_MENU_TEXT_MAX];
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   uint32_t prompt_handle = 0;
   uint32_t substate;
   int16_t operation;
   int16_t phase;
   uint8_t choice;
   bool have_status;
   bool have_prompt;
   bool have_choice = false;

   if (task_type != DW2_MEMORY_CARD_FILE_TASK_TYPE
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x13cu, &data_offset, &data_size)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   operation = dw2_read_s16(ram, data_offset + 0x78u);
   title = dw2_memory_card_title(operation, false);
   if (!title)
      return false;
   if (dw2_submit_memory_card_file_list(ram, ram_size, task_address,
            task_offset, data_offset, data_size, renderer_data_offset,
            operation, title)
         || dw2_submit_memory_card_conversion_list(ram, ram_size,
            task_address, task_offset, data_offset, data_size,
            renderer_data_offset, operation, title))
      return true;

   have_status = dw2_memory_card_decode_handle(ram, ram_size,
         renderer_data_offset, data_offset, data_size, 8u, status,
         sizeof(status), NULL, NULL);
   have_prompt = dw2_memory_card_decode_handle(ram, ram_size,
         renderer_data_offset, data_offset, data_size, 4u, prompt,
         sizeof(prompt), NULL, &prompt_handle);
   if (!have_status && !have_prompt)
      return false;
   if (have_prompt)
      have_choice = dw2_renderer_yes_no_choice(ram, ram_size,
            renderer_data_offset, prompt_handle, &choice);

   memset(&snapshot, 0, sizeof(snapshot));
   if (have_status)
      strcpy(snapshot.label, status);
   else
      strcpy(snapshot.label, prompt);
   if (have_prompt && (!have_status || strcmp(prompt, status)))
   {
      if (have_status)
         dw2_append_sentence(snapshot.details, sizeof(snapshot.details),
               prompt);
   }
   if (have_choice)
      dw2_append_sentence(snapshot.details, sizeof(snapshot.details),
            choice ? "Selected No" : "Selected Yes");
   substate = dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET);
   phase = dw2_read_s16(ram, data_offset + 0x86u);
   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 5;
   snapshot.context = have_choice ? BEETLE_DW2_MENU_PROMPT
      : BEETLE_DW2_MENU_SYSTEM;
   snapshot.task = task_address;
   snapshot.focus_id = (substate << 16)
      | ((uint32_t)(uint16_t)phase << 8)
      | (have_choice ? choice : 0u);
   strcpy(snapshot.title, title);
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_memory_card_digimon_task(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   dw2_grid_owner_t owner;
   char text[BEETLE_DW2_MENU_TEXT_MAX];
   size_t data_offset;
   size_t data_size;
   size_t renderer_data_offset;
   uint32_t handle;
   uint32_t substate;
   uint32_t focus;
   uint8_t choice;

   if (task_type != DW2_MEMORY_CARD_DIGIMON_TASK_TYPE
         || !dw2_memory_card_task_data(ram, ram_size, task_offset,
            task_type, 0x1a8u, &data_offset, &data_size))
      return false;
   substate = dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET);
   memset(&snapshot, 0, sizeof(snapshot));
   /* FUN_80066748 initializes at most 0x24 candidate records. The remaining
    * task bytes include control state and must never be treated as rows. */
   if (substate == 2u
         && dw2_memory_card_grid_focus(ram, data_offset, data_size,
            0x50u, 0x54u, DW2_MEMORY_CARD_DIGIMON_RECORD_LIMIT, &focus))
   {
      owner.task_address = task_address;
      owner.task_type = task_type;
      owner.data_offset = data_offset;
      owner.data_size = data_size;
      owner.cursor_relative = 0x50u;
      owner.bounds_relative = 0x54u;
      if (!dw2_decode_digimon_record_label(ram, ram_size, &owner, focus,
               snapshot.label, sizeof(snapshot.label)))
         return false;
      if (dw2_find_renderer_data(ram, ram_size, &renderer_data_offset)
            && dw2_memory_card_decode_handle(ram, ram_size,
               renderer_data_offset, data_offset, data_size, 0x48u, text,
               sizeof(text), NULL, NULL)
            && strcmp(text, snapshot.label))
         dw2_append_sentence(snapshot.details, sizeof(snapshot.details),
               text);
      snapshot.context = BEETLE_DW2_MENU_LIST;
      snapshot.focus_id = (substate << 16) | focus;
   }
   else if ((substate == 3u || substate == 4u)
         && dw2_find_renderer_data(ram, ram_size, &renderer_data_offset)
         && dw2_memory_card_decode_handle(ram, ram_size,
            renderer_data_offset, data_offset, data_size, 0x40u,
            snapshot.label, sizeof(snapshot.label), &snapshot.enabled,
            &handle))
   {
      snapshot.context = BEETLE_DW2_MENU_PROMPT;
      snapshot.focus_id = substate << 16;
      if (dw2_renderer_yes_no_choice(ram, ram_size, renderer_data_offset,
               handle, &choice))
      {
         snapshot.focus_id |= choice;
         strcpy(snapshot.details,
               choice ? "Selected No" : "Selected Yes");
      }
   }
   else
      return false;

   snapshot.active = true;
   if (substate == 2u)
      snapshot.enabled = true;
   snapshot.layer = 6;
   snapshot.task = task_address;
   strcpy(snapshot.title, "Digimon Selection");
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_submit_digimon_message_task(const uint8_t *ram,
      size_t ram_size, uint32_t task_address, size_t task_offset,
      uint32_t task_type)
{
   beetle_dw2_menu_snapshot_t snapshot;
   uint32_t data_address;
   uint32_t handle;
   uint32_t substate;
   size_t data_offset;
   size_t native_data_size;
   size_t renderer_data_offset;
   uint8_t choice;

   if (task_type != DW2_DIGIMON_LIST_TASK_TYPE
         || dw2_read_u32(ram, task_offset + DW2_TASK_STATE_OFFSET) != 1)
      return false;
   substate = dw2_read_u32(ram, task_offset + DW2_TASK_SUBSTATE_OFFSET);
   if (substate != 4u && substate != 5u)
      return false;
   if (!dw2_task_data_size(ram, ram_size, task_type, &native_data_size)
         || native_data_size < 0x44u)
      return false;
   data_address = dw2_read_u32(ram, task_offset + DW2_TASK_DATA_OFFSET);
   if (!dw2_address_to_offset(data_address, ram_size, native_data_size,
            &data_offset)
         || !dw2_find_renderer_data(ram, ram_size, &renderer_data_offset))
      return false;
   handle = dw2_read_u32(ram, data_offset + 0x40u);

   memset(&snapshot, 0, sizeof(snapshot));
   if (!dw2_decode_renderer_handle(ram, ram_size, renderer_data_offset,
            handle, snapshot.label, sizeof(snapshot.label),
            &snapshot.enabled))
      return false;
   snapshot.active = true;
   snapshot.layer = 4;
   snapshot.context = BEETLE_DW2_MENU_PROMPT;
   snapshot.task = task_address;
   snapshot.focus_id = substate << 8;
   if (dw2_renderer_yes_no_choice(ram, ram_size, renderer_data_offset,
            handle, &choice))
   {
      snapshot.focus_id |= choice;
      strcpy(snapshot.details, choice ? "Selected No" : "Selected Yes");
   }
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

static bool dw2_bounded_text_valid(const char *text, bool require_visible)
{
   size_t index;
   bool visible = false;

   for (index = 0; index < BEETLE_DW2_MENU_TEXT_MAX; index++)
   {
      unsigned char value = (unsigned char)text[index];
      if (!value)
         return !require_visible || visible;
      if (!isspace(value))
         visible = true;
   }
   return false;
}

static bool dw2_snapshot_valid(const beetle_dw2_menu_snapshot_t *snapshot)
{
   if (!snapshot || !snapshot->active || snapshot->context == BEETLE_DW2_MENU_NONE)
      return false;
   return dw2_bounded_text_valid(snapshot->title, false)
      && dw2_bounded_text_valid(snapshot->label, true)
      && dw2_bounded_text_valid(snapshot->details, false);
}

static bool dw2_snapshot_equal(const beetle_dw2_menu_snapshot_t *left,
      const beetle_dw2_menu_snapshot_t *right)
{
   return left->active == right->active
      && left->enabled == right->enabled
      && left->layer == right->layer
      && left->context == right->context
      && left->task == right->task
      && left->focus_id == right->focus_id
      && !strcmp(left->title, right->title)
      && !strcmp(left->label, right->label)
      && !strcmp(left->details, right->details);
}

static bool dw2_text_contains_unavailable(const char *text)
{
   static const char needle[] = "unavailable";
   size_t offset;
   size_t needle_index;

   for (offset = 0; text[offset]; offset++)
   {
      for (needle_index = 0; needle[needle_index]; needle_index++)
      {
         unsigned char value = (unsigned char)text[offset + needle_index];
         if (!value || (unsigned char)tolower(value)
               != (unsigned char)needle[needle_index])
            break;
      }
      if (!needle[needle_index])
         return true;
   }
   return false;
}

static void dw2_append_sentence(char *output, size_t output_size,
      const char *text)
{
   size_t output_length;
   size_t start = 0;
   size_t end;
   size_t copy_length;
   unsigned char last;

   if (!output || !output_size || !text)
      return;
   while (text[start] && isspace((unsigned char)text[start]))
      start++;
   end = strlen(text);
   while (end > start && isspace((unsigned char)text[end - 1]))
      end--;
   if (end == start)
      return;

   output_length = strlen(output);
   if (output_length && output_length + 1 < output_size)
      output[output_length++] = ' ';
   if (output_length >= output_size - 1)
      return;
   copy_length = end - start;
   if (copy_length > output_size - output_length - 1)
      copy_length = output_size - output_length - 1;
   memcpy(output + output_length, text + start, copy_length);
   output_length += copy_length;
   output[output_length] = '\0';
   if (!copy_length || output_length >= output_size - 1)
      return;

   last = (unsigned char)output[output_length - 1];
   if (last != '.' && last != '!' && last != '?')
   {
      output[output_length++] = '.';
      output[output_length] = '\0';
   }
}

static void dw2_speak_snapshot(const beetle_dw2_menu_snapshot_t *snapshot,
      bool announce_title)
{
   char speech[BEETLE_DW2_MENU_SPEECH_MAX];

   speech[0] = '\0';
   if (announce_title && snapshot->title[0])
      dw2_append_sentence(speech, sizeof(speech), snapshot->title);
   if (snapshot->context == BEETLE_DW2_MENU_DIALOGUE_CHOICE
         && announce_title && snapshot->details[0])
      dw2_append_sentence(speech, sizeof(speech), snapshot->details);
   dw2_append_sentence(speech, sizeof(speech), snapshot->label);
   if (!snapshot->enabled
         && !dw2_text_contains_unavailable(snapshot->details))
      dw2_append_sentence(speech, sizeof(speech), "Unavailable");
   if (snapshot->details[0]
         && snapshot->context != BEETLE_DW2_MENU_DIALOGUE_CHOICE)
      dw2_append_sentence(speech, sizeof(speech), snapshot->details);
   if (speech[0])
      beetle_accessibility_speak(speech, 10, "menu");
}

void beetle_accessibility_dw2_menu_reset(void)
{
   dw2_best_valid = false;
   dw2_pending_valid = false;
   dw2_spoken_valid = false;
   dw2_pending_frames = 0;
   memset(&dw2_best, 0, sizeof(dw2_best));
   memset(&dw2_pending, 0, sizeof(dw2_pending));
   memset(&dw2_spoken, 0, sizeof(dw2_spoken));
   dw2_battle_event_reset();
}

void beetle_accessibility_dw2_menu_begin_frame(void)
{
   dw2_best_valid = false;
}

void beetle_accessibility_dw2_menu_poll_native(const uint8_t *main_ram,
      size_t ram_size, uint32_t overlay_tag)
{
   size_t count;
   size_t index;
   bool foreground_task = false;
   bool save_foreground = false;
   bool stag2000_foreground = false;

   if (!main_ram)
      return;
   count = dw2_active_task_count(main_ram, ram_size);
   for (index = 0; index < count; index++)
   {
      size_t task_offset;
      uint32_t type;

      if (!dw2_active_task(main_ram, ram_size, index, NULL, &task_offset)
            || dw2_read_u32(main_ram,
               task_offset + DW2_TASK_STATE_OFFSET) != 1)
         continue;
      type = dw2_read_u32(main_ram, task_offset + DW2_TASK_TYPE_OFFSET);
      if (type >= DW2_CIRCLE_TASK_TYPE
            && type <= DW2_DIGIMON_STATUS_ALT3_TASK_TYPE)
         foreground_task = true;
      if (type >= DW2_SAVE_ROOT_TASK_TYPE
            && type <= DW2_SAVE_TASK_TYPE_LAST)
      {
         foreground_task = true;
         save_foreground = true;
      }
      if (overlay_tag == DW2_STAG2000_TAG
            && (type == DW2_DIGIVOLVE_MODE_TASK_TYPE
               || type == DW2_DIGIVOLVE_ROSTER_TASK_TYPE
               || type == DW2_DIGIVOLVE_MESSAGE_TASK_TYPE
               || type == DW2_DIGIVOLVE_PANEL_TASK_TYPE
               || type == DW2_ITEM_SHOP_ROOT_TASK_TYPE
               || type == DW2_ITEM_SHOP_LIST_TASK_TYPE))
      {
         foreground_task = true;
         stag2000_foreground = true;
      }
   }
   for (index = 0; index < count; index++)
   {
      uint32_t task_address;
      size_t task_offset;
      uint32_t type;

      if (!dw2_active_task(main_ram, ram_size, index,
               &task_address, &task_offset))
         continue;
      type = dw2_read_u32(main_ram, task_offset + DW2_TASK_TYPE_OFFSET);
      if (overlay_tag == DW2_STAG3000_TAG
            && (dw2_submit_battle_learned_technique(main_ram, ram_size,
                  task_address, task_offset)
               || dw2_submit_battle_results(main_ram, ram_size,
                  task_address, task_offset)
               || dw2_submit_battle_target(main_ram, ram_size,
                  task_address, task_offset)
               || dw2_submit_battle_technique(main_ram, ram_size,
                  task_address, task_offset)
               || dw2_submit_battle_cannon(main_ram, ram_size,
                  task_address, task_offset)
               || dw2_submit_battle_command(main_ram, ram_size,
                  task_address, task_offset)))
         continue;
      if (!foreground_task && overlay_tag == DW2_STAG2000_TAG
            && type == DW2_DOMAIN_TASK_TYPE)
         dw2_submit_domain_selection(main_ram, ram_size,
               task_address, task_offset);
      else if (overlay_tag == DW2_STAG2000_TAG
            && type == DW2_ITEM_SHOP_ROOT_TASK_TYPE)
         dw2_submit_item_shop_root(main_ram, ram_size, task_address,
               task_offset, type);
      else if (overlay_tag == DW2_STAG2000_TAG
            && type == DW2_ITEM_SHOP_LIST_TASK_TYPE)
         dw2_submit_item_shop_list(main_ram, ram_size, task_address,
               task_offset, type);
      else if (overlay_tag == DW2_STAG2000_TAG
            && type == DW2_DIGIVOLVE_MODE_TASK_TYPE)
         dw2_submit_digivolve_mode(main_ram, ram_size, task_address,
               task_offset, type);
      else if (overlay_tag == DW2_STAG2000_TAG
            && type == DW2_DIGIVOLVE_ROSTER_TASK_TYPE)
         dw2_submit_digivolve_roster(main_ram, ram_size, task_address,
               task_offset, type);
      else if (overlay_tag == DW2_STAG2000_TAG
            && type == DW2_DIGIVOLVE_MESSAGE_TASK_TYPE)
         dw2_submit_stag2000_message(main_ram, ram_size, task_address,
               task_offset, type);
      else if (overlay_tag == DW2_STAG2000_TAG
            && type == DW2_DIGIVOLVE_PANEL_TASK_TYPE)
         dw2_submit_digivolution_panel(main_ram, ram_size, task_address,
               task_offset, type);
      else if (stag2000_foreground)
         continue;
      else if (!save_foreground && type == DW2_CIRCLE_TASK_TYPE)
         dw2_submit_domain_circle(main_ram, ram_size,
               task_address, task_offset);
      else if (type == DW2_MEMORY_CARD_SLOT_TASK_TYPE
            || type == DW2_MEMORY_CARD_SLOT_ALT_TASK_TYPE)
         dw2_submit_memory_card_slot_task(main_ram, ram_size,
               task_address, task_offset, type);
      else if (type == DW2_MEMORY_CARD_FILE_TASK_TYPE)
         dw2_submit_memory_card_file_task(main_ram, ram_size,
               task_address, task_offset, type);
      else if (type == DW2_MEMORY_CARD_DIGIMON_TASK_TYPE)
         dw2_submit_memory_card_digimon_task(main_ram, ram_size,
               task_address, task_offset, type);
      else if (type == DW2_DIGIMON_LIST_TASK_TYPE
            && dw2_submit_digimon_message_task(main_ram, ram_size,
               task_address, task_offset, type))
         continue;
      else if (dw2_interactive_task_layout(type, NULL, NULL, NULL))
         dw2_submit_interactive_task(main_ram, ram_size,
               task_address, task_offset, type);
      else if (dw2_display_task_layout(type, NULL, NULL, NULL))
         dw2_submit_display_task(main_ram, ram_size,
               task_address, task_offset, type);
   }
}

bool beetle_accessibility_dw2_menu_active(void)
{
   return dw2_best_valid;
}

bool beetle_accessibility_dw2_menu_blocks_dialog(void)
{
   /* The wild-party info window stays open behind gift failures/results
    * (80069c94 / 80069f84). It pauses movement, but owns no dialog prompt. */
   return dw2_best_valid && dw2_best.context != BEETLE_DW2_MENU_INSPECTION;
}

bool beetle_accessibility_dw2_menu_battle_active(void)
{
   return dw2_best_valid && dw2_best.context == BEETLE_DW2_MENU_BATTLE;
}

bool beetle_accessibility_dw2_menu_observe_probe(const uint8_t *main_ram,
      size_t ram_size, const beetle_dw2_menu_probe_t *probe)
{
   if (!main_ram || !probe)
      return false;
   if (probe->adapter == BEETLE_DW2_PROFILE_ADAPTER_GRID)
      return dw2_submit_common_grid(main_ram, ram_size, probe);
   return false;
}

bool beetle_accessibility_dw2_menu_submit(
      const beetle_dw2_menu_snapshot_t *snapshot)
{
   if (!dw2_snapshot_valid(snapshot))
      return false;
   if (!dw2_best_valid || snapshot->layer >= dw2_best.layer)
   {
      dw2_best = *snapshot;
      dw2_best_valid = true;
   }
   return true;
}

void beetle_accessibility_dw2_menu_end_frame(void)
{
   bool announce_title;

   if (!dw2_best_valid)
   {
      dw2_pending_valid = false;
      dw2_pending_frames = 0;
      dw2_spoken_valid = false;
      return;
   }

   if (dw2_spoken_valid && dw2_snapshot_equal(&dw2_best, &dw2_spoken))
   {
      dw2_pending_valid = false;
      dw2_pending_frames = 0;
      return;
   }

   if (!dw2_pending_valid || !dw2_snapshot_equal(&dw2_best, &dw2_pending))
   {
      dw2_pending = dw2_best;
      dw2_pending_valid = true;
      dw2_pending_frames = 1;
      return;
   }

   if (dw2_pending_frames < BEETLE_DW2_MENU_STABLE_FRAMES)
      dw2_pending_frames++;
   if (dw2_pending_frames < BEETLE_DW2_MENU_STABLE_FRAMES)
      return;

   announce_title = !dw2_spoken_valid
      || dw2_spoken.context != dw2_pending.context
      || dw2_spoken.layer != dw2_pending.layer
      || dw2_spoken.task != dw2_pending.task;
   dw2_speak_snapshot(&dw2_pending, announce_title);
   dw2_spoken = dw2_pending;
   dw2_spoken_valid = true;
   dw2_pending_valid = false;
   dw2_pending_frames = 0;
}
