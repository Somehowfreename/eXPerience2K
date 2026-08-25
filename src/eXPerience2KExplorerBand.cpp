#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <olectl.h>
#include <shlobj.h>
#include <shlguid.h>
#include <exdisp.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stddef.h>

/*
 * Experimental native-x64 Windows 2000 Explorer pane.
 *
 * The DLL is deliberately a shell extension, not a replacement explorer.exe.
 * It leaves the XP x64 desktop shell, namespace implementation, context menus,
 * file operations, drag/drop, and every application-facing shell API native.
 * A BHO inserts a 200-pixel information pane inside each native Shell view,
 * leaving Explorer Bars (especially Win+E's Folders tree) independent.  The
 * pane uses the exact wvleft.bmp installed from the user's Windows 2000 media.
 */

static HINSTANCE g_instance;
static LONG g_objects;
static LONG g_locks;

/* Keep this NT 5.2 shell extension independent of the MinGW C++ runtime.
   The stock runtime imports post-XP APIs when it is statically linked. */
void *operator new(size_t size) throw()
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size ? size : 1);
}

void *operator new[](size_t size) throw()
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size ? size : 1);
}

void operator delete(void *memory) throw()
{
    if (memory) HeapFree(GetProcessHeap(), 0, memory);
}

void operator delete[](void *memory) throw()
{
    if (memory) HeapFree(GetProcessHeap(), 0, memory);
}

/* {6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64} */
static const CLSID CLSID_E2KExplorerBand =
    {0x6d638b73, 0x08f5, 0x4b6d, {0xa8, 0xcc, 0x5a, 0x7b, 0x31, 0xfc, 0x2a, 0x64}};
/* {7D298B9A-9BE0-48E9-9733-AD9A17EA6D20} */
static const CLSID CLSID_E2KExplorerHook =
    {0x7d298b9a, 0x9be0, 0x48e9, {0x97, 0x33, 0xad, 0x9a, 0x17, 0xea, 0x6d, 0x20}};

static const wchar_t BAND_CLSID_TEXT[] =
    L"{6D638B73-08F5-4B6D-A8CC-5A7B31FC2A64}";
static const wchar_t FOLDERS_CLSID_TEXT[] =
    L"{EFA24E64-B078-11D0-89E4-00C04FC9E26E}";
static const wchar_t PANE_CLASS[] = L"eXPerience2K.Windows2000ExplorerPane";
static const wchar_t HOOK_CLASS[] = L"eXPerience2K.Windows2000ExplorerHook";

enum ExplorerLinkTarget {
    LINK_NONE = 0,
    LINK_MY_DOCUMENTS,
    LINK_MY_COMPUTER,
    LINK_NETWORK_PLACES,
    LINK_CONNECTIONS,
    LINK_WINDOWS_UPDATE,
    LINK_WINDOWS_SUPPORT
};

static void safe_release(IUnknown *value)
{
    if (value) value->Release();
}

static BOOL location_path(IWebBrowser2 *browser, wchar_t *path, size_t count)
{
    BSTR url = NULL;
    DWORD chars = (DWORD)count;
    HRESULT result;
    if (!browser || !path || count == 0) return FALSE;
    path[0] = 0;
    result = browser->get_LocationURL(&url);
    if (FAILED(result) || !url) return FALSE;
    result = PathCreateFromUrlW(url, path, &chars, 0);
    SysFreeString(url);
    return SUCCEEDED(result) && path[0] != 0;
}

static void location_name(IWebBrowser2 *browser, wchar_t *name, size_t count)
{
    BSTR text = NULL;
    if (!name || count == 0) return;
    name[0] = 0;
    if (browser && SUCCEEDED(browser->get_LocationName(&text)) && text) {
        lstrcpynW(name, text, (int)count);
        SysFreeString(text);
    }
    if (!name[0]) lstrcpynW(name, L"Windows Explorer", (int)count);
}

static HICON location_icon(const wchar_t *title, const wchar_t *path)
{
    SHFILEINFOW file_info;
    ZeroMemory(&file_info, sizeof(file_info));
    if (path && path[0] &&
        SHGetFileInfoW(path, 0, &file_info, sizeof(file_info),
                       SHGFI_ICON | SHGFI_LARGEICON))
        return file_info.hIcon;

    int csidl = -1;
    if (lstrcmpiW(title, L"My Documents") == 0) csidl = CSIDL_PERSONAL;
    else if (lstrcmpiW(title, L"My Computer") == 0) csidl = CSIDL_DRIVES;
    else if (lstrcmpiW(title, L"My Network Places") == 0) csidl = CSIDL_NETHOOD;
    else if (lstrcmpiW(title, L"Control Panel") == 0) csidl = CSIDL_CONTROLS;
    else if (lstrcmpiW(title, L"Recycle Bin") == 0) csidl = CSIDL_BITBUCKET;
    if (csidl >= 0) {
        LPITEMIDLIST item = NULL;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &item)) && item) {
            if (!SHGetFileInfoW((LPCWSTR)item, 0, &file_info, sizeof(file_info),
                                SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON))
                file_info.hIcon = NULL;
            CoTaskMemFree(item);
            return file_info.hIcon;
        }
    }
    return CopyIcon(LoadIconW(NULL, IDI_APPLICATION));
}

