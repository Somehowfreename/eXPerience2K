#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <shellapi.h>
#include <stdlib.h>

/* Windows 2000's standard.htt uses the legacy WMP ActiveX control. Clean
   XP x64 provides this control only to 32-bit clients. This small, separate
   process embeds the actual control in either native Explorer architecture,
   without replacing Explorer or loading 32-bit code into a 64-bit process. */
static HWND g_parent, g_host, g_player;
static DWORD g_parent_pid;
static HANDLE g_stop;
static IDispatch *g_dispatch;

static HRESULT dispatch_name(const wchar_t *name, DISPID *identifier)
{
    LPOLESTR mutable_name = const_cast<LPOLESTR>(name);
    if (!g_dispatch) return E_UNEXPECTED;
    return g_dispatch->GetIDsOfNames(IID_NULL, &mutable_name, 1,
                                     LOCALE_USER_DEFAULT, identifier);
}

static HRESULT put_bool(const wchar_t *name, BOOL value)
{
    DISPID identifier, property_put = DISPID_PROPERTYPUT;
    VARIANT argument;
    DISPPARAMS parameters;
    HRESULT result = dispatch_name(name, &identifier);
    if (FAILED(result)) return result;
    VariantInit(&argument);
    argument.vt = VT_BOOL;
    argument.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    parameters.rgvarg = &argument;
    parameters.rgdispidNamedArgs = &property_put;
    parameters.cArgs = 1;
    parameters.cNamedArgs = 1;
    return g_dispatch->Invoke(identifier, IID_NULL, LOCALE_USER_DEFAULT,
                               DISPATCH_PROPERTYPUT, &parameters, NULL, NULL, NULL);
}

static HRESULT open_media(const wchar_t *path)
{
    DISPID identifier;
    VARIANT argument;
    DISPPARAMS parameters;
    HRESULT result = dispatch_name(L"Open", &identifier);
    if (FAILED(result)) return result;
    VariantInit(&argument);
    argument.vt = VT_BSTR;
    argument.bstrVal = SysAllocString(path);
    if (!argument.bstrVal) return E_OUTOFMEMORY;
    parameters.rgvarg = &argument;
    parameters.rgdispidNamedArgs = NULL;
    parameters.cArgs = 1;
    parameters.cNamedArgs = 0;
    result = g_dispatch->Invoke(identifier, IID_NULL, LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD, &parameters, NULL, NULL, NULL);
    VariantClear(&argument);
    return result;
}

static void stop_media()
{
    DISPID identifier;
    DISPPARAMS parameters = {NULL, NULL, 0, 0};
    if (SUCCEEDED(dispatch_name(L"Stop", &identifier)))
        g_dispatch->Invoke(identifier, IID_NULL, LOCALE_USER_DEFAULT,
                             DISPATCH_METHOD, &parameters, NULL, NULL, NULL);
}

static BOOL parent_alive()
{
    wchar_t name[80];
    DWORD pid = 0;
    return IsWindow(g_parent) &&
           GetWindowThreadProcessId(g_parent, &pid) && pid == g_parent_pid &&
           GetClassNameW(g_parent, name, 80) &&
           lstrcmpW(name, L"eXPerience2K.Windows2000ExplorerPane") == 0;
}

