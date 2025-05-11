#include "UI.h"
#include <iostream>
#include "utils/logging.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    DisableThreadLibraryCalls(hinstDLL);
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        UI::hCurrentModule = hinstDLL;
#ifdef _DEBUG
        L::AttachConsole(TEXT("The Cooler, Debug YYC Toolbox Console"));
#endif
        UI::Render();
    }

    return TRUE;
}