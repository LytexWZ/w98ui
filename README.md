# Win98UI Library

A lightweight, open-source C++ library for building authentic **Windows 98-style** desktop GUIs using the native Win32 API.

No external dependencies. Pure Win32. Drop in your CMake project and go.

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

## License

MIT Build something beautiful and classic.
