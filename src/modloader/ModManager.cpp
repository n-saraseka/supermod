#include "ModManager.h"

#include <Config.h>
#include <logs/Console.h>
#include <events/D3dInitEvent.h>
#include <events/TickEvent.h>
#include <mod/ModImplInternal.h>
#include <sdk/Game.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>
#include <ui/NotificationManager.h>

void modloader::ModManager::Init()
{
    if (!exists(sdk::Game::GetModsPath()))
        create_directories(sdk::Game::GetModsPath());

    AddInternalMod(ModImplInternal::CreateMod());
    SaveConfig();
    Tick();

    EventManager::On<BeforeTickEvent>([]
    {
       Tick();
    });
}

void modloader::ModManager::Tick()
{
    UpdateStates();

    if (IsDirty(DirtyFlag::DEPS))
    {
        ClearDirty(DirtyFlag::DEPS);
    }

    for (const auto& mod : mods)
    {
        mod->Tick();
    }
}

std::shared_ptr<modloader::Mod> modloader::ModManager::FindModByID(const std::string& id)
{
    if (!mods_map.contains(id)) return nullptr;
    return mods_map[id];
}

void modloader::ModManager::ReorderMods(const std::vector<std::shared_ptr<Mod>>& newMods)
{
    assert(newMods.size() == mods.size() && std::ranges::is_permutation(mods, newMods) && "Reordered mods list is different from mods list");

    mods = newMods;

    SaveConfig();
}

void modloader::ModManager::ToggleMod(const std::shared_ptr<Mod>& mod, bool enabled)
{
    const auto state = mod->IsEnabled();
    if (state == enabled) return;

    spdlog::info(
        "{} mod {} with dependencies",
        Console::StyleToggle(enabled ? "Enabling" : "Disabling", enabled),
        Console::StyleModName(mod->GetID())
    );

    if (enabled)
    {
        mod->Toggle(true);
    }
    else
    {
        mod->Toggle(false);
    }

    SaveConfig();
}

void modloader::ModManager::ReloadMod(const std::shared_ptr<Mod>& mod)
{
    const auto info = std::dynamic_pointer_cast<ModInfoFilesystem>(mod->GetInfo());
    if (!info) return;

    const auto newInfo = std::make_shared<ModInfoFilesystem>();
    newInfo->FromPath(info->basePath);
    mod->SetInfo(newInfo);
}

void modloader::ModManager::SaveConfig(const std::shared_ptr<Mod>& mod)
{
    if (mod->HasFlag(Mod::Flag::INTERNAL)) return;

    ValidateConfig();

    auto& cfg = Config::GetYaml();
    const auto id = mod->GetID();

    PopulateConfig(mod, cfg["mods"][id]);
    Config::Get().Save();
}

void modloader::ModManager::UpdateStates()
{
    uint8_t tickCounter = 0;
    do
    {
        ClearDirty(DirtyFlag::STATES);
        for (const auto& mod : mods)
        {
            mod->Update();
        }
        tickCounter++;
    } while(IsDirty(DirtyFlag::STATES) && tickCounter < MAX_STATE_UPDATE_TICKS);

    if (tickCounter >= MAX_STATE_UPDATE_TICKS)
        spdlog::warn("Reached mod state update tick limit per script tick. Calculated {} update ticks in 1 script tick", tickCounter);
}

void modloader::ModManager::AddInternalMod(const std::shared_ptr<Mod>& mod)
{
    internal_mods.push_back(mod);
    mods_map[mod->GetInfo()->GetID()] = mod;
    mod->SetFlag(Mod::Flag::EXISTS);
}

void modloader::ModManager::ValidateConfig()
{
    auto& cfg = Config::GetYaml();

    if (!cfg["mods"].IsMap())
    {
        if (cfg["mods"] && !cfg["mods"].IsNull())
            spdlog::warn("'mods' in modcfg.yml was not a map. Overriding with an empty map instead");
        cfg["mods"] = YAML::Node(YAML::NodeType::Map);
    }

    Config::Get().Save();
}

void modloader::ModManager::SaveConfig()
{
    ValidateConfig();

    auto& cfg = Config::GetYaml();

    auto oldTree = cfg["mods"];
    auto tree = YAML::Node();

    for (const auto& mod : mods)
    {
        const auto id = mod->GetID();

        if (oldTree[id].IsMap()) tree[id] = oldTree[id];
        PopulateConfig(mod, tree[id]);
    }

    cfg["mods"] = tree;
    Config::Get().Save();
}

void modloader::ModManager::PopulateConfig(const std::shared_ptr<Mod>& mod, YAML::Node&& node)
{
    node["enabled"] = mod->IsEnabled();

    if (mod->HasFlag(Mod::Flag::REMOVAL_SCHEDULED) && mod->HasFlag(Mod::Flag::REMOVE_WITH_FILES))
        node["remove"] = true;
    else node.remove("remove");
}

#ifdef UNIT_TESTS
void modloader::ModManager::Reset()
{
    mods.clear();
    mods_map.clear();
}
#endif