static HWND find_descendant(HWND parent, const wchar_t *class_name)
{
    HWND child = NULL;
    wchar_t current[96];
    while ((child = FindWindowExW(parent, child, NULL, NULL)) != NULL) {
        current[0] = 0;
        GetClassNameW(child, current, (int)(sizeof(current) / sizeof(current[0])));
        if (lstrcmpiW(current, class_name) == 0) return child;
        HWND nested = find_descendant(child, class_name);
        if (nested) return nested;
    }
    return NULL;
}

static BOOL CALLBACK find_checked_folders_button(HWND window, LPARAM result_value)
{
    BOOL *found = (BOOL *)result_value;
    wchar_t class_name[96];
    class_name[0] = 0;
    GetClassNameW(window, class_name,
                  (int)(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, TOOLBARCLASSNAMEW) != 0) return TRUE;
    int count = (int)SendMessageW(window, TB_BUTTONCOUNT, 0, 0);
    for (int index = 0; index < count; ++index) {
        TBBUTTON button;
        ZeroMemory(&button, sizeof(button));
        if (!SendMessageW(window, TB_GETBUTTON, index, (LPARAM)&button)) continue;
        if (!(button.fsState & TBSTATE_CHECKED)) continue;
        wchar_t text[96];
        text[0] = 0;
        if (SendMessageW(window, TB_GETBUTTONTEXTW, button.idCommand,
                         (LPARAM)text) >= 0 && StrStrIW(text, L"Folders")) {
            *found = TRUE;
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL folders_mode_requested(HWND root)
{
    BOOL found = FALSE;
    if (root) EnumChildWindows(root, find_checked_folders_button, (LPARAM)&found);
    return found;
}

/* LVM_SETVIEW changes only the SysListView32 control.  XP's shell still
   records the namespace view as FVM_TILE, so its View menu keeps Tiles
   selected and recreates that mode on the next window.  Set the mode through
   IFolderView once per native shell view so the menu and ShellBag persist the
   same Icons choice that is rendered on screen. */
static BOOL initialize_shell_view_mode(IWebBrowser2 *browser)
{
    IServiceProvider *provider = NULL;
    IShellBrowser *shell_browser = NULL;
    IShellView *shell_view = NULL;
    IFolderView *folder_view = NULL;
    UINT mode = FVM_AUTO;
    HRESULT result;
    if (!browser ||
        FAILED(browser->QueryInterface(IID_IServiceProvider, (void **)&provider)))
        return FALSE;
    result = provider->QueryService(SID_STopLevelBrowser, IID_IShellBrowser,
                                    (void **)&shell_browser);
    provider->Release();
    if (FAILED(result) || !shell_browser) return FALSE;
    result = shell_browser->QueryActiveShellView(&shell_view);
    shell_browser->Release();
    if (FAILED(result) || !shell_view) return FALSE;
    result = shell_view->QueryInterface(IID_IFolderView, (void **)&folder_view);
    shell_view->Release();
    if (FAILED(result) || !folder_view) return FALSE;
    result = folder_view->GetCurrentViewMode(&mode);
    if (SUCCEEDED(result) && mode == FVM_TILE)
        result = folder_view->SetCurrentViewMode(FVM_ICON);
    folder_view->Release();
    return SUCCEEDED(result);
}

static BOOL selected_item(HWND pane, wchar_t *name, size_t count)
{
    HWND root, list;
    int index;
    if (!pane || !name || count == 0) return FALSE;
    name[0] = 0;
    root = GetAncestor(pane, GA_ROOT);
    list = find_descendant(root, WC_LISTVIEWW);
    if (!list) list = find_descendant(root, L"SysListView32");
    if (!list) return FALSE;
    index = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    if (index < 0) return FALSE;
    ListView_GetItemText(list, index, 0, name, (int)count);
    return name[0] != 0;
}

class ExplorerBand : public IDeskBand, public IObjectWithSite,
                     public IPersistStream, public IInputObject {
public:
    ExplorerBand() : refs_(1), site_(NULL), browser_(NULL), window_(NULL),
                     background_(NULL), normal_font_(NULL), title_font_(NULL),
                     bold_font_(NULL), link_font_(NULL), link_count_(0),
                     inline_mode_(FALSE), view_mode_initialized_(FALSE)
    {
        InterlockedIncrement(&g_objects);
    }

    virtual ~ExplorerBand()
    {
        detach();
        if (background_) DeleteObject(background_);
        if (normal_font_) DeleteObject(normal_font_);
        if (title_font_) DeleteObject(title_font_);
        if (bold_font_) DeleteObject(bold_font_);
        if (link_font_) DeleteObject(link_font_);
        InterlockedDecrement(&g_objects);
    }

    HRESULT AttachInline(IWebBrowser2 *browser, HWND parent)
    {
        if (!browser || !parent) return E_INVALIDARG;
        detach();
        browser_ = browser;
        browser_->AddRef();
        inline_mode_ = TRUE;
        view_mode_initialized_ = FALSE;
        register_window_class();
        window_ = CreateWindowExW(0, PANE_CLASS, L"", WS_CHILD | WS_VISIBLE,
                                  0, 0, 200, 200, parent, NULL, g_instance, this);
        if (!window_) {
            HRESULT result = HRESULT_FROM_WIN32(GetLastError());
            detach();
            return result;
        }
        load_visuals();
        SetTimer(window_, 1, 200, NULL);
        enforce_windows_2000_layout();
        return S_OK;
    }

    HWND PaneWindow() const { return window_; }

    STDMETHODIMP QueryInterface(REFIID iid, void **out)
    {
        if (!out) return E_POINTER;
        *out = NULL;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IOleWindow) ||
            IsEqualIID(iid, IID_IDockingWindow) || IsEqualIID(iid, IID_IDeskBand))
            *out = static_cast<IDeskBand *>(this);
        else if (IsEqualIID(iid, IID_IObjectWithSite))
            *out = static_cast<IObjectWithSite *>(this);
        else if (IsEqualIID(iid, IID_IPersist) || IsEqualIID(iid, IID_IPersistStream))
            *out = static_cast<IPersistStream *>(this);
        else if (IsEqualIID(iid, IID_IInputObject))
            *out = static_cast<IInputObject *>(this);
        if (!*out) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release()
    {
        LONG value = InterlockedDecrement(&refs_);
        if (!value) delete this;
        return (ULONG)value;
    }

    STDMETHODIMP GetWindow(HWND *out)
    {
        if (!out) return E_POINTER;
        *out = window_;
        return window_ ? S_OK : E_FAIL;
    }
    STDMETHODIMP ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }
    STDMETHODIMP ShowDW(BOOL show)
    {
        if (window_) ShowWindow(window_, show ? SW_SHOW : SW_HIDE);
        return S_OK;
    }
    STDMETHODIMP CloseDW(DWORD)
    {
        detach();
        return S_OK;
    }
    STDMETHODIMP ResizeBorderDW(const RECT *, IUnknown *, BOOL) { return E_NOTIMPL; }

    STDMETHODIMP GetBandInfo(DWORD, DWORD view_mode, DESKBANDINFO *info)
    {
        if (!info) return E_POINTER;
        if (info->dwMask & DBIM_MINSIZE) {
            info->ptMinSize.x = 200;
            info->ptMinSize.y = 40;
        }
        if (info->dwMask & DBIM_MAXSIZE) {
            info->ptMaxSize.x = 200;
            info->ptMaxSize.y = -1;
        }
        if (info->dwMask & DBIM_INTEGRAL) {
            info->ptIntegral.x = 1;
            info->ptIntegral.y = 1;
        }
        if (info->dwMask & DBIM_ACTUAL) info->ptActual.x = 200;
        if (info->dwMask & DBIM_MODEFLAGS)
            info->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;
        if (info->dwMask & DBIM_BKCOLOR) info->crBkgnd = RGB(255, 255, 255);
        if ((info->dwMask & DBIM_TITLE) && (view_mode & DBIF_VIEWMODE_FLOATING))
            lstrcpynW(info->wszTitle, L"Windows 2000", (int)(sizeof(info->wszTitle) / sizeof(info->wszTitle[0])));
        return S_OK;
    }

    STDMETHODIMP SetSite(IUnknown *site)
    {
        detach();
        inline_mode_ = FALSE;
        if (!site) return S_OK;
        site_ = site;
        site_->AddRef();
        IOleWindow *ole_window = NULL;
        HWND parent = NULL;
        if (FAILED(site_->QueryInterface(IID_IOleWindow, (void **)&ole_window)) ||
            FAILED(ole_window->GetWindow(&parent)) || !parent) {
            safe_release(ole_window);
            detach();
            return E_FAIL;
        }
        safe_release(ole_window);
        register_window_class();
        window_ = CreateWindowExW(0, PANE_CLASS, L"", WS_CHILD | WS_VISIBLE,
                                  0, 0, 200, 200, parent, NULL, g_instance, this);
        if (!window_) {
            detach();
            return HRESULT_FROM_WIN32(GetLastError());
        }
        IServiceProvider *provider = NULL;
        if (SUCCEEDED(site_->QueryInterface(IID_IServiceProvider, (void **)&provider))) {
            provider->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2,
                                   (void **)&browser_);
            provider->Release();
        }
        load_visuals();
        SetTimer(window_, 1, 300, NULL);
        return S_OK;
    }

    STDMETHODIMP GetSite(REFIID iid, void **out)
    {
        if (!out) return E_POINTER;
        *out = NULL;
        return site_ ? site_->QueryInterface(iid, out) : E_FAIL;
    }

    STDMETHODIMP GetClassID(CLSID *clsid)
    {
        if (!clsid) return E_POINTER;
        *clsid = CLSID_E2KExplorerBand;
        return S_OK;
    }
    STDMETHODIMP IsDirty() { return S_FALSE; }
    STDMETHODIMP Load(IStream *) { return S_OK; }
    STDMETHODIMP Save(IStream *, BOOL) { return S_OK; }
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *size)
    {
        if (!size) return E_POINTER;
        size->QuadPart = 0;
        return S_OK;
    }

    STDMETHODIMP UIActivateIO(BOOL activate, MSG *)
    {
        if (activate && window_) SetFocus(window_);
        return S_OK;
    }
    STDMETHODIMP HasFocusIO() { return window_ && IsChild(window_, GetFocus()) ? S_OK : S_FALSE; }
    STDMETHODIMP TranslateAcceleratorIO(MSG *) { return S_FALSE; }

    void paint(HDC dc)
    {
        RECT client;
        wchar_t title[260], path[MAX_PATH], selection[260];
        HBRUSH white = (HBRUSH)GetStockObject(WHITE_BRUSH);
        GetClientRect(window_, &client);
        FillRect(dc, &client, white);
        link_count_ = 0;

        if (background_) {
            HDC memory = CreateCompatibleDC(dc);
            HGDIOBJ old = SelectObject(memory, background_);
            BITMAP bitmap;
            GetObject(background_, sizeof(bitmap), &bitmap);
            BitBlt(dc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, memory, 0, 0, SRCCOPY);
            SelectObject(memory, old);
            DeleteDC(memory);
        }

        location_name(browser_, title, sizeof(title) / sizeof(title[0]));
        path[0] = 0;
        location_path(browser_, path, sizeof(path) / sizeof(path[0]));
        HICON icon = location_icon(title, path);
        DrawIconEx(dc, 12, 11, icon, 32, 32, 0, NULL, DI_NORMAL);
        if (icon) DestroyIcon(icon);

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(0, 0, 0));
        HFONT old_font = (HFONT)SelectObject(dc, title_font_);
        RECT title_rect = {12, 49, client.right - 5, 79};
        DrawTextW(dc, title, -1, &title_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(dc, normal_font_);
        HPEN line = CreatePen(PS_SOLID, 1, RGB(99, 149, 197));
        HPEN old_pen = (HPEN)SelectObject(dc, line);
        MoveToEx(dc, 12, 73, NULL);
        LineTo(dc, client.right - 1, 73);
        SelectObject(dc, old_pen);
        DeleteObject(line);

        if (selected_item(window_, selection, sizeof(selection) / sizeof(selection[0]))) {
            SelectObject(dc, bold_font_);
            RECT item_rect = {12, 88, client.right - 8, 108};
            DrawTextW(dc, selection, -1, &item_rect,
                      DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(dc, normal_font_);
            if (path[0]) {
                wchar_t full[MAX_PATH];
                lstrcpynW(full, path, MAX_PATH);
                if (PathAppendW(full, selection)) {
                    WIN32_FILE_ATTRIBUTE_DATA data;
                    if (GetFileAttributesExW(full, GetFileExInfoStandard, &data)) {
                        wchar_t kind[128];
                        SHFILEINFOW details;
                        ZeroMemory(&details, sizeof(details));
                        if (SHGetFileInfoW(full, data.dwFileAttributes, &details,
                                           sizeof(details), SHGFI_TYPENAME))
                            lstrcpynW(kind, details.szTypeName,
                                      (int)(sizeof(kind) / sizeof(kind[0])));
                        else
                            lstrcpynW(kind,
                                      (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                          ? L"File Folder" : L"File",
                                      (int)(sizeof(kind) / sizeof(kind[0])));
                        RECT kind_rect = {12, 108, client.right - 8, 126};
                        DrawTextW(dc, kind, -1, &kind_rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
                        int y = 126;
                        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            ULONGLONG bytes = ((ULONGLONG)data.nFileSizeHigh << 32) |
                                              data.nFileSizeLow;
                            wchar_t formatted[64], line_text[96];
                            StrFormatByteSizeW((LONGLONG)bytes, formatted,
                                               (UINT)(sizeof(formatted) / sizeof(formatted[0])));
                            wsprintfW(line_text, L"Size: %s", formatted);
                            RECT size_rect = {12, y, client.right - 8, y + 18};
                            DrawTextW(dc, line_text, -1, &size_rect,
                                      DT_LEFT | DT_TOP | DT_SINGLELINE);
                            y += 18;
                        }
                        FILETIME local_time;
                        SYSTEMTIME system_time;
                        if (FileTimeToLocalFileTime(&data.ftLastWriteTime, &local_time) &&
                            FileTimeToSystemTime(&local_time, &system_time)) {
                            wchar_t date[64], time[64], modified[160];
                            GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                                           &system_time, NULL, date,
                                           (int)(sizeof(date) / sizeof(date[0])));
                            GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &system_time,
                                           NULL, time,
                                           (int)(sizeof(time) / sizeof(time[0])));
                            wsprintfW(modified, L"Modified: %s %s", date, time);
                            RECT modified_rect = {12, y, client.right - 8, y + 36};
                            DrawTextW(dc, modified, -1, &modified_rect,
                                      DT_LEFT | DT_TOP | DT_WORDBREAK);
                        }
                    }
                }
            }
        } else {
            paint_location_details(dc, client, title);
        }
        SelectObject(dc, old_font);
    }

    int draw_message(HDC dc, const RECT &client, int top, const wchar_t *text)
    {
        RECT measure = {15, top + 3, client.right - 7, client.bottom};
        DrawTextW(dc, text, -1, &measure,
                  DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT);
        RECT box = {12, top, client.right - 7, measure.bottom + 3};
        FillRect(dc, &box, GetSysColorBrush(COLOR_INFOBK));
        HBRUSH border = CreateSolidBrush(RGB(211, 211, 211));
        FrameRect(dc, &box, border);
        DeleteObject(border);
        SetTextColor(dc, GetSysColor(COLOR_INFOTEXT));
        DrawTextW(dc, text, -1, &measure, DT_LEFT | DT_TOP | DT_WORDBREAK);
        SetTextColor(dc, RGB(0, 0, 0));
        return box.bottom;
    }

    int draw_link(HDC dc, const RECT &client, int top, const wchar_t *text,
                  ExplorerLinkTarget target)
    {
        if (link_count_ >= (int)(sizeof(link_rects_) / sizeof(link_rects_[0])))
            return top;
        RECT link = {12, top, client.right - 5, top + 18};
        HFONT old = (HFONT)SelectObject(dc, link_font_);
        SetTextColor(dc, RGB(0, 0, 255));
        DrawTextW(dc, text, -1, &link,
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old);
        SetTextColor(dc, RGB(0, 0, 0));
        link_rects_[link_count_] = link;
        link_targets_[link_count_] = target;
        ++link_count_;
        return top + 18;
    }

    void paint_location_details(HDC dc, const RECT &client, const wchar_t *title)
    {
        RECT hint = {12, 88, client.right - 8, 126};
        if (lstrcmpiW(title, L"Control Panel") == 0) {
            DrawTextW(dc, L"Use the settings in Control Panel to personalize your computer.",
                      -1, &hint, DT_LEFT | DT_TOP | DT_WORDBREAK);
            RECT prompt = {12, 132, client.right - 8, 164};
            DrawTextW(dc, L"Select an item to view its description.", -1, &prompt,
                      DT_LEFT | DT_TOP | DT_WORDBREAK);
            int y = 166;
            y = draw_link(dc, client, y, L"Windows Update", LINK_WINDOWS_UPDATE);
            draw_link(dc, client, y, L"Windows 2000 Support", LINK_WINDOWS_SUPPORT);
            return;
        }

        if (lstrcmpiW(title, L"Recycle Bin") == 0) {
            DrawTextW(dc,
                      L"This folder contains files and folders that you have deleted from your computer.",
                      -1, &hint, DT_LEFT | DT_TOP | DT_WORDBREAK);
            RECT empty = {12, 140, client.right - 8, 190};
            DrawTextW(dc, L"There are no items in the Recycle Bin.", -1, &empty,
                      DT_LEFT | DT_TOP | DT_WORDBREAK);
            return;
        }

        if (lstrcmpiW(title, L"My Network Places") == 0) {
            DrawTextW(dc,
                      L"Use this folder to open files and folders on other computers and to install network printers.",
                      -1, &hint, DT_LEFT | DT_TOP | DT_WORDBREAK);
            RECT prompt = {12, 150, client.right - 8, 182};
            DrawTextW(dc, L"Select an item to view its description.", -1, &prompt,
                      DT_LEFT | DT_TOP | DT_WORDBREAK);
            RECT see_also = {12, 190, client.right - 8, 208};
            DrawTextW(dc, L"See also:", -1, &see_also,
                      DT_LEFT | DT_TOP | DT_SINGLELINE);
            int y = 210;
            y = draw_link(dc, client, y, L"My Documents", LINK_MY_DOCUMENTS);
            draw_link(dc, client, y, L"My Computer", LINK_MY_COMPUTER);
            return;
        }

        if (lstrcmpiW(title, L"Printers") == 0 ||
            lstrcmpiW(title, L"Printers and Faxes") == 0) {
            RECT intro = {12, 88, client.right - 8, 260};
            DrawTextW(dc,
                      L"This folder contains information about printers that are currently installed, and a wizard to help you install new printers.\n\nTo get information about a printer that is currently installed, right-click the printer's icon.\n\nTo install a new printer, click the Add Printer icon.",
                      -1, &intro, DT_LEFT | DT_TOP | DT_WORDBREAK);
            return;
        }

        if (lstrcmpiW(title, L"Network Connections") == 0 ||
            lstrcmpiW(title, L"Network and Dial-up Connections") == 0) {
            RECT intro = {12, 88, client.right - 8, 290};
            DrawTextW(dc,
                      L"This folder contains network connections for this computer, and a wizard to help you create a new connection.\n\nTo create a new connection, click Make New Connection.\n\nTo open a connection, click its icon.\n\nTo access settings and components of a connection, right-click its icon and then click Properties.",
                      -1, &intro, DT_LEFT | DT_TOP | DT_WORDBREAK);
            return;
        }

        if (lstrcmpiW(title, L"Scheduled Tasks") == 0) {
            RECT intro = {12, 88, client.right - 8, 250};
            DrawTextW(dc,
                      L"This folder contains tasks you've scheduled for Windows. Windows automatically performs each task at the scheduled time.\n\nFor example, you can schedule a time for Windows to clean up your hard disk by deleting unnecessary files.",
                      -1, &intro, DT_LEFT | DT_TOP | DT_WORDBREAK);
            return;
        }

        DrawTextW(dc, L"Select an item to view its description.", -1, &hint,
                  DT_LEFT | DT_TOP | DT_WORDBREAK);
        int bottom = 108;
        if (lstrcmpiW(title, L"My Documents") == 0) {
            bottom = draw_message(dc, client, 112, L"Stores and manages documents");
            RECT see_also = {12, bottom + 11, client.right - 8, bottom + 29};
            DrawTextW(dc, L"See also:", -1, &see_also,
                      DT_LEFT | DT_TOP | DT_SINGLELINE);
            int y = bottom + 31;
            y = draw_link(dc, client, y, L"My Network Places", LINK_NETWORK_PLACES);
            draw_link(dc, client, y, L"My Computer", LINK_MY_COMPUTER);
        } else if (lstrcmpiW(title, L"My Computer") == 0) {
            bottom = draw_message(dc, client, 112,
                                  L"Displays the files and folders on your computer");
            RECT see_also = {12, bottom + 11, client.right - 8, bottom + 29};
            DrawTextW(dc, L"See also:", -1, &see_also,
                      DT_LEFT | DT_TOP | DT_SINGLELINE);
            int y = bottom + 31;
            y = draw_link(dc, client, y, L"My Documents", LINK_MY_DOCUMENTS);
            y = draw_link(dc, client, y, L"My Network Places", LINK_NETWORK_PLACES);
            draw_link(dc, client, y, L"Network and Dial-up Connections", LINK_CONNECTIONS);
        }
    }

    void navigate_link(ExplorerLinkTarget target)
    {
        const wchar_t *address = NULL;
        switch (target) {
        case LINK_MY_DOCUMENTS: address = L"shell:Personal"; break;
        case LINK_MY_COMPUTER: address = L"shell:MyComputerFolder"; break;
        case LINK_NETWORK_PLACES: address = L"shell:NetworkPlacesFolder"; break;
        case LINK_CONNECTIONS: address = L"shell:ConnectionsFolder"; break;
        case LINK_WINDOWS_UPDATE: address = L"http://windowsupdate.microsoft.com/"; break;
        case LINK_WINDOWS_SUPPORT: address = L"https://support.microsoft.com/"; break;
        default: return;
        }
        if (!browser_) return;
        BSTR url = SysAllocString(address);
        if (!url) return;
        VARIANT empty;
        VariantInit(&empty);
        browser_->Navigate(url, &empty, &empty, &empty, &empty);
        SysFreeString(url);
    }

    void enforce_windows_2000_layout()
    {
        if (!window_) return;
        if (!view_mode_initialized_ && browser_)
            view_mode_initialized_ = initialize_shell_view_mode(browser_);
        if (inline_mode_) {
            HWND view = GetParent(window_);
            if (!view) return;
            RECT area;
            GetClientRect(view, &area);
            int pane_width = area.right < 400 ? area.right / 2 : 200;
            SetWindowPos(window_, HWND_TOP, 0, 0, pane_width, area.bottom,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
            HWND list = FindWindowExW(view, NULL, L"SysListView32", NULL);
            if (list) {
                SetWindowPos(list, window_, pane_width, 0,
                             area.right - pane_width, area.bottom,
                             SWP_NOACTIVATE | SWP_SHOWWINDOW);
                if (SendMessageW(list, LVM_ISGROUPVIEWENABLED, 0, 0))
                    SendMessageW(list, LVM_ENABLEGROUPVIEW, FALSE, 0);
                if (SendMessageW(list, LVM_GETVIEW, 0, 0) != LV_VIEW_ICON)
                    SendMessageW(list, LVM_SETVIEW, LV_VIEW_ICON, 0);
            }
            return;
        }
        HWND root = GetAncestor(window_, GA_ROOT);
        if (folders_mode_requested(root) && browser_) {
            VARIANT clsid, show, size;
            VariantInit(&clsid);
            VariantInit(&show);
            VariantInit(&size);
            clsid.vt = VT_BSTR;
            clsid.bstrVal = SysAllocString(FOLDERS_CLSID_TEXT);
            show.vt = VT_BOOL;
            show.boolVal = VARIANT_TRUE;
            AddRef();
            KillTimer(window_, 1);
            browser_->ShowBrowserBar(&clsid, &show, &size);
            VariantClear(&clsid);
            Release();
            return;
        }
        HWND rebar = GetParent(window_);
        if (!rebar) return;
        HWND sibling = NULL;
        wchar_t class_name[96];
        while ((sibling = FindWindowExW(rebar, sibling, NULL, NULL)) != NULL) {
            if (sibling == window_) continue;
            class_name[0] = 0;
            GetClassNameW(sibling, class_name,
                          (int)(sizeof(class_name) / sizeof(class_name[0])));
            if (lstrcmpiW(class_name, TOOLBARCLASSNAMEW) == 0)
                ShowWindow(sibling, SW_HIDE);
        }
        RECT area;
        GetClientRect(rebar, &area);
        SetWindowPos(window_, HWND_TOP, 0, 0, area.right, area.bottom,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);

        HWND list = find_descendant(root, L"SysListView32");
        if (list) {
            if (SendMessageW(list, LVM_ISGROUPVIEWENABLED, 0, 0))
                SendMessageW(list, LVM_ENABLEGROUPVIEW, FALSE, 0);
            if (SendMessageW(list, LVM_GETVIEW, 0, 0) != LV_VIEW_ICON)
                SendMessageW(list, LVM_SETVIEW, LV_VIEW_ICON, 0);
        }
    }

private:
    LONG refs_;
    IUnknown *site_;
    IWebBrowser2 *browser_;
    HWND window_;
    HBITMAP background_;
    HFONT normal_font_;
    HFONT title_font_;
    HFONT bold_font_;
    HFONT link_font_;
    RECT link_rects_[4];
    ExplorerLinkTarget link_targets_[4];
    int link_count_;
    BOOL inline_mode_;
    BOOL view_mode_initialized_;

    static LRESULT CALLBACK pane_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        ExplorerBand *self = (ExplorerBand *)GetWindowLongPtrW(window, GWLP_USERDATA);
        if (message == WM_NCCREATE) {
            CREATESTRUCTW *create = (CREATESTRUCTW *)lparam;
            self = (ExplorerBand *)create->lpCreateParams;
            SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)self);
        }
        switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_TIMER:
            if (self) self->enforce_windows_2000_layout();
            InvalidateRect(window, NULL, FALSE);
            return 0;
        case WM_PAINT:
            if (self) {
                PAINTSTRUCT paint;
                HDC dc = BeginPaint(window, &paint);
                self->paint(dc);
                EndPaint(window, &paint);
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (self) {
                POINT point = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                for (int index = 0; index < self->link_count_; ++index) {
                    if (PtInRect(&self->link_rects_[index], point)) {
                        self->navigate_link(self->link_targets_[index]);
                        return 0;
                    }
                }
            }
            break;
        case WM_SETCURSOR:
            if (self) {
                POINT point;
                GetCursorPos(&point);
                ScreenToClient(window, &point);
                for (int index = 0; index < self->link_count_; ++index) {
                    if (PtInRect(&self->link_rects_[index], point)) {
                        SetCursor(LoadCursorW(NULL, IDC_HAND));
                        return TRUE;
                    }
                }
            }
            break;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    static void register_window_class()
    {
        static LONG registered;
        if (InterlockedCompareExchange(&registered, 1, 0) != 0) return;
        WNDCLASSW cls;
        ZeroMemory(&cls, sizeof(cls));
        cls.lpfnWndProc = pane_proc;
        cls.hInstance = g_instance;
        cls.hCursor = LoadCursorW(NULL, IDC_ARROW);
        cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        cls.lpszClassName = PANE_CLASS;
        RegisterClassW(&cls);
    }

    void load_visuals()
    {
        wchar_t path[MAX_PATH];
        UINT dpi = 96;
        HDC screen = GetDC(NULL);
        if (screen) {
            dpi = (UINT)GetDeviceCaps(screen, LOGPIXELSY);
            ReleaseDC(NULL, screen);
        }
        GetWindowsDirectoryW(path, MAX_PATH);
        PathAppendW(path, L"Web\\wvleft.bmp");
        background_ = (HBITMAP)LoadImageW(NULL, path, IMAGE_BITMAP, 0, 0,
                                          LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        normal_font_ = CreateFontW(-MulDiv(8, (int)dpi, 72), 0, 0, 0, FW_NORMAL,
                                   FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");
        title_font_ = CreateFontW(-MulDiv(13, (int)dpi, 72), 0, 0, 0, FW_BOLD,
                                  FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");
        bold_font_ = CreateFontW(-MulDiv(8, (int)dpi, 72), 0, 0, 0, FW_BOLD,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");
        link_font_ = CreateFontW(-MulDiv(8, (int)dpi, 72), 0, 0, 0, FW_NORMAL,
                                 FALSE, TRUE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");
    }

    void detach()
    {
        if (window_) {
            KillTimer(window_, 1);
            DestroyWindow(window_);
            window_ = NULL;
        }
        safe_release(browser_);
        browser_ = NULL;
        safe_release(site_);
        site_ = NULL;
        inline_mode_ = FALSE;
        view_mode_initialized_ = FALSE;
    }
};

class ExplorerHook : public IObjectWithSite {
public:
    ExplorerHook() : refs_(1), site_(NULL), browser_(NULL), timer_window_(NULL),
                     inline_band_(NULL)
    {
        InterlockedIncrement(&g_objects);
    }
    virtual ~ExplorerHook()
    {
        cleanup();
        InterlockedDecrement(&g_objects);
    }
    STDMETHODIMP QueryInterface(REFIID iid, void **out)
    {
        if (!out) return E_POINTER;
        *out = NULL;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IObjectWithSite))
            *out = static_cast<IObjectWithSite *>(this);
        if (!*out) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release()
    {
        LONG value = InterlockedDecrement(&refs_);
        if (!value) delete this;
        return (ULONG)value;
    }
    STDMETHODIMP SetSite(IUnknown *site)
    {
        cleanup();
        site_ = site;
        if (site_) site_->AddRef();
        if (!site_) return S_OK;
        if (FAILED(site_->QueryInterface(IID_IWebBrowser2, (void **)&browser_)))
            return S_OK;
        register_hook_class();
        timer_window_ = CreateWindowExW(0, HOOK_CLASS, L"", WS_POPUP,
                                        0, 0, 0, 0, HWND_MESSAGE, NULL,
                                        g_instance, this);
        if (timer_window_) SetTimer(timer_window_, 1, 150, NULL);
        return S_OK;
    }
    STDMETHODIMP GetSite(REFIID iid, void **out)
    {
        if (!out) return E_POINTER;
        *out = NULL;
        return site_ ? site_->QueryInterface(iid, out) : E_FAIL;
    }
private:
    LONG refs_;
    IUnknown *site_;
    IWebBrowser2 *browser_;
    HWND timer_window_;
    ExplorerBand *inline_band_;

    void ensure_inline_pane()
    {
        if (!browser_) return;
        SHANDLE_PTR browser_window = 0;
        if (FAILED(browser_->get_HWND(&browser_window)) || !browser_window) return;
        HWND view = find_descendant((HWND)browser_window, L"SHELLDLL_DefView");
        if (!view) return;
        if (inline_band_ && GetParent(inline_band_->PaneWindow()) == view) return;
        if (inline_band_) {
            inline_band_->Release();
            inline_band_ = NULL;
        }
        inline_band_ = new ExplorerBand();
        if (!inline_band_ || FAILED(inline_band_->AttachInline(browser_, view))) {
            if (inline_band_) inline_band_->Release();
            inline_band_ = NULL;
        }
    }

    void cleanup()
    {
        if (timer_window_) {
            KillTimer(timer_window_, 1);
            DestroyWindow(timer_window_);
            timer_window_ = NULL;
        }
        if (inline_band_) {
            inline_band_->Release();
            inline_band_ = NULL;
        }
        safe_release(browser_);
        browser_ = NULL;
        safe_release(site_);
        site_ = NULL;
    }

    static LRESULT CALLBACK hook_proc(HWND window, UINT message,
                                      WPARAM wparam, LPARAM lparam)
    {
        ExplorerHook *self = (ExplorerHook *)GetWindowLongPtrW(window, GWLP_USERDATA);
        if (message == WM_NCCREATE) {
            CREATESTRUCTW *create = (CREATESTRUCTW *)lparam;
            self = (ExplorerHook *)create->lpCreateParams;
            SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)self);
        }
        if (message == WM_TIMER && self) {
            self->ensure_inline_pane();
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    static void register_hook_class()
    {
        static LONG registered;
        if (InterlockedCompareExchange(&registered, 1, 0) != 0) return;
        WNDCLASSW cls;
        ZeroMemory(&cls, sizeof(cls));
        cls.lpfnWndProc = hook_proc;
        cls.hInstance = g_instance;
        cls.lpszClassName = HOOK_CLASS;
        RegisterClassW(&cls);
    }
};

class ClassFactory : public IClassFactory {
public:
    ClassFactory(REFCLSID clsid) : refs_(1), clsid_(clsid) {}
    virtual ~ClassFactory() {}
    STDMETHODIMP QueryInterface(REFIID iid, void **out)
    {
        if (!out) return E_POINTER;
        *out = NULL;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IClassFactory))
            *out = static_cast<IClassFactory *>(this);
        if (!*out) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release()
    {
        LONG value = InterlockedDecrement(&refs_);
        if (!value) delete this;
        return (ULONG)value;
    }
    STDMETHODIMP CreateInstance(IUnknown *outer, REFIID iid, void **out)
    {
        if (outer) return CLASS_E_NOAGGREGATION;
        IUnknown *object;
        if (IsEqualCLSID(clsid_, CLSID_E2KExplorerBand))
            object = static_cast<IDeskBand *>(new ExplorerBand());
        else
            object = static_cast<IObjectWithSite *>(new ExplorerHook());
        if (!object) return E_OUTOFMEMORY;
        HRESULT result = object->QueryInterface(iid, out);
        object->Release();
        return result;
    }
    STDMETHODIMP LockServer(BOOL lock)
    {
        if (lock) InterlockedIncrement(&g_locks);
        else InterlockedDecrement(&g_locks);
        return S_OK;
    }
private:
    LONG refs_;
    CLSID clsid_;
};

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_instance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) HRESULT WINAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    if (!IsEqualCLSID(clsid, CLSID_E2KExplorerBand) &&
        !IsEqualCLSID(clsid, CLSID_E2KExplorerHook)) return CLASS_E_CLASSNOTAVAILABLE;
    ClassFactory *factory = new ClassFactory(clsid);
    if (!factory) return E_OUTOFMEMORY;
    HRESULT result = factory->QueryInterface(iid, out);
    factory->Release();
    return result;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(void)
{
    return (g_objects == 0 && g_locks == 0) ? S_OK : S_FALSE;
}
