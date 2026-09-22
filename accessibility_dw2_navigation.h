#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_NAVIGATION_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_NAVIGATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BEETLE_DW2_NAV_MAX_WIDTH 64
#define BEETLE_DW2_NAV_MAX_HEIGHT 64
#define BEETLE_DW2_NAV_MAX_CELLS \
   (BEETLE_DW2_NAV_MAX_WIDTH * BEETLE_DW2_NAV_MAX_HEIGHT)
#define BEETLE_DW2_NAV_MAX_TARGETS 64
#define BEETLE_DW2_NAV_LABEL_MAX 256

typedef enum beetle_dw2_navigation_context
{
   BEETLE_DW2_NAV_CONTEXT_NONE = 0,
   BEETLE_DW2_NAV_CONTEXT_DOMAIN,
   BEETLE_DW2_NAV_CONTEXT_CITY
} beetle_dw2_navigation_context_t;

typedef enum beetle_dw2_navigation_target_kind
{
   BEETLE_DW2_NAV_TARGET_STORY_EVENT = 0,
   BEETLE_DW2_NAV_TARGET_ENEMY_DIGIMON,
   BEETLE_DW2_NAV_TARGET_FLOOR_PORTAL,
   BEETLE_DW2_NAV_TARGET_EXIT_PORTAL,
   BEETLE_DW2_NAV_TARGET_EXIT,
   BEETLE_DW2_NAV_TARGET_NPC,
   BEETLE_DW2_NAV_TARGET_HAZARD,
   BEETLE_DW2_NAV_TARGET_TREASURE_BOX
} beetle_dw2_navigation_target_kind_t;

typedef enum beetle_dw2_navigation_obstruction_kind
{
   BEETLE_DW2_NAV_OBSTRUCTION_NONE = 0,
   BEETLE_DW2_NAV_OBSTRUCTION_ELECTRO_SPORE = 6,
   BEETLE_DW2_NAV_OBSTRUCTION_BIG_ROCK = 7,
   BEETLE_DW2_NAV_OBSTRUCTION_LAND_MINE = 8,
   BEETLE_DW2_NAV_OBSTRUCTION_BIT_BUG_NEST = 9,
   BEETLE_DW2_NAV_OBSTRUCTION_ENERGY_BUG_NEST = 10,
   BEETLE_DW2_NAV_OBSTRUCTION_RETURN_BUG_NEST = 11,
   BEETLE_DW2_NAV_OBSTRUCTION_MEMORY_BUG_NEST = 12
} beetle_dw2_navigation_obstruction_kind_t;

typedef enum beetle_dw2_navigation_command
{
   BEETLE_DW2_NAV_COMMAND_PREVIOUS_CATEGORY = 0,
   BEETLE_DW2_NAV_COMMAND_NEXT_CATEGORY,
   BEETLE_DW2_NAV_COMMAND_PREVIOUS_TARGET,
   BEETLE_DW2_NAV_COMMAND_NEXT_TARGET,
   BEETLE_DW2_NAV_COMMAND_REPEAT,
   BEETLE_DW2_NAV_COMMAND_START_ROUTE,
   BEETLE_DW2_NAV_COMMAND_LOCATION
} beetle_dw2_navigation_command_t;

typedef struct beetle_dw2_navigation_target
{
   beetle_dw2_navigation_target_kind_t kind;
   uint32_t id;
   uint32_t generation;
   int16_t x;
   int16_t y;
   bool approach_adjacent;
   bool information_only;
   bool portal;
   /* Connected native transition tiles belonging to one city doorway. */
   uint8_t entrance_tile_count;
   uint8_t entrance_tiles[BEETLE_DW2_NAV_MAX_TARGETS][2];
   char label[BEETLE_DW2_NAV_LABEL_MAX];
} beetle_dw2_navigation_target_t;

typedef struct beetle_dw2_navigation_snapshot
{
   beetle_dw2_navigation_context_t context;
   uint16_t width;
   uint16_t height;
   int16_t player_x;
   int16_t player_y;
   bool player_settled;
   bool player_position_unavailable;
   bool approach_eight_way;
   bool preferred_direction_valid;
   uint8_t preferred_direction;
   int8_t direction_x[4];
   int8_t direction_y[4];
   char location[BEETLE_DW2_NAV_LABEL_MAX];
   uint8_t blocked[BEETLE_DW2_NAV_MAX_CELLS];
   uint8_t obstruction_kind[BEETLE_DW2_NAV_MAX_CELLS];
   uint8_t obstruction_level[BEETLE_DW2_NAV_MAX_CELLS];
   beetle_dw2_navigation_target_t targets[BEETLE_DW2_NAV_MAX_TARGETS];
   size_t target_count;
} beetle_dw2_navigation_snapshot_t;

void beetle_accessibility_dw2_navigation_reset(void);
void beetle_accessibility_dw2_navigation_frame(const uint8_t *main_ram,
      size_t ram_size, uint32_t overlay_tag, bool suppressed);
void beetle_accessibility_dw2_navigation_update_snapshot(
      const beetle_dw2_navigation_snapshot_t *snapshot, bool suppressed);
void beetle_accessibility_dw2_navigation_observe_speaker(
      const char *speaker);
void beetle_accessibility_dw2_navigation_command(
      beetle_dw2_navigation_command_t command);

#endif
