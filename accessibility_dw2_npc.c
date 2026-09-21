#include <string.h>

#include "accessibility_dw2_npc.h"
#include "accessibility_dw2_text.h"

#define DW2_NPC_RAM_MASK 0x001fffffu

/* 8001e28c stores the current scene's actor resource group here, and every
 * native accessor (8001e390, 8001e2e0, 8001e4cc, 8001e514) reads it back.
 * It is NOT a constant: 80066714 hands the city init
 * u32[city record + 0x20], which differs for every scene. */
#define DW2_NPC_ACTOR_GROUP_OFFSET 0x0005d560u

/* 80066714: res(0x309, scene - 1) is the current scene's city record. */
#define DW2_NPC_CITY_GROUP 0x0309u
#define DW2_NPC_SCENE_OFFSET 0x0005f788u
#define DW2_NPC_SCENE_LIMIT 0x29u
#define DW2_NPC_CITY_ACTOR_GROUP_OFFSET 0x20u

#define DW2_NPC_RESOURCE_SLOTS_OFFSET 0x0005f8c8u
#define DW2_NPC_RESOURCE_SLOT_STRIDE 0x10u
#define DW2_NPC_RESOURCE_SLOT_LIMIT 0x50u

#define DW2_NPC_TASK_MODEL_OFFSET 0x0cu
#define DW2_NPC_DATA_RECORD_INDEX_OFFSET 0x1cu

/* 8001e4cc: res(group, 0) + index * 0x2c. 8001e390 stops at the first
 * zero model, which is also the table terminator. */
#define DW2_NPC_RECORD_SIZE 0x2cu
#define DW2_NPC_RECORD_LIMIT 256u
#define DW2_NPC_RECORD_MODEL_OFFSET 0x00u
#define DW2_NPC_RECORD_SLOT_CONDITION_OFFSET 0x06u
#define DW2_NPC_RECORD_MESSAGE_OFFSET 0x14u
#define DW2_NPC_MESSAGE_SLOTS 6u

/* 8001e2e0: res(group, 2) + u8[record + 6 + slot] * 0x18. */
#define DW2_NPC_CONDITION_SIZE 0x18u
#define DW2_NPC_CONDITION_PAIRS 6u

/* 80021e78 flag banks. */
#define DW2_NPC_FLAG_BANK_A 0x0005f624u
#define DW2_NPC_FLAG_BANK_B 0x0005f644u
#define DW2_NPC_FLAG_BANK_C 0x0005f64cu
#define DW2_NPC_FLAG_BANK_D 0x0005f654u
#define DW2_NPC_PROGRESS_OFFSET 0x0005f664u
#define DW2_NPC_PARTY_SCAN_OFFSET 0x0005e686u
#define DW2_NPC_PARTY_SCAN_COUNT 48u
#define DW2_NPC_ITEM_TABLE_OFFSET 0x0005f3f4u
#define DW2_NPC_ROSTER_OFFSET 0x0005e620u
#define DW2_NPC_ROSTER_COUNT 36u
#define DW2_NPC_ROSTER_STRIDE 0x5cu
#define DW2_NPC_FLAG_STATE_LIMIT 0x60000u

/* 8006aa4c: models 497-499 spawn with data +0x64 == 0 and never become a
 * walk-up NPC, and 499 additionally takes role -2 (the scripted cutscene
 * actor, which carries a different identity on nearly every record). Model
 * 500 is the player. None may borrow a name from a model. */
#define DW2_NPC_SCRIPT_MODEL_FIRST 497u
#define DW2_NPC_SCRIPT_MODEL_LAST 499u
#define DW2_NPC_PLAYER_MODEL 500u

/* 8001e67c picks the model-name bank; 8001e6a8 scans 0x28-byte records and
 * 8001e758 returns base + u32[record]. */
#define DW2_NPC_NAME_GROUP_LOW 0x0cb9u
#define DW2_NPC_NAME_GROUP_HIGH 0x0cbau
#define DW2_NPC_NAME_GROUP_MID 0x0cbbu
#define DW2_NPC_NAME_RECORD_SIZE 0x28u
#define DW2_NPC_NAME_RECORD_LIMIT 1024u

/* 80021e78's id >= 4000 arm: FUN_80013378() is u32[gp + 4] with gp fixed at
 * 0x800506f8 by the boot code at 80010dfc, and only mode 2 reaches the
 * STAG2000 predicate 80066b48. */
#define DW2_NPC_MODE_OFFSET 0x000506fcu
#define DW2_NPC_MODE_CITY 2u
#define DW2_NPC_OVERLAY_ID_FIRST 9000
#define DW2_NPC_OVERLAY_ID_LAST 9035

/* Save block: u32[0x00050720] == 0x8005e620, the base every other native
 * reader already hard-codes. */
#define DW2_NPC_SAVE_POINTER_OFFSET 0x00050720u
#define DW2_NPC_SAVE_OFFSET 0x0005e620u
#define DW2_NPC_SAVE_CARGO_OFFSET 0x34u
#define DW2_NPC_SAVE_INVENTORY_OFFSET 0x66u
#define DW2_NPC_INVENTORY_LIMIT 48u
#define DW2_NPC_ROSTER_STATE_OFFSET 0xe4u
#define DW2_NPC_ROSTER_SPECIES_OFFSET 0xe5u
#define DW2_NPC_ROSTER_EXTRA_OFFSET 0xfau
#define DW2_NPC_BITS_OFFSET 0x0005e628u
#define DW2_NPC_RANK_OFFSET 0x0005e632u
#define DW2_NPC_TOURNAMENT_OFFSET 0x0005e64eu
#define DW2_NPC_LOCATION_OFFSET 0x0005f790u
#define DW2_NPC_ENTRY_INDEX_OFFSET 0x0005f794u

