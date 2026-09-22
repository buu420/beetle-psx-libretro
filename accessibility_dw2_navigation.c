#include "accessibility_dw2_navigation.h"

#include "accessibility_speech.h"
#include "accessibility_dw2_text.h"
#include "accessibility_dw2_story.h"
#include "accessibility_dw2_npc.h"

#include <stdio.h>
#include <string.h>

#define DW2_NAV_RAM_MASK 0x001fffffu
#define DW2_NAV_STAG2000_TAG 0x00000192u
#define DW2_NAV_STAG3000_TAG 0x00000193u
#define DW2_NAV_STAG4000_TAG 0x0000019au
#define DW2_NAV_TASK_COUNT_OFFSET 0x00050798u
#define DW2_NAV_TASK_LIST_OFFSET 0x0005079cu
#define DW2_NAV_TASK_LIMIT 128u
#define DW2_NAV_TASK_TYPE_OFFSET 0x00u
#define DW2_NAV_TASK_ACTOR_OFFSET 0x04u
#define DW2_NAV_TASK_MODEL_OFFSET 0x0cu
#define DW2_NAV_TASK_STATE_OFFSET 0x10u
#define DW2_NAV_TASK_SUBSTATE_OFFSET 0x14u
#define DW2_NAV_TASK_DATA_OFFSET 0x2cu
#define DW2_NAV_TASK_RENDERER_OFFSET 0x38u
#define DW2_NAV_CITY_ACTOR_TASK_TYPE 0x0302u
#define DW2_NAV_CITY_GRID_OFFSET 0x00070768u
#define DW2_NAV_CITY_GRID_SIZE 24u
#define DW2_NAV_CITY_PLAYER_BLOCK_MASK 0x7fu
#define DW2_NAV_CITY_ACTOR_VISIBLE_OFFSET 0x64u
#define DW2_NAV_CITY_ACTOR_RESOURCE_HANDLE_OFFSET 0x1cu
#define DW2_NAV_CITY_SCRIPT_MODEL_FIRST 497u
#define DW2_NAV_CITY_PLAYER_MODEL 500u
#define DW2_NAV_CITY_INTERACTION_FLAG 0x40u
#define DW2_NAV_CITY_RENDERER_X_OFFSET 0x30u
#define DW2_NAV_CITY_RENDERER_Y_OFFSET 0x38u
#define DW2_NAV_CITY_RENDERER_FACING_OFFSET 0x42u
#define DW2_NAV_CITY_RENDERER_SCREEN_OFFSET 0x68u
#define DW2_NAV_CITY_SCENE_OFFSET 0x0005f788u
#define DW2_NAV_CITY_MAIN_GATE_SCENE 1u
#define DW2_NAV_CITY_SCENE_LIMIT 0x29u
#define DW2_NAV_CITY_EXTERNAL_SCENE_MIN 0x2au
#define DW2_NAV_RESOURCE_SLOT_OFFSET 0x0005f8c8u
#define DW2_NAV_RESOURCE_SLOT_STRIDE 0x10u
#define DW2_NAV_RESOURCE_SLOT_LIMIT 0x50u
#define DW2_NAV_CITY_RESOURCE_GROUP 0x0309u
#define DW2_NAV_CITY_ACTOR_RECORD_SIZE 0x2cu
#define DW2_NAV_CITY_ACTOR_RECORD_LIMIT 256u
#define DW2_NAV_CITY_ACTOR_RECORD_MODEL_OFFSET 0x00u
#define DW2_NAV_CITY_ACTOR_RECORD_MESSAGE_OFFSET 0x14u
#define DW2_NAV_CITY_ACTOR_RECORD_MESSAGE_COUNT 6u
#define DW2_NAV_DOMAIN_STATE_POINTER_OFFSET 0x00072b60u
#define DW2_NAV_DOMAIN_ROOT_POINTER_OFFSET 0x0005071cu
#define DW2_NAV_DOMAIN_ROOT_ENTITY_COUNT_OFFSET 0x0cu
#define DW2_NAV_DOMAIN_ROOT_ENTITY_TABLE_OFFSET 0x18u
#define DW2_NAV_DOMAIN_ROOT_CONTROL_FLAGS_OFFSET 0x0ba0u
#define DW2_NAV_DOMAIN_ROOT_CONTROL_ROTATION_OFFSET 0x0ba4u
#define DW2_NAV_DOMAIN_ROOT_DIMENSIONS_OFFSET 0x0e54u
#define DW2_NAV_DOMAIN_ROOT_TILES_OFFSET 0x0e58u
#define DW2_NAV_DOMAIN_ENTITY_LIMIT 41u
#define DW2_NAV_DOMAIN_ENTITY_STRIDE 0x48u
#define DW2_NAV_DOMAIN_ENTITY_ACTIVE_FLAG 0x8000u
#define DW2_NAV_DOMAIN_ENTITY_TYPE_OFFSET 0x08u
#define DW2_NAV_DOMAIN_ENTITY_DATA_OFFSET 0x10u
#define DW2_NAV_DOMAIN_ENTITY_X_OFFSET 0x18u
#define DW2_NAV_DOMAIN_ENTITY_Y_OFFSET 0x1au
#define DW2_NAV_DOMAIN_ENTITY_REMAINING_OFFSET 0x20u
#define DW2_NAV_DOMAIN_ENTITY_FACING_OFFSET 0x0bu
#define DW2_NAV_DOMAIN_TILE_STRIDE 4u
#define DW2_NAV_DOMAIN_TILE_WALKABLE_FLAG 0x8000u
#define DW2_NAV_DOMAIN_TILE_BLOCK_MASK 0x0030u
#define DW2_NAV_DOMAIN_TILE_BLOCK_VALUE 0x0020u
#define DW2_NAV_NPC_NAME_CACHE_LIMIT 64u

typedef enum dw2_route_direction
{
   DW2_ROUTE_RIGHT = 0,
   DW2_ROUTE_UP,
   DW2_ROUTE_LEFT,
   DW2_ROUTE_DOWN
} dw2_route_direction_t;

typedef struct dw2_npc_name_cache_entry
{
   bool valid;
   uint8_t scene;
   uint32_t id;
   uint32_t generation;
   int16_t x;
   int16_t y;
   char name[BEETLE_DW2_NAV_LABEL_MAX];
} dw2_npc_name_cache_entry_t;

typedef struct dw2_city_transition
{
   uint32_t address;
   uint32_t generation;
   uint8_t x;
   uint8_t y;
   uint8_t destination;
} dw2_city_transition_t;

static const int8_t dw2_direction_x[] = { 1, 0, -1, 0 };
static const int8_t dw2_direction_y[] = { 0, -1, 0, 1 };
static const char *const dw2_direction_name[] = {
   "Right", "Up", "Left", "Down"
};

static beetle_dw2_navigation_snapshot_t dw2_snapshot;
static bool dw2_snapshot_valid;
static bool dw2_suppressed;
static unsigned dw2_category;
static bool dw2_selected_valid;
static uint32_t dw2_selected_id;
static uint32_t dw2_selected_generation;
static beetle_dw2_navigation_target_kind_t dw2_selected_kind;
static bool dw2_route_active;
static bool dw2_story_tracking;
static uint32_t dw2_tracked_story_id;
static bool dw2_resume_pending;
static bool dw2_pending_start;
static bool dw2_pending_repeat;
static uint8_t dw2_route[BEETLE_DW2_NAV_MAX_CELLS];
static size_t dw2_route_length;
static bool dw2_route_waiting_obstruction;
static int16_t dw2_route_obstruction_x;
static int16_t dw2_route_obstruction_y;
static uint8_t dw2_route_obstruction_kind;
static uint8_t dw2_route_obstruction_level;
static int16_t dw2_segment_start_x;
static int16_t dw2_segment_start_y;
static int16_t dw2_segment_end_x;
static int16_t dw2_segment_end_y;
#define DW2_NAV_ROUTE_STATES (BEETLE_DW2_NAV_MAX_CELLS * 4u)
#define DW2_NAV_ROUTE_NONE 0xffffu
static uint16_t dw2_state_depth[DW2_NAV_ROUTE_STATES];
static uint16_t dw2_state_turns[DW2_NAV_ROUTE_STATES];
static uint16_t dw2_state_parent[DW2_NAV_ROUTE_STATES];
static uint8_t dw2_state_first_direction[DW2_NAV_ROUTE_STATES];
static uint16_t dw2_current_states[DW2_NAV_ROUTE_STATES];
static uint16_t dw2_next_states[DW2_NAV_ROUTE_STATES];
static uint8_t dw2_reverse_route[BEETLE_DW2_NAV_MAX_CELLS];
static uint8_t dw2_current_city_scene;
static dw2_npc_name_cache_entry_t
   dw2_npc_name_cache[DW2_NAV_NPC_NAME_CACHE_LIMIT];
static size_t dw2_npc_name_cache_next;
static char dw2_last_location[BEETLE_DW2_NAV_LABEL_MAX];
static char dw2_pending_location[BEETLE_DW2_NAV_LABEL_MAX];

static void dw2_speak_selection(void);
static size_t dw2_native_task_count(const uint8_t *ram, size_t ram_size);
static bool dw2_native_task(const uint8_t *ram, size_t ram_size,
      size_t index, uint32_t *address, size_t *offset);
static const char *dw2_obstruction_color(unsigned level);
static void dw2_obstruction_target_label(uint8_t kind, uint8_t level,
      char *text, size_t text_size);

static bool dw2_has_native_direction_map(
      const beetle_dw2_navigation_snapshot_t *snapshot)
{
   unsigned direction;

   for (direction = 0; direction < 4; direction++)
      if (snapshot->direction_x[direction]
            || snapshot->direction_y[direction])
         return true;
   return false;
}

static void dw2_direction_step(
      const beetle_dw2_navigation_snapshot_t *snapshot,
      unsigned direction, int *x, int *y)
{
   if (dw2_has_native_direction_map(snapshot))
   {
      *x = snapshot->direction_x[direction];
      *y = snapshot->direction_y[direction];
   }
   else
   {
      *x = dw2_direction_x[direction];
      *y = dw2_direction_y[direction];
   }
}

static bool dw2_direction_map_valid(
      const beetle_dw2_navigation_snapshot_t *snapshot)
{
   unsigned direction;

   if (!dw2_has_native_direction_map(snapshot))
      return true;
   for (direction = 0; direction < 4; direction++)
   {
      int x = snapshot->direction_x[direction];
      int y = snapshot->direction_y[direction];

      if (x < -1 || x > 1 || y < -1 || y > 1 || (!x && !y))
         return false;
   }
   return true;
}

static bool dw2_direction_maps_equal(
      const beetle_dw2_navigation_snapshot_t *left,
      const beetle_dw2_navigation_snapshot_t *right)
{
   unsigned direction;

   for (direction = 0; direction < 4; direction++)
   {
      int left_x;
      int left_y;
      int right_x;
      int right_y;

      dw2_direction_step(left, direction, &left_x, &left_y);
      dw2_direction_step(right, direction, &right_x, &right_y);
      if (left_x != right_x || left_y != right_y)
         return false;
   }
   return true;
}

static uint16_t dw2_native_u16(const uint8_t *ram, size_t offset)
{
   return (uint16_t)ram[offset] | ((uint16_t)ram[offset + 1] << 8);
}

static int16_t dw2_native_s16(const uint8_t *ram, size_t offset)
{
   return (int16_t)dw2_native_u16(ram, offset);
}

static uint32_t dw2_native_u32(const uint8_t *ram, size_t offset)
{
   return (uint32_t)ram[offset]
      | ((uint32_t)ram[offset + 1] << 8)
      | ((uint32_t)ram[offset + 2] << 16)
      | ((uint32_t)ram[offset + 3] << 24);
}

static int32_t dw2_native_s32(const uint8_t *ram, size_t offset)
{
   return (int32_t)dw2_native_u32(ram, offset);
}

static bool dw2_native_range(size_t ram_size, size_t offset, size_t length)
{
   return length && offset < ram_size && length <= ram_size - offset;
}

static bool dw2_native_address(uint32_t address, size_t ram_size,
      size_t length, size_t *offset)
{
   uint32_t region = address & 0xffe00000u;
   size_t candidate;

   if (!offset || (region != 0x80000000u && region != 0xa0000000u))
      return false;
   candidate = address & DW2_NAV_RAM_MASK;
   if (!dw2_native_range(ram_size, candidate, length))
      return false;
   *offset = candidate;
   return true;
}

static bool dw2_native_add_target(
      beetle_dw2_navigation_snapshot_t *snapshot,
      beetle_dw2_navigation_target_kind_t kind, uint32_t id,
      uint32_t generation,
      int16_t x, int16_t y, bool approach_adjacent,
      const char *label, unsigned ordinal)
{
   beetle_dw2_navigation_target_t *target;

   if (!snapshot || snapshot->target_count >= BEETLE_DW2_NAV_MAX_TARGETS
         || x < 0 || y < 0 || x >= (int16_t)snapshot->width
         || y >= (int16_t)snapshot->height)
      return false;
   target = &snapshot->targets[snapshot->target_count++];
   memset(target, 0, sizeof(*target));
   target->kind = kind;
   target->id = id;
   target->generation = generation;
   target->x = x;
   target->y = y;
   target->approach_adjacent = approach_adjacent;
   if (ordinal)
      snprintf(target->label, sizeof(target->label), "%s %u", label,
            ordinal);
   else
      snprintf(target->label, sizeof(target->label), "%s", label);
   return true;
}

