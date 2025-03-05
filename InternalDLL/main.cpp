#include "UI.h"
#include <iostream>
#include "hooking/hooks.h"
#include "utils/logging.h"
#include "utils/hwid.h"
#include "dependencies/httplib.h"
#include "dependencies/memory.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    DisableThreadLibraryCalls(hinstDLL);
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        UI::hCurrentModule = hinstDLL;
        UI::Render();
    }

    return TRUE;
}