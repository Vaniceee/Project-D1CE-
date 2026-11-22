#pragma once

#include "GameState.h"
#include "VictoryUI.h"

class VictoryState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    VictoryUI ui;
};