/* 8006e60c / 8006e6cc: the domain dialogue-event list. */
#define DW2_NPC_DOMAIN_STATE_POINTER_OFFSET 0x00072b60u
#define DW2_NPC_DOMAIN_STATE_SIZE 400u
#define DW2_NPC_DOMAIN_EVENT_OFFSET 0x144u
#define DW2_NPC_DOMAIN_EVENT_STRIDE 8u
#define DW2_NPC_DOMAIN_EVENT_COUNT_OFFSET 0x16cu
#define DW2_NPC_DOMAIN_EVENT_CONSUMED 0xffffu

typedef struct dw2_npc_catalog_entry
{
   uint16_t model;
   const char *name;
} dw2_npc_catalog_entry_t;

/* Cross-scene model catalogue, derived by build_npc_catalog.py from the
 * actor records of every city scene. Each name is the native speaker row
 * of records carrying that model id; a model with more than one label
 * anywhere in the game (411, 413, 450) is deliberately absent, as are
 * the hidden script models 497-499 and the player model 500. The trailing
 * comment lists the scenes the label was read from. */
static const dw2_npc_catalog_entry_t dw2_npc_catalog[] = {
   { 3   , "Agumon" },                        /* scenes 03,05,23 */
   { 31  , "Patamon" },                       /* scenes 07,09,1a,22 */
   { 34  , "Ogremon" },                       /* scenes 19 */
   { 36  , "Centarumon" },                    /* scenes 19,23 */
   { 38  , "Drimogemon" },                    /* scenes 1a */
   { 45  , "Biyomon" },                       /* scenes 1a,24 */
   { 48  , "Leomon" },                        /* scenes 11 */
   { 70  , "WaruMonzaemon" },                 /* scenes 1a */
   { 76  , "Tankmon" },                       /* scenes 22 */
   { 117 , "Jijimon" },                       /* scenes 24 */
   { 143 , "Clockmon" },                      /* scenes 19 */
   { 183 , "WereGarurumon" },                 /* scenes 19,1a */
   { 217 , "DemiDevimon" },                   /* scenes 0b,0d */
   { 400 , "Gold Hawk Security Guard" },      /* scenes 03 */
   { 401 , "Blue Falcon Security Guard" },    /* scenes 07 */
   { 402 , "Black Sword Security Guard" },    /* scenes 0b */
   { 403 , "DNA Digivolve Operator" },        /* scenes 05,09,0d */
   { 404 , "Gold Hawk Tamer" },               /* scenes 01,12,1b */
   { 405 , "Gold Hawk Tamer" },               /* scenes 01,06,12,18 */
   { 406 , "Vandar" },                        /* scenes 04 */
   { 407 , "Blue Falcon Tamer" },             /* scenes 01,0a,0f,1b,1c */
   { 408 , "Blue Falcon Tamer" },             /* scenes 0a,12 */
   { 409 , "Cecilia" },                       /* scenes 08 */
   { 410 , "Digimon Center Attendant" },      /* scenes 10 */
   { 412 , "Coliseum Attendant" },            /* scenes 12 */
   { 414 , "Mission Chief Carol" },           /* scenes 01 */
   { 415 , "Chief Engineer Maestro" },        /* scenes 02 */
   { 416 , "Parts Vendor" },                  /* scenes 02 */
   { 417 , "Digi-Beetle Mechanic" },          /* scenes 02 */
   /* MESS2040/2080/2120 interrogation: F4 02 04 01 08 animates model418
    * immediately before the Commander Damien speaker row. MESS2150 also
    * explicitly moves disguised model436 out and model418 in. */
   { 418 , "Commander Damien" },
   { 419 , "Ben Oldman" },                    /* scenes 04,08,0c,1b,24,25 */
   { 420 , "Bertran" },                       /* scenes 01,06,0a,0e,0f,17 */
   { 421 , "Joy Joy" },                       /* scenes 01,06,0a,0e,0f */
   { 422 , "Lucky Luis" },                    /* scenes 03,06,07,0a,0b,0e,0f,10 */
   { 423 , "Esteena" },                       /* scenes 01,10,24 */
   { 424 , "Professor Piyotte" },             /* scenes 15,16 */
   { 425 , "Gus Getum" },                     /* scenes 14 */
   { 426 , "Brian Wiseman" },                 /* scenes 02,06,11 */
   { 427 , "Mark Shultz" },                   /* scenes 02,06,0f,11 */
   { 428 , "Esmeralda" },                     /* scenes 06,0f,11,18 */
   { 429 , "Zudokorn" },                      /* scenes 0f,11,12,13 */
   { 430 , "Debbie" },                        /* scenes 0a,0f */
   { 431 , "Doug Duem" },                     /* scenes 01,0a,0f,12,13,14 */
   { 432 , "Karen Bates" },                   /* scenes 0e,0f,18 */
   { 433 , "Chris Conner" },                  /* scenes 0e,10 */
   { 434 , "Sheena" },                        /* scenes 01,0e,0f,10,18 */
   { 435 , "Skull" },                         /* scenes 0c */
   { 436 , "Black Sword Tamer" },             /* scenes 01,0f,1b */
   { 437 , "Black Sword Tamer" },             /* scenes 01,0e */
   { 441 , "Item Vendor" },                   /* scenes 14 */
   { 442 , "Special Item Vendor" },           /* scenes 14 */
   { 443 , "Ammo Man" },                      /* scenes 02 */
   { 444 , "Kim" },                           /* scenes 14 */
   { 445 , "Techna-Donna" },                  /* scenes 16,24 */
   { 446 , "Digi-Beetle Mechanic" },          /* scenes 16 */
   { 447 , "DNA Digivolve Operator" },        /* scenes 15 */
   { 448 , "Parts Vendor" },                  /* scenes 16 */
   { 449 , "Ammo Man" },                      /* scenes 16 */
};

