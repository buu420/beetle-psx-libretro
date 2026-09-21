#include <stdio.h>
#include <string.h>

#include "accessibility_dw2_domain_menu.h"
#include "accessibility_dw2_menu.h"
#include "accessibility_dw2_text.h"

#define DW2_DOMAIN_RAM_MASK 0x001fffffu
#define DW2_DOMAIN_STAG4000_TAG 0x0000019au

#define DW2_DOMAIN_TASK_COUNT_OFFSET 0x00050798u
#define DW2_DOMAIN_TASK_LIST_OFFSET 0x0005079cu
#define DW2_DOMAIN_TASK_LIST_LIMIT 128u
#define DW2_DOMAIN_TASK_TYPE_OFFSET 0x00u
#define DW2_DOMAIN_TASK_STATE_OFFSET 0x10u
#define DW2_DOMAIN_TASK_DATA_OFFSET 0x2cu
#define DW2_DOMAIN_TASK_BANKS_OFFSET 0x00040d50u
#define DW2_DOMAIN_TASK_BANK_LIMIT 8u
#define DW2_DOMAIN_TASK_CALLBACK_OFFSET 0x04u
#define DW2_DOMAIN_TASK_DATA_SIZE_OFFSET 0x10u

#define DW2_DOMAIN_RESOURCE_SLOTS_OFFSET 0x0005f8c8u
#define DW2_DOMAIN_RESOURCE_SLOT_STRIDE 0x10u
#define DW2_DOMAIN_RESOURCE_SLOT_LIMIT 0x50u

/* 80063f00 stores the 400-byte domain state block here. */
#define DW2_DOMAIN_STATE_POINTER_OFFSET 0x00072b60u
#define DW2_DOMAIN_STATE_SIZE 400u

/* 800681bc fills the target table; 80069c94 walks it with +0xac. */
#define DW2_DOMAIN_TARGET_TABLE_OFFSET 0x80u
#define DW2_DOMAIN_TARGET_LIMIT 10u
#define DW2_DOMAIN_TARGET_COUNT_OFFSET 0xa8u
#define DW2_DOMAIN_TARGET_CURSOR_OFFSET 0xacu

/* 8006e920 fills the item table; 8006ae74/8006ad10 drive the cursor. */
#define DW2_DOMAIN_ITEM_TABLE_OFFSET 0xb0u
#define DW2_DOMAIN_ITEM_LIMIT 48u
#define DW2_DOMAIN_ITEM_COUNT_OFFSET 0xe1u
#define DW2_DOMAIN_ITEM_CURSOR_OFFSET 0xe2u
#define DW2_DOMAIN_ITEM_SCROLL_OFFSET 0xe3u
#define DW2_DOMAIN_ITEM_GIFT_MODE_OFFSET 0xe4u

/* Task bank 2 entry 0x0b, descriptor 80072708. */
#define DW2_DOMAIN_LIST_TASK_TYPE 0x020bu
#define DW2_DOMAIN_LIST_CALLBACK 0x80066e48u
#define DW2_DOMAIN_LIST_DATA_SIZE 0x48u
#define DW2_DOMAIN_LIST_WINDOW_OFFSET 0x1cu
#define DW2_DOMAIN_LIST_DESCRIPTION_FLAG_OFFSET 0x20u
#define DW2_DOMAIN_LIST_ROW_TEXT_OFFSET 0x28u
#define DW2_DOMAIN_LIST_ROW_LIMIT 6u
#define DW2_DOMAIN_LIST_DESCRIPTION_TEXT_OFFSET 0x40u
#define DW2_DOMAIN_LIST_VISIBLE_OFFSET 0x44u
#define DW2_DOMAIN_LIST_CURSOR_ROW_OFFSET 0x46u

/* Task bank 2 entry 0x0c, descriptor 8007275c. */
#define DW2_DOMAIN_TARGET_TASK_TYPE 0x020cu
#define DW2_DOMAIN_TARGET_CALLBACK 0x800671f0u
#define DW2_DOMAIN_TARGET_DATA_SIZE 0x38u
#define DW2_DOMAIN_TARGET_WINDOW_OFFSET 0x00u

