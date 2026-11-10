#pragma once
#include <string>

namespace Config {
// Далі додавати так: constexpr const char* PATH_MENUBG = "assets/MainMenu.png";
// constexpr const char* - тип для всього (не помню чому такий але воно працює
// PATH_MENUBG - код для шляху (завжди кабсом, PATH_НАЗВАФАЙЛУКОРОТКО)
// = "assets/MainMenu.png"; - адреса, бажано файли називати теж зрозуміло

	// ШАБЛОН ДЛЯ ГОЛОВНОГО МЕНЮ
	constexpr const char* PATH_PNG_MENU_BG = "assets/menu/MainMenuBG.png";
	constexpr const char* PATH_AUDIO_MENU_MUSIC = "assets/menu/MenuTheme.wav";
	//------------------------------------------
	// Конфігурація кроків
	constexpr const char* PATH_STEP_WOOD_1 = "assets/sfx/stp1.wav";
	constexpr const char* PATH_STEP_WOOD_2 = "assets/sfx/stp2.wav";
	constexpr const char* PATH_STEP_WOOD_3 = "assets/sfx/stp3.wav";
	constexpr const char* PATH_STEP_WOOD_4 = "assets/sfx/stp4.wav";


	//--------ГРАВЕЦЬ
	constexpr const char* PATH_PNG_PLAYER_SPRITESHEET = "assets/player/PlayerAnimations.png";
	constexpr const char* PATH_PNG_CARD_LIVES = "assets/player/LifesAce.png";
	constexpr const char* PATH_AUDIO_AMBIENT = "assets/music/ambient_1.wav";
	constexpr const char* PATH_MAP = "map/Casino oop/casino_map_3232.tmj"; // мапа в tmj
	constexpr const char* PATH_SHADER_LIGHTING = "assets/shaders/lighting.frag";
	// усі тайлсети..
	constexpr const char* PATH_PNG_OTHER_OBJECTS = "map/CASINO TILES SET/Objects_devided/32x32/1 other objects.png";
	constexpr const char* PATH_PNG_FLOOR = "map/CASINO TILES SET/Floor/1_FLOOR.png";
	constexpr const char* PATH_PNG_RUGS = "map/CASINO TILES SET/Floor/1_RUGS.png";
	constexpr const char* PATH_PNG_PLAYING_TABLES = "map/CASINO TILES SET/Objects_devided/32x32/Playing tables.png";
	constexpr const char* PATH_PNG_SLOT_MACHINES = "map/CASINO TILES SET/Objects_devided/32x32/Slot_machines.png";
	constexpr const char* PATH_PNG_WALLS_1 = "map/CASINO TILES SET/Walls_32x32/WALLS 1 32x32.png";
	constexpr const char* PATH_PNG_WALLS_2 = "map/CASINO TILES SET/Walls_32x32/WALLS 2 32x32.png";
	constexpr const char* PATH_PNG_SLOT_MACHINE_SIDE = "map/CASINO TILES SET/Objects_devided/96x96/Slot machine side.png";
	constexpr const char* PATH_PNG_TABLES_1 = "map/CASINO TILES SET/Objects_devided/32x32/Tables 1.png";
	constexpr const char* PATH_PNG_COUCHES_1 = "map/CASINO TILES SET/Objects_devided/32x32/Couches 1.png";

}