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
	constexpr const char* PATH_SHADER_MENU = "assets/shaders/menu.frag";
	constexpr const char* PATH_SHADER_GAMEOVER = "assets/shaders/gameover.frag";

	//--------ПРЕДМЕТИ / ІКОНКИ ІНВЕНТАРЮ
	constexpr const char* PATH_PNG_ICON_ROOM_KEY = "assets/items/RoomKey.png";
	constexpr const char* PATH_PNG_ICON_SAFE_CARD = "assets/items/SafeKeyCard.png";
	constexpr const char* PATH_PNG_ICON_MASTER_KEY = "assets/items/MasterKey.png";

	// Конфігурація кроків
	constexpr const char* PATH_STEP_WOOD_1 = "assets/sfx/stp1.wav";
	constexpr const char* PATH_STEP_WOOD_2 = "assets/sfx/stp2.wav";
	constexpr const char* PATH_STEP_WOOD_3 = "assets/sfx/stp3.wav";
	constexpr const char* PATH_STEP_WOOD_4 = "assets/sfx/stp4.wav";

	//--------ГРАВЕЦЬ
	constexpr const char* PATH_PNG_PLAYER_SPRITESHEET = "assets/player/PlayerAnimations.png";
	constexpr const char* PATH_PNG_CARD_LIVES = "assets/player/LifesAce.png";
	constexpr const char* PATH_SFX_CARD_RIP = "assets/sfx/PaperRip.wav";
	constexpr const char* PATH_AUDIO_AMBIENT = "assets/music/ambient_1.wav";
	constexpr const char* PATH_SHADER_LIGHTING = "assets/shaders/lighting.frag";


	//--------ОХОРОНЕЦЬ (NPC)
	constexpr const char* PATH_AUDIO_GUARD_CHASE = "assets/music/ChaseSFX.wav";
	constexpr const char* PATH_AUDIO_GUARD_BREATH = "assets/Guard/Guard_Breathe.wav";



	constexpr const char* PATH_MAP = "assets/map/CasinoMap.tmj"; // мапа в tmj

	// усі тайлсети..
	constexpr const char* PATH_PNG_FLOOR = "assets/map/png/tiles/floor.png";
	constexpr const char* PATH_PNG_RUGS = "assets/map/png/tiles/rugs.png";
	constexpr const char* PATH_PNG_WALLS_1 = "assets/map/png/tiles/walls.png";
	constexpr const char* PATH_PNG_WALLS_2 = "assets/map/png/tiles/walls1.png";

}