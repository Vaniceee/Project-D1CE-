#include "GameManager.h"

void ExportProjectCode();

int main() {
    ExportProjectCode();
    GameManager::Instance().Init();
    GameManager::Instance().Run();
    GameManager::Instance().Shutdown();
    return 0;
}