#define DW2_DOMAIN_ROOT_POINTER_OFFSET 0x0005071cu
#define DW2_DOMAIN_ROOT_ENTITY_COUNT_OFFSET 0x0cu
#define DW2_DOMAIN_ROOT_ENTITY_TABLE_OFFSET 0x18u
#define DW2_DOMAIN_ENTITY_STRIDE 0x48u
#define DW2_DOMAIN_ENTITY_LIMIT 41u
#define DW2_DOMAIN_ENTITY_ACTIVE_FLAG 0x8000u
#define DW2_DOMAIN_ENTITY_TYPE_OFFSET 0x08u
#define DW2_DOMAIN_ENTITY_RECORD_OFFSET 0x10u
#define DW2_DOMAIN_ENTITY_WILD_TYPE 0x01u

/* 800671f0 draws +0x0b rows of {EL, name, class, stage}; 80067454 draws the
 * EL numbers from +0x16. */
#define DW2_DOMAIN_PARTY_COUNT_OFFSET 0x0bu
#define DW2_DOMAIN_PARTY_SPECIES_OFFSET 0x10u
#define DW2_DOMAIN_PARTY_LEVEL_OFFSET 0x16u
#define DW2_DOMAIN_PARTY_LIMIT 3u
#define DW2_DOMAIN_PARTY_RECORD_SIZE 0x1cu

/* 8001d8c4 scans 0x12-byte records in resource group 0xc6c. */
#define DW2_DOMAIN_SPECIES_GROUP 0x0c6cu
#define DW2_DOMAIN_SPECIES_RECORD_SIZE 0x12u
#define DW2_DOMAIN_SPECIES_RECORD_LIMIT 1024u
#define DW2_DOMAIN_SPECIES_CLASS_OFFSET 0x04u

/* 8001e67c picks the name bank; 8001e6a8 scans 0x28-byte records. */
#define DW2_DOMAIN_NAME_GROUP_LOW 0x0cb9u
#define DW2_DOMAIN_NAME_GROUP_HIGH 0x0cbau
#define DW2_DOMAIN_NAME_GROUP_MID 0x0cbbu
#define DW2_DOMAIN_NAME_RECORD_SIZE 0x28u
#define DW2_DOMAIN_NAME_RECORD_LIMIT 1024u

#define DW2_DOMAIN_CLASS_COUNT 3u
#define DW2_DOMAIN_STAGE_COUNT 4u

static bool dw2_domain_ram_range(size_t ram_size, size_t offset, size_t length)
{
   return offset <= ram_size && length <= ram_size - offset;
}

static uint32_t dw2_domain_read_u32(const uint8_t *ram, size_t offset)
{
   return (uint32_t)ram[offset]
      | ((uint32_t)ram[offset + 1u] << 8)
      | ((uint32_t)ram[offset + 2u] << 16)
      | ((uint32_t)ram[offset + 3u] << 24);
}

static uint16_t dw2_domain_read_u16(const uint8_t *ram, size_t offset)
{
   return (uint16_t)((uint32_t)ram[offset]
         | ((uint32_t)ram[offset + 1u] << 8));
}

static int16_t dw2_domain_read_s16(const uint8_t *ram, size_t offset)
{
   return (int16_t)dw2_domain_read_u16(ram, offset);
}

static bool dw2_domain_address_to_offset(uint32_t address, size_t ram_size,
      size_t length, size_t *offset)
{
   uint32_t region = address & 0xffe00000u;
   size_t candidate;

   if (!offset || (region != 0x80000000u && region != 0xa0000000u))
      return false;
   candidate = address & DW2_DOMAIN_RAM_MASK;
   if (!dw2_domain_ram_range(ram_size, candidate, length))
      return false;
   *offset = candidate;
   return true;
}

static bool dw2_domain_pointer(const uint8_t *ram, size_t ram_size,
      size_t pointer_offset, size_t length, size_t *offset)
{
   if (!dw2_domain_ram_range(ram_size, pointer_offset, 4u))
      return false;
   return dw2_domain_address_to_offset(
         dw2_domain_read_u32(ram, pointer_offset), ram_size, length, offset);
}

