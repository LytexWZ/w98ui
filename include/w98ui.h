#pragma once

// -----------------------------------------------------------------------------
// Index of this file:
// - Header configuration and dependencies
// - Namespace setup and forward declarations
// - Widget API (base class + concrete controls)
// - Layout data model (LayoutItem)
// - Layout API (base class + concrete layout strategies)
// - Window API (frame rendering, dispatch, and custom chrome)
// - Template helpers (Layout::Add / Layout::AddExpanded)
// -----------------------------------------------------------------------------

#define NOMINMAX  // Prevent windows.h from defining min/max macros
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace w98 {

// -----------------------------------------------------------------------------
// Namespace setup and forward declarations
// -----------------------------------------------------------------------------

bool Initialize(HINSTANCE hInstance);
class Layout;
class Widget;
class Window;

// -----------------------------------------------------------------------------
// Widget API
// -----------------------------------------------------------------------------

// Base class for all controls in this library.
//
// Widget stores the HWND and lightweight sizing metadata used by layout classes.
// It also exposes overridable handlers for command and scroll notifications so
// Window can route Win32 messages to strongly typed control instances.
class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    HWND GetHWND() const { return m_hwnd; }
    int  GetId()   const { return m_id; }

    virtual bool HandleCommand(int code) { (void)code; return false; }
    virtual bool HandleScroll(UINT msg, WPARAM wParam) { (void)msg; (void)wParam; return false; }

    int GetPrefWidth()  const { return m_prefW; }
    int GetPrefHeight() const { return m_prefH; }

protected:
    HWND m_hwnd  = NULL;
    int  m_id    = 0;
    int  m_prefW = 0;
    int  m_prefH = 0;
};

// Push button control (BS_PUSHBUTTON).
// Triggers OnClick callback when the control receives BN_CLICKED.
class Button : public Widget {
public:
    Button(HWND parent, const std::wstring& text, int id, int w, int h);
    Button& OnClick(std::function<void()> cb) { m_onClick = std::move(cb); return *this; }
    bool HandleCommand(int code) override;
private:
    std::function<void()> m_onClick;
};

// Check box control (BS_AUTOCHECKBOX).
// Triggers OnChange callback with the new checked state.
class CheckBox : public Widget {
public:
    CheckBox(HWND parent, const std::wstring& text, int id, int w, int h);
    CheckBox& OnChange(std::function<void(bool)> cb) { m_onChange = std::move(cb); return *this; }
    bool IsChecked() const;
    bool HandleCommand(int code) override;
private:
    std::function<void(bool)> m_onChange;
};

// Static text label (SS_LEFT). Non-interactive helper control.
class Label : public Widget {
public:
    Label(HWND parent, const std::wstring& text, int id, int w, int h);
    void SetText(const std::wstring& text);
};

// Edit control wrapper.
// Supports single-line and multiline modes, plus OnChange callback routing.
class TextBox : public Widget {
public:
    TextBox(HWND parent, const std::wstring& text, int id, int w, int h, bool multiline = false);
    TextBox& OnChange(std::function<void(const std::wstring&)> cb) { m_onChange = std::move(cb); return *this; }
    std::wstring GetText() const;
    void SetText(const std::wstring& text);
    void AppendLine(const std::wstring& line);
    bool HandleCommand(int code) override;
private:
    std::function<void(const std::wstring&)> m_onChange;
};

// Drop-down list wrapper (CBS_DROPDOWNLIST).
// Exposes AddItem/SetSelection and OnChange(index, text).
class ComboBox : public Widget {
public:
    ComboBox(HWND parent, int id, int w, int dropdownH);
    void AddItem(const std::wstring& text);
    void SetSelection(int index);
    ComboBox& OnChange(std::function<void(int, const std::wstring&)> cb) { m_onChange = std::move(cb); return *this; }
    bool HandleCommand(int code) override;
private:
    std::function<void(int, const std::wstring&)> m_onChange;
};

// Trackbar wrapper (TRACKBAR_CLASSW).
// OnChange runs while dragging; OnRelease runs on TB_ENDTRACK.
class Slider : public Widget {
public:
    Slider(HWND parent, int id, int w, int h);
    Slider& OnChange(std::function<void(int)> cb)  { m_onChange  = std::move(cb); return *this; }
    Slider& OnRelease(std::function<void(int)> cb) { m_onRelease = std::move(cb); return *this; }
    void SetRange(int lo, int hi);
    int  GetValue() const;
    bool HandleScroll(UINT msg, WPARAM wParam) override;
private:
    std::function<void(int)> m_onChange;
    std::function<void(int)> m_onRelease;
};

