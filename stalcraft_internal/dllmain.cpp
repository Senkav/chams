#include "hooks/hooks.hpp"
#include <windows.h>

// ── Глобальные настройки ──────────────────────────────────────────────────────
bool  g_chams_enabled = true;
bool  g_boxes_enabled = true;
float g_chams_color[3] = { 1.f, 0.f, 0.f }; // красный по умолчанию

// Горячие клавиши:
// INSERT — вкл/выкл чамсы
// F1 — красный | F2 — зелёный | F3 — синий | F4 — белый
// F5 — вкл/выкл 3D боксы

static DWORD WINAPI hotkeysThread(LPVOID) {
    bool wasInsert=false, wasF1=false, wasF2=false, wasF3=false, wasF4=false, wasF5=false;
    while (true) {
        Sleep(50);
        #define CHECK(key, was, action) { bool now=(GetAsyncKeyState(key)&0x8000)!=0; if(now&&!was){action;} was=now; }
        CHECK(VK_INSERT, wasInsert, g_chams_enabled = !g_chams_enabled)
        CHECK(VK_F1, wasF1, (g_chams_color[0]=1,g_chams_color[1]=0,g_chams_color[2]=0))
        CHECK(VK_F2, wasF2, (g_chams_color[0]=0,g_chams_color[1]=1,g_chams_color[2]=0))
        CHECK(VK_F3, wasF3, (g_chams_color[0]=0,g_chams_color[1]=0,g_chams_color[2]=1))
        CHECK(VK_F4, wasF4, (g_chams_color[0]=1,g_chams_color[1]=1,g_chams_color[2]=1))
        CHECK(VK_F5, wasF5, g_boxes_enabled = !g_boxes_enabled)
        #undef CHECK
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle_module, DWORD ul_reason_for_call, LPVOID reserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(handle_module);
            hooks::initialize();
            CreateThread(nullptr, 0, hotkeysThread, nullptr, 0, nullptr);
            break;
        }
    }
    return TRUE;
}