typedef enum dw2_npc_condition_result
{
   DW2_NPC_CONDITION_FAILED = 0,
   DW2_NPC_CONDITION_PASSED,
   /* The condition record itself is not readable. Native evaluation cannot be
    * reproduced, so the slot selection is abandoned rather than guessed. */
   DW2_NPC_CONDITION_UNKNOWN
} dw2_npc_condition_result_t;

static bool dw2_npc_range(size_t ram_size, size_t offset, size_t length)
{
   return offset <= ram_size && length <= ram_size - offset;
}

static uint32_t dw2_npc_u32(const uint8_t *ram, size_t offset)
{
   return (uint32_t)ram[offset]
      | ((uint32_t)ram[offset + 1u] << 8)
      | ((uint32_t)ram[offset + 2u] << 16)
      | ((uint32_t)ram[offset + 3u] << 24);
}

static uint16_t dw2_npc_u16(const uint8_t *ram, size_t offset)
{
   return (uint16_t)((uint32_t)ram[offset] | ((uint32_t)ram[offset + 1u] << 8));
}

static int16_t dw2_npc_s16(const uint8_t *ram, size_t offset)
{
   return (int16_t)dw2_npc_u16(ram, offset);
}

static int32_t dw2_npc_s32(const uint8_t *ram, size_t offset)
{
   return (int32_t)dw2_npc_u32(ram, offset);
}

static bool dw2_npc_address(uint32_t address, size_t ram_size, size_t length,
      size_t *offset)
{
   uint32_t region = address & 0xffe00000u;
   size_t candidate;

   if (!offset || (region != 0x80000000u && region != 0xa0000000u))
      return false;
   candidate = address & DW2_NPC_RAM_MASK;
   if (!dw2_npc_range(ram_size, candidate, length))
      return false;
   *offset = candidate;
   return true;
}

/* 80023db0: the loaded-resource slot table keyed by group id. */
static bool dw2_npc_group_base(const uint8_t *ram, size_t ram_size,
      uint32_t group, size_t *base_offset)
{
   size_t slot_index;

   if (!group || group > 0xffffu)
      return false;
   for (slot_index = 0; slot_index < DW2_NPC_RESOURCE_SLOT_LIMIT; slot_index++)
   {
      size_t slot_offset = DW2_NPC_RESOURCE_SLOTS_OFFSET
         + slot_index * DW2_NPC_RESOURCE_SLOT_STRIDE;

      if (!dw2_npc_range(ram_size, slot_offset, DW2_NPC_RESOURCE_SLOT_STRIDE))
         return false;
      if (dw2_npc_u32(ram, slot_offset + 4u) != group)
         continue;
      return dw2_npc_address(dw2_npc_u32(ram, slot_offset + 0x0cu), ram_size,
            4u, base_offset);
   }
   return false;
}

/* 800239a0(group << 16 | index) == base + u32[base + index * 4]. */
static bool dw2_npc_group_entry(const uint8_t *ram, size_t ram_size,
      size_t base_offset, size_t index, size_t length, size_t *offset)
{
   uint32_t relative;

   if (!dw2_npc_range(ram_size, base_offset + index * 4u, 4u))
      return false;
   relative = dw2_npc_u32(ram, base_offset + index * 4u);
   if (!relative || relative >= ram_size - base_offset
         || !dw2_npc_range(ram_size, base_offset + relative, length))
      return false;
   *offset = base_offset + relative;
   return true;
}

static bool dw2_npc_script_model(uint32_t model)
{
   return (model >= DW2_NPC_SCRIPT_MODEL_FIRST
         && model <= DW2_NPC_SCRIPT_MODEL_LAST)
      || model == DW2_NPC_PLAYER_MODEL;
}

/* 80066a4c: any roster slot holding species `species` with state >= 2. */
static bool dw2_npc_roster_species(const uint8_t *ram, unsigned species)
{
   unsigned index;

   for (index = 0; index < DW2_NPC_ROSTER_COUNT; index++)
   {
      size_t entry = DW2_NPC_ROSTER_OFFSET + index * DW2_NPC_ROSTER_STRIDE;

      if (ram[entry + DW2_NPC_ROSTER_STATE_OFFSET] >= 2u
            && ram[entry + DW2_NPC_ROSTER_SPECIES_OFFSET] == species)
         return true;
   }
   return false;
}

/* 8002281c: the item list is eight slots, or (cargo - 0x49) * 8 when a Cargo
 * part 0x4b..0x4f is equipped. */
static unsigned dw2_npc_inventory_size(const uint8_t *ram, size_t ram_size)
{
   size_t save_offset;
   uint32_t cargo;

   if (!dw2_npc_range(ram_size, DW2_NPC_SAVE_POINTER_OFFSET, 4u)
         || !dw2_npc_address(dw2_npc_u32(ram, DW2_NPC_SAVE_POINTER_OFFSET),
            ram_size, DW2_NPC_SAVE_CARGO_OFFSET + 2u, &save_offset))
      return 0u;
   cargo = dw2_npc_u16(ram, save_offset + DW2_NPC_SAVE_CARGO_OFFSET);
   if (cargo - 0x4bu < 5u)
      return (cargo - 0x49u) * 8u;
   return 8u;
}

/* 80066b48, case for case. Every arm is plain RAM; the switch has no other
 * cases, so any other id is natively false. */