// Listbox wrapper (LBS_NOTIFY).
// OnChange receives current selected index on LBN_SELCHANGE.
class ListBox : public Widget {
public:
    ListBox(HWND parent, int id, int w, int h);
    void AddItem(const std::wstring& text);
    ListBox& OnChange(std::function<void(int)> cb) { m_onChange = std::move(cb); return *this; }
    bool HandleCommand(int code) override;
private:
    std::function<void(int)> m_onChange;
};

// Group box frame control used for titled panel-like containers.
class GroupBox : public Widget {
public:
    GroupBox(HWND parent, const std::wstring& text, int id, int w, int h);
};

// -----------------------------------------------------------------------------
// Layout data model
// -----------------------------------------------------------------------------

// Internal unit consumed by layout engines.
//
// Exactly one of `hwnd` / `widget` / `childLayout` is typically active:
// - `widget` means owned control created by this library.
// - `hwnd` means foreign/external child window handle.
// - `childLayout` means nested layout subtree.
struct LayoutItem {
    HWND hwnd = NULL;
    std::unique_ptr<Widget> widget;
    std::unique_ptr<Layout> childLayout;

    int minWidth  = 0;
    int minHeight = 0;
    bool expandHorizontal = false;
    bool expandVertical   = false;
    int stretch    = 0;
    bool isComboBox = false;

    int gridColumn     = 0;
    int gridRow        = 0;
    int gridColumnSpan = 1;
    int gridRowSpan    = 1;

    LayoutItem() = default;
    LayoutItem(LayoutItem&&) = default;
    LayoutItem& operator=(LayoutItem&&) = default;
    LayoutItem(const LayoutItem&) = delete;
    LayoutItem& operator=(const LayoutItem&) = delete;
};

// -----------------------------------------------------------------------------
// Layout API
// -----------------------------------------------------------------------------

// Abstract base layout.
//
// Layout manages a sequence of LayoutItem entries and computes child rectangles
// from available client area, padding, spacing, and expansion/stretch policy.
class Layout {
public:
    Layout() = default;
    virtual ~Layout() = default;
    Layout(const Layout&) = delete;
    Layout& operator=(const Layout&) = delete;

    virtual void Calculate(const RECT& availableRect) = 0;
    virtual void Show(bool show);
    virtual bool HandleNotify(LPNMHDR hdr);

    // Add widget items with optional expansion and proportional stretch.
    // - expandH / expandV controls whether extra space can be consumed.
    // - stretch participates in distributing extra space among expanders.
    void AddWidget(std::unique_ptr<Widget> widget, bool expandH = false, bool expandV = false, int stretch = 0);
    void AddWidget(std::unique_ptr<Widget> widget, int minW, int minH, bool expandH = false, bool expandV = false, int stretch = 0);
    void AddWidget(HWND hwnd, int minW = 0, int minH = 0, bool expandH = false, bool expandV = false, int stretch = 0);
    void AddWidget(const Widget& w, bool expandH = false, bool expandV = false, int stretch = 0);

    // Add nested layout subtree or flexible spacer item.
    void AddLayout(std::unique_ptr<Layout> layout, int minW = 0, int minH = 0, bool expandH = false, bool expandV = false, int stretch = 0);
    void AddSpacer(int size = 0, int stretch = 1);

    // Factory helper: create control with `m_parent`, register with owning
    // Window, then add to this layout.
    template<typename T, typename... Args>
    T& Add(Args&&... args);

    // Same as Add(), but allows passing expansion/stretch policy in one call.
    template<typename T, typename... Args>
    T& AddExpanded(bool expandH, bool expandV, int stretch, Args&&... args);

    void SetPadding(int p);
    void SetPadding(int left, int top, int right, int bottom);
    void SetSpacing(int spacing);

    HWND GetParentWindow() const { return m_parent; }
    virtual void SetParentWindow(HWND parent);
    virtual void SetWindow(Window* win);

protected:
    std::vector<LayoutItem> m_items;
    HWND m_parent  = NULL;
    Window* m_win  = nullptr;
    int m_paddingLeft = 0, m_paddingTop = 0, m_paddingRight = 0, m_paddingBottom = 0;
    int m_spacing  = 0;

    RECT GetPaddedRect(const RECT& rect) const;
    void PlaceItem(LayoutItem& item, const RECT& itemRect);
};

// Vertical stack layout.
// Items are laid top-to-bottom with spacing and optional stretch expansion.
class VBox : public Layout {
public:
    explicit VBox(HWND parent = NULL) { m_parent = parent; }
    void Calculate(const RECT& availableRect) override;
};

// Horizontal stack layout.
// Items are laid left-to-right with spacing and optional stretch expansion.
class HBox : public Layout {
public:
    explicit HBox(HWND parent = NULL) { m_parent = parent; }
    void Calculate(const RECT& availableRect) override;
};

// Grid layout.
// Supports fixed or auto row count plus row/column span placement.
class GridLayout : public Layout {
public:
    GridLayout(int columns, int rows = -1, HWND parent = NULL);
    void Calculate(const RECT& availableRect) override;
    
