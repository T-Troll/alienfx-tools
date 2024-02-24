#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

HANDLE guiEvent = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        guiEvent = OpenEvent(EVENT_MODIFY_STATE, false, "LightFXActive");
        if (guiEvent)
            SetEvent(guiEvent);
    } break;
    //case DLL_THREAD_ATTACH:
    //case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        if (guiEvent) {
            ResetEvent(guiEvent);
            CloseHandle(guiEvent);
        }
        break;
    }
    return TRUE;
}

