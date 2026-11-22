#include "VictoryState.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "GameManager.h"
#include "raylib.h"

void VictoryState::Init() {
    ui.Init();
}

void VictoryState::Cleanup() {
    ui.Cleanup();
}

void VictoryState::Update() {
    float dt = GetFrameTime();
    VictoryAction action = ui.Update(dt);

    if (action == VictoryAction::PLAY_AGAIN) {
        GameManager::Instance().ChangeState(new GameplayState());
    }
    else if (action == VictoryAction::MAIN_MENU) {
        GameManager::Instance().ChangeState(new MainMenuState());
    }
}

void VictoryState::Draw() {
    ui.Draw();
}