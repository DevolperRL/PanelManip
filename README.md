# PanelOverlay System

This project provides a system to add interactive overlay buttons to a panel in an X-Plane plugin. Buttons can have custom cursor images and respond to mouse events like clicks and scrolls.

---

## 📦 Features

- Add floating overlay buttons tied to panel coordinates.
- Custom cursor icons per button.
- Mouse event handling (click, hover, scroll).

---

## 🧩 How to Use

### Step 1: Create a Mouse Event Callback

You can use a lambda or any function that takes a `RO::MouseEvent` argument.

```
auto callback = [](RO::MouseEvent event) {
    switch (event) {
        case RO::MouseEvent::LeftClick:

            break;
        case RO::MouseEvent::Hover:

            break;
        case RO::MouseEvent::ScrollUp:

            break;
        case RO::MouseEvent::ScrollDown:

            break;
    }
};

string cursorPath = "Resources/cursors/my_cursor.png";

RO::PanelOverlay::AddButton(0.5, 0.5, .2, .2, cursorPath, callback);

```