/* 80023db0: the loaded-resource slot table keyed by group id. */
static bool dw2_domain_group_base(const uint8_t *ram, size_t ram_size,
      uint32_t group, size_t *base_offset)
{
   size_t slot_index;

   for (slot_index = 0; slot_index < DW2_DOMAIN_RESOURCE_SLOT_LIMIT;
         slot_index++)
   {
      size_t slot_offset = DW2_DOMAIN_RESOURCE_SLOTS_OFFSET
         + slot_index * DW2_DOMAIN_RESOURCE_SLOT_STRIDE;

      if (!dw2_domain_ram_range(ram_size, slot_offset,
               DW2_DOMAIN_RESOURCE_SLOT_STRIDE))
         return false;
      if (dw2_domain_read_u32(ram, slot_offset + 4u) != group)
         continue;
      return dw2_domain_pointer(ram, ram_size, slot_offset + 0x0cu, 4u,
            base_offset);
   }
   return false;
}

/* 800239a0(group << 16 | index) == base + u32[base + index * 4]. */
static bool dw2_domain_group_entry(const uint8_t *ram, size_t ram_size,
      size_t base_offset, size_t index, size_t length, size_t *offset)
{
   uint32_t relative;

   if (!dw2_domain_ram_range(ram_size, base_offset + index * 4u, 4u))
      return false;
   relative = dw2_domain_read_u32(ram, base_offset + index * 4u);
   if (relative >= ram_size - base_offset
         || !dw2_domain_ram_range(ram_size, base_offset + relative, length))
      return false;
   *offset = base_offset + relative;
   return true;
}

static bool dw2_domain_task_descriptor(const uint8_t *ram, size_t ram_size,
      uint32_t task_type, uint32_t *callback, uint32_t *data_size)
{
   uint32_t bank_index = task_type >> 8;
   uint32_t entry_index = task_type & 0xffu;
   size_t bank_offset;
   size_t descriptor_offset;

   if (bank_index >= DW2_DOMAIN_TASK_BANK_LIMIT
         || !dw2_domain_pointer(ram, ram_size,
            DW2_DOMAIN_TASK_BANKS_OFFSET + bank_index * 4u,
            ((size_t)entry_index + 1u) * 4u, &bank_offset))
      return false;
   if (!dw2_domain_pointer(ram, ram_size,
            bank_offset + (size_t)entry_index * 4u,
            DW2_DOMAIN_TASK_DATA_SIZE_OFFSET + 4u, &descriptor_offset))
      return false;
   *callback = dw2_domain_read_u32(ram,
         descriptor_offset + DW2_DOMAIN_TASK_CALLBACK_OFFSET);
   *data_size = dw2_domain_read_u32(ram,
         descriptor_offset + DW2_DOMAIN_TASK_DATA_SIZE_OFFSET);
   return true;
}

/* 800136e4 increases the window scale to 1000. Both callers then build
 * their text and enter substate 1. State 1 alone includes the opening. */
static bool dw2_domain_window_data(const uint8_t *ram, size_t ram_size,
      size_t task_offset, uint32_t task_type, uint32_t expected_callback,
      uint32_t expected_data_size, size_t *data_offset)
{
   uint32_t callback;
   uint32_t data_size;

   if (dw2_domain_read_u32(ram, task_offset + DW2_DOMAIN_TASK_TYPE_OFFSET)
         != task_type
         || dw2_domain_read_u32(ram,
            task_offset + DW2_DOMAIN_TASK_STATE_OFFSET) != 1u
         || dw2_domain_read_u32(ram, task_offset + 0x14u) != 1u)
      return false;
   if (!dw2_domain_task_descriptor(ram, ram_size, task_type, &callback,
            &data_size)
         || callback != expected_callback || data_size != expected_data_size)
      return false;
   return dw2_domain_pointer(ram, ram_size,
         task_offset + DW2_DOMAIN_TASK_DATA_OFFSET, expected_data_size,
         data_offset);
}

static bool dw2_domain_state(const uint8_t *ram, size_t ram_size,
      size_t *state_offset)
{
   return dw2_domain_pointer(ram, ram_size, DW2_DOMAIN_STATE_POINTER_OFFSET,
         DW2_DOMAIN_STATE_SIZE, state_offset);
}

