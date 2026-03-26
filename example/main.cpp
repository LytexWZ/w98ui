#include "w98ui.h"

// -----------------------------------------------------------------------------
// Index of this file:
// - Demo overview and execution flow
// - Controller module (UI reaction logic)
// - Window + root layout setup
// - General tab construction
// - Display tab construction
// - Shared log/action row construction
// - Event wiring
// - Message loop
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Demo overview
// -----------------------------------------------------------------------------
//
// This file demonstrates typical usage of the w98 library:
// 1) Initialize w98 globals and register the custom window class.
// 2) Build a layout tree using VBox/HBox/Panel/TabPanel.
// 3) Keep behavior in a small controller object.
// 4) Attach callbacks from widgets to controller methods.
// 5) Submit layout to the window and run standard Win32 message pumping.

// -----------------------------------------------------------------------------
// Controller module
// -----------------------------------------------------------------------------
//
// AppController centralizes side effects so widget construction remains focused
// on structure/layout. It does not own controls; it only references widgets
// that need runtime updates (log output and FOV label text).
struct AppController {
    w98::TextBox& log;
    w98::Label&   slider1Label;
    w98::Label&   slider2Label;

    void OnCheckBox1(bool on)     { log.AppendLine(on ? L"Checkbox 1: Enabled" : L"Checkbox 1: Disabled"); }
    void OnCheckBox2(bool on)     { log.AppendLine(on ? L"Checkbox 2: Enabled" : L"Checkbox 2: Disabled"); }
    void OnTextBox1(const std::wstring& t) { log.AppendLine(L"TextBox 1: " + t); }
    void OnComboBox1(int, const std::wstring& v) { log.AppendLine(L"ComboBox 1: " + v); }
    void OnComboBox2(int, const std::wstring& v) { log.AppendLine(L"ComboBox 2: " + v); }
    void OnSlider1Move(int v)     { slider1Label.SetText(L"Slider 1: " + std::to_wstring(v)); }
    void OnSlider1Done(int v)     { log.AppendLine(L"Slider 1 set to: " + std::to_wstring(v)); }
    void OnSlider2Move(int v)     { slider2Label.SetText(L"Slider 2: " + std::to_wstring(v)); }
    void OnSlider2Done(int v)     { log.AppendLine(L"Slider 2 set to: " + std::to_wstring(v)); }
    void OnButton1()              { log.AppendLine(L"Button 1 pressed"); }
    void OnButton2()              { log.AppendLine(L"Button 2 pressed"); }
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // -------------------------------------------------------------------------
    // Library and window initialization
    // -------------------------------------------------------------------------
    // Register control classes/theme setup used by all windows and controls.
    w98::Initialize(hInstance);

    // Main host window. All child controls are parented under this HWND.
    w98::Window win(L"Win98UI Showcase", 100, 100, 520, 480);
    HWND hwnd = win.GetHWND();

    // -------------------------------------------------------------------------
    // Root layout
    // -------------------------------------------------------------------------
    // This is the top-level vertical flow:
    // [Tab panel] -> [Activity label] -> [Log box] -> [Buttons row]
    auto mainLayout = std::make_unique<w98::VBox>(hwnd);
    mainLayout->SetPadding(15);
    mainLayout->SetSpacing(10);

    // -------------------------------------------------------------------------
    // General tab
    // -------------------------------------------------------------------------
    // Demonstrates mixed controls: checkbox, edit, combo, slider, and labels.
    auto generalTab = std::make_unique<w98::VBox>(hwnd);
    generalTab->SetPadding(10);

    auto configPanel  = std::make_unique<w98::Panel>(hwnd, L"", 200);
    auto configLayout = std::make_unique<w98::VBox>(hwnd);
    configLayout->SetSpacing(10);

    // Feature toggles.
    auto& check1 = configLayout->Add<w98::CheckBox>(L"Checkbox 1", 101, 250, 20);
    auto& check2 = configLayout->Add<w98::CheckBox>(L"Checkbox 2", 102, 250, 20);

    // Two-column row: profile text on left, aim target combo on right.
    auto row1     = std::make_unique<w98::HBox>(hwnd);
    row1->SetSpacing(15);
    auto leftCol  = std::make_unique<w98::VBox>(hwnd);
    leftCol->SetSpacing(5);
    auto rightCol = std::make_unique<w98::VBox>(hwnd);
    rightCol->SetSpacing(5);

    leftCol->Add<w98::Label>(L"TextBox 1:", 103, 100, 15);
    auto& userText = leftCol->AddExpanded<w98::TextBox>(true, false, 0, L"LytexWZ", 104, 150, 22);

    rightCol->Add<w98::Label>(L"ComboBox 1:", 109, 100, 15);
    auto& aimCombo = rightCol->AddExpanded<w98::ComboBox>(true, false, 0, 110, 200, 200);
    aimCombo.AddItem(L"Item 1"); aimCombo.AddItem(L"Item 2"); aimCombo.AddItem(L"Item 3");

    row1->AddLayout(std::move(leftCol),  0, 42, true, false, 1);
    row1->AddLayout(std::move(rightCol), 0, 42, true, false, 1);
    configLayout->AddLayout(std::move(row1), 0, 42, true, false);

    auto& fovLabel  = configLayout->Add<w98::Label>(L"Slider 1: 50", 105, 200, 15);
    auto& fovSlider = configLayout->AddExpanded<w98::Slider>(true, false, 0, 106, 300, 30);

