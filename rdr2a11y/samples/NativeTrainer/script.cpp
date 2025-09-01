/*
	THIS FILE IS A PART OF RDR 2 SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2019
*/

#include "script.h"
#include "scriptmenu.h"
#include "keyboard.h"
#include "a11y.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>

using namespace std;

#include "scriptinfo.h"

// Last-known stable player money (in cents) as a fallback when direct reads are unreliable
int g_lastKnownMoneyCents = -1;

// Forward declaration (defined at bottom)
static bool TryReadHonor(int& outHonor);

// Best-effort wallet reader: prefer CASH natives, then fall back to PED::GET_PED_MONEY
static int ReadPlayerMoneyCents(Ped me)
{
	int best = -1;
	// Try CASH getters (names unknown in headers; using hashes via wrappers)
	// These return Any; interpret as int cents if plausible
	__try {
		int v0 = (int)CASH::_0x0C02DABFA3B98176();
		if (v0 > best) best = v0;
	} __except(EXCEPTION_EXECUTE_HANDLER) {}
	__try {
		int v1 = (int)CASH::_0xA46FD001D1BE896C();
		if (v1 > best) best = v1;
	} __except(EXCEPTION_EXECUTE_HANDLER) {}
	__try {
		int v2 = (int)CASH::_0x282D36FF103D78DF();
		if (v2 > best) best = v2;
	} __except(EXCEPTION_EXECUTE_HANDLER) {}
	__try {
		int v3 = (int)CASH::_0x8A67120DBC299525();
		if (v3 > best) best = v3;
	} __except(EXCEPTION_EXECUTE_HANDLER) {}
	// Fallback: ped money
	int pedMoney = PED::GET_PED_MONEY(me);
	if (pedMoney > best) best = pedMoney;
	return best;
}

class MenuItemPlayerFastHeal : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
			PLAYER::SET_PLAYER_HEALTH_RECHARGE_MULTIPLIER(PLAYER::PLAYER_ID(), 1.0);
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::SET_PLAYER_HEALTH_RECHARGE_MULTIPLIER(PLAYER::PLAYER_ID(), 1000.0);
	}
public:
	MenuItemPlayerFastHeal(string caption)
		: MenuItemSwitchable(caption) {}
};
class MenuItemPlayerFix : public MenuItemDefault
{
	virtual void OnSelect()
	{
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		ENTITY::SET_ENTITY_HEALTH(playerPed, ENTITY::GET_ENTITY_MAX_HEALTH(playerPed, FALSE), FALSE);
		PED::CLEAR_PED_WETNESS(playerPed);
		PLAYER::RESTORE_PLAYER_STAMINA(PLAYER::PLAYER_ID(), 100.0);
		PLAYER::RESTORE_SPECIAL_ABILITY(PLAYER::PLAYER_ID(), -1, FALSE);

		if (PED::IS_PED_ON_MOUNT(playerPed))
		{
			Ped horse = PED::GET_MOUNT(playerPed);
			ENTITY::SET_ENTITY_HEALTH(horse, ENTITY::GET_ENTITY_MAX_HEALTH(horse, FALSE), FALSE);
			PED::SET_PED_STAMINA(horse, 100.0);
			SetStatusText("player and horse fixed");
		} else
		if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE))
		{
			Vehicle veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
			ENTITY::SET_ENTITY_HEALTH(veh, ENTITY::GET_ENTITY_MAX_HEALTH(veh, FALSE), FALSE);
			SetStatusText("player and vehicle fixed");
		} else
			SetStatusText("player fixed");
	}
public:
	MenuItemPlayerFix(string caption)
		: MenuItemDefault(caption) {}
};

class MenuItemVehicleBoost : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (newState)
			SetStatusText("PAGEUP / NUM9\nPAGEDOWN / NUM6");
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (!GetState())
			return;
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (!PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
			return;

		Vehicle veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
		DWORD model = ENTITY::GET_ENTITY_MODEL(veh);
		BOOL bTrain = VEHICLE::IS_THIS_MODEL_A_TRAIN(model);

		bool bUp = IsKeyDownLong(VK_NUMPAD9) || IsKeyDownLong(VK_PRIOR);
		bool bDown = IsKeyDown(VK_NUMPAD3) || IsKeyDown(VK_NEXT);

		if (!(bUp || bDown))
			return;

		if (bTrain)
		{
			float speed = bUp ? 30.0f : 0.0f;
			VEHICLE::SET_TRAIN_SPEED(veh, speed);
			VEHICLE::SET_TRAIN_CRUISE_SPEED(veh, speed);
			return;
		}

		float speed = ENTITY::GET_ENTITY_SPEED(veh);
		if (bUp)
		{
			if (speed < 3.0f) speed = 3.0f;
			speed += speed * 0.03f;
			VEHICLE::SET_VEHICLE_FORWARD_SPEED(veh, speed);
		} else
		if (ENTITY::IS_ENTITY_IN_AIR(veh, 0) || speed > 5.0)
			VEHICLE::SET_VEHICLE_FORWARD_SPEED(veh, 0.0);
	}
public:
	MenuItemVehicleBoost(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerAddCash : public MenuItemDefault
{
	int m_value;
	virtual void OnSelect()	{ 
		CASH::PLAYER_ADD_CASH(m_value, 0x2cd419dc);
		std::wstring wcaption(GetCaption().begin(), GetCaption().end());
		if (!wcaption.empty()) A11y::speak(wcaption, true);
	}
public:
	MenuItemPlayerAddCash(string caption, int value)
		: MenuItemDefault(caption),
			m_value(value) {}
};

class MenuItemPlayerInvincible : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
			PLAYER::SET_PLAYER_INVINCIBLE(PLAYER::PLAYER_ID(), FALSE);
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::SET_PLAYER_INVINCIBLE(PLAYER::PLAYER_ID(), TRUE);
	}
public:
	MenuItemPlayerInvincible(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerHorseInvincible : public MenuItemSwitchable
{
	void SetPlayerHorseInvincible(bool set)
	{
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_ON_MOUNT(playerPed))
		{
			Ped horse = PED::GET_MOUNT(playerPed);
			ENTITY::SET_ENTITY_INVINCIBLE(horse, set);
		}
	}
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
			SetPlayerHorseInvincible(false);
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
			SetPlayerHorseInvincible(true);
	}
public:
	MenuItemPlayerHorseInvincible(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerUnlimStamina : public MenuItemSwitchable
{
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::RESTORE_PLAYER_STAMINA(PLAYER::PLAYER_ID(), 100.0);
	}
public:
	MenuItemPlayerUnlimStamina(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerUnlimAbility : public MenuItemSwitchable
{
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::RESTORE_SPECIAL_ABILITY(PLAYER::PLAYER_ID(), -1, FALSE);
	}
public:
	MenuItemPlayerUnlimAbility(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerHorseUnlimStamina : public MenuItemSwitchable
{
	virtual void OnFrame()
	{
		if (GetState())
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (PED::IS_PED_ON_MOUNT(playerPed))
			{
				Ped horse = PED::GET_MOUNT(playerPed);
				PED::SET_PED_STAMINA(horse, 100.0);
			}
		}
	}
public:
	MenuItemPlayerHorseUnlimStamina(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerSuperJump : public MenuItemSwitchable
{
	virtual void OnFrame()
	{		
		if (GetState())
			GAMEPLAY::SET_SUPER_JUMP_THIS_FRAME(PLAYER::PLAYER_ID());
	}
public:
	MenuItemPlayerSuperJump(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerNoiseless : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
		{
			PLAYER::SET_PLAYER_NOISE_MULTIPLIER(PLAYER::PLAYER_ID(), 1.0);
			PLAYER::SET_PLAYER_SNEAKING_NOISE_MULTIPLIER(PLAYER::PLAYER_ID(), 1.0);
		}
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
		{
			PLAYER::SET_PLAYER_NOISE_MULTIPLIER(PLAYER::PLAYER_ID(), 0.0);
			PLAYER::SET_PLAYER_SNEAKING_NOISE_MULTIPLIER(PLAYER::PLAYER_ID(), 0.0);
		}
	}
public:
	MenuItemPlayerNoiseless(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerEveryoneIgnored : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
			PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_ID(), FALSE);
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_ID(), TRUE);
	}
public:
	MenuItemPlayerEveryoneIgnored(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemPlayerTeleport : public MenuItemDefault
{
	Vector3 m_pos;
	virtual void OnSelect()
	{
		Entity e = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_ON_MOUNT(e))
			e = PED::GET_MOUNT(e);
		else
		if (PED::IS_PED_IN_ANY_VEHICLE(e, FALSE))
			e = PED::GET_VEHICLE_PED_IS_USING(e);
		ENTITY::SET_ENTITY_COORDS(e, m_pos.x, m_pos.y, m_pos.z, 0, 0, 1, FALSE);
	}
public:
	MenuItemPlayerTeleport(string caption, Vector3 pos)
		: MenuItemDefault(caption), 
		  m_pos(pos) {}
};

class MenuItemPlayerTeleportToMarker : public MenuItemDefault
{
	virtual void OnSelect()
	{
		if (!RADAR::IS_WAYPOINT_ACTIVE())
		{
			SetStatusText("map marker isn't set");
			return;
		}

		Vector3 coords = RADAR::GET_WAYPOINT_COORDS_3D();

		Entity e = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_ON_MOUNT(e))
			e = PED::GET_MOUNT(e);
		else
		if (PED::IS_PED_IN_ANY_VEHICLE(e, FALSE))
			e = PED::GET_VEHICLE_PED_IS_USING(e);

		if (!GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(coords.x, coords.y, 100.0, &coords.z, FALSE))
		{
			static const float groundCheckHeight[] = {
				100.0, 150.0, 50.0, 0.0, 200.0, 250.0, 300.0, 350.0, 400.0,
				450.0, 500.0, 550.0, 600.0, 650.0, 700.0, 750.0, 800.0
			};
			for each (float height in groundCheckHeight)
			{
				ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, coords.x, coords.y, height, 0, 0, 1);
				WaitAndDraw(100);
				if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(coords.x, coords.y, height, &coords.z, FALSE))
				{
					coords.z += 3.0;
					break;
				}
			}
		}

		ENTITY::SET_ENTITY_COORDS(e, coords.x, coords.y, coords.z, 0, 0, 1, FALSE);
	}
public:
	MenuItemPlayerTeleportToMarker(string caption)
		: MenuItemDefault(caption) {}
};

class MenuItemPlayerClearWanted : public MenuItemDefault
{
	bool m_headPrice;
	bool m_pursuit;
	virtual void OnSelect()
	{
		Player player = PLAYER::PLAYER_ID();
		if (m_headPrice)
			PURSUIT::SET_PLAYER_PRICE_ON_A_HEAD(player, 0);
		if (m_pursuit)
		{
			PURSUIT::CLEAR_CURRENT_PURSUIT();
			PURSUIT::SET_PLAYER_WANTED_INTENSITY(player, 0);
		}
		SetStatusText("Player has to be in pursuit\n\n"
					  "head price: " + to_string(PURSUIT::GET_PLAYER_PRICE_ON_A_HEAD(player) / 100) + "\n" + 
					  "wanted intensity: " + to_string(PURSUIT::GET_PLAYER_WANTED_INTENSITY(player) / 100));
	}
public:
	MenuItemPlayerClearWanted(string caption, bool headPrice, bool pursuit)
		: MenuItemDefault(caption),
			m_headPrice(headPrice), m_pursuit(pursuit) {}
};

class MenuItemPlayerNeverWanted : public MenuItemSwitchable
{	
	virtual void OnSelect()
	{
		bool newstate = !GetState();
		if (!newstate)
			PLAYER::SET_WANTED_LEVEL_MULTIPLIER(1.0);
		SetState(newstate);
	}
	virtual void OnFrame()
	{
		if (GetState())
		{
			Player player = PLAYER::PLAYER_ID();
			PURSUIT::CLEAR_CURRENT_PURSUIT();
			PURSUIT::SET_PLAYER_PRICE_ON_A_HEAD(player, 0);
			PURSUIT::SET_PLAYER_WANTED_INTENSITY(player, 0);
			PLAYER::SET_WANTED_LEVEL_MULTIPLIER(0.0);
		}
	}
public:
	MenuItemPlayerNeverWanted(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemChangePlayerModel : public MenuItemDefault
{
	string		m_model;

	virtual void OnSelect()
	{
		DWORD model = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(m_model.c_str()));
		if (STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_VALID(model))
		{
			UINT64 *ptr1 = getGlobalPtr(0x28) + 0x27;
			UINT64 *ptr2 = getGlobalPtr(((DWORD)7 << 18) | 0x1890C) + 2;
			UINT64 bcp1 = *ptr1;
			UINT64 bcp2 = *ptr2;
			*ptr1 = *ptr2 = model;			
			WaitAndDraw(1000);
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			PED::SET_PED_VISIBLE(playerPed, TRUE);
			if (ENTITY::GET_ENTITY_MODEL(playerPed) != model)
			{
				*ptr1 = bcp1;
				*ptr2 = bcp2;
			}
		}
	}
public:
	MenuItemChangePlayerModel(string caption, string model)
		: MenuItemDefault(caption),
			m_model(model) { }
};

class MenuItemSpawnPed : public MenuItemDefault
{
	string		m_model;

	virtual string GetModel() { return m_model; }

	virtual void OnSelect()
	{
		DWORD model = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(GetModel().c_str()));
		if (STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_VALID(model))
		{
			STREAMING::REQUEST_MODEL(model, FALSE);
			while (!STREAMING::HAS_MODEL_LOADED(model))
				WaitAndDraw(0); // WAIT(0);
			Vector3 coords = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER::PLAYER_PED_ID(), 0.0, 3.0, -0.3);
			Ped ped = PED::CREATE_PED(model, coords.x, coords.y, coords.z, static_cast<float>(rand() % 360), 0, 0, 0, 0);
			PED::SET_PED_VISIBLE(ped, TRUE);
			ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&ped);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
		}
	}
public:
	MenuItemSpawnPed(string caption, string model)
		: MenuItemDefault(caption),
			m_model(model) { }
};

class MenuItemSpawnHorseRandom : public MenuItemSpawnPed
{
	virtual string GetModel()
	{
		while (true)
		{
			int index = rand() % ARRAY_LENGTH(pedModelInfos);
			if (pedModelInfos[index].horse)
			{
				SetStatusText(pedModelInfos[index].name);
				return pedModelInfos[index].model;
			}
		}
	}
public:
	MenuItemSpawnHorseRandom(string caption)
		: MenuItemSpawnPed(caption, "") { }
};

