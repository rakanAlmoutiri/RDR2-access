#include "a11y.h"
#include <windows.h>
#include <queue>
#include <thread>

namespace A11y {

static BOOL (WINAPI *pTolk_Load)() = NULL;
static VOID (WINAPI *pTolk_Unload)() = NULL;
static BOOL (WINAPI *pTolk_Output)(LPCWSTR, BOOL) = NULL;
static HMODULE hTolk = NULL;
static BOOL g_loadedOK = FALSE;

// Thread safety and performance optimization
static CRITICAL_SECTION g_speakLock;
static bool g_lockInitialized = false;
static std::wstring g_lastSpokenText;  // Cache to avoid repeated speech
static DWORD g_lastSpeakTime = 0;
static const DWORD MIN_SPEAK_INTERVAL = 100;  // Minimum 100ms between speaks

// Async queue system for NVDA speech (guarantees no main-thread blocking)
static std::queue<std::pair<std::wstring, BOOL>> g_speakQueue;
static CRITICAL_SECTION g_queueLock;
static HANDLE g_queueEvent = NULL;
static bool g_threadRunning = false;
static std::thread* g_speakWorkerThread = nullptr;

// Avoid SEH in Release builds (C2712). We assume Tolk.dll behaves and simply
// call through when function pointers are valid.
static __declspec(noinline) BOOL call_load() {
    return pTolk_Load ? pTolk_Load() : FALSE;
}
static __declspec(noinline) BOOL call_output(LPCWSTR txt, BOOL intr) {
    return pTolk_Output ? pTolk_Output(txt, intr) : FALSE;
}

void InitLock() {
    if (!g_lockInitialized) {
        InitializeCriticalSection(&g_speakLock);
        g_lockInitialized = true;
    }
}

// Async worker thread: processes NVDA speech queue safely (NEVER blocks main thread)
static void SpeakWorkerThread() {
    while (g_threadRunning) {
        WaitForSingleObject(g_queueEvent, 100);  // Wait for queue signal or timeout
        
        EnterCriticalSection(&g_queueLock);
        while (!g_speakQueue.empty()) {
            auto item = g_speakQueue.front();
            g_speakQueue.pop();
            std::wstring text = item.first;
            BOOL interrupt = item.second;
            LeaveCriticalSection(&g_queueLock);
            
            // CRITICAL: NVDA calls happen ONLY in this worker thread, NEVER on main thread
            if (hTolk && pTolk_Output && g_loadedOK) {
                call_output(text.c_str(), interrupt);
            }
            
            EnterCriticalSection(&g_queueLock);
        }
        LeaveCriticalSection(&g_queueLock);
    }
}

TolkApi& api();
bool& enabledFlag();
void setEnabled(bool on);

void init() {
    if (!enabledFlag() || hTolk) return;
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
    if (!g_loadedOK) {
        setEnabled(false);
        return;
    }

    // Initialize queue and worker thread on first successful NVDA load
    if (!g_queueEvent) {
        InitializeCriticalSection(&g_queueLock);
        g_queueEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        g_threadRunning = true;
        g_speakWorkerThread = new std::thread(SpeakWorkerThread);
        g_speakWorkerThread->detach();
    }
}

void shutdown() {
    g_threadRunning = false;
    if (g_queueEvent) {
        SetEvent(g_queueEvent);
        Sleep(500);  // Give worker thread time to finish
        CloseHandle(g_queueEvent);
        g_queueEvent = NULL;
    }
    if (g_speakWorkerThread) {
        delete g_speakWorkerThread;
        g_speakWorkerThread = nullptr;
    }
    if (pTolk_Unload) {
        pTolk_Unload();
    }
    if (hTolk) FreeLibrary(hTolk);
    hTolk = NULL; 
    pTolk_Load = NULL; 
    pTolk_Unload = NULL; 
    pTolk_Output = NULL; 
    g_loadedOK = FALSE;
    if (g_lockInitialized) {
        DeleteCriticalSection(&g_speakLock);
        g_lockInitialized = false;
    }
    DeleteCriticalSection(&g_queueLock);
}

void speak(const std::wstring &text, bool interrupt) {
    if (!enabledFlag()) return;
    if (!hTolk || !g_loadedOK) {
        init();
        if (!hTolk || !g_loadedOK) return;
    }
    if (text.empty()) return;
    
    // Cache check: don't queue same text twice in short succession
    DWORD now = GetTickCount();
    if (g_lastSpokenText == text && (now - g_lastSpeakTime) < MIN_SPEAK_INTERVAL) {
        return;
    }
    
    g_lastSpokenText = text;
    g_lastSpeakTime = now;
    
    // Queue the speak request (main thread only adds to queue, worker thread processes)
    EnterCriticalSection(&g_queueLock);
    g_speakQueue.push({text, interrupt ? TRUE : FALSE});
    LeaveCriticalSection(&g_queueLock);
    
    // Signal worker thread to process queue
    if (g_queueEvent) {
        SetEvent(g_queueEvent);
    }
}

} // namespace A11y
