#include "accessibility_dw2_story.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static unsigned story_collection_clues;

void beetle_accessibility_dw2_story_reset(void)
{ story_collection_clues=0; }

/* US SLUS-01193. 80021e78 reads these banks; 800221c4 assigns chapter
 * with action 1900+n. Rank is a DIFFERENT byte (80066b48: 9024..9033).
 * Provenance/mission transitions: original CITY/MESS2010..2390 and
 * DUNG/MESS6724,6820; artifacts/dw2-story-npcs-20260919. Never run scripts. */
static uint16_t story_u16(const uint8_t *r, size_t p)
{ return (uint16_t)(r[p] | ((unsigned)r[p+1] << 8)); }

static uint32_t story_u32(const uint8_t *r, size_t p)
{ return story_u16(r,p) | ((uint32_t)story_u16(r,p+2) << 16); }

static bool story_flag(const uint8_t *r, unsigned id)
{
   size_t base;
   if (id < 600) base=0x5f624;
   else if (id < 700) { base=0x5f644; id-=600; }
   else if (id < 800) { base=0x5f64c; id-=700; }
   else { base=0x5f654; id-=800; }
   return (r[base+id/8] & (1u << (id%8))) != 0;
}

static bool story_item(const uint8_t *r, unsigned id)
{ return story_u16(r,0x5f3f4+id*2) != 0; }

void beetle_accessibility_dw2_story_observe_dialogue(const uint8_t *ram,
      size_t ram_size, const char *visible_text)
{
   if (!ram || ram_size<0x5f668u || !visible_text
         || story_u32(ram,0x5f664)!=4 || !story_flag(ram,846)) return;
   /* MESS2200: Gus's paid clue is spoken but explicitly not saved in the
    * Browser. Observe only text accepted by the live dialogue renderer. */
   if (strstr(visible_text,"Yanmamon is at SCSI Domain")) story_collection_clues|=1u;
   if (strstr(visible_text,"Syakomon is at Web Domain")) story_collection_clues|=2u;
   if (strstr(visible_text,"Ikkakumon is at Video Domain")) story_collection_clues|=4u;
}

/* Native 80066a4c includes only Digimon in the Digi-Beetle memory, not
 * those left in the server. This is the actual collection-quest gate. */
static bool story_species(const uint8_t *r, unsigned species)
{
   unsigned i;
   for (i=0;i<36;i++)
      if (r[0x5e704+i*0x5c] >= 2 && r[0x5e705+i*0x5c] == species)
         return true;
   return false;
}

static bool story_server_species(const uint8_t *r, unsigned species)
{
   unsigned i;
   for (i=0;i<36;i++)
      if (r[0x5e704+i*0x5c] == 1 && r[0x5e705+i*0x5c] == species)
         return true;
   return false;
}

const char *beetle_accessibility_dw2_story_domain_name(unsigned domain)
{
   /* DATA4000 entry 10, DUNG4000..7300 native displayed floor labels.
    * Giga precedes Diode; Boot is 29 and Tera is 32, not the reverse. */
   static const char *const names[] = {
      "SCSI Domain", "Video Domain", "Disk Domain", "BIOS Domain",
      "Drive Domain", "Web Domain", "Modem Domain",
      "SCSI Domain", "Video Domain", "Disk Domain", "BIOS Domain",
      "Drive Domain", "Web Domain", "Modem Domain", "Code Domain",
      "Laser Domain", "Giga Domain", "Diode Domain", "Port Domain",
      "Scan Domain", "Data Domain", "Patch Domain", "Mega Domain",
      "Soft Domain", "Bug Domain", "RAM Domain", "ROM Domain",
      "Core Tower", "Chaos Tower", "Boot Domain", "DVD Domain",
      "Power Domain", "Tera Domain", "Training Domain"
   };
   return domain < sizeof(names)/sizeof(names[0]) ? names[domain] : "Domain";
}