static bool dw2_npc_overlay_predicate(const uint8_t *ram, size_t ram_size,
      int id)
{
   static const int32_t bits_steps[8] = {
      500, 1000, 1500, 2000, 2500, 3000, 3500, 4000
   };
   unsigned index;

   /* 80021e78 only reaches 80066b48 while 80013378 reports mode 2. */
   if (!dw2_npc_range(ram_size, DW2_NPC_MODE_OFFSET, 4u)
         || dw2_npc_u32(ram, DW2_NPC_MODE_OFFSET) != DW2_NPC_MODE_CITY)
      return false;

   switch (id)
   {
      case 9000:
      {
         /* A free item slot inside the equipped cargo size. */
         unsigned size = dw2_npc_inventory_size(ram, ram_size);
         if (!size || size > DW2_NPC_INVENTORY_LIMIT)
            return false;
         for (index = 0; index < size; index++)
            if (!dw2_npc_u16(ram, DW2_NPC_PARTY_SCAN_OFFSET + index * 2u))
               return true;
         return false;
      }
      case 9001:
      {
         /* A free roster slot, with fewer than twelve raised Digimon. */
         bool empty = false;
         unsigned raised = 0;
         for (index = 0; index < DW2_NPC_ROSTER_COUNT; index++)
         {
            uint8_t state = ram[DW2_NPC_ROSTER_OFFSET
               + index * DW2_NPC_ROSTER_STRIDE + DW2_NPC_ROSTER_STATE_OFFSET];
            if (!state)
               empty = true;
            if (state > 1u)
               raised++;
         }
         return empty && raised < 12u;
      }
      case 9003:
         return dw2_npc_roster_species(ram, 218u);
      case 9004:
         return dw2_npc_roster_species(ram, 209u);
      case 9005:
         return dw2_npc_roster_species(ram, 67u);
      case 9009:
         return dw2_npc_u16(ram, DW2_NPC_TOURNAMENT_OFFSET) >= 16u;
      case 9010:
         return dw2_npc_u16(ram, DW2_NPC_TOURNAMENT_OFFSET) >= 31u;
      case 9012:
         return dw2_npc_s32(ram, DW2_NPC_LOCATION_OFFSET) == 0x32a;
      case 9034:
         return dw2_npc_s32(ram, DW2_NPC_LOCATION_OFFSET) == 0x32b;
      case 9013:
      case 9014:
      {
         /* Scene entry pairs: 0x301 wants entry 3 or 4, 0x321 wants 2 or 3. */
         int32_t scene = dw2_npc_s32(ram, DW2_NPC_SCENE_OFFSET);
         int32_t entry = dw2_npc_s32(ram, DW2_NPC_ENTRY_INDEX_OFFSET);
         int32_t want = id == 9013 ? 3 : 4;
         if (scene == 0x301)
            return entry == want;
         if (scene == 0x321)
            return entry == want - 1;
         return false;
      }
      case 9023:
      {
         /* No partner in the first three roster slots carries +0xfa. */
         for (index = 0; index < 3u; index++)
         {
            size_t entry = DW2_NPC_ROSTER_OFFSET
               + index * DW2_NPC_ROSTER_STRIDE;
            if (ram[entry + DW2_NPC_ROSTER_STATE_OFFSET] == index + 3u
                  && dw2_npc_s16(ram, entry + DW2_NPC_ROSTER_EXTRA_OFFSET))
               return false;
         }
         return true;
      }
      default:
         break;
   }
   if (id >= 9015 && id <= 9022)
      return dw2_npc_s32(ram, DW2_NPC_BITS_OFFSET) >= bits_steps[id - 9015];
   if (id >= 9024 && id <= 9033)
      return ram[DW2_NPC_RANK_OFFSET] < (unsigned)(id - 9022);
   return false;
}

static bool dw2_npc_flag(const uint8_t *ram, size_t ram_size, int id,
      bool *value);

/* 8005f664 is the story chapter, not a rank: MESS2360's Jijimon action 1907
 * assigns it. The Tamer rank lives at 8005e632 and is read by 80066b48. */
static bool dw2_npc_flag(const uint8_t *ram, size_t ram_size, int id,
      bool *value)
{
   size_t offset;
   unsigned bit;
   unsigned index;

   if (id < 0 || ram_size < DW2_NPC_FLAG_STATE_LIMIT)
      return false;
   if (id >= 4000)
   {
      /* 9035 is the only arm that recurses into the flag banks. */
      if (id == 9035)
      {
         bool a, b, c;
         if (!dw2_npc_range(ram_size, DW2_NPC_MODE_OFFSET, 4u)
               || dw2_npc_u32(ram, DW2_NPC_MODE_OFFSET) != DW2_NPC_MODE_CITY)
            *value = false;
         else if (!dw2_npc_flag(ram, ram_size, 710, &a)
               || !dw2_npc_flag(ram, ram_size, 711, &b)
               || !dw2_npc_flag(ram, ram_size, 712, &c))
            return false;
         else
            *value = a && b && c;
         return true;
      }
      *value = dw2_npc_overlay_predicate(ram, ram_size, id);
      return true;
   }
   if (id < 1000)
   {
      if (id < 600)      { offset = DW2_NPC_FLAG_BANK_A; bit = (unsigned)id; }
      else if (id < 700) { offset = DW2_NPC_FLAG_BANK_B; bit = (unsigned)id - 600u; }
      else if (id < 800) { offset = DW2_NPC_FLAG_BANK_C; bit = (unsigned)id - 700u; }
      else               { offset = DW2_NPC_FLAG_BANK_D; bit = (unsigned)id - 800u; }
      *value = (ram[offset + bit / 8u] & (1u << (bit % 8u))) != 0;
   }
   else if (id < 1600)
   {
      /* 8005f664 is the story chapter. */
      int32_t chapter = dw2_npc_s32(ram, DW2_NPC_PROGRESS_OFFSET);
      *value = id < 1100 ? chapter >= id - 1000 : chapter < id - 1500;
   }
   else if (id < 2237)
   {
      *value = false;
      for (index = 0; index < DW2_NPC_PARTY_SCAN_COUNT; index++)
         if (dw2_npc_u16(ram, DW2_NPC_PARTY_SCAN_OFFSET + index * 2u)
               == (uint32_t)(id - 2000))
            *value = true;
   }
   else if (id < 3000)
      *value = dw2_npc_s16(ram,
            DW2_NPC_ITEM_TABLE_OFFSET + (size_t)(id - 2000) * 2u) != 0;
   else
   {
      *value = false;
      for (index = 0; index < DW2_NPC_ROSTER_COUNT; index++)
      {
         offset = DW2_NPC_ROSTER_OFFSET + index * DW2_NPC_ROSTER_STRIDE;
         if (ram[offset + 0xe5u] == (unsigned)(id - 3000)
               && ram[offset + 0xe4u] > 1u)
            *value = true;
      }
   }
   return true;
}

