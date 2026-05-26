#include <imgui_internal.h>
#include <modloader/ModManager.h>
#include <ui/Ui.h>

#include "../widgets.h"

void ui::widgets::mods::ContextMenu(const std::shared_ptr<modloader::Mod>& mod)
{
    if (ImGui::MenuItem(mod->IsEnabled() ? ICON_MD_POWER_SETTINGS_NEW " Выключить" : ICON_MD_POWER_SETTINGS_NEW "Включить"))
    {
        modloader::ModManager::ToggleMod(mod, !mod->IsEnabled());
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_MD_REFRESH " Перезагрузить", nullptr, false, !mod->IsActive()))
    {
        modloader::ModManager::ReloadMod(mod);
    }
    if (mod->IsActive()) Tooltip("Мод должен быть выключен для перезагрузки", ImGuiHoveredFlags_AllowWhenDisabled);

    ImGui::PopStyleColor();
}
