/* Game message data is generated locally by tools/generate_dw2_messages.py. */
#include "accessibility_dw2_profile.h"

#include <stddef.h>

const char beetle_accessibility_dw2_serial[] = "SLUS_011.93";

/* Native RAM watch used by the Digimon World 2 title-menu profile: 0x062A48. */

const char *beetle_accessibility_dw2_title_lookup(uint16_t signature,
      bool start_pressed)
{
   switch (signature)
   {
      case 0x0578:
         return start_pressed ? "New Game" : "Press Start";
      case 0x2643:
         return "New Game";
      case 0x01e2:
         return "Continue";
      default:
         return NULL;
   }
}

/* Native post-Start title selection index observed at RAM 0x15858C. */
const char *beetle_accessibility_dw2_title_selection_lookup(uint8_t selection)
{
   switch (selection)
   {
      case 0:
         return "New Game";
      case 1:
         return "Continue";
      case 2:
         return "Delete Game";
      default:
         return NULL;
   }
}

static const struct beetle_accessibility_dw2_message dw2_messages[] = {
#include "accessibility_dw2_messages.inc"
};

struct dw2_movie_cue
{
   uint32_t disc_sector;
   const char *text;
};

/* MOVIE000.STR: LBA 30914, 15500 sectors, 15 fps. */
static const struct dw2_movie_cue dw2_movie_cues[] = {
   { 30914, "A red and white BAN DAI logo appears on a black screen, then fades." },
   { 31214, "The screen fills with cascading green binary code, scrolling downward like digital rain." },
   { 31364, "The words NOW LOADING glow in white over the code, with a soft pulse of light." },
   { 31514, "A burst of static and digital noise. A distorted, mask-like shape flickers briefly before the screen clears." },
   { 31814, "Agumon, a small bipedal dinosaur Digimon with orange-yellow scales, large red eyes, and oversized clawed hands, stands in the shifting code. He raises his claws toward his face, alarmed, then turns sharply." },
   { 32564, "The scene cuts to a dark, rocky landscape under a dramatic, storm-lit sky." },
   { 32714, "Garurumon, a large blue-striped wolf Digimon with piercing eyes and sharp fangs, dominates the frame. He growls and bares his teeth as the camera pulls back." },
   { 33164, "Two more Digimon appear in quick cuts. SkullGreymon snarls with a black skull-like head, red and yellow bone accents, and heavy armored limbs. Then a blue-and-white horned Digimon fills the frame." },
   { 33764, "The screen tears apart in digital static and fragmented noise." },
   { 34064, "A title card appears over scrolling binary code: the Digimon crest above DIGIMON WORLD 2 in bold stylized lettering." },
   { 34514, "Under a bright blue sky, Machinedramon, a colossal blue-and-yellow robotic dragon Digimon with rounded red eyes and heavy cannon-barrel arms, towers into frame." },
   { 34814, "Agumon sits perched atop Machinedramon's head, looking out." },
   { 35114, "The camera pans across a vivid purple-and-blue sky. Several small dark silhouettes wheel and soar overhead in formation." },
   { 35564, "On a rocky outcrop, Piedmon, a humanoid Digimon in ornate dark armor with a white clown-mask face and red cape, stands with MetalSeadramon and Garurumon." },
   { 36074, "Digital noise and static wash over the screen. A metallic jaw-like structure breaks apart, then a mechanical lever flashes in close-up." },
   { 36614, "A green radar screen appears, a circular grid like sonar. Fiery and winged digital outlines pulse across it." },
   { 37064, "Binary code streams down a dark background. A spectral red dragon-like Digimon glows within it, outlined in circuit-board patterns." },
   { 37664, "The camera moves through dark, rocky terrain. A massive clawed foot hits the ground, followed by a blurred rush of creatures in motion." },
   { 38264, "Three figures stand on a rocky rise surrounded by floating islands and dark clouds: Piedmon, a yellow-and-orange metallic dragon Digimon, and the blue wolf." },
   { 38714, "The camera closes in on the wolf's face, intense eyes and jaw set as rocks scatter past." },
   { 39014, "Cut to SkullGreymon. Fiery red and yellow bone crests blaze from its head and shoulders." },
   { 39314, "The blue-and-white horned Digimon fills the frame, red eyes wide and teeth bared." },
   { 39614, "All three explode into motion. The metallic dragon lunges, SkullGreymon charges through debris, and the wolf leaps." },
   { 39914, "WarGreymon, a yellow-and-orange armored dragon Digimon with dark plating and a great horned helmet, hovers amid floating rocks." },
   { 40064, "Piedmon spreads his arms wide, pink energy crackling at his feet as he hovers. Then he vanishes in a blur." },
   { 40514, "MetalGarurumon, a sleek white-and-blue armored wolf Digimon with spiked metallic joints and sharp claws, flexes and moves with purpose. A red-and-yellow bird Digimon appears beside it." },
   { 40964, "The bird Digimon is engulfed in blazing blue-white light and disappears. A serpentine Digimon with a metallic hexagonal body stares straight at the camera." },
   { 41264, "A wall of brilliant light overwhelms the scene. A blue-and-yellow robotic Digimon emerges from the brightness." },
   { 41564, "Agumon appears, eyes wide and fearful." },
   { 41714, "WarGreymon lunges forward. MetalGarurumon drops into a defensive stance. They collide in a clash of force and light." },
   { 42314, "The blue-and-yellow armored Digimon strikes a bold pose, then turns to face Magnamon, a golden-armored Digimon with broad metallic wings and a red chest crest." },
   { 42764, "MetalGarurumon and Magnamon square off. MetalGarurumon crouches low, Magnamon spreads its arms, and purple energy erupts as they lunge." },
   { 43364, "WereGarurumon, a dark blue-and-grey upright wolf Digimon with golden accents and powerful limbs, stands in swirling blue energy." },
   { 43664, "WarGreymon opens its jaws and unleashes a torrent of fire." },
   { 43814, "A blue-and-white shark-like Digimon thrusts into frame, snarling with red eyes and serrated teeth." },
   { 44114, "A red, white, and yellow bird Digimon perches on a pale structure, glancing sideways. Then WarGreymon opens its jaws wide." },
   { 44414, "Rapid close-ups: a gleaming red eye, the shark Digimon's snarling mouth, WarGreymon's enormous teeth, and WereGarurumon growling." },
   { 45164, "A full confrontation. Multiple Digimon face off, with the blue shark Digimon and MetalGarurumon prominent in the foreground while others fan out behind them." },
   { 45614, "A flash of brilliant white light, and the DIGIMON WORLD 2 title logo burns onto the screen." },
   { 45764, "The image dissolves into television static, grainy and buzzing." },
   { 46064, "Fade to black." },
};

const char *beetle_accessibility_dw2_message_lookup(uint16_t file_index,
      uint32_t offset)
{
   size_t i;
   for (i = 0; i < sizeof(dw2_messages) / sizeof(dw2_messages[0]); i++)
   {
      if (dw2_messages[i].file_index == file_index
            && dw2_messages[i].offset == offset)
         return dw2_messages[i].text;
   }
   return NULL;
}

const char *beetle_accessibility_dw2_movie_cue_lookup(uint32_t disc_sector)
{
   size_t i;
   for (i = 0; i < sizeof(dw2_movie_cues) / sizeof(dw2_movie_cues[0]); i++)
   {
      if (dw2_movie_cues[i].disc_sector == disc_sector)
         return dw2_movie_cues[i].text;
   }
   return NULL;
}

size_t beetle_accessibility_dw2_message_count(void)
{
   return sizeof(dw2_messages) / sizeof(dw2_messages[0]);
}