/* 80022038: six {s16 id, s16 expected} pairs at +i*4, -1 skipped. */
static dw2_npc_condition_result_t dw2_npc_condition(const uint8_t *ram,
      size_t ram_size, size_t condition)
{
   unsigned pair;

   if (!dw2_npc_range(ram_size, condition, DW2_NPC_CONDITION_SIZE))
      return DW2_NPC_CONDITION_UNKNOWN;
   for (pair = 0; pair < DW2_NPC_CONDITION_PAIRS; pair++)
   {
      int id = dw2_npc_s16(ram, condition + pair * 4u);
      bool expected = dw2_npc_s16(ram, condition + pair * 4u + 2u) != 0;
      bool value;

      if (id == -1)
         continue;
      if (!dw2_npc_flag(ram, ram_size, id, &value))
         return DW2_NPC_CONDITION_UNKNOWN;
      if (value != expected)
         return DW2_NPC_CONDITION_FAILED;
   }
   return DW2_NPC_CONDITION_PASSED;
}

/* The scene's own city record must agree with the global. They only differ
 * if 8001e28c has not run for the scene the player is standing in. Domains
 * set the same global from their floor descriptor and do not keep group
 * 0x309 resident, so the check is skipped rather than failed there. */
static bool dw2_npc_actor_group(const uint8_t *ram, size_t ram_size,
      uint32_t *group)
{
   uint32_t candidate;
   uint32_t scene;
   size_t city_base;
   size_t record_offset;

   if (!dw2_npc_range(ram_size, DW2_NPC_ACTOR_GROUP_OFFSET, 4u))
      return false;
   candidate = dw2_npc_u32(ram, DW2_NPC_ACTOR_GROUP_OFFSET);
   if (!candidate || candidate > 0xffffu)
      return false;
   *group = candidate;

   if (!dw2_npc_range(ram_size, DW2_NPC_SCENE_OFFSET, 2u))
      return true;
   scene = dw2_npc_u16(ram, DW2_NPC_SCENE_OFFSET);
   /* Actual city state is 0x300+scene. Also accept the low scene index used
    * by detached navigation snapshots and the actor-table audit. */
   if (scene >= 0x301u && scene <= 0x300u + DW2_NPC_SCENE_LIMIT)
      scene -= 0x300u;
   if (!scene || scene > DW2_NPC_SCENE_LIMIT
         || !dw2_npc_group_base(ram, ram_size, DW2_NPC_CITY_GROUP, &city_base)
         || !dw2_npc_group_entry(ram, ram_size, city_base,
            (size_t)scene - 1u,
            DW2_NPC_CITY_ACTOR_GROUP_OFFSET + 4u, &record_offset))
      return true;
   return dw2_npc_u32(ram, record_offset + DW2_NPC_CITY_ACTOR_GROUP_OFFSET)
      == candidate;
}

static bool dw2_npc_slot_label(const uint8_t *ram, size_t ram_size,
      size_t base_offset, size_t record_offset, unsigned slot,
      char *out, size_t out_size)
{
   uint32_t relative = dw2_npc_u32(ram, record_offset
         + DW2_NPC_RECORD_MESSAGE_OFFSET + (size_t)slot * 4u);

   out[0] = '\0';
   if (!relative || relative >= ram_size - base_offset)
      return false;
   return beetle_accessibility_dw2_decode_speaker_label(ram, ram_size,
         0x80000000u | (uint32_t)(base_offset + relative), out, out_size)
      && out[0] != '\0';
}

/* 8001e514: the first slot whose condition passes, otherwise slot 0. It never
 * searches on for a slot that happens to carry a name, which is what keeps a
 * story-gated page out of reach. */
static bool dw2_npc_selected_label(const uint8_t *ram, size_t ram_size,
      size_t base_offset, size_t condition_table, size_t record_offset,
      char *out, size_t out_size, bool *allow_fallback)
{
   unsigned slot;
   unsigned selected = 0;
   uint32_t relative;

   *allow_fallback = false;

   for (slot = 0; slot < DW2_NPC_MESSAGE_SLOTS; slot++)
   {
      size_t condition = condition_table
         + (size_t)ram[record_offset + DW2_NPC_RECORD_SLOT_CONDITION_OFFSET
            + slot] * DW2_NPC_CONDITION_SIZE;
      dw2_npc_condition_result_t result = dw2_npc_condition(ram, ram_size,
            condition);

      if (result == DW2_NPC_CONDITION_UNKNOWN)
         return false;
      if (result == DW2_NPC_CONDITION_PASSED)
      {
         selected = slot;
         break;
      }
   }
   if (dw2_npc_slot_label(ram, ram_size, base_offset, record_offset,
            selected, out, out_size))
      return true;
   relative = dw2_npc_u32(ram, record_offset + DW2_NPC_RECORD_MESSAGE_OFFSET
         + selected * 4u);
   /* A valid page may start with the player's question (Ben Oldman at
    * Archive Port) or be a script placeholder. Identity may then come from
    * the actor's other records, provided all their labels agree. */
   *allow_fallback = !relative || relative < ram_size - base_offset;
   return false;
}

