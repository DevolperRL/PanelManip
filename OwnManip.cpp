#include "OwnManip.h"
#include <XPLMUtilities.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMDataAccess.h>
#include <stb/stb_image.h>
#include <XPLMProcessing.h>

using namespace RO;
using namespace std;

XPLMWindowID overlayWindow = nullptr;
PanelOverlay* gOverlay = nullptr;
vector<Button> PanelOverlay::buttons;

static XPLMDataRef clickXRef = XPLMFindDataRef("sim/graphics/view/click_3d_x");
static XPLMDataRef clickYRef = XPLMFindDataRef("sim/graphics/view/click_3d_y");
static XPLMDataRef overPanelDtf = XPLMFindDataRef("sim/graphics/view/click_3d_x_pixels");

float mouseX = 0.0f;
float mouseY = 0.0f;

bool drawCreated = false;
bool overPanel = false;

void DrawCursorOverlay(XPLMWindowID, void*) {
    if (!gOverlay) return;

    const Button* hovered = nullptr;

    for (const auto& btn : gOverlay->buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            hovered = &btn;
            break;
        }
    }

    if (!hovered || hovered->cursorType == 0) return;

    auto it = cursorByID.find(hovered->cursorType);
    if (it == cursorByID.end()) return;

    const auto& def = it->second;

    int mX, mY;
    XPLMGetMouseLocation(&mX, &mY);

    float hw = def.texWidth * 0.5f;
    float hh = def.texHeight * 0.5f;

    float x = mX;
    float y = mY;

    XPLMSetGraphicsState(0, 1, 0, 0, 1, 1, 0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    XPLMBindTexture2d(def.texture, 0);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 1); glVertex2f(x - hw, y - hh);
        glTexCoord2f(0, 0); glVertex2f(x - hw, y + hh);
        glTexCoord2f(1, 0); glVertex2f(x + hw, y + hh);
        glTexCoord2f(1, 1); glVertex2f(x + hw, y - hh);
    }
    glEnd();
}


void PanelOverlay::InitOverlayWindow() {
    if (overlayWindow) return;

    XPLMCreateWindow_t winParams{};
    winParams.structSize = sizeof(winParams);
    winParams.visible = 1;
    winParams.left = 100;
    winParams.top = 100;
    winParams.right = 100;
    winParams.bottom = 100;
    winParams.drawWindowFunc = DrawCursorOverlay;
    winParams.handleMouseClickFunc = [](XPLMWindowID, int x, int y, XPLMMouseStatus s, void*) {
        return gOverlay->HandleMouseClick(x, y, s);
        };

    winParams.handleRightClickFunc = [](XPLMWindowID, int x, int y, XPLMMouseStatus s, void*) {
        return gOverlay->HandleRightMouseClick(x, y, s);
        };

    winParams.handleCursorFunc = [](XPLMWindowID    , int x, int y, void*) {
        return gOverlay->HandleCursor(x, y);
        };

    winParams.handleMouseWheelFunc = [](XPLMWindowID, int x, int y, int dx, int dy, void*) {
        return gOverlay->HandleScroll(x, y, dx, dy);
        };

    winParams.refcon = nullptr;
    winParams.layer = xplm_WindowLayerFloatingWindows;
    winParams.decorateAsFloatingWindow = xplm_WindowDecorationNone;

    overlayWindow = XPLMCreateWindowEx(&winParams);
    XPLMSetWindowPositioningMode(overlayWindow, xplm_WindowPositionFree, -1);
    XPLMSetWindowResizingLimits(overlayWindow, 300, 300, 300, 300);
    XPLMSetWindowGravity(overlayWindow, 0, 0, 1, 1);
}

int PanelOverlay::HandleRightMouseClick(int, int, XPLMMouseStatus status) {
    for (auto& btn : buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height && btn.callback) {
            if (status == xplm_MouseDown) btn.callback(MouseEvent::RightClick);
            else if (status == xplm_MouseUp) btn.callback(MouseEvent::Hover);
            return 1;
        }
    }
    return 0;
}

int PanelOverlay::HandleMouseClick(int, int, XPLMMouseStatus status) {
    for (auto& btn : buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height && btn.callback) {
            if (status == xplm_MouseDown) btn.callback(MouseEvent::LeftClick);
            else if (status == xplm_MouseUp) btn.callback(MouseEvent::Hover);
            return 1;
        }
    }
    return 0;
}

int PanelOverlay::HandleCursor(int, int) {
    for (const auto& btn : buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            XPLMDebugString("Hide\n");
            return xplm_CursorHidden;
        }
    }
    XPLMDebugString("Don't Hide\n");
    return xplm_CursorDefault;
}

int PanelOverlay::HandleScroll(int, int, int, int dy) {
    for (auto& btn : buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height && btn.callback) {
            btn.callback(dy > 0 ? MouseEvent::ScrollUp : MouseEvent::ScrollDown);
            return 1;
        }
    }
    return 0;
}

PanelOverlay::PanelOverlay() {
    gOverlay = this;
    if (!drawCreated) {
        drawCreated = true;
        XPLMRegisterFlightLoopCallback(UpdateFunc, -1, this);
    }
}

PanelOverlay::~PanelOverlay() {
    XPLMUnregisterFlightLoopCallback(UpdateFunc, this);
}

void PanelOverlay::AddButton(float x, float y, float w, float h,
    const string& cursorPath,
    function<void(MouseEvent)> cb) {
    if (!gOverlay) {
        static PanelOverlay overlayInstance;
        gOverlay = &overlayInstance;
    }

    InitOverlayWindow();

    int cursorID = 0;
    auto it = loadedCursors.find(cursorPath);

    if (it == loadedCursors.end()) {
        int imgW, imgH, ch;
        unsigned char* data = stbi_load(cursorPath.c_str(), &imgW, &imgH, &ch, 4);
        if (!data) {
            XPLMDebugString(("PanelOverlay: Failed to load cursor: " + cursorPath + "\n").c_str());
            return;
        }

        int texID;
        XPLMGenerateTextureNumbers(&texID, 1);
        XPLMBindTexture2d(texID, 0);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgW, imgH, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        CursorInfo info{ texID, imgW, imgH, 0, 0, 1, 1, 0, 0 };
        cursorID = nextCursorID++;
        loadedCursors[cursorPath] = info;
        cursorByID[cursorID] = info;


        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else {
        for (const auto& [id, cur] : cursorByID) {
            if (&cur == &it->second) {
                cursorID = id;
                break;
            }
        }
    }

    buttons.push_back(Button{ x, y, w, h, cursorID, cb });
}

float PanelOverlay::UpdateFunc(float elapsedSinceLastCall, float elapsedTimeSinceLastFlightLoop, int counter, void* refcon) {
    auto* self = static_cast<PanelOverlay*>(refcon);

    float currentX = XPLMGetDataf(clickXRef);
    float currentY = XPLMGetDataf(clickYRef);
    float overPn = XPLMGetDataf(overPanelDtf);

    if (overPn == -1) {
        overPanel = false;
    }
    else {
        overPanel = true;
    }

    mouseX = currentX;
    mouseY = currentY;

    bool overButton = false;

    for (auto& btn : buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            overButton = true;
        }
    }   

    if (overPanel && overButton) {
        int mX, mY;
        XPLMGetMouseLocationGlobal(&mX, &mY);
        XPLMSetWindowGeometry(overlayWindow, mX - 150, mY, mX, mY - 150);
    }
    else {
        XPLMSetWindowGeometry(overlayWindow, -1000, -1000, -1000, -1000);
    }

    return -1;
}
