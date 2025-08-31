#include "a11y.h"
#include <windows.h>

namespace A11y {

static BOOL (WINAPI *pTolk_Load)() = NULL;
static VOID (WINAPI *pTolk_Unload)() = NULL;
static BOOL (WINAPI *pTolk_Output)(LPCWSTR, BOOL) = NULL;
static HMODULE hTolk = NULL;
static BOOL g_loadedOK = FALSE;
static bool g_enabled = true;

static __declspec(noinline) BOOL call_load() {
    __try { return pTolk_Load ? pTolk_Load() : FALSE; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}
static __declspec(noinline) BOOL call_output(LPCWSTR txt, BOOL intr) {
    __try { return pTolk_Output ? pTolk_Output(txt, intr) : FALSE; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

TolkApi& api();
bool& enabledFlag();
void setEnabled(bool on);

void init() {
    if (!g_enabled || hTolk) return;
    wchar_t path[MAX_PATH]; path[0] = L'\0';
    HMODULE hMod = GetModuleHandleW(NULL);
    if (GetModuleFileNameW(hMod, path, MAX_PATH)) {
        std::wstring p(path);
        size_t pos = p.find_last_of(L"\\/");
        if (pos != std::wstring::npos) p = p.substr(0, pos + 1);
        p += L"tolk.dll";
        hTolk = LoadLibraryW(p.c_str());
    }
    if (!hTolk) hTolk = LoadLibraryW(L"tolk.dll");
    if (!hTolk) { setEnabled(false); return; }

    pTolk_Load   = (BOOL (WINAPI*)())GetProcAddress(hTolk, "Tolk_Load");
    pTolk_Unload = (VOID (WINAPI*)())GetProcAddress(hTolk, "Tolk_Unload");
    pTolk_Output = (BOOL (WINAPI*)(LPCWSTR, BOOL))GetProcAddress(hTolk, "Tolk_Output");
    if (!(pTolk_Load && pTolk_Output)) { setEnabled(false); return; }

    g_loadedOK = call_load();
    if (!g_loadedOK) setEnabled(false);
}

void shutdown() {
    if (pTolk_Unload) {
        __try { pTolk_Unload(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    }
    if (hTolk) FreeLibrary(hTolk);
    hTolk = NULL; pTolk_Load = NULL; pTolk_Unload = NULL; pTolk_Output = NULL; g_loadedOK = FALSE;
}

void speak(const std::wstring &text, bool interrupt) {
    if (!g_enabled) return;
    if (!hTolk || !g_loadedOK) init();
    if (!(hTolk && pTolk_Output && g_loadedOK) || text.empty()) return;
    if (!call_output(text.c_str(), interrupt ? TRUE : FALSE)) setEnabled(false);
}

} // namespace A11y