    void AddWidget(std::unique_ptr<Widget> widget, int col, int row, int colspan = 1, int rowspan = 1);
    void AddWidget(HWND hwnd,       int col, int row, int colspan = 1, int rowspan = 1);
    void AddWidget(const Widget& w, int col, int row, int colspan = 1, int rowspan = 1);
    void AddLayout(std::unique_ptr<Layout> layout, int col, int row, int colspan = 1, int rowspan = 1);
private:
    int m_columns, m_rows;
    void CalculateGridMetrics(const RECT& rect, std::vector<int>& colW, std::vector<int>& rowH);
};

// Panel is a layout wrapper that can optionally draw a titled GroupBox frame.
// Child content is delegated to `m_childLayout` with interior padding.
class Panel : public Layout {
public:
    Panel(HWND parent = NULL, const std::wstring& title = L"", int id = 0);
    void Calculate(const RECT& availableRect) override;
    void Show(bool show) override;
    bool HandleNotify(LPNMHDR hdr) override;
    void SetParentWindow(HWND parent) override;
    void SetWindow(Window* win) override;
    void SetChildLayout(std::unique_ptr<Layout> layout);
private:
    HWND m_groupBox = NULL;
    std::unique_ptr<Layout> m_childLayout;
    std::wstring m_title;
};

// Tabbed layout container.
// Owns a native tab control and one child layout per tab page.
class TabPanel : public Layout {
public:
    TabPanel(HWND parent = NULL, int id = 0);
    void Calculate(const RECT& availableRect) override;
    void Show(bool show) override;
    bool HandleNotify(LPNMHDR hdr) override;
    void SetParentWindow(HWND parent) override;
    void SetWindow(Window* win) override;
    void AddTab(const std::wstring& title, std::unique_ptr<Layout> layout);
    HWND GetHWND() const { return m_tabControl; }
private:
    HWND m_tabControl = NULL;
    std::vector<std::unique_ptr<Layout>> m_tabLayouts;
    int m_activeTab = 0;
};

// -----------------------------------------------------------------------------
// Window API
// -----------------------------------------------------------------------------

// Top-level host window with custom Win98-style non-client rendering.
//
// Window handles:
// - Frame/title rendering and caption button interaction.
// - Widget event dispatch from WM_COMMAND/WM_*SCROLL.
// - Root layout sizing and propagation on WM_SIZE.
class Window {
public:
    Window(const std::wstring& title, int x, int y, int width, int height);
    virtual ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void Show(int nCmdShow = SW_SHOW);
    void Update();
    HWND GetHWND() const { return m_hwnd; }

    void RegisterWidget(Widget* w);
    void SetLayout(std::unique_ptr<Layout> layout);
    Layout* GetLayout() const { return m_rootLayout.get(); }
    RECT GetClientArea() const;

    using MessageCallback = std::function<void(HWND, UINT, WPARAM, LPARAM)>;
    void SetMessageCallback(MessageCallback cb) { m_msgCallback = std::move(cb); }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    HWND m_hwnd;
    MessageCallback m_msgCallback;

private:
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void DrawWin98Frame(HDC hdc, int width, int height);
    void DrawTitleBarBackground(HDC hdc, const RECT& rect, bool active);
    void UpdateMetrics(int width, int height);
    void UpdateLayout();

    std::wstring m_title;
    bool m_isActive = false, m_isMoving = false, m_isMaximized = false;
    RECT m_titleRect = {}, m_closeRect = {}, m_maxRect = {}, m_minRect = {};
    bool m_closeDown = false, m_maxDown = false, m_minDown = false;
    bool m_mouseInClose = false, m_mouseInMax = false, m_mouseInMin = false;

    std::unique_ptr<Layout> m_rootLayout;

    std::unordered_map<int,  Widget*> m_widgetsById;
    std::unordered_map<HWND, Widget*> m_widgetsByHwnd;
};

// -----------------------------------------------------------------------------
// Template helper implementations
// -----------------------------------------------------------------------------

// Create widget `T`, register with owning window (if available), then append.
template<typename T, typename... Args>
T& Layout::Add(Args&&... args) {
    auto w = std::make_unique<T>(m_parent, std::forward<Args>(args)...);
    T& ref = *w;
    if (m_win) m_win->RegisterWidget(&ref);
    AddWidget(std::move(w));
    return ref;
}

// Same as Add(), but immediately attaches expansion/stretch policy.
template<typename T, typename... Args>
T& Layout::AddExpanded(bool expandH, bool expandV, int stretch, Args&&... args) {
    auto w = std::make_unique<T>(m_parent, std::forward<Args>(args)...);
    T& ref = *w;
    if (m_win) m_win->RegisterWidget(&ref);
    AddWidget(std::move(w), expandH, expandV, stretch);
    return ref;
}

}
