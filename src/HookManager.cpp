#include "HookManager.h"
#include "main.h"
#include <windows.h>
#include <vector>
#include <cmath>
#include <algorithm>
#define MODLOADER 1
#include "cwsdk.h"
#include "DLL.h"
#include "macros.h"
#include "Logger.h"

#include "callbacks/ChatHandler.h"
#include "callbacks/P2PRequestHandler.h"
#include "callbacks/CheckInventoryFullHandler.h"
#include "callbacks/GameTickHandler.h"
#include "callbacks/ZoneGeneratedHandler.h"
#include "callbacks/ZoneDestroyHandler.h"
#include "callbacks/WindowProcHandler.h"
#include "callbacks/GetKeyboardStateHandler.h"
#include "callbacks/GetMouseStateHandler.h"
#include "callbacks/PresentHandler.h"
#include "callbacks/CreatureCriticalCalculatedHandler.h"
#include "callbacks/CreatureAttackPowerCalculatedHandler.h"
#include "callbacks/CreatureSpellPowerCalculatedHandler.h"
#include "callbacks/CreatureHasteCalculatedHandler.h"
#include "callbacks/CreatureHPCalculatedHandler.h"
#include "callbacks/CreatureResistanceCalculatedHandler.h"
#include "callbacks/CreatureRegenerationCalculatedHandler.h"
#include "callbacks/CreatureManaGenerationCalculatedHandler.h"
#include "callbacks/ChunkRemeshHandler.h"
#include "callbacks/ChunkRemeshedHandler.h"

#include "callbacks/gui/cube__StartMenuWidget__Draw.h"
#include "callbacks/gui/cube__CharacterPreviewWidget__Draw.h"
#include "callbacks/gui/cube__GUI__Load.h"
#include "callbacks/creature/cube__Creature__GetArmor.h"
#include "callbacks/creature/cube__Creature__OnPlayerCombatDeath.h"
#include "callbacks/creature/cube__Creature__OnPlayerDrownDeath.h"
#include "callbacks/creature/cube__Creature__OnPlayerFallDeath.h"
#include "callbacks/creature/cube__Creature__OnCreatureDeath.h"
#include "callbacks/creature/cube__Creature__CanEquipItem.h"
#include "callbacks/game/cube__Game__MouseUp.h"
#include "callbacks/game/cube__Game__Update.h"
#include "callbacks/item/cube__Item__OnGetBuyingPrice.h"
#include "callbacks/item/cube__Item__OnGetSellingPrice.h"
#include "callbacks/item/cube__Item__OnGetGoldBagValue.h"
#include "callbacks/item/cube__Item__OnClassCanWearItem.h"

namespace cw {
  void HookManager::SetupHandlers() {
    CW_LOG_DEBUG("Setting up internal game function hooks (Gameplay, Items, World, Combat Suite)...");
    setup_function(cube__Creature__GetArmor);
    setup_function(cube__Creature__OnPlayerCombatDeath);
    setup_function(cube__Creature__OnPlayerDrownDeath);
    setup_function(cube__Creature__OnPlayerFallDeath);
    // Note: cube__Creature__CanEquipItem (0x50640) and cube__Item__OnClassCanWearItem (0x1094D0)
    // overwrite native Cube World item tables with incomplete logic causing weapons/armor
    // to be marked unknown and unwearable. They are disabled to preserve native game engine behavior.
    // setup_function(cube__Creature__CanEquipItem);
    setup_function(cube__StartMenuWidget__Draw);
    setup_function(cube__Game__Update);
    setup_function(cube__Item__GetBuyingPrice);
    setup_function(cube__Item__OnGetSellingPrice);
    setup_function(cube__Item__OnGetGoldBagValue);
    // setup_function(cube__Item__OnClassCanWearItem);

    SetupChatHandler();
    SetupP2PRequestHandler();
    SetupCheckInventoryFullHandler();
    SetupGameTickHandler();
    SetupZoneGeneratedHandler();
    SetupZoneDestroyHandler();
    SetupWindowProcHandler();
    SetupGetKeyboardStateHandler();
    SetupGetMouseStateHandler();
    SetupPresentHandler();
    SetupCreatureCriticalCalculatedHandler();
    SetupCreatureAttackPowerCalculatedHandler();
    SetupCreatureSpellPowerCalculatedHandler();
    SetupCreatureHasteCalculatedHandler();
    SetupCreatureHPCalculatedHandler();
    SetupCreatureResistanceCalculatedHandler();
    SetupCreatureRegenerationCalculatedHandler();
    SetupCreatureManaGenerationCalculatedHandler();
    SetupChunkRemeshHandler();
    SetupChunkRemeshedHandler();
    CW_LOG_INFO("Gameplay, Items, World, and Combat hooks initialized successfully.");
  }
} // namespace cw