const char *beetle_accessibility_dw2_story_scene_name(unsigned scene)
{
   static const char *const names[] = {
      "Area Selection", "Main Gate", "Digi-Beetle Factory",
      "Gold Hawk HQ", "Gold Hawk Leader's Room", "Gold Hawk DNA Digivolve Room",
      "Gold Hawk Tamer's Room", "Blue Falcon HQ", "Blue Falcon Leader's Room",
      "Blue Falcon DNA Digivolve Room", "Blue Falcon Tamer's Room",
      "Black Sword HQ", "Black Sword Leader's Room", "Black Sword DNA Digivolve Room",
      "Black Sword Tamer's Room", "Tamer's Club", "Digimon Center", "Coliseum",
      "Coliseum Registration", "Device Dome Entrance", "Device Dome",
      "Device Dome DNA Digivolve Room", "Device Dome Laboratory",
      "Meditation Dome Entrance", "Meditation Dome", "Clockmon's Room",
      "WereGarurumon's Room", "Archive Port", "Archive Ship Control Room",
      "Archive Ship Teleport Gate", "Shuttle Port", "Shuttle Port Warp Gate",
      "Shuttle Port Exit", "File City Main Gate", "File City Item Shop",
      "File City Digi-Beetle Factory", "Jijimon's House", "Archive Ship on File Island",
      "Archive Ship Teleport Gate", "Kernel Zone Shuttle", "Kernel Zone Exit",
      "Main Gate"
   };
   return scene < sizeof(names)/sizeof(names[0]) ? names[scene] : "Area Selection";
}

static void story_add(beetle_dw2_story_objective_t *out, size_t cap,
      size_t *n, int scene, int domain, const char *npc, const char *label)
{
   const unsigned char *p;
   uint32_t hash=2166136261u;
   beetle_dw2_story_objective_t *o;
   if (*n >= cap) return;
   o=&out[(*n)++];
   memset(o,0,sizeof(*o));
   o->scene=(int16_t)scene; o->domain=(int16_t)domain;
   snprintf(o->npc,sizeof(o->npc),"%s",npc ? npc : "");
   snprintf(o->label,sizeof(o->label),"%s",label);
   for (p=(const unsigned char *)label;*p;p++) hash=(hash^*p)*16777619u;
   o->id=(hash^(uint32_t)(scene+256*(domain+1))) | 0x80000000u;
}

static void story_domain(beetle_dw2_story_objective_t *out, size_t cap,
      size_t *n, unsigned domain, const char *purpose)
{
   char label[192];
   snprintf(label,sizeof(label),"%s: %s",beetle_accessibility_dw2_story_domain_name(domain),purpose);
   story_add(out,cap,n,0,(int)domain,NULL,label);
}

static void story_collect(const uint8_t *ram, beetle_dw2_story_objective_t *out,
      size_t cap, size_t *n, unsigned species, unsigned domain,
      const char *name, const char *recipient, bool location_known)
{
   char label[192];
   if (story_species(ram,species)) return;
   if (story_server_species(ram,species))
   {
      snprintf(label,sizeof(label),"Move %s from the server into Digi-Beetle memory at the Digimon Center for %s.",name,recipient);
      story_add(out,cap,n,16,-1,"Digimon Center Attendant",label);
   }
   else
   {
      if (!location_known)
      {
         snprintf(label,sizeof(label),"Recruit %s for %s. Gus Getum at Device Dome offers the three locations for 1000 BITs.",name,recipient);
         story_add(out,cap,n,20,-1,"Gus Getum",label);
         return;
      }
      snprintf(label,sizeof(label),"recruit %s for %s and keep it in Digi-Beetle memory",name,recipient);
      story_domain(out,cap,n,domain,label);
   }
}