class MenuItemSpawnAnimalRandom : public MenuItemSpawnPed
{
	virtual string GetModel()  
	{ 
		while (true)
		{
			int index = rand() % ARRAY_LENGTH(pedModelInfos);
			if (pedModelInfos[index].animal && !pedModelInfos[index].fish && !pedModelInfos[index].horse)
			{
				SetStatusText(pedModelInfos[index].name);
				return pedModelInfos[index].model;
			}
		}
	}
public:
	MenuItemSpawnAnimalRandom(string caption)
		: MenuItemSpawnPed(caption, "") { }
};

class MenuItemSpawnPedRandom : public MenuItemSpawnPed
{
	virtual string GetModel()
	{
		while (true)
		{
			int index = rand() % ARRAY_LENGTH(pedModelInfos);
			if (!pedModelInfos[index].animal)
			{
				SetStatusText(pedModelInfos[index].name);
				return pedModelInfos[index].model;
			}
		}
	}
public:
	MenuItemSpawnPedRandom(string caption)
		: MenuItemSpawnPed(caption, "") { }
};

class MenuItemSpawnVehicle : public MenuItemDefault
{
	string		m_model;
	Vector3		m_pos;
	float		m_heading;
	bool		m_resetHeading;
	bool		m_noPeds;

	MenuItemSwitchable * m_menuItemWrapIn;
	MenuItemSwitchable * m_menuItemSetProperly;

	virtual void OnSelect()
	{
		DWORD model = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(m_model.c_str()));
		if (STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_VALID(model))
		{
			STREAMING::REQUEST_MODEL(model, FALSE);
			while (!STREAMING::HAS_MODEL_LOADED(model))
				WaitAndDraw(0); // WAIT(0);
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			float playerHeading = ENTITY::GET_ENTITY_HEADING(playerPed) + 5.0f;
			float heading = playerHeading + m_heading;
			Vector3 coords = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, m_pos.x, m_pos.y, m_pos.z);
			Vehicle veh = VEHICLE::CREATE_VEHICLE(model, coords.x, coords.y, coords.z, heading, 0, 0, m_noPeds, 0);
			DECORATOR::DECOR_SET_BOOL(veh, "wagon_block_honor", TRUE);
			bool wrapIn = m_menuItemWrapIn && m_menuItemWrapIn->GetState();
			bool setProperly = m_menuItemSetProperly && m_menuItemSetProperly->GetState();
			if (setProperly)
			{
				VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(veh, 0);
				WaitAndDraw(100);
			}
			if (m_resetHeading || wrapIn)
				ENTITY::SET_ENTITY_HEADING(veh, wrapIn ? playerHeading : heading);
			if (wrapIn)
				PED::SET_PED_INTO_VEHICLE(playerPed, veh, -1);
			ENTITY::SET_VEHICLE_AS_NO_LONGER_NEEDED(&veh);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
		}
	}
public:
	MenuItemSpawnVehicle(string model, 
		Vector3 pos, float heading, 
		MenuItemSwitchable *menuItemWrapIn,
		MenuItemSwitchable *menuItemSetProperly,
		bool resetHeading, 
		bool noPeds)
		: MenuItemDefault(model),			
			m_model(model), 
			m_pos(pos), m_heading(heading),
			m_menuItemWrapIn(menuItemWrapIn),
			m_menuItemSetProperly(menuItemSetProperly),
			m_resetHeading(resetHeading), 
			m_noPeds(noPeds) { }
};

class MenuItemGiveWeapon : public MenuItemDefault
{
	string m_name;

	virtual void OnSelect()
	{
		Hash hash = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(("WEAPON_" + m_name).c_str()));
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		WEAPON::GIVE_DELAYED_WEAPON_TO_PED(playerPed, hash, 100, 1, 0x2cd419dc);
		WEAPON::SET_PED_AMMO(playerPed, hash, 100);
		WEAPON::SET_CURRENT_PED_WEAPON(playerPed, hash, 1, 0, 0, 0);
	}
public:
	MenuItemGiveWeapon(string caption, string weaponName)
		: MenuItemDefault(caption),
			m_name(weaponName) {}
};

class MenuItemWeaponPowerfullGuns : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
			PLAYER::SET_PLAYER_WEAPON_DAMAGE_MODIFIER(PLAYER::PLAYER_ID(), 1.0);
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::SET_PLAYER_WEAPON_DAMAGE_MODIFIER(PLAYER::PLAYER_ID(), 100.0);
	}
public:
	MenuItemWeaponPowerfullGuns(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemWeaponPowerfullMelee : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (!newState)
			PLAYER::SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(PLAYER::PLAYER_ID(), 1.0);
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (GetState())
			PLAYER::SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(PLAYER::PLAYER_ID(), 100.0);
	}
public:
	MenuItemWeaponPowerfullMelee(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemWeaponNoReload : public MenuItemSwitchable
{
	virtual void OnFrame()
	{
	// Disabled per user request: do not give ammo automatically.
	// Keeping this method as a no-op ensures no ammo is added when holding/switching weapons.
	return;
	}
public:
	MenuItemWeaponNoReload(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemWeaponDropCurrent : public MenuItemDefault
{
	virtual void OnSelect()
	{
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		Hash unarmed = GAMEPLAY::GET_HASH_KEY("WEAPON_UNARMED");
		Hash cur;
		if (WEAPON::GET_CURRENT_PED_WEAPON(playerPed, &cur, 0, 0, 0) && WEAPON::IS_WEAPON_VALID(cur) && cur != unarmed)
			WEAPON::SET_PED_DROPS_INVENTORY_WEAPON(playerPed, cur, 0.0, 0.0, 0.0, 1);
	}
public:
	MenuItemWeaponDropCurrent(string caption)
		: MenuItemDefault(caption) {}
};

class MenuItemTimeTitle : public MenuItemTitle
{
	virtual string GetCaption()
	{		
		time_t now = time(0);
		tm t;
		localtime_s(&t, &now);
		char str[32];
		sprintf_s(str, "%02d%s%02d", 
			TIME::GET_CLOCK_HOURS(), 
			t.tm_sec % 2 ? ":" : " ", 
			TIME::GET_CLOCK_MINUTES()
		);
		return MenuItemTitle::GetCaption() + "   " + str;
	}
public:
	MenuItemTimeTitle(string caption)
		: MenuItemTitle(caption) {}
};

class MenuItemTimeAdjust : public MenuItemDefault
{
	int m_difHours;
	virtual void OnSelect()
	{
		TIME::ADD_TO_CLOCK_TIME(m_difHours, 0, 0);
	}
public:
	MenuItemTimeAdjust(string caption, int difHours)
		: MenuItemDefault(caption),
			m_difHours(difHours) {}
};

class MenuItemTimePause : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		TIME::PAUSE_CLOCK(newState, 0);
		SetState(newState);
	}
public:
	MenuItemTimePause(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemTimeRealistic : public MenuItemSwitchable
{
	int m_difHour;
	int m_difMin;
	virtual void OnSelect()
	{
		bool newState = !GetState();
		if (newState)
		{
			time_t now = time(0);
			tm t;
			localtime_s(&t, &now);
			m_difHour = TIME::GET_CLOCK_HOURS() - t.tm_hour;
			m_difMin = TIME::GET_CLOCK_MINUTES() - t.tm_min;
		}
		SetState(newState);
	}
	virtual void OnFrame()
	{
		if (!GetState())
			return;
		time_t now = time(0);
		tm t;
		localtime_s(&t, &now);
		int hours = t.tm_hour + m_difHour;
		int mins = t.tm_min + m_difMin;
		if (mins >= 60)
		{
			mins -= 60;
			hours++;
		} else
		if (mins < 0)
		{
			mins += 60;
			hours--;
		}
		if (hours >= 24)
			hours -= 24;
		else
		if (hours < 0)
			hours += 24;
		TIME::SET_CLOCK_TIME(hours, mins, t.tm_sec);
	}
public:
	MenuItemTimeRealistic(string caption)
		: MenuItemSwitchable(caption),
			m_difHour(0), m_difMin(0) {}
};

class MenuItemTimeSystemSynced : public MenuItemSwitchable
{
	virtual void OnFrame()
	{
		if (GetState())
		{
			time_t now = time(0);
			tm t;
			localtime_s(&t, &now);
			TIME::SET_CLOCK_TIME(t.tm_hour, t.tm_min, t.tm_sec);
		}		
	}
public:
	MenuItemTimeSystemSynced(string caption)
		: MenuItemSwitchable(caption) {}
};


class MenuItemWeatherFreeze : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newstate = !GetState();
		if (!newstate)
			GAMEPLAY::FREEZE_WEATHER(false);
		SetState(newstate);
	}
	virtual void OnFrame()
	{
		if (GetState())
			GAMEPLAY::FREEZE_WEATHER(true);
	}
public:
	MenuItemWeatherFreeze(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemWeatherWind : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newstate = !GetState();
		if (newstate)
		{
			GAMEPLAY::SET_WIND_SPEED(50.0);
			GAMEPLAY::SET_WIND_DIRECTION(ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()));
		} else
		{
			GAMEPLAY::SET_WIND_SPEED(0.0);
		}
		SetState(newstate);
	}
public:
	MenuItemWeatherWind(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemWeatherSelect : public MenuItemDefault
{
	virtual void OnSelect()
	{
		GAMEPLAY::CLEAR_OVERRIDE_WEATHER();
		Hash weather = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(GetCaption().c_str()));		
		GAMEPLAY::SET_WEATHER_TYPE(weather, TRUE, TRUE, FALSE, 0.0, FALSE);
		GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
	}
public:
	MenuItemWeatherSelect(string caption)
		: MenuItemDefault(caption) { }
};

class MenuItemMiscHideHud : public MenuItemSwitchable
{
	virtual void OnFrame()
	{
		if (GetState())
			UI::HIDE_HUD_AND_RADAR_THIS_FRAME();
	}
public:
	MenuItemMiscHideHud(string caption)
		: MenuItemSwitchable(caption) {}
};

class MenuItemMiscAddHonor : public MenuItemDefault
{
	virtual void OnSelect()
	{
		Hash unarmed = GAMEPLAY::GET_HASH_KEY("WEAPON_UNARMED");
		WEAPON::SET_CURRENT_PED_WEAPON(PLAYER::PLAYER_PED_ID(), unarmed, 1, 0, 0, 0);
		DWORD model = GAMEPLAY::GET_HASH_KEY("U_M_O_VHTEXOTICSHOPKEEPER_01");
		STREAMING::REQUEST_MODEL(model, FALSE);
		while (!STREAMING::HAS_MODEL_LOADED(model))
			WaitAndDraw(0);
		Vector3 coords = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER::PLAYER_PED_ID(), 0.0, 3.0, -0.3);
		Ped ped = PED::CREATE_PED(model, coords.x, coords.y, coords.z, 0.0, 0, 0, 0, 0);
		PED::SET_PED_VISIBLE(ped, TRUE);
		DECORATOR::DECOR_SET_INT(ped, "honor_override", -9999);
		AI::TASK_COMBAT_PED(ped, PLAYER::PLAYER_PED_ID(), 0, 0);
		//ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&ped);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
	}
public:
	MenuItemMiscAddHonor(string caption)
		: MenuItemDefault(caption) {}
};

class MenuItemMiscRevealMap : public MenuItemDefault
{
	virtual void OnSelect()
	{
		RADAR::_SET_MINIMAP_REVEALED(TRUE);
		RADAR::REVEAL_MAP(0);
		SetStatusText("map revealed");
	}
public:
	MenuItemMiscRevealMap(string caption) :
		MenuItemDefault(caption) {}
};

class MenuItemMiscTransportGuns : public MenuItemSwitchable
{
	bool m_isHorse;
	bool m_isBullet;

	DWORD m_lastShootTime;

	virtual void OnSelect()
	{
		bool newstate = !GetState();
		if (newstate)
			SetStatusText("WARN: may cause ERR_GFX_STATE while breaking certain objects\n\nINSERT / NUM+", 5000);
		SetState(newstate);
	}

	virtual void OnFrame()
	{
		if (!GetState())
			return;

		if (!IsKeyDownLong(VK_ADD) && !IsKeyDownLong(VK_INSERT)) // num plus / insert
			return;

		if (m_lastShootTime + (m_isBullet ? 50 : 250) > GetTickCount())
			return;

		Player player = PLAYER::PLAYER_ID();
		Ped playerPed = PLAYER::PLAYER_PED_ID();

		if (!PLAYER::IS_PLAYER_CONTROL_ON(player))
			return;

		Entity transport;
		if (m_isHorse)
			if (PED::IS_PED_ON_MOUNT(playerPed))
				transport = PED::GET_MOUNT(playerPed);
			else
				return;
		else
			if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
				transport = PED::GET_VEHICLE_PED_IS_USING(playerPed);
			else
				return;

		Vector3 v0, v1;
		GAMEPLAY::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(transport), &v0, &v1);

		Hash modelHash = m_isBullet ? 0 : GAMEPLAY::GET_HASH_KEY("S_CANNONBALL");
		Hash weaponHash = GAMEPLAY::GET_HASH_KEY(m_isBullet ? "WEAPON_TURRET_GATLING" : "WEAPON_TURRET_REVOLVING_CANNON");

		if (modelHash && !STREAMING::HAS_MODEL_LOADED(modelHash))
		{
			STREAMING::REQUEST_MODEL(modelHash, FALSE);
			while (!STREAMING::HAS_MODEL_LOADED(modelHash))
				WAIT(0);
		}

		Vector3 coords0from = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(transport, -(v1.x + 0.25f), v1.y + 1.25f, 0.1);
		Vector3 coords1from = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(transport, (v1.x + 0.25f), v1.y + 1.25f, 0.1);
		Vector3 coords0to = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(transport, -v1.x, v1.y + 100.0f, 0.1f);
		Vector3 coords1to = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(transport, v1.x, v1.y + 100.0f, 0.1f);

		GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords0from.x, coords0from.y, coords0from.z,
			coords0to.x, coords0to.y, coords0to.z,
			250, 1, weaponHash, playerPed, 1, 1, -1.0, 0);
		GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords1from.x, coords1from.y, coords1from.z,
			coords1to.x, coords1to.y, coords1to.z,
			250, 1, weaponHash, playerPed, 1, 1, -1.0, 0);

		if (m_isBullet)
		{
			weaponHash = GAMEPLAY::GET_HASH_KEY("WEAPON_SNIPERRIFLE_CARCANO");
			GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords0from.x, coords0from.y, coords0from.z,
				coords0to.x, coords0to.y, coords0to.z,
				250, 1, weaponHash, playerPed, 1, 0, -1.0, 0);
			GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords1from.x, coords1from.y, coords1from.z,
				coords1to.x, coords1to.y, coords1to.z,
				250, 1, weaponHash, playerPed, 1, 0, -1.0, 0);
		}

		m_lastShootTime = GetTickCount();
	}
