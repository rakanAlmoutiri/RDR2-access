// Runtime loader for Tolk.dll to speak captions via NVDA/JAWS if available.
#pragma once
#include <windows.h>
#include <string>

namespace A11y {

struct TolkApi {
    // Match Tolk's exported signatures: BOOL WINAPI Tolk_Load(void), VOID WINAPI Tolk_Unload(void), BOOL WINAPI Tolk_Output(LPCWSTR, BOOL)
    typedef BOOL (WINAPI *PFN_Load)();
    typedef VOID (WINAPI *PFN_Unload)();
    typedef BOOL (WINAPI *PFN_Output)(LPCWSTR, BOOL);

    HMODULE dll;
    PFN_Load Load;
    PFN_Unload Unload;
    PFN_Output Output;
    BOOL loadedOK;

    TolkApi() : dll(NULL), Load(NULL), Unload(NULL), Output(NULL), loadedOK(FALSE) {}
    bool available() const { return dll && Load && Unload && Output; }
};

inline TolkApi& api() { static TolkApi inst; return inst; }

inline bool& enabledFlag() { static bool flag = true; return flag; }
inline void setEnabled(bool on) { enabledFlag() = on; }
inline bool isEnabled() { return enabledFlag(); }

inline bool isReady() {
    TolkApi &t = api();
    return isEnabled() && t.dll && t.Load && t.Unload && t.Output && t.loadedOK;
}

    // Implemented in a11y.cpp to keep SEH out of headers
    void init();
    void shutdown();
    void speak(const std::wstring &text, bool interrupt = true);
    void playCustomSound(const wchar_t* filename);

} // namespace A11y
