#pragma once
#include "../../Logger.h"

extern "C" void cube__Creature__OnCreatureDeath(cube::Creature* creature, cube::Creature* attacker)
{
    if (!creature) return;
	cube::Game* game = cube::GetGame();
	if (!attacker && game) {
		attacker = game->GetPlayer();
	}
	creature->entity_data.HP = 0;

	CW_LOG_DEBUG("cube__Creature__OnCreatureDeath: creature=%p, attacker=%p", creature, attacker);

	for (uint8_t priority = 0; priority <= 4; priority += 1) {
		for (DLL* dll : modDLLs) {
			if (dll && dll->mod && dll->mod->OnCreatureDeathPriority == (GenericMod::Priority)priority) {
				dll->mod->OnCreatureDeath(game, creature, attacker);
			}
		}
	}
}

GETTER_VAR(void*, ASM_cube__Creature__OnCreatureDeath_JMPBACK);
extern "C" void ASM_cube__Creature__OnCreatureDeath();

void setup_cube__Creature__OnCreatureDeath() {
	WriteFarJMP(CWOffset(0x29E494), (void*)&ASM_cube__Creature__OnCreatureDeath);
	ASM_cube__Creature__OnCreatureDeath_JMPBACK = CWOffset(0x29E4A5);
}