/* Every labelled page of one record, when they all agree. */
static bool dw2_npc_record_label(const uint8_t *ram, size_t ram_size,
      size_t base_offset, size_t record_offset, char *out, size_t out_size)
{
   char unanimous[BEETLE_DW2_NPC_NAME_MAX];
   bool valid = false;
   unsigned slot;

   out[0] = '\0';
   for (slot = 0; slot < DW2_NPC_MESSAGE_SLOTS; slot++)
   {
      char candidate[BEETLE_DW2_NPC_NAME_MAX];

      if (!dw2_npc_slot_label(ram, ram_size, base_offset, record_offset, slot,
               candidate, sizeof(candidate)))
         continue;
      if (!valid)
      {
         memcpy(unanimous, candidate, sizeof(unanimous));
         valid = true;
      }
      else if (strcmp(unanimous, candidate))
         return false;
   }
   if (!valid || strlen(unanimous) + 1u > out_size)
      return false;
   memcpy(out, unanimous, strlen(unanimous) + 1u);
   return true;
}

/* Every labelled page of every record carrying the same model in this scene's
 * table, when they all agree. This is what names the leaders standing at a
 * story transition, whose own record holds only a placeholder page.
 *
 * CONFLICT is deliberately distinct from ABSENT: a scene whose own records
 * give one model two identities has settled that this model does not identify
 * a person, and no weaker source may overrule it. */
typedef enum dw2_npc_model_result
{
   DW2_NPC_MODEL_ABSENT = 0,
   DW2_NPC_MODEL_FOUND,
   DW2_NPC_MODEL_CONFLICT
} dw2_npc_model_result_t;

static dw2_npc_model_result_t dw2_npc_scene_model_label(const uint8_t *ram,
      size_t ram_size, size_t base_offset, size_t table_offset,
      uint16_t model, char *out, size_t out_size)
{
   char unanimous[BEETLE_DW2_NPC_NAME_MAX];
   bool valid = false;
   size_t index;

   out[0] = '\0';
   if (dw2_npc_script_model(model))
      return DW2_NPC_MODEL_ABSENT;
   for (index = 0; index < DW2_NPC_RECORD_LIMIT; index++)
   {
      size_t record_offset = table_offset + index * DW2_NPC_RECORD_SIZE;
      unsigned slot;

      if (!dw2_npc_range(ram_size, record_offset, DW2_NPC_RECORD_SIZE))
         break;
      if (!dw2_npc_u16(ram, record_offset + DW2_NPC_RECORD_MODEL_OFFSET))
         break;
      if (dw2_npc_u16(ram, record_offset + DW2_NPC_RECORD_MODEL_OFFSET)
            != model)
         continue;
      for (slot = 0; slot < DW2_NPC_MESSAGE_SLOTS; slot++)
      {
         char candidate[BEETLE_DW2_NPC_NAME_MAX];

         if (!dw2_npc_slot_label(ram, ram_size, base_offset, record_offset,
                  slot, candidate, sizeof(candidate)))
            continue;
         if (!valid)
         {
            memcpy(unanimous, candidate, sizeof(unanimous));
            valid = true;
         }
         else if (strcmp(unanimous, candidate))
         {
            out[0] = '\0';
            return DW2_NPC_MODEL_CONFLICT;
         }
      }
   }
   if (!valid)
      return DW2_NPC_MODEL_ABSENT;
   if (strlen(unanimous) + 1u > out_size)
      return DW2_NPC_MODEL_CONFLICT;
   memcpy(out, unanimous, strlen(unanimous) + 1u);
   return DW2_NPC_MODEL_FOUND;
}

/* 8001e67c then 8001e6a8 then 8001e758. City models 400-500 carry an empty
 * string here; Digimon models resolve to the species name. */
static bool dw2_npc_model_registry_label(const uint8_t *ram, size_t ram_size,
      uint16_t model, char *out, size_t out_size)
{
   uint32_t group;
   size_t base_offset;
   size_t table_offset;
   size_t index;

   out[0] = '\0';
   if (!model || dw2_npc_script_model(model))
      return false;
   if (model < 300u)
      group = DW2_NPC_NAME_GROUP_LOW;
   else if (model >= 400u && model <= 500u)
      group = DW2_NPC_NAME_GROUP_MID;
   else
      group = DW2_NPC_NAME_GROUP_HIGH;
   if (!dw2_npc_group_base(ram, ram_size, group, &base_offset)
         || !dw2_npc_group_entry(ram, ram_size, base_offset, 0u,
            DW2_NPC_NAME_RECORD_SIZE, &table_offset))
      return false;
   for (index = 0; index < DW2_NPC_NAME_RECORD_LIMIT; index++)
   {
      size_t record_offset = table_offset + index * DW2_NPC_NAME_RECORD_SIZE;
      uint32_t identity;
      uint32_t relative;

      if (!dw2_npc_range(ram_size, record_offset, DW2_NPC_NAME_RECORD_SIZE))
         return false;
      identity = (dw2_npc_u32(ram, record_offset + 4u) >> 1) & 0x7fffu;
      if (!identity)
         return false;
      if (identity != model)
         continue;
      relative = dw2_npc_u32(ram, record_offset);
      if (relative >= ram_size - base_offset)
         return false;
      return beetle_accessibility_dw2_decode_text(ram, ram_size,
            0x80000000u | (uint32_t)(base_offset + relative),
            BEETLE_DW2_TEXT_MENU, out, out_size) && out[0] != '\0';
   }
   return false;
}

