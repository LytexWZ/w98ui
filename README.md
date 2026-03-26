# Win98UI Library

A lightweight, open-source C++ library for building authentic **Windows 98-style** desktop GUIs using the native Win32 API.

No external dependencies. Pure Win32. Drop in your CMake project and go.

---

## Index

- Features
- Requirements
- CMake Instructions
- Quick Start
- Architecture Overview
- API Reference
- Project Structure
- Notes and Caveats
- License

---

## Features

- Custom Windows 98 titlebar, borders, and caption buttons (close / max / min)
- **Component-driven widget system** -> typed `Button`, `CheckBox`, `Slider`, `ComboBox`, `TextBox`, `Label`, `ListBox`, `GroupBox` classes
- **Contextual Layouts** -> `VBox`, `HBox`, etc. automatically handle parent HWND and Window registration. No verbose threading of pointers.
- **Hierarchical Ownership** -> widgets are owned by the layouts they belong to. No manual `delete` or complex lifetime management.
- **Declarative Style** -> `layout->Add<T>(...)` creates and adds widgets in one step.
- Automatic layout recalculation on resize
- CMake native build

---

## Requirements

- Windows SDK (any recent version)
- C++17 compiler (MSVC recommended)
- CMake 3.10+

---

## CMake Instructions

Build from the repository root:

```powershell
mkdir build
cd build
```

Build from inside the `build` directory:

```powershell
cmake ..
cmake --build . --config Release
```

Run the example app:

```powershell
cd build\example\Release
.\example_app.exe
```

Final executable output path:

- `w98\build\example\Release\example_app.exe`

If your generator supports configurations, switch `Release` to `Debug` while iterating.

---

## Quick Start

```cpp
#include "w98ui.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    w98::Initialize(hInstance);

    w98::Window window(L"My App", 100, 100, 400, 300);
    HWND hwnd = window.GetHWND();

    auto layout = std::make_unique<w98::VBox>(hwnd);
    layout->SetPadding(15);
    layout->SetSpacing(10);

    // No need to pass 'win' or 'hwnd'
    auto& label = layout->Add<w98::Label>(L"Hello World", 100, 100, 20);
    auto& btn   = layout->Add<w98::Button>(L"Click Me", 101, 100, 25);

    btn.OnClick([&] { label.SetText(L"Clicked!"); });

    window.SetLayout(std::move(layout));
    window.Show(nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
```

---

## Architecture Overview

### 1) Widget layer

All controls inherit from `w98::Widget`, which provides:

- `HWND` storage and ID tracking
- Preferred size metadata used by layouts
- Message hooks (`HandleCommand`, `HandleScroll`) for event dispatch

Concrete controls (`Button`, `CheckBox`, `TextBox`, `ComboBox`, etc.) wrap native Win32 controls and expose typed callbacks such as `OnClick` and `OnChange`.

### 2) Layout layer

`w98::Layout` owns and arranges `LayoutItem` entries:

- `VBox`: vertical stacking
- `HBox`: horizontal stacking
- `GridLayout`: table-style placement with row/column spans
- `Panel`: optional group box frame + child layout
- `TabPanel`: native tab control + per-tab layout pages

Layout options:

- `expandH`, `expandV`: allow item expansion on each axis
- `stretch`: proportional share of extra space among expandable peers
- `padding`, `spacing`: outer/inner rhythm controls

### 3) Window layer

`w98::Window` hosts the full UI tree and handles:

- Custom Win98 frame painting/title bar rendering
- Caption button interaction (close/max/min)
- Root layout calculation on resize
- Widget event dispatch from Win32 message traffic

---

## API Reference

### Layout System

Layouts manage the positioning and **ownership** of widgets and sub-layouts.

#### Adding Widgets

The `Add<T>` template is the primary way to create and add widgets. It automatically handles Win32 parent-child relationships and event registration.

```cpp
// 1. Standard: uses widget's preferred size
auto& btn = layout->Add<w98::Button>(L"OK", id, w, h);

// 2. Expanded: includes layout flags
auto& edit = layout->AddExpanded<w98::TextBox>(/*expandH=*/true, /*expandV=*/false, /*stretch=*/0, 
                                               L"Initial Text", id, w, h);
```

#### Layout Flags

| Parameter | Meaning |
|-----------|---------|
| `expandH` | Stretch to fill available width |
| `expandV` | Stretch to fill available height |
| `stretch` | Proportional share of extra space (0 = fixed) |

---

### Widget Types & Events

| Widget | Creator | Event Method | Signature |
|--------|---------|--------------|-----------|
| `Button` | `Add<w98::Button>` | `OnClick` | `void()` |
| `CheckBox` | `Add<w98::CheckBox>` | `OnChange` | `void(bool checked)` |
| `TextBox` | `Add<w98::TextBox>` | `OnChange` | `void(const std::wstring&)` |
| `ComboBox` | `Add<w98::ComboBox>` | `OnChange` | `void(int index, const std::wstring& value)` |
| `Slider` | `Add<w98::Slider>` | `OnChange` | `void(int value)` |
| `ListBox` | `Add<w98::ListBox>` | `OnChange` | `void(int index)` |

---

### Controller Pattern (Recommended)

To keep your code scalable, group event logic into a "Controller" struct:

```cpp
struct AppController {
    w98::Label& label;
    void OnClick() { label.SetText(L"Action Performed"); }
};

// ... inside WinMain ...
AppController ctrl{ myLabel };
myButton.OnClick([&ctrl]{ ctrl.OnClick(); });
```

---

## Project Structure

```text
include/
    w98ui.h        Public API declarations
src/
    w98ui.cpp      Library implementation
example/
    main.cpp       Demo app showing tabs, panels, events, and controller wiring
```

---

## Notes and Caveats

- This library targets Windows and native Win32 behavior.
- Fonts are configured for a classic look (`MS Sans Serif`) to better match Win98-era visuals.
- `TabPanel` and `Panel` are layout composites and are best used as structural containers in your layout tree.
- For larger apps, keep behavior logic in a controller/service layer and let layout code focus on structure.

---

## License

MIT Build something beautiful and classic.