static void dw2_native_domain_location(const uint8_t *ram, size_t ram_size,
      size_t root_offset, size_t dimensions_offset,
      char *text, size_t text_size)
{
   size_t index;
   size_t count;
   size_t length;
   size_t digit_start;
   char label[BEETLE_DW2_NAV_LABEL_MAX];
   uint8_t encoded[17];
   bool visible = false;

   /* 80070dc0 copies the current floor's label into this 16-byte HUD buffer;
    * 80066720 passes it to the text renderer. The floor index itself changes
    * before the fade, so use the displayed label and a ready HUD instead. */
   if (ram[root_offset + 2] != 0
         || !dw2_native_range(ram_size, dimensions_offset + 0xe, 16)
         || !ram[dimensions_offset + 0xd]
         || ram[dimensions_offset + 0xd] > 16
         || !dw2_native_range(ram_size, 0x726c8u, 0x14u)
         || dw2_native_u32(ram, 0x726ccu) != 0x80066720u
         || dw2_native_u32(ram, 0x726d8u) != 0x18u)
      return;
   count = dw2_native_task_count(ram, ram_size);
   for (index = 0; index < count; index++)
   {
      size_t task_offset;
      size_t data_offset;
      uint32_t type;
      if (!dw2_native_task(ram, ram_size, index, NULL, &task_offset))
         continue;
      type = dw2_native_u32(ram, task_offset);
      if (type != 0x209u
            || dw2_native_u32(ram, task_offset + 0x10) != 1
            || dw2_native_u32(ram, task_offset + 0x14) != 1
            || !dw2_native_address(dw2_native_u32(ram, task_offset + 0x2c),
               ram_size, 0x18, &data_offset)
            || !dw2_native_u32(ram, data_offset + 0xc))
         continue;
      visible = true;
      break;
   }
   if (!visible)
      return;
   /* The native byte count permits a full 16-character label without a
    * terminator inside the field. Decode an explicitly bounded copy. */
   memset(encoded, 0xff, sizeof(encoded));
   memcpy(encoded, ram + dimensions_offset + 0xe,
         ram[dimensions_offset + 0xd]);
   if (!beetle_accessibility_dw2_decode_text(encoded, sizeof(encoded),
            0x80000000u, BEETLE_DW2_TEXT_MENU,
            label, sizeof(label)))
      return;

   length = strlen(label);
   digit_start = length;
   if (length && label[length - 1] == 'F')
   {
      digit_start = length - 1;
      while (digit_start && label[digit_start - 1] >= '0'
            && label[digit_start - 1] <= '9')
         digit_start--;
   }
   if (digit_start && digit_start < length - 1)
      snprintf(text, text_size, "%.*s. Floor %.*s.", (int)digit_start,
            label, (int)(length - digit_start - 1), label + digit_start);
   else
      snprintf(text, text_size, "%s.", label);
}

