#include "ThunderSkill.h"

void ThunderSkill::init()
{
	mp_cost = 10;
	damage = 1;
	damage_level = 0;
	effect_level = 0;
	mp_cost_level = 0;
	impactTime = 3000;
	scale = { 2.5f,2.5f };
}
bool ThunderSkill::level_up(int select)
{
	bool success = false;
	switch (select) 
	{
		case LEVEL_UP_DAMAGE: // increase damage
			if (damage_level < 5) 
			{
				damage += 1.f;
				mp_cost += 2.f;
				success = true;
				damage_level++;
			}
			break;
		case LEVEL_UP_EFFECT: // 
			if (effect_level < 5)
			{
				scale = { scale.x * 1.1f , scale.y * 1.1f };
				mp_cost += 2.f;
				success = true;
				effect_level++;
			}
			
			break;
		case LEVEL_UP_MANA_COST:
			if (mp_cost_level < 5)
			{
				mp_cost *= 0.8;
				success = true;
				mp_cost_level++;
			}
			break;
		default:
			break;
	}
	return success;

}

float ThunderSkill::get_mpcost()
{
	return mp_cost;
}

float ThunderSkill::drop_thunder(std::vector<Thunder*> &thunders, vec2 position)
{
	Thunder* t = new Thunder(position, impactTime,damage,scale);
	thunders.emplace_back(t);
	return mp_cost;
}
