// This patch fixes the camera zoom looking further or closer than expected
#include "Utils.h"
#include "events/EventManager.h"
#include <events/ResolutionChangeEvent.h>
#include "memory/Memory.h"
#include "memory/HookManager.h"
#include "sdk/Game.h"

// Ratio 
static vector2 vec = sdk::Game::GetRenderSize();
static float realH = vec.y;

// Zoom
static float zoomOne = 1.0;
static float zoomHalf = 0.5;
static float zoomThreeFifths = 0.6;
static float zoomMax = 5.0;
static float defaultZoom = 1.0;
static float lastDefaultZoom = 1.0;

static float zoomStatic(float orig, float height) {
    return orig / (height / 600);
}

static void ApplyChanges() {
    // Zoom patterns
    constexpr Memory::Pattern defaultZoomAddr("C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8");
    constexpr Memory::Pattern cheatkeyDefaultZoomAddr("C7 05 ? ? ? ? ? ? ? ? E9 ? ? ? ? 83 7D ? ? 75 ? 0F B6 0D");
    constexpr Memory::Pattern cheatkeyMinZoomAddr("C7 05 ? ? ? ? ? ? ? ? EB ? D9 45");
    constexpr Memory::Pattern cheatkeyMaxZoomAddr("C7 05 ? ? ? ? ? ? ? ? EB ? 8B 55 ? 89 15");
    constexpr Memory::Pattern baseSceneZoomIfMin("D8 1D ? ? ? ? DF E0 F6 C4 ? 7A ? C7 05 ? ? ? ? ? ? ? ? EB ? D9 45");
    constexpr Memory::Pattern baseSceneZoomIfMax("D8 1D ? ? ? ? DF E0 F6 C4 ? 75 ? C7 05 ? ? ? ? ? ? ? ? EB ? 8B 55");
    constexpr Memory::Pattern defaultZoomInAnimAddr("D8 2D ? ? ? ? D9 1D ? ? ? ? E9 ? ? ? ? D9 05 ? ? ? ? D8 4D ? D8 2D");

    // Animation patterns
    constexpr Memory::Pattern levelDoneAnimDuration("C7 05 ? ? ? ? ? ? ? ? EB ? C7 05 ? ? ? ? ? ? ? ? EB ? C7 05 ? ? ? ? ? ? ? ? 8B E5");
    constexpr Memory::Pattern maxLevelAnimAddr("D9 05 ? ? ? ? D8 4D ? D8 2D ? ? ? ? D9 1D ? ? ? ? E9");
    constexpr Memory::Pattern maxBossAnimAddr("D9 05 ? ? ? ? D8 4D ? D8 2D ? ? ? ? D9 1D ? ? ? ? 8B 4D");

    // Set zoom variables
    zoomOne = zoomStatic(defaultZoom, realH);
    zoomHalf = zoomStatic(defaultZoom * 0.5, realH);
    zoomThreeFifths = zoomStatic(defaultZoom * 0.6, realH);
    zoomMax = zoomStatic(defaultZoom * 5.0, realH);

    // Write
    defaultZoomAddr.Search().Add(6).Put((float_t) zoomOne);
    cheatkeyDefaultZoomAddr.Search().Add(6).Put((float_t) zoomOne);
    cheatkeyMinZoomAddr.Search().Add(6).Put((float_t) zoomHalf);
    cheatkeyMaxZoomAddr.Search().Add(6).Put((float_t) zoomMax);
    baseSceneZoomIfMin.Search().Add(2).Put((int32_t) &zoomHalf);
    baseSceneZoomIfMax.Search().Add(2).Put((int32_t) &zoomMax);
    defaultZoomInAnimAddr.Search().Add(2).Put((int32_t) &zoomOne);
    maxLevelAnimAddr.Search().Add(2).Put((int32_t) &zoomHalf);
    maxLevelAnimAddr.Search().Add(11).Put((int32_t) &zoomOne); // Default level anim
    maxBossAnimAddr.Search().Add(2).Put((int32_t) &zoomThreeFifths);
    maxBossAnimAddr.Search().Add(11).Put((int32_t) &zoomOne); // Default boss anim
    levelDoneAnimDuration.Search().Add(6).Put((float_t) 5500.0); // Prevents camera from clamping to level right border on level end anim (according to Neo)
} 

inline EventManager::Ready $dynamic_zoom_patch([] {
    ApplyChanges();
    EventManager::On<ResolutionChangeEvent>([](const ResolutionChangeEvent& evt) {
        realH = evt.y;
        ApplyChanges();
    });
});