static bool dw2_npc_catalog_label(uint16_t model, char *out, size_t out_size)
{
   size_t index;

   out[0] = '\0';
   if (dw2_npc_script_model(model))
      return false;
   for (index = 0; index < sizeof(dw2_npc_catalog)
         / sizeof(dw2_npc_catalog[0]); index++)
   {
      const char *name = dw2_npc_catalog[index].name;

      if (dw2_npc_catalog[index].model != model)
         continue;
      if (strlen(name) + 1u > out_size)
         return false;
      memcpy(out, name, strlen(name) + 1u);
      return true;
   }
   return false;
}

/* Walk the table the way 8001e390 does, stopping at the first zero model, so
 * an index past the terminator resolves nothing. */
static bool dw2_npc_record_at(const uint8_t *ram, size_t ram_size,
      size_t table_offset, size_t record_index, size_t *record_offset)
{
   size_t scan_index;

   if (record_index >= DW2_NPC_RECORD_LIMIT)
      return false;
   for (scan_index = 0; scan_index <= record_index; scan_index++)
   {
      *record_offset = table_offset + scan_index * DW2_NPC_RECORD_SIZE;
      if (!dw2_npc_range(ram_size, *record_offset, DW2_NPC_RECORD_SIZE)
            || !dw2_npc_u16(ram, *record_offset + DW2_NPC_RECORD_MODEL_OFFSET))
         return false;
   }
   return true;
}

static beetle_dw2_npc_source_t dw2_npc_resolve(const uint8_t *ram,
      size_t ram_size, size_t base_offset, size_t table_offset,
      size_t condition_table, size_t record_offset, char *out,
      size_t out_size)
{
   uint16_t model = dw2_npc_u16(ram,
         record_offset + DW2_NPC_RECORD_MODEL_OFFSET);
   bool allow_fallback;
   char catalog_name[BEETLE_DW2_NPC_NAME_MAX];
   char record_name[BEETLE_DW2_NPC_NAME_MAX];

   if (dw2_npc_selected_label(ram, ram_size, base_offset, condition_table,
            record_offset, out, out_size, &allow_fallback))
      return BEETLE_DW2_NPC_SOURCE_SLOT;
   if (!allow_fallback)
      return BEETLE_DW2_NPC_SOURCE_NONE;
   switch (dw2_npc_scene_model_label(ram, ram_size, base_offset, table_offset,
            model, out, out_size))
   {
      case DW2_NPC_MODEL_FOUND:
         /* A mismatched or modified record cannot borrow a later name
          * that contradicts this model's independently verified identity.
          * Explicit current-page names still take priority above. */
         if (dw2_npc_catalog_label(model,catalog_name,sizeof(catalog_name))
               && strcmp(catalog_name,out))
         {
            out[0] = '\0';
            return BEETLE_DW2_NPC_SOURCE_NONE;
         }
         if (dw2_npc_record_label(ram,ram_size,base_offset,record_offset,record_name,sizeof(record_name)))
            return BEETLE_DW2_NPC_SOURCE_RECORD;
         return BEETLE_DW2_NPC_SOURCE_SCENE_MODEL;
      case DW2_NPC_MODEL_CONFLICT:
         /* This scene's own records disagree about the model. Neither the
          * registry nor the catalogue may overrule that. */
         out[0] = '\0';
         return BEETLE_DW2_NPC_SOURCE_NONE;
      case DW2_NPC_MODEL_ABSENT:
         break;
   }
   if (dw2_npc_model_registry_label(ram, ram_size, model, out, out_size))
      return BEETLE_DW2_NPC_SOURCE_MODEL_REGISTRY;
   if (dw2_npc_catalog_label(model, out, out_size))
      return BEETLE_DW2_NPC_SOURCE_CATALOG;
   out[0] = '\0';
   return BEETLE_DW2_NPC_SOURCE_NONE;
}

const char *beetle_accessibility_dw2_npc_source_label(
      beetle_dw2_npc_source_t source)
{
   switch (source)
   {
      case BEETLE_DW2_NPC_SOURCE_SLOT: return "current page";
      case BEETLE_DW2_NPC_SOURCE_RECORD: return "same record";
      case BEETLE_DW2_NPC_SOURCE_SCENE_MODEL: return "same model in scene";
      case BEETLE_DW2_NPC_SOURCE_MODEL_REGISTRY: return "model registry";
      case BEETLE_DW2_NPC_SOURCE_CATALOG: return "model catalogue";
      case BEETLE_DW2_NPC_SOURCE_UNKNOWN: return "not shown yet";
      case BEETLE_DW2_NPC_SOURCE_NONE: break;
   }
   return "none";
}