static bool dw2_native_domain_snapshot(const uint8_t *ram, size_t ram_size,
      beetle_dw2_navigation_snapshot_t *snapshot)
{
   static const uint8_t control_direction[] = { 6, 4, 2, 0 };
   static const int8_t world_x[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
   static const int8_t world_y[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
   uint32_t state_address;
   uint32_t root_address;
   uint32_t player_address;
   uint32_t dimensions_address;
   uint32_t tiles_address;
   size_t state_offset;
   size_t root_offset;
   size_t player_offset;
   size_t dimensions_offset;
   size_t tiles_offset;
   uint16_t entity_count;
   unsigned enemy_count = 0;
   unsigned hazard_count = 0;
   unsigned treasure_count = 0;
   unsigned rotation = 0;
   unsigned direction;
   unsigned player_facing;
   size_t entity_index;
   size_t cell;
   uint8_t acid_level[BEETLE_DW2_NAV_MAX_CELLS];
   uint8_t acid_seen[BEETLE_DW2_NAV_MAX_CELLS];


   if (!ram || !snapshot
         || !dw2_native_range(ram_size,
            DW2_NAV_DOMAIN_STATE_POINTER_OFFSET, 4)
         || !dw2_native_range(ram_size,
            DW2_NAV_DOMAIN_ROOT_POINTER_OFFSET, 4))
      return false;
   state_address = dw2_native_u32(ram,
         DW2_NAV_DOMAIN_STATE_POINTER_OFFSET);
   root_address = dw2_native_u32(ram,
         DW2_NAV_DOMAIN_ROOT_POINTER_OFFSET);
   if (!dw2_native_address(state_address, ram_size, 8, &state_offset)
         || !dw2_native_address(root_address, ram_size,
            DW2_NAV_DOMAIN_ROOT_TILES_OFFSET + 4, &root_offset))
      return false;
   player_address = dw2_native_u32(ram, state_offset + 4);
   dimensions_address = dw2_native_u32(ram,
         root_offset + DW2_NAV_DOMAIN_ROOT_DIMENSIONS_OFFSET);
   tiles_address = dw2_native_u32(ram,
         root_offset + DW2_NAV_DOMAIN_ROOT_TILES_OFFSET);
   if (!dw2_native_address(player_address, ram_size,
            DW2_NAV_DOMAIN_ENTITY_REMAINING_OFFSET + 2, &player_offset)
         || !dw2_native_address(dimensions_address, ram_size, 4,
            &dimensions_offset))
      return false;

   memset(snapshot, 0, sizeof(*snapshot));
   memset(acid_level, 0, sizeof(acid_level));
   memset(acid_seen, 0, sizeof(acid_seen));
   snapshot->context = BEETLE_DW2_NAV_CONTEXT_DOMAIN;
   snapshot->approach_eight_way = true;
   snapshot->width = dw2_native_u16(ram, dimensions_offset);
   snapshot->height = dw2_native_u16(ram, dimensions_offset + 2);
   if (!snapshot->width || snapshot->width > BEETLE_DW2_NAV_MAX_WIDTH
         || !snapshot->height
         || snapshot->height > BEETLE_DW2_NAV_MAX_HEIGHT
         || !dw2_native_address(tiles_address, ram_size,
            (size_t)snapshot->width * snapshot->height
               * DW2_NAV_DOMAIN_TILE_STRIDE, &tiles_offset))
      return false;
   if ((dw2_native_u32(ram,
            root_offset + DW2_NAV_DOMAIN_ROOT_CONTROL_FLAGS_OFFSET) & 2u)
         != 0)
      rotation = ram[root_offset
         + DW2_NAV_DOMAIN_ROOT_CONTROL_ROTATION_OFFSET] & 7u;
   for (direction = 0; direction < 4; direction++)
   {
      unsigned world_direction =
         (control_direction[direction] + rotation) & 7u;

      snapshot->direction_x[direction] = world_x[world_direction];
      snapshot->direction_y[direction] = world_y[world_direction];
   }
   player_facing = ram[player_offset
      + DW2_NAV_DOMAIN_ENTITY_FACING_OFFSET] & 7u;
   for (direction = 0; direction < 4; direction++)
   {
      if (((control_direction[direction] + rotation) & 7u)
            == player_facing)
      {
         snapshot->preferred_direction_valid = true;
         snapshot->preferred_direction = (uint8_t)direction;
         break;
      }
   }
   snapshot->player_x = dw2_native_s16(ram,
         player_offset + DW2_NAV_DOMAIN_ENTITY_X_OFFSET);
   snapshot->player_y = dw2_native_s16(ram,
         player_offset + DW2_NAV_DOMAIN_ENTITY_Y_OFFSET);
   snapshot->player_settled = dw2_native_u16(ram,
         player_offset + DW2_NAV_DOMAIN_ENTITY_REMAINING_OFFSET) == 0;
   if (snapshot->player_x < 0 || snapshot->player_y < 0
         || snapshot->player_x >= (int16_t)snapshot->width
         || snapshot->player_y >= (int16_t)snapshot->height)
      return false;
   dw2_native_domain_location(ram, ram_size, root_offset,
         dimensions_offset, snapshot->location,
         sizeof(snapshot->location));

   for (cell = 0; cell < (size_t)snapshot->width * snapshot->height; cell++)
   {
      uint16_t flags = dw2_native_u16(ram,
            tiles_offset + cell * DW2_NAV_DOMAIN_TILE_STRIDE);
      size_t x = cell % snapshot->width;
      size_t y = cell / snapshot->width;

      snapshot->blocked[y * BEETLE_DW2_NAV_MAX_WIDTH + x] =
         (flags & DW2_NAV_DOMAIN_TILE_WALKABLE_FLAG) == 0
         || (flags & DW2_NAV_DOMAIN_TILE_BLOCK_MASK)
            == DW2_NAV_DOMAIN_TILE_BLOCK_VALUE;
      if ((flags & 0x000fu) >= 8u && (flags & 0x000fu) <= 12u)
         acid_level[y * BEETLE_DW2_NAV_MAX_WIDTH + x] =
            (uint8_t)((flags & 0x000fu) - 7u);
   }

   entity_count = dw2_native_u16(ram,
         root_offset + DW2_NAV_DOMAIN_ROOT_ENTITY_COUNT_OFFSET);
   if (!entity_count || entity_count > DW2_NAV_DOMAIN_ENTITY_LIMIT
         || !dw2_native_range(ram_size,
            root_offset + DW2_NAV_DOMAIN_ROOT_ENTITY_TABLE_OFFSET,
            (size_t)entity_count * DW2_NAV_DOMAIN_ENTITY_STRIDE))
      return false;
   for (entity_index = 0; entity_index < entity_count; entity_index++)
   {
      size_t entity_offset = root_offset
         + DW2_NAV_DOMAIN_ROOT_ENTITY_TABLE_OFFSET
         + entity_index * DW2_NAV_DOMAIN_ENTITY_STRIDE;
      uint32_t entity_address = root_address
         + DW2_NAV_DOMAIN_ROOT_ENTITY_TABLE_OFFSET
         + (uint32_t)entity_index * DW2_NAV_DOMAIN_ENTITY_STRIDE;
      uint8_t type;
      uint32_t data_address;
      int16_t x;
      int16_t y;

      if ((dw2_native_u16(ram, entity_offset)
               & DW2_NAV_DOMAIN_ENTITY_ACTIVE_FLAG) == 0)
         continue;
      type = ram[entity_offset + DW2_NAV_DOMAIN_ENTITY_TYPE_OFFSET];
      data_address = dw2_native_u32(ram,
            entity_offset + DW2_NAV_DOMAIN_ENTITY_DATA_OFFSET);
      x = dw2_native_s16(ram,
            entity_offset + DW2_NAV_DOMAIN_ENTITY_X_OFFSET);
      y = dw2_native_s16(ram,
            entity_offset + DW2_NAV_DOMAIN_ENTITY_Y_OFFSET);
      if (x < 0 || y < 0 || x >= (int16_t)snapshot->width
            || y >= (int16_t)snapshot->height)
         continue;
      /* Ghidra's live interaction dispatcher and SYS_MESS table identify
       * types 6-12 as visible Electro-Spores, Big Rocks, Land Mines, and
       * four Bug Nest variants. They are native occupancy until removed. */
      if ((type >= 1 && type <= 4) || (type >= 6 && type <= 12))
         snapshot->blocked[(size_t)y * BEETLE_DW2_NAV_MAX_WIDTH
            + (size_t)x] = 1;
      if (type == 1)
      {
         /* 8006c0b4 sets 1000 upon entering the enemy's room; 8006bfb0
          * uses it for the map icon, not existence. Enumerate every active
          * enemy on this floor, including undiscovered spawned 0xc002 slots. */
         if (!dw2_native_add_target(snapshot,
                  BEETLE_DW2_NAV_TARGET_ENEMY_DIGIMON, entity_address,
                  data_address, x, y, true, "Enemy Digimon", ++enemy_count))
            return false;
      }
      else if (type == 2)
      {
         /* Direct portal guidance is an explicit player preference.
          * Native types 2/3 are usable before the map marker is discovered;
          * retain the active-slot and coordinate checks above. */
         if (!dw2_native_add_target(snapshot,
                  BEETLE_DW2_NAV_TARGET_FLOOR_PORTAL, entity_address,
                  data_address, x, y, false, "Floor Portal", 0))
            return false;

      }
      else if (type == 3)
      {
         if (!dw2_native_add_target(snapshot,
                  BEETLE_DW2_NAV_TARGET_EXIT_PORTAL, entity_address,
                  data_address, x, y, false, "Exit Portal", 0))
            return false;
      }
      else if (type == 4)
      {
         size_t data_offset;
         /* 8006c84c marks discovered boxes with 1000 and clears their flags
          * on collection. 80067db4's 4000/0400 bits control model drawing;
          * 800682dc/8006e200 permit X interaction with an active type-4 slot
          * without those display bits. Include all boxes on this floor.
          * +1=ff is an opened box whose item remains (inventory was full).
          * Neither contents nor hidden booby-trap strength are disclosed. */
         if (!dw2_native_address(data_address, ram_size, 2, &data_offset))
            continue;
         if (!dw2_native_add_target(snapshot,
                  BEETLE_DW2_NAV_TARGET_TREASURE_BOX, entity_address,
                  data_address, x, y, true, ram[data_offset + 1] == 0xff
                     ? "Opened Treasure Box" : "Treasure Box",
                  ++treasure_count))
            return false;
      }
      else if (type >= 6 && type <= 12)
      {
         size_t data_offset;
         uint8_t level = 0;
         char label[BEETLE_DW2_NAV_LABEL_MAX];
         size_t obstacle_cell = (size_t)y * BEETLE_DW2_NAV_MAX_WIDTH
            + (size_t)x;

         if (dw2_native_address(data_address, ram_size, 2, &data_offset))
            level = ram[data_offset + 1];
         snapshot->obstruction_kind[obstacle_cell] = type;
         snapshot->obstruction_level[obstacle_cell] = level;
         dw2_obstruction_target_label(type, level, label, sizeof(label));
         if (!dw2_native_add_target(snapshot,
                  BEETLE_DW2_NAV_TARGET_HAZARD, entity_address,
                  data_address, x, y, true, label, 0))
            return false;
         hazard_count++;
      }
   }
   for (cell = 0; cell < (size_t)snapshot->width * snapshot->height;
         cell++)
   {
      static const char *const tires[] = {
         "", "Ring TIRES", "ChainTIRES", "PlateTIRES",
         "Aero TIRES", "GraviTIRES"
      };
      size_t stored_cell;
      size_t queue[BEETLE_DW2_NAV_MAX_CELLS];
      size_t queue_read = 0;
      size_t queue_count = 0;
      int16_t target_x;
      int16_t target_y;
      uint8_t level;
      char label[BEETLE_DW2_NAV_LABEL_MAX];

      target_x = (int16_t)(cell % snapshot->width);
      target_y = (int16_t)(cell / snapshot->width);
      stored_cell = (size_t)target_y * BEETLE_DW2_NAV_MAX_WIDTH
         + (size_t)target_x;
      level = acid_level[stored_cell];
      if (!level || acid_seen[stored_cell])
         continue;
      acid_seen[stored_cell] = 1;
      queue[queue_count++] = stored_cell;
      while (queue_read < queue_count)
      {
         size_t current = queue[queue_read++];
         int x = (int)(current % BEETLE_DW2_NAV_MAX_WIDTH);
         int y = (int)(current / BEETLE_DW2_NAV_MAX_WIDTH);
         unsigned direction_index;

         for (direction_index = 0; direction_index < 4;
               direction_index++)
         {
            int next_x = x + dw2_direction_x[direction_index];
            int next_y = y + dw2_direction_y[direction_index];
            size_t next;

            if (next_x < 0 || next_y < 0
                  || next_x >= (int)snapshot->width
                  || next_y >= (int)snapshot->height)
               continue;
            next = (size_t)next_y * BEETLE_DW2_NAV_MAX_WIDTH
               + (size_t)next_x;
            if (acid_seen[next] || acid_level[next] != level)
               continue;
            acid_seen[next] = 1;
            if (queue_count < BEETLE_DW2_NAV_MAX_CELLS)
               queue[queue_count++] = next;
         }
      }
      snprintf(label, sizeof(label), "%s Acid Swamp. %s prevents damage",
            dw2_obstruction_color(level), tires[level]);
      /* Leave room for the current Story steps on dense hazard floors. */
      if (snapshot->target_count >= BEETLE_DW2_NAV_MAX_TARGETS
            - BEETLE_DW2_STORY_MAX_OBJECTIVES)
         break;
      if (!dw2_native_add_target(snapshot,
               BEETLE_DW2_NAV_TARGET_HAZARD,
               tiles_address + (uint32_t)(cell * DW2_NAV_DOMAIN_TILE_STRIDE),
               level, target_x, target_y, true, label, 0))
         break;
      hazard_count++;
   }
   (void)hazard_count;
   snapshot->blocked[(size_t)snapshot->player_y
      * BEETLE_DW2_NAV_MAX_WIDTH + (size_t)snapshot->player_x] = 0;
   return true;
}

static size_t dw2_native_task_count(const uint8_t *ram, size_t ram_size)
{
   uint32_t count;

   if (!dw2_native_range(ram_size, DW2_NAV_TASK_COUNT_OFFSET, 4))
      return 0;
   count = dw2_native_u32(ram, DW2_NAV_TASK_COUNT_OFFSET);
   if (!count || count > DW2_NAV_TASK_LIMIT
         || !dw2_native_range(ram_size, DW2_NAV_TASK_LIST_OFFSET,
            (size_t)count * 4))
      return 0;
   return (size_t)count;
}

static bool dw2_native_task(const uint8_t *ram, size_t ram_size,
      size_t index, uint32_t *address, size_t *offset)
{
   uint32_t task_address = dw2_native_u32(ram,
         DW2_NAV_TASK_LIST_OFFSET + index * 4);
   size_t task_offset;

   if (!dw2_native_address(task_address, ram_size,
            DW2_NAV_TASK_RENDERER_OFFSET + 4, &task_offset))
      return false;
   if (address)
      *address = task_address;
   if (offset)
      *offset = task_offset;
   return true;
}

static bool dw2_native_resource_group_base(const uint8_t *ram,
      size_t ram_size, uint32_t group, size_t *base_offset)
{
   size_t slot_index;

   if (!base_offset)
      return false;
   for (slot_index = 0; slot_index < DW2_NAV_RESOURCE_SLOT_LIMIT;
         slot_index++)
   {
      size_t slot = DW2_NAV_RESOURCE_SLOT_OFFSET
         + slot_index * DW2_NAV_RESOURCE_SLOT_STRIDE;
      uint32_t base_address;

      if (!dw2_native_range(ram_size, slot, DW2_NAV_RESOURCE_SLOT_STRIDE))
         return false;
      if (dw2_native_u32(ram, slot + 4u) != group)
         continue;
      base_address = dw2_native_u32(ram, slot + 0x0cu);
      if (dw2_native_address(base_address, ram_size, 4u, base_offset))
         return true;
   }
   return false;
}

static bool dw2_native_city_resource(const uint8_t *ram, size_t ram_size,
      uint32_t scene, size_t *resource_offset)
{
   size_t base_offset;
   uint32_t relative;
   size_t index;

   if (!resource_offset || scene == 0
         || !dw2_native_resource_group_base(ram, ram_size,
            DW2_NAV_CITY_RESOURCE_GROUP, &base_offset))
      return false;
   index = (size_t)((scene + 0x0308ffffu) & 0xffffu);
   if (index > (SIZE_MAX / 4) - 1
         || !dw2_native_range(ram_size, base_offset, (index + 1) * 4))
      return false;
   relative = dw2_native_u32(ram, base_offset + index * 4);
   if (relative >= ram_size - base_offset
         || !dw2_native_range(ram_size, base_offset + relative, 0x10))
      return false;
   *resource_offset = base_offset + relative;
   return true;
}

/* Read-only equivalent of 80021e78. Overlay predicates (IDs >= 4000) are
 * deliberately unresolved; guessing false could select a future greeting. */
static bool dw2_native_city_transition_table(const uint8_t *ram,
      size_t ram_size, uint8_t scene, uint32_t *table_address,
      size_t *table_offset)
{
   size_t resource_offset;
   uint32_t address;
   size_t offset;

   if (!dw2_native_city_resource(ram, ram_size, scene, &resource_offset))
      return false;
   address = dw2_native_u32(ram, resource_offset + 0x0c);
   if (!dw2_native_address(address, ram_size, 4, &offset))
      return false;
   if (table_address)
      *table_address = address;
   if (table_offset)
      *table_offset = offset;
   return true;
}

static bool dw2_native_city_transition_record(const uint8_t *ram,
      size_t ram_size, uint8_t scene, uint32_t table_address,
      size_t table_offset, size_t index,
      dw2_city_transition_t *transition)
{
   size_t record;

   if (!transition || index >= BEETLE_DW2_NAV_MAX_TARGETS
         || !table_address)
      return false;
   record = table_offset + index * 4;
   if (!dw2_native_range(ram_size, record, 4) || !ram[record])
      return false;
   transition->address = table_address + (uint32_t)index * 4u;
   transition->x = ram[record];
   transition->y = ram[record + 1];
   transition->destination = ram[record + 2];
   transition->generation = ((uint32_t)scene << 16)
      | ((uint32_t)transition->destination << 8) | ram[record + 3];
   return true;
}

static void dw2_native_city_transitions(const uint8_t *ram, size_t ram_size,
      beetle_dw2_navigation_snapshot_t *snapshot)
{
   uint8_t scene;
   size_t transition_index;
   size_t original_target_count;
   uint32_t transitions_address;
   size_t transitions_offset;
   dw2_city_transition_t transitions[BEETLE_DW2_NAV_MAX_TARGETS];
   uint8_t groups[BEETLE_DW2_NAV_MAX_TARGETS];
   size_t transition_count = 0;
   unsigned exit_count = 0;

   if (!snapshot)
      return;
   original_target_count = snapshot->target_count;
   if (!dw2_native_range(ram_size, DW2_NAV_CITY_SCENE_OFFSET, 4))
      return;
   /* FUN_80066714 uses LBU for the current scene before constructing the
    * native 0x0309 resource ID. The surrounding word contains other state. */
   scene = ram[DW2_NAV_CITY_SCENE_OFFSET];
   if (!dw2_native_city_transition_table(ram, ram_size, scene,
            &transitions_address, &transitions_offset))
      return;
   for (transition_index = 0;
          transition_index < BEETLE_DW2_NAV_MAX_TARGETS;
          transition_index++)
   {
      dw2_city_transition_t *transition = &transitions[transition_index];
      if (!dw2_native_city_transition_record(ram, ram_size, scene,
               transitions_address, transitions_offset, transition_index,
               transition))
      {
         if (!dw2_native_range(ram_size,
                  transitions_offset + transition_index * 4u, 4))
            return;
         break;
      }
      if (transition->x >= snapshot->width
            || transition->y >= snapshot->height)
         return;
      groups[transition_index] = (uint8_t)transition_index;
      transition_count++;
   }
   /* No terminator within the bound means an incomplete table. */
   if (transition_count == BEETLE_DW2_NAV_MAX_TARGETS)
      return;
   for (transition_index = 0; transition_index < transition_count;
         transition_index++)
   {
      size_t other;
      for (other = 0; other < transition_index; other++)
      {
         const dw2_city_transition_t *a = &transitions[transition_index];
         const dw2_city_transition_t *b = &transitions[other];
         int dx = (int)a->x - b->x;
         int dy = (int)a->y - b->y;
         size_t member;
         uint8_t from = groups[transition_index];
         uint8_t to = groups[other];
         if (dx < 0) dx = -dx;
         if (dy < 0) dy = -dy;
         if (from == to || a->generation != b->generation || dx + dy > 1)
            continue;
         if (from < to)
         {
            uint8_t swap = from;
            from = to;
            to = swap;
         }
         for (member = 0; member < transition_count; member++)
            if (groups[member] == from)
               groups[member] = to;
      }
   }
   for (transition_index = 0; transition_index < transition_count;
         transition_index++)
   {
      const dw2_city_transition_t *transition = &transitions[transition_index];
      beetle_dw2_navigation_target_t *target;
      size_t member;
      if (groups[transition_index] != transition_index)
         continue;
      /* FUN_8006a744 treats every record as a trigger tile. Connected tiles
       * with the same destination AND arrival index form one doorway. Keep
       * all its tiles as route goals so a blocked edge cannot hide the door. */
      if (!dw2_native_add_target(snapshot, BEETLE_DW2_NAV_TARGET_EXIT,
               transition->address, transition->generation,
               transition->x, transition->y, false, "Exit", ++exit_count))
      {
         snapshot->target_count = original_target_count;
         return;
      }
      target = &snapshot->targets[snapshot->target_count - 1];
      for (member = transition_index; member < transition_count; member++)
         if (groups[member] == transition_index)
         {
            size_t tile = target->entrance_tile_count++;
            target->entrance_tiles[tile][0] = transitions[member].x;
            target->entrance_tiles[tile][1] = transitions[member].y;
         }
   }
}

static const char *dw2_native_city_cached_name(uint8_t scene,
      uint32_t id, uint32_t generation, int16_t x, int16_t y)
{
   size_t index;

   for (index = 0; index < DW2_NAV_NPC_NAME_CACHE_LIMIT; index++)
   {
      const dw2_npc_name_cache_entry_t *entry =
         &dw2_npc_name_cache[index];

      if (!entry->valid || entry->scene != scene)
         continue;
      if ((entry->id == id && entry->generation == generation)
            || (entry->x == x && entry->y == y))
         return entry->name;
   }
   return NULL;
}

static bool dw2_native_city_snapshot(const uint8_t *ram, size_t ram_size,
      beetle_dw2_navigation_snapshot_t *snapshot)
{
   size_t task_count;
   size_t task_index;
   unsigned npc_count = 0;
   bool player_found = false;
   size_t grid_x;
   size_t grid_y;
   uint8_t scene = 0;

   if (!ram || !snapshot
         || !dw2_native_range(ram_size, DW2_NAV_CITY_GRID_OFFSET,
            DW2_NAV_CITY_GRID_SIZE * DW2_NAV_CITY_GRID_SIZE))
      return false;
   task_count = dw2_native_task_count(ram, ram_size);
   if (!task_count)
      return false;
   if (dw2_native_range(ram_size, DW2_NAV_CITY_SCENE_OFFSET, 1))
      scene = ram[DW2_NAV_CITY_SCENE_OFFSET];
   dw2_current_city_scene = scene;

   memset(snapshot, 0, sizeof(*snapshot));
   snapshot->context = BEETLE_DW2_NAV_CONTEXT_CITY;
   snprintf(snapshot->location, sizeof(snapshot->location), "%s",
         beetle_accessibility_dw2_story_scene_name(scene));
   snapshot->width = DW2_NAV_CITY_GRID_SIZE;
   snapshot->height = DW2_NAV_CITY_GRID_SIZE;
   for (grid_x = 0; grid_x < DW2_NAV_CITY_GRID_SIZE; grid_x++)
   {
      for (grid_y = 0; grid_y < DW2_NAV_CITY_GRID_SIZE; grid_y++)
      {
         uint8_t flags = ram[DW2_NAV_CITY_GRID_OFFSET
            + grid_x * DW2_NAV_CITY_GRID_SIZE + grid_y];

         snapshot->blocked[grid_y * BEETLE_DW2_NAV_MAX_WIDTH + grid_x] =
            (flags & DW2_NAV_CITY_PLAYER_BLOCK_MASK) != 0;
      }
   }

   for (task_index = 0; task_index < task_count; task_index++)
   {
      uint32_t task_address;
      size_t task_offset;
      uint32_t type;
      uint32_t actor;
      uint16_t model;
      uint32_t data_address;
      uint32_t renderer_address;
      size_t data_offset;
      size_t renderer_offset;
      int32_t world_x;
      int32_t world_y;
      int64_t adjusted_x;
      int64_t adjusted_y;
      int64_t tile_x;
      int64_t tile_y;
      bool actor_settled;
      int16_t x;
      int16_t y;

      if (!dw2_native_task(ram, ram_size, task_index,
               &task_address, &task_offset)
            || dw2_native_u32(ram,
               task_offset + DW2_NAV_TASK_STATE_OFFSET) != 1)
         continue;
      type = dw2_native_u32(ram,
            task_offset + DW2_NAV_TASK_TYPE_OFFSET);
      if (type != DW2_NAV_CITY_ACTOR_TASK_TYPE)
         continue;
      actor = dw2_native_u32(ram,
            task_offset + DW2_NAV_TASK_ACTOR_OFFSET);
      model = dw2_native_u16(ram,
            task_offset + DW2_NAV_TASK_MODEL_OFFSET);
      data_address = dw2_native_u32(ram,
            task_offset + DW2_NAV_TASK_DATA_OFFSET);
      if (!dw2_native_address(data_address, ram_size,
               DW2_NAV_CITY_ACTOR_VISIBLE_OFFSET + 4, &data_offset))
         continue;
      renderer_address = dw2_native_u32(ram,
            task_offset + DW2_NAV_TASK_RENDERER_OFFSET);
      if (!dw2_native_address(renderer_address, ram_size,
               DW2_NAV_CITY_RENDERER_SCREEN_OFFSET + 4, &renderer_offset))
         continue;
      /* FUN_80067504 is the game's live grid-position helper. It derives the
       * tile from renderer coordinates; data +4/+6 are only spawn fields. */
      world_x = dw2_native_s32(ram,
            renderer_offset + DW2_NAV_CITY_RENDERER_X_OFFSET);
      world_y = dw2_native_s32(ram,
            renderer_offset + DW2_NAV_CITY_RENDERER_Y_OFFSET);
      adjusted_x = (int64_t)world_x + 0x4500;
      adjusted_y = (int64_t)world_y + 0x4500;
      tile_x = adjusted_x / 0x600;
      tile_y = 0x16 - adjusted_y / 0x600;
      if (tile_x < 0 || tile_y < 0
            || tile_x >= (int64_t)snapshot->width
            || tile_y >= (int64_t)snapshot->height)
         continue;
      x = (int16_t)tile_x;
      y = (int16_t)tile_y;
      /* FUN_80067568 supplies the native movement-settle test. */
      actor_settled = (((int64_t)world_x + 0x12c73) % 0x600) < 0xe7
         && (((int64_t)world_y + 0x12c73) % 0x600) < 0xe7;
      if (actor == 0)
      {
         snapshot->player_x = x;
         snapshot->player_y = y;
         snapshot->player_settled = actor_settled;
         {
            unsigned native_direction = (dw2_native_u16(ram,
                  renderer_offset + DW2_NAV_CITY_RENDERER_FACING_OFFSET)
                  & 0x0fffu) >> 10;

            snapshot->preferred_direction_valid = true;
            snapshot->preferred_direction = (uint8_t)(3u - native_direction);
         }
         player_found = true;
      }
      /* FUN_8006adf8 keeps both idle/talking (substate 0) and walking
       * (substate 1) actors live. List the whole current area regardless of
       * 8006b7c8/800209f8 screen culling. Keep the native model-presence gate:
       * 8006aa4c uses models 497-499 for script actors, and 500 for the player.
       * Talk readiness and tile occupancy are transient, not NPC existence. */
      else if (actor == 1
            && (model < DW2_NAV_CITY_SCRIPT_MODEL_FIRST
               || model > DW2_NAV_CITY_PLAYER_MODEL)
            && dw2_native_u32(ram,
               task_offset + DW2_NAV_TASK_SUBSTATE_OFFSET) <= 1
            && dw2_native_u32(ram,
               data_offset + DW2_NAV_CITY_ACTOR_VISIBLE_OFFSET) != 0)
      {
         uint32_t generation = dw2_native_u32(ram, data_offset
            + DW2_NAV_CITY_ACTOR_RESOURCE_HANDLE_OFFSET);
         char native_name[BEETLE_DW2_NAV_LABEL_MAX];
         bool native_named = beetle_accessibility_dw2_npc_name(ram, ram_size,
               task_offset, data_offset, native_name, sizeof(native_name));
         const char *cached = native_named ? NULL
            : dw2_native_city_cached_name(scene, task_address,
               generation, x, y);
         const char *label = native_named ? native_name
            : cached ? cached : "Person";

         if (!dw2_native_add_target(snapshot, BEETLE_DW2_NAV_TARGET_NPC,
                  task_address, generation, x, y, true, label,
                  native_named || cached ? 0 : npc_count + 1))
            return false;
         npc_count++;
      }
   }
   if (!player_found)
      return false;
   snapshot->blocked[(size_t)snapshot->player_y
      * BEETLE_DW2_NAV_MAX_WIDTH + (size_t)snapshot->player_x] = 0;
   /* FUN_8006a744 consumes the per-scene four-byte transition records at
    * resource offset +0x0c directly.  Task 0x0304 merely manages the
    * transition and is not itself an exit target. */
   dw2_native_city_transitions(ram, ram_size, snapshot);
   return true;
}

static unsigned dw2_story_world(unsigned scene)
{
   if (scene >= 33 && scene <= 38) return 1;
   if (scene == 39 || scene == 44) return 2;
   if (scene == 43 || scene == 46) return 1;
   return 0;
}

static unsigned dw2_story_domain_world(unsigned domain)
{
   if ((domain >= 16 && domain <= 23) || domain == 31) return 1;
   if (domain == 27 || domain == 28) return 2;
   return 0;
}

static const char *dw2_story_area(unsigned scene)
{
   if (scene <= 18 || scene == 41 || scene == 45) return "Digital City";
   if (scene <= 22) return "Device Dome";
   if (scene <= 26) return "Meditation Dome";
   if (scene <= 29) return "Archive Port";
   if (scene <= 32 || scene == 40) return "Shuttle Port";
   if (scene <= 36 || scene == 46) return "File City";
   if (scene <= 38) return "Archive Ship";
   return "Shuttle Port";
}

/* Find the first doorway in this room on the native city graph. Rooms in
 * other buildings are joined by an area-selection screen, not a walkable
 * corridor. Remember an external doorway reached by BFS as the fallback. */
static bool dw2_story_city_door(const uint8_t *ram, size_t ram_size,
      unsigned scene, unsigned destination, dw2_city_transition_t *out)
{
   uint8_t queue[DW2_NAV_CITY_SCENE_LIMIT];
   uint8_t seen[DW2_NAV_CITY_SCENE_LIMIT+1] = {0};
   dw2_city_transition_t first[DW2_NAV_CITY_SCENE_LIMIT+1];
   dw2_city_transition_t external;
   size_t head=0, tail=0, i;
   bool have_external=false;
   if (!scene || scene>DW2_NAV_CITY_SCENE_LIMIT) return false;
   memset(first,0,sizeof(first)); memset(&external,0,sizeof(external));
   queue[tail++]=(uint8_t)scene; seen[scene]=1;
   while (head<tail)
   {
      unsigned node=queue[head++];
      size_t table;
      uint32_t address;
      if (!dw2_native_city_transition_table(ram,ram_size,(uint8_t)node,&address,&table)) continue;
      for (i=0;i<BEETLE_DW2_NAV_MAX_TARGETS;i++)
      {
         dw2_city_transition_t edge, hop;
         unsigned next;
         if (!dw2_native_city_transition_record(ram,ram_size,(uint8_t)node,address,table,i,&edge)) break;
         hop=node==scene ? edge : first[node]; next=edge.destination;
         if (next==destination) { *out=hop; return true; }
         if (next>=DW2_NAV_CITY_EXTERNAL_SCENE_MIN)
         {
            if (!have_external) { external=hop; have_external=true; }
            continue;
         }
         if (!next || seen[next]) continue;
         seen[next]=1; first[next]=hop;
         if (tail<sizeof(queue)) queue[tail++]=(uint8_t)next;
      }
   }
   if (have_external) *out=external;
   return have_external;
}

static void dw2_story_copy_target(beetle_dw2_navigation_target_t *dest,
      const beetle_dw2_navigation_target_t *source)
{
   uint32_t objective=dest->id;
   char label[BEETLE_DW2_NAV_LABEL_MAX];
   snprintf(label,sizeof(label),"%s",dest->label);
   *dest=*source;
   dest->id=objective;
   dest->generation=source->id ^ source->generation;
   dest->kind=BEETLE_DW2_NAV_TARGET_STORY_EVENT;
   dest->information_only=source->information_only;
   dest->portal=!source->information_only
      && (source->kind==BEETLE_DW2_NAV_TARGET_FLOOR_PORTAL
         || source->kind==BEETLE_DW2_NAV_TARGET_EXIT_PORTAL);
   snprintf(dest->label,sizeof(dest->label),"%s",label);
}

static void dw2_story_city_step(const uint8_t *ram, size_t ram_size,
      beetle_dw2_navigation_snapshot_t *snapshot,
      const beetle_dw2_story_objective_t *objective,
      beetle_dw2_navigation_target_t *target)
{
   unsigned scene=ram[DW2_NAV_CITY_SCENE_OFFSET];
   unsigned destination=(unsigned)objective->scene;
   unsigned world=objective->domain>=0
      ? dw2_story_domain_world((unsigned)objective->domain) : dw2_story_world(destination);
   const char *npc=objective->npc;
   const char *travel=NULL;
   bool boarding=false;
   char action[96]="";
   size_t i;
   dw2_city_transition_t door;
   if (!destination && objective->domain<0) return;

   if (dw2_story_world(scene)!=world)
   {
      if (dw2_story_world(scene)==1)
      { destination=38; npc=""; travel="Archive Ship Teleport Gate to Directory Continent"; }
      else if (dw2_story_world(scene)==2)
      { destination=39; npc=""; travel="Shuttle Port Warp Gate to Directory Continent"; }
      else if (world==1)
      { destination=28; npc=""; travel="Archive Port Teleport Gate to File Island"; }
      else if (world==2)
      { destination=31; npc=""; travel="Shuttle Port Warp Gate to Kernel Zone"; }
   }
   else if (objective->domain>=0)
   {
      /* File City and the Kernel shuttle have physical world-map exits.
       * Only Digital City uses Carol's boarding dialogue. */
      destination=world==1 ? 43 : world==2 ? 44 : 1;
      npc=world==0 ? "Carol" : "";
      if (world==0 && ram[0x5f664]==10) npc="New GAIA";
      snprintf(action,sizeof(action),"Choose %s",beetle_accessibility_dw2_story_domain_name((unsigned)objective->domain));
      boarding=world==0;
   }

   /* Digital City's Main Gate exit leads to its building-selection menu.
    * File City's Main Gate also has a separate exit to world map43. */
   if ((scene<=18 || scene==41 || scene==45) && destination>18)
   { destination=1; npc="Carol"; boarding=true; }
   else if (scene>=33 && scene<=36 && (destination<33 || destination>36))
   { destination=43; npc=""; boarding=false; }

   if (scene>=42 && scene<=46)
   {
      if (scene==45 || scene==46)
      {
         unsigned building=destination;
         if (building>=3 && building<=14) building=3+4*((building-3)/4);
         if (building==18) building=17;
         if (scene==45 && (building>18 || !building)) building=1;
         if (scene==46 && (building<33 || building>36)) building=33;
         /* A city selection menu offers buildings. Reach its Main Gate
          * before the separate world map can offer the domain itself. */
         if (dw2_story_world(scene)!=world)
            building=scene==46 ? 33 : 1;
         snprintf(action,sizeof(action),"Choose %s",beetle_accessibility_dw2_story_scene_name(building));
      }
      else if (travel)
         snprintf(action,sizeof(action),"Choose %s",dw2_story_area(destination));
      else if (objective->domain<0)
         snprintf(action,sizeof(action),"Choose %s",dw2_story_area(destination));
      snprintf(target->label,sizeof(target->label),"%.160s %.90s.",objective->label,action);
      return;
   }
   if (scene==destination || (scene==41 && destination==1))
   {
      if (travel && !boarding)
      {
         snprintf(target->label,sizeof(target->label),"%.156s Use the %.80s.",objective->label,travel);
         return;
      }
      if (!npc[0]) return;
      for (i=0;i<snapshot->target_count;i++)
      {
         beetle_dw2_navigation_target_t *candidate=&snapshot->targets[i];
         if (candidate->kind!=BEETLE_DW2_NAV_TARGET_NPC || !strstr(candidate->label,npc)) continue;
         if (boarding)
            snprintf(target->label,sizeof(target->label),"%.156s Board at %.70s.",objective->label,candidate->label);
         dw2_story_copy_target(target,candidate);
         return;
      }
      /* Don't substitute a random visible NPC when the requested person is
       * in a cutscene, walking in, or temporarily absent. */
      return;
   }
   if (!dw2_story_city_door(ram,ram_size,scene,destination,&door)) return;
   if (travel)
      snprintf(target->label,sizeof(target->label),"%.156s Via %.80s.",objective->label,travel);
   else
      snprintf(target->label,sizeof(target->label),"%.170s Via %.72s.",objective->label,
            destination==43 ? "File City Main Gate" : destination==44 ? "Kernel Zone map"
               : beetle_accessibility_dw2_story_scene_name(destination));
   target->x=(int16_t)door.x; target->y=(int16_t)door.y;
   target->generation=door.address ^ door.generation;
   target->information_only=false;
   /* Preserve the whole native doorway, so a blocked left tile doesn't
    * invalidate a reachable opening two tiles to its right. */
   for (i=0;i<snapshot->target_count;i++)
   {
      const beetle_dw2_navigation_target_t *candidate=&snapshot->targets[i];
      unsigned j;
      if (candidate->kind!=BEETLE_DW2_NAV_TARGET_EXIT) continue;
      for (j=0;j<candidate->entrance_tile_count;j++)
         if (candidate->entrance_tiles[j][0]==door.x && candidate->entrance_tiles[j][1]==door.y)
         { dw2_story_copy_target(target,candidate); return; }
   }
}

/* Final-floor mission records in the original US floor message resources.
 * These are encounter/rescue/reward triggers, not NPC positions or arbitrary
 * enemies. Order matters: Soft's Crimson record 1 precedes Chaos Lord 0;
 * Boot's tutorials and the post-mission "quiet" records are not destinations.
 * 8006e60c filters spawn conditions into controller+144; 8006e6cc consumes a
 * trigger on its EXACT tile, so an adjacent-enemy route does not suffice.
 * Coordinates always come from the live list and must match its resource. */
static bool dw2_story_domain_encounter(const uint8_t *ram, size_t ram_size,
      const beetle_dw2_navigation_snapshot_t *snapshot, unsigned domain,
      const beetle_dw2_story_objective_t *objective,
      beetle_dw2_navigation_target_t *target)
{
   static const struct {
      uint16_t group;
      uint8_t domain, count, records[6];
   } missions[] = {
      {0x0d00, 0,2,{0,1}}, {0x0dfa, 1,2,{0,1}},
      {0x0dfb, 2,3,{0,3,1}}, {0x0dfd, 3,1,{0}},
      {0x0e15, 4,2,{0,1}}, {0x0e16, 5,1,{0}},
      {0x0e18, 6,3,{0,3,4}}, {0x0e19, 7,1,{0}},
      {0x0e1a, 8,1,{0}}, {0x0e22, 9,4,{1,2,0,3}},
      {0x0e23,10,2,{0,1}}, {0x0e24,11,1,{0}},
      {0x0e25,12,1,{0}}, {0x0e37,13,6,{1,5,2,6,0,4}},
      {0x0e26,14,2,{0,1}}, {0x0d26,15,3,{0,1,3}},
      {0x0d38,16,2,{0,1}}, {0x0d39,17,1,{0}},
      {0x0d3a,18,2,{1,2}}, {0x0d3d,19,1,{0}},
      {0x0d6d,20,2,{0,1}}, {0x0d6e,21,2,{0,1}},
      {0x0d70,22,2,{0,1}}, {0x0d73,23,4,{1,3,0,2}},
      {0x0e3b,24,2,{0,2}}, {0x0e3e,25,2,{0,2}},
      {0x0e3f,26,2,{0,2}}, {0x0e43,27,6,{0,1,6,7,8,9}},
      {0x0e44,28,2,{1,2}}, {0x0cdf,29,1,{1}},
      {0x0e27,30,2,{0,1}}, {0x0d3e,31,2,{0,1}}
   };
   size_t state, descriptor, base, table, coordinates, mission, order, i;
   uint32_t group, count, relative;
   if (!dw2_native_address(dw2_native_u32(ram,
               DW2_NAV_DOMAIN_STATE_POINTER_OFFSET),ram_size,0x170u,&state)
         || !dw2_native_address(dw2_native_u32(ram,state+0x10u),
            ram_size,8u,&descriptor)) return false;
   group=dw2_native_u32(ram,descriptor+4u);
   count=dw2_native_u32(ram,state+0x16cu);
   if (!count || count>BEETLE_DW2_NPC_EVENT_LIMIT
         || !dw2_native_range(ram_size,0x5d560u,4u)
         || group!=dw2_native_u32(ram,0x5d560u)) return false;
   for (mission=0;mission<sizeof(missions)/sizeof(missions[0]);mission++)
      if (missions[mission].domain==domain && missions[mission].group==group) break;
   if (mission==sizeof(missions)/sizeof(missions[0])
         || !dw2_native_resource_group_base(ram,ram_size,group,&base)
         || !dw2_native_range(ram_size,base,8u)) return false;
   relative=dw2_native_u32(ram,base);
   if (relative>=ram_size-base) return false;
   table=base+relative;
   relative=dw2_native_u32(ram,base+4u);
   if (relative>=ram_size-base) return false;
   coordinates=base+relative;
   for (order=0;order<missions[mission].count;order++)
   {
      unsigned record=missions[mission].records[order];
      size_t actor=table+record*0x2cu, coordinate;
      if (!dw2_native_range(ram_size,actor,0x2cu)
            || dw2_native_u16(ram,actor)!=499u) continue;
      coordinate=coordinates+ram[actor+4u]*12u;
      if (!dw2_native_range(ram_size,coordinate,2u)) continue;
      for (i=0;i<count;i++)
      {
         size_t entry=state+0x144u+i*8u;
         uint16_t x=dw2_native_u16(ram,entry), y=dw2_native_u16(ram,entry+2u);
         if (dw2_native_u32(ram,entry+4u)!=record
               || x>=snapshot->width || y>=snapshot->height
               || (unsigned)x+1u!=ram[coordinate]
               || (unsigned)y+1u!=ram[coordinate+1u]) continue;
         target->x=(int16_t)x; target->y=(int16_t)y;
         target->generation=0x4d000000u | (group<<8) | record;
         target->information_only=false;
         target->approach_adjacent=false;
         target->portal=false;
         snprintf(target->label,sizeof(target->label),
               "%.170s. Continue to the mission encounter.",objective->label);
         return true;
      }
   }
   return false;
}

static void dw2_story_domain_step(const uint8_t *ram, size_t ram_size,
      beetle_dw2_navigation_snapshot_t *snapshot,
      const beetle_dw2_story_objective_t *objective,
      beetle_dw2_navigation_target_t *target)
{
   size_t root, i;
   int domain=-1;
   const beetle_dw2_navigation_target_t *portal=NULL;
   if (dw2_native_address(dw2_native_u32(ram,DW2_NAV_DOMAIN_ROOT_POINTER_OFFSET),ram_size,0x105au,&root))
      domain=dw2_native_s16(ram,root+0x1058u);
   for (i=0;i<snapshot->target_count;i++)
   {
      const beetle_dw2_navigation_target_t *candidate=&snapshot->targets[i];
      if (candidate->kind==(domain==objective->domain
               ? BEETLE_DW2_NAV_TARGET_FLOOR_PORTAL : BEETLE_DW2_NAV_TARGET_EXIT_PORTAL)) portal=candidate;
   }
   if (portal)
   {
      snprintf(target->label,sizeof(target->label),"%.180s. Use the %s",objective->label,
            domain==objective->domain ? "Floor Portal" : "Exit Portal");
      dw2_story_copy_target(target,portal);
   }
   else if (domain!=objective->domain)
      snprintf(target->label,sizeof(target->label),"%.160s. Leave this domain using Auto Pilot or an Exit Portal.",objective->label);
   else if (!dw2_story_domain_encounter(ram,ram_size,snapshot,
            (unsigned)domain,objective,target))
      snprintf(target->label,sizeof(target->label),"%.170s. Mission destination not available on this floor.",objective->label);
}

static void dw2_native_story_targets(const uint8_t *ram, size_t ram_size,
      beetle_dw2_navigation_snapshot_t *snapshot)
{
   beetle_dw2_story_objective_t objectives[BEETLE_DW2_STORY_MAX_OBJECTIVES];
   size_t count=beetle_accessibility_dw2_story_objectives(ram,ram_size,
         objectives,BEETLE_DW2_STORY_MAX_OBJECTIVES), i;
   for (i=0;i<count && snapshot->target_count<BEETLE_DW2_NAV_MAX_TARGETS;i++)
   {
      beetle_dw2_navigation_target_t target;
      memset(&target,0,sizeof(target));
      target.kind=BEETLE_DW2_NAV_TARGET_STORY_EVENT;
      target.id=objectives[i].id;
      target.x=snapshot->player_x; target.y=snapshot->player_y;
      target.information_only=true;
      snprintf(target.label,sizeof(target.label),"%s",objectives[i].label);
      if (snapshot->context==BEETLE_DW2_NAV_CONTEXT_CITY)
         dw2_story_city_step(ram,ram_size,snapshot,&objectives[i],&target);
      else
         dw2_story_domain_step(ram,ram_size,snapshot,&objectives[i],&target);
      /* Story id follows the quest; generation follows the immediate step.
       * Portal entity slots are reused on each floor, and information-only
       * map steps otherwise all have generation zero. */
      if (snapshot->context==BEETLE_DW2_NAV_CONTEXT_CITY)
         target.generation=(target.generation * 16777619u)
            ^ ram[DW2_NAV_CITY_SCENE_OFFSET];
      else if (snapshot->location[0])
      {
         const unsigned char *p=(const unsigned char *)snapshot->location;
         while (*p) target.generation=(target.generation ^ *p++) * 16777619u;
      }
      snapshot->targets[snapshot->target_count++]=target;
   }
}

static unsigned dw2_category_count(beetle_dw2_navigation_context_t context)
{
   if (context == BEETLE_DW2_NAV_CONTEXT_DOMAIN)
      return 6;
   if (context == BEETLE_DW2_NAV_CONTEXT_CITY)
      return 3;
   return 0;
}

static beetle_dw2_navigation_target_kind_t dw2_category_kind(void)
{
   static const beetle_dw2_navigation_target_kind_t domain_kinds[] = {
      BEETLE_DW2_NAV_TARGET_STORY_EVENT,
      BEETLE_DW2_NAV_TARGET_ENEMY_DIGIMON,
      BEETLE_DW2_NAV_TARGET_FLOOR_PORTAL,
      BEETLE_DW2_NAV_TARGET_EXIT_PORTAL,
      BEETLE_DW2_NAV_TARGET_HAZARD,
      BEETLE_DW2_NAV_TARGET_TREASURE_BOX
   };
   static const beetle_dw2_navigation_target_kind_t city_kinds[] = {
      BEETLE_DW2_NAV_TARGET_EXIT,
      BEETLE_DW2_NAV_TARGET_NPC,
      BEETLE_DW2_NAV_TARGET_STORY_EVENT
   };

   if (dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_DOMAIN
         && dw2_category < sizeof(domain_kinds) / sizeof(domain_kinds[0]))
      return domain_kinds[dw2_category];
   if (dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_CITY
         && dw2_category < sizeof(city_kinds) / sizeof(city_kinds[0]))
      return city_kinds[dw2_category];
   return BEETLE_DW2_NAV_TARGET_STORY_EVENT;
}

static const char *dw2_category_label(void)
{
   static const char *const domain_labels[] = {
      "Story Events", "Enemy Digimon", "Floor Portal", "Exit Portal",
      "Hazards and Obstacles", "Treasure Boxes"
   };
   static const char *const city_labels[] = {
      "Exits", "Relevant NPCs", "Story"
   };

   if (dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_DOMAIN
         && dw2_category < sizeof(domain_labels) / sizeof(domain_labels[0]))
      return domain_labels[dw2_category];
   if (dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_CITY
         && dw2_category < sizeof(city_labels) / sizeof(city_labels[0]))
      return city_labels[dw2_category];
   return "Navigation";
}

static bool dw2_snapshot_is_valid(
      const beetle_dw2_navigation_snapshot_t *snapshot)
{
   return snapshot
      && snapshot->context != BEETLE_DW2_NAV_CONTEXT_NONE
      && snapshot->width > 0
      && snapshot->width <= BEETLE_DW2_NAV_MAX_WIDTH
      && snapshot->height > 0
      && snapshot->height <= BEETLE_DW2_NAV_MAX_HEIGHT
      && snapshot->player_x >= 0
      && snapshot->player_x < (int16_t)snapshot->width
      && snapshot->player_y >= 0
      && snapshot->player_y < (int16_t)snapshot->height
      && snapshot->target_count <= BEETLE_DW2_NAV_MAX_TARGETS
      && (!snapshot->preferred_direction_valid
         || snapshot->preferred_direction < 4)
      && dw2_direction_map_valid(snapshot);
}

static size_t dw2_targets_in_category(void)
{
   beetle_dw2_navigation_target_kind_t kind = dw2_category_kind();
   size_t count = 0;
   size_t index;

   for (index = 0; index < dw2_snapshot.target_count; index++)
      if (dw2_snapshot.targets[index].kind == kind)
         count++;
   return count;
}

static const beetle_dw2_navigation_target_t *dw2_target_at(size_t wanted)
{
   beetle_dw2_navigation_target_kind_t kind = dw2_category_kind();
   size_t seen = 0;
   size_t index;

   for (index = 0; index < dw2_snapshot.target_count; index++)
   {
      if (dw2_snapshot.targets[index].kind != kind)
         continue;
      if (seen == wanted)
         return &dw2_snapshot.targets[index];
      seen++;
   }
   return NULL;
}

static const beetle_dw2_navigation_target_t *dw2_selected_target(
      size_t *ordinal)
{
   beetle_dw2_navigation_target_kind_t kind = dw2_category_kind();
   const beetle_dw2_navigation_target_t *first = NULL;
   size_t seen = 0;
   size_t index;

   for (index = 0; index < dw2_snapshot.target_count; index++)
   {
      const beetle_dw2_navigation_target_t *target =
         &dw2_snapshot.targets[index];
      if (target->kind != kind)
         continue;
      if (!first)
         first = target;
      if (dw2_selected_valid && target->id == dw2_selected_id
            && target->generation == dw2_selected_generation
            && target->kind == dw2_selected_kind)
      {
         if (ordinal)
            *ordinal = seen;
         return target;
      }
      seen++;
   }

   if (dw2_selected_valid)
   {
      if (ordinal)
         *ordinal = 0;
      return NULL;
   }
   if (first)
   {
      dw2_selected_valid = true;
      dw2_selected_id = first->id;
      dw2_selected_generation = first->generation;
      dw2_selected_kind = first->kind;
      if (ordinal)
         *ordinal = 0;
   }
   else
   {
      dw2_selected_valid = false;
      if (ordinal)
         *ordinal = 0;
   }
   return first;
}

static const char *dw2_target_label(
      const beetle_dw2_navigation_target_t *target)
{
   if (target && target->label[0])
      return target->label;
   return dw2_category_label();
}

static const char *dw2_obstruction_color(unsigned level)
{
   static const char *const colors[] = {
      "", "Yellow", "Green", "Blue", "Purple", "Red"
   };

   return level > 0 && level < sizeof(colors) / sizeof(colors[0])
      ? colors[level] : "Unknown-level";
}

static void dw2_obstruction_description(uint8_t kind, uint8_t level,
      char *text, size_t text_size)
{
   static const char *const arms[] = {
      "", "Shovel ARM", "Drill ARM", "Jet ARM", "Laser ARM",
      "Magnum ARM"
   };
   const char *color = dw2_obstruction_color(level);
   unsigned strength = level >= 1 && level <= 5 ? level : 1;

   if (!text || !text_size)
      return;
   switch (kind)
   {
      case BEETLE_DW2_NAV_OBSTRUCTION_ELECTRO_SPORE:
         snprintf(text, text_size,
               "%s Electro-Spore blocks the route. Use Mag.Miss-%u or WaveMiss-%u with MissileGun.",
               color, strength, strength);
         break;
      case BEETLE_DW2_NAV_OBSTRUCTION_BIG_ROCK:
         snprintf(text, text_size,
               "%s Big Rock blocks the route. Use DrillMiss%u or WaveMiss-%u with MissileGun.",
               color, strength, strength);
         break;
      case BEETLE_DW2_NAV_OBSTRUCTION_LAND_MINE:
         snprintf(text, text_size,
               "%s Land Mine blocks the route. Use %s or stronger.",
               color, arms[strength]);
         break;
      case BEETLE_DW2_NAV_OBSTRUCTION_BIT_BUG_NEST:
         snprintf(text, text_size,
               "%s Bit Bug Nest blocks the route. Use BitBugZap%u with Bug Zapper.",
               color, strength > 3 ? 3 : strength);
         break;
      case BEETLE_DW2_NAV_OBSTRUCTION_ENERGY_BUG_NEST:
         snprintf(text, text_size,
               "%s Energy Bug Nest blocks the route. Use EP-BugZap%u with Bug Zapper.",
               color, strength > 3 ? 3 : strength);
         break;
      case BEETLE_DW2_NAV_OBSTRUCTION_RETURN_BUG_NEST:
         snprintf(text, text_size,
               "%s Return Bug Nest blocks the route. Use RetBugZap%u with Bug Zapper.",
               color, strength > 3 ? 3 : strength);
         break;
      case BEETLE_DW2_NAV_OBSTRUCTION_MEMORY_BUG_NEST:
         snprintf(text, text_size,
               "%s Memory Bug Nest blocks the route. Use MemBugZap%u with Bug Zapper.",
               color, strength > 3 ? 3 : strength);
         break;
      default:
         snprintf(text, text_size, "An obstacle blocks the route.");
         break;
   }
}

static void dw2_obstruction_target_label(uint8_t kind, uint8_t level,
      char *text, size_t text_size)
{
   static const char *const names[] = {
      "Electro-Spore", "Big Rock", "Land Mine", "Bit Bug Nest",
      "Energy Bug Nest", "Return Bug Nest", "Memory Bug Nest"
   };
   if (!text || !text_size)
      return;
   snprintf(text, text_size, "%s %s", dw2_obstruction_color(level),
         kind >= BEETLE_DW2_NAV_OBSTRUCTION_ELECTRO_SPORE
               && kind <= BEETLE_DW2_NAV_OBSTRUCTION_MEMORY_BUG_NEST
            ? names[kind - BEETLE_DW2_NAV_OBSTRUCTION_ELECTRO_SPORE]
            : "Obstacle");
}

static void dw2_speak(const char *text)
{
   if (dw2_suppressed)
      return;
   if (dw2_pending_location[0])
   {
      char speech[512];
      snprintf(speech, sizeof(speech), "%s%s%s", dw2_pending_location,
            text && *text ? " " : "", text ? text : "");
      if (beetle_accessibility_speak(speech, 8, "navigation"))
      {
         snprintf(dw2_last_location, sizeof(dw2_last_location), "%s",
               dw2_pending_location);
         dw2_pending_location[0] = '\0';
      }
   }
   else if (text && text[0])
      beetle_accessibility_speak(text, 8, "navigation");
}

static bool dw2_is_goal(const beetle_dw2_navigation_target_t *target,
      int x, int y)
{
   int distance_x;
   int distance_y;

   if (!target)
      return false;
   if (target->kind == BEETLE_DW2_NAV_TARGET_EXIT
         && target->entrance_tile_count)
   {
      unsigned tile;
      for (tile = 0; tile < target->entrance_tile_count
            && tile < BEETLE_DW2_NAV_MAX_TARGETS; tile++)
         if (x == target->entrance_tiles[tile][0]
               && y == target->entrance_tiles[tile][1])
            return true;
      return false;
   }
   distance_x = x - target->x;
   if (distance_x < 0)
      distance_x = -distance_x;
   distance_y = y - target->y;
   if (distance_y < 0)
      distance_y = -distance_y;
   if (!target->approach_adjacent)
      return distance_x == 0 && distance_y == 0;
   if (dw2_snapshot.approach_eight_way)
      return distance_x <= 1 && distance_y <= 1
         && (distance_x != 0 || distance_y != 0);
   return distance_x + distance_y == 1;
}

static bool dw2_cell_blocked(int x, int y)
{
   if (x < 0 || y < 0 || x >= (int)dw2_snapshot.width
         || y >= (int)dw2_snapshot.height)
      return true;
   return dw2_snapshot.blocked[
      (size_t)y * BEETLE_DW2_NAV_MAX_WIDTH + (size_t)x] != 0;
}

static bool dw2_cell_has_obstruction(int x, int y)
{
   if (x < 0 || y < 0 || x >= (int)dw2_snapshot.width
         || y >= (int)dw2_snapshot.height)
      return false;
   return dw2_snapshot.obstruction_kind[
      (size_t)y * BEETLE_DW2_NAV_MAX_WIDTH + (size_t)x]
         != BEETLE_DW2_NAV_OBSTRUCTION_NONE;
}

static bool dw2_blocked_target_cell_enterable(
      const beetle_dw2_navigation_target_t *target, int x, int y)
{
   return target && !target->approach_adjacent
      && dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_DOMAIN
      && (target->kind == BEETLE_DW2_NAV_TARGET_FLOOR_PORTAL
         || target->kind == BEETLE_DW2_NAV_TARGET_EXIT_PORTAL || target->portal)
      && x == target->x && y == target->y;
}

static bool dw2_step_traversable(
      const beetle_dw2_navigation_target_t *target,
      int x, int y, unsigned direction, int *next_x, int *next_y,
      bool allow_obstructions)
{
   int step_x;
   int step_y;
   int candidate_x;
   int candidate_y;

   dw2_direction_step(&dw2_snapshot, direction, &step_x, &step_y);
   candidate_x = x + step_x;
   candidate_y = y + step_y;
   if (candidate_x < 0 || candidate_y < 0
         || candidate_x >= (int)dw2_snapshot.width
         || candidate_y >= (int)dw2_snapshot.height)
      return false;
   if (dw2_cell_blocked(candidate_x, candidate_y)
         && !(allow_obstructions
            && dw2_cell_has_obstruction(candidate_x, candidate_y))
         && !dw2_blocked_target_cell_enterable(target,
            candidate_x, candidate_y))
      return false;
   if (step_x && step_y
         && (dw2_cell_blocked(x + step_x, y)
            || dw2_cell_blocked(x, y + step_y)))
      return false;
   if (next_x)
      *next_x = candidate_x;
   if (next_y)
      *next_y = candidate_y;
   return true;
}

static bool dw2_current_segment_traversable(
      const beetle_dw2_navigation_target_t *target)
{
   int x = dw2_snapshot.player_x;
   int y = dw2_snapshot.player_y;
   int step_x;
   int step_y;
   size_t steps = 0;

   if (!dw2_route_length)
      return false;
   dw2_direction_step(&dw2_snapshot, dw2_route[0], &step_x, &step_y);
   while (x != dw2_segment_end_x || y != dw2_segment_end_y)
   {
      int next_x;
      int next_y;

      if (++steps > BEETLE_DW2_NAV_MAX_CELLS
            || !dw2_step_traversable(target, x, y, dw2_route[0],
               &next_x, &next_y, false))
         return false;
      x = next_x;
      y = next_y;
   }
   return true;
}

static bool dw2_build_route(
      const beetle_dw2_navigation_target_t *target,
      bool allow_obstructions)
{
   uint16_t found = DW2_NAV_ROUTE_NONE;
   uint16_t *current = dw2_current_states;
   uint16_t *next = dw2_next_states;
   size_t current_count = 0;
   size_t index;
   unsigned direction;

   dw2_route_length = 0;
   memset(dw2_state_depth, 0xff, sizeof(dw2_state_depth));
   memset(dw2_state_turns, 0xff, sizeof(dw2_state_turns));
   memset(dw2_state_parent, 0xff, sizeof(dw2_state_parent));
   memset(dw2_state_first_direction, 0xff,
         sizeof(dw2_state_first_direction));
   for (direction = 0; direction < 4; direction++)
   {
      int next_x;
      int next_y;
      size_t next_cell;
      uint16_t state;

      if (!dw2_step_traversable(target, dw2_snapshot.player_x,
               dw2_snapshot.player_y, direction, &next_x, &next_y,
               allow_obstructions))
         continue;
      next_cell = (size_t)next_y * BEETLE_DW2_NAV_MAX_WIDTH
         + (size_t)next_x;
      state = (uint16_t)(next_cell * 4u + direction);
      dw2_state_depth[state] = 1;
      dw2_state_turns[state] = 0;
      dw2_state_parent[state] = DW2_NAV_ROUTE_NONE;
      dw2_state_first_direction[state] = (uint8_t)direction;
      current[current_count++] = state;
   }

   while (current_count)
   {
      size_t next_count = 0;
      uint16_t best_turns = DW2_NAV_ROUTE_NONE;
      unsigned best_heading_penalty = 2;
      unsigned best_first = 4;

      for (index = 0; index < current_count; index++)
      {
         uint16_t state = current[index];
         size_t cell = state / 4u;
         int x = (int)(cell % BEETLE_DW2_NAV_MAX_WIDTH);
         int y = (int)(cell / BEETLE_DW2_NAV_MAX_WIDTH);
         unsigned first = dw2_state_first_direction[state];
         unsigned heading_penalty = dw2_snapshot.preferred_direction_valid
            && dw2_snapshot.preferred_direction < 4
            && first != dw2_snapshot.preferred_direction;

         if (!dw2_is_goal(target, x, y))
            continue;
         if (found == DW2_NAV_ROUTE_NONE
               || dw2_state_turns[state] < best_turns
               || (dw2_state_turns[state] == best_turns
                  && heading_penalty < best_heading_penalty)
               || (dw2_state_turns[state] == best_turns
                  && heading_penalty == best_heading_penalty
                  && first < best_first))
         {
            found = state;
            best_turns = dw2_state_turns[state];
            best_heading_penalty = heading_penalty;
            best_first = first;
         }
      }
      if (found != DW2_NAV_ROUTE_NONE)
         break;

      for (index = 0; index < current_count; index++)
      {
         uint16_t state = current[index];
         size_t cell = state / 4u;
         int x = (int)(cell % BEETLE_DW2_NAV_MAX_WIDTH);
         int y = (int)(cell / BEETLE_DW2_NAV_MAX_WIDTH);
         unsigned incoming = state & 3u;

         for (direction = 0; direction < 4; direction++)
         {
            int next_x;
            int next_y;
            size_t next_cell;
            uint16_t next_state;
            uint16_t candidate_depth;
            uint16_t candidate_turns;
            uint8_t candidate_first =
               dw2_state_first_direction[state];
            bool first_seen;
            bool better;
            unsigned candidate_heading_penalty;
            unsigned existing_heading_penalty;

            if (!dw2_step_traversable(target, x, y, direction,
                     &next_x, &next_y, allow_obstructions))
               continue;
            next_cell = (size_t)next_y * BEETLE_DW2_NAV_MAX_WIDTH
               + (size_t)next_x;
            next_state = (uint16_t)(next_cell * 4u + direction);
            candidate_depth = (uint16_t)(dw2_state_depth[state] + 1u);
            candidate_turns = (uint16_t)(dw2_state_turns[state]
               + (incoming != direction));
            first_seen = dw2_state_depth[next_state] == DW2_NAV_ROUTE_NONE;
            candidate_heading_penalty =
               dw2_snapshot.preferred_direction_valid
               && dw2_snapshot.preferred_direction < 4
               && candidate_first != dw2_snapshot.preferred_direction;
            existing_heading_penalty =
               dw2_snapshot.preferred_direction_valid
               && dw2_snapshot.preferred_direction < 4
               && dw2_state_first_direction[next_state]
                  != dw2_snapshot.preferred_direction;
            better = first_seen
               || candidate_depth < dw2_state_depth[next_state]
               || (candidate_depth == dw2_state_depth[next_state]
                  && candidate_turns < dw2_state_turns[next_state])
               || (candidate_depth == dw2_state_depth[next_state]
                  && candidate_turns == dw2_state_turns[next_state]
                  && candidate_heading_penalty
                     < existing_heading_penalty)
               || (candidate_depth == dw2_state_depth[next_state]
                  && candidate_turns == dw2_state_turns[next_state]
                  && candidate_heading_penalty
                     == existing_heading_penalty
                  && candidate_first
                     < dw2_state_first_direction[next_state]);
            if (!better)
               continue;
            if (first_seen)
            {
               if (next_count >= DW2_NAV_ROUTE_STATES)
                  return false;
               next[next_count++] = next_state;
            }
            dw2_state_depth[next_state] = candidate_depth;
            dw2_state_turns[next_state] = candidate_turns;
            dw2_state_parent[next_state] = state;
            dw2_state_first_direction[next_state] = candidate_first;
         }
      }

      {
         uint16_t *swap = current;
         current = next;
         next = swap;
         current_count = next_count;
      }
   }

   if (found == DW2_NAV_ROUTE_NONE)
      return false;
   while (found != DW2_NAV_ROUTE_NONE)
   {
      uint8_t route_direction = (uint8_t)(found & 3u);

      if (route_direction > DW2_ROUTE_DOWN
            || dw2_route_length >= BEETLE_DW2_NAV_MAX_CELLS)
         return false;
      dw2_reverse_route[dw2_route_length++] = route_direction;
      found = dw2_state_parent[found];
   }
   for (index = 0; index < dw2_route_length; index++)
      dw2_route[index] = dw2_reverse_route[dw2_route_length - index - 1];
   return true;
}

static bool dw2_prepare_route(
      const beetle_dw2_navigation_target_t *target)
{
   int x = dw2_snapshot.player_x;
   int y = dw2_snapshot.player_y;
   size_t index;

   dw2_route_waiting_obstruction = false;
   dw2_route_obstruction_x = -1;
   dw2_route_obstruction_y = -1;
   dw2_route_obstruction_kind = BEETLE_DW2_NAV_OBSTRUCTION_NONE;
   dw2_route_obstruction_level = 0;
   if (dw2_build_route(target, false))
      return true;
   if (!dw2_build_route(target, true))
      return false;

   for (index = 0; index < dw2_route_length; index++)
   {
      int step_x;
      int step_y;
      size_t cell;

      dw2_direction_step(&dw2_snapshot, dw2_route[index],
            &step_x, &step_y);
      x += step_x;
      y += step_y;
      if (!dw2_cell_has_obstruction(x, y))
         continue;
      cell = (size_t)y * BEETLE_DW2_NAV_MAX_WIDTH + (size_t)x;
      dw2_route_waiting_obstruction = true;
      dw2_route_obstruction_x = (int16_t)x;
      dw2_route_obstruction_y = (int16_t)y;
      dw2_route_obstruction_kind = dw2_snapshot.obstruction_kind[cell];
      dw2_route_obstruction_level = dw2_snapshot.obstruction_level[cell];
      /* Navigation ends beside the live native obstacle.  Once the player
       * removes it, the ordinary snapshot update resumes the route. */
      dw2_route_length = index;
      break;
   }
   return dw2_route_length || dw2_route_waiting_obstruction;
}

static size_t dw2_first_run(void)
{
   size_t count = 0;

   if (!dw2_route_length)
      return 0;
   while (count < dw2_route_length && dw2_route[count] == dw2_route[0])
      count++;
   return count;
}

static size_t dw2_segment_remaining(void)
{
   int step_x;
   int step_y;
   int distance_x;
   int distance_y;
   int remaining_x;
   int remaining_y;

   if (!dw2_route_length)
      return 0;
   dw2_direction_step(&dw2_snapshot, dw2_route[0], &step_x, &step_y);
   distance_x = dw2_segment_end_x - dw2_snapshot.player_x;
   distance_y = dw2_segment_end_y - dw2_snapshot.player_y;
   remaining_x = distance_x * step_x;
   remaining_y = distance_y * step_y;
   if (!step_x && distance_x == 0 && remaining_y >= 0)
      return (size_t)remaining_y;
   if (!step_y && distance_y == 0 && remaining_x >= 0)
      return (size_t)remaining_x;
   if (step_x && step_y && remaining_x == remaining_y
         && remaining_x >= 0)
      return (size_t)remaining_x;
   return dw2_first_run();
}

static void dw2_set_segment(void)
{
   size_t count = dw2_first_run();
   uint8_t direction = dw2_route[0];
   int step_x;
   int step_y;

   dw2_direction_step(&dw2_snapshot, direction, &step_x, &step_y);

   dw2_segment_start_x = dw2_snapshot.player_x;
   dw2_segment_start_y = dw2_snapshot.player_y;
   dw2_segment_end_x = (int16_t)(dw2_segment_start_x
      + step_x * (int)count);
   dw2_segment_end_y = (int16_t)(dw2_segment_start_y
      + step_y * (int)count);
}

static void dw2_speak_segment(bool include_target,
      const beetle_dw2_navigation_target_t *target)
{
   char speech[BEETLE_DW2_NAV_LABEL_MAX + 256];
   size_t count = dw2_segment_remaining();

   if (!count)
      return;
   if (include_target)
      snprintf(speech, sizeof(speech), "%s. %s %u.",
            dw2_target_label(target), dw2_direction_name[dw2_route[0]],
            (unsigned)count);
   else
      snprintf(speech, sizeof(speech), "%s %u.",
            dw2_direction_name[dw2_route[0]], (unsigned)count);
   dw2_speak(speech);
}

static void dw2_speak_obstruction(void)
{
   char speech[192];

   dw2_obstruction_description(dw2_route_obstruction_kind,
         dw2_route_obstruction_level, speech, sizeof(speech));
   dw2_speak(speech);
}

static void dw2_speak_reached(
      const beetle_dw2_navigation_target_t *target)
{
   char speech[BEETLE_DW2_NAV_LABEL_MAX + 192];
   unsigned direction;

   if (target && target->kind == BEETLE_DW2_NAV_TARGET_HAZARD
         && target->x >= 0 && target->y >= 0
         && target->x < dw2_snapshot.width && target->y < dw2_snapshot.height)
   {
      size_t cell = (size_t)target->y * BEETLE_DW2_NAV_MAX_WIDTH
         + (size_t)target->x;
      if (dw2_snapshot.obstruction_kind[cell])
      {
         dw2_obstruction_description(dw2_snapshot.obstruction_kind[cell],
               dw2_snapshot.obstruction_level[cell], speech, sizeof(speech));
         dw2_speak(speech);
         return;
      }
   }

   if (target && target->approach_adjacent
         && ((dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_CITY
               && (target->kind == BEETLE_DW2_NAV_TARGET_NPC
                  || target->kind == BEETLE_DW2_NAV_TARGET_STORY_EVENT))
            || target->kind == BEETLE_DW2_NAV_TARGET_TREASURE_BOX))
   {
      int target_x = target->x - dw2_snapshot.player_x;
      int target_y = target->y - dw2_snapshot.player_y;

      for (direction = 0; direction < 4; direction++)
      {
         int step_x;
         int step_y;

         dw2_direction_step(&dw2_snapshot, direction, &step_x, &step_y);
         if (step_x == target_x && step_y == target_y)
         {
            snprintf(speech, sizeof(speech),
                  "%s. Face %s and press X.", dw2_target_label(target),
                  dw2_direction_name[direction]);
            dw2_speak(speech);
            return;
         }
      }
      /* Domain interactions inspect all eight adjacent tiles (800682dc).
       * The goal test accepts those positions, so every one needs a facing
       * cue. 80068604/800728d4 maps two adjacent D-pad buttons to the
       * intermediate facing. Use the current rotated control vectors. */
      if (dw2_snapshot.approach_eight_way)
      {
         for (direction = 0; direction < 4; direction++)
         {
            unsigned next = (direction + 1u) & 3u;
            int first_x;
            int first_y;
            int next_x;
            int next_y;
            int step_x;
            int step_y;

            dw2_direction_step(&dw2_snapshot, direction, &first_x, &first_y);
            dw2_direction_step(&dw2_snapshot, next, &next_x, &next_y);
            step_x = first_x + next_x;
            step_y = first_y + next_y;
            step_x = (step_x > 0) - (step_x < 0);
            step_y = (step_y > 0) - (step_y < 0);
            if (step_x == target_x && step_y == target_y)
            {
               snprintf(speech, sizeof(speech),
                     "%s. Face %s and %s together, then press X.",
                     dw2_target_label(target), dw2_direction_name[direction],
                     dw2_direction_name[next]);
               dw2_speak(speech);
               return;
            }
         }
      }
   }

   snprintf(speech, sizeof(speech), "%s reached.", dw2_target_label(target));
   dw2_speak(speech);
}

static void dw2_recalculate(bool speak,
      const beetle_dw2_navigation_target_t *target)
{
   char speech[BEETLE_DW2_NAV_LABEL_MAX + 32];

   if (target && target->kind == BEETLE_DW2_NAV_TARGET_STORY_EVENT)
      dw2_tracked_story_id = target->id;
   if (target && target->information_only)
   {
      dw2_route_active = false;
      dw2_route_length = 0;
      if (speak) dw2_speak(target->label);
      return;
   }
   if (dw2_is_goal(target, dw2_snapshot.player_x, dw2_snapshot.player_y))
   {
      dw2_route_active = false;
      dw2_route_length = 0;
      if (speak)
         dw2_speak_reached(target);
      return;
   }
   if (!dw2_prepare_route(target))
   {
      dw2_route_active = false;
      snprintf(speech, sizeof(speech), "No route to %s.",
            dw2_target_label(target));
      if (speak)
         dw2_speak(speech);
      return;
   }
   dw2_route_active = true;
   if (!dw2_route_length && dw2_route_waiting_obstruction)
   {
      if (speak)
         dw2_speak_obstruction();
      return;
   }
   dw2_set_segment();
   if (speak)
      dw2_speak_segment(false, target);
}

static void dw2_repeat_route(
      const beetle_dw2_navigation_target_t *target)
{
   char speech[BEETLE_DW2_NAV_LABEL_MAX + 32];

   if (!dw2_route_active)
   {
      dw2_speak_selection();
      return;
   }
   if (dw2_is_goal(target, dw2_snapshot.player_x, dw2_snapshot.player_y))
   {
      dw2_route_active = false;
      dw2_route_length = 0;
      dw2_speak_reached(target);
      return;
   }
   if (!dw2_prepare_route(target))
   {
      dw2_route_active = false;
      snprintf(speech, sizeof(speech), "No route to %s.",
            dw2_target_label(target));
      dw2_speak(speech);
      return;
   }
   if (!dw2_route_length && dw2_route_waiting_obstruction)
   {
      dw2_speak_obstruction();
      return;
   }
   dw2_set_segment();
   dw2_speak_segment(true, target);
}

static bool dw2_on_current_segment(int x, int y)
{
   uint8_t direction;
   size_t count;
   int step_x;
   int step_y;
   int distance_x;
   int distance_y;
   int steps_x;
   int steps_y;

   if (!dw2_route_length)
      return false;
   direction = dw2_route[0];
   count = dw2_first_run();
   dw2_direction_step(&dw2_snapshot, direction, &step_x, &step_y);
   distance_x = x - dw2_segment_start_x;
   distance_y = y - dw2_segment_start_y;
   steps_x = distance_x * step_x;
   steps_y = distance_y * step_y;
   if (!step_x)
      return distance_x == 0 && steps_y >= 0
         && steps_y <= (int)count;
   if (!step_y)
      return distance_y == 0 && steps_x >= 0
         && steps_x <= (int)count;
   return steps_x == steps_y && steps_x >= 0
      && steps_x <= (int)count;
}

static void dw2_speak_selection(void)
{
   const beetle_dw2_navigation_target_t *target;
   char speech[BEETLE_DW2_NAV_LABEL_MAX + 64];
   size_t ordinal = 0;
   size_t count = dw2_targets_in_category();

   target = dw2_selected_target(&ordinal);
   if (!target)
      snprintf(speech, sizeof(speech), "%s. Empty.", dw2_category_label());
   else
      snprintf(speech, sizeof(speech), "%s. %s. %u of %u.",
            dw2_category_label(), dw2_target_label(target),
            (unsigned)(ordinal + 1), (unsigned)count);
   dw2_speak(speech);
}

static bool dw2_continue_story_tracking(void)
{
   const beetle_dw2_navigation_target_t *target = NULL;
   size_t index;

   if (!dw2_story_tracking || !dw2_snapshot_valid)
      return false;
   for (index = 0; index < dw2_snapshot.target_count; index++)
   {
      if (dw2_snapshot.targets[index].kind
            == BEETLE_DW2_NAV_TARGET_STORY_EVENT)
      {
         if (!target) target = &dw2_snapshot.targets[index];
         if (dw2_snapshot.targets[index].id == dw2_tracked_story_id)
         { target = &dw2_snapshot.targets[index]; break; }
      }
   }
   if (!target)
      return false;
   dw2_category = dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_CITY
      ? 2u : 0u;
   dw2_selected_valid = true;
   dw2_selected_id = target->id;
   dw2_tracked_story_id = target->id;
   dw2_selected_generation = target->generation;
   dw2_selected_kind = target->kind;
   dw2_resume_pending = false;
   dw2_pending_repeat = false;
   dw2_route_length = 0;
   if (dw2_snapshot.player_settled)
   {
      dw2_pending_start = false;
      dw2_recalculate(true, target);
   }
   else
   {
      dw2_route_active = false;
      dw2_pending_start = true;
   }
   return true;
}

void beetle_accessibility_dw2_navigation_reset(void)
{
   beetle_accessibility_dw2_story_reset();
   memset(&dw2_snapshot, 0, sizeof(dw2_snapshot));
   dw2_snapshot_valid = false;
   dw2_suppressed = false;
   dw2_category = 0;
   dw2_selected_valid = false;
   dw2_selected_id = 0;
   dw2_selected_generation = 0;
   dw2_selected_kind = BEETLE_DW2_NAV_TARGET_STORY_EVENT;
   dw2_route_active = false;
   dw2_story_tracking = false;
   dw2_tracked_story_id = 0;
   dw2_resume_pending = false;
   dw2_pending_start = false;
   dw2_pending_repeat = false;
   dw2_route_length = 0;
   dw2_route_waiting_obstruction = false;
   dw2_route_obstruction_x = -1;
   dw2_route_obstruction_y = -1;
   dw2_route_obstruction_kind = BEETLE_DW2_NAV_OBSTRUCTION_NONE;
   dw2_route_obstruction_level = 0;
   dw2_current_city_scene = 0;
   memset(dw2_npc_name_cache, 0, sizeof(dw2_npc_name_cache));
   dw2_npc_name_cache_next = 0;
   dw2_last_location[0] = '\0';
   dw2_pending_location[0] = '\0';
   dw2_segment_start_x = 0;
   dw2_segment_start_y = 0;
   dw2_segment_end_x = 0;
   dw2_segment_end_y = 0;
}

void beetle_accessibility_dw2_navigation_frame(const uint8_t *main_ram,
      size_t ram_size, uint32_t overlay_tag, bool suppressed)
{
   beetle_dw2_navigation_snapshot_t snapshot;
   bool valid = false;

   /* STAG3000 owns the entire battle, including animations and transitions
    * where its command-menu label is absent.  Preserve a live route across
    * every such frame instead of treating the temporary invalid snapshot as
    * a context loss. */
   if (overlay_tag == DW2_NAV_STAG3000_TAG)
      suppressed = true;

   if (overlay_tag == DW2_NAV_STAG4000_TAG)
      valid = dw2_native_domain_snapshot(main_ram, ram_size, &snapshot);
   else if (overlay_tag == DW2_NAV_STAG2000_TAG)
   {
      valid = dw2_native_city_snapshot(main_ram, ram_size, &snapshot);
      /* 32a..32e are the three world maps and two city-selection screens.
       * They have native menu focus but no walkable player grid. Story can
       * still describe the next selection without manufacturing a route. */
      if (!valid && main_ram && ram_size > DW2_NAV_CITY_SCENE_OFFSET
            && main_ram[DW2_NAV_CITY_SCENE_OFFSET]>=42
            && main_ram[DW2_NAV_CITY_SCENE_OFFSET]<=46)
      {
         memset(&snapshot,0,sizeof(snapshot));
         snapshot.context=BEETLE_DW2_NAV_CONTEXT_CITY;
         snapshot.width=1; snapshot.height=1;
         snapshot.player_settled=true;
         snapshot.player_position_unavailable=true;
         snprintf(snapshot.location, sizeof(snapshot.location), "%s",
               beetle_accessibility_dw2_story_scene_name(
                  main_ram[DW2_NAV_CITY_SCENE_OFFSET]));
         valid=true;
      }
   }
   if (valid) dw2_native_story_targets(main_ram,ram_size,&snapshot);

   beetle_accessibility_dw2_navigation_update_snapshot(
         valid ? &snapshot : NULL, suppressed);
}

static void dw2_update_snapshot(
      const beetle_dw2_navigation_snapshot_t *snapshot, bool suppressed)
{
   beetle_dw2_navigation_context_t old_context = dw2_snapshot.context;
   int16_t old_player_x = dw2_snapshot.player_x;
   int16_t old_player_y = dw2_snapshot.player_y;
   bool old_player_settled = dw2_snapshot.player_settled;
   bool was_suppressed = dw2_suppressed;
   bool direction_map_changed = snapshot
      && old_context == snapshot->context
      && (!dw2_direction_maps_equal(&dw2_snapshot, snapshot)
         || dw2_snapshot.approach_eight_way
            != snapshot->approach_eight_way);
   const beetle_dw2_navigation_target_t *target;
   unsigned category_count;

   if (suppressed)
   {
      dw2_suppressed = true;
      if (dw2_snapshot_valid && dw2_route_active)
         dw2_resume_pending = true;
      return;
   }
   dw2_suppressed = false;
   if (!dw2_snapshot_is_valid(snapshot))
   {
      dw2_snapshot_valid = false;
      dw2_selected_valid = false;
      dw2_route_active = false;
      dw2_resume_pending = false;
      dw2_pending_start = false;
      dw2_pending_repeat = false;
      dw2_route_length = 0;
      return;
   }
   dw2_snapshot = *snapshot;
   dw2_snapshot_valid = true;
   if (was_suppressed && dw2_route_active)
      dw2_resume_pending = true;
   category_count = dw2_category_count(dw2_snapshot.context);
   if (old_context != dw2_snapshot.context)
   {
      dw2_category = 0;
      dw2_selected_valid = false;
      dw2_route_active = false;
      dw2_resume_pending = false;
      dw2_pending_start = false;
      dw2_pending_repeat = false;
      dw2_route_length = 0;
      if (dw2_continue_story_tracking())
         return;
      return;
   }
   if (dw2_category >= category_count)
      dw2_category = 0;
   if (!dw2_selected_valid && dw2_continue_story_tracking())
      return;
   if (!dw2_snapshot.player_settled)
      return;

   target = NULL;
   if (dw2_selected_valid)
   {
      target = dw2_selected_target(NULL);
      if (!target)
      {
         bool continue_story = dw2_story_tracking
            && dw2_selected_kind == BEETLE_DW2_NAV_TARGET_STORY_EVENT;

         dw2_route_active = false;
         dw2_route_length = 0;
         dw2_selected_valid = false;
         dw2_resume_pending = false;
         dw2_pending_start = false;
         dw2_pending_repeat = false;
         if (continue_story && dw2_continue_story_tracking())
            return;
         dw2_speak("Target no longer available.");
         return;
      }
   }

   if (dw2_pending_start || dw2_pending_repeat)
   {
      bool start_pending = dw2_pending_start;
      bool repeat_pending = dw2_pending_repeat;

      dw2_pending_start = false;
      dw2_pending_repeat = false;
      dw2_resume_pending = false;
      if (!target)
         target = dw2_selected_target(NULL);
      if (!target)
      {
         dw2_speak_selection();
         return;
      }
      if (start_pending)
      {
         dw2_story_tracking = target->kind
            == BEETLE_DW2_NAV_TARGET_STORY_EVENT;
         dw2_recalculate(true, target);
      }
      else if (repeat_pending)
         dw2_repeat_route(target);
      return;
   }

   if (!dw2_route_active)
   {
      dw2_resume_pending = false;
      return;
   }
   if (!target)
      target = dw2_selected_target(NULL);
   if (!target)
   {
      dw2_route_active = false;
      dw2_route_length = 0;
      dw2_resume_pending = false;
      return;
   }
   if (dw2_resume_pending)
   {
      dw2_resume_pending = false;
      dw2_recalculate(true, target);
      return;
   }
   if (direction_map_changed)
   {
      dw2_recalculate(true, target);
      return;
   }
   if (dw2_route_waiting_obstruction && !dw2_route_length)
   {
      size_t obstruction_cell;

      if (dw2_route_obstruction_x >= 0 && dw2_route_obstruction_y >= 0
            && dw2_route_obstruction_x < (int16_t)dw2_snapshot.width
            && dw2_route_obstruction_y < (int16_t)dw2_snapshot.height)
      {
         obstruction_cell = (size_t)dw2_route_obstruction_y
            * BEETLE_DW2_NAV_MAX_WIDTH
            + (size_t)dw2_route_obstruction_x;
         if (dw2_snapshot.obstruction_kind[obstruction_cell]
               == dw2_route_obstruction_kind)
            return;
      }
      dw2_recalculate(true, target);
      return;
   }
   if (dw2_is_goal(target, dw2_snapshot.player_x, dw2_snapshot.player_y))
   {
      dw2_route_active = false;
      dw2_route_length = 0;
      dw2_speak_reached(target);
      return;
   }
   if (!dw2_current_segment_traversable(target))
   {
      dw2_recalculate(true, target);
      return;
   }
   if (old_player_x == dw2_snapshot.player_x
         && old_player_y == dw2_snapshot.player_y
         && old_player_settled)
      return;
   if (dw2_snapshot.player_x == dw2_segment_end_x
         && dw2_snapshot.player_y == dw2_segment_end_y)
   {
      dw2_recalculate(true, target);
      return;
   }
   if (!dw2_on_current_segment(dw2_snapshot.player_x,
            dw2_snapshot.player_y))
      dw2_recalculate(true, target);
}

void beetle_accessibility_dw2_navigation_update_snapshot(
      const beetle_dw2_navigation_snapshot_t *snapshot, bool suppressed)
{
   bool ready = !suppressed && dw2_snapshot_is_valid(snapshot)
      && snapshot->player_settled;

   dw2_pending_location[0] = '\0';
   if (ready && snapshot->context == BEETLE_DW2_NAV_CONTEXT_CITY)
      dw2_last_location[0] = '\0';
   if (ready && snapshot->context == BEETLE_DW2_NAV_CONTEXT_DOMAIN
         && snapshot->location[0]
         && memchr(snapshot->location, '\0', sizeof(snapshot->location))
         && strcmp(snapshot->location, dw2_last_location))
      snprintf(dw2_pending_location, sizeof(dw2_pending_location), "%s",
            snapshot->location);

   /* A floor change and resumed route share one utterance, so the frontend
    * cannot cancel the floor announcement with a direction in this frame. */
   dw2_update_snapshot(snapshot, suppressed);
   if (dw2_pending_location[0])
      dw2_speak(NULL);
   dw2_pending_location[0] = '\0';
}

static bool dw2_native_speaker_name_valid(const char *speaker)
{
   size_t length;
   size_t index;

   if (!speaker || speaker[0] < 'A' || speaker[0] > 'Z')
      return false;
   length = strlen(speaker);
   if (!length || length > 40)
      return false;
   for (index = 0; index < length; index++)
   {
      unsigned char value = (unsigned char)speaker[index];

      if ((value >= 'A' && value <= 'Z')
            || (value >= 'a' && value <= 'z')
            || (value >= '0' && value <= '9')
            || value == ' ' || value == '\'' || value == '-')
         continue;
      return false;
   }
   return true;
}

void beetle_accessibility_dw2_navigation_observe_speaker(
      const char *speaker)
{
   beetle_dw2_navigation_target_t *candidate = NULL;
   int facing_x = 0;
   int facing_y = 0;
   size_t index;
   unsigned adjacent_count = 0;
   bool facing_match = false;

   if (!dw2_native_speaker_name_valid(speaker) || !dw2_snapshot_valid
         || dw2_snapshot.context != BEETLE_DW2_NAV_CONTEXT_CITY)
      return;
   if (dw2_snapshot.preferred_direction_valid)
   {
      int step_x;
      int step_y;

      dw2_direction_step(&dw2_snapshot,
            dw2_snapshot.preferred_direction, &step_x, &step_y);
      facing_x = dw2_snapshot.player_x + step_x;
      facing_y = dw2_snapshot.player_y + step_y;
   }
   for (index = 0; index < dw2_snapshot.target_count; index++)
   {
      beetle_dw2_navigation_target_t *target =
         &dw2_snapshot.targets[index];
      int distance_x;
      int distance_y;

      if (target->kind != BEETLE_DW2_NAV_TARGET_NPC)
         continue;
      if (dw2_snapshot.preferred_direction_valid
            && target->x == facing_x && target->y == facing_y)
      {
         candidate = target;
         facing_match = true;
         break;
      }
      distance_x = target->x - dw2_snapshot.player_x;
      if (distance_x < 0)
         distance_x = -distance_x;
      distance_y = target->y - dw2_snapshot.player_y;
      if (distance_y < 0)
         distance_y = -distance_y;
      if (distance_x + distance_y == 1)
      {
         candidate = target;
         adjacent_count++;
      }
   }
   if (!candidate || (!facing_match && adjacent_count != 1))
      return;
   for (index = 0; index < DW2_NAV_NPC_NAME_CACHE_LIMIT; index++)
   {
      dw2_npc_name_cache_entry_t *entry = &dw2_npc_name_cache[index];

      if (!entry->valid || entry->scene != dw2_current_city_scene)
         continue;
      if ((entry->id == candidate->id
                && entry->generation == candidate->generation)
            || (entry->x == candidate->x && entry->y == candidate->y))
      {
         /* The first native speaker associated with an actor is its label.
          * Later player-name rows in the same conversation must not replace
          * that actor's identity. */
         return;
      }
   }
   snprintf(candidate->label, sizeof(candidate->label), "%s", speaker);
   {
      dw2_npc_name_cache_entry_t *entry =
         &dw2_npc_name_cache[dw2_npc_name_cache_next];

      memset(entry, 0, sizeof(*entry));
      entry->valid = true;
      entry->scene = dw2_current_city_scene;
      entry->id = candidate->id;
      entry->generation = candidate->generation;
      entry->x = candidate->x;
      entry->y = candidate->y;
      snprintf(entry->name, sizeof(entry->name), "%s", speaker);
      dw2_npc_name_cache_next = (dw2_npc_name_cache_next + 1)
         % DW2_NAV_NPC_NAME_CACHE_LIMIT;
   }
}

void beetle_accessibility_dw2_navigation_command(
      beetle_dw2_navigation_command_t command)
{
   const beetle_dw2_navigation_target_t *target;
   unsigned category_count;
   size_t target_count;
   size_t ordinal = 0;
   size_t wanted;

   /* Stop also works while moving, in a menu/battle, or between maps. In
    * particular, story tracking must not silently restart a cancelled route. */
   if (command == BEETLE_DW2_NAV_COMMAND_START_ROUTE
         && (dw2_route_active || dw2_pending_start || dw2_resume_pending
            || dw2_story_tracking))
   {
      dw2_route_active = false;
      dw2_pending_start = false;
      dw2_pending_repeat = false;
      dw2_resume_pending = false;
      dw2_story_tracking = false;
      dw2_tracked_story_id = 0;
      dw2_route_length = 0;
      dw2_route_waiting_obstruction = false;
      beetle_accessibility_speak("Guidance stopped.", 8, "navigation");
      return;
   }

   if (dw2_suppressed)
   {
      beetle_accessibility_speak(
            "Navigation paused while a menu is open.", 8, "navigation");
      return;
   }
   if (!dw2_snapshot_valid)
   {
      dw2_speak("Navigation unavailable here.");
      return;
   }
   if (command == BEETLE_DW2_NAV_COMMAND_LOCATION)
   {
      char speech[BEETLE_DW2_NAV_LABEL_MAX + 80];
      const char *location = dw2_snapshot.context == BEETLE_DW2_NAV_CONTEXT_DOMAIN
         ? "Domain" : "City";

      if (dw2_snapshot.location[0]
            && memchr(dw2_snapshot.location, '\0', sizeof(dw2_snapshot.location)))
         location = dw2_snapshot.location;
      if (dw2_snapshot.player_position_unavailable)
         snprintf(speech, sizeof(speech), "%s. Coordinates unavailable here.", location);
      else if (!dw2_snapshot.player_settled)
         snprintf(speech, sizeof(speech), "%s. Moving. Stop to read coordinates.", location);
      else
         snprintf(speech, sizeof(speech), "%s. X %d, Y %d.", location,
               (int)dw2_snapshot.player_x, (int)dw2_snapshot.player_y);
      dw2_speak(speech);
      return;
   }
   category_count = dw2_category_count(dw2_snapshot.context);
   if (!category_count)
      return;

   if (!dw2_snapshot.player_settled
         && (command == BEETLE_DW2_NAV_COMMAND_REPEAT
            || command == BEETLE_DW2_NAV_COMMAND_START_ROUTE))
   {
      const beetle_dw2_navigation_target_t *pending_target =
         dw2_selected_target(NULL);

      dw2_pending_start =
         command == BEETLE_DW2_NAV_COMMAND_START_ROUTE;
      dw2_pending_repeat =
         command == BEETLE_DW2_NAV_COMMAND_REPEAT;
      if (dw2_pending_start)
         dw2_story_tracking = pending_target
            && pending_target->kind == BEETLE_DW2_NAV_TARGET_STORY_EVENT;
      return;
   }

   if (command == BEETLE_DW2_NAV_COMMAND_PREVIOUS_CATEGORY
         || command == BEETLE_DW2_NAV_COMMAND_NEXT_CATEGORY)
   {
      if (command == BEETLE_DW2_NAV_COMMAND_PREVIOUS_CATEGORY)
         dw2_category = dw2_category == 0 ? category_count - 1
            : dw2_category - 1;
      else
         dw2_category = (dw2_category + 1) % category_count;
      dw2_selected_valid = false;
      dw2_route_active = false;
      dw2_story_tracking = false;
      dw2_resume_pending = false;
      dw2_pending_start = false;
      dw2_pending_repeat = false;
      dw2_route_length = 0;
      dw2_speak_selection();
      return;
   }

   target = dw2_selected_target(&ordinal);
   target_count = dw2_targets_in_category();
   if (command == BEETLE_DW2_NAV_COMMAND_PREVIOUS_TARGET
         || command == BEETLE_DW2_NAV_COMMAND_NEXT_TARGET)
   {
      if (target_count)
      {
         if (command == BEETLE_DW2_NAV_COMMAND_PREVIOUS_TARGET)
            wanted = ordinal == 0 ? target_count - 1 : ordinal - 1;
         else
            wanted = (ordinal + 1) % target_count;
         target = dw2_target_at(wanted);
         if (target)
         {
            dw2_selected_valid = true;
            dw2_selected_id = target->id;
            dw2_selected_generation = target->generation;
            dw2_selected_kind = target->kind;
         }
      }
      dw2_route_active = false;
      dw2_story_tracking = false;
      dw2_resume_pending = false;
      dw2_pending_start = false;
      dw2_pending_repeat = false;
      dw2_route_length = 0;
      dw2_speak_selection();
      return;
   }

   if (!target)
   {
      char speech[BEETLE_DW2_NAV_LABEL_MAX + 16];
      snprintf(speech, sizeof(speech), "%s. Empty.", dw2_category_label());
      dw2_speak(speech);
      return;
   }
   if (command == BEETLE_DW2_NAV_COMMAND_REPEAT)
   {
      dw2_repeat_route(target);
      return;
   }
   if (command == BEETLE_DW2_NAV_COMMAND_START_ROUTE)
   {
      dw2_story_tracking = target->kind
         == BEETLE_DW2_NAV_TARGET_STORY_EVENT;
      dw2_recalculate(true, target);
   }
}