static bool dw2_domain_append(char *output, size_t output_size,
      const char *text, const char *separator)
{
   size_t length = strlen(output);
   size_t gap = length ? strlen(separator) : 0u;
   size_t extra = strlen(text);

   if (length + gap + extra + 1u > output_size)
      return false;
   memcpy(output + length, separator, gap);
   memcpy(output + length + gap, text, extra + 1u);
   return true;
}

/* ---------------------------------------------------------------- gifts */

/* 8006ad10 writes the six visible row texts, the description and the page
 * geometry into the 0x20b allocation, so the window's own data is the
 * record of what is on screen. */
static bool dw2_domain_submit_list(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   size_t state_offset;
   uint32_t visible;
   uint32_t row;
   uint32_t item_id = 0u;
   uint32_t position = 0u;
   uint32_t total = 0u;
   bool gift_mode;
   char name[BEETLE_DW2_MENU_TEXT_MAX];
   char description[BEETLE_DW2_MENU_TEXT_MAX];
   int written;

   if (!dw2_domain_window_data(ram, ram_size, task_offset,
            DW2_DOMAIN_LIST_TASK_TYPE, DW2_DOMAIN_LIST_CALLBACK,
            DW2_DOMAIN_LIST_DATA_SIZE, &data_offset)
         || dw2_domain_read_u32(ram,
            data_offset + DW2_DOMAIN_LIST_WINDOW_OFFSET) != 0x1000u
         || !dw2_domain_state(ram, ram_size, &state_offset))
      return false;

   visible = ram[data_offset + DW2_DOMAIN_LIST_VISIBLE_OFFSET];
   row = ram[data_offset + DW2_DOMAIN_LIST_CURSOR_ROW_OFFSET];
   if (!visible || visible > DW2_DOMAIN_LIST_ROW_LIMIT || row >= visible)
      return false;
   if (!beetle_accessibility_dw2_decode_text(ram, ram_size,
            dw2_domain_read_u32(ram, data_offset
               + DW2_DOMAIN_LIST_ROW_TEXT_OFFSET + (size_t)row * 4u),
            BEETLE_DW2_TEXT_MENU, name, sizeof(name))
         || !name[0])
      return false;

   /* The controller keeps the authoritative entry count and cursor. State
    * the position only when the page geometry it produced still matches the
    * drawn window; never guess a number the player cannot see. */
   {
      uint32_t count = ram[state_offset + DW2_DOMAIN_ITEM_COUNT_OFFSET];
      uint32_t cursor = ram[state_offset + DW2_DOMAIN_ITEM_CURSOR_OFFSET];
      uint32_t scroll = ram[state_offset + DW2_DOMAIN_ITEM_SCROLL_OFFSET];
      uint32_t page = count - scroll;

      if (page > DW2_DOMAIN_LIST_ROW_LIMIT)
         page = DW2_DOMAIN_LIST_ROW_LIMIT;
      if (count && count <= DW2_DOMAIN_ITEM_LIMIT && cursor < count
            && scroll <= cursor && cursor - scroll == row && page == visible)
      {
         position = cursor + 1u;
         total = count;
         item_id = ram[state_offset + DW2_DOMAIN_ITEM_TABLE_OFFSET + cursor];
      }
   }

   /* 80066e48 only builds the description sprite when the controller asked
    * for one, so stay silent about it otherwise. */
   description[0] = '\0';
   if (dw2_domain_read_u32(ram,
            data_offset + DW2_DOMAIN_LIST_DESCRIPTION_FLAG_OFFSET))
      beetle_accessibility_dw2_decode_text_fragment(ram, ram_size,
            dw2_domain_read_u32(ram,
               data_offset + DW2_DOMAIN_LIST_DESCRIPTION_TEXT_OFFSET),
            BEETLE_DW2_TEXT_MENU, description, sizeof(description));

   memset(&snapshot, 0, sizeof(snapshot));
   gift_mode = ram[state_offset + DW2_DOMAIN_ITEM_GIFT_MODE_OFFSET] != 0u;
   /* SYS_MESS 1fd0053 calls the state-0x1a payload a "gift"; the 800682dc
    * object path shoots inventory items instead. */
   strcpy(snapshot.title, gift_mode ? "Gifts" : "Items");
   if (!dw2_domain_append(snapshot.label, sizeof(snapshot.label), name, ""))
      return false;
   if (position)
   {
      char sentence[64];

      written = snprintf(sentence, sizeof(sentence), "%u of %u.",
            (unsigned)position, (unsigned)total);
      if (written < 0 || (size_t)written >= sizeof(sentence)
            || !dw2_domain_append(snapshot.details, sizeof(snapshot.details),
               sentence, " "))
         return false;
   }
   if (description[0]
         && !dw2_domain_append(snapshot.details, sizeof(snapshot.details),
            description, " "))
      return false;

   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 6;
   snapshot.context = BEETLE_DW2_MENU_LIST;
   snapshot.task = task_address;
   snapshot.focus_id = 0x44000000u | ((uint32_t)gift_mode << 23)
      | (row << 20) | (position << 8) | item_id;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

/* --------------------------------------------------------------- targets */

/* 8001e67c splits the name banks by species id. */
static uint32_t dw2_domain_name_group(uint16_t species)
{
   if (species < 300u)
      return DW2_DOMAIN_NAME_GROUP_LOW;
   if (species >= 400u && species <= 500u)
      return DW2_DOMAIN_NAME_GROUP_MID;
   return DW2_DOMAIN_NAME_GROUP_HIGH;
}

/* 8001e6a8 then 8001e758: the record holds the name offset at +0 relative to
 * the bank base and the species id in bits 1..15 of +4. */
static bool dw2_domain_species_name(const uint8_t *ram, size_t ram_size,
      uint16_t species, char *out, size_t out_size)
{
   size_t base_offset;
   size_t table_offset;
   size_t index;

   if (!dw2_domain_group_base(ram, ram_size, dw2_domain_name_group(species),
            &base_offset)
         || !dw2_domain_group_entry(ram, ram_size, base_offset, 0u,
            DW2_DOMAIN_NAME_RECORD_SIZE, &table_offset))
      return false;
   for (index = 0; index < DW2_DOMAIN_NAME_RECORD_LIMIT; index++)
   {
      size_t record_offset = table_offset
         + index * DW2_DOMAIN_NAME_RECORD_SIZE;
      uint32_t candidate;
      uint32_t relative;

      if (!dw2_domain_ram_range(ram_size, record_offset,
               DW2_DOMAIN_NAME_RECORD_SIZE))
         return false;
      candidate = (dw2_domain_read_u32(ram, record_offset + 4u) >> 1)
         & 0x7fffu;
      if (!candidate)
         return false;
      if (candidate != species)
         continue;
      relative = dw2_domain_read_u32(ram, record_offset);
      if (relative >= ram_size - base_offset)
         return false;
      return beetle_accessibility_dw2_decode_text(ram, ram_size,
            0x80000000u | (uint32_t)(base_offset + relative),
            BEETLE_DW2_TEXT_MENU, out, out_size) && out[0];
   }
   return false;
}

/* 8001d8c4 then 8001d934/8001d958: the class nibbles share +4. */
static bool dw2_domain_species_class(const uint8_t *ram, size_t ram_size,
      uint16_t species, const char **attribute, const char **stage)
{
   /* SYS_MESS 1fd00c3..5 and 1fd00c6..9, the ids 800671f0 draws. */
   static const char *const attributes[DW2_DOMAIN_CLASS_COUNT] = {
      "Data", "Vaccine", "Virus"
   };
   static const char *const stages[DW2_DOMAIN_STAGE_COUNT] = {
      "Rookie", "Champion", "Ultimate", "Mega"
   };
   size_t base_offset;
   size_t index;

   if (!dw2_domain_group_base(ram, ram_size, DW2_DOMAIN_SPECIES_GROUP,
            &base_offset))
      return false;
   for (index = 0; index < DW2_DOMAIN_SPECIES_RECORD_LIMIT; index++)
   {
      size_t record_offset = base_offset
         + index * DW2_DOMAIN_SPECIES_RECORD_SIZE;
      int16_t candidate;
      uint16_t classes;

      if (!dw2_domain_ram_range(ram_size, record_offset,
               DW2_DOMAIN_SPECIES_RECORD_SIZE))
         return false;
      candidate = dw2_domain_read_s16(ram, record_offset);
      if (!candidate)
         return false;
      if (candidate != (int16_t)species)
         continue;
      classes = dw2_domain_read_u16(ram,
            record_offset + DW2_DOMAIN_SPECIES_CLASS_OFFSET);
      if ((classes & 0x0fu) >= DW2_DOMAIN_CLASS_COUNT
            || ((classes >> 4) & 0x0fu) >= DW2_DOMAIN_STAGE_COUNT)
         return false;
      *attribute = attributes[classes & 0x0fu];
      *stage = stages[(classes >> 4) & 0x0fu];
      return true;
   }
   return false;
}

/* 800681bc only ever stores pointers into the floor entity table, so reject
 * anything that is not an aligned, live, wild-Digimon entry. */
static bool dw2_domain_target_entity(const uint8_t *ram, size_t ram_size,
      uint32_t address, size_t *entity_offset)
{
   size_t root_offset;
   size_t table_offset;
   size_t offset;
   size_t relative;
   int16_t entity_count;

   if (!dw2_domain_pointer(ram, ram_size, DW2_DOMAIN_ROOT_POINTER_OFFSET,
            DW2_DOMAIN_ROOT_ENTITY_TABLE_OFFSET, &root_offset))
      return false;
   entity_count = dw2_domain_read_s16(ram,
         root_offset + DW2_DOMAIN_ROOT_ENTITY_COUNT_OFFSET);
   if (entity_count <= 0 || (uint32_t)entity_count > DW2_DOMAIN_ENTITY_LIMIT)
      return false;
   table_offset = root_offset + DW2_DOMAIN_ROOT_ENTITY_TABLE_OFFSET;
   if (!dw2_domain_address_to_offset(address, ram_size,
            DW2_DOMAIN_ENTITY_STRIDE, &offset)
         || offset < table_offset)
      return false;
   relative = offset - table_offset;
   if (relative % DW2_DOMAIN_ENTITY_STRIDE
         || relative / DW2_DOMAIN_ENTITY_STRIDE >= (size_t)entity_count)
      return false;
   if (!(dw2_domain_read_u16(ram, offset) & DW2_DOMAIN_ENTITY_ACTIVE_FLAG)
         || ram[offset + DW2_DOMAIN_ENTITY_TYPE_OFFSET]
            != DW2_DOMAIN_ENTITY_WILD_TYPE)
      return false;
   *entity_offset = offset;
   return true;
}

static bool dw2_domain_submit_target(const uint8_t *ram, size_t ram_size,
      uint32_t task_address, size_t task_offset)
{
   beetle_dw2_menu_snapshot_t snapshot;
   size_t data_offset;
   size_t state_offset;
   size_t entity_offset;
   size_t record_offset;
   uint32_t count;
   uint32_t cursor;
   uint32_t members;
   uint32_t member;
   uint16_t lead_species = 0u;
   int written;

   if (!dw2_domain_window_data(ram, ram_size, task_offset,
            DW2_DOMAIN_TARGET_TASK_TYPE, DW2_DOMAIN_TARGET_CALLBACK,
            DW2_DOMAIN_TARGET_DATA_SIZE, &data_offset)
         || dw2_domain_read_u32(ram,
            data_offset + DW2_DOMAIN_TARGET_WINDOW_OFFSET) != 0x1000u
         || !dw2_domain_state(ram, ram_size, &state_offset))
      return false;

   count = dw2_domain_read_u32(ram,
         state_offset + DW2_DOMAIN_TARGET_COUNT_OFFSET);
   cursor = dw2_domain_read_u32(ram,
         state_offset + DW2_DOMAIN_TARGET_CURSOR_OFFSET);
   if (!count || count > DW2_DOMAIN_TARGET_LIMIT || cursor >= count)
      return false;
   if (!dw2_domain_target_entity(ram, ram_size,
            dw2_domain_read_u32(ram, state_offset
               + DW2_DOMAIN_TARGET_TABLE_OFFSET + (size_t)cursor * 4u),
            &entity_offset))
      return false;
   if (!dw2_domain_pointer(ram, ram_size,
            entity_offset + DW2_DOMAIN_ENTITY_RECORD_OFFSET,
            DW2_DOMAIN_PARTY_RECORD_SIZE, &record_offset))
      return false;
   members = ram[record_offset + DW2_DOMAIN_PARTY_COUNT_OFFSET];
   if (!members || members > DW2_DOMAIN_PARTY_LIMIT)
      return false;

   memset(&snapshot, 0, sizeof(snapshot));
   for (member = 0; member < members; member++)
   {
      const char *attribute;
      const char *stage;
      char name[BEETLE_DW2_MENU_TEXT_MAX];
      char sentence[BEETLE_DW2_MENU_TEXT_MAX];
      uint16_t species = (uint16_t)dw2_domain_read_s16(ram, record_offset
            + DW2_DOMAIN_PARTY_SPECIES_OFFSET + (size_t)member * 2u);
      int16_t level = dw2_domain_read_s16(ram, record_offset
            + DW2_DOMAIN_PARTY_LEVEL_OFFSET + (size_t)member * 2u);

      if (!species || level < 0
            || !dw2_domain_species_name(ram, ram_size, species, name,
               sizeof(name))
            || !dw2_domain_species_class(ram, ram_size, species, &attribute,
               &stage))
         return false;
      if (!member)
         lead_species = species;
      /* 800671f0 draws SYS_MESS 1fd0081 "EL" next to the 80067454 number. */
      written = snprintf(sentence, sizeof(sentence), "%s EL %d, %s, %s",
            name, (int)level, attribute, stage);
      if (written < 0 || (size_t)written >= sizeof(sentence)
            || !dw2_domain_append(snapshot.label, sizeof(snapshot.label),
               sentence, ". "))
         return false;
   }

   written = snprintf(snapshot.details, sizeof(snapshot.details),
         "%u of %u.", (unsigned)(cursor + 1u), (unsigned)count);
   if (written < 0 || (size_t)written >= sizeof(snapshot.details))
      return false;

   snapshot.active = true;
   snapshot.enabled = true;
   snapshot.layer = 5;
   snapshot.context = BEETLE_DW2_MENU_INSPECTION;
   snapshot.task = task_address;
   /* Same wording the floor navigation already uses for these entities. */
   strcpy(snapshot.title, "Enemy Digimon");
   snapshot.focus_id = 0x45000000u | ((cursor & 0x0fu) << 20)
      | ((count & 0x0fu) << 16) | lead_species;
   return beetle_accessibility_dw2_menu_submit(&snapshot);
}

void beetle_accessibility_dw2_domain_menu_poll(const uint8_t *ram,
      size_t ram_size, uint32_t overlay_tag)
{
   uint32_t count;
   uint32_t index;

   if (!ram || overlay_tag != DW2_DOMAIN_STAG4000_TAG
         || !dw2_domain_ram_range(ram_size, DW2_DOMAIN_TASK_COUNT_OFFSET, 4u))
      return;
   count = dw2_domain_read_u32(ram, DW2_DOMAIN_TASK_COUNT_OFFSET);
   if (!count || count > DW2_DOMAIN_TASK_LIST_LIMIT
         || !dw2_domain_ram_range(ram_size, DW2_DOMAIN_TASK_LIST_OFFSET,
            (size_t)count * 4u))
      return;

   for (index = 0; index < count; index++)
   {
      uint32_t task_address = dw2_domain_read_u32(ram,
            DW2_DOMAIN_TASK_LIST_OFFSET + (size_t)index * 4u);
      size_t task_offset;

      if (!dw2_domain_address_to_offset(task_address, ram_size, 0x38u,
               &task_offset))
         continue;
      if (dw2_domain_submit_list(ram, ram_size, task_address, task_offset))
         continue;
      dw2_domain_submit_target(ram, ram_size, task_address, task_offset);
   }
}