bool beetle_accessibility_dw2_npc_name_ex(const uint8_t *ram, size_t ram_size,
      size_t task_offset, size_t data_offset, char *out, size_t out_size,
      beetle_dw2_npc_source_t *source)
{
   uint32_t group;
   size_t base_offset;
   size_t table_offset;
   size_t condition_table;
   size_t record_offset = 0;
   size_t record_index;
   uint16_t model;
   beetle_dw2_npc_source_t resolved;

   if (source)
      *source = BEETLE_DW2_NPC_SOURCE_NONE;
   if (!ram || !out || !out_size)
      return false;
   out[0] = '\0';
   if (!dw2_npc_range(ram_size, task_offset, 0x38u)
         || !dw2_npc_range(ram_size,
            data_offset + DW2_NPC_DATA_RECORD_INDEX_OFFSET, 4u))
      return false;

   /* 8006aa4c hides models 497-499 and marks 500 as the player: those tasks
    * are cutscene machinery, not a person the player can walk up to, and
    * their records carry many different identities. */
   model = dw2_npc_u16(ram, task_offset + DW2_NPC_TASK_MODEL_OFFSET);
   if (!model || dw2_npc_script_model(model))
      return false;

   if (!dw2_npc_actor_group(ram, ram_size, &group)
         || !dw2_npc_group_base(ram, ram_size, group, &base_offset)
         || !dw2_npc_group_entry(ram, ram_size, base_offset, 0u,
            DW2_NPC_RECORD_SIZE, &table_offset)
         || !dw2_npc_group_entry(ram, ram_size, base_offset, 2u,
            DW2_NPC_CONDITION_SIZE, &condition_table))
      return false;

   record_index = dw2_npc_u32(ram,
         data_offset + DW2_NPC_DATA_RECORD_INDEX_OFFSET);
   if (!dw2_npc_record_at(ram, ram_size, table_offset, record_index,
            &record_offset))
      return false;
   /* 8006aa4c copies the record's model into task +0x0c at spawn, so this
    * equality is what proves the index still addresses this actor. */
   if (dw2_npc_u16(ram, record_offset + DW2_NPC_RECORD_MODEL_OFFSET) != model)
      return false;

   resolved = dw2_npc_resolve(ram, ram_size, base_offset, table_offset,
         condition_table, record_offset, out, out_size);
   if (resolved == BEETLE_DW2_NPC_SOURCE_NONE)
   {
      /* A live actor the game has not identified. Report that rather than
       * borrowing an identity it has not shown. */
      if (sizeof(BEETLE_DW2_NPC_UNKNOWN_NAME) > out_size)
         return false;
      memcpy(out, BEETLE_DW2_NPC_UNKNOWN_NAME,
            sizeof(BEETLE_DW2_NPC_UNKNOWN_NAME));
      resolved = BEETLE_DW2_NPC_SOURCE_UNKNOWN;
   }
   if (source)
      *source = resolved;
   return true;
}

bool beetle_accessibility_dw2_npc_name(const uint8_t *ram, size_t ram_size,
      size_t task_offset, size_t data_offset, char *out, size_t out_size)
{
   return beetle_accessibility_dw2_npc_name_ex(ram, ram_size, task_offset,
         data_offset, out, out_size, NULL);
}

size_t beetle_accessibility_dw2_npc_domain_events(const uint8_t *ram,
      size_t ram_size, size_t domain_offset, beetle_dw2_npc_event_t *out,
      size_t out_max)
{
   uint32_t group;
   uint32_t count;
   size_t base_offset;
   size_t table_offset;
   size_t condition_table;
   size_t written = 0;
   uint32_t index;

   if (!ram || !out || !out_max)
      return 0;
   if (!domain_offset
         && !dw2_npc_address(dw2_npc_range(ram_size,
                  DW2_NPC_DOMAIN_STATE_POINTER_OFFSET, 4u)
               ? dw2_npc_u32(ram, DW2_NPC_DOMAIN_STATE_POINTER_OFFSET) : 0u,
            ram_size, DW2_NPC_DOMAIN_STATE_SIZE, &domain_offset))
      return 0;
   if (!dw2_npc_range(ram_size, domain_offset, DW2_NPC_DOMAIN_STATE_SIZE))
      return 0;
   count = dw2_npc_u32(ram, domain_offset + DW2_NPC_DOMAIN_EVENT_COUNT_OFFSET);
   /* 8006e60c writes entries from +0x144 up to the count word itself, so more
    * than five would have overwritten the count. Refuse rather than read on. */
   if (!count || count > BEETLE_DW2_NPC_EVENT_LIMIT)
      return 0;
   if (!dw2_npc_actor_group(ram, ram_size, &group)
         || !dw2_npc_group_base(ram, ram_size, group, &base_offset)
         || !dw2_npc_group_entry(ram, ram_size, base_offset, 0u,
            DW2_NPC_RECORD_SIZE, &table_offset)
         || !dw2_npc_group_entry(ram, ram_size, base_offset, 2u,
            DW2_NPC_CONDITION_SIZE, &condition_table))
      return 0;

   for (index = 0; index < count && written < out_max; index++)
   {
      size_t entry = domain_offset + DW2_NPC_DOMAIN_EVENT_OFFSET
         + (size_t)index * DW2_NPC_DOMAIN_EVENT_STRIDE;
      size_t record_offset;
      beetle_dw2_npc_event_t *event = &out[written];
      uint16_t x = dw2_npc_u16(ram, entry);
      uint16_t y = dw2_npc_u16(ram, entry + 2u);

      /* 8006e6cc writes 0xffff into both halves once the player has stepped
       * on the tile, so such an entry is no longer an active target. */
      if (x == DW2_NPC_DOMAIN_EVENT_CONSUMED
            || y == DW2_NPC_DOMAIN_EVENT_CONSUMED)
         continue;
      memset(event, 0, sizeof(*event));
      event->x = x;
      event->y = y;
      event->record_index = dw2_npc_u32(ram, entry + 4u);
      if (dw2_npc_record_at(ram, ram_size, table_offset,
               event->record_index, &record_offset))
      {
         event->source = dw2_npc_resolve(ram, ram_size, base_offset,
               table_offset, condition_table, record_offset, event->name,
               sizeof(event->name));
         event->named = event->source != BEETLE_DW2_NPC_SOURCE_NONE;
      }
      if (!event->named)
      {
         event->source = BEETLE_DW2_NPC_SOURCE_UNKNOWN;
         memcpy(event->name, BEETLE_DW2_NPC_UNKNOWN_NAME,
               sizeof(BEETLE_DW2_NPC_UNKNOWN_NAME));
      }
      written++;
   }
   return written;
}