    // Place content into panel and then panel into tab page.
    configPanel->SetChildLayout(std::move(configLayout));
    generalTab->AddLayout(std::move(configPanel), 0, 217, true, false);

    // Flexible spacer keeps controls at top and consumes extra vertical room.
    generalTab->AddSpacer(0, 1);

    // -------------------------------------------------------------------------
    // Display tab
    // -------------------------------------------------------------------------
    // Demonstrates side-by-side panels and horizontal sharing of available width.
    auto displayTab      = std::make_unique<w98::VBox>(hwnd);
    displayTab->SetPadding(10);
    auto horizontalShare = std::make_unique<w98::HBox>(hwnd);
    horizontalShare->SetSpacing(10);

    auto colorsPanel = std::make_unique<w98::Panel>(hwnd, L"Colors", 301);
    auto colorsVBox  = std::make_unique<w98::VBox>(hwnd);
    auto& comboColors = colorsVBox->AddExpanded<w98::ComboBox>(true, false, 0, 302, 150, 200);
    comboColors.AddItem(L"Option 1"); comboColors.AddItem(L"Option 2"); comboColors.AddItem(L"Option 3");
    colorsVBox->Add<w98::Label>(L"ComboBox 2 sample options.", 303, 150, 40);
    colorsPanel->SetChildLayout(std::move(colorsVBox));
    horizontalShare->AddLayout(std::move(colorsPanel), 0, 100, true, false, 1);

    auto screenPanel = std::make_unique<w98::Panel>(hwnd, L"Screen area", 304);
    auto screenVBox  = std::make_unique<w98::VBox>(hwnd);
    // Slider expands to panel width to demonstrate AddExpanded behavior.
    auto& slider2 = screenVBox->AddExpanded<w98::Slider>(true, false, 0, 305, 150, 30);
    auto& slider2Label = screenVBox->Add<w98::Label>(L"Slider 2: 50", 306, 150, 15);
    screenPanel->SetChildLayout(std::move(screenVBox));
    horizontalShare->AddLayout(std::move(screenPanel), 0, 100, true, false, 1);

    displayTab->AddLayout(std::move(horizontalShare), 0, 100, true, false);
    displayTab->AddSpacer(0, 1);

    // -------------------------------------------------------------------------
    // Main window assembly
    // -------------------------------------------------------------------------
    // Tab panel plus shared log/output section beneath tabs.
    auto tabs = std::make_unique<w98::TabPanel>(hwnd, 300);
    tabs->AddTab(L"Controls", std::move(generalTab));
    tabs->AddTab(L"Layout",   std::move(displayTab));
    mainLayout->AddLayout(std::move(tabs), 0, 260, true, false);

    mainLayout->Add<w98::Label>(L"Event Log:", 107, 100, 15);
    auto& log = mainLayout->AddExpanded<w98::TextBox>(true, true, 1, 
        L"Showcase initialized.\r\nInteract with controls to see events.\r\n", 108, 100, 80, true);

    auto buttonRow = std::make_unique<w98::HBox>(hwnd);
    buttonRow->SetSpacing(5);
    // Right-align action buttons using a stretch spacer in front.
    buttonRow->AddSpacer(0, 1);
    auto& discardBtn = buttonRow->Add<w98::Button>(L"Button 2", 112, 80, 25);
    auto& saveBtn    = buttonRow->Add<w98::Button>(L"Button 1", 111, 80, 25);
    mainLayout->AddLayout(std::move(buttonRow), 0, 25, true, false);

    // -------------------------------------------------------------------------
    // Event wiring
    // -------------------------------------------------------------------------
    // Connect control events to controller callbacks.
    // Lambdas keep wiring local while behavior remains centralized in AppController.
    AppController ctrl{ log, fovLabel, slider2Label };
    check1  .OnChange([&ctrl](bool on) { ctrl.OnCheckBox1(on); });
    check2  .OnChange([&ctrl](bool on) { ctrl.OnCheckBox2(on); });
    userText.OnChange([&ctrl](const std::wstring& t) { ctrl.OnTextBox1(t); });
    aimCombo.OnChange([&ctrl](int i, const std::wstring& v) { ctrl.OnComboBox1(i, v); });
    comboColors.OnChange([&ctrl](int i, const std::wstring& v) { ctrl.OnComboBox2(i, v); });
    fovSlider.OnChange ([&ctrl](int v) { ctrl.OnSlider1Move(v); });
    fovSlider.OnRelease([&ctrl](int v) { ctrl.OnSlider1Done(v); });
    slider2.OnChange ([&ctrl](int v) { ctrl.OnSlider2Move(v); });
    slider2.OnRelease([&ctrl](int v) { ctrl.OnSlider2Done(v); });
    saveBtn  .OnClick  ([&ctrl] { ctrl.OnButton1(); });
    discardBtn.OnClick ([&ctrl] { ctrl.OnButton2(); });

    // Submit fully-constructed widget/layout tree to the host window.
    win.SetLayout(std::move(mainLayout));
    
    win.Show(nCmdShow);
    win.Update();

    // -------------------------------------------------------------------------
    // Message loop
    // -------------------------------------------------------------------------
    // Standard Win32 dispatch loop. Terminates when WM_QUIT is posted.
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
