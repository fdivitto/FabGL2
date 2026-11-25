
/* configuration given by filename:
      "PROGNAME [EXP] [ADDRESS].EXT"
        PROGNAME : program name (not included extension)
        EXP      : expansion ("3K", "8K", "16K", "24K", "27K", "32K", "35K")
        ADDRESS  : hex address ("20"=0x2000, "40"=0x4000, "60"=0x6000 or "A0"=0xA000), "A0" or automatic is the default
        EXT      : "CRT" or "PRG"
*/
static char const * const WEB_LIST[] = {

    "Battlezone 60.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Battlezone-6000.prg",

    "Battlezone A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Battlezone-a000.prg",

    "Blitz.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games.basic/unexpanded/Vic%20Blitz.prg",

    "Abductor.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Abductor.prg",

    "Aggressor.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Aggressor.prg",

    "Alien Attack.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Alien%20Attack.prg",

    "Alien Blitz.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Alien%20Blitz.PAL.prg",

    "Amok.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/4k/Amok!.prg",

    "Apple Panic 60.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Apple%20Panic-6000.prg",

    "Apple Panic A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Apple%20Panic-a000.prg",

    "Arcadia A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Arcadia.prg",

    "Attack Mut. Cam.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Attack%20of%20the%20Mutant%20Camels.prg",

    "Avenger.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Avenger.prg",

    "Blockbuster.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Blockbuster.prg",

    "Bomber.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Bomber.prg",

    "Breakout.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games.basic/unexpanded/Break%20Out%204.prg",

    "Centipede 20.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Centipede-2000.prg",

    "Centipede A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Centipede-a000.prg",

    "Choplifter.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Choplifter.prg",

    "Cosmic Firebirds 16K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/8k/Cosmic%20Firebirds.prg",

    "Crossfire.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Crossfire.PAL.prg",

    "Cyclon.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Cyclon.prg",

    "D'Fuse.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/D'Fuse.prg",

    "Demon Attack.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/4k/Demon%20Attack.PAL.prg",

    "Donkey Kong 20.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Donkey%20Kong-2000.prg",

    "Donkey Kong A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Donkey%20Kong-a000.prg",

    "Dragonfire.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Dragonfire.prg",

    "Frogger.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Frogger.prg",

    "Galaxian.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Galaxian.prg",

    "Garden Wars.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Garden%20Wars.prg",

    "Jelly Monsters.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Jelly%20Monsters.prg",

    "Jupiter Lander.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Jupiter%20Lander.prg",

    "Key Quest.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/KeyQuest.prg",

    "Lazerzone.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Lazerzone.prg",

    "Matrix 8K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/8k/Matrix.prg",

    "Miner 2049.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Miner%202049'er.prg",

    "Mole Attack.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Mole%20Attack.prg",

    "Omega Race.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Omega%20Race.prg",

    "Outworld.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Outworld.prg",

    "PacMan.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Pac-Man.prg",

    "Polaris.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Polaris.prg",

    "Pole Position 60.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Pole%20Position-6000.prg",

    "Pole Position A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/16k/Pole%20Position-a000.prg",

    "Protector.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Protector.prg",

    "Quackers.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Quackers.prg",

    "River Rescue.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/River%20Rescue.prg",

    "Robot Panic.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Robot%20Panic.prg",

    "Satellite Patrol.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/4k/Satellite%20Patrol.prg",

    "Satellites & Met.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Satellites%20and%20Meteorites.PAL.prg",

    "Scorpion.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Scorpion.prg",

    "Serpentine.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Serpentine.PAL.prg",

    "Shamus.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Shamus.prg",

    "Spiders Mars 60.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Spiders%20of%20Mars-6000.prg",

    "Spiders Mars A0.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Spiders%20of%20Mars-a000.prg",

    "Star Battle.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Star%20Battle.prg",

    "Star Trek SOS.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Star%20Trek%20-%20S.O.S.prg",

    "Sub Chase.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/4k/Sub%20Chase.prg",

    "Super Amok.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Super%20Amok.prg",

    "Super Smash.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Super%20Smash.prg",

    "Terraguard.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/4k/Terraguard.prg",

    "Threshold.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/8k/Threshold.prg",

    "Tom 16K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/16k/Tom.prg",

    "Tron 8K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/8k/Tron.prg",

    "The Catch.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/The%20Catch.prg",

    "Orbis.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Orbis.prg",

    "Fantasia.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Fantasia.prg",

    "Practicalc 16K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/utilities/16k/Practicalc.prg",

    "Screen40 8K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/utilities/8k/Screen-40.prg",

    "WaterlooBASIC.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/tools/4k/WaterlooBASIC.prg",

    "VICMON 60.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/tools/4k/Vicmon-6000.prg",

    "Super Expander 27K.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/tools/4k/Super%20Expander.prg",

    "Vic Forth.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/tools/8k/Vic%20Forth%201.10.prg",

    "Mikro Assem. 60 27K.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/tools/8k/Mikro%20Assembler-6000.prg",

    "Mikro Assem. A0 27K.CRT",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/roms/tools/8k/Mikro%20Assembler-a000.prg",

    "Tower Of Evil 8K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/8k/Tower%20of%20Evil.prg",

    "Thunder Flash.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Thunder%20Flash.prg",

    "Space Phreaks.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/unexpanded/Space%20Phreaks.prg",

    "Skramble 16K.PRG",
    "http://www.zimmers.net/anonftp/pub/cbm/vic20/games/16k/Skramble.prg",

    "Road Race A0.CRT",
    "https://www.fabglib.org/downloads/vic20/Road%20Race.crt",

    "Car Race.PRG",
    "https://www.fabglib.org/downloads/vic20/Car%20Race%20Commodore.prg",

    "Wall Defence.PRG",
    "https://www.fabglib.org/downloads/vic20/Wall%20Defense.prg",

    "Perilous Cav. 16K.PRG",
    "https://www.fabglib.org/downloads/vic20/Perilous%20Cavern%2016k.prg",

    "Meteor Run 8K.PRG",
    "https://www.fabglib.org/downloads/vic20/Meteor%20Run%208K.prg",

    "Hellgate 8K.PRG",
    "https://www.fabglib.org/downloads/vic20/Hellgate%208K.prg",

    "Gladiator.PRG",
    "https://www.fabglib.org/downloads/vic20/Gladiator.prg",

    "Flowers Evil 8K.PRG",
    "https://www.fabglib.org/downloads/vic20/Flowers%20of%20Evil.prg",

    "Fire Galaxy 16K.PRG",
    "https://www.fabglib.org/downloads/vic20/Fire%20Galaxy%2016K.prg",

    "Falcons Land 8K.PRG",
    "https://www.fabglib.org/downloads/vic20/Falcons%20Land%208K.prg",

    "Defender.PRG",
    "https://www.fabglib.org/downloads/vic20/Defender.prg",

    "Cube Bert 16K.PRG",
    "https://www.fabglib.org/downloads/vic20/Cube%20Bert%2016K.prg",

    "Witch Way A0.CRT",
    "https://www.fabglib.org/downloads/vic20/Witch%20Way%201983%20CDS%20NTSC%20A000.crt",

};