public:
	MenuItemMiscTransportGuns(string caption, bool isHorse, bool isBullet)
		: MenuItemSwitchable(caption),
			m_isHorse(isHorse), m_isBullet(isBullet),
			m_lastShootTime(0) {}
};

MenuBase *CreatePlayerTeleportMenu(MenuController *controller)
{
	MenuBase *menu = new MenuBase(new MenuItemListTitle("TELEPORT"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemPlayerTeleportToMarker("MARKER"));

	menu->AddItem(new MenuItemPlayerTeleport("SOUTH MAP",     { -5311.2583,  -4612.00,   -10.63389 }));
	menu->AddItem(new MenuItemPlayerTeleport("SOUTH GUAMA",   { 1315.66381,  -6815.48,   42.377101 }));
	menu->AddItem(new MenuItemPlayerTeleport("ANNESBURG",     { 2898.593994, 1239.85253, 44.073299 }));
	menu->AddItem(new MenuItemPlayerTeleport("STRAWBERRY",	  { -1725.22143, -418.11560, 153.55740 }));
	menu->AddItem(new MenuItemPlayerTeleport("VALENTINE",	  { -213.152496, 691.802979, 112.37100 }));
	menu->AddItem(new MenuItemPlayerTeleport("RHODES",		  { 1282.707520, -1275.7485, 74.945099 }));
	menu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS",   { 2336.584961, -1106.2358, 44.737598 }));
	menu->AddItem(new MenuItemPlayerTeleport("WAPITI",		  { 538.738525,  2217.46557, 240.23280 }));
	menu->AddItem(new MenuItemPlayerTeleport("BUTCHERCREEK",  { 2552.203613, 835.510010, 81.183098 }));
	menu->AddItem(new MenuItemPlayerTeleport("BLACKWATER",	  { -798.338379, -1238.9395, 43.537899 }));
	menu->AddItem(new MenuItemPlayerTeleport("BEECHERS",	  { -1653.19738, -1448.8156, 82.503502 }));
	menu->AddItem(new MenuItemPlayerTeleport("CALIGA HALL",   { 1705.509888, -1386.3237, 42.884998 }));	
	menu->AddItem(new MenuItemPlayerTeleport("BRAITHWAITE",   { 1011.190674, -1661.6768, 45.918301 }));	
	menu->AddItem(new MenuItemPlayerTeleport("VANHORN",		  { 2982.234863, 445.724915, 51.491501 }));	
	menu->AddItem(new MenuItemPlayerTeleport("CORNWALL",	  { 437.7247920, 494.582092, 107.67649 }));
	menu->AddItem(new MenuItemPlayerTeleport("COLTER",		  { -1371.6590,  2388.5073,  307.7218  }));
	menu->AddItem(new MenuItemPlayerTeleport("EMERALD RANCH", { 1332.332642, 300.425110, 86.306297 }));	
	menu->AddItem(new MenuItemPlayerTeleport("PRONGHORN",	  { -2616.57714, 519.256775, 144.10809 }));
	menu->AddItem(new MenuItemPlayerTeleport("MANZANITA POST",{ -1977.98754, -1545.6749, 112.87020 }));
	menu->AddItem(new MenuItemPlayerTeleport("LAGRAS",		  { 2111.099121, -662.25317, 41.259899 }));
	menu->AddItem(new MenuItemPlayerTeleport("ARMADILLO",	  { -3622.65527, -2586.5795, -15.36900 }));
	menu->AddItem(new MenuItemPlayerTeleport("TUMBLEWEED",	  { -5382.39453, -2940.1596, 1.582700  }));	
	menu->AddItem(new MenuItemPlayerTeleport("MACFARLANES RANCH", { -2296.26318, -2454.4101, 60.969898 }));
	menu->AddItem(new MenuItemPlayerTeleport("BENEDICT POINT",{ -5269.60400, -3411.0588, -23.15930 }));		

	return menu;
}

MenuBase *CreatePlayerChangeModelHorseMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemListTitle("HORSE  MODELS"));
	controller->RegisterMenu(menu);

	unordered_map<string, vector<pair<string, string>>> breeds;
	for each (auto &modelInfo in pedModelInfos)
		if (modelInfo.horse)
		{				
			size_t pos = modelInfo.name.find_first_of(' ');
			string breed = modelInfo.name.substr(0, pos);
			string kind = modelInfo.name.substr(pos + 1, modelInfo.name.size() - pos - 1);
			breeds[breed].push_back({ kind, modelInfo.model });
		}

	for each (auto &breed in breeds)
	{
		auto breedMenu = new MenuBase(new MenuItemListTitle(breed.first));
		controller->RegisterMenu(breedMenu);
		menu->AddItem(new MenuItemMenu(breed.first, breedMenu));
		for each (auto &kindAndModel in breed.second)
			breedMenu->AddItem(new MenuItemChangePlayerModel(kindAndModel.first, kindAndModel.second));
	}	

	return menu;
}

MenuBase *CreatePlayerChangeModelAnimalMenuExactFilter(MenuController *controller, bool horse, bool dog, bool fish)
{
	auto menu = new MenuBase(new MenuItemListTitle("ANIMAL  MODELS"));
	controller->RegisterMenu(menu);

	for each (auto &modelInfo in pedModelInfos)
		if (modelInfo.animal &&
			modelInfo.horse == horse && modelInfo.dog == dog && modelInfo.fish == fish)
				menu->AddItem(new MenuItemChangePlayerModel(modelInfo.name, modelInfo.model));

	return menu;
}

MenuBase *CreatePlayerChangeModelAnimalMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("ANIMAL  MODELS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("HORSES", CreatePlayerChangeModelHorseMenu(controller)));
	menu->AddItem(new MenuItemMenu("DOGS", CreatePlayerChangeModelAnimalMenuExactFilter(controller, false, true, false)));
	menu->AddItem(new MenuItemMenu("FISH", CreatePlayerChangeModelAnimalMenuExactFilter(controller, false, false, true)));
	menu->AddItem(new MenuItemMenu("OTHER", CreatePlayerChangeModelAnimalMenuExactFilter(controller, false, false, false)));

	return menu;
}

MenuBase *CreatePlayerChangeModelHumanMenuExactFilter(MenuController *controller, bool cutscene, bool male, bool female, bool young, bool middleaged, bool old)
{
	auto menu = new MenuBase(new MenuItemListTitle("PED  MODELS"));
	controller->RegisterMenu(menu);

	for each (auto &modelInfo in pedModelInfos)
		if (!modelInfo.animal &&
			modelInfo.cutscene == cutscene && modelInfo.male == male && modelInfo.female == female &&
			modelInfo.young == young && modelInfo.middleaged == middleaged && modelInfo.old == old)
				menu->AddItem(new MenuItemChangePlayerModel(modelInfo.name, modelInfo.model));

	return menu;
}

MenuBase *CreatePlayerChangeModelHumanMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("PED  MODELS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("CUTSCENE", CreatePlayerChangeModelHumanMenuExactFilter(controller, true, false, false, false, false, false)));
	menu->AddItem(new MenuItemMenu("MALE YOUNG", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, true, false, true, false, false)));
	menu->AddItem(new MenuItemMenu("MALE MIDDLE", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, true, false, false, true, false)));
	menu->AddItem(new MenuItemMenu("MALE OLD", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, true, false, false, false, true)));
	menu->AddItem(new MenuItemMenu("FEMALE YOUNG", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, false, true, true, false, false)));
	menu->AddItem(new MenuItemMenu("FEMALE MIDDLE", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, false, true, false, true, false)));
	menu->AddItem(new MenuItemMenu("FEMALE OLD", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, false, true, false, false, true)));
	menu->AddItem(new MenuItemMenu("MISC", CreatePlayerChangeModelHumanMenuExactFilter(controller, false, false, false, false, false, false)));

	return menu;
}

MenuBase *CreatePlayerChangeModelMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("SKIN  CHANGER"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("ANIMALS", CreatePlayerChangeModelAnimalMenu(controller)));
	menu->AddItem(new MenuItemMenu("PEDS", CreatePlayerChangeModelHumanMenu(controller)));

	return menu;
}

MenuBase *CreatePlayerWantedMenu(MenuController *controller)
{
	MenuBase *menu = new MenuBase(new MenuItemTitle("WANTED  OPTIONS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemPlayerClearWanted("CLEAR BOUNTY", true, false));
	menu->AddItem(new MenuItemPlayerClearWanted("CLEAR WANTED", true, true));
	menu->AddItem(new MenuItemPlayerNeverWanted("NEVER WANTED"));

	return menu;
}

MenuBase *CreatePlayerTransportMenu(MenuController *controller)
{
	MenuBase *menu = new MenuBase(new MenuItemTitle("TRANSPORT  OPTIONS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemPlayerHorseInvincible("INVINCIBLE HORSE"));
	menu->AddItem(new MenuItemPlayerHorseUnlimStamina("UNLIM HORSE STAMINA"));
	menu->AddItem(new MenuItemVehicleBoost("VEHICLE BOOST"));

	return menu;
}

MenuBase *CreatePlayerMiscMenu(MenuController *controller)
{
	MenuBase *menu = new MenuBase(new MenuItemTitle("PLAYER  MISC"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemPlayerEveryoneIgnored("EVERYONE IGNORED"));
	menu->AddItem(new MenuItemPlayerNoiseless("NOISELESS"));
	menu->AddItem(new MenuItemPlayerSuperJump("SUPER JUMP"));

	return menu;
}

MenuBase *CreatePlayerMenu(MenuController *controller)
{
	MenuBase *menu = new MenuBase(new MenuItemTitle("PLAYER  OPTIONS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("TRANSPORT", CreatePlayerTransportMenu(controller)));
	menu->AddItem(new MenuItemMenu("TELEPORT", CreatePlayerTeleportMenu(controller)));
	menu->AddItem(new MenuItemMenu("SKINS", CreatePlayerChangeModelMenu(controller)));
	menu->AddItem(new MenuItemMenu("WANTED", CreatePlayerWantedMenu(controller)));
	menu->AddItem(new MenuItemPlayerFix("FIX PLAYER"));
	menu->AddItem(new MenuItemPlayerAddCash("ADD CASH", 1000 * 100));
	menu->AddItem(new MenuItemPlayerFastHeal("FAST HEAL"));
	menu->AddItem(new MenuItemPlayerInvincible("INVINCIBLE"));	
	menu->AddItem(new MenuItemPlayerUnlimStamina("UNLIM STAMINA"));	
	menu->AddItem(new MenuItemPlayerUnlimAbility("UNLIM ABILITY"));
	menu->AddItem(new MenuItemMenu("MISC", CreatePlayerMiscMenu(controller)));

	return menu;
}

MenuBase *CreateHorseSpawnerMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemListTitle("HORSE  SPAWNER"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemSpawnHorseRandom("random horse"));

	unordered_map<string, vector<pair<string, string>>> breeds;
	for each (auto &modelInfo in pedModelInfos)
		if (modelInfo.horse)
		{				
			size_t pos = modelInfo.name.find_first_of(' ');
			string breed = modelInfo.name.substr(0, pos);
			string kind = modelInfo.name.substr(pos + 1, modelInfo.name.size() - pos - 1);
			breeds[breed].push_back({ kind, modelInfo.model });
		}

	for each (auto &breed in breeds)
	{
		auto breedMenu = new MenuBase(new MenuItemListTitle(breed.first));
		controller->RegisterMenu(breedMenu);
		menu->AddItem(new MenuItemMenu(breed.first, breedMenu));
		for each (auto &kindAndModel in breed.second)
			breedMenu->AddItem(new MenuItemSpawnPed(kindAndModel.first, kindAndModel.second));
	}	

	return menu;
}

MenuBase *CreateAnimalSpawnerMenuExactFilter(MenuController *controller, bool horse, bool dog, bool fish)
{
	auto menu = new MenuBase(new MenuItemListTitle("ANIMAL  SPAWNER"));
	controller->RegisterMenu(menu);

	for each (auto &modelInfo in pedModelInfos)
		if (modelInfo.animal &&
			modelInfo.horse == horse && modelInfo.dog == dog && modelInfo.fish == fish)
				menu->AddItem(new MenuItemSpawnPed(modelInfo.name, modelInfo.model));

	return menu;
}

MenuBase *CreateAnimalSpawnerMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("ANIMAL  SPAWNER"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemSpawnAnimalRandom("RANDOM"));
	menu->AddItem(new MenuItemMenu("HORSES", CreateHorseSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("DOGS", CreateAnimalSpawnerMenuExactFilter(controller, false, true, false)));
	menu->AddItem(new MenuItemMenu("FISH", CreateAnimalSpawnerMenuExactFilter(controller, false, false, true)));
	menu->AddItem(new MenuItemMenu("OTHER", CreateAnimalSpawnerMenuExactFilter(controller, false, false, false)));

	return menu;
}

MenuBase *CreateHumanSpawnerMenuExactFilter(MenuController *controller, bool cutscene, bool male, bool female, bool young, bool middleaged, bool old)
{
	auto menu = new MenuBase(new MenuItemListTitle("PED  SPAWNER"));
	controller->RegisterMenu(menu);

	for each (auto &modelInfo in pedModelInfos)
		if (!modelInfo.animal &&
			modelInfo.cutscene == cutscene && modelInfo.male == male && modelInfo.female == female &&
			modelInfo.young == young && modelInfo.middleaged == middleaged && modelInfo.old == old)
				menu->AddItem(new MenuItemSpawnPed(modelInfo.name, modelInfo.model));

	return menu;
}

MenuBase *CreateHumanSpawnerMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("PED  SPAWNER"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemSpawnPedRandom("RANDOM PED"));
	menu->AddItem(new MenuItemMenu("CUTSCENE", CreateHumanSpawnerMenuExactFilter(controller, true, false, false, false, false, false)));
	menu->AddItem(new MenuItemMenu("MALE YOUNG", CreateHumanSpawnerMenuExactFilter(controller, false, true, false, true, false, false)));
	menu->AddItem(new MenuItemMenu("MALE MIDDLE", CreateHumanSpawnerMenuExactFilter(controller, false, true, false, false, true, false)));
	menu->AddItem(new MenuItemMenu("MALE OLD", CreateHumanSpawnerMenuExactFilter(controller, false, true, false, false, false, true)));
	menu->AddItem(new MenuItemMenu("FEMALE YOUNG", CreateHumanSpawnerMenuExactFilter(controller, false, false, true, true, false, false)));
	menu->AddItem(new MenuItemMenu("FEMALE MIDDLE", CreateHumanSpawnerMenuExactFilter(controller, false, false, true, false, true, false)));
	menu->AddItem(new MenuItemMenu("FEMALE OLD", CreateHumanSpawnerMenuExactFilter(controller, false, false, true, false, false, true)));
	menu->AddItem(new MenuItemMenu("MISC", CreateHumanSpawnerMenuExactFilter(controller, false, false, false, false, false, false)));	

	return menu;
}

enum eVehicleType
{
	vtAirbaloon,
	vtBoat,
	vtCannon,
	vtTrain,	
	vtWagon
};

eVehicleType GetVehicleTypeUsingModel(string model)
{
	Hash hash = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(model.c_str()));
	if (VEHICLE::IS_THIS_MODEL_A_BOAT(hash))
		return vtBoat;
	if (VEHICLE::IS_THIS_MODEL_A_TRAIN(hash))
		return vtTrain;
	if (model == "gatling_gun" || model == "gatlingMaxim02" || model == "hotchkiss_cannon" || model == "breach_cannon")
		return vtCannon;
	if (model == "hotAirBalloon01")
		return vtAirbaloon;
	return vtWagon;
}

MenuBase *CreateCannonSpawnerMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("CANNON  SPAWNER"));
	controller->RegisterMenu(menu);

	auto menuItemWrapIn = new MenuItemSwitchable("WRAP IN SPAWNED");
	menu->AddItem(menuItemWrapIn);

	auto menuItemSetProperly = new MenuItemSwitchable("SET PROPERLY");
	menu->AddItem(menuItemSetProperly);

	for each (auto &model in vehicleModels)
		if (GetVehicleTypeUsingModel(model) == vtCannon)
			menu->AddItem(new MenuItemSpawnVehicle(model, { 0.0, 3.0, 0.0 }, 0.0, menuItemWrapIn, menuItemSetProperly, true, false));

	return menu;
}

MenuBase *CreateBoatSpawnerMenu(MenuController *controller, 
	MenuItemSwitchable *menuItemWrapIn, MenuItemSwitchable *menuItemSetProperly)
{
	auto menu = new MenuBase(new MenuItemListTitle("BOAT  SPAWNER"));
	controller->RegisterMenu(menu);

	for each (auto &model in vehicleModels)
		if (GetVehicleTypeUsingModel(model) == vtBoat)
			menu->AddItem(new MenuItemSpawnVehicle(model, { 0.0, 10.0, 0.0 }, 90.0, menuItemWrapIn, menuItemSetProperly, false, false));

	return menu;
}

MenuBase *CreateTrainSpawnerMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemListTitle("TRAIN  SPAWNER"));
	controller->RegisterMenu(menu);

	for each (auto &model in vehicleModels)
		if (GetVehicleTypeUsingModel(model) == vtTrain)
			menu->AddItem(new MenuItemSpawnVehicle(model, { 0.0, 5.0, -1.0 }, 90.0, NULL, NULL, false, false));

	return menu;
}

