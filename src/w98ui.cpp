#include "w98ui.h"
#include <windowsx.h>
#include <uxtheme.h>
#include <algorithm>

// -----------------------------------------------------------------------------
// Index of this file:
// - Includes, libraries, and style constants
// - Shared helper for classic control creation
// - Widget implementations (Button, CheckBox, Label, TextBox, ComboBox, etc.)
// - Layout base implementation and placement helpers
// - Concrete layout engines (VBox, HBox, GridLayout)
// - Composite containers (Panel, TabPanel)
// - Window lifecycle, painting, metrics, and message routing
// -----------------------------------------------------------------------------

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

namespace w98 {

// -----------------------------------------------------------------------------
// Window style constants
// -----------------------------------------------------------------------------

const wchar_t CLASS_NAME[]    = L"Win98UI_Window";
const int BORDER_THICKNESS    = 4;
const int TITLE_BAR_HEIGHT    = 18;
const int CAPTION_BUTTON_SIZE = 14;

// -----------------------------------------------------------------------------
// Shared helper: create a child control with Win98-like visual defaults
// -----------------------------------------------------------------------------
//
// Every stock control in this library goes through this helper so we get:
// - Consistent font (MS Sans Serif 8pt equivalent)
// - Theme suppression for classic look
// - Shared base style flags (WS_CHILD/WS_VISIBLE/WS_CLIPSIBLINGS)
static HWND CreateClassicControl(DWORD exStyle, const wchar_t* cls, HWND parent,
                                  const std::wstring& text, DWORD style, int id,
                                  int x, int y, int w, int h) {
    HWND child = CreateWindowExW(exStyle, cls, text.c_str(),
                                  style | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                  x, y, w, h, parent, (HMENU)(INT_PTR)id,
                                  GetModuleHandle(NULL), NULL);
    SetWindowTheme(child, L" ", L" ");
    HFONT hFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS Sans Serif");
    SendMessageW(child, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    return child;
}

// -----------------------------------------------------------------------------
// Widget implementations
// -----------------------------------------------------------------------------
//
// Control constructors build native Win32 child windows and cache preferred size
// so layout engines can allocate space before controls are actually measured

Button::Button(HWND parent, const std::wstring& text, int id, int w, int h) {
    m_id = id; m_prefW = w; m_prefH = h;
    m_hwnd = CreateClassicControl(0, L"BUTTON", parent, text, BS_PUSHBUTTON | WS_TABSTOP, id, 0, 0, w, h);
}
bool Button::HandleCommand(int code) {
    if (code == BN_CLICKED && m_onClick) { m_onClick(); return true; }
    return false;
}

CheckBox::CheckBox(HWND parent, const std::wstring& text, int id, int w, int h) {
    m_id = id; m_prefW = w; m_prefH = h;
    m_hwnd = CreateClassicControl(0, L"BUTTON", parent, text, BS_AUTOCHECKBOX | WS_TABSTOP, id, 0, 0, w, h);
}
bool CheckBox::IsChecked() const {
    return SendMessageW(m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
bool CheckBox::HandleCommand(int code) {
    if (code == BN_CLICKED && m_onChange) { m_onChange(IsChecked()); return true; }
    return false;
}

Label::Label(HWND parent, const std::wstring& text, int id, int w, int h) {
    m_id = id; m_prefW = w; m_prefH = h;
    m_hwnd = CreateClassicControl(0, L"STATIC", parent, text, SS_LEFT, id, 0, 0, w, h);
}
void Label::SetText(const std::wstring& text) { SetWindowTextW(m_hwnd, text.c_str()); }

TextBox::TextBox(HWND parent, const std::wstring& text, int id, int w, int h, bool multiline) {
    m_id = id; m_prefW = w; m_prefH = h;
    DWORD style = ES_LEFT | WS_TABSTOP;
    if (multiline) style |= ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | WS_VSCROLL;
    else           style |= ES_AUTOHSCROLL;
    m_hwnd = CreateClassicControl(WS_EX_CLIENTEDGE, L"EDIT", parent, text, style, id, 0, 0, w, h);
}
std::wstring TextBox::GetText() const {
    int len = GetWindowTextLengthW(m_hwnd);
    std::wstring buf(len, L'\0');
    GetWindowTextW(m_hwnd, &buf[0], len + 1);
    return buf;
}
void TextBox::SetText(const std::wstring& text) { SetWindowTextW(m_hwnd, text.c_str()); }
void TextBox::AppendLine(const std::wstring& line) {
    int len = GetWindowTextLengthW(m_hwnd);
    SendMessageW(m_hwnd, EM_SETSEL, len, len);
    std::wstring msg = line + L"\r\n";
    SendMessageW(m_hwnd, EM_REPLACESEL, 0, (LPARAM)msg.c_str());
}
bool TextBox::HandleCommand(int code) {
    if (code == EN_CHANGE && m_onChange) { m_onChange(GetText()); return true; }
    return false;
}

ComboBox::ComboBox(HWND parent, int id, int w, int dropdownH) {
    // The closed combobox face is visually fixed-height; dropdownH controls list popup area
    m_id = id; m_prefW = w; m_prefH = 22;
    m_hwnd = CreateClassicControl(WS_EX_CLIENTEDGE, L"COMBOBOX", parent, L"",
                                   CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, id, 0, 0, w, dropdownH);
}
void ComboBox::AddItem(const std::wstring& text) {
    SendMessageW(m_hwnd, CB_ADDSTRING, 0, (LPARAM)text.c_str());
    if (SendMessageW(m_hwnd, CB_GETCOUNT, 0, 0) == 1)
        SendMessageW(m_hwnd, CB_SETCURSEL, 0, 0);
}
void ComboBox::SetSelection(int index) { SendMessageW(m_hwnd, CB_SETCURSEL, index, 0); }
bool ComboBox::HandleCommand(int code) {
    if (code == CBN_SELCHANGE && m_onChange) {
        int idx = (int)SendMessageW(m_hwnd, CB_GETCURSEL, 0, 0);
        wchar_t buf[256] = {};
        SendMessageW(m_hwnd, CB_GETLBTEXT, idx, (LPARAM)buf);
        m_onChange(idx, buf);
        return true;
    }
    return false;
}

Slider::Slider(HWND parent, int id, int w, int h) {
    m_id = id; m_prefW = w; m_prefH = h;
    m_hwnd = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBS_HORZ | TBS_AUTOTICKS | WS_TABSTOP,
                              0, 0, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);
    SetWindowTheme(m_hwnd, L" ", L" ");
    SendMessageW(m_hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageW(m_hwnd, TBM_SETPOS,   TRUE, 50);
}
void Slider::SetRange(int lo, int hi) { SendMessageW(m_hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(lo, hi)); }
int  Slider::GetValue() const { return (int)SendMessageW(m_hwnd, TBM_GETPOS, 0, 0); }
bool Slider::HandleScroll(UINT /*msg*/, WPARAM wParam) {
    int val = GetValue();
    if (m_onChange)  m_onChange(val);
    if (LOWORD(wParam) == TB_ENDTRACK && m_onRelease) m_onRelease(val);
    return true;
}

ListBox::ListBox(HWND parent, int id, int w, int h) {
    m_id = id; m_prefW = w; m_prefH = h;
    m_hwnd = CreateClassicControl(WS_EX_CLIENTEDGE, L"LISTBOX", parent, L"",
                                   WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP, id, 0, 0, w, h);
}
void ListBox::AddItem(const std::wstring& text) { SendMessageW(m_hwnd, LB_ADDSTRING, 0, (LPARAM)text.c_str()); }
bool ListBox::HandleCommand(int code) {
    if (code == LBN_SELCHANGE && m_onChange) {
        m_onChange((int)SendMessageW(m_hwnd, LB_GETCURSEL, 0, 0));
        return true;
    }
    return false;
}

GroupBox::GroupBox(HWND parent, const std::wstring& text, int id, int w, int h) {
    m_id = id; m_prefW = w; m_prefH = h;
    m_hwnd = CreateWindowExW(WS_EX_TRANSPARENT, L"BUTTON", text.c_str(),
                              BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
                              0, 0, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);
    SetWindowTheme(m_hwnd, L" ", L" ");
    HFONT hFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS Sans Serif");
    SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
}

// -----------------------------------------------------------------------------
// Layout base implementation
// -----------------------------------------------------------------------------
//
// Layout owns and arranges a linear list of LayoutItem entries. Concrete layout
// classes only implement Calculate(); all ownership and propagation logic lives
// in this shared base layer

void Layout::Show(bool show) {
    for (auto& item : m_items) {
        if (item.hwnd)        ShowWindow(item.hwnd, show ? SW_SHOWNA : SW_HIDE);
        if (item.childLayout) item.childLayout->Show(show);
    }
}

bool Layout::HandleNotify(LPNMHDR hdr) {
    for (auto& item : m_items)
        if (item.childLayout && item.childLayout->HandleNotify(hdr)) return true;
    return false;
}

void Layout::AddWidget(std::unique_ptr<Widget> w, bool expandH, bool expandV, int stretch) {
    AddWidget(std::move(w), 0, 0, expandH, expandV, stretch);
}

void Layout::AddWidget(std::unique_ptr<Widget> w, int minW, int minH, bool expandH, bool expandV, int stretch) {
    LayoutItem item;
    item.hwnd             = w->GetHWND();
    int prefW = w->GetPrefWidth(), prefH = w->GetPrefHeight();
    item.minWidth         = (minW > 0) ? minW : prefW;
    item.minHeight        = (minH > 0) ? minH : prefH;
    item.expandHorizontal = expandH;
    item.expandVertical   = expandV;
    item.stretch          = stretch;
    WCHAR cls[64];
    if (item.hwnd && GetClassNameW(item.hwnd, cls, 64) && _wcsicmp(cls, L"ComboBox") == 0)
        item.isComboBox = true;
    item.widget = std::move(w); // TAKE OWNERSHIP
    m_items.push_back(std::move(item));
}

void Layout::AddWidget(HWND hwnd, int minW, int minH, bool expandH, bool expandV, int stretch) {
    LayoutItem item;
    item.hwnd             = hwnd;
    item.minWidth         = minW;
    item.minHeight        = minH;
    item.expandHorizontal = expandH;
    item.expandVertical   = expandV;
    item.stretch          = stretch;
    WCHAR cls[64];
    if (hwnd && GetClassNameW(hwnd, cls, 64) && _wcsicmp(cls, L"ComboBox") == 0)
        item.isComboBox = true;
    m_items.push_back(std::move(item));
}

void Layout::AddWidget(const Widget& w, bool expandH, bool expandV, int stretch) {
    AddWidget(w.GetHWND(), w.GetPrefWidth(), w.GetPrefHeight(), expandH, expandV, stretch);
}

void Layout::AddLayout(std::unique_ptr<Layout> layout, int minW, int minH, bool expandH, bool expandV, int stretch) {
    LayoutItem item;
    item.minWidth         = minW;
    item.minHeight        = minH;
    item.expandHorizontal = expandH;
    item.expandVertical   = expandV;
    item.stretch          = stretch;
    layout->SetParentWindow(m_parent);
    layout->SetWindow(m_win); // Propagate window context
    item.childLayout      = std::move(layout);
    m_items.push_back(std::move(item));
}

void Layout::AddSpacer(int size, int stretch) {
    LayoutItem item;
    item.minWidth = item.minHeight = size;
    item.stretch          = stretch;
    item.expandHorizontal = true;
    item.expandVertical   = true;
    m_items.push_back(std::move(item));
}

void Layout::SetPadding(int p) { m_paddingLeft = m_paddingTop = m_paddingRight = m_paddingBottom = p; }
void Layout::SetPadding(int l, int t, int r, int b) { m_paddingLeft=l; m_paddingTop=t; m_paddingRight=r; m_paddingBottom=b; }
void Layout::SetSpacing(int s) { m_spacing = s; }

void Layout::SetParentWindow(HWND parent) {
    m_parent = parent;
    for (auto& item : m_items)
        if (item.childLayout) item.childLayout->SetParentWindow(parent);
}

void Layout::SetWindow(Window* win) {
    m_win = win;
    for (auto& item : m_items) {
        if (m_win && item.widget) m_win->RegisterWidget(item.widget.get());
        if (item.childLayout) item.childLayout->SetWindow(win);
    }
}

RECT Layout::GetPaddedRect(const RECT& rect) const {
    return { rect.left + m_paddingLeft, rect.top + m_paddingTop,
             rect.right - m_paddingRight, rect.bottom - m_paddingBottom };
}

void Layout::PlaceItem(LayoutItem& item, const RECT& r) {
    if (item.hwnd) {
        int h = r.bottom - r.top;
        // Native ComboBox dropdown list requires extra height to remain usable
        // The visible closed control remains normal-sized via Windows' own paint behavior
        if (item.isComboBox) h = std::max(h, 200);
        MoveWindow(item.hwnd, r.left, r.top, r.right - r.left, h, TRUE);
    } else if (item.childLayout) {
        item.childLayout->Calculate(r);
    }
}

// -----------------------------------------------------------------------------
// VBox
// -----------------------------------------------------------------------------
//
// Vertical stack layout:
// - Non expanding items get minHeight
// - Expanding items share remaining height by stretch factor
// - Width can optionally expand to full row width per item

void VBox::Calculate(const RECT& availableRect) {
    RECT work = GetPaddedRect(availableRect);
    int availH = work.bottom - work.top;
    int availW = work.right  - work.left;
    int spacingTotal = ((int)m_items.size() > 1) ? ((int)m_items.size() - 1) * m_spacing : 0;

    int fixedH = spacingTotal, totalStretch = 0;
    for (const auto& item : m_items) {
        if (!item.expandVertical || item.stretch == 0) fixedH += item.minHeight;
        else totalStretch += (item.stretch > 0) ? item.stretch : 1;
    }

    int expandH = std::max(0, availH - fixedH);
    int curY    = work.top;

    for (auto& item : m_items) {
        int h = (item.expandVertical && totalStretch > 0)
              ? std::max(item.minHeight, expandH * ((item.stretch>0)?item.stretch:1) / totalStretch)
              : item.minHeight;
        int w = item.expandHorizontal ? availW : item.minWidth;
        RECT r = { work.left, curY, work.left + w, curY + h };
        PlaceItem(item, r);
        curY += h + m_spacing;
    }
}

// -----------------------------------------------------------------------------
// HBox
// -----------------------------------------------------------------------------
//
// Horizontal counterpart to VBox using the same expansion/stretch logic

void HBox::Calculate(const RECT& availableRect) {
    RECT work = GetPaddedRect(availableRect);
    int availW = work.right - work.left;
    int availH = work.bottom - work.top;
    int spacingTotal = ((int)m_items.size() > 1) ? ((int)m_items.size() - 1) * m_spacing : 0;

    int fixedW = spacingTotal, totalStretch = 0;
    for (const auto& item : m_items) {
        if (!item.expandHorizontal || item.stretch == 0) fixedW += item.minWidth;
        else totalStretch += (item.stretch > 0) ? item.stretch : 1;
    }

    int expandW = std::max(0, availW - fixedW);
    int curX    = work.left;

    for (auto& item : m_items) {
        int w = (item.expandHorizontal && totalStretch > 0)
              ? std::max(item.minWidth, expandW * ((item.stretch>0)?item.stretch:1) / totalStretch)
              : item.minWidth;
        int h = item.expandVertical ? availH : item.minHeight;
        RECT r = { curX, work.top, curX + w, work.top + h };
        PlaceItem(item, r);
        curX += w + m_spacing;
    }
}

// -----------------------------------------------------------------------------
// GridLayout
// -----------------------------------------------------------------------------
//
// Table-style layout with optional auto row count
// - Supports row/column spans
// - Computes per column/per row baseline from minimum sizes
// - Distributes extra free space evenly across columns and rows

GridLayout::GridLayout(int columns, int rows, HWND parent)
    : m_columns(columns), m_rows(rows) { m_parent = parent; }

void GridLayout::AddWidget(std::unique_ptr<Widget> w, int col, int row, int colspan, int rowspan) {
    LayoutItem item;
    item.hwnd = w->GetHWND();
    item.gridColumn = col; item.gridRow = row;
    item.gridColumnSpan = colspan; item.gridRowSpan = rowspan;
    item.minWidth = w->GetPrefWidth(); item.minHeight = w->GetPrefHeight();
    WCHAR cls[64];
    if (item.hwnd && GetClassNameW(item.hwnd, cls, 64) && _wcsicmp(cls, L"ComboBox") == 0) item.isComboBox = true;
    item.widget = std::move(w); // TAKE OWNERSHIP
    m_items.push_back(std::move(item));
}

void GridLayout::AddWidget(HWND hwnd, int col, int row, int colspan, int rowspan) {
    LayoutItem item;
    item.hwnd = hwnd;
    item.gridColumn = col; item.gridRow = row;
    item.gridColumnSpan = colspan; item.gridRowSpan = rowspan;
    RECT rc; GetWindowRect(hwnd, &rc);
    item.minWidth = rc.right - rc.left; item.minHeight = rc.bottom - rc.top;
    WCHAR cls[64];
    if (hwnd && GetClassNameW(hwnd, cls, 64) && _wcsicmp(cls, L"ComboBox") == 0) item.isComboBox = true;
    m_items.push_back(std::move(item));
}

void GridLayout::AddWidget(const Widget& w, int col, int row, int colspan, int rowspan) {
    AddWidget(w.GetHWND(), col, row, colspan, rowspan);
}

void GridLayout::AddLayout(std::unique_ptr<Layout> layout, int col, int row, int colspan, int rowspan) {
    LayoutItem item;
    item.gridColumn = col; item.gridRow = row;
    item.gridColumnSpan = colspan; item.gridRowSpan = rowspan;
    layout->SetParentWindow(m_parent);
    item.childLayout = std::move(layout);
    m_items.push_back(std::move(item));
}

void GridLayout::CalculateGridMetrics(const RECT& availableRect, std::vector<int>& colW, std::vector<int>& rowH) {
    RECT work = GetPaddedRect(availableRect);
    int availW = work.right - work.left, availH = work.bottom - work.top;
    int actualRows = m_rows;
    if (m_rows == -1) { actualRows = 0; for (const auto& i : m_items) actualRows = std::max(actualRows, i.gridRow + i.gridRowSpan); }
    colW.assign(m_columns, 0); rowH.assign(actualRows, 0);
    for (const auto& item : m_items) {
        if (item.gridColumnSpan == 1) colW[item.gridColumn] = std::max(colW[item.gridColumn], item.minWidth);
        if (item.gridRowSpan    == 1) rowH[item.gridRow]    = std::max(rowH[item.gridRow],    item.minHeight);
    }
    int totalW = (m_columns - 1) * m_spacing, totalH = (actualRows - 1) * m_spacing;
    for (int w : colW) totalW += w; for (int h : rowH) totalH += h;
    int extraW = std::max(0, availW - totalW), extraH = std::max(0, availH - totalH);
    if (m_columns > 0) { int dw = extraW / m_columns; for (int& w : colW) w += dw; }
    if (actualRows > 0) { int dh = extraH / actualRows; for (int& h : rowH) h += dh; }
}

void GridLayout::Calculate(const RECT& availableRect) {
    if (m_items.empty() || m_columns == 0) return;
    std::vector<int> colW, rowH;
    CalculateGridMetrics(availableRect, colW, rowH);
    RECT work = GetPaddedRect(availableRect);
    for (auto& item : m_items) {
        int x = work.left; for (int i = 0; i < item.gridColumn; i++) x += colW[i] + m_spacing;
        int y = work.top;  for (int i = 0; i < item.gridRow;    i++) y += rowH[i] + m_spacing;
        int w = 0; for (int i = 0; i < item.gridColumnSpan; i++) { if (item.gridColumn+i < (int)colW.size()) { w += colW[item.gridColumn+i]; if(i>0) w+=m_spacing; } }
        int h = 0; for (int i = 0; i < item.gridRowSpan;    i++) { if (item.gridRow+i    < (int)rowH.size()) { h += rowH[item.gridRow+i];    if(i>0) h+=m_spacing; } }
        RECT r = { x, y, x + w, y + h };
        PlaceItem(item, r);
    }
}

// -----------------------------------------------------------------------------
// Panel
// -----------------------------------------------------------------------------
//
// Optional GroupBox wrapper over a child layout
// When titled, panel reserves extra top inset so child controls do not overlap
// the group caption line

Panel::Panel(HWND parent, const std::wstring& title, int id) : m_title(title) {
    m_parent = parent;
    if (!title.empty() && parent) {
        GroupBox gb(parent, title, id, 100, 100);
        m_groupBox = gb.GetHWND();
        SetPadding(15, 35, 15, 15);
    }
}

void Panel::SetChildLayout(std::unique_ptr<Layout> layout) {
    m_childLayout = std::move(layout);
    if (m_childLayout) {
        m_childLayout->SetParentWindow(m_parent);
        m_childLayout->SetWindow(m_win);
    }
}

void Panel::SetParentWindow(HWND parent) {
    Layout::SetParentWindow(parent);
    if (m_childLayout) m_childLayout->SetParentWindow(parent);
}

void Panel::SetWindow(Window* win) {
    Layout::SetWindow(win);
    if (m_childLayout) m_childLayout->SetWindow(win);
}

void Panel::Calculate(const RECT& r) {
    if (m_groupBox) MoveWindow(m_groupBox, r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE);
    if (m_childLayout) m_childLayout->Calculate(GetPaddedRect(r));
}

void Panel::Show(bool show) {
    if (m_groupBox)   ShowWindow(m_groupBox, show ? SW_SHOWNA : SW_HIDE);
    if (m_childLayout) m_childLayout->Show(show);
}

bool Panel::HandleNotify(LPNMHDR hdr) {
    return m_childLayout ? m_childLayout->HandleNotify(hdr) : false;
}

// -----------------------------------------------------------------------------
// TabPanel
// -----------------------------------------------------------------------------
//
// Native tab control container with one Layout subtree per tab page
// Only active page is visible and laid out to avoid input/paint overlap

TabPanel::TabPanel(HWND parent, int id) : m_activeTab(0) {
    m_parent = parent;
    m_tabControl = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_FOCUSNEVER,
                                   0, 0, 100, 100, parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);
    SetWindowTheme(m_tabControl, L" ", L" ");
    HFONT hFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS Sans Serif");
    SendMessageW(m_tabControl, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
}

void TabPanel::AddTab(const std::wstring& title, std::unique_ptr<Layout> layout) {
    TCITEMW tci = {}; tci.mask = TCIF_TEXT; tci.pszText = (LPWSTR)title.c_str();
    SendMessageW(m_tabControl, TCM_INSERTITEMW, m_tabLayouts.size(), (LPARAM)&tci);
    layout->SetParentWindow(m_parent);
    layout->SetWindow(m_win);
    layout->Show(m_tabLayouts.empty()); // show first tab only
    m_tabLayouts.push_back(std::move(layout));
}

void TabPanel::SetParentWindow(HWND parent) {
    Layout::SetParentWindow(parent);
    for (auto& layout : m_tabLayouts)
        if (layout) layout->SetParentWindow(parent);
}

void TabPanel::SetWindow(Window* win) {
    Layout::SetWindow(win);
    for (auto& layout : m_tabLayouts)
        if (layout) layout->SetWindow(win);
}

void TabPanel::Calculate(const RECT& availableRect) {
    if (!m_tabControl) return;
    SetWindowPos(m_tabControl, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    MoveWindow(m_tabControl, availableRect.left, availableRect.top,
               availableRect.right - availableRect.left, availableRect.bottom - availableRect.top, TRUE);
    RECT displayRect = availableRect;
    SendMessageW(m_tabControl, TCM_ADJUSTRECT, FALSE, (LPARAM)&displayRect);
    displayRect.left += 5; displayRect.top += 5; displayRect.right -= 5; displayRect.bottom -= 5;
    if (m_activeTab >= 0 && m_activeTab < (int)m_tabLayouts.size() && m_tabLayouts[m_activeTab])
        m_tabLayouts[m_activeTab]->Calculate(displayRect);
}

void TabPanel::Show(bool show) {
    if (m_tabControl) ShowWindow(m_tabControl, show ? SW_SHOWNA : SW_HIDE);
    if (m_activeTab >= 0 && m_activeTab < (int)m_tabLayouts.size())
        m_tabLayouts[m_activeTab]->Show(show);
}

bool TabPanel::HandleNotify(LPNMHDR hdr) {
    if (hdr->hwndFrom == m_tabControl && hdr->code == TCN_SELCHANGE) {
        int newTab = (int)SendMessageW(m_tabControl, TCM_GETCURSEL, 0, 0);
        if (newTab != m_activeTab) {
            if (m_activeTab >= 0 && m_activeTab < (int)m_tabLayouts.size())
                m_tabLayouts[m_activeTab]->Show(false);
            m_activeTab = newTab;
            if (m_activeTab >= 0 && m_activeTab < (int)m_tabLayouts.size()) {
                m_tabLayouts[m_activeTab]->Show(true);
                RECT displayRect; GetWindowRect(m_tabControl, &displayRect);
                MapWindowPoints(NULL, m_parent, (LPPOINT)&displayRect, 2);
                SendMessageW(m_tabControl, TCM_ADJUSTRECT, FALSE, (LPARAM)&displayRect);
                displayRect.left += 5; displayRect.top += 5; displayRect.right -= 5; displayRect.bottom -= 5;
                m_tabLayouts[m_activeTab]->Calculate(displayRect);
            }
        }
        return true;
    }
    for (auto& layout : m_tabLayouts) if (layout && layout->HandleNotify(hdr)) return true;
    return false;
}

// -----------------------------------------------------------------------------
// Window implementation
// -----------------------------------------------------------------------------
//
// Handles class registration, non client emulation, custom frame painting,
// widget message dispatch, and root layout updates

// Register a single custom top level class used by all w98::Window instances
bool Initialize(HINSTANCE hInstance) {
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);
    WNDCLASSEXW wc    = {};
    wc.cbSize         = sizeof(wc);
    wc.lpfnWndProc    = Window::WindowProc;
    wc.hInstance      = hInstance;
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName  = CLASS_NAME;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    return RegisterClassExW(&wc) != 0;
}

Window::Window(const std::wstring& title, int x, int y, int width, int height) : m_title(title) {
    m_hwnd = CreateWindowExW(WS_EX_APPWINDOW, CLASS_NAME, title.c_str(),
                              WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CLIPCHILDREN,
                              x, y, width, height, NULL, NULL, GetModuleHandle(NULL), this);
    SetWindowTheme(m_hwnd, L" ", L" ");
}

Window::~Window() {
    if (m_hwnd) DestroyWindow(m_hwnd);
}

void Window::Show(int nCmdShow) { ShowWindow(m_hwnd, nCmdShow); }
void Window::Update() { UpdateWindow(m_hwnd); }

void Window::RegisterWidget(Widget* w) {
    if (!w) return;
    m_widgetsById[w->GetId()] = w;
    m_widgetsByHwnd[w->GetHWND()] = w;
}

void Window::SetLayout(std::unique_ptr<Layout> layout) {
    m_rootLayout = std::move(layout);
    if (m_rootLayout) {
        m_rootLayout->SetParentWindow(m_hwnd);
        m_rootLayout->SetWindow(this);
    }
}

RECT Window::GetClientArea() const {
    RECT rc; GetClientRect(m_hwnd, &rc);
    rc.left += BORDER_THICKNESS; rc.top += BORDER_THICKNESS + TITLE_BAR_HEIGHT;
    rc.right -= BORDER_THICKNESS; rc.bottom -= BORDER_THICKNESS;
    return rc;
}

void Window::UpdateLayout() {
    if (m_rootLayout && m_hwnd && IsWindow(m_hwnd)) {
        RECT ca = GetClientArea();
        if (ca.right > ca.left && ca.bottom > ca.top) m_rootLayout->Calculate(ca);
    }
}

// Static bridge: retrieve the C++ object pointer from GWLP_USERDATA and forward
LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pThis;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Window::UpdateMetrics(int width, int height) {
    // Precompute caption and button hit rectangles for painting and hit testing
    m_titleRect = { BORDER_THICKNESS, BORDER_THICKNESS, width - BORDER_THICKNESS, BORDER_THICKNESS + TITLE_BAR_HEIGHT };
    m_closeRect = { m_titleRect.right - CAPTION_BUTTON_SIZE - 2, m_titleRect.top + 2,
                    m_titleRect.right - 2, m_titleRect.top + 2 + CAPTION_BUTTON_SIZE };
    m_maxRect = m_closeRect; OffsetRect(&m_maxRect, -(CAPTION_BUTTON_SIZE + 2), 0);
    m_minRect = m_maxRect;   OffsetRect(&m_minRect, -CAPTION_BUTTON_SIZE, 0);
}

void Window::DrawTitleBarBackground(HDC hdc, const RECT& rect, bool active) {
    COLORREF bg = active ? RGB(0, 0, 128) : RGB(128, 128, 128);
    HBRUSH br = CreateSolidBrush(bg); FillRect(hdc, &rect, br); DeleteObject(br);
}

void Window::DrawWin98Frame(HDC hdc, int width, int height) {
    // Draw raised outer frame and inset interior edges to mimic Win98 chrome
    RECT wndRect = { 0, 0, width, height };
    DrawEdge(hdc, &wndRect, EDGE_RAISED, BF_RECT);
    HBRUSH face = GetSysColorBrush(COLOR_BTNFACE);
    RECT inner = wndRect; InflateRect(&inner, -2, -2); FrameRect(hdc, &inner, face);
    RECT inner2 = inner;   InflateRect(&inner2, -1, -1); FrameRect(hdc, &inner2, face);
    DrawTitleBarBackground(hdc, m_titleRect, m_isActive);
    HFONT hFont = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS Sans Serif");
    HFONT old = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, m_isActive ? RGB(255,255,255) : RGB(192,192,192));
    RECT textRect = m_titleRect; textRect.left += 4; textRect.right = m_minRect.left - 2;
    DrawTextW(hdc, m_title.c_str(), -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(hdc, old); DeleteObject(hFont);
    UINT closeState = DFCS_CAPTIONCLOSE; if (m_closeDown && m_mouseInClose) closeState |= DFCS_PUSHED;
    RECT cr = m_closeRect; DrawFrameControl(hdc, &cr, DFC_CAPTION, closeState);
    UINT maxState = m_isMaximized ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX; if (m_maxDown && m_mouseInMax) maxState |= DFCS_PUSHED;
    RECT mr = m_maxRect; DrawFrameControl(hdc, &mr, DFC_CAPTION, maxState);
    UINT minState = DFCS_CAPTIONMIN; if (m_minDown && m_mouseInMin) minState |= DFCS_PUSHED;
    RECT mnr = m_minRect; DrawFrameControl(hdc, &mnr, DFC_CAPTION, minState);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Route control originated messages before window level processing
    if (uMsg == WM_COMMAND) {
        int id = LOWORD(wParam), code = HIWORD(wParam);
        auto it = m_widgetsById.find(id);
        if (it != m_widgetsById.end()) it->second->HandleCommand(code);
    } else if (uMsg == WM_HSCROLL || uMsg == WM_VSCROLL) {
        HWND ctrl = (HWND)lParam;
        auto it = m_widgetsByHwnd.find(ctrl);
        if (it != m_widgetsByHwnd.end()) it->second->HandleScroll(uMsg, wParam);
    }
    // Let application code observe/intercept raw messages
    if (m_msgCallback) m_msgCallback(m_hwnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        m_isMaximized = (wParam == SIZE_MAXIMIZED);
        UpdateMetrics(w, h); UpdateLayout();
        InvalidateRect(m_hwnd, NULL, FALSE); return 0;
    }
    case WM_PAINT: {
        // Use a memory DC for flicker free custom frame painting
        PAINTSTRUCT ps; HDC hdc = BeginPaint(m_hwnd, &ps);
        RECT rc; GetClientRect(m_hwnd, &rc);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP old = (HBITMAP)SelectObject(hdcMem, bmp);
        FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BTNFACE));
        DrawWin98Frame(hdcMem, rc.right, rc.bottom);
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, old); DeleteObject(bmp); DeleteDC(hdcMem);
        EndPaint(m_hwnd, &ps); return 0;
    }
    case WM_NOTIFY:
        if (m_rootLayout && m_rootLayout->HandleNotify((LPNMHDR)lParam)) return 0;
        break;
    case WM_ACTIVATE:
        m_isActive = (LOWORD(wParam) != WA_INACTIVE);
        InvalidateRect(m_hwnd, &m_titleRect, FALSE); return 0;
    case WM_NCPAINT: return 0;
    case WM_NCACTIVATE:
        m_isActive = (wParam != 0);
        InvalidateRect(m_hwnd, &m_titleRect, FALSE); return TRUE;
    case WM_NCCALCSIZE:
        if (wParam == TRUE) return 0;
        break;
    case WM_NCHITTEST: {
        // Recreate non client sizing behavior for our custom frame
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(m_hwnd, &pt);
        RECT rc; GetClientRect(m_hwnd, &rc);
        bool isL = pt.x < BORDER_THICKNESS, isR = pt.x >= rc.right - BORDER_THICKNESS;
        bool isT = pt.y < BORDER_THICKNESS, isB = pt.y >= rc.bottom - BORDER_THICKNESS;
        if (isT && isL) return HTTOPLEFT;  if (isT && isR) return HTTOPRIGHT;
        if (isB && isL) return HTBOTTOMLEFT; if (isB && isR) return HTBOTTOMRIGHT;
        if (isT) return HTTOP; if (isB) return HTBOTTOM;
        if (isL) return HTLEFT; if (isR) return HTRIGHT;
        if (PtInRect(&m_titleRect, pt) && !PtInRect(&m_closeRect, pt) &&
            !PtInRect(&m_maxRect, pt) && !PtInRect(&m_minRect, pt)) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_LBUTTONDOWN: {
        // Capture mouse while pressing caption buttons for correct pressed state visuals
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (PtInRect(&m_closeRect, pt)) { m_closeDown=true; m_mouseInClose=true; SetCapture(m_hwnd); InvalidateRect(m_hwnd,&m_closeRect,FALSE); return 0; }
        if (PtInRect(&m_maxRect,   pt)) { m_maxDown=true;   m_mouseInMax=true;   SetCapture(m_hwnd); InvalidateRect(m_hwnd,&m_maxRect,FALSE);   return 0; }
        if (PtInRect(&m_minRect,   pt)) { m_minDown=true;   m_mouseInMin=true;   SetCapture(m_hwnd); InvalidateRect(m_hwnd,&m_minRect,FALSE);   return 0; }
        break;
    }
    case WM_MOUSEMOVE:
        if (GetCapture() == m_hwnd) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (m_closeDown) { bool in=PtInRect(&m_closeRect,pt); if(in!=m_mouseInClose){m_mouseInClose=in;InvalidateRect(m_hwnd,&m_closeRect,FALSE);} }
            if (m_maxDown)   { bool in=PtInRect(&m_maxRect,pt);   if(in!=m_mouseInMax)  {m_mouseInMax=in;  InvalidateRect(m_hwnd,&m_maxRect,FALSE);  } }
            if (m_minDown)   { bool in=PtInRect(&m_minRect,pt);   if(in!=m_mouseInMin)  {m_mouseInMin=in;  InvalidateRect(m_hwnd,&m_minRect,FALSE);  } }
        }
        break;
    case WM_LBUTTONUP:
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // Trigger action only when release occurs over the same button
            if (m_closeDown && PtInRect(&m_closeRect, pt)) PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            if (m_maxDown   && PtInRect(&m_maxRect,   pt)) ShowWindow(m_hwnd, m_isMaximized ? SW_RESTORE : SW_MAXIMIZE);
            if (m_minDown   && PtInRect(&m_minRect,   pt)) ShowWindow(m_hwnd, SW_MINIMIZE);
            m_closeDown=false; m_mouseInClose=false;
            m_maxDown=false;   m_mouseInMax=false;
            m_minDown=false;   m_mouseInMin=false;
            InvalidateRect(m_hwnd, &m_titleRect, FALSE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

}
