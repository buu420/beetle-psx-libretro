#ifndef BEETLE_PSX_ACCESSIBILITY_DW2_NPC_H
#define BEETLE_PSX_ACCESSIBILITY_DW2_NPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BEETLE_DW2_NPC_NAME_MAX 128u

/* Shown when a visible actor exists but the game has not stated who it is.
 * This is an accessibility-authored placeholder, not native text. */
#define BEETLE_DW2_NPC_UNKNOWN_NAME "Person"

/* 8006e60c writes at most five entries between domain +0x144 and the count
 * at +0x16c. */
#define BEETLE_DW2_NPC_EVENT_LIMIT 5u

/* Where a name came from, weakest last. Callers that want to treat a
 * borrowed name differently from the page the game is showing can branch on
 * this; `beetle_accessibility_dw2_npc_name` hides it. */
typedef enum beetle_dw2_npc_source
{
   BEETLE_DW2_NPC_SOURCE_NONE = 0,
   /* The speaker row of the page the game would show right now. */
   BEETLE_DW2_NPC_SOURCE_SLOT,
   /* Another page of this same record; every labelled page agrees. */
   BEETLE_DW2_NPC_SOURCE_RECORD,
   /* Another record of the same model in this same scene's actor table. */
   BEETLE_DW2_NPC_SOURCE_SCENE_MODEL,
   /* 8001e758's model name registry. */
   BEETLE_DW2_NPC_SOURCE_MODEL_REGISTRY,
   /* The compiled cross-scene model catalogue. */
   BEETLE_DW2_NPC_SOURCE_CATALOG,
   /* A valid visible actor whose identity the game has not shown yet. */
   BEETLE_DW2_NPC_SOURCE_UNKNOWN
} beetle_dw2_npc_source_t;

typedef struct beetle_dw2_npc_event
{
   uint16_t x;
   uint16_t y;
   uint32_t record_index;
   bool named;
   beetle_dw2_npc_source_t source;
   char name[BEETLE_DW2_NPC_NAME_MAX];
} beetle_dw2_npc_event_t;

/* Native identity for a city actor task (type 0x302).
 *
 * `task_offset` and `data_offset` are the RAM offsets the navigation snapshot
 * already holds: the task allocation and `[task + 0x2c]`. The name is the
 * speaker row of the dialogue page the game itself would show right now, so
 * it tracks story stage and never reveals a later page's identity.
 *
 * Returns false and writes an empty string when the task is not a resolvable
 * city actor. A live actor whose identity is genuinely not shown yet returns
 * true with BEETLE_DW2_NPC_UNKNOWN_NAME. Never writes to `ram`. */
bool beetle_accessibility_dw2_npc_name(const uint8_t *ram, size_t ram_size,
      size_t task_offset, size_t data_offset, char *out, size_t out_size);

bool beetle_accessibility_dw2_npc_name_ex(const uint8_t *ram, size_t ram_size,
      size_t task_offset, size_t data_offset, char *out, size_t out_size,
      beetle_dw2_npc_source_t *source);

/* Active domain dialogue triggers: the list 8006e60c builds at
 * `domain + 0x144` and 8006e6cc consumes when the player steps on the tile.
 *
 * `domain_offset` is the RAM offset of the 400-byte domain state block; pass
 * 0 to resolve it from `[0x00072b60]`. Entries already consumed (x or y set
 * to 0xffff) are skipped. Returns the number written, never more than
 * `out_max` or BEETLE_DW2_NPC_EVENT_LIMIT. These are invisible script
 * triggers (all original DUNG records use model499), NOT visible NPCs.
 * Story routing requires a separately verified mission-record binding;
 * an arbitrary trigger or speaker name is not enough. Never writes to `ram`. */
size_t beetle_accessibility_dw2_npc_domain_events(const uint8_t *ram,
      size_t ram_size, size_t domain_offset, beetle_dw2_npc_event_t *out,
      size_t out_max);

const char *beetle_accessibility_dw2_npc_source_label(
      beetle_dw2_npc_source_t source);

#endif