MenuBase *CreateWagonSpawnerMenu(MenuController *controller, 
	MenuItemSwitchable *menuItemWrapIn, MenuItemSwitchable *menuItemSetProperly, bool noPeds)
{
	auto menu = new MenuBase(new MenuItemListTitle("WAGON  SPAWNER"));
	controller->RegisterMenu(menu);

	for each (auto &model in vehicleModels)
		if (GetVehicleTypeUsingModel(model) == vtWagon)
			menu->AddItem(new MenuItemSpawnVehicle(model, { 1.0, 5.0, 0.0 }, 90.0, menuItemWrapIn, menuItemSetProperly, true, noPeds));

	return menu;
}

MenuBase *CreateVehicleMiscSpawnerMenu(MenuController *controller, MenuItemSwitchable *menuItemWrapIn)
{
	auto menu = new MenuBase(new MenuItemListTitle("MISC  SPAWNER"));
	controller->RegisterMenu(menu);

	for each (auto &model in vehicleModels)
		if (GetVehicleTypeUsingModel(model) == vtAirbaloon)
			menu->AddItem(new MenuItemSpawnVehicle(model, { 0.0, 5.0, 0.0 }, 0.0, menuItemWrapIn, NULL, false, false));

	return menu;
}

MenuBase *CreateVehicleSpawnerMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("VEHICLE  SPAWNER"));
	controller->RegisterMenu(menu);

	auto menuItemWrapIn = new MenuItemSwitchable("WRAP IN SPAWNED");
	menu->AddItem(menuItemWrapIn);

	auto menuItemSetProperly = new MenuItemSwitchable("SET PROPERLY");
	menu->AddItem(menuItemSetProperly);

	menu->AddItem(new MenuItemMenu("BOATS", CreateBoatSpawnerMenu(controller, menuItemWrapIn, menuItemSetProperly)));
	menu->AddItem(new MenuItemMenu("TRAINS", CreateTrainSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("WAGONS", CreateWagonSpawnerMenu(controller, menuItemWrapIn, menuItemSetProperly, false)));
	menu->AddItem(new MenuItemMenu("JUST WAGONS", CreateWagonSpawnerMenu(controller, menuItemWrapIn, menuItemSetProperly, true)));
	menu->AddItem(new MenuItemMenu("MISC", CreateVehicleMiscSpawnerMenu(controller, menuItemWrapIn)));

	return menu;
}

MenuBase *CreateWeaponSelectMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemListTitle("GET WEAPON"));
	controller->RegisterMenu(menu);

	for each (auto info in weaponInfos)
		menu->AddItem(new MenuItemGiveWeapon(info.uiname, info.name));

	return menu;
}

MenuBase *CreateWeaponMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("WEAPON  OPTIONS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("GET ALL WEAPON", CreateWeaponSelectMenu(controller)));
	menu->AddItem(new MenuItemWeaponPowerfullGuns("POWERFULL GUNS"));
	menu->AddItem(new MenuItemWeaponPowerfullMelee("POWERFULL MELEE"));
	menu->AddItem(new MenuItemWeaponNoReload("NO RELOAD"));
	menu->AddItem(new MenuItemWeaponDropCurrent("DROP CURRENT"));

	return menu;
}

MenuBase *CreateTimeMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTimeTitle("TIME"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemTimeAdjust("HOUR FORWARD", 1));
	menu->AddItem(new MenuItemTimeAdjust("HOUR BACKWARD", -1));
	menu->AddItem(new MenuItemTimePause("CLOCK PAUSED"));
	menu->AddItem(new MenuItemTimeRealistic("CLOCK REAL"));
	menu->AddItem(new MenuItemTimeSystemSynced("SYNC WITH SYSTEM"));

	return menu;
}

MenuBase *CreateWeatherMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemListTitle("WEATHER"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemWeatherFreeze("FREEZE CURRENT"));
	menu->AddItem(new MenuItemWeatherWind("FAST WIND"));

	for each (string weather in weatherTypes)
		menu->AddItem(new MenuItemWeatherSelect(weather));

	return menu;
}

MenuBase *CreateMiscMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("MISC  OPTIONS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMiscRevealMap("REVEAL MAP"));
	menu->AddItem(new MenuItemMiscAddHonor("ADD HONOR"));
	menu->AddItem(new MenuItemMiscHideHud("HIDE HUD"));

	menu->AddItem(new MenuItemMiscTransportGuns("HORSE TURRETS", true, true));
	menu->AddItem(new MenuItemMiscTransportGuns("HORSE CANNONS", true, false));
	menu->AddItem(new MenuItemMiscTransportGuns("VEHICLE TURRETS", false, true));
	menu->AddItem(new MenuItemMiscTransportGuns("VEHICLE CANNONS", false, false));

	return menu;
}