static LRESULT CALLBACK host_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_SIZE:
        if (g_player) MoveWindow(g_player, 0, 0, LOWORD(lparam), HIWORD(lparam), TRUE);
        return 0;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
    typedef BOOL (WINAPI *AtlAxWinInitProc)(void);
    typedef HRESULT (WINAPI *AtlAxGetControlProc)(HWND, IUnknown **);
    int count = 0, result = 1;
    wchar_t **arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments || count != 6) {
        if (arguments) LocalFree(arguments);
        return ERROR_INVALID_PARAMETER;
    }
    g_parent = (HWND)(ULONG_PTR)wcstoul(arguments[1], NULL, 10);
    g_parent_pid = wcstoul(arguments[2], NULL, 10);
    BOOL sound = lstrcmpW(arguments[3], L"sound") == 0;
    DWORD attributes = GetFileAttributesW(arguments[5]);
    if ((!sound && lstrcmpW(arguments[3], L"movie") != 0) ||
        !parent_alive() || attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        LocalFree(arguments);
        return ERROR_INVALID_PARAMETER;
    }
    g_stop = OpenEventW(SYNCHRONIZE, FALSE, arguments[4]);
    if (!g_stop || WaitForSingleObject(g_stop, 0) != WAIT_TIMEOUT) {
        if (g_stop) CloseHandle(g_stop);
        LocalFree(arguments);
        return ERROR_CANCELLED;
    }
    HRESULT initialized = OleInitialize(NULL);
    if (FAILED(initialized)) {
        CloseHandle(g_stop);
        LocalFree(arguments);
        return 2;
    }
    /* Load only the system ATL, never a DLL found in the selected folder. */
    wchar_t atl_path[MAX_PATH];
    HMODULE atl = NULL;
    UINT length = GetSystemDirectoryW(atl_path, MAX_PATH);
    if (length && length < MAX_PATH - 9) {
        lstrcatW(atl_path, L"\\atl.dll");
        atl = LoadLibraryW(atl_path);
    }
    AtlAxWinInitProc initialize = NULL;
    AtlAxGetControlProc get_control = NULL;
    if (atl) {
        FARPROC procedure = GetProcAddress(atl, "AtlAxWinInit");
        CopyMemory(&initialize, &procedure, sizeof(initialize));
        procedure = GetProcAddress(atl, "AtlAxGetControl");
        CopyMemory(&get_control, &procedure, sizeof(get_control));
    }
    if (initialize && get_control && initialize()) {
        WNDCLASSW definition;
        ZeroMemory(&definition, sizeof(definition));
        definition.lpfnWndProc = host_proc;
        definition.hInstance = instance;
        definition.hCursor = LoadCursorW(NULL, IDC_ARROW);
        definition.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        definition.lpszClassName = L"eXPerience2K.MediaPreviewHost";
        if (RegisterClassW(&definition)) {
            int height = sound ? 46 : 136;
            g_host = CreateWindowExW(WS_EX_NOPARENTNOTIFY, definition.lpszClassName,
                L"Media preview", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                12, 184, 176, height, g_parent, NULL, instance, NULL);
            if (g_host && parent_alive() && WaitForSingleObject(g_stop, 0) == WAIT_TIMEOUT) {
                g_player = CreateWindowExW(0, L"AtlAxWin", L"MediaPlayer.MediaPlayer.1",
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 176, height,
                    g_host, NULL, instance, NULL);
                IUnknown *control = NULL;
                if (g_player && SUCCEEDED(get_control(g_player, &control)) && control) {
                    control->QueryInterface(IID_IDispatch, (void **)&g_dispatch);
                    control->Release();
                }
                if (g_dispatch) {
                    put_bool(L"ShowDisplay", FALSE);
                    put_bool(L"AutoPlay", FALSE);
                    put_bool(L"AutoStart", FALSE);
                    put_bool(L"EnableContextMenu", FALSE);
                    put_bool(L"SendErrorEvents", TRUE);
                    if (SUCCEEDED(open_media(arguments[5]))) {
                        result = 0;
                        BOOL running = TRUE;
                        while (running && parent_alive()) {
                            DWORD wait = MsgWaitForMultipleObjects(1, &g_stop, FALSE, 250, QS_ALLINPUT);
                            if (wait == WAIT_OBJECT_0 || wait == WAIT_FAILED) break;
                            MSG message;
                            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                                if (message.message == WM_QUIT) { running = FALSE; break; }
                                TranslateMessage(&message);
                                DispatchMessageW(&message);
                            }
                        }
                    }
                }
            }
        }
    }
    stop_media();
    if (g_dispatch) g_dispatch->Release();
    if (IsWindow(g_host)) DestroyWindow(g_host);
    if (atl) FreeLibrary(atl);
    OleUninitialize();
    CloseHandle(g_stop);
    LocalFree(arguments);
    return result;
}
