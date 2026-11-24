#pragma once
#define NOMINMAX
#include <windows.h>
#include <unordered_set>
#include <vector>
#include <deque>
#include <functional>
#include <algorithm>
#include <chrono>
#include "KeyMap.hpp"

/// High-precision monotonic clock in seconds.
inline double nowSeconds() {
    static const auto start = std::chrono::steady_clock::now();
    auto cur = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(cur - start).count();
}

/// Hardware-level full-state keyboard & mouse engine via Windows Raw Input API (RIDEV_INPUTSINK).
/// Captures true N-key rollover make/break scancodes across typing (any IME) and games!
class InputEngine {
public:
    std::unordered_set<PhysicalKey> activeKeys;
    std::vector<PhysicalKey> pressedOrder;

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    bool otherMouseDown = false;
    double lastMouseMoveTime = 0.0;
    double lastMouseClickTime = 0.0;
    double lastKeyActivityTime = 0.0;

    POINT lastCursorPos = { 0, 0 };
    Pt mouseVelocity = { 0.0f, 0.0f };
    float rawDeltaAccumX = 0.0f;
    float rawDeltaAccumY = 0.0f;

    int totalKeystrokesToday = 0;
    bool isRawInputRegistered = false;

    std::function<void(PhysicalKey)> onKeyDown;
    std::function<void(PhysicalKey)> onKeyUp;
    std::function<void(int, bool)> onMouseClick;

    bool registerRawInput(HWND hwnd) {
        RAWINPUTDEVICE rid[2] = {};
        // Keyboard (Generic Desktop 0x01, Usage 0x06)
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x06;
        rid[0].dwFlags = RIDEV_INPUTSINK;
        rid[0].hwndTarget = hwnd;
        // Mouse (Generic Desktop 0x01, Usage 0x02)
        rid[1].usUsagePage = 0x01;
        rid[1].usUsage = 0x02;
        rid[1].dwFlags = RIDEV_INPUTSINK;
        rid[1].hwndTarget = hwnd;

        isRawInputRegistered = (RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)) != FALSE);
        GetCursorPos(&lastCursorPos);
        return isRawInputRegistered;
    }

    void handleRawInput(LPARAM lParam) {
        UINT size = 0;
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) {
            return;
        }
        std::vector<BYTE> buffer(size);
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
            return;
        }

        const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
        if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            const RAWKEYBOARD& kb = raw->data.keyboard;
            // Ignore synthetic overrun / pause prefix
            if (kb.MakeCode == 0 && kb.VKey == 0xFF) return;
            PhysicalKey key = mapRawInputKey(kb.MakeCode, kb.Flags, kb.VKey);
            if (key == PhysicalKey::None) return;

            bool isBreak = (kb.Flags & RI_KEY_BREAK) != 0;
            setKeyState(key, !isBreak);
        } else if (raw->header.dwType == RIM_TYPEMOUSE) {
            const RAWMOUSE& m = raw->data.mouse;
            double now = nowSeconds();
            if ((m.usFlags & MOUSE_MOVE_ABSOLUTE) == 0 && (m.lLastX != 0 || m.lLastY != 0)) {
                rawDeltaAccumX += (float)m.lLastX;
                rawDeltaAccumY += (float)m.lLastY;
                lastMouseMoveTime = now;
            }
            if (m.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)   updateMouseButton(0, true);
            if (m.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)     updateMouseButton(0, false);
            if (m.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)  updateMouseButton(1, true);
            if (m.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)    updateMouseButton(1, false);
            if (m.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) updateMouseButton(2, true);
            if (m.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)   updateMouseButton(2, false);
            if (m.usButtonFlags & RI_MOUSE_WHEEL)              lastMouseMoveTime = now;
        }
    }

    /// Called at 60Hz from main loop to compute smooth cursor velocity (works in desktop & locked-cursor games!).
    void pollMouse(float dt) {
        POINT cur = lastCursorPos;
        GetCursorPos(&cur);
        float dx = (float)(cur.x - lastCursorPos.x);
        float dy = (float)(cur.y - lastCursorPos.y);
        lastCursorPos = cur;

        // If cursor is locked by a game (dx==0, dy==0), fall back to raw mouse hardware deltas!
        if (std::fabs(dx) < 0.5f && std::fabs(dy) < 0.5f && (std::fabs(rawDeltaAccumX) > 0.5f || std::fabs(rawDeltaAccumY) > 0.5f)) {
            dx = rawDeltaAccumX;
            dy = rawDeltaAccumY;
        }
        rawDeltaAccumX = 0.0f;
        rawDeltaAccumY = 0.0f;

        float dist = std::hypot(dx, dy);
        if (dist > 0.4f && dt > 0.001f) {
            lastMouseMoveTime = nowSeconds();
            mouseVelocity.x = dx / dt;
            mouseVelocity.y = dy / dt;
        } else {
            mouseVelocity.x *= 0.82f;
            mouseVelocity.y *= 0.82f;
        }

        double now = nowSeconds();
        while (!recentKeyTimes_.empty() && now - recentKeyTimes_.front() > 10.0) {
            recentKeyTimes_.pop_front();
        }
    }

    int currentKPM() const {
        return (int)recentKeyTimes_.size() * 6;
    }

private:
    std::deque<double> recentKeyTimes_;

    void setKeyState(PhysicalKey key, bool isDown) {
        bool wasActive = activeKeys.count(key) > 0;
        if (isDown && !wasActive) {
            activeKeys.insert(key);
            pressedOrder.erase(std::remove(pressedOrder.begin(), pressedOrder.end(), key), pressedOrder.end());
            pressedOrder.push_back(key);
            double now = nowSeconds();
            lastKeyActivityTime = now;
            recentKeyTimes_.push_back(now);
            totalKeystrokesToday++;
            if (onKeyDown) onKeyDown(key);
        } else if (!isDown && wasActive) {
            activeKeys.erase(key);
            pressedOrder.erase(std::remove(pressedOrder.begin(), pressedOrder.end(), key), pressedOrder.end());
            if (onKeyUp) onKeyUp(key);
        }
    }

    void updateMouseButton(int button, bool isDown) {
        bool prev = false;
        if (button == 0) { prev = leftMouseDown; leftMouseDown = isDown; }
        else if (button == 1) { prev = rightMouseDown; rightMouseDown = isDown; }
        else { prev = otherMouseDown; otherMouseDown = isDown; }

        if (prev != isDown) {
            double now = nowSeconds();
            lastMouseMoveTime = now;
            if (isDown) lastMouseClickTime = now;
            if (onMouseClick) onMouseClick(button, isDown);
        }
    }
};
