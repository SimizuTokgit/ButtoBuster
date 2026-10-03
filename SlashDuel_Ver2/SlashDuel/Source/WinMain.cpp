#include <Windows.h>

#include "System.h"
#include "SceneManager.h"
#include "TitleScene.h"

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    System system;

    // 最初のシーンはここで決める
    bool result = system.Main([]() {
        return SceneManager::Instance().LoadScene<TitleScene>();
    });

    return result ? 0 : -1;
}
