#include "GameManager.h"

int main() {
    GameManager::Instance().Init();
    GameManager::Instance().Run();
    GameManager::Instance().Shutdown();
    return 0;
}