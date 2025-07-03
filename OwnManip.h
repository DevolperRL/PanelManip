#ifndef OWNMANIP_H
#define OWNMANIP_H

#if IBM
#include <windows.h>
#endif

#if LIN
#include <GL/gl.h>
#elif __GNUC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <XPLMGraphics.h>
#include <XPLMDisplay.h>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>

namespace RO {

    enum class MouseEvent {
        LeftClick,
        RightClick,
        ScrollUp,
        ScrollDown,
        Hover,
    };

    struct Button {
        float x, y, width, height;
        int cursorType;
        std::function<void(MouseEvent)> callback;
    };

    struct CursorInfo {
        int texture;
        int texWidth;
        int texHeight;
        float u1, v1, u2, v2;
        int hotspotX, hotspotY;
    };

    static std::unordered_map<std::string, CursorInfo> loadedCursors;
    static std::unordered_map<int, CursorInfo> cursorByID;
    static int nextCursorID = 1;

    class PanelOverlay {
    public:
        PanelOverlay();
        ~PanelOverlay();
        static void InitOverlayWindow();
        static void AddButton(float x, float y, float w, float h, const std::string& cursorPath, std::function<void(MouseEvent)> cb);
        static std::vector<Button> buttons;
        static float UpdateFunc(float elapsedSinceLastCall, float elapsedTimeSinceLastFlightLoop, int counter, void* refcon);
        int HandleRightMouseClick(int, int, XPLMMouseStatus status);
        int HandleMouseClick(int x, int y, XPLMMouseStatus status);
        int HandleCursor(int x, int y);
        int HandleScroll(int x, int y, int dx, int dy);
    private:
    };

}

#endif