size_t beetle_accessibility_dw2_story_objectives(const uint8_t *ram,
      size_t ram_size, beetle_dw2_story_objective_t *out, size_t capacity)
{
   unsigned chapter, guild=0, i;
   int leader, lobby, tamers;
   size_t n=0;
   const char *leader_name;
   char label[192];
   if (!ram || !out || !capacity || ram_size < 0x5f668u) return 0;
   chapter=story_u32(ram,0x5f664);
   if (chapter>10) return 0;
   if (chapter!=4) story_collection_clues=0;
   if (capacity>BEETLE_DW2_STORY_MAX_OBJECTIVES) capacity=BEETLE_DW2_STORY_MAX_OBJECTIVES;
   /* Guard-Team IDs in Important Items are 239,240,241, not model IDs. */
   for (i=0;i<3;i++) if (story_item(ram,239+i)) { guild=i+1; break; }
   leader=guild ? (int)guild*4 : 0; lobby=leader-1; tamers=leader+2;
   leader_name=guild==1 ? "Vandar" : guild==2 ? "Cecilia" : "Skull";
#define F(id) story_flag(ram,(id))
#define I(id) story_item(ram,(id))
#define NPC(scene,name,text) do { story_add(out,capacity,&n,scene,-1,name,text); return n; } while (0)
#define DOMAIN(id,text) do { story_domain(out,capacity,&n,id,text); return n; } while (0)
#define HQ(text) do { snprintf(label,sizeof(label),"Talk to %s at your HQ: %s",leader_name,text); NPC(leader,leader_name,label); } while (0)
#define PENDING(id,text) do { if (!F(700+(id))) story_domain(out,capacity,&n,id,text); } while (0)
   if (chapter && !guild)
      NPC(0,"","Story state unavailable: Guard Team identity could not be read.");
   switch (chapter)
   {
      case 0:
         if (!F(800)) NPC(1,"Zudokorn","Meet Zudokorn at Main Gate for your training mission.");
         if (!F(809) && F(729)) NPC(1,"Zudokorn","Return to Zudokorn at Main Gate after completing Boot Domain.");
         if (!F(809)) DOMAIN(29,"finish the training mission with Zudokorn, then return to Main Gate");
         story_add(out,capacity,&n,4,-1,"Vandar","Choose a Guard Team: speak to Vandar about joining the Gold Hawks.");
         story_add(out,capacity,&n,8,-1,"Cecilia","Choose a Guard Team: speak to Cecilia about joining the Blue Falcons.");
         story_add(out,capacity,&n,12,-1,"Skull","Choose a Guard Team: speak to Skull about joining the Black Swords.");
         return n;
      case 1:
         if (!F(700)) DOMAIN(0,"complete your first Guard Team mission");
         HQ("report completing SCSI Domain");
      case 2:
         PENDING(1,"complete the assigned mission"); PENDING(2,"complete the mission and meet Angemon");
         if (n) return n;
         if (!F(35)) HQ("report completing Video and Disk Domains");
         if (!F(56)) NPC(15,"Mark Shultz","Talk to Mark Shultz at Tamer's Club about BIOS Domain. Leave an item slot free in the Digi-Beetle.");
         if (!F(703)) DOMAIN(3,"complete Mark Shultz's challenge");
         HQ("report BIOS Domain and receive your Entry Pass");
      case 3:
         if (!F(52)) HQ("hear about the missing Generator Parts");
         PENDING(4,"search for the missing Generator Parts"); PENDING(5,"search for the missing Generator Parts");
         if (n) return n;
         if (!F(829)) NPC(15,"","Enter Tamer's Club to follow the Generator Parts lead.");
         if (!F(61)) NPC(24,"Angemon","Ask Angemon at Meditation Dome where to find Kim.");
         if (!F(835)) DOMAIN(6,"find Kim and learn where Device Dome is");
         if (!F(59)) NPC(20,"Kim","Meet Kim at Device Dome about the Generator Parts.");
         if (!F(836)) NPC(22,"Techna-Donna","Meet Techna-Donna in Device Dome's laboratory about the Generator Parts.");
         if (!F(58)) HQ("report the restored Power Generator");
         if (ram[0x5e632]<3) NPC(18,"Coliseum Attendant","Enter Coliseum tournaments and reach Rank 3 Rookie Tamer.");
         HQ("receive the next mission as a Rank 3 Rookie Tamer");
      case 4:
         PENDING(7,"defeat the Blood Knights"); PENDING(8,"defeat the Blood Knights"); PENDING(9,"defeat the Blood Knights");
         if (n) return n;
         if (!F(844)) HQ("report the Blood Knights' letter about the Archive Ship");
         if (!F(846)) NPC(22,"Professor Piyotte","Ask Professor Piyotte in Device Dome's laboratory about the Archive Ship.");
         if (!F(847))
         {
            /* 9003/4/5 -> 80066a4c(218/209/67). All must be in beetle memory. */
            /* MESS2200 Gus Getum's location information. A stored Digimon
             * needs transferring, not catching again. These missions use
             * the reopened SCSI/Video, but the original Web Domain. */
            story_collect(ram,out,capacity,&n,67,7,"Yanmamon","Professor Piyotte",(story_collection_clues&1u)!=0);
            story_collect(ram,out,capacity,&n,218,5,"Syakomon","Professor Piyotte",(story_collection_clues&2u)!=0);
            story_collect(ram,out,capacity,&n,209,8,"Ikkakumon","Professor Piyotte",(story_collection_clues&4u)!=0);
            if (n) return n;
            NPC(22,"Professor Piyotte","Show Professor Piyotte the three requested Digimon in your Digi-Beetle memory.");
         }
         if (!F(848)) HQ("report Piyotte's information about Ben Oldman");
         if (!F(849)) NPC(21,"Professor Piyotte","Ask Professor Piyotte in Device Dome's DNA Digivolve Room about the sick Digimon.");
         if (!F(850) && !I(249))
            story_add(out,capacity,&n,25,-1,"Clockmon","Bring a Digivice to Clockmon at Meditation Dome for the DATA Patch.");
         if (!F(851) && !I(248))
            story_add(out,capacity,&n,26,-1,"WereGarurumon","Meditate with WereGarurumon at Meditation Dome for the VACCINE Patch.");
         if (n) return n;
         if (!F(852)) NPC(24,"Angemon","Ask Angemon about a Virus Type Digimon for the final DNA Patch.");
         if (!F(853)) NPC(20,"Kim","Ask Kim at Device Dome for the VIRUS Patch.");
         if (!F(854)) NPC(21,"Professor Piyotte","Bring all three DNA Patches to Professor Piyotte for the Wild Code.");
         HQ("report Piyotte's Wild Code and receive the next mission");
      case 5:
         PENDING(10,"defeat the Blood Knights"); PENDING(11,"defeat the Blood Knights"); PENDING(12,"defeat the Blood Knights");
         if (n) return n;
         if (!F(859)) NPC(20,"Kim","Talk to Kim at Device Dome about the machine she is building.");
         if (!F(860))
         {
            story_collect(ram,out,capacity,&n,76,12,"Tankmon","Kim",true);
            if (n) return n;
            NPC(20,"Kim","Show Kim the Tankmon in your Digi-Beetle memory.");
         }
         if (!F(861)) NPC(lobby,"Lucky Luis","Return to your HQ and meet Lucky Luis.");
         if (!F(862)) NPC(tamers,"Lucky Luis","Talk to Lucky Luis in your HQ's Tamer's Room about Bertran and Joy Joy.");
         if (!F(713)) DOMAIN(13,"rescue Joy Joy and Bertran and finish the Blood Knights mission");
         if (!F(121)) NPC(lobby,"Lucky Luis","Return to your HQ after rescuing Joy Joy and Bertran.");
         if (!F(866)) NPC(tamers,"Joy Joy","Talk to Joy Joy in your HQ's Tamer's Room about Ben Oldman.");
         if (!F(867)) NPC(27,"Ben Oldman","Meet Ben Oldman at Archive Port.");
         if (!F(730)) DOMAIN(30,"accept Ben Oldman's challenge");
         if (!F(869)) NPC(27,"Ben Oldman","Return to Ben Oldman at Archive Port for the Archive Ship information.");
         if (!F(870)) HQ("report Ben Oldman's Old Map data");
         if (!F(714)) DOMAIN(14,"complete the mission to find the Blood Knights' officer");
         if (!F(120)) NPC(27,"","Return to Archive Port after the attack on the ship.");
         HQ("meet Ben Oldman and receive the Laser Domain mission");
      case 6:
         if (!F(715)) DOMAIN(15,"defeat all three bosses and recover the Navi-Disk");
         if (!F(878)) HQ("report Laser Domain and ask about the Ship Key");
         if (!F(879)) NPC(24,"Angemon","Ask Angemon at Meditation Dome about the Ship Key.");
         if (!F(880)) NPC(17,"Leomon","Talk to Leomon at the Coliseum about File Island.");
         if (!F(881)) NPC(16,"Agumon","Meet Agumon at the Digimon Center for the Ship Key.");
         if (!F(882)) NPC(27,"Ben Oldman","Bring the Ship Key to Ben Oldman at Archive Port and travel to File Island.");
         if (!F(883)) NPC(33,"MasterTyrannomon","Meet MasterTyrannomon at File City Main Gate.");
         if (!F(731)) DOMAIN(31,"complete MasterTyrannomon's challenge");
         if (!F(885)) NPC(33,"MasterTyrannomon","Return to MasterTyrannomon for entry to File City.");
         if (!F(891))
         {
            /* MESS2330 only identifies four possible prisons. Soft Domain
             * is not named until Jijimon's later report (MESS2360). */
            story_domain(out,capacity,&n,16,"search for Jijimon; finish his rescue if you have found him");
            PENDING(17,"search for Jijimon"); PENDING(18,"search for Jijimon"); PENDING(19,"search for Jijimon");
            return n;
         }
         if (F(891) && !F(117)) NPC(36,"Jijimon","Meet the rescued Jijimon at his house in File City.");
         PENDING(17,"search for clues about Soft Domain"); PENDING(18,"search for clues about Soft Domain"); PENDING(19,"search for clues about Soft Domain");
         if (n) return n;
         NPC(36,"Jijimon","Ask Jijimon about Soft Domain and the three Chaos Rings.");
      case 7:
         if (!F(893)) NPC(33,"MasterTyrannomon","Go to File City Main Gate to continue your mission.");
         if (!F(894)) NPC(36,"Jijimon","Return to Jijimon's House after the disturbance in File City.");
         if (!I(262)) PENDING(20,"obtain the Pied Ring from the Chaos General");
         if (!I(263)) PENDING(21,"obtain the W. Grey Ring from the Chaos General");
         if (!I(264)) PENDING(22,"obtain the Seadra Ring from the Chaos General");
         if (n) return n;
         if (!F(131)) NPC(36,"Jijimon","Bring all three Chaos Rings to Jijimon to locate Soft Domain.");
         if (!F(723)) DOMAIN(23,"confront Crimson and defeat Chaos Lord");
         if (!F(128)) NPC(36,"Jijimon","Report defeating Chaos Lord to Jijimon.");
         HQ("report File Island and pursue the Blood Knights on Directory Continent");
      case 8:
         PENDING(24,"pursue the Blood Knights"); PENDING(25,"pursue the Blood Knights");
         if (n) return n;
         if (!F(923)) NPC(1,"","Return to Main Gate to find the Blood Knights' trail.");
         if (!F(924)) NPC(17,"","Follow the suspect to the Coliseum.");
         if (!F(925)) NPC(15,"","Follow the suspect to Tamer's Club.");
         if (!F(132)) NPC(20,"Kim","Take Esteena to see GAIA at Device Dome.");
         if (!F(646)) HQ("question Commander Damien about Crimson's location");
         if (!F(726)) DOMAIN(26,"confront Crimson");
         if (!F(134)) HQ("report defeating Crimson");
         if (!F(130)) NPC(18,"Coliseum Attendant","Enter Coliseum tournaments and become a Rank 9 Chief Tamer.");
         HQ("meet the three Guard Team Leaders as Chief Tamer");
      case 9:
         if (!F(652)) NPC(30,"","Go to Shuttle Port to depart for the Kernel Zone.");
         if (!F(653)) DOMAIN(27,"meet your Guard Team's Guardian and complete the trial");
         DOMAIN(28,"confront OverLord GAIA and complete the final mission");
      case 10:
         if (!F(137)) NPC(17,"","Story complete. Go to the Coliseum for the celebration.");
         if (!F(138)) NPC(1,"New GAIA","Story complete. Meet New GAIA at Main Gate to resume exploring.");
         story_add(out,capacity,&n,0,32,NULL,"Story complete. Tera Domain is an optional challenge; you can explore freely.");
         return n;
   }
#undef PENDING
#undef HQ
#undef DOMAIN
#undef NPC
#undef I
#undef F
   return n;
}