MenuBase *CreateMainMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("NATIVE  TRAINER  (AB)"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("PLAYER", CreatePlayerMenu(controller)));
	menu->AddItem(new MenuItemMenu("ANIMALS", CreateAnimalSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("PEDS", CreateHumanSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("CANNONS", CreateCannonSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("VEHICLES", CreateVehicleSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("WEAPON", CreateWeaponMenu(controller)));
	menu->AddItem(new MenuItemMenu("TIME", CreateTimeMenu(controller)));
	menu->AddItem(new MenuItemMenu("WEATHER", CreateWeatherMenu(controller)));
	menu->AddItem(new MenuItemMenu("MISC", CreateMiscMenu(controller)));
	
	return menu;
}

void main()
{
	auto menuController = new MenuController();
	auto mainMenu = CreateMainMenu(menuController);
	// One-time ready announcement
	static bool greeted = false;
	if (!greeted) {
		A11y::speak(L"Accessibility mod loaded. Ready.", true);
		greeted = true;
	}

	static bool announcedReady = false;
	static DWORD startupMs = GetTickCount();
	// Track last aimed entity to avoid repeating announcements
	static Entity lastAimedEntity = 0;
	static DWORD lastAimSpeakMs = 0;
	static int lastAimedDistM = -1;
	static int lastAimedHealthPct = -1;
	static int camRayHandle = 0;
	static Entity camRayLastIgnore = 0;
	static Vector3 camRayEnd{};
	static Vector3 camRayNorm{};
	// Attribution for money changes (recent targeted ped)
	static Entity lastTargetPedForMoney = 0;
	static DWORD lastTargetPedForMoneyMs = 0;
	static wchar_t lastTargetPedForMoneyName[128] = {0};
	// Track current weapon to announce on change and on-demand
	static Hash lastWeaponHash = 0;              // last announced weapon hash
	static DWORD lastWeaponAnnounceMs = 0;       // last announcement time
	static Hash weaponCandidateHash = 0;         // current candidate weapon
	static DWORD weaponCandidateSinceMs = 0;     // since when the candidate is stable
	// Bodyguard state
	static Ped g_bodyguard = 0;                  // current bodyguard ped
	static DWORD g_bodyguardLastTaskMs = 0;
	// Wolf companion state
	static Ped g_wolf = 0;
	static DWORD g_wolfLastTaskMs = 0;
	// Respawn scheduling
	static DWORD g_guardRespawnAtMs = 0;
	static DWORD g_wolfRespawnAtMs = 0;
	// Pending human attack after wolf falls
	static Entity g_pendingAttackTarget = 0; // when wolf dies, human will attack this
	static DWORD g_humanThreatUntilMs = 0;   // small aim window to simulate threats
	// Wolf pack assist state
	static Ped g_wolfPack[8] = { 0 };
	static int g_wolfPackSize = 0;
	static DWORD g_nextHowlAtMs = 0;
	static int g_howlsRemaining = 0;
	static DWORD g_lastAttackSeenMs = 0;
	static DWORD g_packCooldownUntilMs = 0;
	// User-requested persistent pack target size (number of helpers to keep around)
	static int g_targetPackSize = 0; // 0 means no persistent pack; manual howls will fill up to this
	const int MAX_WOLF_PACK = 6;     // realistic cap of helpers (not counting the main wolf)

	// Helper: safe wolf howl cue; try a real anim if available, else stand-still as a cue
	static auto TryWolfHowl = [](Ped p) -> bool {
		if (!p || !ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p)) return false;
		// Try a few likely anim dict/name pairs non-blocking; if cached they'll play, else we gracefully fall back
		struct Pair { const char* dict; const char* name; };
		static const Pair cands[] = {
			{"creatures_mammal@wolf@normal@idle", "howl"},
			{"creatures_mammal@wolf@interaction@idle", "howl"},
			{"ai_ambient@animal@wolf@behaviour@howl", "howl"}
		};
		for (auto &c : cands) {
			STREAMING::REQUEST_ANIM_DICT((char*)c.dict);
			if (STREAMING::HAS_ANIM_DICT_LOADED((char*)c.dict)) {
				AI::TASK_PLAY_ANIM(p, (char*)c.dict, (char*)c.name, 8.0f, 1.0f, 1600, 0, 0.0f, FALSE, FALSE, FALSE, 0, FALSE);
				return true;
			}
		}
		// Fallback cue (guaranteed safe)
		AI::TASK_STAND_STILL(p, 900);
		return true;
	};

	// Helper: safe player call cue (no audio native to avoid crashes)
	static auto TryPlayerCall = [](Ped playerPed) -> bool {
		if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed) || ENTITY::IS_ENTITY_DEAD(playerPed)) return false;
		AI::TASK_STAND_STILL(playerPed, 400);
		return true;
	};

	// Deferred howl after player's call for realism
	static DWORD g_forcedHowlAtMs = 0; static bool g_forcedHowlAll = false;

	// Helper: try safe frontend sounds for audible feedback (names may no-op but won’t crash)
	static auto TryFrontendBeep = []() {
		// Try a few common cues; if invalid, they silently do nothing
		AUDIO::PLAY_SOUND_FRONTEND((char*)"SELECT", (char*)"HUD_SHOP_SOUNDSET", FALSE, FALSE);
		AUDIO::PLAY_SOUND_FRONTEND((char*)"NAV_UP_DOWN", (char*)"HUD_SHOP_SOUNDSET", FALSE, FALSE);
		AUDIO::PLAY_SOUND_FRONTEND((char*)"TOGGLE_ON", (char*)"HUD_SHOP_SOUNDSET", FALSE, FALSE);
	};
	
	while (true)
	{		
		if (!menuController->HasActiveMenu() && MenuInput::MenuSwitchPressed())
		{
			MenuInput::MenuInputBeep();
			menuController->PushMenu(mainMenu);
			// Deferred: no immediate speech on open to avoid early-load issues.
		}

		// NumPad 1: announce player's health (only when menu is closed)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD1))
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			int health = ENTITY::GET_ENTITY_HEALTH(playerPed);
			int maxHealth = PED::GET_PED_MAX_HEALTH(playerPed);
			if (maxHealth <= 0) maxHealth = 1; // avoid division by zero
			if (health > maxHealth) health = maxHealth; // clamp
			int percent = static_cast<int>((health * 100.0f) / static_cast<float>(maxHealth) + 0.5f);
			if (percent < 0) percent = 0; if (percent > 100) percent = 100;
			wchar_t buf[64];
			swprintf_s(buf, L"health %d (%d%%)", health, percent);
			A11y::speak(buf, true);

			// Heuristic getters for core values (stamina/dead eye) via ATTRIBUTE namespace.
			auto getCorePercent = [](Ped ped, int attrIdx) -> int {
				// Try two candidate natives; interpret as either 0..1 or 0..100.
				float v1 = ATTRIBUTE::_0x4C9F782180712742(ped, attrIdx);
				float v2 = ATTRIBUTE::_0xB429F58803D285B1(ped, attrIdx);
				float v = (v1 > 0.0f ? v1 : (v2 > 0.0f ? v2 : 0.0f));
				if (v <= 0.0f) return -1; // unknown/unavailable (avoid reporting 0 unless confident)
				if (v <= 1.0f) return static_cast<int>(v * 100.0f + 0.5f);
				if (v <= 100.0f) return static_cast<int>(v + 0.5f);
				return -1;
			};

			int staminaPct = getCorePercent(playerPed, 1); // 0: health, 1: stamina, 2: dead eye (convention)
			int deadeyePct = getCorePercent(playerPed, 2);

			// Then announce player context: state, speed, cores, wanted (no ammo to avoid instability)
			bool onMount = PED::IS_PED_ON_MOUNT(playerPed) ? true : false;
			bool inVeh = PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0) ? true : false;
			const char* state = onMount ? "mount" : (inVeh ? "vehicle" : "on foot");
			Entity speedEnt = playerPed;
			if (onMount) {
				Entity m = PED::GET_MOUNT(playerPed);
				if (ENTITY::DOES_ENTITY_EXIST(m)) speedEnt = m;
			}
			else if (inVeh) {
				Entity v = PED::GET_VEHICLE_PED_IS_USING(playerPed);
				if (ENTITY::DOES_ENTITY_EXIST(v)) speedEnt = v;
			}
			int spd = static_cast<int>(ENTITY::GET_ENTITY_SPEED(speedEnt) * 3.6f + 0.5f);
			const char* move = "idle";
			if (!inVeh) {
				if (spd >= 14) move = "sprinting";
				else if (spd >= 7) move = "running";
				else if (spd >= 2) move = "walking";
			}
			// Ammo intentionally omitted
			int wanted = PURSUIT::GET_PLAYER_WANTED_INTENSITY(PLAYER::PLAYER_ID());
			bool isWanted = wanted > 0;

			std::string ctx;
			ctx.reserve(224);
			// base movement/speed
			{
				char t[96];
				snprintf(t, sizeof(t), "%s, %s, speed %d", state, move, spd);
				ctx += t;
			}
			// cores
			if (staminaPct >= 0) {
				char t[32]; snprintf(t, sizeof(t), ", stamina %d%%", staminaPct);
				ctx += t;
			}
			if (deadeyePct >= 0) {
				char t[32]; snprintf(t, sizeof(t), ", dead eye %d%%", deadeyePct);
				ctx += t;
			}
			// mounted horse stamina (if available)
			if (onMount) {
				Ped horse = PED::GET_MOUNT(playerPed);
				if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse)) {
					int horseStam = getCorePercent(horse, 1);
					if (horseStam >= 0) {
						char t[40]; snprintf(t, sizeof(t), ", horse stamina %d%%", horseStam);
						ctx += t;
					}
				}
			}
			// ammo omitted
			// wanted
			if (isWanted) ctx += ", wanted";

			std::wstring wctx(ctx.begin(), ctx.end());
			A11y::speak(wctx, false);
		}

		// NumPad 2: announce what you're riding/driving (only when menu is closed)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD2))
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			std::wstring wout;
			if (PED::IS_PED_ON_MOUNT(playerPed))
			{
				Ped mount = PED::GET_MOUNT(playerPed);
				if (!ENTITY::DOES_ENTITY_EXIST(mount)) { wout = L"mount"; if (!wout.empty()) A11y::speak(wout, true); goto after_num2; }
				Hash model = ENTITY::GET_ENTITY_MODEL(mount);
				std::string name;
				for each (auto &info in pedModelInfos)
				{
					if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(info.model.c_str())) == model) { name = info.name; break; }
				}
				int h = ENTITY::GET_ENTITY_HEALTH(mount);
				int mh = PED::GET_PED_MAX_HEALTH(mount); if (mh <= 0) mh = 1;
				if (h > mh) h = mh; // clamp
				int p = static_cast<int>((h * 100.0f) / static_cast<float>(mh) + 0.5f);
				if (p < 0) p = 0; if (p > 100) p = 100;
				wchar_t wname[64]; wname[0] = L'\0';
				if (!name.empty()) { size_t n = name.size(); if (n > 63) n = 63; size_t converted = 0; mbstowcs_s(&converted, wname, 64, name.c_str(), n); wname[n] = L'\0'; }
				wchar_t buf[128];
				if (wname[0]) swprintf_s(buf, L"mount: %s, health %d (%d%%)", wname, h, p);
				else swprintf_s(buf, L"mount, health %d (%d%%)", h, p);
				wout = buf;
			}
			else if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
			{
				Vehicle veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
				if (!ENTITY::DOES_ENTITY_EXIST(veh)) { wout = L"vehicle"; if (!wout.empty()) A11y::speak(wout, true); goto after_num2; }
				Hash model = ENTITY::GET_ENTITY_MODEL(veh);
				bool isTrain = VEHICLE::IS_THIS_MODEL_A_TRAIN(model) ? true : false;
				bool isBoat = VEHICLE::IS_THIS_MODEL_A_BOAT(model) ? true : false;
				std::string kind = isTrain ? "train" : (isBoat ? "boat" : "vehicle");
				std::string modelName;
				for each (auto &vm in vehicleModels)
				{
					if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(vm.c_str())) == model) { modelName = vm; break; }
				}
				float spd = ENTITY::GET_ENTITY_SPEED(veh) * 3.6f; // km/h
				int ispd = static_cast<int>(spd + 0.5f);
				wchar_t wmodel[96]; wmodel[0] = L'\0';
				if (!modelName.empty()) { size_t n = modelName.size(); if (n > 95) n = 95; mbstowcs(wmodel, modelName.c_str(), n); wmodel[n] = L'\0'; }
				wchar_t buf[160];
				if (wmodel[0]) swprintf_s(buf, L"%hs: %s, speed %d", kind.c_str(), wmodel, ispd);
				else swprintf_s(buf, L"%hs, speed %d", kind.c_str(), ispd);
				wout = buf;
			}
			else
			{
				wout = L"not mounted";
			}
			if (!wout.empty()) A11y::speak(wout, true);
			after_num2:;
		}
		// NumPad 4: feed horse (accessible alternative to vanilla menu)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD4))
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (PED::IS_PED_ON_MOUNT(playerPed))
			{
				Ped horse = PED::GET_MOUNT(playerPed);
				if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse))
				{
					int h = ENTITY::GET_ENTITY_HEALTH(horse);
					int mh = PED::GET_PED_MAX_HEALTH(horse); if (mh <= 0) mh = 1; if (h > mh) h = mh;
					int add = mh / 4; if (add < 10) add = 10; // modest heal
					int newH = h + add; if (newH > mh) newH = mh;
					ENTITY::SET_ENTITY_HEALTH(horse, newH, FALSE);
					PED::SET_PED_STAMINA(horse, 100.0);
					int pct = static_cast<int>((newH * 100.0f) / static_cast<float>(mh) + 0.5f); if (pct > 100) pct = 100; if (pct < 0) pct = 0;
					wchar_t buf[128]; swprintf_s(buf, L"fed horse, health %d (%d%%), stamina 100%%", newH, pct);
					A11y::speak(buf, true);
				}
				else
				{
					A11y::speak(L"no valid horse", true);
				}
			}
			else
			{
				A11y::speak(L"not on horse", true);
			}
		}

		// NumPad 5: brush horse (clean wetness as simple accessible proxy)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD5))
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (PED::IS_PED_ON_MOUNT(playerPed))
			{
				Ped horse = PED::GET_MOUNT(playerPed);
				if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse))
				{
					PED::CLEAR_PED_WETNESS(horse);
					A11y::speak(L"brushed horse", true);
				}
				else
				{
					A11y::speak(L"no valid horse", true);
				}
			}
			else
			{
				A11y::speak(L"not on horse", true);
			}
		}

		// Removed on-demand camera scan (NumPad 0); automatic announcements occur while aiming only

	// NumPad 3: on-demand current weapon info (name only)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD3))
		{
			Ped ped = PLAYER::PLAYER_PED_ID();
			if (!ENTITY::DOES_ENTITY_EXIST(ped)) { A11y::speak(L"unarmed", true); goto after_num3; }
			Hash cur = 0; Hash unarmed = GAMEPLAY::GET_HASH_KEY("WEAPON_UNARMED");
			if (WEAPON::GET_CURRENT_PED_WEAPON(ped, &cur, 0, 0, 0) && cur && WEAPON::IS_WEAPON_VALID(cur) && cur != unarmed && WEAPON::HAS_PED_GOT_WEAPON(ped, cur, 0, 0))
			{
				// Lookup friendly name
				std::string uname;
				for each (auto info in weaponInfos)
				{
					Hash wh = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(("WEAPON_" + info.name).c_str()));
					if (wh == cur) { uname = info.uiname; break; }
				}
		wchar_t wname[96]; wname[0] = L'\0';
		if (!uname.empty()) { size_t n = uname.size(); if (n > 95) n = 95; mbstowcs(wname, uname.c_str(), n); wname[n] = L'\0'; }
		if (wname[0]) A11y::speak(wname, true);
		else A11y::speak(L"weapon", true);
			}
			else
			{
				A11y::speak(L"unarmed", true);
			}
			after_num3:;
		}

		// NumPad 6: on-demand money total (USD dollars)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD6))
		{
			Ped me = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(me)) {
				int cur = ReadPlayerMoneyCents(me);
				// Fallback to last-known stable money if native returns 0
				extern int g_lastKnownMoneyCents; if (cur <= 0 && g_lastKnownMoneyCents > 0) cur = g_lastKnownMoneyCents;
				if (cur < 0) cur = 0;
				int d = cur / 100; int c = cur % 100; if (c < 0) c = -c;
				wchar_t buf[64];
				swprintf_s(buf, L"You have %d.%02d$", d, c);
				A11y::speak(buf, true);
			}
		}

		// NumPad 8: toggle bodyguard (spawn/dismiss)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD8))
		{
			if (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard))
			{
				// Release from group and tasks, make him wander naturally
				PED::REMOVE_PED_FROM_GROUP(g_bodyguard);
				AI::CLEAR_PED_TASKS_IMMEDIATELY(g_bodyguard, TRUE, TRUE);
				Vector3 here = ENTITY::GET_ENTITY_COORDS(g_bodyguard, TRUE, FALSE);
				AI::TASK_WANDER_IN_AREA(g_bodyguard, here.x, here.y, here.z, 20.0f, 3.0f, 5.0f, TRUE);
				ENTITY::SET_ENTITY_INVINCIBLE(g_bodyguard, FALSE);
				ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&g_bodyguard);
				g_bodyguard = 0; g_bodyguardLastTaskMs = 0;
				A11y::speak(L"bodyguard dismissed", true);
			}
			else
			{
				Ped me = PLAYER::PLAYER_PED_ID(); if (!ENTITY::DOES_ENTITY_EXIST(me)) { A11y::speak(L"no player", true); goto after_bg_toggle; }
				// Choose a guard model
				DWORD model = GAMEPLAY::GET_HASH_KEY("S_M_M_CORNWALLGUARD_01");
				if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model)) { A11y::speak(L"model invalid", true); goto after_bg_toggle; }
				STREAMING::REQUEST_MODEL(model, FALSE);
				while (!STREAMING::HAS_MODEL_LOADED(model)) WAIT(0);
				Vector3 pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(me, 0.0f, -1.8f, 0.0f);
				Ped p = PED::CREATE_PED(model, pos.x, pos.y, pos.z, ENTITY::GET_ENTITY_HEADING(me), 0, 0, 0, 0);
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
				if (!p || !ENTITY::DOES_ENTITY_EXIST(p)) { A11y::speak(L"spawn failed", true); goto after_bg_toggle; }
				PED::SET_PED_VISIBLE(p, TRUE);
				// Friendly relationships (match player's default group)
				Hash myGroup = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(me);
				PED::SET_PED_RELATIONSHIP_GROUP_HASH(p, myGroup);
				int grp = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
				if (grp) PED::SET_PED_AS_GROUP_MEMBER(p, grp);
				// Make him durable and capable
				ENTITY::SET_ENTITY_INVINCIBLE(p, TRUE);
				PED::SET_PED_ACCURACY(p, 70);
				PED::SET_PED_COMBAT_ABILITY(p, 2);
				PED::SET_PED_COMBAT_MOVEMENT(p, 2);
				// Arm him
				Hash w = GAMEPLAY::GET_HASH_KEY("WEAPON_REPEATER_CARBINE");
				WEAPON::GIVE_DELAYED_WEAPON_TO_PED(p, w, 120, 1, 0x2cd419dc);
				WEAPON::SET_CURRENT_PED_WEAPON(p, w, 1, 0, 0, 0);
				// Follow player
				AI::TASK_CLEAR_LOOK_AT(p);
				AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(p, me, 0.0f, -1.6f, 0.0f, 2.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
				g_bodyguard = p; g_bodyguardLastTaskMs = GetTickCount();
				A11y::speak(L"bodyguard ready", true);
			}
			after_bg_toggle:;
		}

		// NumPad 9: toggle wolf companion (spawn/dismiss)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD9))
		{
			if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf))
			{
				AI::CLEAR_PED_TASKS_IMMEDIATELY(g_wolf, TRUE, TRUE);
				Vector3 here = ENTITY::GET_ENTITY_COORDS(g_wolf, TRUE, FALSE);
				AI::TASK_WANDER_IN_AREA(g_wolf, here.x, here.y, here.z, 30.0f, 4.0f, 6.0f, TRUE);
				ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&g_wolf);
				g_wolf = 0; g_wolfLastTaskMs = 0; g_wolfRespawnAtMs = 0;
				// Dismiss wolf pack if any
				for (int i = 0; i < g_wolfPackSize; ++i) {
					Ped wp = g_wolfPack[i];
					if (wp && ENTITY::DOES_ENTITY_EXIST(wp)) {
						Vector3 wph = ENTITY::GET_ENTITY_COORDS(wp, TRUE, FALSE);
						AI::CLEAR_PED_TASKS_IMMEDIATELY(wp, TRUE, TRUE);
						AI::TASK_WANDER_IN_AREA(wp, wph.x, wph.y, wph.z, 40.0f, 3.0f, 6.0f, TRUE);
						ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&wp);
					}
					g_wolfPack[i] = 0;
				}
				g_wolfPackSize = 0; g_howlsRemaining = 0; g_nextHowlAtMs = 0; g_lastAttackSeenMs = 0; g_packCooldownUntilMs = GetTickCount() + 3000; g_targetPackSize = 0; // reset persistent pack when dismissing
				A11y::speak(L"wolf dismissed", true);
			}
			else
			{
				Ped me = PLAYER::PLAYER_PED_ID(); if (!ENTITY::DOES_ENTITY_EXIST(me)) { A11y::speak(L"no player", true); goto after_wolf_toggle; }
				DWORD model = GAMEPLAY::GET_HASH_KEY("A_C_WOLF");
				if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model)) { A11y::speak(L"wolf model invalid", true); goto after_wolf_toggle; }
				STREAMING::REQUEST_MODEL(model, FALSE);
				while (!STREAMING::HAS_MODEL_LOADED(model)) WAIT(0);
				Vector3 pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(me, -1.0f, -2.2f, 0.0f);
				Ped p = PED::CREATE_PED(model, pos.x, pos.y, pos.z, ENTITY::GET_ENTITY_HEADING(me), 0, 0, 0, 0);
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
				if (!p || !ENTITY::DOES_ENTITY_EXIST(p)) { A11y::speak(L"wolf spawn failed", true); goto after_wolf_toggle; }
				PED::SET_PED_VISIBLE(p, TRUE);
				// Be friendly to player
				Hash myGroup = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(me);
				PED::SET_PED_RELATIONSHIP_GROUP_HASH(p, myGroup);
				PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(p, TRUE);
				PED::SET_PED_COMBAT_ABILITY(p, 2);
				PED::SET_PED_COMBAT_MOVEMENT(p, 2);
				PED::SET_PED_FLEE_ATTRIBUTES(p, 0, FALSE);
				// Follow player
				AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(p, me, -0.5f, -1.8f, 0.0f, 2.2f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
				g_wolf = p; g_wolfLastTaskMs = GetTickCount(); g_wolfRespawnAtMs = 0; g_targetPackSize = 1; // default to one helper when (re)spawning
				A11y::speak(L"wolf ready", true);
			}
			after_wolf_toggle:;
		}

		// NumPad - : decrease persistent pack size by one and dismiss one extra helper immediately
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_SUBTRACT))
		{
			if (g_targetPackSize > 0) {
				g_targetPackSize -= 1; if (g_targetPackSize < 0) g_targetPackSize = 0;
				// If we now have more helpers than target, dismiss one immediately
				if (g_wolfPackSize > g_targetPackSize) {
					int idx = g_wolfPackSize - 1;
					Ped wp = g_wolfPack[idx];
					if (wp && ENTITY::DOES_ENTITY_EXIST(wp)) {
						Vector3 wph = ENTITY::GET_ENTITY_COORDS(wp, TRUE, FALSE);
						AI::CLEAR_PED_TASKS_IMMEDIATELY(wp, TRUE, TRUE);
						AI::TASK_WANDER_IN_AREA(wp, wph.x, wph.y, wph.z, 50.0f, 3.0f, 6.0f, TRUE);
						ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&wp);
					}
					g_wolfPack[idx] = 0; g_wolfPackSize -= 1;
				}
				wchar_t buf[64]; swprintf_s(buf, L"pack %d", g_targetPackSize);
				A11y::speak(buf, true);
			} else {
				A11y::speak(L"pack 0", true);
			}
		}

		// NumPad 0: command attack on aimed target (wolf first, then human)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD0))
		{
			Entity t = 0; Player pl = PLAYER::PLAYER_ID();
			if (!PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(pl, &t) || !ENTITY::DOES_ENTITY_EXIST(t))
			{
				// fallback to melee target
				Ped me = PLAYER::PLAYER_PED_ID(); Ped mt = PED::GET_MELEE_TARGET_FOR_PED(me); if (mt && ENTITY::DOES_ENTITY_EXIST(mt)) t = mt;
				// fallback to last auto-announced entity under aim
				if ((!t || !ENTITY::DOES_ENTITY_EXIST(t)) && lastAimedEntity && ENTITY::DOES_ENTITY_EXIST(lastAimedEntity)) t = lastAimedEntity;
			}
			if (t && ENTITY::DOES_ENTITY_EXIST(t) && ENTITY::IS_ENTITY_A_PED(t))
			{
				bool wolfReady = (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf));
				bool guardReady = (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard) && !ENTITY::IS_ENTITY_DEAD(g_bodyguard));
				if (wolfReady)
				{
					// Ensure immediate, aggressive pursuit regardless of relation
					PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(g_wolf, TRUE);
					PED::SET_PED_COMBAT_ABILITY(g_wolf, 2);
					PED::SET_PED_COMBAT_MOVEMENT(g_wolf, 2);
					PED::SET_PED_FLEE_ATTRIBUTES(g_wolf, 0, FALSE);
					AI::TASK_COMBAT_PED(g_wolf, (Ped)t, 0, 0);
					g_pendingAttackTarget = t; // after wolf falls, human will engage
					A11y::speak(L"wolf attacking", true);
				}
				else if (guardReady)
				{
					// brief threat then engage
					AI::TASK_GOTO_ENTITY_AIMING(g_bodyguard, t, 18.0f, 25.0f);
					g_humanThreatUntilMs = GetTickCount() + 1500;
					g_pendingAttackTarget = t;
					A11y::speak(L"bodyguard attacking", true);
				}
				else
				{
					A11y::speak(L"no companion", true);
				}
			}
			else
			{
				A11y::speak(L"no target", true);
			}
		}

		// NumPad 7/Home: on-demand honor + bounty/wanted status (Home when NumLock is off)
		if (!menuController->HasActiveMenu() && (IsKeyJustUp(VK_NUMPAD7) || IsKeyJustUp(VK_HOME)))
		{
			static int lastHonor = INT_MIN; static DWORD lastHonorChangeMs = 0;
			int honor = 0; bool okHonor = TryReadHonor(honor);

			// Read bounty (head price) and wanted intensity
			Player pl = PLAYER::PLAYER_ID();
			int headPriceCents = PURSUIT::GET_PLAYER_PRICE_ON_A_HEAD(pl);
			int wiRaw = PURSUIT::GET_PLAYER_WANTED_INTENSITY(pl);
			int wantedPct = wiRaw / 100; if (wantedPct < 0) wantedPct = 0; if (wantedPct > 100) wantedPct = 100;
			int bountyD = headPriceCents / 100; int bountyC = abs(headPriceCents % 100);

			// Compose message
			wchar_t buf[192];
			if (!okHonor) {
				swprintf_s(buf, L"Honor: unavailable, bounty %d.%02d$, %s%d%%",
					bountyD, bountyC, (wantedPct>0?L"wanted ":L"wanted "), wantedPct);
			} else {
				const wchar_t* trend = L"";
				if (lastHonor != INT_MIN) {
					if (honor > lastHonor) trend = L" (increasing)";
					else if (honor < lastHonor) trend = L" (decreasing)";
				}
				swprintf_s(buf, L"Honor: %d%s, bounty %d.%02d$, %s%d%%",
					honor, trend, bountyD, bountyC, (wantedPct>0?L"wanted ":L"wanted "), wantedPct);
				if (honor != lastHonor) { lastHonor = honor; lastHonorChangeMs = GetTickCount(); }
			}
			A11y::speak(buf, true);
		}

		// NumPad *: make wolf howl and call in more pack members up to a cap
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_MULTIPLY))
		{
			if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf))
			{
				// Each press increases desired pack by 2 up to MAX_WOLF_PACK
				int before = g_targetPackSize;
				g_targetPackSize += 2; if (g_targetPackSize > MAX_WOLF_PACK) g_targetPackSize = MAX_WOLF_PACK;
				int need = g_targetPackSize - g_wolfPackSize; if (need < 0) need = 0;
				if (need > 0)
				{
					DWORD now = GetTickCount();
					// schedule a burst of howls to start summoning
					g_howlsRemaining = (need * 2 > 6 ? 6 : need * 2);
					g_nextHowlAtMs = now; g_packCooldownUntilMs = now + 1000; // short cooldown for manual
					A11y::speak(L"wolf howls for the pack", true);
					// Try to play an actual howl on the main wolf for feedback
					TryWolfHowl(g_wolf);
				}
				else
				{
					A11y::speak(L"pack ready", true);
				}
			}
			else
			{
				A11y::speak(L"no wolf", true);
			}
		}

		// NumPad . (Decimal): Player calls first, then force all wolves to howl (audible)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_DECIMAL))
		{
			Ped me = PLAYER::PLAYER_PED_ID();
			bool didCall = TryPlayerCall(me);
			TryFrontendBeep(); // audible cue for the call
			g_forcedHowlAll = true; g_forcedHowlAtMs = GetTickCount() + 500; // wolves answer after half a second
			// Also kick a short howl cadence to help gather or replenish, but don't exceed target cap
			int maxAllowed = (g_targetPackSize > 0 ? g_targetPackSize : MAX_WOLF_PACK);
			if (g_wolfPackSize < maxAllowed) { g_howlsRemaining = (g_howlsRemaining < 2 ? 2 : g_howlsRemaining); g_nextHowlAtMs = GetTickCount(); }
			A11y::speak(didCall ? L"calling" : L"call", true);
			// Rally the wolf and pack to player for a short time
			if (ENTITY::DOES_ENTITY_EXIST(me)) {
				if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf)) {
					AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_wolf, me, -0.5f, -1.6f, 0.0f, 3.2f, 2000, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
					g_wolfLastTaskMs = GetTickCount();
				}
				for (int i = 0; i < g_wolfPackSize; ++i) {
					Ped wp = g_wolfPack[i]; if (!wp || ENTITY::IS_ENTITY_DEAD(wp)) continue;
					AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(wp, me, -0.6f, -2.0f, 0.0f, 3.2f, 2000, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
				}
			}
		}

		// Auto-speak the entity you're aiming/targeting at (works with and without weapon)
		if (!menuController->HasActiveMenu())
		{
			Player player = PLAYER::PLAYER_ID();
			// Skip auto features during loading/early startup to avoid invalid data
			if (GetTickCount() - startupMs < 4000) { menuController->Update(); WAIT(0); continue; }
			if (!PLAYER::IS_PLAYER_CONTROL_ON(player) || !CAM::IS_SCREEN_FADED_IN() || DLC2::GET_IS_LOADING_SCREEN_ACTIVE()) { menuController->Update(); WAIT(0); continue; }

			// Shared: remember most recent targeted ped and its name/time for money attribution
			static Entity lastTargetPedForMoney = 0; static DWORD lastTargetPedForMoneyMs = 0; static wchar_t lastTargetPedForMoneyName[128];

			// Auto-announce money changes (e.g., after robbing someone) with stability debounce
			{
				static int lastMoneyCents = -1;
				static int moneyCandidate = -1; static DWORD moneyCandidateSinceMs = 0; static DWORD lastMoneyAnnounceMs = 0; static int lastDeltaCents = 0;
				Ped me = PLAYER::PLAYER_PED_ID();
				if (ENTITY::DOES_ENTITY_EXIST(me)) {
					int cur = ReadPlayerMoneyCents(me);
					DWORD now = GetTickCount();
					if (cur != moneyCandidate) { moneyCandidate = cur; moneyCandidateSinceMs = now; }
					bool stable = (now - moneyCandidateSinceMs >= 800);
					if (stable && cur != lastMoneyCents && (now - lastMoneyAnnounceMs > 500)) {
						int delta = (lastMoneyCents >= 0) ? (cur - lastMoneyCents) : 0;
						lastDeltaCents = delta;
						// Format as dollars.cents
						int absDelta = delta < 0 ? -delta : delta;
						int d_dollars = absDelta / 100; int d_cents = absDelta % 100;
						int t_dollars = (cur >= 0 ? cur : 0) / 100; int t_cents = (cur >= 0 ? cur : 0) % 100;
						wchar_t buf[200];
						if (delta > 0) {
							// Try attribute to most recent targeted ped within 3s and within 25m
							bool attributed = false; wchar_t src[128]; src[0] = L'\0';
							if (lastTargetPedForMoney && ENTITY::DOES_ENTITY_EXIST(lastTargetPedForMoney)) {
								DWORD ago = now - lastTargetPedForMoneyMs;
								if (ago <= 3000) {
									Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
									Vector3 sC = ENTITY::GET_ENTITY_COORDS(lastTargetPedForMoney, TRUE, FALSE);
									float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, sC.x, sC.y, sC.z, TRUE);
									if (dist <= 25.0f) { wcsncpy_s(src, lastTargetPedForMoneyName, _TRUNCATE); attributed = true; }
								}
							}
							if (attributed && src[0]) swprintf_s(buf, L"%s gave you %d.%02d$ — You have %d.%02d$", src, d_dollars, d_cents, t_dollars, t_cents);
							else swprintf_s(buf, L"You received %d.%02d$ — You have %d.%02d$", d_dollars, d_cents, t_dollars, t_cents);
						}
						else if (delta < 0) swprintf_s(buf, L"You spent %d.%02d$", d_dollars, d_cents);
						else swprintf_s(buf, L"You have %d.%02d$", t_dollars, t_cents);
						A11y::speak(buf, true);
						lastMoneyCents = cur; lastMoneyAnnounceMs = now;
						g_lastKnownMoneyCents = cur;
					}
					else if (lastMoneyCents < 0) {
						// Initialize baseline silently once stable
						if (stable) { lastMoneyCents = cur; g_lastKnownMoneyCents = cur; }
					}
				}
			}
			// Maintain bodyguard behavior (follow/defend) and wolf behavior + respawns
			if (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard))
			{
				Ped me = PLAYER::PLAYER_PED_ID();
				if (!ENTITY::IS_ENTITY_DEAD(g_bodyguard))
				{
					// Keep near player
					Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
					Vector3 bgC = ENTITY::GET_ENTITY_COORDS(g_bodyguard, TRUE, FALSE);
					float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, bgC.x, bgC.y, bgC.z, TRUE);
					DWORD now = GetTickCount();
					if (dist > 8.0f && now - g_bodyguardLastTaskMs > 800)
					{
						AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_bodyguard, me, 0.0f, -1.6f, 0.0f, 2.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						g_bodyguardLastTaskMs = now;
					}
					// Defend against threats around player
					if (!PED::IS_PED_IN_COMBAT(g_bodyguard, 0))
					{
						AI::TASK_COMBAT_HATED_TARGETS_AROUND_PED(g_bodyguard, 32.0f, 0, 0);
					}
				}
				else { g_bodyguard = 0; g_guardRespawnAtMs = GetTickCount() + 2000; }
			}
			// Wolf maintenance
			if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf))
			{
				Ped me = PLAYER::PLAYER_PED_ID();
				if (!ENTITY::IS_ENTITY_DEAD(g_wolf))
				{
					Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
					Vector3 wC = ENTITY::GET_ENTITY_COORDS(g_wolf, TRUE, FALSE);
					float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, wC.x, wC.y, wC.z, TRUE);
					DWORD now = GetTickCount();
					if (!PED::IS_PED_IN_COMBAT(g_wolf, 0) && dist > 8.0f && now - g_wolfLastTaskMs > 800)
					{
						AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_wolf, me, -0.5f, -1.8f, 0.0f, 2.6f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						g_wolfLastTaskMs = now;
					}
					// If drifted too far and not in combat, recall more urgently
					if (!PED::IS_PED_IN_COMBAT(g_wolf, 0) && dist > 35.0f && now - g_wolfLastTaskMs > 400)
					{
						AI::CLEAR_PED_TASKS(g_wolf, TRUE, TRUE);
						AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_wolf, me, -0.6f, -2.0f, 0.0f, 3.2f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						g_wolfLastTaskMs = now;
					}
					// No auto-combat for main wolf; only attacks on command
				}
				else { g_wolf = 0; g_wolfRespawnAtMs = GetTickCount() + 2000; }
			}

			// Wolf pack maintenance: prune dead, keep close, and top-up to target size
			{
				// Compact the pack array and remove dead/missing helpers
				int write = 0;
				for (int i = 0; i < g_wolfPackSize; ++i) {
					Ped wp = g_wolfPack[i];
					if (!wp || !ENTITY::DOES_ENTITY_EXIST(wp) || ENTITY::IS_ENTITY_DEAD(wp)) {
						continue; // drop
					}
					g_wolfPack[write++] = wp;
				}
				if (write != g_wolfPackSize) {
					g_wolfPackSize = write;
				}
				// If below target, trigger howls to replenish silently (keep trying until filled)
				if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf)) {
					int deficit = g_targetPackSize - g_wolfPackSize;
					if (deficit > 0 && g_howlsRemaining == 0) { g_howlsRemaining = (deficit > 2 ? 2 : deficit); g_nextHowlAtMs = GetTickCount(); }
				}
				// Keep helpers close if idle
				if (g_wolfPackSize > 0) {
					Ped me = PLAYER::PLAYER_PED_ID(); Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
					for (int i = 0; i < g_wolfPackSize; ++i) {
						Ped wp = g_wolfPack[i]; if (!wp || ENTITY::IS_ENTITY_DEAD(wp)) continue;
						Vector3 wc = ENTITY::GET_ENTITY_COORDS(wp, TRUE, FALSE);
						float d = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, wc.x, wc.y, wc.z, TRUE);
						if (!PED::IS_PED_IN_COMBAT(wp, 0) && d > 14.0f) {
							AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(wp, me, -0.8f, -2.4f, 0.0f, 3.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						}
					}
				}
			}

			// Shooter/melee/thief detection and wolf-pack assistance
			{
				Ped me = PLAYER::PLAYER_PED_ID(); if (ENTITY::DOES_ENTITY_EXIST(me)) {
					DWORD now = GetTickCount();
					int packed[33] = { 32 };
					int count = PED::GET_PED_NEARBY_PEDS(me, packed, -1, 0);
					bool shooterFound = false; Ped shooters[16]; int shooterN = 0;
					bool meleeDanger = PED::IS_PED_IN_MELEE_COMBAT(me) ? true : false;
					bool thiefThreat = false;
					Ped suspects[24]; int suspectN = 0;
					if (count > 0) {
						int lim = packed[0]; if (lim > 32) lim = 32;
						Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
						for (int i = 1; i <= lim; ++i) {
							Ped p = (Ped)packed[i]; if (!p || p == me) continue;
							if (!ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p) || PED::IS_PED_A_PLAYER(p)) continue;
							Vector3 pc = ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE);
							float d = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, pc.x, pc.y, pc.z, TRUE);
							if (d > 70.0f) continue;
							if (PED::IS_PED_SHOOTING(p)) { shooterFound = true; if (shooterN < 16) shooters[shooterN++] = p; if (suspectN < 24) suspects[suspectN++] = p; }
							if (d < 3.2f && (PED::IS_PED_IN_MELEE_COMBAT(p) || meleeDanger)) { if (suspectN < 24) suspects[suspectN++] = p; }
								// Treat nearby fast-moving/fleeing peds as potential thief threats
								if (d < 4.5f && (AI::IS_MOVE_BLEND_RATIO_SPRINTING(p) || AI::IS_PED_RUNNING(p) || PED::IS_PED_FLEEING(p)) && !PED::IS_PED_IN_COMBAT(p, me)) {
									thiefThreat = true; if (suspectN < 24) suspects[suspectN++] = p;
								}
						}
					}
					if (shooterFound || meleeDanger || thiefThreat) {
						g_lastAttackSeenMs = now;
						if (now >= g_packCooldownUntilMs && g_howlsRemaining == 0 && g_wolf) {
							g_howlsRemaining = 4; g_nextHowlAtMs = now; g_packCooldownUntilMs = now + 12000; A11y::speak(L"wolf howls", false);
							// Safe cue (no audio native)
							TryWolfHowl(g_wolf);
						}
			// If a forced howl after player's call is scheduled, play it now
			{
				DWORD now = GetTickCount();
				if (g_forcedHowlAll && now >= g_forcedHowlAtMs) {
					int howled = 0;
					TryFrontendBeep(); // audible cue for wolves answering
					if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf)) {
						if (TryWolfHowl(g_wolf)) howled++;
					}
					for (int i = 0; i < g_wolfPackSize; ++i) {
						Ped wp = g_wolfPack[i];
						if (wp && ENTITY::DOES_ENTITY_EXIST(wp) && !ENTITY::IS_ENTITY_DEAD(wp)) {
							if (TryWolfHowl(wp)) howled++;
						}
					}
					g_forcedHowlAll = false; g_forcedHowlAtMs = 0;
				}
			}
						for (int i = 0; i < g_wolfPackSize; ++i) {
							Ped wp = g_wolfPack[i]; if (!wp || !ENTITY::DOES_ENTITY_EXIST(wp) || ENTITY::IS_ENTITY_DEAD(wp)) continue;
							if (!PED::IS_PED_IN_COMBAT(wp, 0)) {
								Ped tgt = 0;
								if (suspectN > 0) tgt = suspects[i % suspectN];
								else if (shooterN > 0) tgt = shooters[i % shooterN];
								if (tgt) { PED::SET_PED_COMBAT_ABILITY(wp, 2); PED::SET_PED_COMBAT_MOVEMENT(wp, 2); PED::SET_PED_FLEE_ATTRIBUTES(wp, 0, FALSE); PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(wp, TRUE); AI::TASK_COMBAT_PED(wp, tgt, 0, 0); }
							}
						}
					} else {
						// Only auto-dismiss if user didn't request a persistent pack
						if (g_wolfPackSize > 0 && g_targetPackSize == 0 && (now - g_lastAttackSeenMs) > 9000) {
							for (int i = 0; i < g_wolfPackSize; ++i) {
								Ped wp = g_wolfPack[i];
								if (wp && ENTITY::DOES_ENTITY_EXIST(wp)) {
									Vector3 wph = ENTITY::GET_ENTITY_COORDS(wp, TRUE, FALSE);
									AI::CLEAR_PED_TASKS_IMMEDIATELY(wp, TRUE, TRUE);
									AI::TASK_WANDER_IN_AREA(wp, wph.x, wph.y, wph.z, 50.0f, 3.0f, 6.0f, TRUE);
									ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&wp);
								}
								g_wolfPack[i] = 0;
							}
							g_wolfPackSize = 0; g_howlsRemaining = 0; g_nextHowlAtMs = 0;
						}
					}
				}
			}

			// Stage howls and spawn helper wolves; retry if a spawn fails instead of silently consuming howls
			if (g_howlsRemaining > 0 && GetTickCount() >= g_nextHowlAtMs) {
				Ped me = PLAYER::PLAYER_PED_ID();
				int maxAllowed = (g_targetPackSize > 0 ? g_targetPackSize : MAX_WOLF_PACK);
				bool spawned = false;
				if (ENTITY::DOES_ENTITY_EXIST(me) && g_wolfPackSize < maxAllowed) {
					DWORD model = GAMEPLAY::GET_HASH_KEY("A_C_WOLF");
					if (STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_VALID(model)) {
						STREAMING::REQUEST_MODEL(model, FALSE); while (!STREAMING::HAS_MODEL_LOADED(model)) WAIT(0);
						Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
						float ang = (float)(rand() % 360) * 3.14159265f / 180.0f;
						float r = 14.0f + (float)(rand() % 60) / 10.0f;
						Vector3 pos = { meC.x + cosf(ang) * r, meC.y + sinf(ang) * r, meC.z };
						Ped wp = PED::CREATE_PED(model, pos.x, pos.y, pos.z, 0.0f, 0, 0, 0, 0);
						STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
						if (wp && ENTITY::DOES_ENTITY_EXIST(wp)) {
							PED::SET_PED_VISIBLE(wp, TRUE);
							Hash myGroup = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(me);
							PED::SET_PED_RELATIONSHIP_GROUP_HASH(wp, myGroup);
							PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(wp, TRUE);
							PED::SET_PED_COMBAT_ABILITY(wp, 2);
							PED::SET_PED_COMBAT_MOVEMENT(wp, 2);
							PED::SET_PED_FLEE_ATTRIBUTES(wp, 0, FALSE);
							AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(wp, me, -0.5f, -2.2f, 0.0f, 3.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
							g_wolfPack[g_wolfPackSize++] = wp;
							spawned = true;
						}
					}
				}
				if (spawned || g_wolfPackSize >= maxAllowed) {
					g_howlsRemaining--; g_nextHowlAtMs = GetTickCount() + 900;
				} else {
					// Retry soon without consuming a howl if we couldn't spawn
					g_nextHowlAtMs = GetTickCount() + 600;
				}
			}

			// Pending human engage after threat window or after wolf death
			if (g_pendingAttackTarget && ENTITY::DOES_ENTITY_EXIST(g_pendingAttackTarget))
			{
				DWORD now = GetTickCount();
				bool wolfGone = (!g_wolf || !ENTITY::DOES_ENTITY_EXIST(g_wolf) || ENTITY::IS_ENTITY_DEAD(g_wolf));
				bool guardReady = (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard) && !ENTITY::IS_ENTITY_DEAD(g_bodyguard));
				if (guardReady && (wolfGone || (g_humanThreatUntilMs && now > g_humanThreatUntilMs)))
				{
					AI::TASK_COMBAT_PED(g_bodyguard, (Ped)g_pendingAttackTarget, 0, 0);
					AI::TASK_COMBAT_HATED_TARGETS_AROUND_PED(g_bodyguard, 28.0f, 0, 0);
					g_pendingAttackTarget = 0; g_humanThreatUntilMs = 0;
				}
			}

			// Respawn companions from distance and make them run to you
			{
				DWORD now = GetTickCount(); Ped me = PLAYER::PLAYER_PED_ID(); if (ENTITY::DOES_ENTITY_EXIST(me))
				{
					if (!g_bodyguard && g_guardRespawnAtMs && now >= g_guardRespawnAtMs)
					{
						g_guardRespawnAtMs = 0;
						DWORD model = GAMEPLAY::GET_HASH_KEY("S_M_M_CORNWALLGUARD_01");
						if (STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_VALID(model))
						{
							STREAMING::REQUEST_MODEL(model, FALSE); while (!STREAMING::HAS_MODEL_LOADED(model)) WAIT(0);
							Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
							// spawn ~80m away behind player heading
							float heading = ENTITY::GET_ENTITY_HEADING(me);
							float rad = heading * 3.14159265f / 180.0f;
							Vector3 pos = { meC.x - cosf(rad) * 80.0f, meC.y - sinf(rad) * 80.0f, meC.z };
							Ped p = PED::CREATE_PED(model, pos.x, pos.y, pos.z, heading, 0, 0, 0, 0);
							STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
							if (p && ENTITY::DOES_ENTITY_EXIST(p))
							{
								PED::SET_PED_VISIBLE(p, TRUE);
								Hash myGroup = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(me);
								PED::SET_PED_RELATIONSHIP_GROUP_HASH(p, myGroup);
								int grp = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()); if (grp) PED::SET_PED_AS_GROUP_MEMBER(p, grp);
								ENTITY::SET_ENTITY_INVINCIBLE(p, TRUE);
								PED::SET_PED_ACCURACY(p, 70);
								PED::SET_PED_COMBAT_ABILITY(p, 2);
								PED::SET_PED_COMBAT_MOVEMENT(p, 2);
								Hash w = GAMEPLAY::GET_HASH_KEY("WEAPON_REPEATER_CARBINE");
								WEAPON::GIVE_DELAYED_WEAPON_TO_PED(p, w, 120, 1, 0x2cd419dc);
								WEAPON::SET_CURRENT_PED_WEAPON(p, w, 1, 0, 0, 0);
								AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(p, me, 0.0f, -1.6f, 0.0f, 3.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
								g_bodyguard = p; g_bodyguardLastTaskMs = now;
							}
						}
					}
					if (!g_wolf && g_wolfRespawnAtMs && now >= g_wolfRespawnAtMs)
					{
						g_wolfRespawnAtMs = 0;
						DWORD model = GAMEPLAY::GET_HASH_KEY("A_C_WOLF");
						if (STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_VALID(model))
						{
							STREAMING::REQUEST_MODEL(model, FALSE); while (!STREAMING::HAS_MODEL_LOADED(model)) WAIT(0);
							Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
							float heading = ENTITY::GET_ENTITY_HEADING(me);
							float rad = heading * 3.14159265f / 180.0f;
							Vector3 pos = { meC.x - cosf(rad) * 85.0f, meC.y - sinf(rad) * 85.0f, meC.z };
							Ped p = PED::CREATE_PED(model, pos.x, pos.y, pos.z, heading, 0, 0, 0, 0);
							STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
							if (p && ENTITY::DOES_ENTITY_EXIST(p))
							{
								PED::SET_PED_VISIBLE(p, TRUE);
								Hash myGroup = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(me);
								PED::SET_PED_RELATIONSHIP_GROUP_HASH(p, myGroup);
								PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(p, TRUE);
								PED::SET_PED_COMBAT_ABILITY(p, 2);
								PED::SET_PED_COMBAT_MOVEMENT(p, 2);
								PED::SET_PED_FLEE_ATTRIBUTES(p, 0, FALSE);
								AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(p, me, -0.5f, -1.8f, 0.0f, 3.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
								g_wolf = p; g_wolfLastTaskMs = now;
							}
						}
					}
				}
			}

			// Auto-announce on weapon change (name only, with stability debounce)
			{
				Ped ped = PLAYER::PLAYER_PED_ID(); if (!ENTITY::DOES_ENTITY_EXIST(ped)) { /* no ped */ }
				Hash cur = 0; BOOL got = WEAPON::GET_CURRENT_PED_WEAPON(ped, &cur, 0, 0, 0);
				DWORD now = GetTickCount();
				if (cur != weaponCandidateHash) { weaponCandidateHash = cur; weaponCandidateSinceMs = now; }
				// wait 1 second to let HUD/ammo settle
				bool stable = (now - weaponCandidateSinceMs >= 1000);
				if (stable && got && cur != lastWeaponHash && (now - lastWeaponAnnounceMs > 1000))
				{
					Hash unarmed = GAMEPLAY::GET_HASH_KEY("WEAPON_UNARMED");
					if (cur && WEAPON::IS_WEAPON_VALID(cur) && cur != unarmed && WEAPON::HAS_PED_GOT_WEAPON(ped, cur, 0, 0))
					{
						std::string uname;
						for each (auto info in weaponInfos)
						{
							Hash wh = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(("WEAPON_" + info.name).c_str()));
							if (wh == cur) { uname = info.uiname; break; }
						}
						wchar_t wname[96]; wname[0] = L'\0';
						if (!uname.empty()) { size_t n = uname.size(); if (n > 95) n = 95; mbstowcs(wname, uname.c_str(), n); wname[n] = L'\0'; }
						if (wname[0]) A11y::speak(wname, true);
						else A11y::speak(L"weapon", true);
					}
					else
					{
						A11y::speak(L"unarmed", true);
					}
					lastWeaponHash = cur;
					lastWeaponAnnounceMs = now;
				}
			}
			// Preferred target selection (aimed entity, melee target, camera ray, headtracked ped, closest ped)
			auto selectPreferredTarget = [&]() -> Entity {
				Player pl = PLAYER::PLAYER_ID();
				Ped mePed = PLAYER::PLAYER_PED_ID();
				Ped mount = 0; if (PED::IS_PED_ON_MOUNT(mePed)) { mount = PED::GET_MOUNT(mePed); }
				Entity t = 0;
				if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(pl, &t) && ENTITY::DOES_ENTITY_EXIST(t)) return t;
				// Melee target
				if (PED::IS_PED_IN_MELEE_COMBAT(mePed)) {
					Ped m = PED::GET_MELEE_TARGET_FOR_PED(mePed);
					if (m && ENTITY::DOES_ENTITY_EXIST(m) && !ENTITY::IS_ENTITY_DEAD(m) && m != mePed && m != mount) return m;
				}
				// Camera raycast fallback aligned with where you look, distance scales with zoom/weapon; use a capsule for wider coverage
				{
					Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
					Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
					float radX = camRot.x * static_cast<float>(3.14159265358979323846 / 180.0);
					float radZ = camRot.z * static_cast<float>(3.14159265358979323846 / 180.0);
					Vector3 dir = { -sinf(radZ) * cosf(radX), cosf(radZ) * cosf(radX), sinf(radX) };
					float fov = CAM::GET_GAMEPLAY_CAM_FOV();
					float zoomFactor = (fov > 0.0f) ? (70.0f / fov) : 1.0f; if (zoomFactor < 0.6f) zoomFactor = 0.6f; if (zoomFactor > 2.5f) zoomFactor = 2.5f;
					float baseDist = 45.0f * zoomFactor;
					// Extend for rifles/snipers a bit more, shorten for melee/throwable
					float dist = baseDist;
					Hash curW = 0; if (WEAPON::GET_CURRENT_PED_WEAPON(mePed, &curW, 0, 0, 0) && WEAPON::IS_WEAPON_VALID(curW)) {
						// crude categories by name
						char wname[64]; wname[0] = '\0';
						for each (auto info in weaponInfos) { Hash wh = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(("WEAPON_" + info.name).c_str())); if (wh == curW) { strncpy_s(wname, info.name.c_str(), _TRUNCATE); break; } }
						std::string n = wname; for (auto &c : n) c = (char)tolower(c);
						if (n.find("sniper") != std::string::npos || n.find("rollingblock") != std::string::npos || n.find("carbine") != std::string::npos || n.find("repeater") != std::string::npos || n.find("rifle") != std::string::npos) dist = baseDist * 1.6f;
						else if (n.find("bow") != std::string::npos || n.find("lasso") != std::string::npos || n.find("thrown") != std::string::npos) dist = baseDist * 0.8f;
					}
					Vector3 to = { camPos.x + dir.x * dist, camPos.y + dir.y * dist, camPos.z + dir.z * dist };
					Entity ignore = mount ? mount : (Entity)mePed; // ignore mount when on horseback, else self
					if (camRayHandle == 0) {
						float radius = 0.25f * (zoomFactor > 1.0f ? (2.0f/zoomFactor) : 1.0f); if (radius < 0.15f) radius = 0.15f; if (radius > 0.4f) radius = 0.4f;
						camRayHandle = SHAPETEST::START_SHAPE_TEST_CAPSULE(camPos.x, camPos.y, camPos.z, to.x, to.y, to.z, radius, 511, ignore, 7);
						camRayLastIgnore = ignore;
					}
					BOOL hit = FALSE; Entity hitEnt = 0; int state = SHAPETEST::GET_SHAPE_TEST_RESULT(camRayHandle, &hit, &camRayEnd, &camRayNorm, &hitEnt);
					if (state) {
						camRayHandle = 0; // completed
						if (hit && ENTITY::DOES_ENTITY_EXIST(hitEnt) && hitEnt != ignore && hitEnt != mePed && hitEnt != mount) return hitEnt;
					}
				}
				// Headtracking ped among nearby (in front of camera)
				{
					int packed[33] = { 32 };
					int count = PED::GET_PED_NEARBY_PEDS(mePed, packed, -1, 0);
					if (count > 0) {
						int lim = packed[0]; if (lim > 32) lim = 32;
						Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
						Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
						float radX = camRot.x * static_cast<float>(3.14159265358979323846 / 180.0);
						float radZ = camRot.z * static_cast<float>(3.14159265358979323846 / 180.0);
						Vector3 dir = { -sinf(radZ) * cosf(radX), cosf(radZ) * cosf(radX), sinf(radX) };
						for (int i = 1; i <= lim; ++i) {
							Ped p = (Ped)packed[i];
							if (!p || p == mePed || p == mount) continue;
							if (!ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p) || PED::IS_PED_A_PLAYER(p)) continue;
							Vector3 pc = ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE);
							Vector3 v = { pc.x - camPos.x, pc.y - camPos.y, pc.z - camPos.z };
							float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); if (len <= 0.001f) continue;
							float dot = (v.x*dir.x + v.y*dir.y + v.z*dir.z) / len; // dir is unit-ish
							if (dot > 0.2f && PED::IS_PED_HEADTRACKING_PED(mePed, p)) return p;
						}
					}
				}
				// Closest ped within a small radius (exclude mount; prefer in front)
				{
					Vector3 me = ENTITY::GET_ENTITY_COORDS(mePed, TRUE, FALSE);
					Ped out = 0;
					float radius = PED::IS_PED_ON_MOUNT(mePed) ? 1.9f : 2.6f;
					if (PED::GET_CLOSEST_PED(me.x, me.y, me.z, radius, TRUE, TRUE, &out, FALSE, FALSE, 0, 0)) {
						if (out && ENTITY::DOES_ENTITY_EXIST(out) && !ENTITY::IS_ENTITY_DEAD(out) && out != mePed && out != mount) {
							// ensure roughly in front of camera
							Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
							Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
							float radX = camRot.x * static_cast<float>(3.14159265358979323846 / 180.0);
							float radZ = camRot.z * static_cast<float>(3.14159265358979323846 / 180.0);
							Vector3 dir = { -sinf(radZ) * cosf(radX), cosf(radZ) * cosf(radX), sinf(radX) };
							Vector3 pc = ENTITY::GET_ENTITY_COORDS(out, TRUE, FALSE);
							Vector3 v = { pc.x - camPos.x, pc.y - camPos.y, pc.z - camPos.z };
							float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
							float dot = (len > 0.001f) ? ((v.x*dir.x + v.y*dir.y + v.z*dir.z) / len) : 0.0f;
							if (dot > 0.1f) return out;
						}
					}
				}
				return 0;
			};

			// Speak about the selected target when relevant
			{
				bool isAimingOrTargeting = PLAYER::IS_PLAYER_FREE_AIMING(player) || PLAYER::IS_PLAYER_TARGETTING_ANYTHING(player) || PED::IS_PED_IN_MELEE_COMBAT(PLAYER::PLAYER_PED_ID());
				Entity target = isAimingOrTargeting ? selectPreferredTarget() : 0;
				if (target && ENTITY::DOES_ENTITY_EXIST(target))
				{
					// Distance and health
					Vector3 me = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
					Vector3 tc = ENTITY::GET_ENTITY_COORDS(target, TRUE, FALSE);
					int distM = static_cast<int>(GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(me.x, me.y, me.z, tc.x, tc.y, tc.z, TRUE) + 0.5f);
					int hp = ENTITY::IS_ENTITY_DEAD(target) ? 0 : ENTITY::GET_ENTITY_HEALTH(target);
					int maxHp = 0;
					if (ENTITY::IS_ENTITY_A_PED(target)) maxHp = PED::GET_PED_MAX_HEALTH(target);
					else maxHp = ENTITY::GET_ENTITY_MAX_HEALTH(target, FALSE);
					if (maxHp <= 0) maxHp = 1;
					if (hp > maxHp) hp = maxHp;
					int hpPct = static_cast<int>((hp * 100.0f) / static_cast<float>(maxHp) + 0.5f);
					if (hpPct < 0) hpPct = 0; if (hpPct > 100) hpPct = 100;
					bool isNew = (target != lastAimedEntity);
					bool changed = (distM != lastAimedDistM) || (hpPct != lastAimedHealthPct);
					if (isNew || changed)
					{
						std::string name;
						if (ENTITY::IS_ENTITY_A_PED(target))
						{
							Hash model = ENTITY::GET_ENTITY_MODEL(target);
							for each (auto &info in pedModelInfos)
							{
								if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(info.model.c_str())) == model) { name = info.name; break; }
							}
							if (name.empty()) name = "ped";
							// Update money source attribution
							lastTargetPedForMoney = target; lastTargetPedForMoneyMs = GetTickCount();
							wchar_t nbuf[128]; nbuf[0] = L'\0';
							if (!name.empty()) { size_t n = name.size(); if (n > 127) n = 127; mbstowcs(nbuf, name.c_str(), n); nbuf[n] = L'\0'; }
							wcsncpy_s(lastTargetPedForMoneyName, nbuf, _TRUNCATE);
						}
						else if (ENTITY::IS_ENTITY_A_VEHICLE(target))
						{
							Hash model = ENTITY::GET_ENTITY_MODEL(target);
							bool isTrain = VEHICLE::IS_THIS_MODEL_A_TRAIN(model) ? true : false;
							bool isBoat = VEHICLE::IS_THIS_MODEL_A_BOAT(model) ? true : false;
							std::string modelName;
							for each (auto &vm in vehicleModels)
							{
								if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(vm.c_str())) == model) { modelName = vm; break; }
							}
							if (isTrain) name = "train"; else if (isBoat) name = "boat"; else name = "vehicle";
							if (!modelName.empty()) name += ": " + modelName;
						}
						else
						{
							int t = ENTITY::GET_ENTITY_TYPE(target);
							if (t == 3)
							{
									auto classifyObject = [&](Entity e) -> std::string {
										Hash m = ENTITY::GET_ENTITY_MODEL(e);
										Vector3 mn{}, mx{}; GAMEPLAY::GET_MODEL_DIMENSIONS(m, &mn, &mx);
									float sx = fabsf(mx.x - mn.x), sy = fabsf(mx.y - mn.y), sz = fabsf(mx.z - mn.z);
									float th = fminf(fabsf(sx), fabsf(sy));
									if (sz > 3.0f && th < 2.0f) return std::string("tree-like object");
									if (sz > 1.0f && sz < 3.5f && (sx < 0.35f || sy < 0.35f)) return std::string("door-like object");
									if (sz < 1.0f && fmaxf(sx, sy) > 2.0f) return std::string("structure");
									if (sz >= 0.5f && sz <= 1.6f && sx <= 2.0f && sy <= 2.0f) return std::string("crate-like object");
									return std::string("object");
								};
								name = classifyObject(target);
							}
							else
							{
								name = "entity";
							}
						}
						wchar_t wbuf[200];
						wchar_t wname[128]; wname[0] = L'\0';
						if (!name.empty()) { size_t n = name.size(); if (n > 127) n = 127; mbstowcs(wname, name.c_str(), n); wname[n] = L'\0'; swprintf_s(wbuf, L"aiming at %s, distance %d meters, health %d (%d%%)", wname, distM, hp, hpPct); }
						else { swprintf_s(wbuf, L"aiming at target, distance %d meters, health %d (%d%%)", distM, hp, hpPct); }
						if (GetTickCount() - lastAimSpeakMs > 200) { A11y::speak(wbuf, true); lastAimSpeakMs = GetTickCount(); }
						lastAimedEntity = target;
						lastAimedDistM = distM;
						lastAimedHealthPct = hpPct;
					}
				}
				else
				{
					lastAimedEntity = 0; lastAimedDistM = -1; lastAimedHealthPct = -1;
				}
			}
		}
		menuController->Update();
		WAIT(0);
	}
}

void ScriptMain()
{
	srand(GetTickCount());
	main();
}

// Try to read player's honor via known DECORATOR keys; returns true if a value was read
static bool TryReadHonor(int& outHonor)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(me)) return false;
	const char* keys[] = { "honor_current", "honor_level", "honor", "player_honor", "honour", "honour_level" };
	for (auto key : keys) {
		if (DECORATOR::DECOR_EXIST_ON(me, const_cast<char*>(key))) {
			outHonor = DECORATOR::DECOR_GET_INT(me, const_cast<char*>(key));
			return true;
		}
	}
	return false;
}