/*
	THIS FILE IS A PART OF RDR 2 SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2019
*/

#include "script.h"
#include "scriptmenu.h"
#include "keyboard.h"
#include "a11y.h"
#include "controller.h"
#include "debuglog.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>

using namespace std;

#include "scriptinfo.h"

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
	virtual void OnSelect()	{ CASH::PLAYER_ADD_CASH(m_value, 0x2cd419dc); }
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
	float m_heading;
	bool m_isIndoor;

	virtual void OnSelect()
	{
		Entity playerPed = PLAYER::PLAYER_PED_ID();
		bool mounted = PED::IS_PED_ON_MOUNT(playerPed);
		Entity horse = mounted ? PED::GET_MOUNT(playerPed) : 0;
		Entity veh = (!mounted && PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE)) ? PED::GET_VEHICLE_PED_IS_USING(playerPed) : 0;

		// Determine if this is a stable or a normal indoor shop
		// All indoor locations (including stables) will dismount the player and teleport them inside,
		// parking the horse/vehicle outside on the nearest road to prevent falling through or clipping inside.
		bool shouldDismount = m_isIndoor;

		if (shouldDismount && (mounted || veh)) {
			// Dismount player instantly
			AI::CLEAR_PED_TASKS_IMMEDIATELY(playerPed, TRUE, TRUE);
			
			// Clear player velocity
			ENTITY::SET_ENTITY_VELOCITY(playerPed, 0.0f, 0.0f, 0.0f);

			// Teleport player inside
			STREAMING::REQUEST_COLLISION_AT_COORD(m_pos.x, m_pos.y, m_pos.z);
			scriptWait(100); // Allow collision loading
			ENTITY::SET_ENTITY_COORDS(playerPed, m_pos.x, m_pos.y, m_pos.z + 1.0f, 0, 0, 1, FALSE);

			// Apply heading to player ped
			if (m_heading != -1.0f) {
				ENTITY::SET_ENTITY_HEADING(playerPed, m_heading);
				CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0.0f, 1.0f);
			}

			// Find nearest road for the horse/vehicle to prevent spawning in walls
			Entity transport = mounted ? horse : veh;
			if (transport) {
				if (mounted) {
					AI::CLEAR_PED_TASKS_IMMEDIATELY(transport, TRUE, TRUE);
				}
				ENTITY::SET_ENTITY_VELOCITY(transport, 0.0f, 0.0f, 0.0f);
				
				Vector3 roadPos = { 0.0f, 0.0f, 0.0f };
				float roadHeading = 0.0f;
				if (PATHFIND::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(m_pos.x, m_pos.y, m_pos.z, &roadPos, &roadHeading, 1, 3.0f, 0)) {
					STREAMING::REQUEST_COLLISION_AT_COORD(roadPos.x, roadPos.y, roadPos.z);
					ENTITY::SET_ENTITY_COORDS(transport, roadPos.x, roadPos.y, roadPos.z, 0, 0, 1, FALSE);
					ENTITY::SET_ENTITY_HEADING(transport, roadHeading);
				} else if (PATHFIND::GET_CLOSEST_VEHICLE_NODE(m_pos.x, m_pos.y, m_pos.z, &roadPos, 1, 3.0f, 0)) {
					STREAMING::REQUEST_COLLISION_AT_COORD(roadPos.x, roadPos.y, roadPos.z);
					ENTITY::SET_ENTITY_COORDS(transport, roadPos.x, roadPos.y, roadPos.z, 0, 0, 1, FALSE);
					ENTITY::SET_ENTITY_HEADING(transport, m_heading);
				} else {
					// Fallback: offset slightly
					float ox = m_pos.x + 3.0f;
					float oy = m_pos.y + 3.0f;
					STREAMING::REQUEST_COLLISION_AT_COORD(ox, oy, m_pos.z);
					ENTITY::SET_ENTITY_COORDS(transport, ox, oy, m_pos.z, 0, 0, 1, FALSE);
					ENTITY::SET_ENTITY_HEADING(transport, m_heading);
				}
			}
			DebugLog::log("Indoor Teleport: Warped player inside to (%f, %f, %f), transport to road", m_pos.x, m_pos.y, m_pos.z);
		} else {
			// Normal / Outdoor Teleport: Keep mounted / Keep vehicle
			Entity targetEnt = playerPed;
			if (mounted) {
				targetEnt = horse;
			} else if (veh) {
				targetEnt = veh;
			}

			ENTITY::SET_ENTITY_VELOCITY(playerPed, 0.0f, 0.0f, 0.0f);
			if (targetEnt != playerPed) {
				ENTITY::SET_ENTITY_VELOCITY(targetEnt, 0.0f, 0.0f, 0.0f);
			}

			STREAMING::REQUEST_COLLISION_AT_COORD(m_pos.x, m_pos.y, m_pos.z);
			scriptWait(100); // Allow collision loading
			ENTITY::SET_ENTITY_COORDS(targetEnt, m_pos.x, m_pos.y, m_pos.z + 1.0f, 0, 0, 1, FALSE);

			if (m_heading != -1.0f) {
				ENTITY::SET_ENTITY_HEADING(targetEnt, m_heading);
				CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0.0f, 1.0f);
			}
			DebugLog::log("Direct Teleport: Warped entity %d to (%f, %f, %f) heading %f", targetEnt, m_pos.x, m_pos.y, m_pos.z, m_heading);
		}
	}
public:
	MenuItemPlayerTeleport(string caption, Vector3 pos, float heading = -1.0f, bool isIndoor = false)
		: MenuItemDefault(caption), 
		  m_pos(pos),
		  m_heading(heading),
		  m_isIndoor(isIndoor) {}
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
		if (!GetState())
			return;
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		Hash cur;
		if (WEAPON::GET_CURRENT_PED_WEAPON(playerPed, &cur, 0, 0, 0) && WEAPON::IS_WEAPON_VALID(cur))
		{
			int maxAmmo;
			if (WEAPON::GET_MAX_AMMO(playerPed, &maxAmmo, cur))
				WEAPON::SET_PED_AMMO(playerPed, cur, maxAmmo);
			maxAmmo = WEAPON::GET_MAX_AMMO_IN_CLIP(playerPed, cur, 1);
			if (maxAmmo > 0)
				WEAPON::SET_AMMO_IN_CLIP(playerPed, cur, maxAmmo);
		}
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

	// Submenu: Towns & Cities
	MenuBase *townsMenu = new MenuBase(new MenuItemListTitle("TOWNS & CITIES"));
	controller->RegisterMenu(townsMenu);
	townsMenu->AddItem(new MenuItemPlayerTeleport("SOUTH MAP",     { -5311.2583,  -4612.00,   -10.63389 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("SOUTH GUAMA",   { 1315.66381,  -6815.48,   42.377101 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("ANNESBURG",     { 2898.593994, 1239.85253, 44.073299 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("STRAWBERRY",	  { -1725.22143, -418.11560, 153.55740 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE",	  { -213.152496, 691.802979, 112.37100 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("RHODES",		  { 1282.707520, -1275.7485, 74.945099 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS",   { 2336.584961, -1106.2358, 44.737598 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("WAPITI",		  { 538.738525,  2217.46557, 240.23280 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("BUTCHERCREEK",  { 2552.203613, 835.510010, 81.183098 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("BLACKWATER",	  { -798.338379, -1238.9395, 43.537899 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("BEECHERS",	  { -1653.19738, -1448.8156, 82.503502 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("CALIGA HALL",   { 1705.509888, -1386.3237, 42.884998 }));	
	townsMenu->AddItem(new MenuItemPlayerTeleport("BRAITHWAITE",   { 1011.190674, -1661.6768, 45.918301 }));	
	townsMenu->AddItem(new MenuItemPlayerTeleport("VANHORN",		  { 2982.234863, 445.724915, 51.491501 }));	
	townsMenu->AddItem(new MenuItemPlayerTeleport("CORNWALL",	  { 437.7247920, 494.582092, 107.67649 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("COLTER",		  { -1371.6590,  2388.5073,  307.7218  }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("EMERALD RANCH", { 1332.332642, 300.425110, 86.306297 }));	
	townsMenu->AddItem(new MenuItemPlayerTeleport("PRONGHORN",	  { -2616.57714, 519.256775, 144.10809 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("MANZANITA POST",{ -1977.98754, -1545.6749, 112.87020 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("LAGRAS",		  { 2111.099121, -662.25317, 41.259899 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("ARMADILLO",	  { -3622.65527, -2586.5795, -15.36900 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("TUMBLEWEED",	  { -5382.39453, -2940.1596, 1.582700  }));	
	townsMenu->AddItem(new MenuItemPlayerTeleport("MACFARLANES RANCH", { -2296.26318, -2454.4101, 60.969898 }));
	townsMenu->AddItem(new MenuItemPlayerTeleport("BENEDICT POINT",{ -5269.60400, -3411.0588, -23.15930 }));		
	menu->AddItem(new MenuItemMenu("TOWNS & CITIES", townsMenu));

	// Submenu: Gunsmiths
	MenuBase *gunsmithsMenu = new MenuBase(new MenuItemListTitle("GUNSMITHS"));
	controller->RegisterMenu(gunsmithsMenu);
	gunsmithsMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE GUNSMITH",   { -281.17f, 778.94f, 119.50f }, 25.0f, true));
	gunsmithsMenu->AddItem(new MenuItemPlayerTeleport("RHODES GUNSMITH",      { 1322.31f, -1323.02f, 77.89f }, 90.0f, true));
	gunsmithsMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS GUNSMITH", { 2717.14f, -1286.90f, 49.64f }, 180.0f, true));
	gunsmithsMenu->AddItem(new MenuItemPlayerTeleport("ANNESBURG GUNSMITH",   { 2946.50f, 1319.53f, 44.82f }, 270.0f, true));
	gunsmithsMenu->AddItem(new MenuItemPlayerTeleport("TUMBLEWEED GUNSMITH",  { -5506.41f, -2963.95f, -0.64f }, 0.0f, true));
	menu->AddItem(new MenuItemMenu("GUNSMITHS", gunsmithsMenu));

	// Submenu: Stables
	MenuBase *stablesMenu = new MenuBase(new MenuItemListTitle("STABLES"));
	controller->RegisterMenu(stablesMenu);
	stablesMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE STABLE",   { -365.20f, 791.94f, 116.18f }, 0.0f));
	stablesMenu->AddItem(new MenuItemPlayerTeleport("RHODES STABLE",      { 1311.61f, -1339.71f, 77.21f }, 90.0f));
	stablesMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS STABLE", { 2550.67f, -1159.46f, 53.73f }, 90.0f));
	stablesMenu->AddItem(new MenuItemPlayerTeleport("BLACKWATER STABLE",  { -825.40f, -1323.76f, 47.91f }, 0.0f));
	stablesMenu->AddItem(new MenuItemPlayerTeleport("TUMBLEWEED STABLE",  { -5517.38f, -2936.82f, -2.22f }, 90.0f));
	menu->AddItem(new MenuItemMenu("STABLES", stablesMenu));

	// Submenu: Barbers
	MenuBase *barbersMenu = new MenuBase(new MenuItemListTitle("BARBERS"));
	controller->RegisterMenu(barbersMenu);
	barbersMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE BARBER",   { -280.85f, 783.15f, 119.51f }, 25.0f, true));
	barbersMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS BARBER", { 2651.49f, -1211.27f, 53.28f }, 270.0f, true));
	barbersMenu->AddItem(new MenuItemPlayerTeleport("BLACKWATER BARBER",  { -814.23f, -1365.77f, 43.68f }, 180.0f, true));
	menu->AddItem(new MenuItemMenu("BARBERS", barbersMenu));

	// Submenu: Saloons
	MenuBase *saloonsMenu = new MenuBase(new MenuItemListTitle("SALOONS"));
	controller->RegisterMenu(saloonsMenu);
	saloonsMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE SALOON",    { -307.96f, 814.16f, 118.99f }, 190.0f, true));
	saloonsMenu->AddItem(new MenuItemPlayerTeleport("RHODES SALOON",       { 1232.20f, -1251.08f, 73.67f }, 90.0f, true));
	saloonsMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS SALOON",  { 2792.44f, -1176.05f, 47.95f }, 90.0f, true));
	saloonsMenu->AddItem(new MenuItemPlayerTeleport("VAN HORN SALOON",     { 2983.45f, 430.15f, 51.18f }, 270.0f, true));
	saloonsMenu->AddItem(new MenuItemPlayerTeleport("STRAWBERRY SALOON",   { -1777.47f, -374.19f, 159.98f }, 270.0f, true));
	menu->AddItem(new MenuItemMenu("SALOONS", saloonsMenu));

	// Submenu: Hotels
	MenuBase *hotelsMenu = new MenuBase(new MenuItemListTitle("HOTELS"));
	controller->RegisterMenu(hotelsMenu);
	hotelsMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE HOTEL",    { -283.83f, 806.40f, 119.38f }, 321.0f, true));
	hotelsMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS HOTEL",  { 2721.45f, -1446.09f, 46.23f }, 321.0f, true));
	hotelsMenu->AddItem(new MenuItemPlayerTeleport("BLACKWATER HOTEL",   { -723.95f, -1324.07f, 43.88f }, 188.0f, true));
	menu->AddItem(new MenuItemMenu("HOTELS", hotelsMenu));

	// Submenu: General Stores
	MenuBase *storesMenu = new MenuBase(new MenuItemListTitle("GENERAL STORES"));
	controller->RegisterMenu(storesMenu);
	storesMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE STORE",   { -324.14f, 803.51f, 117.88f }, 278.0f, true));
	storesMenu->AddItem(new MenuItemPlayerTeleport("STRAWBERRY STORE",  { -1789.78f, -388.15f, 48.14f }, 270.0f, true));
	storesMenu->AddItem(new MenuItemPlayerTeleport("RHODES STORE",      { 1328.99f, -1293.28f, 77.02f }, 90.0f, true));
	storesMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS STORE", { 2859.36f, -1202.19f, 49.59f }, 270.0f, true));
	storesMenu->AddItem(new MenuItemPlayerTeleport("BLACKWATER STORE",  { -784.77f, -1322.15f, 44.02f }, 90.0f, true));
	menu->AddItem(new MenuItemMenu("GENERAL STORES", storesMenu));

	// Submenu: Doctors
	MenuBase *doctorsMenu = new MenuBase(new MenuItemListTitle("DOCTORS"));
	controller->RegisterMenu(doctorsMenu);
	doctorsMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE DOCTOR",   { -325.29f, 766.24f, 117.48f }, 210.0f, true));
	doctorsMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS DOCTOR", { 2725.05f, -1240.17f, 49.93f }, 0.0f, true));
	menu->AddItem(new MenuItemMenu("DOCTORS", doctorsMenu));

	// Submenu: Fences
	MenuBase *fencesMenu = new MenuBase(new MenuItemListTitle("FENCES"));
	controller->RegisterMenu(fencesMenu);
	fencesMenu->AddItem(new MenuItemPlayerTeleport("EMERALD RANCH FENCE",  { 1417.82f, 268.03f, 89.62f }, 0.0f, true));
	fencesMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS FENCE",    { 2849.29f, -1203.05f, 47.69f }, 180.0f, true));
	fencesMenu->AddItem(new MenuItemPlayerTeleport("RHODES FENCE",         { 1232.21f, -1251.09f, 73.68f }, 90.0f, true));
	fencesMenu->AddItem(new MenuItemPlayerTeleport("VAN HORN FENCE",       { 2983.45f, 430.15f, 51.18f }, 270.0f, true));
	menu->AddItem(new MenuItemMenu("FENCES", fencesMenu));

	// Submenu: Trappers & Tailors
	MenuBase *trappersMenu = new MenuBase(new MenuItemListTitle("TRAPPERS & TAILORS"));
	controller->RegisterMenu(trappersMenu);
	trappersMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS TRAPPER",     { 2832.54f, -1225.60f, 46.86f }, 0.0f, true));
	trappersMenu->AddItem(new MenuItemPlayerTeleport("RIGGS STATION TRAPPER",   { -1006.94f, -549.39f, 98.59f }, 0.0f, true));
	trappersMenu->AddItem(new MenuItemPlayerTeleport("BIG VALLEY TRAPPER",      { -2843.53f, 142.12f, 183.80f }, 0.0f, true));
	trappersMenu->AddItem(new MenuItemPlayerTeleport("STRAWBERRY TRAPPER",      { -1746.63f, -389.24f, 155.74f }, 0.0f, true));
	trappersMenu->AddItem(new MenuItemPlayerTeleport("TUMBLEWEED TRAPPER",      { -5512.21f, -2952.12f, -2.59f }, 0.0f, true));
	trappersMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS TAILOR",      { 2567.0f, -1232.0f, 46.0f }, 270.0f, true));
	menu->AddItem(new MenuItemMenu("TRAPPERS & TAILORS", trappersMenu));

	// Submenu: Post Offices
	MenuBase *postOfficesMenu = new MenuBase(new MenuItemListTitle("POST OFFICES"));
	controller->RegisterMenu(postOfficesMenu);
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("VALENTINE POST OFFICE",   { -174.0f, 633.0f, 114.0f }, 162.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("RHODES POST OFFICE",      { 1230.00f, -1295.00f, 76.90f }, 90.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("SAINT DENIS POST OFFICE", { 2747.49f, -1403.77f, 46.19f }, 180.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("ANNESBURG POST OFFICE",    { 2904.36f, 1248.80f, 44.87f }, 90.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("VAN HORN POST OFFICE",     { 2983.45f, 430.15f, 51.17f }, 270.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("EMERALD STATION POST",     { 1417.82f, 268.03f, 89.62f }, 0.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("WALLACE STATION POST",     { -1350.0f, 530.0f, 96.0f }, 90.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("FLATNECK STATION POST",    { -182.0f, 757.0f, 118.0f }, 180.0f, true));
	postOfficesMenu->AddItem(new MenuItemPlayerTeleport("RIGGS STATION POST",       { -4612.0f, -2728.0f, -4.0f }, 0.0f, true));
	menu->AddItem(new MenuItemMenu("POST OFFICES", postOfficesMenu));

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

// =======================================
// SQUAD-BASED BODYGUARD SYSTEM (v2)
// =======================================

// --- Guard/Squad data structures ---
struct GuardInfo { Ped ped{0}; Ped horse{0}; bool mounted{false}; };
struct Squad { std::vector<GuardInfo> guards; };
static std::vector<Squad> g_squads;
static int g_activeSquad = -1;

struct PendingSpawn {
	string pedModelName;
	DWORD pedModelHash;
	bool needsHorse;
	DWORD horseModelHash;
	unsigned long queueTime;
};
static std::vector<PendingSpawn> g_pendingSpawns;

// Squad behavior
enum class SquadOrder { Follow = 0, Hold = 1, GuardHere = 2, Patrol = 3 };
static SquadOrder g_activeSquadOrder = SquadOrder::Follow;
static Vector3 g_guardHerePos{};

// Formation
enum class GuardFormation { Column = 0, Line = 1, Wedge = 2 };
static GuardFormation g_activeFormation = GuardFormation::Column;

// Auto-defense
static bool g_autoDefenseEnabled = false;
static float g_autoDefenseRadius = 30.0f;
static DWORD g_lastAutoDefenseScanMs = 0;

// Throttle timers
static DWORD g_lastFormationTickMs = 0;
static DWORD g_lastPatrolTickMs = 0;
static DWORD g_lastCrowdControlTickMs = 0;
static DWORD g_lastGuardDefenseTickMs = 0;

// Auto mount/dismount tracking
static bool g_playerWasMounted = false;

// General menu tracking
enum ActiveMenuType {
	MENU_NONE,
	MENU_PAUSE,
	MENU_SATCHEL,
	MENU_JOURNAL
};
static ActiveMenuType g_activeMenuType = MENU_NONE;

// Satchel categories
static const wchar_t* g_satchelCategories[] = {
	L"Provisions", L"Kit", L"Valuables", L"Documents", L"Ingredients"
};
static const int g_satchelCategoryCount = 5;
static int g_satchelCategoryIndex = 0;
static int g_satchelItemIndex = 0;

// Journal categories
static const wchar_t* g_journalCategories[] = {
	L"Log", L"Objectives", L"Notes", L"Map Legend"
};
static const int g_journalCategoryCount = 4;
static int g_journalCategoryIndex = 0;
static int g_journalItemIndex = 0;

// Pause menu tracking
static bool g_wasPauseMenuActive = false;

// =======================================
// GUARD SETTINGS (toggled from Squad Manager menu)
// =======================================
static bool g_guardHorseEnabled = true;
static bool g_guardInvincibleEnabled = true;

// =======================================
// ACCESSIBILITY AUTO-ANNOUNCE SETTINGS
// =======================================
static bool g_autoMoneyAnnounce = true;
static bool g_autoHorseStateAnnounce = true;
static bool g_autoGuardStatusAnnounce = true;
static bool g_autoHorseCallAnnounce = true;
static bool g_autoCompassAnnounce = true;
static bool g_autoHonorAnnounce = false;
static int g_lastPlayerHonor = -9999;
static DWORD g_lastHonorCheckMs = 0;

static bool g_autoLootAnnounce = true;
static bool g_isLootingCorpse = false;
static Ped g_lootedPed = 0;
static DWORD g_lootCheckMs = 0;

// Auto-aim system
static bool g_autoAimEnabled = false;
static DWORD g_lastAutoAimMs = 0;
static Entity g_autoAimTarget = 0;

// Auto-announce tracking state
static int g_lastMoneyCents = -1;
static int g_lastHorseStateId = -1;  // 0=idle,1=walk,2=trot,3=canter,4=gallop
static DWORD g_lastMoneyCheckMs = 0;
static DWORD g_lastHorseStateCheckMs = 0;
static DWORD g_lastGuardAutoStatusMs = 0;
static int g_lastAliveCount = -1;
static int g_lastFightingCount = -1;

// Auto weapon change tracking
static Hash g_lastWeaponHash = 0;
static DWORD g_lastWeaponCheckMs = 0;

// Auto mount/horse proximity tracking
static bool g_wasPlayerMounted = false;
static Ped g_lastAnnouncedHorse = 0;
static DWORD g_lastHorseProximityCheckMs = 0;
static bool g_horseWhistleActive = false;
static DWORD g_horseWhistleStartMs = 0;
static Ped g_whistleTargetHorse = 0;
static DWORD g_lastHorseDistAnnounceMs = 0;
static float g_lastHorseAnnouncedDist = 0.0f;

// Auto compass direction tracking
static int g_lastCompassDir = -1;
static DWORD g_lastCompassCheckMs = 0;

// Animal guard behavior tracking
static bool g_autoAnimalBehaviorAnnounce = true;
static DWORD g_lastAnimalBehaviorCheckMs = 0;
// Per-guard last announced behavior (indexed by ped handle)
static std::unordered_map<int, int> g_animalLastBehavior; // ped -> behavior id

// Aiming target announcements
static Entity g_lastAimedEntity = 0;
static bool g_wasAiming = false;
static DWORD g_lastAimCheckMs = 0;





// =======================================
// WEAPON NAME LOOKUP TABLE
// =======================================
struct WeaponNameEntry { Hash hash; const wchar_t* name; };
static const WeaponNameEntry g_weaponNames[] = {
	// Revolvers
	{ 0x169F59F7, L"Cattleman Revolver" },
	{ 0xF5175BA1, L"Double Action Revolver" },
	{ 0xA77C3B93, L"Schofield Revolver" },
	{ 0xEED88FDA, L"Navy Revolver" },
	{ 0x7BBD1A30, L"LeMat Revolver" },
	// Pistols
	{ 0x7BBFC4A4, L"Mauser Pistol" },
	{ 0x83DD5220, L"Semi-Auto Pistol" },
	{ 0x657065F4, L"Volcanic Pistol" },
	// Rifles
	{ 0xA3B3F5F4, L"Springfield Rifle" },
	{ 0x772C8DD6, L"Bolt Action Rifle" },
	{ 0x63F46DE6, L"Varmint Rifle" },
	{ 0x65B3C226, L"Rolling Block Rifle" },
	{ 0xC734385A, L"Carcano Rifle" },
	// Repeaters
	{ 0xFB4D4D58, L"Carbine Repeater" },
	{ 0xC8B8C13D, L"Lancaster Repeater" },
	{ 0xA84B2E9A, L"Litchfield Repeater" },
	{ 0xCA2B854C, L"Evans Repeater" },
	// Shotguns
	{ 0x63CE75C0, L"Double-barreled Shotgun" },
	{ 0x31B7B9FE, L"Pump Shotgun" },
	{ 0x7CE36CB0, L"Sawed-off Shotgun" },
	{ 0x28950C71, L"Semi-Auto Shotgun" },
	{ 0xEAF672B7, L"Repeating Shotgun" },
	// Sniper
	{ 0x65B3C226, L"Rolling Block Rifle" },
	// Melee
	{ 0xDB21AC8C, L"Knife" },
	{ 0xF9DCBF2D, L"Machete" },
	{ 0x58CFBE96, L"Hatchet" },
	{ 0x12CB1839, L"Tomahawk" },
	// Thrown
	{ 0x5B78DAD4, L"Dynamite" },
	{ 0x7067E7A7, L"Fire Bottle" },
	{ 0x787F1B5C, L"Fire Bottle" },
	{ 0x2320496A, L"Throwing Knife" },
	{ 0xF92CDD67, L"Throwing Knife" },
	{ 0x21E70A35, L"Throwing Knife" },
	// Bow/Lasso
	{ 0x88A8505C, L"Bow" },
	{ 0x4007B858, L"Lasso" },
	// Unarmed  
	{ 0xD3BD8255, L"Unarmed" },
	{ 0, nullptr }
};

static const wchar_t* GetWeaponName(Hash weaponHash) {
	// Try game's label system first to get localized/correct names
	const char* label = invoke<const char*>(0xBD5DD5EAE2B6CE14, weaponHash); // GET_STRING_FROM_HASH_KEY
	if (label && label[0] != '\0' && strcmp(label, "NULL") != 0) {
		const char* text = label;
		if (UI::DOES_TEXT_LABEL_EXIST(const_cast<char*>(label))) {
			text = UI::_GET_LABEL_TEXT(const_cast<char*>(label));
		}
		if (text && text[0] != '\0') {
			static wchar_t wbuf[128];
			MultiByteToWideChar(CP_UTF8, 0, text, -1, wbuf, 128);
			return wbuf;
		}
	}
	// Fallback to hardcoded table lookup
	for (int i = 0; g_weaponNames[i].name != nullptr; i++) {
		if (g_weaponNames[i].hash == weaponHash) return g_weaponNames[i].name;
	}
	return L"Unknown weapon";
}

static bool IsPedHorse(Ped ped) {
	if (!ENTITY::DOES_ENTITY_EXIST(ped) || ENTITY::IS_ENTITY_DEAD(ped)) return false;
	Hash model = ENTITY::GET_ENTITY_MODEL(ped);
	for (int i = 0; i < ARRAY_LENGTH(pedModelInfos); i++) {
		if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(pedModelInfos[i].model.c_str())) == model) {
			return pedModelInfos[i].horse;
		}
	}
	return false;
}

static Ped GetPlayerHorse(Ped playerPed) {
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return 0;
	Ped horse = invoke<Ped>(0x4C8B59171957BCF7, playerPed); // _GET_LAST_MOUNT
	if (horse && ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse)) {
		return horse;
	}
	// Fallback: scan world for the nearest horse that belongs to the player
	int peds[1024];
	int count = worldGetAllPeds(peds, 1024);
	float bestDist = 300.0f; // max whistle range
	Ped bestHorse = 0;
	Vector3 pPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	for (int i = 0; i < count; i++) {
		Ped p = peds[i];
		if (p && p != playerPed && ENTITY::DOES_ENTITY_EXIST(p) && !ENTITY::IS_ENTITY_DEAD(p)) {
			if (IsPedHorse(p)) {
				// Check if this horse belongs to the player (bonding level > 0, or currently ridden)
				int bondLevel = invoke<int>(0xA4C8E23E29040DE0, p, 7); // GET_ATTRIBUTE_RANK (PA_BONDING = 7)
				bool isCurrentMount = (p == PED::GET_MOUNT(playerPed));
				if (bondLevel > 0 || isCurrentMount) {
					Vector3 hPos = ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE);
					float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
					if (dist < bestDist) {
						bestDist = dist;
						bestHorse = p;
					}
				}
			}
		}
	}
	return bestHorse;
}

// =======================================
// HORSE BREED NAME LOOKUP TABLE
// =======================================
struct HorseNameEntry { const char* model; const wchar_t* name; };
static const HorseNameEntry g_horseNames[] = {
	// Arabians
	{ "A_C_Horse_Arabian_Black", L"Black Arabian" },
	{ "A_C_Horse_Arabian_Grey", L"Grey Arabian" },
	{ "A_C_Horse_Arabian_White", L"White Arabian" },
	{ "A_C_Horse_Arabian_RoseGreyBay", L"Rose Grey Bay Arabian" },
	// Turkoman
	{ "A_C_Horse_Turkoman_DarkBay", L"Dark Bay Turkoman" },
	{ "A_C_Horse_Turkoman_Gold", L"Gold Turkoman" },
	{ "A_C_Horse_Turkoman_Silver", L"Silver Turkoman" },
	{ "A_C_Horse_Turkoman_GoldPalomino", L"Gold Palomino Turkoman" },
	// Missouri Fox Trotter
	{ "A_C_Horse_MissouriFoxTrotter_AmberChampagne", L"Amber Champagne Missouri Fox Trotter" },
	{ "A_C_Horse_MissouriFoxTrotter_SableChampagne", L"Sable Champagne Missouri Fox Trotter" },
	// Thoroughbred
	{ "A_C_Horse_Thoroughbred_BlackChestnut", L"Black Chestnut Thoroughbred" },
	{ "A_C_Horse_Thoroughbred_BloodBay", L"Blood Bay Thoroughbred" },
	{ "A_C_Horse_Thoroughbred_Brindle", L"Brindle Thoroughbred" },
	{ "A_C_Horse_Thoroughbred_DappleGrey", L"Dapple Grey Thoroughbred" },
	// American Standardbred
	{ "A_C_Horse_AmericanStandardbred_Black", L"Black American Standardbred" },
	{ "A_C_Horse_AmericanStandardbred_Buckskin", L"Buckskin American Standardbred" },
	{ "A_C_Horse_AmericanStandardbred_PalominoDapple", L"Palomino Dapple American Standardbred" },
	// American Paint
	{ "A_C_Horse_AmericanPaint_Greyovero", L"Grey Overo American Paint" },
	{ "A_C_Horse_AmericanPaint_Overo", L"Overo American Paint" },
	{ "A_C_Horse_AmericanPaint_SplashedWhite", L"Splashed White American Paint" },
	{ "A_C_Horse_AmericanPaint_Tobiano", L"Tobiano American Paint" },
	// Appaloosa
	{ "A_C_Horse_Appaloosa_BlackSnowflake", L"Black Snowflake Appaloosa" },
	{ "A_C_Horse_Appaloosa_Blanket", L"Blanket Appaloosa" },
	{ "A_C_Horse_Appaloosa_BrownLeopard", L"Brown Leopard Appaloosa" },
	{ "A_C_Horse_Appaloosa_FewSpotted_PC", L"Few Spotted Appaloosa" },
	{ "A_C_Horse_Appaloosa_Leopard", L"Leopard Appaloosa" },
	{ "A_C_Horse_Appaloosa_LeopardBlanket", L"Leopard Blanket Appaloosa" },
	// Hungarian Halfbred
	{ "A_C_Horse_HungarianHalfbred_DarkDappleGrey", L"Dark Dapple Grey Hungarian Halfbred" },
	{ "A_C_Horse_HungarianHalfbred_FlaxenChestnut", L"Flaxen Chestnut Hungarian Halfbred" },
	{ "A_C_Horse_HungarianHalfbred_LiverChestnut", L"Liver Chestnut Hungarian Halfbred" },
	{ "A_C_Horse_HungarianHalfbred_PiebaldTobiano", L"Piebald Tobiano Hungarian Halfbred" },
	// Dutch Warmblood
	{ "A_C_Horse_DutchWarmblood_ChocolateRoan", L"Chocolate Roan Dutch Warmblood" },
	{ "A_C_Horse_DutchWarmblood_SealBrown", L"Seal Brown Dutch Warmblood" },
	{ "A_C_Horse_DutchWarmblood_SootyBuckskin", L"Sooty Buckskin Dutch Warmblood" },
	// Tennessee Walker
	{ "A_C_Horse_TennesseeWalker_BlackRabicano", L"Black Rabicano Tennessee Walker" },
	{ "A_C_Horse_TennesseeWalker_Chestnut", L"Chestnut Tennessee Walker" },
	{ "A_C_Horse_TennesseeWalker_DappleBay", L"Dapple Bay Tennessee Walker" },
	{ "A_C_Horse_TennesseeWalker_FlaxenRoan", L"Flaxen Roan Tennessee Walker" },
	{ "A_C_Horse_TennesseeWalker_MahoganyBay", L"Mahogany Bay Tennessee Walker" },
	{ "A_C_Horse_TennesseeWalker_RedRoan", L"Red Roan Tennessee Walker" },
	// Morgan
	{ "A_C_Horse_Morgan_Bay", L"Bay Morgan" },
	{ "A_C_Horse_Morgan_BayRoan", L"Bay Roan Morgan" },
	{ "A_C_Horse_Morgan_FlaxenChestnut", L"Flaxen Chestnut Morgan" },
	{ "A_C_Horse_Morgan_LiverChestnut_PC", L"Liver Chestnut Morgan" },
	{ "A_C_Horse_Morgan_Palomino", L"Palomino Morgan" },
	// Kentucky Saddler
	{ "A_C_Horse_KentuckySaddle_Black", L"Black Kentucky Saddler" },
	{ "A_C_Horse_KentuckySaddle_ButterMilkBuckskin", L"Buttermilk Buckskin Kentucky Saddler" },
	{ "A_C_Horse_KentuckySaddle_ChestnutPinto", L"Chestnut Pinto Kentucky Saddler" },
	{ "A_C_Horse_KentuckySaddle_Grey", L"Grey Kentucky Saddler" },
	{ "A_C_Horse_KentuckySaddle_SilverBay", L"Silver Bay Kentucky Saddler" },
	// Nokota
	{ "A_C_Horse_Nokota_BlueRoan", L"Blue Roan Nokota" },
	{ "A_C_Horse_Nokota_ReverseDappleRoan", L"Reverse Dapple Roan Nokota" },
	{ "A_C_Horse_Nokota_WhiteRoan", L"White Roan Nokota" },
	// Shire
	{ "A_C_Horse_Shire_DarkBay", L"Dark Bay Shire" },
	{ "A_C_Horse_Shire_LightGrey", L"Light Grey Shire" },
	{ "A_C_Horse_Shire_RavenBlack", L"Raven Black Shire" },
	// Suffolk Punch
	{ "A_C_Horse_SuffolkPunch_RedChestnut", L"Red Chestnut Suffolk Punch" },
	{ "A_C_Horse_SuffolkPunch_Sorrel", L"Sorrel Suffolk Punch" },
	// Mustang
	{ "A_C_Horse_Mustang_GoldenDun", L"Golden Dun Mustang" },
	{ "A_C_Horse_Mustang_GrulloDun", L"Grullo Dun Mustang" },
	{ "A_C_Horse_Mustang_TigerStripedBay", L"Tiger Striped Bay Mustang" },
	{ "A_C_Horse_Mustang_WildBay", L"Wild Bay Mustang" },
	// Belgian
	{ "A_C_Horse_Belgian_BlondChestnut", L"Blond Chestnut Belgian" },
	{ "A_C_Horse_Belgian_MealyChestnut", L"Mealy Chestnut Belgian" },
	// Ardennes
	{ "A_C_Horse_Ardennes_BayRoan", L"Bay Roan Ardennes" },
	{ "A_C_Horse_Ardennes_IronGreyRoan", L"Iron Grey Roan Ardennes" },
	{ "A_C_Horse_Ardennes_StrawberryRoan", L"Strawberry Roan Ardennes" },
	{ nullptr, nullptr }
};

static const wchar_t* GetHorseName(Hash modelHash) {
	for (int i = 0; g_horseNames[i].model != nullptr; i++) {
		if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(g_horseNames[i].model)) == modelHash)
			return g_horseNames[i].name;
	}
	// Fallback: try game's label system
	const char* label = invoke<const char*>(0xBD5DD5EAE2B6CE14, modelHash);
	if (label && label[0] != '\0' && strcmp(label, "NULL") != 0) {
		static wchar_t wbuf[128];
		MultiByteToWideChar(CP_UTF8, 0, label, -1, wbuf, 128);
		return wbuf;
	}
	return L"Unknown horse";
}

// =======================================
// NUMPAD MODE SWITCHING SYSTEM
// =======================================
enum class NumpadMode { Bodyguard = 0, Global = 1, Horse = 2 };
static NumpadMode g_currentNumpadMode = NumpadMode::Bodyguard;
static const wchar_t* GetModeName(NumpadMode m) {
	switch (m) {
		case NumpadMode::Bodyguard:  return L"Bodyguard mode";
		case NumpadMode::Global:     return L"Global mode";
		case NumpadMode::Horse:      return L"Horse mode";
		default: return L"Unknown";
	}
}

// --- Formation offset calculation ---
static inline void GetFormationOffset(int idx, GuardFormation form, bool mounted, float &outX, float &outY) {
	const float stepBack = mounted ? 4.0f : 2.8f;
	const float lateral = mounted ? 2.6f : 1.8f;
	switch (form) {
		case GuardFormation::Column:
			outX = 0.0f; outY = -(2.0f + stepBack * (idx + 1));
			break;
		case GuardFormation::Line: {
			int side = (idx % 2 == 0) ? -1 : 1;
			int rank = (idx / 2);
			outX = side * (lateral * (rank + 1));
			outY = -(3.5f + (mounted ? 2.0f : 1.2f));
			break;
		}
		case GuardFormation::Wedge: default: {
			int row = 0; int count = 0;
			while (count + (row + 1) <= idx) { count += (row + 1); ++row; }
			int posInRow = idx - count;
			float baseBack = 3.5f + row * stepBack;
			float startX = -(row * 0.5f) * lateral;
			outX = startX + posInRow * lateral;
			outY = -baseBack;
			break;
		}
	}
}

// --- Follow command helper ---
static inline void CommandGuardFollowOffset(const GuardInfo &gi, Ped player, float offX, float offY) {
	if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped)) return;
	bool isMounted = PED::IS_PED_ON_MOUNT(gi.ped) ? true : false;
	if (isMounted && gi.horse && ENTITY::DOES_ENTITY_EXIST(gi.horse)) {
		// Use gallop speed (3.5f) so mounted guards keep up with player
		AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(gi.horse, player, offX, offY, 0.0f, 3.5f, -1, 1.8f, TRUE, FALSE, FALSE, FALSE, FALSE);
	} else {
		AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(gi.ped, player, offX, offY, 0.0f, 2.6f, -1, 1.6f, TRUE, FALSE, FALSE, FALSE, FALSE);
	}
}

// --- Check if a ped is one of our guards ---
static bool IsOurGuard(Ped p) {
	for (const auto &sq : g_squads) {
		for (const auto &gi : sq.guards) {
			if (gi.ped == p || gi.horse == p) return true;
		}
	}
	return false;
}

// --- Cleanup dead guards from active squad ---
static void CleanupDeadGuards() {
	if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) return;
	auto &guards = g_squads[g_activeSquad].guards;
	for (int i = (int)guards.size() - 1; i >= 0; --i) {
		if (!guards[i].ped || !ENTITY::DOES_ENTITY_EXIST(guards[i].ped) || ENTITY::IS_ENTITY_DEAD(guards[i].ped)) {
			guards.erase(guards.begin() + i);
		}
	}
}

static int GetTotalGuardCount();

// --- Spawn a guard into the active squad ---
static bool SpawnGuard(const string &pedModel) {
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return false;

	// Ensure we have at least one squad
	if (g_squads.empty()) {
		g_squads.push_back(Squad());
		g_activeSquad = 0;
	}
	if (g_activeSquad < 0) g_activeSquad = 0;

	// Cleanup dead guards first
	CleanupDeadGuards();

	// Check capacity (max 14 per squad, including pending spawns)
	int currentTotal = (int)g_squads[g_activeSquad].guards.size() + (int)g_pendingSpawns.size();
	if (currentTotal >= 14) {
		DebugLog::log("Cannot queue guard spawn: squad limit (14) reached. Current: %d", currentTotal);
		return false;
	}

	// Validate model (only IS_MODEL_IN_CDIMAGE)
	DWORD model = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(pedModel.c_str()));
	if (!STREAMING::IS_MODEL_IN_CDIMAGE(model)) {
		model = GAMEPLAY::GET_HASH_KEY("S_M_M_ValCowpoke_01");
		if (!STREAMING::IS_MODEL_IN_CDIMAGE(model)) return false;
	}

	bool isAnimal = (model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF")
				|| model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_MEDIUM")
				|| model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_SMALL")
				|| model == GAMEPLAY::GET_HASH_KEY("A_C_COUGAR_01")
				|| model == GAMEPLAY::GET_HASH_KEY("A_C_PANTHER_01"));

	bool needsHorse = (!isAnimal && g_guardHorseEnabled);
	Hash horseModel = 0;
	if (needsHorse) {
		horseModel = GAMEPLAY::GET_HASH_KEY("A_C_Horse_Turkoman_Gold");
		if (!STREAMING::IS_MODEL_IN_CDIMAGE(horseModel)) {
			horseModel = GAMEPLAY::GET_HASH_KEY("A_C_Horse_Morgan_Bay");
		}
		STREAMING::REQUEST_MODEL(horseModel, FALSE);
	}

	STREAMING::REQUEST_MODEL(model, FALSE);

	PendingSpawn ps;
	ps.pedModelName = pedModel;
	ps.pedModelHash = model;
	ps.needsHorse = needsHorse;
	ps.horseModelHash = horseModel;
	ps.queueTime = GetTickCount();

	g_pendingSpawns.push_back(ps);
	DebugLog::log("Queued guard spawn: ped=%s, hash=0x%08X, needsHorse=%d", pedModel.c_str(), model, needsHorse);

	return true;
}

static void ProcessPendingSpawns() {
	if (g_pendingSpawns.empty()) return;

	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

	for (auto it = g_pendingSpawns.begin(); it != g_pendingSpawns.end(); ) {
		bool pedLoaded = STREAMING::HAS_MODEL_LOADED(it->pedModelHash) ? true : false;
		if (!pedLoaded) {
			STREAMING::REQUEST_MODEL(it->pedModelHash, FALSE);
		}
		bool horseLoaded = true;
		if (it->needsHorse) {
			horseLoaded = STREAMING::HAS_MODEL_LOADED(it->horseModelHash) ? true : false;
			if (!horseLoaded) {
				STREAMING::REQUEST_MODEL(it->horseModelHash, FALSE);
			}
		}

		if (pedLoaded && horseLoaded) {
			Vector3 spawnPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, 0.0f, 3.0f, -0.3f);
			Ped newPed = PED::CREATE_PED(it->pedModelHash, spawnPos.x, spawnPos.y, spawnPos.z, static_cast<float>(rand() % 360), 0, 0, 0, 0);
			WAIT(0);

			if (ENTITY::DOES_ENTITY_EXIST(newPed)) {
				PED::SET_PED_VISIBLE(newPed, TRUE);
				PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(newPed, TRUE);
				PED::SET_PED_KEEP_TASK(newPed, TRUE);
				PED::SET_PED_COMBAT_ABILITY(newPed, 2);
				PED::SET_PED_COMBAT_MOVEMENT(newPed, 2);
				PED::SET_PED_FLEE_ATTRIBUTES(newPed, 0, FALSE);
				PED::SET_PED_ACCURACY(newPed, 70);

				Hash myGroup = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(playerPed);
				PED::SET_PED_RELATIONSHIP_GROUP_HASH(newPed, myGroup);

				int grp = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
				if (grp) PED::SET_PED_AS_GROUP_MEMBER(newPed, grp);

				bool isAnimal = (it->pedModelHash == GAMEPLAY::GET_HASH_KEY("A_C_WOLF")
							|| it->pedModelHash == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_MEDIUM")
							|| it->pedModelHash == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_SMALL")
							|| it->pedModelHash == GAMEPLAY::GET_HASH_KEY("A_C_COUGAR_01")
							|| it->pedModelHash == GAMEPLAY::GET_HASH_KEY("A_C_PANTHER_01"));

				if (g_guardInvincibleEnabled) {
					ENTITY::SET_ENTITY_INVINCIBLE(newPed, TRUE);
					ENTITY::SET_ENTITY_CAN_BE_DAMAGED(newPed, FALSE);
				}

				if (!isAnimal) {
					Hash w = GAMEPLAY::GET_HASH_KEY("WEAPON_REPEATER_CARBINE");
					WEAPON::GIVE_DELAYED_WEAPON_TO_PED(newPed, w, 200, 1, 0x2cd419dc);
					WEAPON::SET_CURRENT_PED_WEAPON(newPed, w, 1, 0, 0, 0);
					WEAPON::SET_PED_AMMO(newPed, w, 999);
				}

				Ped guardHorse = 0;
				if (it->needsHorse) {
					Vector3 horsePos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, 2.0f, 4.0f + (float)(g_squads[g_activeSquad].guards.size()) * 2.0f, -0.3f);
					guardHorse = PED::CREATE_PED(it->horseModelHash, horsePos.x, horsePos.y, horsePos.z, static_cast<float>(rand() % 360), 0, 0, 0, 0);
					WAIT(0);

					if (ENTITY::DOES_ENTITY_EXIST(guardHorse)) {
						if (g_guardInvincibleEnabled) {
							ENTITY::SET_ENTITY_INVINCIBLE(guardHorse, TRUE);
							ENTITY::SET_ENTITY_CAN_BE_DAMAGED(guardHorse, FALSE);
						}
						PED::SET_PED_VISIBLE(guardHorse, TRUE);
						PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(guardHorse, TRUE);
						invoke<Void>(0xED1C764997A86D5A, newPed, guardHorse);
					}
					STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(it->horseModelHash);
				}

				int idx = (int)g_squads[g_activeSquad].guards.size();
				float offX = 0.0f, offY = 0.0f;
				GetFormationOffset(idx, g_activeFormation, false, offX, offY);
				AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(newPed, playerPed, offX, offY, 0.0f, 2.6f, -1, 1.6f, TRUE, FALSE, FALSE, FALSE, FALSE);

				GuardInfo gi;
				gi.ped = newPed;
				gi.horse = guardHorse;
				gi.mounted = (guardHorse != 0 && ENTITY::DOES_ENTITY_EXIST(guardHorse));
				g_squads[g_activeSquad].guards.push_back(gi);

				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(it->pedModelHash);

				int totalCount = GetTotalGuardCount();
				wchar_t speechBuf[100];
				swprintf_s(speechBuf, L"Guard spawned. Squad size %d", totalCount);
				A11y::speak(speechBuf, false);

				DebugLog::log("Async spawned guard success: ped=%d, horse=%d, total squad=%d", newPed, guardHorse, totalCount);
			} else {
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(it->pedModelHash);
				if (it->needsHorse) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(it->horseModelHash);
				A11y::speak(L"Guard spawn failed entity creation", false);
				DebugLog::log("Async spawn failed: entity creation failed for model hash 0x%08X", it->pedModelHash);
			}

			it = g_pendingSpawns.erase(it);
		}
		else {
			if (GetTickCount() - it->queueTime > 15000) {
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(it->pedModelHash);
				if (it->needsHorse) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(it->horseModelHash);
				A11y::speak(L"Guard spawn timed out loading model", false);
				DebugLog::log("Async spawn timed out loading model hash 0x%08X", it->pedModelHash);

				it = g_pendingSpawns.erase(it);
			} else {
				++it;
			}
		}
	}
}

// --- Give weapon to all guards in active squad ---
static void GiveWeaponToSquad(Hash weaponHash) {
	if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) return;
	for (auto &gi : g_squads[g_activeSquad].guards) {
		if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
		WEAPON::GIVE_DELAYED_WEAPON_TO_PED(gi.ped, weaponHash, 500, 1, 0x2cd419dc);
		WEAPON::SET_CURRENT_PED_WEAPON(gi.ped, weaponHash, 1, 0, 0, 0);
		WEAPON::SET_PED_AMMO(gi.ped, weaponHash, 999);
	}
}

// --- Dismiss all guards ---
static void DismissAllGuards() {
	for (const auto &ps : g_pendingSpawns) {
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(ps.pedModelHash);
		if (ps.needsHorse) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(ps.horseModelHash);
	}
	g_pendingSpawns.clear();

	for (auto &sq : g_squads) {
		for (auto &gi : sq.guards) {
			if (gi.horse && ENTITY::DOES_ENTITY_EXIST(gi.horse)) {
				ENTITY::SET_ENTITY_AS_MISSION_ENTITY(gi.horse, 0, 0);
				ENTITY::DELETE_ENTITY(&gi.horse);
			}
			if (gi.ped && ENTITY::DOES_ENTITY_EXIST(gi.ped)) {
				PED::REMOVE_PED_FROM_GROUP(gi.ped);
				AI::CLEAR_PED_TASKS(gi.ped, FALSE, FALSE);
				ENTITY::SET_ENTITY_AS_MISSION_ENTITY(gi.ped, 0, 0);
				ENTITY::DELETE_ENTITY(&gi.ped);
			}
		}
		sq.guards.clear();
	}
	g_squads.clear();
	g_activeSquad = -1;
	g_activeSquadOrder = SquadOrder::Follow;
}

// --- Auto-defense: scan nearby hostiles and order guards to engage ---
static void SquadAutoDefense() {
	if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) return;
	if (g_activeSquadOrder == SquadOrder::Hold) return;

	DWORD now = GetTickCount();
	if ((now - g_lastGuardDefenseTickMs) < 700) return;
	g_lastGuardDefenseTickMs = now;

	Ped me = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(me)) return;

	Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
	Ped attackers[24]; int attackerN = 0;

	int packed[33] = { 32 };
	int count = PED::GET_PED_NEARBY_PEDS(me, packed, -1, 0);
	bool meleeDanger = PED::IS_PED_IN_MELEE_COMBAT(me) ? true : false;

	if (count > 0) {
		int lim = packed[0]; if (lim > 32) lim = 32;
		for (int i = 1; i <= lim; ++i) {
			Ped q = (Ped)packed[i];
			if (!q || q == me) continue;
			if (!ENTITY::DOES_ENTITY_EXIST(q) || ENTITY::IS_ENTITY_DEAD(q) || PED::IS_PED_A_PLAYER(q)) continue;
			if (IsOurGuard(q)) continue;

			Vector3 qc = ENTITY::GET_ENTITY_COORDS(q, TRUE, FALSE);
			float d = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, qc.x, qc.y, qc.z, TRUE);
			if (d > 65.0f) continue;

			bool shot = PED::IS_PED_SHOOTING(q) ? true : false;
			bool attackingMe = PED::IS_PED_IN_COMBAT(q, me) ? true : false;
			bool damaged = ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(me, q, TRUE, FALSE) ? true : false;
			bool closeMelee = (d < 6.5f) && (PED::IS_PED_IN_MELEE_COMBAT(q) || meleeDanger);

			if (shot || attackingMe || damaged || closeMelee) {
				if (attackerN < 24) attackers[attackerN++] = q;
			}
		}
	}

	if (attackerN > 0) {
		auto &sq = g_squads[g_activeSquad];
		int assignIdx = 0;
		for (auto &gi : sq.guards) {
			Ped gp = gi.ped;
			if (!gp || !ENTITY::DOES_ENTITY_EXIST(gp) || ENTITY::IS_ENTITY_DEAD(gp)) continue;
			if (PED::IS_PED_IN_COMBAT(gp, 0)) continue;

			Ped tgt = attackers[assignIdx % attackerN]; assignIdx++;
			PED::SET_PED_COMBAT_ABILITY(gp, 2);
			PED::SET_PED_COMBAT_MOVEMENT(gp, 2);
			PED::SET_PED_FLEE_ATTRIBUTES(gp, 0, FALSE);
			PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(gp, TRUE);
			AI::TASK_COMBAT_PED(gp, tgt, 0, 0);
		}
		ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(me);
	}
}

// --- Silent auto-defense: instant kill hostiles within radius ---
static void ScanAndEliminateHostiles() {
	if (!g_autoDefenseEnabled) return;

	DWORD now = GetTickCount();
	if ((now - g_lastAutoDefenseScanMs) < 500) return;
	g_lastAutoDefenseScanMs = now;

	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed) || ENTITY::IS_ENTITY_DEAD(playerPed)) return;

	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	Ped playerHorse = 0;
	if (PED::IS_PED_ON_MOUNT(playerPed)) playerHorse = PED::GET_MOUNT(playerPed);

	int packed[33] = { 32 };
	int count = PED::GET_PED_NEARBY_PEDS(playerPed, packed, -1, 0);
	if (count <= 0) return;

	int lim = packed[0]; if (lim > 32) lim = 32;
	for (int i = 1; i <= lim; ++i) {
		Ped target = (Ped)packed[i];
		if (!target || target == playerPed) continue;
		if (!ENTITY::DOES_ENTITY_EXIST(target) || ENTITY::IS_ENTITY_DEAD(target)) continue;
		if (PED::IS_PED_A_PLAYER(target)) continue;
		if (playerHorse && target == playerHorse) continue;
		if (IsOurGuard(target)) continue;

		Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, TRUE, FALSE);
		float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z, targetPos.x, targetPos.y, targetPos.z, TRUE);
		if (distance > g_autoDefenseRadius) continue;

		bool isHostile = PED::IS_PED_IN_COMBAT(target, playerPed) || PED::IS_PED_SHOOTING(target);
		if (ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(playerPed, target, TRUE, FALSE)) isHostile = true;

		if (isHostile) {
			ENTITY::SET_ENTITY_HEALTH(target, 0, 0);
		}
	}
}

// --- Squad background maintenance (called each frame) ---
static void SquadFrameMaintenance() {
	if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) return;
	DWORD now = GetTickCount();
	Ped me = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(me)) return;

	// Auto-defense: guards engage attackers
	SquadAutoDefense();

	// Silent protection
	ScanAndEliminateHostiles();

	// Formation maintenance (every 1.2s during Follow)
	if (g_activeSquadOrder == SquadOrder::Follow && (now - g_lastFormationTickMs) > 1200) {
		g_lastFormationTickMs = now;
		int i = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			bool mounted = PED::IS_PED_ON_MOUNT(gi.ped) ? true : false;
			float ox = 0.0f, oy = 0.0f;
			GetFormationOffset(i, g_activeFormation, mounted, ox, oy); ++i;
			CommandGuardFollowOffset(gi, me, ox, oy);
		}
	}

	// Patrol wander (every 8s during Patrol)
	if (g_activeSquadOrder == SquadOrder::Patrol && (now - g_lastPatrolTickMs) > 8000) {
		g_lastPatrolTickMs = now;
		Vector3 c = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			float rx = ((GAMEPLAY::GET_RANDOM_INT_IN_RANGE(-100, 101)) / 100.0f) * 2.5f;
			float ry = ((GAMEPLAY::GET_RANDOM_INT_IN_RANGE(-100, 101)) / 100.0f) * 2.5f;
			AI::TASK_WANDER_IN_AREA(gi.ped, c.x + rx, c.y + ry, c.z, 4.0f, 1.8f, 3.5f, TRUE);
		}
	}

	// Crowd control bubble (every 900ms)
	if ((now - g_lastCrowdControlTickMs) > 900) {
		g_lastCrowdControlTickMs = now;
		Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
		bool playerMounted = PED::IS_PED_ON_MOUNT(me) ? true : false;
		float bubbleR = playerMounted ? 5.0f : 3.4f;

		int packed[33] = { 32 };
		int count = PED::GET_PED_NEARBY_PEDS(me, packed, -1, 0);
		if (count > 0) {
			int lim = packed[0]; if (lim > 32) lim = 32;
			for (int i = 1; i <= lim; ++i) {
				Ped p = (Ped)packed[i];
				if (!p || p == me) continue;
				if (!ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p) || PED::IS_PED_A_PLAYER(p)) continue;
				if (IsOurGuard(p)) continue;

				Vector3 pc = ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE);
				float d = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(meC.x, meC.y, meC.z, pc.x, pc.y, pc.z, TRUE);
				if (d < bubbleR) {
					Vector3 dir = { pc.x - meC.x, pc.y - meC.y, 0.0f };
					float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
					if (len < 0.001f) len = 0.001f;
					dir.x /= len; dir.y /= len;
					float pushDist = playerMounted ? 6.0f : 4.0f;
					Vector3 tgt = { meC.x + dir.x * (pushDist + 1.0f), meC.y + dir.y * (pushDist + 1.0f), pc.z };
					AI::TASK_GO_TO_COORD_ANY_MEANS(p, tgt.x, tgt.y, tgt.z, 1.4f, 0, FALSE, 0, 0.0f);
				}
			}
		}
	}

	// Auto mount/dismount: sync guards with player mount state
	bool playerNowMounted = PED::IS_PED_ON_MOUNT(me) ? true : false;
	if (playerNowMounted && !g_playerWasMounted) {
		// Player just mounted - command human guards to mount their horses
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			if (gi.horse && ENTITY::DOES_ENTITY_EXIST(gi.horse) && !ENTITY::IS_ENTITY_DEAD(gi.horse)) {
				if (!PED::IS_PED_ON_MOUNT(gi.ped)) {
					invoke<Void>(0xED1C764997A86D5A, gi.ped, gi.horse); // _SET_PED_ONTO_MOUNT
				}
			}
		}
	} else if (!playerNowMounted && g_playerWasMounted) {
		// Player just dismounted - command guards to dismount
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			if (PED::IS_PED_ON_MOUNT(gi.ped)) {
				AI::CLEAR_PED_TASKS_IMMEDIATELY(gi.ped, TRUE, TRUE);
			}
		}
	}
	g_playerWasMounted = playerNowMounted;
}

// --- Get total guard count across all squads ---
static int GetTotalGuardCount() {
	int total = 0;
	for (const auto &sq : g_squads) total += (int)sq.guards.size();
	return total;
}

// =======================================
// SQUAD MENU ITEMS
// =======================================

class MenuItemSquadSpawn : public MenuItemDefault {
	string m_model;
	virtual void OnSelect() {
		if (SpawnGuard(m_model)) {
			int count = GetTotalGuardCount() + (int)g_pendingSpawns.size();
			wchar_t buf[64]; swprintf_s(buf, L"Guard queued. Total: %d", count);
			A11y::speak(buf, true);
			SetStatusText("Guard queued! Total: " + to_string(count));
		} else {
			A11y::speak(L"Spawn failed or squad full", true);
			SetStatusText("Squad full (14) or spawn failed!");
		}
	}
public:
	MenuItemSquadSpawn(string caption, string model) : MenuItemDefault(caption), m_model(model) {}
};

class MenuItemSquadGiveWeapon : public MenuItemDefault {
	string m_weaponName;
	virtual void OnSelect() {
		Hash wh = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(("WEAPON_" + m_weaponName).c_str()));
		GiveWeaponToSquad(wh);
		A11y::speak(L"Weapon given", true);
		SetStatusText("Weapon: " + m_weaponName);
	}
public:
	MenuItemSquadGiveWeapon(string caption, string weaponName) : MenuItemDefault(caption), m_weaponName(weaponName) {}
};

class MenuItemSquadDismiss : public MenuItemDefault {
	virtual void OnSelect() {
		DismissAllGuards();
		A11y::speak(L"All guards dismissed", true);
		SetStatusText("All guards dismissed!");
	}
public:
	MenuItemSquadDismiss(string caption) : MenuItemDefault(caption) {}
};

class MenuItemSquadFollow : public MenuItemDefault {
	virtual void OnSelect() {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) {
			A11y::speak(L"No squad", true); return;
		}
		Ped me = PLAYER::PLAYER_PED_ID();
		int i = 0, applied = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			float ox = 0.0f, oy = 0.0f;
			GetFormationOffset(i, g_activeFormation, PED::IS_PED_ON_MOUNT(gi.ped) ? true : false, ox, oy); ++i;
			CommandGuardFollowOffset(gi, me, ox, oy);
			++applied;
		}
		g_activeSquadOrder = SquadOrder::Follow;
		wchar_t buf[64]; swprintf_s(buf, L"Following: %d guards", applied);
		A11y::speak(buf, true);
	}
public:
	MenuItemSquadFollow(string caption) : MenuItemDefault(caption) {}
};

class MenuItemSquadHold : public MenuItemDefault {
	virtual void OnSelect() {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) {
			A11y::speak(L"No squad", true); return;
		}
		int applied = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			AI::CLEAR_PED_TASKS(gi.ped, TRUE, TRUE);
			AI::TASK_STAND_STILL(gi.ped, 30000);
			++applied;
		}
		g_activeSquadOrder = SquadOrder::Hold;
		wchar_t buf[64]; swprintf_s(buf, L"Holding: %d guards", applied);
		A11y::speak(buf, true);
	}
public:
	MenuItemSquadHold(string caption) : MenuItemDefault(caption) {}
};

class MenuItemSquadGuardHere : public MenuItemDefault {
	virtual void OnSelect() {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) {
			A11y::speak(L"No squad", true); return;
		}
		Ped me = PLAYER::PLAYER_PED_ID();
		Vector3 c = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
		float baseH = ENTITY::GET_ENTITY_HEADING(me);
		static const float ring[7][2] = { {1.5f,0.0f}, {-1.5f,0.0f}, {0.0f,1.5f}, {0.0f,-1.5f}, {2.0f,0.8f}, {-2.0f,0.8f}, {0.0f,2.5f} };
		int applied = 0, i = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			float dx = ring[i % 7][0], dy = ring[i % 7][1]; ++i;
			AI::CLEAR_PED_TASKS(gi.ped, TRUE, TRUE);
			AI::TASK_STAND_GUARD(gi.ped, c.x + dx, c.y + dy, c.z, baseH, (char *)"WORLD_HUMAN_STAND_GUARD");
			++applied;
		}
		g_activeSquadOrder = SquadOrder::GuardHere;
		g_guardHerePos = c;
		wchar_t buf[64]; swprintf_s(buf, L"Guarding here: %d guards", applied);
		A11y::speak(buf, true);
	}
public:
	MenuItemSquadGuardHere(string caption) : MenuItemDefault(caption) {}
};

class MenuItemSquadToggleDefense : public MenuItemDefault {
	virtual void OnSelect() {
		g_autoDefenseEnabled = !g_autoDefenseEnabled;
		if (g_autoDefenseEnabled)
			A11y::speak(L"Auto defense activated", true);
		else
			A11y::speak(L"Auto defense deactivated", true);
	}
public:
	MenuItemSquadToggleDefense(string caption) : MenuItemDefault(caption) {}
};

class MenuItemSquadFormation : public MenuItemDefault {
	virtual void OnSelect() {
		int f = (int)g_activeFormation;
		f = (f + 1) % 3;
		g_activeFormation = (GuardFormation)f;
		const wchar_t *names[] = { L"Column formation", L"Line formation", L"Wedge formation" };
		A11y::speak(names[f], true);
	}
public:
	MenuItemSquadFormation(string caption) : MenuItemDefault(caption) {}
};

// =======================================
// SQUAD MENU BUILDERS
// =======================================

MenuBase *CreateSquadWeaponMenu(MenuController *controller) {
	auto menu = new MenuBase(new MenuItemListTitle("WEAPON SELECT"));
	controller->RegisterMenu(menu);
	menu->AddItem(new MenuItemSquadGiveWeapon("REVOLVER CATTLEMAN", "REVOLVER_CATTLEMAN"));
	menu->AddItem(new MenuItemSquadGiveWeapon("REVOLVER DOUBLE ACTION", "REVOLVER_DOUBLE_ACTION"));
	menu->AddItem(new MenuItemSquadGiveWeapon("PISTOL MAUSER", "PISTOL_MAUSER"));
	menu->AddItem(new MenuItemSquadGiveWeapon("RIFLE SPRINGFIELD", "RIFLE_SPRINGFIELD"));
	menu->AddItem(new MenuItemSquadGiveWeapon("RIFLE BOLT ACTION", "RIFLE_BOLTACTION"));
	menu->AddItem(new MenuItemSquadGiveWeapon("CARBINE REPEATER", "CARBINE_REPEATER"));
	menu->AddItem(new MenuItemSquadGiveWeapon("SHOTGUN SAWEDOFF", "SHOTGUN_SAWEDOFF"));
	menu->AddItem(new MenuItemSquadGiveWeapon("REPEATER CARBINE", "REPEATER_CARBINE"));
	menu->AddItem(new MenuItemSquadGiveWeapon("SNIPER CARCANO", "SNIPER_CARCANO"));
	return menu;
}

MenuBase *CreateSquadSpawnMenu(MenuController *controller) {
	auto menu = new MenuBase(new MenuItemListTitle("SPAWN GUARD"));
	controller->RegisterMenu(menu);
	menu->AddItem(new MenuItemSquadSpawn("COWBOY", "S_M_M_ValCowpoke_01"));
	menu->AddItem(new MenuItemSquadSpawn("COWBOY 2", "S_M_M_BLWCOWPOKE_01"));
	menu->AddItem(new MenuItemSquadSpawn("OUTLAW", "U_M_M_LNSOUTLAW_01"));
	menu->AddItem(new MenuItemSquadSpawn("OUTLAW 2", "U_M_M_LNSOUTLAW_02"));
	menu->AddItem(new MenuItemSquadSpawn("GUNSLINGER", "A_M_M_UniGunslinger_01"));
	menu->AddItem(new MenuItemSquadSpawn("CORNWALL GUARD", "S_M_M_CORNWALLGUARD_01"));
	menu->AddItem(new MenuItemSquadSpawn("WOLF", "A_C_WOLF"));
	menu->AddItem(new MenuItemSquadSpawn("WOLF MEDIUM", "A_C_WOLF_MEDIUM"));
	menu->AddItem(new MenuItemSquadSpawn("WOLF SMALL", "A_C_WOLF_SMALL"));
	menu->AddItem(new MenuItemSquadSpawn("COUGAR", "A_C_COUGAR_01"));
	menu->AddItem(new MenuItemSquadSpawn("PANTHER", "A_C_PANTHER_01"));
	return menu;
}

MenuBase *CreateSquadOrderMenu(MenuController *controller) {
	auto menu = new MenuBase(new MenuItemTitle("SQUAD ORDERS"));
	controller->RegisterMenu(menu);
	menu->AddItem(new MenuItemSquadFollow("FOLLOW ME"));
	menu->AddItem(new MenuItemSquadHold("HOLD POSITION"));
	menu->AddItem(new MenuItemSquadGuardHere("GUARD HERE"));
	menu->AddItem(new MenuItemSquadToggleDefense("TOGGLE AUTO DEFENSE"));
	menu->AddItem(new MenuItemSquadFormation("CHANGE FORMATION"));
	return menu;
}

// =======================================
// GUARD SETTINGS MENU ITEM CLASSES
// =======================================
class MenuItemGuardHorseToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_guardHorseEnabled = !g_guardHorseEnabled;
		SetState(g_guardHorseEnabled);
		A11y::speak(g_guardHorseEnabled ? L"Guard horses enabled" : L"Guard horses disabled", true);
	}
public:
	MenuItemGuardHorseToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_guardHorseEnabled); }
};

class MenuItemGuardInvincibleToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_guardInvincibleEnabled = !g_guardInvincibleEnabled;
		SetState(g_guardInvincibleEnabled);
		// Apply to existing guards immediately
		if (g_activeSquad >= 0 && g_activeSquad < (int)g_squads.size()) {
			for (auto &gi : g_squads[g_activeSquad].guards) {
				if (gi.ped && ENTITY::DOES_ENTITY_EXIST(gi.ped) && !ENTITY::IS_ENTITY_DEAD(gi.ped)) {
					ENTITY::SET_ENTITY_INVINCIBLE(gi.ped, g_guardInvincibleEnabled);
					ENTITY::SET_ENTITY_CAN_BE_DAMAGED(gi.ped, !g_guardInvincibleEnabled);
				}
				if (gi.horse && ENTITY::DOES_ENTITY_EXIST(gi.horse) && !ENTITY::IS_ENTITY_DEAD(gi.horse)) {
					ENTITY::SET_ENTITY_INVINCIBLE(gi.horse, g_guardInvincibleEnabled);
					ENTITY::SET_ENTITY_CAN_BE_DAMAGED(gi.horse, !g_guardInvincibleEnabled);
				}
			}
		}
		A11y::speak(g_guardInvincibleEnabled ? L"Guards invincible" : L"Guards can be killed", true);
	}
public:
	MenuItemGuardInvincibleToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_guardInvincibleEnabled); }
};

MenuBase *CreateGuardSettingsMenu(MenuController *controller) {
	auto menu = new MenuBase(new MenuItemTitle("GUARD SETTINGS"));
	controller->RegisterMenu(menu);
	menu->AddItem(new MenuItemGuardHorseToggle("SPAWN WITH HORSE"));
	menu->AddItem(new MenuItemGuardInvincibleToggle("INVINCIBLE"));
	return menu;
}

MenuBase *CreateBodyguardMenu(MenuController *controller) {
	auto menu = new MenuBase(new MenuItemTitle("SQUAD MANAGER"));
	controller->RegisterMenu(menu);
	menu->AddItem(new MenuItemMenu("SPAWN GUARD", CreateSquadSpawnMenu(controller)));
	menu->AddItem(new MenuItemMenu("ORDERS", CreateSquadOrderMenu(controller)));
	menu->AddItem(new MenuItemMenu("GIVE WEAPON", CreateSquadWeaponMenu(controller)));
	menu->AddItem(new MenuItemMenu("SETTINGS", CreateGuardSettingsMenu(controller)));
	menu->AddItem(new MenuItemSquadDismiss("DISMISS ALL"));
	return menu;
}

// =======================================
// NUMPAD HOTKEYS FOR SQUAD CONTROL
// =======================================

static void HandleSquadHotkeys() {
	// Numpad 1: Squad status with type breakdown
	if (IsKeyJustUp(VK_NUMPAD1)) {
		CleanupDeadGuards();
		int count = GetTotalGuardCount();
		if (count == 0) {
			A11y::speak(L"No guards", true);
		} else {
			int alive = 0, fighting = 0;
			int humans = 0, wolves = 0, cougars = 0, panthers = 0;
			if (g_activeSquad >= 0 && g_activeSquad < (int)g_squads.size()) {
				for (auto &gi : g_squads[g_activeSquad].guards) {
					if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
					alive++;
					if (PED::IS_PED_IN_COMBAT(gi.ped, 0)) fighting++;
					Hash m = ENTITY::GET_ENTITY_MODEL(gi.ped);
					if (m == GAMEPLAY::GET_HASH_KEY("A_C_WOLF") || m == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_MEDIUM") || m == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_SMALL")) wolves++;
					else if (m == GAMEPLAY::GET_HASH_KEY("A_C_COUGAR_01")) cougars++;
					else if (m == GAMEPLAY::GET_HASH_KEY("A_C_PANTHER_01")) panthers++;
					else humans++;
				}
			}
			wchar_t buf[300];
			int pos = swprintf_s(buf, L"%d alive, %d fighting. ", alive, fighting);
			bool first = true;
			if (humans > 0) { pos += swprintf_s(buf + pos, 300 - pos, L"%d human%s", humans, humans > 1 ? L"s" : L""); first = false; }
			if (wolves > 0) { pos += swprintf_s(buf + pos, 300 - pos, L"%s%d %s", first ? L"" : L", ", wolves, wolves > 1 ? L"wolves" : L"wolf"); first = false; }
			if (cougars > 0) { pos += swprintf_s(buf + pos, 300 - pos, L"%s%d cougar%s", first ? L"" : L", ", cougars, cougars > 1 ? L"s" : L""); first = false; }
			if (panthers > 0) { pos += swprintf_s(buf + pos, 300 - pos, L"%s%d panther%s", first ? L"" : L", ", panthers, panthers > 1 ? L"s" : L""); first = false; }
			A11y::speak(buf, true);
		}
	}

	// Numpad 2: Teleport guards to player (regroup)
	if (IsKeyJustUp(VK_NUMPAD2)) {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) { A11y::speak(L"No squad", true); return; }
		Ped me = PLAYER::PLAYER_PED_ID();
		if (!ENTITY::DOES_ENTITY_EXIST(me)) { A11y::speak(L"No player", true); return; }
		int i = 0, applied = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			bool mounted = PED::IS_PED_ON_MOUNT(gi.ped) ? true : false;
			float ox = 0.0f, oy = 0.0f;
			GetFormationOffset(i, g_activeFormation, mounted, ox, oy); ++i;
			Vector3 to = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(me, ox, oy, 0.0f);
			if (mounted && gi.horse && ENTITY::DOES_ENTITY_EXIST(gi.horse))
				ENTITY::SET_ENTITY_COORDS(gi.horse, to.x, to.y, to.z, FALSE, FALSE, FALSE, TRUE);
			ENTITY::SET_ENTITY_COORDS(gi.ped, to.x, to.y, to.z, FALSE, FALSE, FALSE, TRUE);
			++applied;
		}
		g_activeSquadOrder = SquadOrder::Follow;
		wchar_t buf[64]; swprintf_s(buf, L"Regrouped %d", applied);
		A11y::speak(buf, true);
	}

	// Numpad 7: Follow me
	if (IsKeyJustUp(VK_NUMPAD7)) {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) { A11y::speak(L"No squad", true); return; }
		Ped me = PLAYER::PLAYER_PED_ID();
		int i = 0, applied = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			bool mounted = PED::IS_PED_ON_MOUNT(gi.ped) ? true : false;
			float ox = 0.0f, oy = 0.0f;
			GetFormationOffset(i, g_activeFormation, mounted, ox, oy); ++i;
			CommandGuardFollowOffset(gi, me, ox, oy);
			++applied;
		}
		g_activeSquadOrder = SquadOrder::Follow;
		wchar_t buf[64]; swprintf_s(buf, L"Follow %d", applied);
		A11y::speak(buf, true);
	}

	// Numpad 8: Hold position
	if (IsKeyJustUp(VK_NUMPAD8)) {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) { A11y::speak(L"No squad", true); return; }
		int applied = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			AI::CLEAR_PED_TASKS(gi.ped, TRUE, TRUE);
			AI::TASK_STAND_STILL(gi.ped, 30000);
			++applied;
		}
		g_activeSquadOrder = SquadOrder::Hold;
		wchar_t buf[64]; swprintf_s(buf, L"Hold %d", applied);
		A11y::speak(buf, true);
	}

	// Numpad 9: Guard here
	if (IsKeyJustUp(VK_NUMPAD9)) {
		if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) { A11y::speak(L"No squad", true); return; }
		Ped me = PLAYER::PLAYER_PED_ID();
		Vector3 c = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
		float baseH = ENTITY::GET_ENTITY_HEADING(me);
		static const float ring[7][2] = { {1.5f,0.0f}, {-1.5f,0.0f}, {0.0f,1.5f}, {0.0f,-1.5f}, {2.0f,0.8f}, {-2.0f,0.8f}, {0.0f,2.5f} };
		int applied = 0, idx = 0;
		for (auto &gi : g_squads[g_activeSquad].guards) {
			if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
			float dx = ring[idx % 7][0], dy = ring[idx % 7][1]; ++idx;
			AI::CLEAR_PED_TASKS(gi.ped, TRUE, TRUE);
			AI::TASK_STAND_GUARD(gi.ped, c.x + dx, c.y + dy, c.z, baseH, (char *)"WORLD_HUMAN_STAND_GUARD");
			++applied;
		}
		g_activeSquadOrder = SquadOrder::GuardHere;
		g_guardHerePos = c;
		wchar_t buf[64]; swprintf_s(buf, L"Guard %d", applied);
		A11y::speak(buf, true);
	}

	// Numpad 6: Toggle auto defense
	if (IsKeyJustUp(VK_NUMPAD6)) {
		g_autoDefenseEnabled = !g_autoDefenseEnabled;
		A11y::speak(g_autoDefenseEnabled ? L"Protection on" : L"Protection off", true);
	}

	// Numpad 0: Change formation
	if (IsKeyJustUp(VK_NUMPAD0)) {
		int f = (int)g_activeFormation;
		f = (f + 1) % 3;
		g_activeFormation = (GuardFormation)f;
		const wchar_t *names[] = { L"Column", L"Line", L"Wedge" };
		A11y::speak(names[f], true);
	}
}

// =======================================
// PAUSE MENU ACCESSIBILITY (with tab/item reading)
// =======================================

// RDR2 pause menu tabs in order (standard Story Mode layout)
static const wchar_t* g_pauseTabNames[] = {
	L"Map",         // Tab 0
	L"Journal",     // Tab 1
	L"Satchel",     // Tab 2
	L"Kit",         // Tab 3 (camp/items)
	L"Player",      // Tab 4
	L"Settings"     // Tab 5
};
static const int g_pauseTabCount = 6;
static int g_pauseTabIndex = 0;
static int g_pauseItemIndex = 0;
static DWORD g_lastPauseNavMs = 0;

// Settings sub-items (known structure)
static const wchar_t* g_settingsItems[] = {
	L"Display", L"Audio", L"Camera", L"Controls", L"General"
};
static const int g_settingsItemCount = 5;

// Satchel sub-items
static const wchar_t* g_satchelItems[] = {
	L"Provisions", L"Kit", L"Valuables", L"Documents", L"Ingredients"
};
static const int g_satchelItemCount = 5;

// Journal sub-items
static const wchar_t* g_journalItems[] = {
	L"Log", L"Objectives", L"Notes", L"Map Legend"
};
static const int g_journalItemCount = 4;

// Player sub-items
static const wchar_t* g_playerItems[] = {
	L"Arthur", L"Horse", L"Gang", L"General", L"Compendium"
};
static const int g_playerItemCount = 5;

// Kit sub-items
static const wchar_t* g_kitItems[] = {
	L"Camp", L"Items", L"Crafting"
};
static const int g_kitItemCount = 3;

static void SpeakPauseTab() {
	if (g_pauseTabIndex >= 0 && g_pauseTabIndex < g_pauseTabCount) {
		wchar_t buf[128];
		swprintf_s(buf, L"Tab: %s", g_pauseTabNames[g_pauseTabIndex]);
		A11y::speak(buf, true);
	}
}

static void GetPauseTabItems(const wchar_t*** outItems, int* outCount) {
	*outItems = nullptr;
	*outCount = 0;
	switch (g_pauseTabIndex) {
		case 1: *outItems = g_journalItems; *outCount = g_journalItemCount; break;
		case 2: *outItems = g_satchelItems; *outCount = g_satchelItemCount; break;
		case 3: *outItems = g_kitItems;     *outCount = g_kitItemCount;     break;
		case 4: *outItems = g_playerItems;  *outCount = g_playerItemCount;  break;
		case 5: *outItems = g_settingsItems; *outCount = g_settingsItemCount; break;
	}
}

static void SpeakPauseItem() {
	const wchar_t** items = nullptr;
	int count = 0;
	GetPauseTabItems(&items, &count);

	if (items && count > 0 && g_pauseItemIndex >= 0 && g_pauseItemIndex < count) {
		wchar_t buf[200];
		swprintf_s(buf, L"%s, item %d of %d", items[g_pauseItemIndex], g_pauseItemIndex + 1, count);
		A11y::speak(buf, true);
	} else if (g_pauseTabIndex == 0) {
		A11y::speak(L"Map. Use stick or arrows to navigate", true);
	}
}

static void HandlePauseMenuAccessibility() {
	// Tell ScriptHook to keep running behind pause menu
	GRAPHICS::SET_SCRIPT_GFX_DRAW_BEHIND_PAUSEMENU(TRUE);

	bool isPaused = UI::IS_PAUSE_MENU_ACTIVE() ? true : false;
	bool isSatchel = invoke<BOOL>(0x25B7A0206BDFAC76, 0xD4D3636C) ? true : false; // IS_UIAPP_ACTIVE_BY_HASH("satchel")
	bool isJournal = invoke<BOOL>(0x25B7A0206BDFAC76, 0xBC931A47) ? true : false; // IS_UIAPP_ACTIVE_BY_HASH("journal")

	ActiveMenuType currentMenu = MENU_NONE;
	if (isPaused) currentMenu = MENU_PAUSE;
	else if (isSatchel) currentMenu = MENU_SATCHEL;
	else if (isJournal) currentMenu = MENU_JOURNAL;

	// Transition: opened a menu
	if (currentMenu != MENU_NONE && g_activeMenuType == MENU_NONE) {
		g_activeMenuType = currentMenu;
		g_pauseTabIndex = 0;
		g_pauseItemIndex = 0;
		g_satchelCategoryIndex = 0;
		g_satchelItemIndex = 0;
		g_journalCategoryIndex = 0;
		g_journalItemIndex = 0;

		if (currentMenu == MENU_PAUSE) {
			A11y::speak(L"Pause menu. Tab: Map. Use Left Right arrows or Q E for tabs, Up Down for items, Escape to close", true);
		} else if (currentMenu == MENU_SATCHEL) {
			A11y::speak(L"Satchel. Category: Provisions. Use Left Right arrows or Q E for categories, Up Down for items, Escape to close", true);
		} else if (currentMenu == MENU_JOURNAL) {
			A11y::speak(L"Journal. Tab: Log. Use Left Right arrows or Q E for tabs, Up Down for items, Escape to close", true);
		}
		g_lastPauseNavMs = GetTickCount();
		return;
	}

	// Transition: closed the menu
	if (currentMenu == MENU_NONE && g_activeMenuType != MENU_NONE) {
		A11y::speak(L"Resumed", true);
		g_activeMenuType = MENU_NONE;
		return;
	}

	// While no menu is active, do nothing
	if (currentMenu == MENU_NONE) return;

	// If menu switched directly
	if (currentMenu != g_activeMenuType) {
		g_activeMenuType = currentMenu;
		g_pauseTabIndex = 0;
		g_pauseItemIndex = 0;
		g_satchelCategoryIndex = 0;
		g_satchelItemIndex = 0;
		g_journalCategoryIndex = 0;
		g_journalItemIndex = 0;
		if (currentMenu == MENU_PAUSE) A11y::speak(L"Pause menu. Tab: Map.", true);
		else if (currentMenu == MENU_SATCHEL) A11y::speak(L"Satchel. Category: Provisions.", true);
		else if (currentMenu == MENU_JOURNAL) A11y::speak(L"Journal. Tab: Log.", true);
		g_lastPauseNavMs = GetTickCount();
		return;
	}

	DWORD now = GetTickCount();
	// Debounce: 250ms between nav announcements
	if ((now - g_lastPauseNavMs) < 250) return;

	// Tab navigation: game frontend controls (LB=206, RB=207) + keyboard (Left/Right, Q/E)
	bool tabRight = IsKeyJustUp(VK_RIGHT) || IsKeyJustUp(0x45 /* E */)
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 207)  // RB
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 175);  // FRONTEND_RIGHT
	bool tabLeft = IsKeyJustUp(VK_LEFT) || IsKeyJustUp(0x51 /* Q */)
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 206)   // LB
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 174);  // FRONTEND_LEFT
	bool navUp = IsKeyJustUp(VK_UP)
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 172);  // FRONTEND_UP
	bool navDown = IsKeyJustUp(VK_DOWN)
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 173);  // FRONTEND_DOWN
	bool navAccept = IsKeyJustUp(VK_RETURN)
		|| CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 201);  // FRONTEND_ACCEPT
	bool reAnnounce = IsKeyJustUp(VK_TAB);

	if (g_activeMenuType == MENU_PAUSE) {
		if (tabRight) {
			g_pauseTabIndex = (g_pauseTabIndex + 1) % g_pauseTabCount;
			g_pauseItemIndex = 0;
			SpeakPauseTab();
			g_lastPauseNavMs = now;
		}
		else if (tabLeft) {
			g_pauseTabIndex = (g_pauseTabIndex - 1 + g_pauseTabCount) % g_pauseTabCount;
			g_pauseItemIndex = 0;
			SpeakPauseTab();
			g_lastPauseNavMs = now;
		}
		else if (navUp) {
			const wchar_t** items = nullptr;
			int count = 0;
			GetPauseTabItems(&items, &count);
			if (count > 0) {
				g_pauseItemIndex = (g_pauseItemIndex - 1 + count) % count;
				SpeakPauseItem();
			}
			g_lastPauseNavMs = now;
		}
		else if (navDown) {
			const wchar_t** items = nullptr;
			int count = 0;
			GetPauseTabItems(&items, &count);
			if (count > 0) {
				g_pauseItemIndex = (g_pauseItemIndex + 1) % count;
				SpeakPauseItem();
			}
			g_lastPauseNavMs = now;
		}
		else if (navAccept) {
			SpeakPauseItem();
			g_lastPauseNavMs = now;
		}
		else if (reAnnounce) {
			wchar_t buf[200];
			swprintf_s(buf, L"Tab: %s. Item %d", g_pauseTabNames[g_pauseTabIndex], g_pauseItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
	}
	else if (g_activeMenuType == MENU_SATCHEL) {
		if (tabRight) {
			g_satchelCategoryIndex = (g_satchelCategoryIndex + 1) % g_satchelCategoryCount;
			g_satchelItemIndex = 0;
			wchar_t buf[200];
			swprintf_s(buf, L"Category: %s", g_satchelCategories[g_satchelCategoryIndex]);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (tabLeft) {
			g_satchelCategoryIndex = (g_satchelCategoryIndex - 1 + g_satchelCategoryCount) % g_satchelCategoryCount;
			g_satchelItemIndex = 0;
			wchar_t buf[200];
			swprintf_s(buf, L"Category: %s", g_satchelCategories[g_satchelCategoryIndex]);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (navUp) {
			if (g_satchelItemIndex > 0) {
				g_satchelItemIndex--;
			}
			wchar_t buf[200];
			swprintf_s(buf, L"Item %d", g_satchelItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (navDown) {
			g_satchelItemIndex++;
			wchar_t buf[200];
			swprintf_s(buf, L"Item %d", g_satchelItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (navAccept) {
			wchar_t buf[200];
			swprintf_s(buf, L"Selected item %d in %s", g_satchelItemIndex + 1, g_satchelCategories[g_satchelCategoryIndex]);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (reAnnounce) {
			wchar_t buf[200];
			swprintf_s(buf, L"Satchel Category: %s. Item %d", g_satchelCategories[g_satchelCategoryIndex], g_satchelItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
	}
	else if (g_activeMenuType == MENU_JOURNAL) {
		if (tabRight) {
			g_journalCategoryIndex = (g_journalCategoryIndex + 1) % g_journalCategoryCount;
			g_journalItemIndex = 0;
			wchar_t buf[200];
			swprintf_s(buf, L"Journal tab: %s", g_journalCategories[g_journalCategoryIndex]);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (tabLeft) {
			g_journalCategoryIndex = (g_journalCategoryIndex - 1 + g_journalCategoryCount) % g_journalCategoryCount;
			g_journalItemIndex = 0;
			wchar_t buf[200];
			swprintf_s(buf, L"Journal tab: %s", g_journalCategories[g_journalCategoryIndex]);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (navUp) {
			if (g_journalItemIndex > 0) {
				g_journalItemIndex--;
			}
			wchar_t buf[200];
			swprintf_s(buf, L"Item %d", g_journalItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (navDown) {
			g_journalItemIndex++;
			wchar_t buf[200];
			swprintf_s(buf, L"Item %d", g_journalItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (navAccept) {
			wchar_t buf[200];
			swprintf_s(buf, L"Selected item %d in %s", g_journalItemIndex + 1, g_journalCategories[g_journalCategoryIndex]);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
		else if (reAnnounce) {
			wchar_t buf[200];
			swprintf_s(buf, L"Journal Tab: %s. Item %d", g_journalCategories[g_journalCategoryIndex], g_journalItemIndex + 1);
			A11y::speak(buf, true);
			g_lastPauseNavMs = now;
		}
	}
}

// =======================================
// WANTED / BOUNTY ANNOUNCEMENTS
// =======================================

static bool g_wasWanted = false;
static DWORD g_lastWantedCheckMs = 0;
static int g_lastBountyAnnounced = -1;

static void HandleWantedAnnouncements() {
	DWORD now = GetTickCount();
	// Check every 2 seconds to avoid spam
	if ((now - g_lastWantedCheckMs) < 2000) return;
	g_lastWantedCheckMs = now;

	Player player = PLAYER::PLAYER_ID();
	int wantedLevel = PLAYER::GET_PLAYER_WANTED_LEVEL(player);
	int bounty = PURSUIT::GET_PLAYER_PRICE_ON_A_HEAD(player);
	int intensity = PURSUIT::GET_PLAYER_WANTED_INTENSITY(player);

	bool isWantedNow = (wantedLevel > 0 || bounty > 0 || intensity > 0);

	// Transition: became wanted
	if (isWantedNow && !g_wasWanted) {
		wchar_t buf[128];
		int bountyDollars = bounty / 100;  // RDR2 stores in cents
		if (bountyDollars > 0)
			swprintf_s(buf, L"Wanted! Bounty: %d dollars", bountyDollars);
		else
			swprintf_s(buf, L"Wanted!");
		A11y::speak(buf, true);
		g_wasWanted = true;
		g_lastBountyAnnounced = bountyDollars;
	}
	// Still wanted: announce bounty changes
	else if (isWantedNow && g_wasWanted) {
		int bountyDollars = bounty / 100;
		if (bountyDollars != g_lastBountyAnnounced && bountyDollars > 0) {
			wchar_t buf[128];
			swprintf_s(buf, L"Bounty: %d dollars", bountyDollars);
			A11y::speak(buf, false);  // don't interrupt other speech
			g_lastBountyAnnounced = bountyDollars;
		}
	}
	// Transition: no longer wanted
	else if (!isWantedNow && g_wasWanted) {
		A11y::speak(L"No longer wanted", true);
		g_wasWanted = false;
		g_lastBountyAnnounced = -1;
	}
}

// =======================================
// COMPASS DIRECTION HELPER
// =======================================
static const wchar_t* GetCompassDirection(float heading) {
	// heading: 0=North, 90=West, 180=South, 270=East (RDR2 uses counter-clockwise)
	if (heading < 0) heading += 360.0f;
	if (heading >= 360.0f) heading -= 360.0f;
	if (heading >= 337.5f || heading < 22.5f)  return L"North";
	if (heading >= 22.5f && heading < 67.5f)   return L"North West";
	if (heading >= 67.5f && heading < 112.5f)  return L"West";
	if (heading >= 112.5f && heading < 157.5f) return L"South West";
	if (heading >= 157.5f && heading < 202.5f) return L"South";
	if (heading >= 202.5f && heading < 247.5f) return L"South East";
	if (heading >= 247.5f && heading < 292.5f) return L"East";
	if (heading >= 292.5f && heading < 337.5f) return L"North East";
	return L"Unknown";
}

static int GetCompassDirIndex(float heading) {
	if (heading < 0) heading += 360.0f;
	if (heading >= 360.0f) heading -= 360.0f;
	if (heading >= 337.5f || heading < 22.5f)  return 0; // N
	if (heading >= 22.5f && heading < 67.5f)   return 1; // NW
	if (heading >= 67.5f && heading < 112.5f)  return 2; // W
	if (heading >= 112.5f && heading < 157.5f) return 3; // SW
	if (heading >= 157.5f && heading < 202.5f) return 4; // S
	if (heading >= 202.5f && heading < 247.5f) return 5; // SE
	if (heading >= 247.5f && heading < 292.5f) return 6; // E
	if (heading >= 292.5f && heading < 337.5f) return 7; // NE
	return -1;
}

// =======================================
// LOCATION / ZONE LOOKUP
// =======================================
struct ZoneEntry { Hash hash; const wchar_t* name; };

static const ZoneEntry g_knownZones[] = {
	// States
	{ 0, L"" }, // placeholder
	// Towns
};

// Get location name at coordinates using _GET_MAP_ZONE_AT_COORDS
static const wchar_t* GetLocationName(float x, float y, float z) {
	// Try town first (type 1), then district (10), then state via hash matching
	static const struct { int type; const wchar_t* prefix; } zoneTypes[] = {
		{ 1, L"" },   // Town
		{ 10, L"" },  // District
	};

	// Known zone hash-to-name mapping (comprehensive RDR2 map)
	static const struct { const char* key; const wchar_t* name; } zoneNames[] = {
		// Towns
		{"valentine", L"Valentine"}, {"Blackwater", L"Blackwater"}, {"StDenis", L"Saint Denis"},
		{"Rhodes", L"Rhodes"}, {"Strawberry", L"Strawberry"}, {"Annesburg", L"Annesburg"},
		{"VANHORN", L"Van Horn"}, {"Armadillo", L"Armadillo"}, {"Tumbleweed", L"Tumbleweed"},
		{"lagras", L"Lagras"}, {"cornwall", L"Cornwall"}, {"Emerald", L"Emerald Station"},
		{"Butcher", L"Butcher Creek"}, {"wallace", L"Wallace Station"},
		{"wapiti", L"Wapiti"}, {"Manzanita", L"Manzanita Post"}, {"Siska", L"Sisika"},
		{"BeechersHope", L"Beechers Hope"}, {"Braithwaite", L"Braithwaite Manor"},
		{"Caliga", L"Caliga Hall"}, {"Manicato", L"Manicato"},
		// Districts / Regions
		{"Heartlands", L"Heartlands"}, {"bigvalley", L"Big Valley"},
		{"BluewaterMarsh", L"Bluewater Marsh"}, {"ChollaSpringsDLC", L"Cholla Springs"},
		{"ChollaSpringsMercer", L"Cholla Springs"}, {"ChollaSpringsSouth", L"Cholla Springs"},
		{"Cumberland", L"Cumberland Forest"}, {"GaptoothRidge", L"Gaptooth Ridge"},
		{"greatPlains", L"Great Plains"}, {"GrizzliesEast", L"Grizzlies East"},
		{"GrizzliesWest", L"Grizzlies West"}, {"HennigansSteadDLC", L"Hennigans Stead"},
		{"HennigansSteadSouth", L"Hennigans Stead"}, {"Perdido", L"Perdido"},
		{"PuntaOrgullo", L"Punta Orgullo"}, {"RioBravo", L"Rio Bravo"},
		{"roanoke", L"Roanoke Ridge"}, {"scarletMeadows", L"Scarlett Meadows"},
		{"TallTrees", L"Tall Trees"}, {"BayouNwa", L"Bayou Nwa"},
		{"DiezCoronas", L"Diez Coronas"}, {"GuarmaD", L"Guarma"},
		// States
		{"Ambarino", L"Ambarino"}, {"Lemoyne", L"Lemoyne"},
		{"NewAustin", L"New Austin"}, {"NewHanover", L"New Hanover"},
		{"WestElizabeth", L"West Elizabeth"}, {"LowerWestElizabeth", L"West Elizabeth"},
		{"UpperWestElizabeth", L"West Elizabeth"}, {"Guarma", L"Guarma"},
		{"NuevoParaiso", L"Nuevo Paraiso"},
	};
	const int numZones = sizeof(zoneNames) / sizeof(zoneNames[0]);

	// Try town (1) then district (10)
	for (int typeIdx = 0; typeIdx < 2; typeIdx++) {
		int zoneType = (typeIdx == 0) ? 1 : 10;
		Hash zoneHash = invoke<Hash>(0x43AD8FC02B429D33, x, y, z, zoneType);
		if (zoneHash == 0) continue;
		for (int i = 0; i < numZones; i++) {
			Hash knownHash = GAMEPLAY::GET_HASH_KEY(const_cast<char*>(zoneNames[i].key));
			if (zoneHash == knownHash) return zoneNames[i].name;
		}
	}
	return L"Wilderness";
}

// Get full location string: "Valentine, facing North"
static void GetFullLocationString(wchar_t* buf, int bufSize) {
	Ped pp = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(pp)) { swprintf_s(buf, bufSize, L"Unknown"); return; }
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(pp, TRUE, FALSE);
	float heading = ENTITY::GET_ENTITY_HEADING(pp);
	const wchar_t* location = GetLocationName(pos.x, pos.y, pos.z);
	const wchar_t* compass = GetCompassDirection(heading);
	swprintf_s(buf, bufSize, L"%s, facing %s", location, compass);
}

// =======================================
// GLOBAL MODE HOTKEYS
// =======================================

static void HandleGlobalHotkeys() {
	// Numpad 3: Current weapon
	if (IsKeyJustUp(VK_NUMPAD3)) {
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) { A11y::speak(L"No player", true); return; }
		Hash weaponHash = 0;
		WEAPON::GET_CURRENT_PED_WEAPON(playerPed, &weaponHash, TRUE, 0, FALSE);
		const wchar_t* weaponName = GetWeaponName(weaponHash);

		// Get ammo info
		Hash ammoType = WEAPON::GET_PED_AMMO_TYPE_FROM_WEAPON(playerPed, weaponHash);
		int ammo = WEAPON::GET_PED_AMMO_BY_TYPE(playerPed, ammoType);

		wchar_t buf[200];
		if (weaponHash == 0xD3BD8255) // unarmed
			swprintf_s(buf, L"Unarmed");
		else
			swprintf_s(buf, L"%s. Ammo: %d", weaponName, ammo);
		A11y::speak(buf, true);
	}

	// Numpad 7: Wanted status
	if (IsKeyJustUp(VK_NUMPAD7)) {
		Player player = PLAYER::PLAYER_ID();
		int wantedLevel = PLAYER::GET_PLAYER_WANTED_LEVEL(player);
		int bounty = PURSUIT::GET_PLAYER_PRICE_ON_A_HEAD(player) / 100;
		int intensity = PURSUIT::GET_PLAYER_WANTED_INTENSITY(player);

		wchar_t buf[200];
		if (wantedLevel > 0 || bounty > 0 || intensity > 0) {
			swprintf_s(buf, L"Wanted level %d. Bounty %d dollars. Intensity %d", wantedLevel, bounty, intensity);
		} else {
			swprintf_s(buf, L"Not wanted. No bounty.");
		}
		A11y::speak(buf, true);
	}

	// Numpad 4: Player status (health core, stamina core, deadeye core, money)
	if (IsKeyJustUp(VK_NUMPAD4)) {
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) { A11y::speak(L"No player", true); return; }

		// Cores: 0=Health, 1=Stamina, 2=DeadEye
		// _GET_ATTRIBUTE_CORE_VALUE hash: 0x36731AC041289BB1
		int healthCore  = invoke<int>(0x36731AC041289BB1, playerPed, 0);
		int staminaCore = invoke<int>(0x36731AC041289BB1, playerPed, 1);
		int deadeyeCore = invoke<int>(0x36731AC041289BB1, playerPed, 2);

		// Current health bar
		int hp    = ENTITY::GET_ENTITY_HEALTH(playerPed);
		int maxHp = ENTITY::GET_ENTITY_MAX_HEALTH(playerPed, FALSE);
		int hpPct = (maxHp > 0) ? (hp * 100 / maxHp) : 0;

		// Money: _MONEY_GET_CASH_BALANCE hash: 0x0C02DABFA3B98176
		int moneyCents = invoke<int>(0x0C02DABFA3B98176);
		int dollars = moneyCents / 100;
		int cents = moneyCents % 100;

		wchar_t buf[300];
		swprintf_s(buf, L"Health %d%%. Health core %d. Stamina core %d. Deadeye core %d. Money %d dollars %d cents",
			hpPct, healthCore, staminaCore, deadeyeCore, dollars, cents);
		A11y::speak(buf, true);
	}

	// Numpad 6: Game time + stamina/deadeye bars
	if (IsKeyJustUp(VK_NUMPAD6)) {
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		int hours = TIME::GET_CLOCK_HOURS();
		int minutes = TIME::GET_CLOCK_MINUTES();

		// Stamina bar percentage
		int stamPts = ATTRIBUTE::_0x219DA04BAA9CB065(playerPed, 1);
		int stamMax = ATTRIBUTE::_0x223BF310F854871C(playerPed, 1);
		int stamPct = (stamMax > 0) ? (stamPts * 100 / stamMax) : 0;

		// Deadeye bar percentage
		int dePts = ATTRIBUTE::_0x219DA04BAA9CB065(playerPed, 2);
		int deMax = ATTRIBUTE::_0x223BF310F854871C(playerPed, 2);
		int dePct = (deMax > 0) ? (dePts * 100 / deMax) : 0;

		// Bounty check
		Player player = PLAYER::PLAYER_ID();
		int bounty = PURSUIT::GET_PLAYER_PRICE_ON_A_HEAD(player) / 100;

		const wchar_t* period = (hours >= 6 && hours < 12) ? L"Morning" :
		                        (hours >= 12 && hours < 18) ? L"Afternoon" :
		                        (hours >= 18 && hours < 21) ? L"Evening" : L"Night";

		Hash wHash = invoke<Hash>(0x4BEB42AEBCA732E9); // _GET_PREV_WEATHER_TYPE_HASH_NAME
		const wchar_t* weather = L"Unknown weather";
		switch(wHash) {
			case 0x060a3b31: weather = L"Sunny"; break;
			case 0xc51d9240: weather = L"Rainy"; break;
			case 0x6eaf37f6: weather = L"Snowing"; break;
			case 0x252b5e01: weather = L"Foggy"; break;
			case 0x710ce29d: weather = L"Cloudy"; break;
			case 0x87b83ee1: weather = L"Overcast"; break;
			case 0xc819fc6e: weather = L"Thunderstorm"; break;
			case 0x4ae43128: weather = L"Drizzling"; break;
			case 0x5248b05b: weather = L"Blizzard"; break;
			case 0x1929f317: weather = L"Hurricane"; break;
			case 0x000f1668: weather = L"Hail"; break;
			case 0xe619af8c: weather = L"Light Snow"; break;
			case 0xdb2be23e: weather = L"Clear Snow"; break;
			case 0x93fd33ac: weather = L"Misty"; break;
			case 0x0a19ffeb: weather = L"Sandstorm"; break;
		}

		wchar_t buf[300];
		swprintf_s(buf, L"%s, %d:%02d, %s", period, hours, minutes, weather);
		A11y::speak(buf, true);
	}

	// Numpad 0: Current location and direction
	if (IsKeyJustUp(VK_NUMPAD0)) {
		wchar_t buf[300];
		GetFullLocationString(buf, 300);
		A11y::speak(buf, true);
	}

	// Numpad 9: Player honor
	if (IsKeyJustUp(VK_NUMPAD9)) {
		struct StatId {
			ALIGN8 Hash BaseId;
			ALIGN8 Hash PermId;
		} statId;
		statId.PermId = 0;

		int honorVal = 0;
		bool success = false;

		// Try joaat("honor_current") = 0x89587b4e
		statId.BaseId = 0x89587b4e;
		if (invoke<BOOL>(0x767FBC2AC802EF3E, &statId, &honorVal)) {
			DebugLog::log("Player honor queried with joaat(honor_current) 0x89587b4e: %d", honorVal);
			success = true;
		} else {
			// Try fallback 0x7C045E1B
			statId.BaseId = 0x7C045E1B;
			if (invoke<BOOL>(0x767FBC2AC802EF3E, &statId, &honorVal)) {
				DebugLog::log("Player honor queried with fallback 0x7C045E1B: %d", honorVal);
				success = true;
			}
		}

		if (success) {
			const wchar_t* level = L"Neutral";
			if (honorVal > 240)       level = L"Very High Honor";
			else if (honorVal > 80)   level = L"High Honor";
			else if (honorVal > 20)   level = L"Good Honor";
			else if (honorVal >= -20) level = L"Neutral";
			else if (honorVal >= -80) level = L"Low Honor";
			else if (honorVal >= -240) level = L"Bad Honor";
			else                      level = L"Very Bad Honor";

			wchar_t buf[200];
			swprintf_s(buf, L"Honor level %s. Value %d", level, honorVal);
			A11y::speak(buf, true);
		} else {
			DebugLog::log("Failed to query player honor stat");
			A11y::speak(L"Failed to read honor", true);
		}
	}
}

// =======================================
// HORSE MODE HOTKEYS
// =======================================

static void HandleHorseHotkeys() {
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

	// Try to get horse: mounted first, then last mount
	Ped horse = 0;
	if (PED::IS_PED_ON_MOUNT(playerPed)) {
		horse = PED::GET_MOUNT(playerPed);
	} else {
		horse = GetPlayerHorse(playerPed);
	}

	bool hasHorse = (horse != 0 && ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse));

	// Numpad 1: Horse name, breed, and bonding level
	if (IsKeyJustUp(VK_NUMPAD1)) {
		if (!hasHorse) { A11y::speak(L"No horse nearby", true); return; }
		Hash model = ENTITY::GET_ENTITY_MODEL(horse);
		const wchar_t* horseName = GetHorseName(model);
		// GET_ATTRIBUTE_RANK (PA_BONDING = 7) - returns bonding level 1-4
		int bondLevel = invoke<int>(0xA4C8E23E29040DE0, horse, 7);
		if (bondLevel < 0) bondLevel = 0;
		if (bondLevel > 4) bondLevel = 4;
		
		// Distance from player
		Vector3 pPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
		Vector3 hPos = ENTITY::GET_ENTITY_COORDS(horse, TRUE, FALSE);
		float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
		
		wchar_t buf[300];
		if (PED::IS_PED_ON_MOUNT(playerPed))
			swprintf_s(buf, L"Mounted: %s. Bonding level %d", horseName, bondLevel);
		else
			swprintf_s(buf, L"Horse: %s. Bonding level %d. Distance %.0f meters", horseName, bondLevel, dist);
		A11y::speak(buf, true);
	}

	// Numpad 4: Horse energy / stamina core
	if (IsKeyJustUp(VK_NUMPAD4)) {
		if (!hasHorse) { A11y::speak(L"No horse nearby", true); return; }

		int hp    = ENTITY::GET_ENTITY_HEALTH(horse);
		int maxHp = ENTITY::GET_ENTITY_MAX_HEALTH(horse, FALSE);
		int hpPct = (maxHp > 0) ? (hp * 100 / maxHp) : 0;

		// Horse cores: 0=Health, 1=Stamina
		int healthCore  = invoke<int>(0x36731AC041289BB1, horse, 0);
		int staminaCore = invoke<int>(0x36731AC041289BB1, horse, 1);

		wchar_t buf[200];
		if (staminaCore < 25)
			swprintf_s(buf, L"Horse needs food! Health %d%%. Health core %d. Stamina core %d, low", hpPct, healthCore, staminaCore);
		else
			swprintf_s(buf, L"Horse health %d%%. Health core %d. Stamina core %d", hpPct, healthCore, staminaCore);
		A11y::speak(buf, true);
	}

	// Numpad 2: Horse movement state and speed
	if (IsKeyJustUp(VK_NUMPAD2)) {
		if (!hasHorse) { A11y::speak(L"No horse nearby", true); return; }

		float speed = ENTITY::GET_ENTITY_SPEED(horse);
		wchar_t buf[128];
		if (speed < 0.5f)
			swprintf_s(buf, L"Horse idle");
		else if (speed < 3.0f)
			swprintf_s(buf, L"Horse walking, speed %.0f", speed);
		else if (speed < 7.0f)
			swprintf_s(buf, L"Horse trotting, speed %.0f", speed);
		else if (speed < 12.0f)
			swprintf_s(buf, L"Horse cantering, speed %.0f", speed);
		else
			swprintf_s(buf, L"Horse galloping, speed %.0f", speed);
		A11y::speak(buf, true);
	}

	// Numpad 3: Horse stamina bar and overall condition
	if (IsKeyJustUp(VK_NUMPAD3)) {
		if (!hasHorse) { A11y::speak(L"No horse nearby", true); return; }

		// Stamina bar (attribute points index 1)
		int stamPts = ATTRIBUTE::_0x219DA04BAA9CB065(horse, 1);
		int stamMax = ATTRIBUTE::_0x223BF310F854871C(horse, 1);
		int stamPct = (stamMax > 0) ? (stamPts * 100 / stamMax) : 0;

		// Health bar (attribute points index 0)
		int hpPts = ATTRIBUTE::_0x219DA04BAA9CB065(horse, 0);
		int hpMax = ATTRIBUTE::_0x223BF310F854871C(horse, 0);
		int hpPct = (hpMax > 0) ? (hpPts * 100 / hpMax) : 0;

		wchar_t buf[200];
		if (stamPct < 25)
			swprintf_s(buf, L"Horse tired! Stamina bar %d%%. Health bar %d%%", stamPct, hpPct);
		else
			swprintf_s(buf, L"Horse stamina bar %d%%. Health bar %d%%", stamPct, hpPct);
		A11y::speak(buf, true);
	}

	// Numpad 0: Horse whistle / distance check
	if (IsKeyJustUp(VK_NUMPAD0)) {
		Ped pp = PLAYER::PLAYER_PED_ID();
		if (!ENTITY::DOES_ENTITY_EXIST(pp)) return;
		bool isMounted = PED::IS_PED_ON_MOUNT(pp) ? true : false;
		if (isMounted) {
			A11y::speak(L"Already mounted", true);
			return;
		}
		Ped wHorse = invoke<Ped>(0x4C8B59171957BCF7, pp);
		if (wHorse && ENTITY::DOES_ENTITY_EXIST(wHorse) && !ENTITY::IS_ENTITY_DEAD(wHorse)) {
			Vector3 pPos = ENTITY::GET_ENTITY_COORDS(pp, TRUE, FALSE);
			Vector3 hPos = ENTITY::GET_ENTITY_COORDS(wHorse, TRUE, FALSE);
			float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
			wchar_t buf[200];
			if (dist > 200.0f)
				swprintf_s(buf, L"Horse too far! %.0f meters, out of range", dist);
			else if (dist > 80.0f)
				swprintf_s(buf, L"Horse called. %.0f meters, coming", dist);
			else if (dist > 20.0f)
				swprintf_s(buf, L"Horse called. %.0f meters, approaching", dist);
			else
				swprintf_s(buf, L"Horse nearby. %.0f meters", dist);
			A11y::speak(buf, true);
			g_horseWhistleActive = true;
			g_horseWhistleStartMs = GetTickCount();
			g_whistleTargetHorse = wHorse;
		} else {
			A11y::speak(L"No horse available", true);
		}
	}
}



// =======================================
// UNIFIED NUMPAD MODE HANDLER
// =======================================

static void HandleNumpadModes() {
	// Numpad 5: Switch mode
	if (IsKeyJustUp(VK_NUMPAD5)) {
		int m = (int)g_currentNumpadMode;
		m = (m + 1) % 3;
		g_currentNumpadMode = (NumpadMode)m;
		A11y::speak(GetModeName(g_currentNumpadMode), true);
		return; // consume the key
	}

	// Dispatch to active mode
	switch (g_currentNumpadMode) {
		case NumpadMode::Bodyguard:
			HandleSquadHotkeys();
			break;
		case NumpadMode::Global:
			HandleGlobalHotkeys();
			break;
		case NumpadMode::Horse:
			HandleHorseHotkeys();
			break;
	}
}

// =======================================
// END OF SQUAD SYSTEM
// =======================================

// =======================================
// ACCESSIBILITY MENU TOGGLE CLASSES
// =======================================
class MenuItemAutoMoneyToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoMoneyAnnounce = !g_autoMoneyAnnounce;
		SetState(g_autoMoneyAnnounce);
		A11y::speak(g_autoMoneyAnnounce ? L"Money announcements on" : L"Money announcements off", true);
	}
public:
	MenuItemAutoMoneyToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoMoneyAnnounce); }
};

class MenuItemAutoHorseStateToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoHorseStateAnnounce = !g_autoHorseStateAnnounce;
		SetState(g_autoHorseStateAnnounce);
		A11y::speak(g_autoHorseStateAnnounce ? L"Horse state announcements on" : L"Horse state announcements off", true);
	}
public:
	MenuItemAutoHorseStateToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoHorseStateAnnounce); }
};

class MenuItemAutoGuardStatusToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoGuardStatusAnnounce = !g_autoGuardStatusAnnounce;
		SetState(g_autoGuardStatusAnnounce);
		A11y::speak(g_autoGuardStatusAnnounce ? L"Guard status announcements on" : L"Guard status announcements off", true);
	}
public:
	MenuItemAutoGuardStatusToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoGuardStatusAnnounce); }
};

class MenuItemAutoHorseCallToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoHorseCallAnnounce = !g_autoHorseCallAnnounce;
		SetState(g_autoHorseCallAnnounce);
		A11y::speak(g_autoHorseCallAnnounce ? L"Horse call announcements on" : L"Horse call announcements off", true);
	}
public:
	MenuItemAutoHorseCallToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoHorseCallAnnounce); }
};

class MenuItemAutoCompassToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoCompassAnnounce = !g_autoCompassAnnounce;
		SetState(g_autoCompassAnnounce);
		A11y::speak(g_autoCompassAnnounce ? L"Compass announcements on" : L"Compass announcements off", true);
	}
public:
	MenuItemAutoCompassToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoCompassAnnounce); }
};

class MenuItemAutoAnimalBehaviorToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoAnimalBehaviorAnnounce = !g_autoAnimalBehaviorAnnounce;
		SetState(g_autoAnimalBehaviorAnnounce);
		A11y::speak(g_autoAnimalBehaviorAnnounce ? L"Animal behavior announcements on" : L"Animal behavior announcements off", true);
	}
public:
	MenuItemAutoAnimalBehaviorToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoAnimalBehaviorAnnounce); }
};

class MenuItemAutoHonorToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoHonorAnnounce = !g_autoHonorAnnounce;
		SetState(g_autoHonorAnnounce);
		A11y::speak(g_autoHonorAnnounce ? L"Honor announcements on" : L"Honor announcements off", true);
	}
public:
	MenuItemAutoHonorToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoHonorAnnounce); }
};

class MenuItemAutoLootToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoLootAnnounce = !g_autoLootAnnounce;
		SetState(g_autoLootAnnounce);
		A11y::speak(g_autoLootAnnounce ? L"Loot announcements on" : L"Loot announcements off", true);
	}
public:
	MenuItemAutoLootToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoLootAnnounce); }
};

class MenuItemAutoAimToggle : public MenuItemSwitchable
{
	virtual void OnSelect() {
		g_autoAimEnabled = !g_autoAimEnabled;
		SetState(g_autoAimEnabled);
		if (g_autoAimEnabled) {
			PLAYER::SET_PLAYER_LOCKON(PLAYER::PLAYER_ID(), TRUE);
			PLAYER::SET_PLAYER_LOCKON_RANGE_OVERRIDE(PLAYER::PLAYER_ID(), 150.0f);
		}
		A11y::speak(g_autoAimEnabled ? L"Auto aim on" : L"Auto aim off", true);
	}
public:
	MenuItemAutoAimToggle(string caption) : MenuItemSwitchable(caption) { SetState(g_autoAimEnabled); }
};



MenuBase *CreateAccessibilityMenu(MenuController *controller) {
	auto menu = new MenuBase(new MenuItemTitle("ACCESSIBILITY"));
	controller->RegisterMenu(menu);
	menu->AddItem(new MenuItemAutoAimToggle("AUTO AIM"));
	menu->AddItem(new MenuItemAutoMoneyToggle("AUTO MONEY ANNOUNCE"));
	menu->AddItem(new MenuItemAutoHorseStateToggle("AUTO HORSE STATE"));
	menu->AddItem(new MenuItemAutoGuardStatusToggle("AUTO GUARD STATUS"));
	menu->AddItem(new MenuItemAutoHorseCallToggle("AUTO HORSE CALL"));
	menu->AddItem(new MenuItemAutoCompassToggle("AUTO COMPASS"));
	menu->AddItem(new MenuItemAutoAnimalBehaviorToggle("AUTO ANIMAL BEHAVIOR"));
	menu->AddItem(new MenuItemAutoHonorToggle("AUTO HONOR ANNOUNCE"));
	menu->AddItem(new MenuItemAutoLootToggle("AUTO LOOT ANNOUNCE"));
	return menu;
}

// =======================================
// AUTO-ANNOUNCEMENT FUNCTIONS
// =======================================

static void HandleAutoMoneyAnnounce() {
	if (!g_autoMoneyAnnounce) return;
	DWORD now = GetTickCount();
	if ((now - g_lastMoneyCheckMs) < 1500) return;
	g_lastMoneyCheckMs = now;

	int currentCents = invoke<int>(0x0C02DABFA3B98176); // _MONEY_GET_CASH_BALANCE
	if (g_lastMoneyCents < 0) { g_lastMoneyCents = currentCents; return; } // first run init

	int diff = currentCents - g_lastMoneyCents;
	if (diff == 0) return;

	int absDollars = (diff > 0 ? diff : -diff) / 100;
	int absCents = (diff > 0 ? diff : -diff) % 100;
	int totalDollars = currentCents / 100;

	if (absDollars == 0 && absCents == 0) { g_lastMoneyCents = currentCents; return; }

	wchar_t buf[256];
	if (diff > 0) {
		if (absCents > 0)
			swprintf_s(buf, L"Earned %d dollars %d cents. Total %d dollars", absDollars, absCents, totalDollars);
		else
			swprintf_s(buf, L"Earned %d dollars. Total %d dollars", absDollars, totalDollars);
	} else {
		if (absCents > 0)
			swprintf_s(buf, L"Spent %d dollars %d cents. Total %d dollars", absDollars, absCents, totalDollars);
		else
			swprintf_s(buf, L"Spent %d dollars. Total %d dollars", absDollars, totalDollars);
	}
	A11y::speak(buf, false);
	g_lastMoneyCents = currentCents;
}

static void HandleAutoHorseStateAnnounce() {
	if (!g_autoHorseStateAnnounce) return;
	DWORD now = GetTickCount();
	if ((now - g_lastHorseStateCheckMs) < 1000) return;
	g_lastHorseStateCheckMs = now;

	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;
	if (!PED::IS_PED_ON_MOUNT(playerPed)) {
		g_lastHorseStateId = -1; // reset when not mounted
		return;
	}

	Ped horse = PED::GET_MOUNT(playerPed);
	if (!horse || !ENTITY::DOES_ENTITY_EXIST(horse)) return;

	float speed = ENTITY::GET_ENTITY_SPEED(horse);
	int stateId;
	const wchar_t* stateName;
	if (speed < 0.5f)       { stateId = 0; stateName = L"Horse idle"; }
	else if (speed < 3.0f)  { stateId = 1; stateName = L"Horse walking"; }
	else if (speed < 7.0f)  { stateId = 2; stateName = L"Horse trotting"; }
	else if (speed < 12.0f) { stateId = 3; stateName = L"Horse cantering"; }
	else                    { stateId = 4; stateName = L"Horse galloping"; }

	if (stateId != g_lastHorseStateId) {
		A11y::speak(stateName, false);
		g_lastHorseStateId = stateId;
	}
}

static void HandleAutoGuardStatusAnnounce() {
	if (!g_autoGuardStatusAnnounce) return;
	if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) return;
	DWORD now = GetTickCount();
	if ((now - g_lastGuardAutoStatusMs) < 3000) return;
	g_lastGuardAutoStatusMs = now;

	int alive = 0, fighting = 0;
	for (auto &gi : g_squads[g_activeSquad].guards) {
		if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
		alive++;
		if (PED::IS_PED_IN_COMBAT(gi.ped, 0)) fighting++;
	}

	// Only announce changes
	if (alive == g_lastAliveCount && fighting == g_lastFightingCount) return;

	// Detect significant events
	if (g_lastAliveCount >= 0) {
		if (fighting > 0 && g_lastFightingCount == 0) {
			wchar_t buf[128];
			swprintf_s(buf, L"Guards in combat! %d fighting", fighting);
			A11y::speak(buf, true);
		} else if (fighting == 0 && g_lastFightingCount > 0) {
			A11y::speak(L"Guards combat ended", false);
		} else if (alive < g_lastAliveCount) {
			wchar_t buf[128];
			swprintf_s(buf, L"Guard down! %d remaining", alive);
			A11y::speak(buf, true);
		}
	}

	g_lastAliveCount = alive;
	g_lastFightingCount = fighting;
}

// =======================================
// ANIMAL GUARD BEHAVIOR ANNOUNCEMENTS
// =======================================

// Determine animal type from model hash
static const wchar_t* GetAnimalTypeName(Ped ped) {
	Hash model = ENTITY::GET_ENTITY_MODEL(ped);
	if (model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF")) return L"Wolf";
	if (model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_MEDIUM")) return L"Wolf";
	if (model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_SMALL")) return L"Small wolf";
	if (model == GAMEPLAY::GET_HASH_KEY("A_C_COUGAR_01")) return L"Cougar";
	if (model == GAMEPLAY::GET_HASH_KEY("A_C_PANTHER_01")) return L"Panther";
	return nullptr; // not an animal guard
}

// Behavior IDs: 0=idle, 1=prowling, 2=running, 3=growling(combat), 4=attacking(melee), 5=howling(stopped+not combat)
static int GetAnimalBehavior(Ped ped) {
	if (PED::IS_PED_IN_MELEE_COMBAT(ped))  return 4; // attacking / mauling
	if (PED::IS_PED_IN_COMBAT(ped, 0))     return 3; // growling / aggressive
	if (AI::IS_PED_SPRINTING(ped))         return 2; // running / chasing
	if (AI::IS_PED_RUNNING(ped))           return 2; // running
	if (AI::IS_PED_WALKING(ped))           return 1; // prowling / stalking
	if (PED::IS_PED_STOPPED(ped))          return 5; // idle / howling
	return 0; // idle
}

static const wchar_t* GetAnimalBehaviorText(const wchar_t* animalName, int behavior, bool isCanine) {
	static wchar_t buf[200];
	switch (behavior) {
		case 1: swprintf_s(buf, L"%s prowling", animalName); break;
		case 2: swprintf_s(buf, L"%s chasing!", animalName); break;
		case 3:
			if (isCanine)
				swprintf_s(buf, L"%s growling!", animalName);
			else
				swprintf_s(buf, L"%s snarling!", animalName);
			break;
		case 4:
			if (isCanine)
				swprintf_s(buf, L"%s attacking!", animalName);
			else
				swprintf_s(buf, L"%s mauling!", animalName);
			break;
		case 5:
			if (isCanine)
				swprintf_s(buf, L"%s howling", animalName);
			else
				swprintf_s(buf, L"%s resting", animalName);
			break;
		default: swprintf_s(buf, L"%s idle", animalName); break;
	}
	return buf;
}

static void HandleAutoAnimalBehaviorAnnounce() {
	if (!g_autoAnimalBehaviorAnnounce) return;
	if (g_activeSquad < 0 || g_activeSquad >= (int)g_squads.size()) return;
	DWORD now = GetTickCount();
	if ((now - g_lastAnimalBehaviorCheckMs) < 2000) return;
	g_lastAnimalBehaviorCheckMs = now;

	for (auto &gi : g_squads[g_activeSquad].guards) {
		if (!gi.ped || !ENTITY::DOES_ENTITY_EXIST(gi.ped) || ENTITY::IS_ENTITY_DEAD(gi.ped)) continue;
		const wchar_t* animalName = GetAnimalTypeName(gi.ped);
		if (!animalName) continue; // not an animal

		Hash model = ENTITY::GET_ENTITY_MODEL(gi.ped);
		bool isCanine = (model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF")
					  || model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_MEDIUM")
					  || model == GAMEPLAY::GET_HASH_KEY("A_C_WOLF_SMALL"));

		int behavior = GetAnimalBehavior(gi.ped);
		int pedKey = (int)gi.ped;

		// Only announce on state change
		auto it = g_animalLastBehavior.find(pedKey);
		if (it != g_animalLastBehavior.end() && it->second == behavior) continue;

		// Skip initial idle announce (don't spam on spawn)
		if (it == g_animalLastBehavior.end() && (behavior == 0 || behavior == 5)) {
			g_animalLastBehavior[pedKey] = behavior;
			continue;
		}

		const wchar_t* text = GetAnimalBehaviorText(animalName, behavior, isCanine);
		bool interrupt = (behavior >= 3); // interrupt speech for combat states
		A11y::speak(text, interrupt);
		g_animalLastBehavior[pedKey] = behavior;
	}
}

static void HandleAutoHonorAnnounce() {
	if (!g_autoHonorAnnounce) return;
	DWORD now = GetTickCount();
	if ((now - g_lastHonorCheckMs) < 2000) return;
	g_lastHonorCheckMs = now;

	int honorVal = 0;
	bool success = false;
	
	struct StatId {
		ALIGN8 Hash BaseId;
		ALIGN8 Hash PermId;
	} statId;
	statId.PermId = 0;

	statId.BaseId = 0x89587b4e;
	if (invoke<BOOL>(0x767FBC2AC802EF3E, &statId, &honorVal)) {
		success = true;
	} else {
		statId.BaseId = 0x7C045E1B;
		if (invoke<BOOL>(0x767FBC2AC802EF3E, &statId, &honorVal)) {
			success = true;
		}
	}

	if (success) {
		if (g_lastPlayerHonor == -9999) {
			g_lastPlayerHonor = honorVal;
			return;
		}

		if (honorVal != g_lastPlayerHonor) {
			int diff = honorVal - g_lastPlayerHonor;
			g_lastPlayerHonor = honorVal;

			const wchar_t* level = L"Neutral";
			if (honorVal > 240)       level = L"Very High Honor";
			else if (honorVal > 80)   level = L"High Honor";
			else if (honorVal > 20)   level = L"Good Honor";
			else if (honorVal >= -20) level = L"Neutral";
			else if (honorVal >= -80) level = L"Low Honor";
			else if (honorVal >= -240) level = L"Bad Honor";
			else                      level = L"Very Bad Honor";

			wchar_t buf[256];
			if (diff > 0) {
				swprintf_s(buf, L"Honor increased. New level %s. Value %d", level, honorVal);
			} else {
				swprintf_s(buf, L"Honor decreased. New level %s. Value %d", level, honorVal);
			}
			A11y::speak(buf, false);
			DebugLog::log("Auto honor change detected. Old: %d, New: %d", g_lastPlayerHonor - diff, honorVal);
		}
	}
}

// Structures and globals for Loot item tracking
struct MonitoredItem {
	const char* codeName;
	Hash hash;
	const wchar_t* displayName;
};

static inline char my_toupper(char c) {
	if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
	return c;
}

static Hash CalculateJoaat(const char* str) {
	Hash hash = 0;
	while (*str) {
		hash += my_toupper((unsigned char)*str);
		hash += (hash << 10);
		hash ^= (hash >> 6);
		str++;
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

static MonitoredItem g_monitoredItems[] = {
    { "AMMO_REVOLVER", 0, L"Revolver Cartridges" },
    { "AMMO_REVOLVER_EXPRESS", 0, L"Express Revolver Cartridges" },
    { "AMMO_REVOLVER_HIGH_VELOCITY", 0, L"High Velocity Revolver Cartridges" },
    { "AMMO_REVOLVER_SPLIT_POINT", 0, L"Split Point Revolver Cartridges" },
    { "AMMO_REVOLVER_EXPRESS_EXPLOSIVE", 0, L"Explosive Revolver Cartridges" },
    { "AMMO_PISTOL", 0, L"Pistol Cartridges" },
    { "AMMO_PISTOL_EXPRESS", 0, L"Express Pistol Cartridges" },
    { "AMMO_PISTOL_HIGH_VELOCITY", 0, L"High Velocity Pistol Cartridges" },
    { "AMMO_PISTOL_SPLIT_POINT", 0, L"Split Point Pistol Cartridges" },
    { "AMMO_PISTOL_EXPRESS_EXPLOSIVE", 0, L"Explosive Pistol Cartridges" },
    { "AMMO_REPEATER", 0, L"Repeater Cartridges" },
    { "AMMO_REPEATER_EXPRESS", 0, L"Express Repeater Cartridges" },
    { "AMMO_REPEATER_HIGH_VELOCITY", 0, L"High Velocity Repeater Cartridges" },
    { "AMMO_REPEATER_SPLIT_POINT", 0, L"Split Point Repeater Cartridges" },
    { "AMMO_REPEATER_EXPRESS_EXPLOSIVE", 0, L"Explosive Repeater Cartridges" },
    { "AMMO_RIFLE", 0, L"Rifle Cartridges" },
    { "AMMO_RIFLE_EXPRESS", 0, L"Express Rifle Cartridges" },
    { "AMMO_RIFLE_HIGH_VELOCITY", 0, L"High Velocity Rifle Cartridges" },
    { "AMMO_RIFLE_SPLIT_POINT", 0, L"Split Point Rifle Cartridges" },
    { "AMMO_RIFLE_EXPRESS_EXPLOSIVE", 0, L"Explosive Rifle Cartridges" },
    { "AMMO_SHOTGUN", 0, L"Shotgun Shells" },
    { "AMMO_SHOTGUN_SLUG", 0, L"Shotgun Slugs" },
    { "AMMO_SHOTGUN_BUCKSHOT_INCENDIARY", 0, L"Incendiary Shotgun Shells" },
    { "AMMO_SHOTGUN_SLUG_EXPLOSIVE", 0, L"Explosive Shotgun Slugs" },
    { "AMMO_ARROW", 0, L"Arrows" },
    { "AMMO_ARROW_IMPROVED", 0, L"Improved Arrows" },
    { "AMMO_ARROW_SMALL_GAME", 0, L"Small Game Arrows" },
    { "AMMO_ARROW_FIRE", 0, L"Fire Arrows" },
    { "AMMO_ARROW_POISON", 0, L"Poison Arrows" },
    { "AMMO_ARROW_DYNAMITE", 0, L"Dynamite Arrows" },
    { "WEAPON_DYNAMITE", 0, L"Dynamite" },
    { "WEAPON_DYNAMITE_VOLATILE", 0, L"Volatile Dynamite" },
    { "WEAPON_TOMAHAWK", 0, L"Tomahawk" },
    { "WEAPON_THROWING_KNIFE", 0, L"Throwing Knife" },
    { "WEAPON_FIREBOTTLE", 0, L"Fire Bottle" },
    { "KIT_GOLD_RING", 0, L"Gold Ring" },
    { "KIT_SILVER_RING", 0, L"Silver Ring" },
    { "KIT_PLATINUM_RING", 0, L"Platinum Ring" },
    { "KIT_GOLD_POCKET_WATCH", 0, L"Gold Watch" },
    { "KIT_SILVER_POCKET_WATCH", 0, L"Silver Watch" },
    { "KIT_PLATINUM_POCKET_WATCH", 0, L"Platinum Watch" },
    { "KIT_GOLD_TOOTH", 0, L"Gold Tooth" },
    { "KIT_GOLD_BAR", 0, L"Gold Bar" },
    { "PROVISION_GOLD_NUGGET", 0, L"Gold Nugget" },
    { "KIT_SILVER_BUCKLE", 0, L"Silver Buckle" },
    { "KIT_GOLD_BUCKLE", 0, L"Gold Buckle" },
    { "KIT_JEWELRY_LARGE", 0, L"Large Jewelry" },
    { "KIT_JEWELRY_SMALL", 0, L"Small Jewelry" },
    { "PROVISION_CIGAR", 0, L"Cigar" },
    { "PROVISION_CIGARETTE", 0, L"Cigarette" },
    { "PROVISION_CHEWING_TOBACCO", 0, L"Chewing Tobacco" },
    { "PROVISION_CIGARETTES", 0, L"Cigarettes" },
    { "PROVISION_CIGARETTES_BOX", 0, L"Premium Cigarettes" },
    { "PROVISION_WHISKEY", 0, L"Whiskey" },
    { "PROVISION_GIN", 0, L"Gin" },
    { "PROVISION_FINE_BRANDY", 0, L"Fine Brandy" },
    { "PROVISION_GUARMA_RUM", 0, L"Guarma Rum" },
    { "PROVISION_CANNED_PEACHES", 0, L"Canned Peaches" },
    { "PROVISION_CANNED_PINEAPPLES", 0, L"Canned Pineapples" },
    { "PROVISION_CANNED_APRICOTS", 0, L"Canned Apricots" },
    { "PROVISION_CANNED_BEANS", 0, L"Canned Beans" },
    { "PROVISION_CANNED_CORN", 0, L"Canned Corn" },
    { "PROVISION_CANNED_SALMON", 0, L"Canned Salmon" },
    { "PROVISION_CANNED_KIDNEY_BEANS", 0, L"Canned Kidney Beans" },
    { "PROVISION_CANNED_PEAS", 0, L"Canned Peas" },
    { "PROVISION_SALTED_BEEF", 0, L"Salted Beef" },
    { "PROVISION_CANNED_SALTED_BEEF", 0, L"Canned Salted Beef" },
    { "PROVISION_CRACKERS", 0, L"Crackers" },
    { "PROVISION_BREAD", 0, L"Bread" },
    { "PROVISION_CHOCOLATE", 0, L"Chocolate" },
    { "PROVISION_CANDY", 0, L"Candy" },
    { "PROVISION_OATCAKES", 0, L"Oatcakes" },
    { "PROVISION_APPLES", 0, L"Apple" },
    { "PROVISION_CARROT", 0, L"Carrot" },
    { "PROVISION_CELERY", 0, L"Celery" },
    { "PROVISION_PEACH", 0, L"Orchard Peach" },
    { "PROVISION_CHERRY", 0, L"Orchard Cherry" },
    { "PROVISION_PEAR", 0, L"Orchard Pear" },
    { "CONSUMABLE_HEALTH_CURE", 0, L"Health Cure" },
    { "CONSUMABLE_HEALTH_CURE_POTENT", 0, L"Potent Health Cure" },
    { "CONSUMABLE_HEALTH_CURE_SPECIAL", 0, L"Special Health Cure" },
    { "CONSUMABLE_SNAKE_OIL", 0, L"Snake Oil" },
    { "CONSUMABLE_SNAKE_OIL_POTENT", 0, L"Potent Snake Oil" },
    { "CONSUMABLE_SNAKE_OIL_SPECIAL", 0, L"Special Snake Oil" },
    { "CONSUMABLE_MIRACLE_TONIC", 0, L"Miracle Tonic" },
    { "CONSUMABLE_MIRACLE_TONIC_POTENT", 0, L"Potent Miracle Tonic" },
    { "CONSUMABLE_MIRACLE_TONIC_SPECIAL", 0, L"Special Miracle Tonic" },
    { "CONSUMABLE_BITTERS", 0, L"Bitters" },
    { "CONSUMABLE_BITTERS_POTENT", 0, L"Potent Bitters" },
    { "CONSUMABLE_BITTERS_SPECIAL", 0, L"Special Bitters" },
    { "CONSUMABLE_COCAINE_CHEWING_GUM", 0, L"Cocaine Chewing Gum" },
    { "CONSUMABLE_MEDICINE", 0, L"Medicine" },
    { "CONSUMABLE_POTENT_MEDICINE", 0, L"Potent Medicine" },
    { "CONSUMABLE_SPECIAL_MEDICINE", 0, L"Special Medicine" },
    { "PROVISION_DEER_PELT", 0, L"Deer Pelt" },
    { "PROVISION_WOLF_PELT", 0, L"Wolf Pelt" },
    { "PROVISION_RABBIT_PELT", 0, L"Rabbit Pelt" },
    { "PROVISION_BOAR_PELT", 0, L"Boar Pelt" },
    { "PROVISION_COYOTE_PELT", 0, L"Coyote Pelt" },
    { "PROVISION_BEAR_PELT", 0, L"Bear Pelt" },
    { "PROVISION_FOX_PELT", 0, L"Fox Pelt" },
    { "PROVISION_ELK_PELT", 0, L"Elk Pelt" },
    { "PROVISION_BISON_PELT", 0, L"Bison Pelt" },
    { "PROVISION_ALLIGATOR_SKIN", 0, L"Alligator Skin" },
    { "PROVISION_COUGAR_PELT", 0, L"Cougar Pelt" },
    { "PROVISION_PANTHER_PELT", 0, L"Panther Pelt" },
    { "PROVISION_MOOSE_PELT", 0, L"Moose Pelt" },
    { "PROVISION_BEAVER_PELT", 0, L"Beaver Pelt" },
    { "PROVISION_RACCOON_PELT", 0, L"Raccoon Pelt" },
    { "PROVISION_BADGER_PELT", 0, L"Badger Pelt" },
    { "PROVISION_OPOSSUM_PELT", 0, L"Opossum Pelt" },
    { "PROVISION_SKUNK_PELT", 0, L"Skunk Pelt" },
    { "PROVISION_MUSKRAT_PELT", 0, L"Muskrat Pelt" },
    { "PROVISION_SQUIRREL_PELT", 0, L"Squirrel Pelt" },
    { "PROVISION_RAT_PELT", 0, L"Rat Pelt" },
    { "PROVISION_SNAKE_SKIN", 0, L"Snake Skin" },
    { "PROVISION_IGUANA_SKIN", 0, L"Iguana Skin" },
    { "PROVISION_ANIMAL_FAT", 0, L"Animal Fat" },
    { "PROVISION_FLIGHT_FEATHER", 0, L"Flight Feather" },
    { "PROVISION_TURKEY_FEATHER", 0, L"Turkey Feather" },
    { "PROVISION_HAWK_FEATHER", 0, L"Hawk Feather" },
    { "PROVISION_EAGLE_FEATHER", 0, L"Eagle Feather" },
    { "PROVISION_OWL_FEATHER", 0, L"Owl Feather" },
    { "PROVISION_RAVEN_FEATHER", 0, L"Raven Feather" },
    { "PROVISION_DUCK_FEATHER", 0, L"Duck Feather" },
    { "PROVISION_GOOSE_FEATHER", 0, L"Goose Feather" },
    { "PROVISION_VENISON", 0, L"Venison" },
    { "PROVISION_DEER_MEAT", 0, L"Deer Meat" },
    { "PROVISION_BIG_GAME_MEAT", 0, L"Big Game Meat" },
    { "PROVISION_GAME_MEAT", 0, L"Game Meat" },
    { "PROVISION_POULTRY_MEAT", 0, L"Poultry Meat" },
    { "PROVISION_MUTTON", 0, L"Mutton" },
    { "PROVISION_BEEF", 0, L"Beef" },
    { "PROVISION_PORK", 0, L"Pork" },
    { "PROVISION_YARROW", 0, L"Yarrow" },
    { "PROVISION_WILD_CARROT", 0, L"Wild Carrot" },
    { "PROVISION_INDIAN_TOBACCO", 0, L"Indian Tobacco" },
    { "PROVISION_GINSENG", 0, L"Ginseng" },
    { "PROVISION_ALASKAN_GINSENG", 0, L"Alaskan Ginseng" },
    { "PROVISION_AMERICAN_GINSENG", 0, L"American Ginseng" },
    { "PROVISION_BAY_BOLETE", 0, L"Bay Bolete" },
    { "PROVISION_BLACK_BLACKBERRY", 0, L"Blackberry" },
    { "PROVISION_BLACK_BLACKCURRANT", 0, L"Blackcurrant" },
    { "PROVISION_BURDOCK_ROOT", 0, L"Burdock Root" },
    { "PROVISION_CHANTERELLES", 0, L"Chanterelles" },
    { "PROVISION_COMMON_BULRUSH", 0, L"Common Bulrush" },
    { "PROVISION_CREEPING_THYME", 0, L"Creeping Thyme" },
    { "PROVISION_ENGLISH_MACE", 0, L"English Mace" },
    { "PROVISION_GOLDEN_CURRANT", 0, L"Golden Currant" },
    { "PROVISION_HUMMINGBIRD_SAGE", 0, L"Hummingbird Sage" },
    { "PROVISION_MILKWEED", 0, L"Milkweed" },
    { "PROVISION_OLEANDER_SAGE", 0, L"Oleander Sage" },
    { "PROVISION_OREGANO", 0, L"Oregano" },
    { "PROVISION_PARASOL_MUSHROOM", 0, L"Parasol Mushroom" },
    { "PROVISION_PRAIRIE_POPPY", 0, L"Prairie Poppy" },
    { "PROVISION_RAMS_HEAD", 0, L"Ram's Head" },
    { "PROVISION_RED_RASPBERRY", 0, L"Red Raspberry" },
    { "PROVISION_RED_SAGE", 0, L"Red Sage" },
    { "PROVISION_VANILLA_FLOWER", 0, L"Vanilla Flower" },
    { "PROVISION_VIOLET_SNOWDROP", 0, L"Violet Snowdrop" },
    { "PROVISION_WILD_FEVERFEW", 0, L"Wild Feverfew" },
    { "PROVISION_WILD_MINT", 0, L"Wild Mint" },
    { "PROVISION_WINTERGREEN_BERRY", 0, L"Wintergreen Berry" },
    { "PROVISION_ORCHID_ACUNAS_STAR", 0, L"Acuna's Star Orchid" },
    { "PROVISION_ORCHID_CIGAR", 0, L"Cigar Orchid" },
    { "PROVISION_ORCHID_CLAM_SHELL", 0, L"Clamshell Orchid" },
    { "PROVISION_ORCHID_DRAGONS_MOUTH", 0, L"Dragon's Mouth Orchid" },
    { "PROVISION_ORCHID_GHOST", 0, L"Ghost Orchid" },
    { "PROVISION_ORCHID_LADY_SLIPPER", 0, L"Lady Slipper Orchid" },
    { "PROVISION_ORCHID_MOCASSIN", 0, L"Moccasin Orchid" },
    { "PROVISION_ORCHID_NIGHT_SCENTED", 0, L"Night-scented Orchid" },
    { "PROVISION_ORCHID_QUEENS", 0, L"Queen's Orchid" },
    { "PROVISION_ORCHID_RAT_TAIL", 0, L"Rat-tail Orchid" },
    { "PROVISION_ORCHID_SPARROWS_FOOT", 0, L"Sparrow's-foot Orchid" },
    { "PROVISION_ORCHID_SPIDER", 0, L"Spider Orchid" },
    { "PROVISION_ALLIGATOR_EGG", 0, L"Alligator Egg" },
    { "PROVISION_BEAR_CLAW", 0, L"Bear Claw" },
    { "PROVISION_BEAVER_TOOTH", 0, L"Beaver Tooth" },
    { "PROVISION_BOAR_TUSK", 0, L"Boar Tusk" },
    { "PROVISION_COUGAR_FANG", 0, L"Cougar Fang" },
    { "PROVISION_ELK_ANTLER", 0, L"Elk Antler" },
    { "PROVISION_EGRET_FEATHER", 0, L"Egret Feather" },
    { "PROVISION_HERON_FEATHER", 0, L"Heron Feather" },
    { "PROVISION_SPOONBILL_FEATHER", 0, L"Spoonbill Feather" },
    { "PROVISION_PELICAN_FEATHER", 0, L"Pelican Feather" },
    { "PROVISION_TURKEY_MEAT", 0, L"Turkey Meat" },
    { "KIT_GOLD_NECKLACE", 0, L"Gold Necklace" },
    { "KIT_SILVER_NECKLACE", 0, L"Silver Necklace" },
    { "KIT_PLATINUM_NECKLACE", 0, L"Platinum Necklace" },
    { "KIT_GOLD_EARRING", 0, L"Gold Earring" },
    { "KIT_SILVER_EARRING", 0, L"Silver Earring" },
    { "KIT_PLATINUM_EARRING", 0, L"Platinum Earring" },
    { "VALUABLE_GOLD_TOOTH", 0, L"Gold Tooth" },
    { "VALUABLE_GOLD_BAR", 0, L"Gold Bar" },
    { "VALUABLE_GOLD_NUGGET", 0, L"Gold Nugget" },
    { "PROVISION_PELT_BEAR_LEGENDARY", 0, L"Legendary Bear Pelt" },
    { "PROVISION_PELT_BEAVER_LEGENDARY", 0, L"Legendary Beaver Pelt" },
    { "PROVISION_PELT_BISON_LEGENDARY", 0, L"Legendary Bison Pelt" },
    { "PROVISION_PELT_BOAR_LEGENDARY", 0, L"Legendary Boar Pelt" },
    { "PROVISION_PELT_COUGAR_LEGENDARY", 0, L"Legendary Cougar Pelt" },
    { "PROVISION_PELT_COYOTE_LEGENDARY", 0, L"Legendary Coyote Pelt" },
    { "PROVISION_PELT_DEER_LEGENDARY", 0, L"Legendary Deer Pelt" },
    { "PROVISION_PELT_ELK_LEGENDARY", 0, L"Legendary Elk Pelt" },
    { "PROVISION_PELT_FOX_LEGENDARY", 0, L"Legendary Fox Pelt" },
    { "PROVISION_PELT_MOOSE_LEGENDARY", 0, L"Legendary Moose Pelt" },
    { "PROVISION_PELT_PANTHER_LEGENDARY", 0, L"Legendary Panther Pelt" },
    { "PROVISION_PELT_RAM_LEGENDARY", 0, L"Legendary Ram Pelt" },
    { "PROVISION_PELT_WOLF_LEGENDARY", 0, L"Legendary Wolf Pelt" },
    { "PROVISION_PELT_LEGENDARY_BEAR", 0, L"Legendary Bear Pelt" },
    { "PROVISION_PELT_LEGENDARY_BEAVER", 0, L"Legendary Beaver Pelt" },
    { "PROVISION_PELT_LEGENDARY_BISON", 0, L"Legendary Bison Pelt" },
    { "PROVISION_PELT_LEGENDARY_BOAR", 0, L"Legendary Boar Pelt" },
    { "PROVISION_PELT_LEGENDARY_COUGAR", 0, L"Legendary Cougar Pelt" },
    { "PROVISION_PELT_LEGENDARY_COYOTE", 0, L"Legendary Coyote Pelt" },
    { "PROVISION_PELT_LEGENDARY_DEER", 0, L"Legendary Deer Pelt" },
    { "PROVISION_PELT_LEGENDARY_ELK", 0, L"Legendary Elk Pelt" },
    { "PROVISION_PELT_LEGENDARY_FOX", 0, L"Legendary Fox Pelt" },
    { "PROVISION_PELT_LEGENDARY_MOOSE", 0, L"Legendary Moose Pelt" },
    { "PROVISION_PELT_LEGENDARY_PANTHER", 0, L"Legendary Panther Pelt" },
    { "PROVISION_PELT_LEGENDARY_RAM", 0, L"Legendary Ram Pelt" },
    { "PROVISION_PELT_LEGENDARY_WOLF", 0, L"Legendary Wolf Pelt" },
    { "PROVISION_BEAR_CLAW_LEGENDARY", 0, L"Legendary Bear Claw" },
    { "PROVISION_LEGENDARY_BEAR_CLAW", 0, L"Legendary Bear Claw" },
    { "PROVISION_COUGAR_FANG_LEGENDARY", 0, L"Legendary Cougar Fang" },
    { "PROVISION_LEGENDARY_COUGAR_FANG", 0, L"Legendary Cougar Fang" },
    { "PROVISION_BEAVER_TOOTH_LEGENDARY", 0, L"Legendary Beaver Tooth" },
    { "PROVISION_LEGENDARY_BEAVER_TOOTH", 0, L"Legendary Beaver Tooth" },
    { "PROVISION_BOAR_TUSK_LEGENDARY", 0, L"Legendary Boar Tusk" },
    { "PROVISION_LEGENDARY_BOAR_TUSK", 0, L"Legendary Boar Tusk" },
    { "PROVISION_ELK_ANTLER_LEGENDARY", 0, L"Legendary Elk Antler" },
    { "PROVISION_LEGENDARY_ELK_ANTLER", 0, L"Legendary Elk Antler" },
    { "PROVISION_FOX_CLAW_LEGENDARY", 0, L"Legendary Fox Claw" },
    { "PROVISION_LEGENDARY_FOX_CLAW", 0, L"Legendary Fox Claw" },
    { "PROVISION_WOLF_HEART_LEGENDARY", 0, L"Legendary Wolf Heart" },
    { "PROVISION_LEGENDARY_WOLF_HEART", 0, L"Legendary Wolf Heart" },
    { "PROVISION_PANTHER_EYE_LEGENDARY", 0, L"Legendary Panther Eye" },
    { "PROVISION_LEGENDARY_PANTHER_EYE", 0, L"Legendary Panther Eye" },
    { "PROVISION_RAM_HORN_LEGENDARY", 0, L"Legendary Ram Horn" },
    { "PROVISION_LEGENDARY_RAM_HORN", 0, L"Legendary Ram Horn" },
    { "PROVISION_BISON_HORN_LEGENDARY", 0, L"Legendary Bison Horn" },
    { "PROVISION_LEGENDARY_BISON_HORN", 0, L"Legendary Bison Horn" },
    { "PROVISION_MOOSE_ANTLER_LEGENDARY", 0, L"Legendary Moose Antler" },
    { "PROVISION_LEGENDARY_MOOSE_ANTLER", 0, L"Legendary Moose Antler" },
    { "PROVISION_COYOTE_FANG_LEGENDARY", 0, L"Legendary Coyote Fang" },
    { "PROVISION_LEGENDARY_COYOTE_FANG", 0, L"Legendary Coyote Fang" },
    { "PROVISION_DEER_ANTLER_LEGENDARY", 0, L"Legendary Deer Antler" },
    { "PROVISION_LEGENDARY_DEER_ANTLER", 0, L"Legendary Deer Antler" },
    { "PROVISION_TATANKA_HORN_LEGENDARY", 0, L"Legendary Tatanka Bison Horn" },
    { "PROVISION_LEGENDARY_TATANKA_HORN", 0, L"Legendary Tatanka Bison Horn" },
    { "PROVISION_OWL_FEATHER_LEGENDARY", 0, L"Legendary Owl Feather" },
    { "PROVISION_LEGENDARY_OWL_FEATHER", 0, L"Legendary Owl Feather" },
    { "PROVISION_FISH_BLUEGILL", 0, L"Bluegill" },
    { "PROVISION_FISH_BULLHEAD_CATFISH", 0, L"Bullhead Catfish" },
    { "PROVISION_FISH_CHAIN_PICKEREL", 0, L"Chain Pickerel" },
    { "PROVISION_FISH_LAKE_STURGEON", 0, L"Lake Sturgeon" },
    { "PROVISION_FISH_LARGEMOUTH_BASS", 0, L"Largemouth Bass" },
    { "PROVISION_FISH_MUSKIE", 0, L"Muskie" },
    { "PROVISION_FISH_PERCH", 0, L"Perch" },
    { "PROVISION_FISH_RED_FIN_PICKEREL", 0, L"Redfin Pickerel" },
    { "PROVISION_FISH_ROCK_BASS", 0, L"Rock Bass" },
    { "PROVISION_FISH_SMALLMOUTH_BASS", 0, L"Smallmouth Bass" },
    { "PROVISION_FISH_SOCKEYE_SALMON", 0, L"Sockeye Salmon" },
    { "PROVISION_FISH_STEELHEAD_TROUT", 0, L"Steelhead Trout" },
    { "PROVISION_FISH_CHANNEL_CATFISH", 0, L"Channel Catfish" },
    { "PROVISION_SUCCULENT_FISH", 0, L"Succulent Fish Meat" },
    { "PROVISION_FLAKY_FISH", 0, L"Flaky Fish Meat" },
    { "PROVISION_GRITTY_FISH", 0, L"Gritty Fish Meat" },
    { "PROVISION_CRUSTACEAN_MEAT", 0, L"Crustacean Meat" },
    { "CONSUMABLE_GUN_OIL", 0, L"Gun Oil" },
    { "CONSUMABLE_HAIR_TONIC", 0, L"Hair Tonic" },
    { "CONSUMABLE_POMADE", 0, L"Hair Pomade" },
    { "CONSUMABLE_OPENED_HEALTH_CURE", 0, L"Opened Health Cure" },
    { "CONSUMABLE_OPENED_SNAKE_OIL", 0, L"Opened Snake Oil" },
    { "CONSUMABLE_OPENED_MIRACLE_TONIC", 0, L"Opened Miracle Tonic" },
    { "CONSUMABLE_OPENED_BITTERS", 0, L"Opened Bitters" },
    { "CONSUMABLE_OPENED_MEDICINE", 0, L"Opened Medicine" },
    { "PROVISION_OPENED_WHISKEY", 0, L"Opened Whiskey" },
    { "PROVISION_OPENED_GIN", 0, L"Opened Gin" },
    { "PROVISION_OPENED_FINE_BRANDY", 0, L"Opened Fine Brandy" },
    { "PROVISION_OPENED_GUARMA_RUM", 0, L"Opened Guarma Rum" },
    { "CONSUMABLE_HORSE_MEDICINE", 0, L"Horse Medicine" },
    { "CONSUMABLE_HORSE_MEDICINE_POTENT", 0, L"Potent Horse Medicine" },
    { "CONSUMABLE_HORSE_MEDICINE_SPECIAL", 0, L"Special Horse Medicine" },
    { "CONSUMABLE_HORSE_STIMULANT", 0, L"Horse Stimulant" },
    { "CONSUMABLE_HORSE_STIMULANT_POTENT", 0, L"Potent Horse Stimulant" },
    { "CONSUMABLE_HORSE_STIMULANT_SPECIAL", 0, L"Special Horse Stimulant" },
    { "CONSUMABLE_HORSE_REVIVER", 0, L"Horse Reviver" },
    { "CONSUMABLE_HORSE_REVIVER_POTENT", 0, L"Potent Horse Reviver" },
    { "CONSUMABLE_HORSE_REVIVER_SPECIAL", 0, L"Special Horse Reviver" },
    { "CONSUMABLE_HORSE_OAT_ROLLS", 0, L"Oat Rolls" },
    { "CONSUMABLE_HORSE_HAY", 0, L"Hay" },
    { "------------", 0, L"------------" },
    { "AMMO_ARROW_CONFUSION", 0, L"Ammo Arrow Confusion" },
    { "AMMO_ARROW_DISORIENT", 0, L"Ammo Arrow Disorient" },
    { "AMMO_ARROW_DRAIN", 0, L"Ammo Arrow Drain" },
    { "AMMO_ARROW_TRAIL", 0, L"Ammo Arrow Trail" },
    { "AMMO_ARROW_WOUND", 0, L"Ammo Arrow Wound" },
    { "AMMO_BULLET_EXPLOSIVE", 0, L"Ammo Bullet Explosive" },
    { "AMMO_BULLET_EXPRESS", 0, L"Ammo Bullet Express" },
    { "AMMO_BULLET_HIGH_VELOCITY", 0, L"Ammo Bullet High Velocity" },
    { "AMMO_BULLET_NORMAL", 0, L"Ammo Bullet Normal" },
    { "AMMO_BULLET_SPLIT_POINT", 0, L"Ammo Bullet Split Point" },
    { "AMMO_DYNAMITE_NORMAL", 0, L"Ammo Dynamite Normal" },
    { "AMMO_DYNAMITE_VOLATILE", 0, L"Ammo Dynamite Volatile" },
    { "AMMO_FIRE_BOTTLE_NORMAL", 0, L"Ammo Fire Bottle Normal" },
    { "AMMO_FIRE_BOTTLE_VOLATILE", 0, L"Ammo Fire Bottle Volatile" },
    { "AMMO_THROWING_KNIVES_CONFUSE", 0, L"Ammo Throwing Knives Confuse" },
    { "AMMO_THROWING_KNIVES_DISORIENT", 0, L"Ammo Throwing Knives Disorient" },
    { "AMMO_THROWING_KNIVES_DRAIN", 0, L"Ammo Throwing Knives Drain" },
    { "AMMO_THROWING_KNIVES_IMPROVED", 0, L"Ammo Throwing Knives Improved" },
    { "AMMO_THROWING_KNIVES_NORMAL", 0, L"Ammo Throwing Knives Normal" },
    { "AMMO_THROWING_KNIVES_POISON", 0, L"Ammo Throwing Knives Poison" },
    { "AMMO_THROWING_KNIVES_TRAIL", 0, L"Ammo Throwing Knives Trail" },
    { "AMMO_THROWING_KNIVES_WOUND", 0, L"Ammo Throwing Knives Wound" },
    { "AMMO_TOMAHAWK", 0, L"Ammo Tomahawk" },
    { "AMMO_TOMAHAWK_ANCIENT", 0, L"Ammo Tomahawk Ancient" },
    { "AMMO_TOMAHAWK_HOMING", 0, L"Ammo Tomahawk Homing" },
    { "AMMO_TOMAHAWK_IMPROVED", 0, L"Ammo Tomahawk Improved" },
    { "CLOTHING_GENERIC_BOOTS", 0, L"Clothing Generic Boots" },
    { "CLOTHING_GENERIC_CHAPS", 0, L"Clothing Generic Chaps" },
    { "CLOTHING_GENERIC_CLOAK", 0, L"Clothing Generic Cloak" },
    { "CLOTHING_GENERIC_COAT", 0, L"Clothing Generic Coat" },
    { "CLOTHING_GENERIC_DRESS", 0, L"Clothing Generic Dress" },
    { "CLOTHING_GENERIC_GLOVE", 0, L"Clothing Generic Glove" },
    { "CLOTHING_GENERIC_GUNBELT", 0, L"Clothing Generic Gunbelt" },
    { "CLOTHING_GENERIC_HAT", 0, L"Clothing Generic Hat" },
    { "CLOTHING_GENERIC_MASK", 0, L"Clothing Generic Mask" },
    { "CLOTHING_GENERIC_NECKERCHIEF", 0, L"Clothing Generic Neckerchief" },
    { "CLOTHING_GENERIC_OUTFIT", 0, L"Clothing Generic Outfit" },
    { "CLOTHING_GENERIC_PANTS", 0, L"Clothing Generic Pants" },
    { "CLOTHING_GENERIC_SHIRT", 0, L"Clothing Generic Shirt" },
    { "CLOTHING_GENERIC_SKIRT", 0, L"Clothing Generic Skirt" },
    { "CLOTHING_GENERIC_SPATS", 0, L"Clothing Generic Spats" },
    { "CLOTHING_GENERIC_SUSPENDERS", 0, L"Clothing Generic Suspenders" },
    { "CLOTHING_GENERIC_VEST", 0, L"Clothing Generic Vest" },
    { "CLOTHING_HAT_000_POLICE", 0, L"Clothing Hat 000 Police" },
    { "CLOTHING_HAT_GRIZZLED_JON", 0, L"Clothing Hat Grizzled Jon" },
    { "CLOTHING_HL_PLAYER_HAT_000_001_1", 0, L"Clothing Hl Player Hat 000 001 1" },
    { "CLOTHING_HL_PLAYER_HAT_000_011_1", 0, L"Clothing Hl Player Hat 000 011 1" },
    { "CLOTHING_HL_PLAYER_HAT_002_1", 0, L"Clothing Hl Player Hat 002 1" },
    { "CLOTHING_HL_PLAYER_HAT_002_2", 0, L"Clothing Hl Player Hat 002 2" },
    { "CLOTHING_HL_PLAYER_HAT_003_1", 0, L"Clothing Hl Player Hat 003 1" },
    { "CLOTHING_HL_PLAYER_HAT_004_1", 0, L"Clothing Hl Player Hat 004 1" },
    { "CLOTHING_HL_PLAYER_HAT_005_1", 0, L"Clothing Hl Player Hat 005 1" },
    { "CLOTHING_HL_PLAYER_HAT_006_1", 0, L"Clothing Hl Player Hat 006 1" },
    { "CLOTHING_HL_PLAYER_HAT_007_1", 0, L"Clothing Hl Player Hat 007 1" },
    { "CLOTHING_HL_PLAYER_HAT_008_1", 0, L"Clothing Hl Player Hat 008 1" },
    { "CLOTHING_HL_PLAYER_HAT_009_1", 0, L"Clothing Hl Player Hat 009 1" },
    { "CLOTHING_HL_PLAYER_HAT_010_1", 0, L"Clothing Hl Player Hat 010 1" },
    { "CLOTHING_HL_PLAYER_HAT_011_1", 0, L"Clothing Hl Player Hat 011 1" },
    { "CLOTHING_HL_PLAYER_HAT_012_1", 0, L"Clothing Hl Player Hat 012 1" },
    { "CLOTHING_HL_PLAYER_HAT_013_1", 0, L"Clothing Hl Player Hat 013 1" },
    { "CLOTHING_HL_PLAYER_HAT_015_1", 0, L"Clothing Hl Player Hat 015 1" },
    { "CLOTHING_HL_PLAYER_HAT_016_1", 0, L"Clothing Hl Player Hat 016 1" },
    { "CLOTHING_HL_PLAYER_HAT_027_1", 0, L"Clothing Hl Player Hat 027 1" },
    { "CLOTHING_HL_PLAYER_HAT_028_1", 0, L"Clothing Hl Player Hat 028 1" },
    { "CLOTHING_HL_PLAYER_HAT_029_1", 0, L"Clothing Hl Player Hat 029 1" },
    { "CLOTHING_HL_PLAYER_HAT_030_1", 0, L"Clothing Hl Player Hat 030 1" },
    { "CLOTHING_HL_PLAYER_HAT_031_1", 0, L"Clothing Hl Player Hat 031 1" },
    { "CLOTHING_HL_PLAYER_HAT_032_1", 0, L"Clothing Hl Player Hat 032 1" },
    { "CLOTHING_HL_PLAYER_HAT_033_1", 0, L"Clothing Hl Player Hat 033 1" },
    { "CLOTHING_HL_PLAYER_HAT_034_1", 0, L"Clothing Hl Player Hat 034 1" },
    { "CLOTHING_HL_PLAYER_HAT_038_1", 0, L"Clothing Hl Player Hat 038 1" },
    { "CLOTHING_HL_PLAYER_HAT_040_1", 0, L"Clothing Hl Player Hat 040 1" },
    { "CLOTHING_HL_PLAYER_HAT_041_1", 0, L"Clothing Hl Player Hat 041 1" },
    { "CLOTHING_HL_PLAYER_HAT_042_1", 0, L"Clothing Hl Player Hat 042 1" },
    { "CLOTHING_HL_PLAYER_HAT_043_1", 0, L"Clothing Hl Player Hat 043 1" },
    { "CLOTHING_HL_PLAYER_HAT_044_1", 0, L"Clothing Hl Player Hat 044 1" },
    { "CLOTHING_HL_PLAYER_HAT_045_1", 0, L"Clothing Hl Player Hat 045 1" },
    { "CLOTHING_HL_PLAYER_HAT_046_1", 0, L"Clothing Hl Player Hat 046 1" },
    { "CLOTHING_HL_PLAYER_HAT_047_1", 0, L"Clothing Hl Player Hat 047 1" },
    { "CLOTHING_HL_PLAYER_HAT_048_1", 0, L"Clothing Hl Player Hat 048 1" },
    { "CLOTHING_HL_PLAYER_HAT_050_1", 0, L"Clothing Hl Player Hat 050 1" },
    { "CLOTHING_HL_PLAYER_HAT_052_1", 0, L"Clothing Hl Player Hat 052 1" },
    { "CLOTHING_HL_PLAYER_HAT_053_1", 0, L"Clothing Hl Player Hat 053 1" },
    { "CLOTHING_HL_PLAYER_HAT_054_1", 0, L"Clothing Hl Player Hat 054 1" },
    { "CLOTHING_HL_PLAYER_HAT_056_1", 0, L"Clothing Hl Player Hat 056 1" },
    { "CLOTHING_HL_PLAYER_HAT_057_1", 0, L"Clothing Hl Player Hat 057 1" },
    { "CLOTHING_HL_PLAYER_HAT_058_1", 0, L"Clothing Hl Player Hat 058 1" },
    { "CLOTHING_HL_PLAYER_HAT_059_1", 0, L"Clothing Hl Player Hat 059 1" },
    { "CLOTHING_HL_PLAYER_HAT_060_1", 0, L"Clothing Hl Player Hat 060 1" },
    { "CLOTHING_HL_PLAYER_HAT_061_1", 0, L"Clothing Hl Player Hat 061 1" },
    { "CLOTHING_HL_PLAYER_HAT_062_1", 0, L"Clothing Hl Player Hat 062 1" },
    { "CLOTHING_HL_PLAYER_HAT_063_1", 0, L"Clothing Hl Player Hat 063 1" },
    { "CLOTHING_HL_PLAYER_HAT_064_1", 0, L"Clothing Hl Player Hat 064 1" },
    { "CLOTHING_HL_PLAYER_HAT_065_1", 0, L"Clothing Hl Player Hat 065 1" },
    { "CLOTHING_HL_PLAYER_HAT_066_1", 0, L"Clothing Hl Player Hat 066 1" },
    { "CLOTHING_HL_PLAYER_HAT_067_1", 0, L"Clothing Hl Player Hat 067 1" },
    { "CLOTHING_HL_PLAYER_HAT_068_1", 0, L"Clothing Hl Player Hat 068 1" },
    { "CLOTHING_HL_PLAYER_HAT_069_1", 0, L"Clothing Hl Player Hat 069 1" },
    { "CLOTHING_HL_PLAYER_HAT_070_1", 0, L"Clothing Hl Player Hat 070 1" },
    { "CLOTHING_HL_PLAYER_HAT_072_1", 0, L"Clothing Hl Player Hat 072 1" },
    { "CLOTHING_HL_PLAYER_HAT_073_1", 0, L"Clothing Hl Player Hat 073 1" },
    { "CLOTHING_HL_PLAYER_HAT_ACCS", 0, L"Clothing Hl Player Hat Accs" },
    { "CLOTHING_HL_PLAYER_SPURS", 0, L"Clothing Hl Player Spurs" },
    { "CLOTHING_HL_PLAYER_UST", 0, L"Clothing Hl Player Ust" },
    { "CLOTHING_ITEM_HAT_PZERO_000", 0, L"Clothing Item Hat Pzero 000" },
    { "CLOTHING_ITEM_MASK_PIG_001", 0, L"Clothing Item Mask Pig 001" },
    { "CLOTHING_ITEM_PZ_HAT_PIRATE_01", 0, L"Clothing Item Pz Hat Pirate 01" },
    { "CLOTHING_ITEM_SKULLMASK_MR1_000_1", 0, L"Clothing Item Skullmask Mr1 000 1" },
    { "CLOTHING_ITEM_SKULLMASK_MR1_001_1", 0, L"Clothing Item Skullmask Mr1 001 1" },
    { "CLOTHING_ITEM_SKULLMASK_MR1_002_1", 0, L"Clothing Item Skullmask Mr1 002 1" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_000", 0, L"Clothing Item Sp Collectable Hat Mr1 000" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_001", 0, L"Clothing Item Sp Collectable Hat Mr1 001" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_002_ALT02", 0, L"Clothing Item Sp Collectable Hat Mr1 002 Alt02" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_004", 0, L"Clothing Item Sp Collectable Hat Mr1 004" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_005", 0, L"Clothing Item Sp Collectable Hat Mr1 005" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_006_ALT02", 0, L"Clothing Item Sp Collectable Hat Mr1 006 Alt02" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_007_ALT02", 0, L"Clothing Item Sp Collectable Hat Mr1 007 Alt02" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_008", 0, L"Clothing Item Sp Collectable Hat Mr1 008" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_012", 0, L"Clothing Item Sp Collectable Hat Mr1 012" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_013", 0, L"Clothing Item Sp Collectable Hat Mr1 013" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_015", 0, L"Clothing Item Sp Collectable Hat Mr1 015" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_016", 0, L"Clothing Item Sp Collectable Hat Mr1 016" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_017_ALT02", 0, L"Clothing Item Sp Collectable Hat Mr1 017 Alt02" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_019_ALT01", 0, L"Clothing Item Sp Collectable Hat Mr1 019 Alt01" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_020", 0, L"Clothing Item Sp Collectable Hat Mr1 020" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_021", 0, L"Clothing Item Sp Collectable Hat Mr1 021" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_025", 0, L"Clothing Item Sp Collectable Hat Mr1 025" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_026", 0, L"Clothing Item Sp Collectable Hat Mr1 026" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_037", 0, L"Clothing Item Sp Collectable Hat Mr1 037" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_041", 0, L"Clothing Item Sp Collectable Hat Mr1 041" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_045", 0, L"Clothing Item Sp Collectable Hat Mr1 045" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_047", 0, L"Clothing Item Sp Collectable Hat Mr1 047" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_048", 0, L"Clothing Item Sp Collectable Hat Mr1 048" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_050", 0, L"Clothing Item Sp Collectable Hat Mr1 050" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_051", 0, L"Clothing Item Sp Collectable Hat Mr1 051" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_052", 0, L"Clothing Item Sp Collectable Hat Mr1 052" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_053_ALT02", 0, L"Clothing Item Sp Collectable Hat Mr1 053 Alt02" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_055", 0, L"Clothing Item Sp Collectable Hat Mr1 055" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_058", 0, L"Clothing Item Sp Collectable Hat Mr1 058" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_060", 0, L"Clothing Item Sp Collectable Hat Mr1 060" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_063_ALT01", 0, L"Clothing Item Sp Collectable Hat Mr1 063 Alt01" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_068", 0, L"Clothing Item Sp Collectable Hat Mr1 068" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_069", 0, L"Clothing Item Sp Collectable Hat Mr1 069" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_075", 0, L"Clothing Item Sp Collectable Hat Mr1 075" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_079", 0, L"Clothing Item Sp Collectable Hat Mr1 079" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_080", 0, L"Clothing Item Sp Collectable Hat Mr1 080" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_086", 0, L"Clothing Item Sp Collectable Hat Mr1 086" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_087", 0, L"Clothing Item Sp Collectable Hat Mr1 087" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_088", 0, L"Clothing Item Sp Collectable Hat Mr1 088" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_089", 0, L"Clothing Item Sp Collectable Hat Mr1 089" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_093", 0, L"Clothing Item Sp Collectable Hat Mr1 093" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_100", 0, L"Clothing Item Sp Collectable Hat Mr1 100" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_102", 0, L"Clothing Item Sp Collectable Hat Mr1 102" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_103", 0, L"Clothing Item Sp Collectable Hat Mr1 103" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_104", 0, L"Clothing Item Sp Collectable Hat Mr1 104" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_105", 0, L"Clothing Item Sp Collectable Hat Mr1 105" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_109", 0, L"Clothing Item Sp Collectable Hat Mr1 109" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_111", 0, L"Clothing Item Sp Collectable Hat Mr1 111" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_112", 0, L"Clothing Item Sp Collectable Hat Mr1 112" },
    { "CLOTHING_ITEM_SP_COLLECTABLE_HAT_MR1_133", 0, L"Clothing Item Sp Collectable Hat Mr1 133" },
    { "CLOTHING_ITEM_SP_EXOTICCOLLECTOR_HAT_000", 0, L"Clothing Item Sp Exoticcollector Hat 000" },
    { "CLOTHING_ITEM_SP_FISHCOLLECTOR_HAT_000", 0, L"Clothing Item Sp Fishcollector Hat 000" },
    { "CLOTHING_ITEM_SP_VALSHERIFF_HAT_000", 0, L"Clothing Item Sp Valsheriff Hat 000" },
    { "CLOTHING_OUTFIT_MP_NED_KELLY", 0, L"Clothing Outfit Mp Ned Kelly" },
    { "CLOTHING_P3_PLAYER_THREE_MS1_HAT_000_000", 0, L"Clothing P3 Player Three Ms1 Hat 000 000" },
    { "CLOTHING_P3_PLAYER_THREE_MS1_HAT_001_000", 0, L"Clothing P3 Player Three Ms1 Hat 001 000" },
    { "CLOTHING_SATCHEL_001", 0, L"Clothing Satchel 001" },
    { "CLOTHING_SATCHEL_003", 0, L"Clothing Satchel 003" },
    { "CLOTHING_SATCHEL_004", 0, L"Clothing Satchel 004" },
    { "CLOTHING_SATCHEL_005", 0, L"Clothing Satchel 005" },
    { "CLOTHING_SATCHEL_006", 0, L"Clothing Satchel 006" },
    { "CLOTHING_SATCHEL_007", 0, L"Clothing Satchel 007" },
    { "CLOTHING_SATCHEL_008", 0, L"Clothing Satchel 008" },
    { "CLOTHING_SP_CHINESE_LABOR_HAT_000_1", 0, L"Clothing Sp Chinese Labor Hat 000 1" },
    { "CLOTHING_SP_CIVIL_WAR_HAT_000_1", 0, L"Clothing Sp Civil War Hat 000 1" },
    { "CLOTHING_SP_CONQUISTADOR_HAT_000_1", 0, L"Clothing Sp Conquistador Hat 000 1" },
    { "CLOTHING_SP_DEAD_MINER_HAT_000_1", 0, L"Clothing Sp Dead Miner Hat 000 1" },
    { "CLOTHING_SP_SCARECROW_01_HAT_000_1", 0, L"Clothing Sp Scarecrow 01 Hat 000 1" },
    { "CLOTHING_SP_SCARECROW_02_HAT_000_1", 0, L"Clothing Sp Scarecrow 02 Hat 000 1" },
    { "CLOTHING_SP_SCARECROW_03_HAT_000_1", 0, L"Clothing Sp Scarecrow 03 Hat 000 1" },
    { "CLOTHING_SP_SCARECROW_04_HAT_000_1", 0, L"Clothing Sp Scarecrow 04 Hat 000 1" },
    { "CLOTHING_SP_VIKING_HAT_000_1", 0, L"Clothing Sp Viking Hat 000 1" },
    { "CLOTHING_WEATHER_COLD", 0, L"Clothing Weather Cold" },
    { "CLOTHING_WEATHER_HOT", 0, L"Clothing Weather Hot" },
    { "CONSUMABLE_AGED_PIRATE_RUM", 0, L"Consumable Aged Pirate Rum" },
    { "CONSUMABLE_APPLE", 0, L"Consumable Apple" },
    { "CONSUMABLE_APRICOTS_CAN", 0, L"Consumable Apricots Can" },
    { "CONSUMABLE_BAKED_BEANS_CAN", 0, L"Consumable Baked Beans Can" },
    { "CONSUMABLE_BEETS", 0, L"Consumable Beets" },
    { "CONSUMABLE_BIG_GAME_MEAT_OREGANO_COOKED", 0, L"Consumable Big Game Meat Oregano Cooked" },
    { "CONSUMABLE_BIG_GAME_MEAT_THYME_COOKED", 0, L"Consumable Big Game Meat Thyme Cooked" },
    { "CONSUMABLE_BIG_GAME_MEAT_WILD_MINT_COOKED", 0, L"Consumable Big Game Meat Wild Mint Cooked" },
    { "CONSUMABLE_BISCUIT_BOX", 0, L"Consumable Biscuit Box" },
    { "CONSUMABLE_BRANDY", 0, L"Consumable Brandy" },
    { "CONSUMABLE_BRANDY_USED", 0, L"Consumable Brandy Used" },
    { "CONSUMABLE_BREAD_CHUNK", 0, L"Consumable Bread Chunk" },
    { "CONSUMABLE_BREAD_ROLL", 0, L"Consumable Bread Roll" },
    { "CONSUMABLE_CANDY_BAG", 0, L"Consumable Candy Bag" },
    { "CONSUMABLE_CARROT", 0, L"Consumable Carrot" },
    { "CONSUMABLE_CELERY", 0, L"Consumable Celery" },
    { "CONSUMABLE_CHEESE_WEDGE", 0, L"Consumable Cheese Wedge" },
    { "CONSUMABLE_CHEWING_TOBACCO", 0, L"Consumable Chewing Tobacco" },
    { "CONSUMABLE_CHEWING_TOBACCO_USED", 0, L"Consumable Chewing Tobacco Used" },
    { "CONSUMABLE_CHOCOLATE_BAR", 0, L"Consumable Chocolate Bar" },
    { "CONSUMABLE_CIGAR", 0, L"Consumable Cigar" },
    { "CONSUMABLE_CIGARETTE_BOX", 0, L"Consumable Cigarette Box" },
    { "CONSUMABLE_CIGARETTE_BOX_CHEAP", 0, L"Consumable Cigarette Box Cheap" },
    { "CONSUMABLE_COCAINE_CHEWING_GUM_USED", 0, L"Consumable Cocaine Chewing Gum Used" },
    { "CONSUMABLE_COFFEE", 0, L"Consumable Coffee" },
    { "CONSUMABLE_COFFEE_GNDS_REG", 0, L"Consumable Coffee Gnds Reg" },
    { "CONSUMABLE_CORN", 0, L"Consumable Corn" },
    { "CONSUMABLE_CORNEDBEEF_CAN", 0, L"Consumable Cornedbeef Can" },
    { "CONSUMABLE_CRACKERS", 0, L"Consumable Crackers" },
    { "CONSUMABLE_CRAFTED_SUPER_MEAL", 0, L"Consumable Crafted Super Meal" },
    { "CONSUMABLE_CRUSTACEAN_MEAT_MINT_COOKED", 0, L"Consumable Crustacean Meat Mint Cooked" },
    { "CONSUMABLE_CRUSTACEAN_MEAT_OREGANO_COOKED", 0, L"Consumable Crustacean Meat Oregano Cooked" },
    { "CONSUMABLE_CRUSTACEAN_MEAT_THYME_COOKED", 0, L"Consumable Crustacean Meat Thyme Cooked" },
    { "CONSUMABLE_EXOTIC_BIRD_OREGANO_COOKED", 0, L"Consumable Exotic Bird Oregano Cooked" },
    { "CONSUMABLE_EXOTIC_BIRD_THYME_COOKED", 0, L"Consumable Exotic Bird Thyme Cooked" },
    { "CONSUMABLE_EXOTIC_BIRD_WILD_MINT_COOKED", 0, L"Consumable Exotic Bird Wild Mint Cooked" },
    { "CONSUMABLE_FLAKEY_FISH_OREGANO_COOKED", 0, L"Consumable Flakey Fish Oregano Cooked" },
    { "CONSUMABLE_FLAKEY_FISH_THYME_COOKED", 0, L"Consumable Flakey Fish Thyme Cooked" },
    { "CONSUMABLE_FLAKEY_FISH_WILD_MINT_COOKED", 0, L"Consumable Flakey Fish Wild Mint Cooked" },
    { "CONSUMABLE_GAME_MEAT_OREGANO_COOKED", 0, L"Consumable Game Meat Oregano Cooked" },
    { "CONSUMABLE_GAME_MEAT_THYME_COOKED", 0, L"Consumable Game Meat Thyme Cooked" },
    { "CONSUMABLE_GAME_MEAT_WILD_MINT_COOKED", 0, L"Consumable Game Meat Wild Mint Cooked" },
    { "CONSUMABLE_GIN", 0, L"Consumable Gin" },
    { "CONSUMABLE_GIN_USED", 0, L"Consumable Gin Used" },
    { "CONSUMABLE_GINSENG_ELIXIER", 0, L"Consumable Ginseng Elixier" },
    { "CONSUMABLE_GRISTLY_MUTTON_OREGANO_COOKED", 0, L"Consumable Gristly Mutton Oregano Cooked" },
    { "CONSUMABLE_GRISTLY_MUTTON_THYME_COOKED", 0, L"Consumable Gristly Mutton Thyme Cooked" },
    { "CONSUMABLE_GRISTLY_MUTTON_WILD_MINT_COOKED", 0, L"Consumable Gristly Mutton Wild Mint Cooked" },
    { "CONSUMABLE_HAIR_GREASE", 0, L"Consumable Hair Grease" },
    { "CONSUMABLE_HAYCUBE", 0, L"Consumable Haycube" },
    { "CONSUMABLE_HEALTH_SERUM", 0, L"Consumable Health Serum" },
    { "CONSUMABLE_HERB_ALASKAN_GINSENG", 0, L"Consumable Herb Alaskan Ginseng" },
    { "CONSUMABLE_HERB_AMERICAN_GINSENG", 0, L"Consumable Herb American Ginseng" },
    { "CONSUMABLE_HERB_BAY_BOLETE", 0, L"Consumable Herb Bay Bolete" },
    { "CONSUMABLE_HERB_BLACK_BERRY", 0, L"Consumable Herb Black Berry" },
    { "CONSUMABLE_HERB_BLACK_CURRANT", 0, L"Consumable Herb Black Currant" },
    { "CONSUMABLE_HERB_BURDOCK_ROOT", 0, L"Consumable Herb Burdock Root" },
    { "CONSUMABLE_HERB_CHANTERELLES", 0, L"Consumable Herb Chanterelles" },
    { "CONSUMABLE_HERB_COMMON_BULRUSH", 0, L"Consumable Herb Common Bulrush" },
    { "CONSUMABLE_HERB_CREEPING_THYME", 0, L"Consumable Herb Creeping Thyme" },
    { "CONSUMABLE_HERB_CURRANT", 0, L"Consumable Herb Currant" },
    { "CONSUMABLE_HERB_DESERT_SAGE", 0, L"Consumable Herb Desert Sage" },
    { "CONSUMABLE_HERB_ENGLISH_MACE", 0, L"Consumable Herb English Mace" },
    { "CONSUMABLE_HERB_EVERGREEN_HUCKLEBERRY", 0, L"Consumable Herb Evergreen Huckleberry" },
    { "CONSUMABLE_HERB_GINSENG", 0, L"Consumable Herb Ginseng" },
    { "CONSUMABLE_HERB_GOLDEN_CURRANT", 0, L"Consumable Herb Golden Currant" },
    { "CONSUMABLE_HERB_HUMMINGBIRD_SAGE", 0, L"Consumable Herb Hummingbird Sage" },
    { "CONSUMABLE_HERB_INDIAN_TOBACCO", 0, L"Consumable Herb Indian Tobacco" },
    { "CONSUMABLE_HERB_MILKWEED", 0, L"Consumable Herb Milkweed" },
    { "CONSUMABLE_HERB_OLEANDER_SAGE", 0, L"Consumable Herb Oleander Sage" },
    { "CONSUMABLE_HERB_OREGANO", 0, L"Consumable Herb Oregano" },
    { "CONSUMABLE_HERB_PARASOL_MUSHROOM", 0, L"Consumable Herb Parasol Mushroom" },
    { "CONSUMABLE_HERB_PRAIRIE_POPPY", 0, L"Consumable Herb Prairie Poppy" },
    { "CONSUMABLE_HERB_RAMS_HEAD", 0, L"Consumable Herb Rams Head" },
    { "CONSUMABLE_HERB_RED_RASPBERRY", 0, L"Consumable Herb Red Raspberry" },
    { "CONSUMABLE_HERB_RED_SAGE", 0, L"Consumable Herb Red Sage" },
    { "CONSUMABLE_HERB_SAGE", 0, L"Consumable Herb Sage" },
    { "CONSUMABLE_HERB_SALTBUSH", 0, L"Consumable Herb Saltbush" },
    { "CONSUMABLE_HERB_VANILLA_FLOWER", 0, L"Consumable Herb Vanilla Flower" },
    { "CONSUMABLE_HERB_VIOLET_SNOWDROP", 0, L"Consumable Herb Violet Snowdrop" },
    { "CONSUMABLE_HERB_WILD_CARROTS", 0, L"Consumable Herb Wild Carrots" },
    { "CONSUMABLE_HERB_WILD_FEVERFEW", 0, L"Consumable Herb Wild Feverfew" },
    { "CONSUMABLE_HERB_WILD_MINT", 0, L"Consumable Herb Wild Mint" },
    { "CONSUMABLE_HERB_WINTERGREEN_BERRY", 0, L"Consumable Herb Wintergreen Berry" },
    { "CONSUMABLE_HERB_YARROW", 0, L"Consumable Herb Yarrow" },
    { "CONSUMABLE_HERBIVORE_BAIT", 0, L"Consumable Herbivore Bait" },
    { "CONSUMABLE_HORSE_MEDICINE_USED", 0, L"Consumable Horse Medicine Used" },
    { "CONSUMABLE_HORSE_OINTMENT", 0, L"Consumable Horse Ointment" },
    { "CONSUMABLE_HORSE_STIMULANT_USED", 0, L"Consumable Horse Stimulant Used" },
    { "CONSUMABLE_JERKY", 0, L"Consumable Jerky" },
    { "CONSUMABLE_JERKY_VENISON", 0, L"Consumable Jerky Venison" },
    { "CONSUMABLE_KIDNEYBEANS_CAN", 0, L"Consumable Kidneybeans Can" },
    { "CONSUMABLE_LOCK_BREAKER", 0, L"Consumable Lock Breaker" },
    { "CONSUMABLE_MATURE_VENISON_OREGANO_COOKED", 0, L"Consumable Mature Venison Oregano Cooked" },
    { "CONSUMABLE_MATURE_VENISON_THYME_COOKED", 0, L"Consumable Mature Venison Thyme Cooked" },
    { "CONSUMABLE_MATURE_VENISON_WILD_MINT_COOKED", 0, L"Consumable Mature Venison Wild Mint Cooked" },
    { "CONSUMABLE_MEAT_BIG_GAME_COOKED", 0, L"Consumable Meat Big Game Cooked" },
    { "CONSUMABLE_MEAT_CRUSTACEAN_COOKED", 0, L"Consumable Meat Crustacean Cooked" },
    { "CONSUMABLE_MEAT_EXOTIC_BIRD_COOKED", 0, L"Consumable Meat Exotic Bird Cooked" },
    { "CONSUMABLE_MEAT_FLAKEY_FISH_COOKED", 0, L"Consumable Meat Flakey Fish Cooked" },
    { "CONSUMABLE_MEAT_GAME_COOKED", 0, L"Consumable Meat Game Cooked" },
    { "CONSUMABLE_MEAT_GAMEY_BIRD_COOKED", 0, L"Consumable Meat Gamey Bird Cooked" },
    { "CONSUMABLE_MEAT_GRISTLY_MUTTON_COOKED", 0, L"Consumable Meat Gristly Mutton Cooked" },
    { "CONSUMABLE_MEAT_GRITTY_FISH_COOKED", 0, L"Consumable Meat Gritty Fish Cooked" },
    { "CONSUMABLE_MEAT_HERPTILE_COOKED", 0, L"Consumable Meat Herptile Cooked" },
    { "CONSUMABLE_MEAT_MATURE_VENISON_COOKED", 0, L"Consumable Meat Mature Venison Cooked" },
    { "CONSUMABLE_MEAT_PLUMP_BIRD_COOKED", 0, L"Consumable Meat Plump Bird Cooked" },
    { "CONSUMABLE_MEAT_PRIME_BEEF_COOKED", 0, L"Consumable Meat Prime Beef Cooked" },
    { "CONSUMABLE_MEAT_STRINGY_COOKED", 0, L"Consumable Meat Stringy Cooked" },
    { "CONSUMABLE_MEAT_SUCCULENT_FISH_COOKED", 0, L"Consumable Meat Succulent Fish Cooked" },
    { "CONSUMABLE_MEAT_TENDER_PORK_COOKED", 0, L"Consumable Meat Tender Pork Cooked" },
    { "CONSUMABLE_MEDICINE_USED", 0, L"Consumable Medicine Used" },
    { "CONSUMABLE_MOONSHINE", 0, L"Consumable Moonshine" },
    { "CONSUMABLE_OAT_CAKES", 0, L"Consumable Oat Cakes" },
    { "CONSUMABLE_OFFAL", 0, L"Consumable Offal" },
    { "CONSUMABLE_PEACH", 0, L"Consumable Peach" },
    { "CONSUMABLE_PEACHES_CAN", 0, L"Consumable Peaches Can" },
    { "CONSUMABLE_PEAR", 0, L"Consumable Pear" },
    { "CONSUMABLE_PEAS_CAN", 0, L"Consumable Peas Can" },
    { "CONSUMABLE_PEPPERMINT", 0, L"Consumable Peppermint" },
    { "CONSUMABLE_PINEAPPLES_CAN", 0, L"Consumable Pineapples Can" },
    { "CONSUMABLE_PLUMP_BIRD_OREGANO_COOKED", 0, L"Consumable Plump Bird Oregano Cooked" },
    { "CONSUMABLE_PLUMP_BIRD_THYME_COOKED", 0, L"Consumable Plump Bird Thyme Cooked" },
    { "CONSUMABLE_PLUMP_BIRD_WILD_MINT_COOKED", 0, L"Consumable Plump Bird Wild Mint Cooked" },
    { "CONSUMABLE_POISON_TONIC", 0, L"Consumable Poison Tonic" },
    { "CONSUMABLE_PORK_COOKED", 0, L"Consumable Pork Cooked" },
    { "CONSUMABLE_POTENT_HERBIVORE_BAIT", 0, L"Consumable Potent Herbivore Bait" },
    { "CONSUMABLE_POTENT_HORSE_MEDICINE", 0, L"Consumable Potent Horse Medicine" },
    { "CONSUMABLE_POTENT_HORSE_STIMULANT", 0, L"Consumable Potent Horse Stimulant" },
    { "CONSUMABLE_POTENT_PREDATOR_BAIT", 0, L"Consumable Potent Predator Bait" },
    { "CONSUMABLE_POTENT_RESTORATIVE", 0, L"Consumable Potent Restorative" },
    { "CONSUMABLE_POTENT_SNAKE_OIL", 0, L"Consumable Potent Snake Oil" },
    { "CONSUMABLE_POTENT_TONIC", 0, L"Consumable Potent Tonic" },
    { "CONSUMABLE_PREDATOR_BAIT", 0, L"Consumable Predator Bait" },
    { "CONSUMABLE_PRIME_BEEF_OREGANO_COOKED", 0, L"Consumable Prime Beef Oregano Cooked" },
    { "CONSUMABLE_PRIME_BEEF_THYME_COOKED", 0, L"Consumable Prime Beef Thyme Cooked" },
    { "CONSUMABLE_PRIME_BEEF_WILD_MINT_COOKED", 0, L"Consumable Prime Beef Wild Mint Cooked" },
    { "CONSUMABLE_RESTORATIVE", 0, L"Consumable Restorative" },
    { "CONSUMABLE_RESTORATIVE_USED", 0, L"Consumable Restorative Used" },
    { "CONSUMABLE_RUM", 0, L"Consumable Rum" },
    { "CONSUMABLE_RUM_USED", 0, L"Consumable Rum Used" },
    { "CONSUMABLE_SALMON_CAN", 0, L"Consumable Salmon Can" },
    { "CONSUMABLE_SCENT_BLOCKER", 0, L"Consumable Scent Blocker" },
    { "CONSUMABLE_SCENT_BLOCKER_USED", 0, L"Consumable Scent Blocker Used" },
    { "CONSUMABLE_SEASONING", 0, L"Consumable Seasoning" },
    { "CONSUMABLE_SNAKE_OIL_USED", 0, L"Consumable Snake Oil Used" },
    { "CONSUMABLE_SPECIAL_HORSE_MEDICINE", 0, L"Consumable Special Horse Medicine" },
    { "CONSUMABLE_SPECIAL_HORSE_REVIVER_CRAFTED", 0, L"Consumable Special Horse Reviver Crafted" },
    { "CONSUMABLE_SPECIAL_HORSE_STIMULANT_CRAFTED", 0, L"Consumable Special Horse Stimulant Crafted" },
    { "CONSUMABLE_SPECIAL_RESTORATIVE", 0, L"Consumable Special Restorative" },
    { "CONSUMABLE_SPECIAL_SNAKE_OIL", 0, L"Consumable Special Snake Oil" },
    { "CONSUMABLE_SPECIAL_TONIC", 0, L"Consumable Special Tonic" },
    { "CONSUMABLE_STRAWBERRIES_CAN", 0, L"Consumable Strawberries Can" },
    { "CONSUMABLE_SUCCULENT_FISH_OREGANO_COOKED", 0, L"Consumable Succulent Fish Oregano Cooked" },
    { "CONSUMABLE_SUCCULENT_FISH_THYME_COOKED", 0, L"Consumable Succulent Fish Thyme Cooked" },
    { "CONSUMABLE_SUCCULENT_FISH_WILD_MINT_COOKED", 0, L"Consumable Succulent Fish Wild Mint Cooked" },
    { "CONSUMABLE_SUGARCUBE", 0, L"Consumable Sugarcube" },
    { "CONSUMABLE_SWEET_CORN_CAN", 0, L"Consumable Sweet Corn Can" },
    { "CONSUMABLE_TENDER_PORK_OREGANO_COOKED", 0, L"Consumable Tender Pork Oregano Cooked" },
    { "CONSUMABLE_TENDER_PORK_THYME_COOKED", 0, L"Consumable Tender Pork Thyme Cooked" },
    { "CONSUMABLE_TENDER_PORK_WILD_MINT_COOKED", 0, L"Consumable Tender Pork Wild Mint Cooked" },
    { "CONSUMABLE_TONIC", 0, L"Consumable Tonic" },
    { "CONSUMABLE_TONIC_USED", 0, L"Consumable Tonic Used" },
    { "CONSUMABLE_VALERIAN_ROOT", 0, L"Consumable Valerian Root" },
    { "CONSUMABLE_WHISKEY", 0, L"Consumable Whiskey" },
    { "CONSUMABLE_WHISKEY_USED", 0, L"Consumable Whiskey Used" },
    { "DOCUMENT_ARTHUR_HAS_POSTER_1", 0, L"Document Arthur Has Poster 1" },
    { "DOCUMENT_ARTHUR_HAS_POSTER_2", 0, L"Document Arthur Has Poster 2" },
    { "DOCUMENT_ARTHUR_HIGH_BOUNTY_1", 0, L"Document Arthur High Bounty 1" },
    { "DOCUMENT_ARTHUR_HIGH_BOUNTY_2", 0, L"Document Arthur High Bounty 2" },
    { "DOCUMENT_BOUNTY_POSTER", 0, L"Document Bounty Poster" },
    { "DOCUMENT_BUSINESS_CARD_ROCK_CARVINGS", 0, L"Document Business Card Rock Carvings" },
    { "DOCUMENT_CIG_CARD_ACT", 0, L"Document Cig Card Act" },
    { "DOCUMENT_CIG_CARD_AML", 0, L"Document Cig Card Aml" },
    { "DOCUMENT_CIG_CARD_ART", 0, L"Document Cig Card Art" },
    { "DOCUMENT_CIG_CARD_GRL", 0, L"Document Cig Card Grl" },
    { "DOCUMENT_CIG_CARD_GUN", 0, L"Document Cig Card Gun" },
    { "DOCUMENT_CIG_CARD_HOR", 0, L"Document Cig Card Hor" },
    { "DOCUMENT_CIG_CARD_INV", 0, L"Document Cig Card Inv" },
    { "DOCUMENT_CIG_CARD_LND", 0, L"Document Cig Card Lnd" },
    { "DOCUMENT_CIG_CARD_PAM", 0, L"Document Cig Card Pam" },
    { "DOCUMENT_CIG_CARD_PLT", 0, L"Document Cig Card Plt" },
    { "DOCUMENT_CIG_CARD_SPT", 0, L"Document Cig Card Spt" },
    { "DOCUMENT_CIG_CARD_VEH", 0, L"Document Cig Card Veh" },
    { "DOCUMENT_DISCO_FORTUNE", 0, L"Document Disco Fortune" },
    { "DOCUMENT_DISCO_FRANKENSTEIN_LETTER", 0, L"Document Disco Frankenstein Letter" },
    { "DOCUMENT_HORSE_DEED", 0, L"Document Horse Deed" },
    { "DOCUMENT_MAP_GENERIC", 0, L"Document Map Generic" },
    { "DOCUMENT_NEWSPAPER", 0, L"Document Newspaper" },
    { "DOCUMENT_PLAYER_JOURNAL", 0, L"Document Player Journal" },
    { "DOCUMENT_RCM_FORMYART_PAINTING", 0, L"Document Rcm Formyart Painting" },
    { "DOCUMENT_RCM_INVENTOR_PLANS", 0, L"Document Rcm Inventor Plans" },
    { "DOCUMENT_WILD_MAN_JOURNAL", 0, L"Document Wild Man Journal" },
    { "FOLDER_BOOKS", 0, L"Folder Books" },
    { "FOLDER_BOUNTY_POSTERS", 0, L"Folder Bounty Posters" },
    { "FOLDER_BUSINESS_CARDS", 0, L"Folder Business Cards" },
    { "FOLDER_CIG_CARD_ACT_SET", 0, L"Folder Cig Card Act Set" },
    { "FOLDER_CIG_CARD_AML_SET", 0, L"Folder Cig Card Aml Set" },
    { "FOLDER_CIG_CARD_ART_SET", 0, L"Folder Cig Card Art Set" },
    { "FOLDER_CIG_CARD_GRL_SET", 0, L"Folder Cig Card Grl Set" },
    { "FOLDER_CIG_CARD_GUN_SET", 0, L"Folder Cig Card Gun Set" },
    { "FOLDER_CIG_CARD_HOR_SET", 0, L"Folder Cig Card Hor Set" },
    { "FOLDER_CIG_CARD_INV_SET", 0, L"Folder Cig Card Inv Set" },
    { "FOLDER_CIG_CARD_LND_SET", 0, L"Folder Cig Card Lnd Set" },
    { "FOLDER_CIG_CARD_PAM_SET", 0, L"Folder Cig Card Pam Set" },
    { "FOLDER_CIG_CARD_PLT_SET", 0, L"Folder Cig Card Plt Set" },
    { "FOLDER_CIG_CARD_SPT_SET", 0, L"Folder Cig Card Spt Set" },
    { "FOLDER_CIG_CARD_VEH_SET", 0, L"Folder Cig Card Veh Set" },
    { "FOLDER_DINOSAUR_NOTES", 0, L"Folder Dinosaur Notes" },
    { "FOLDER_DRAWINGS", 0, L"Folder Drawings" },
    { "FOLDER_EXOTIC_ORDERS", 0, L"Folder Exotic Orders" },
    { "FOLDER_HANDBILLS", 0, L"Folder Handbills" },
    { "FOLDER_INVITATIONS", 0, L"Folder Invitations" },
    { "FOLDER_KIT_KEEPSAKES", 0, L"Folder Kit Keepsakes" },
    { "FOLDER_KIT_KEYCHAIN", 0, L"Folder Kit Keychain" },
    { "FOLDER_LETTERS", 0, L"Folder Letters" },
    { "FOLDER_MAPS", 0, L"Folder Maps" },
    { "FOLDER_NEWSPAPER_SCRAPS", 0, L"Folder Newspaper Scraps" },
    { "FOLDER_NEWSPAPERS", 0, L"Folder Newspapers" },
    { "FOLDER_NOTES", 0, L"Folder Notes" },
    { "FOLDER_PHOTOGRAPHS", 0, L"Folder Photographs" },
    { "FOLDER_RARE_ORCHID_ORDERS", 0, L"Folder Rare Orchid Orders" },
    { "FOLDER_RECIPE_PAMPHLETS", 0, L"Folder Recipe Pamphlets" },
    { "FOLDER_TAXIDERMIST_ORDERS", 0, L"Folder Taxidermist Orders" },
    { "FOLDER_TREASURE_MAPS", 0, L"Folder Treasure Maps" },
    { "GENERIC_BOOK", 0, L"Generic Book" },
    { "GENERIC_BOTTLE", 0, L"Generic Bottle" },
    { "GENERIC_DINOSAUR_NOTE", 0, L"Generic Dinosaur Note" },
    { "GENERIC_ENVELOPE", 0, L"Generic Envelope" },
    { "GENERIC_EXOTIC_ORDER", 0, L"Generic Exotic Order" },
    { "GENERIC_HANDBILL", 0, L"Generic Handbill" },
    { "GENERIC_HORSE_EQUIP_BEDROLL", 0, L"Generic Horse Equip Bedroll" },
    { "GENERIC_HORSE_EQUIP_BLANKET", 0, L"Generic Horse Equip Blanket" },
    { "GENERIC_HORSE_EQUIP_HORN", 0, L"Generic Horse Equip Horn" },
    { "GENERIC_HORSE_EQUIP_MANE", 0, L"Generic Horse Equip Mane" },
    { "GENERIC_HORSE_EQUIP_SADDLE", 0, L"Generic Horse Equip Saddle" },
    { "GENERIC_HORSE_EQUIP_SADDLEBAG", 0, L"Generic Horse Equip Saddlebag" },
    { "GENERIC_HORSE_EQUIP_STIRRUP", 0, L"Generic Horse Equip Stirrup" },
    { "GENERIC_HORSE_EQUIP_TAIL", 0, L"Generic Horse Equip Tail" },
    { "GENERIC_INVITATION", 0, L"Generic Invitation" },
    { "GENERIC_NEWSCLIPPING", 0, L"Generic Newsclipping" },
    { "GENERIC_NOTE", 0, L"Generic Note" },
    { "GENERIC_NOTE_SMALL", 0, L"Generic Note Small" },
    { "GENERIC_OFFICIAL_DOC", 0, L"Generic Official Doc" },
    { "GENERIC_ORCHID_ORDER", 0, L"Generic Orchid Order" },
    { "GENERIC_PAMPHLET", 0, L"Generic Pamphlet" },
    { "GENERIC_PHOTOGRAPH", 0, L"Generic Photograph" },
    { "GENERIC_TAXIDERMIST_ORDER", 0, L"Generic Taxidermist Order" },
    { "GENERIC_TICKET", 0, L"Generic Ticket" },
    { "GENERIC_TREASURE_MAP", 0, L"Generic Treasure Map" },
    { "KIT_BANDANA", 0, L"Kit Bandana" },
    { "KIT_BRASS_KNUCKLES", 0, L"Kit Brass Knuckles" },
    { "KIT_CAMP", 0, L"Kit Camp" },
    { "KIT_CAMP_SIMPLE", 0, L"Kit Camp Simple" },
    { "KIT_CUSTOM_SATCHEL", 0, L"Kit Custom Satchel" },
    { "KIT_GUN_OIL", 0, L"Kit Gun Oil" },
    { "KIT_HARMONICA", 0, L"Kit Harmonica" },
    { "KIT_HORSE_BRUSH", 0, L"Kit Horse Brush" },
    { "KIT_LIGHTNING_CONDUCTOR", 0, L"Kit Lightning Conductor" },
    { "KIT_MASK_BLACK_HOOD", 0, L"Kit Mask Black Hood" },
    { "KIT_MASK_BROWN_SACK", 0, L"Kit Mask Brown Sack" },
    { "KIT_MASK_GREY_CLOTH", 0, L"Kit Mask Grey Cloth" },
    { "KIT_MASK_METAL", 0, L"Kit Mask Metal" },
    { "KIT_MASK_PSYCHO", 0, L"Kit Mask Psycho" },
    { "KIT_OATS_BAG", 0, L"Kit Oats Bag" },
    { "KIT_PLAYER_POCKETWATCH", 0, L"Kit Player Pocketwatch" },
    { "KIT_POUCH_AMMO", 0, L"Kit Pouch Ammo" },
    { "KIT_POUCH_INGREDIENTS", 0, L"Kit Pouch Ingredients" },
    { "KIT_POUCH_KIT", 0, L"Kit Pouch Kit" },
    { "KIT_POUCH_LEGENDARY", 0, L"Kit Pouch Legendary" },
    { "KIT_POUCH_MATERIALS", 0, L"Kit Pouch Materials" },
    { "KIT_POUCH_PROVISIONS", 0, L"Kit Pouch Provisions" },
    { "KIT_POUCH_REMEDIES", 0, L"Kit Pouch Remedies" },
    { "KIT_POUCH_VALUABLES", 0, L"Kit Pouch Valuables" },
    { "KIT_POUCH_WEAPONS", 0, L"Kit Pouch Weapons" },
    { "KIT_SHAVING_KIT", 0, L"Kit Shaving Kit" },
    { "KIT_TOBACCO_PIPE", 0, L"Kit Tobacco Pipe" },
    { "KIT_UPGRADE_CAMP", 0, L"Kit Upgrade Camp" },
    { "KIT_UPGRADE_CAMP_TABLE", 0, L"Kit Upgrade Camp Table" },
    { "KIT_UPGRADE_CAMP_WAGON", 0, L"Kit Upgrade Camp Wagon" },
    { "KIT_WARDROBE", 0, L"Kit Wardrobe" },
    { "MONEY_BILLSTACK", 0, L"Money Billstack" },
    { "MONEY_COIN", 0, L"Money Coin" },
    { "MONEY_COINPURSE", 0, L"Money Coinpurse" },
    { "MONEY_COINSTACK", 0, L"Money Coinstack" },
    { "MONEY_LOANSHARK_GWEN_DEBT", 0, L"Money Loanshark Gwen Debt" },
    { "MONEY_MONEYCLIP", 0, L"Money Moneyclip" },
    { "MONEY_MONEYSTACK", 0, L"Money Moneystack" },
    { "PROVISION_ASTEROID_CHUNK", 0, L"Provision Asteroid Chunk" },
    { "PROVISION_BEAUS_GIFT_1", 0, L"Provision Beaus Gift 1" },
    { "PROVISION_BEAUS_GIFT_2", 0, L"Provision Beaus Gift 2" },
    { "PROVISION_BEAUS_POSSESSIONS", 0, L"Provision Beaus Possessions" },
    { "PROVISION_BRA_SHIELD", 0, L"Provision Bra Shield" },
    { "PROVISION_BRACELET_GOLD", 0, L"Provision Bracelet Gold" },
    { "PROVISION_BRACELET_PLATINUM", 0, L"Provision Bracelet Platinum" },
    { "PROVISION_BRACELET_SILVER", 0, L"Provision Bracelet Silver" },
    { "PROVISION_BUCKLE_GOLD", 0, L"Provision Buckle Gold" },
    { "PROVISION_BUCKLE_PLATINUM", 0, L"Provision Buckle Platinum" },
    { "PROVISION_BUCKLE_SILVER", 0, L"Provision Buckle Silver" },
    { "PROVISION_CALDERON_CROSS", 0, L"Provision Calderon Cross" },
    { "PROVISION_CC_VINTAGE_HANDCUFFS", 0, L"Provision Cc Vintage Handcuffs" },
    { "PROVISION_CHICKEN_EGG", 0, L"Provision Chicken Egg" },
    { "PROVISION_DB_FINGER_BONE", 0, L"Provision Db Finger Bone" },
    { "PROVISION_DB_SKULL_STATUE", 0, L"Provision Db Skull Statue" },
    { "PROVISION_DEPUTY_STAR", 0, L"Provision Deputy Star" },
    { "PROVISION_DIAMOND", 0, L"Provision Diamond" },
    { "PROVISION_DIAMOND_RING", 0, L"Provision Diamond Ring" },
    { "PROVISION_DISCO_AMMOLITE", 0, L"Provision Disco Ammolite" },
    { "PROVISION_DISCO_ANCIENT_EAGLE", 0, L"Provision Disco Ancient Eagle" },
    { "PROVISION_DISCO_ANCIENT_NECKLACE", 0, L"Provision Disco Ancient Necklace" },
    { "PROVISION_DISCO_ARROWHEAD_01", 0, L"Provision Disco Arrowhead 01" },
    { "PROVISION_DISCO_ARROWHEAD_02", 0, L"Provision Disco Arrowhead 02" },
    { "PROVISION_DISCO_ARROWHEAD_03", 0, L"Provision Disco Arrowhead 03" },
    { "PROVISION_DISCO_EMERALD", 0, L"Provision Disco Emerald" },
    { "PROVISION_DISCO_FERTILITY_STATUE", 0, L"Provision Disco Fertility Statue" },
    { "PROVISION_DISCO_FLUORITE", 0, L"Provision Disco Fluorite" },
    { "PROVISION_DISCO_GATOR_EGG", 0, L"Provision Disco Gator Egg" },
    { "PROVISION_DISCO_SHOES", 0, L"Provision Disco Shoes" },
    { "PROVISION_DISCO_SHRUNKEN_HEAD", 0, L"Provision Disco Shrunken Head" },
    { "PROVISION_DISCO_URN", 0, L"Provision Disco Urn" },
    { "PROVISION_DISCO_VIKING_COMB", 0, L"Provision Disco Viking Comb" },
    { "PROVISION_DOG_TAG", 0, L"Provision Dog Tag" },
    { "PROVISION_EARRING_GOLD", 0, L"Provision Earring Gold" },
    { "PROVISION_EARRING_PEARL", 0, L"Provision Earring Pearl" },
    { "PROVISION_EARRING_PLATINUM", 0, L"Provision Earring Platinum" },
    { "PROVISION_EARRING_SILVER", 0, L"Provision Earring Silver" },
    { "PROVISION_FISH_CATFISH_LEGENDARY", 0, L"Provision Fish Catfish Legendary" },
    { "PROVISION_FISH_CHAIN_PICKEREL_LEGENDARY", 0, L"Provision Fish Chain Pickerel Legendary" },
    { "PROVISION_FISH_FIN_PICKEREL_LEGENDARY", 0, L"Provision Fish Fin Pickerel Legendary" },
    { "PROVISION_FISH_LAKE_STURGEON_LEGENDARY", 0, L"Provision Fish Lake Sturgeon Legendary" },
    { "PROVISION_FISH_LMBASS_LEGENDARY", 0, L"Provision Fish Lmbass Legendary" },
    { "PROVISION_FISH_LONG_NOSE_GAR_LEGENDARY", 0, L"Provision Fish Long Nose Gar Legendary" },
    { "PROVISION_FISH_LONGNOSE_GAR", 0, L"Provision Fish Longnose Gar" },
    { "PROVISION_FISH_MEAT", 0, L"Provision Fish Meat" },
    { "PROVISION_FISH_MUSKIE_LEGENDARY", 0, L"Provision Fish Muskie Legendary" },
    { "PROVISION_FISH_NORTHERN_PIKE", 0, L"Provision Fish Northern Pike" },
    { "PROVISION_FISH_NORTHERN_PIKE_LEGENDARY", 0, L"Provision Fish Northern Pike Legendary" },
    { "PROVISION_FISH_PERCH_LEGENDARY", 0, L"Provision Fish Perch Legendary" },
    { "PROVISION_FISH_REDFIN_PICKEREL", 0, L"Provision Fish Redfin Pickerel" },
    { "PROVISION_FISH_ROCK_BASS_LEGENDARY", 0, L"Provision Fish Rock Bass Legendary" },
    { "PROVISION_FISH_SALMON_LEGENDARY", 0, L"Provision Fish Salmon Legendary" },
    { "PROVISION_FISH_SM_BASS_LEGENDARY", 0, L"Provision Fish Sm Bass Legendary" },
    { "PROVISION_FISH_SPECKLED_TROUT", 0, L"Provision Fish Speckled Trout" },
    { "PROVISION_FISH_STEELHEAD_BLUEGILL_LEGENDARY", 0, L"Provision Fish Steelhead Bluegill Legendary" },
    { "PROVISION_FISH_STEELHEAD_TROUT_LEGENDARY", 0, L"Provision Fish Steelhead Trout Legendary" },
    { "PROVISION_FOLDER_WATCHES", 0, L"Provision Folder Watches" },
    { "PROVISION_FSH_COPPER_SPOOL", 0, L"Provision Fsh Copper Spool" },
    { "PROVISION_FSH_WHITTLED_MINNOW", 0, L"Provision Fsh Whittled Minnow" },
    { "PROVISION_GOLDBAR_SMALL", 0, L"Provision Goldbar Small" },
    { "PROVISION_GOLDEN_WEDDING_CHAIN_RING", 0, L"Provision Golden Wedding Chain Ring" },
    { "PROVISION_GOLDRING", 0, L"Provision Goldring" },
    { "PROVISION_GOLDTOOTH", 0, L"Provision Goldtooth" },
    { "PROVISION_JAIL_KEYS", 0, L"Provision Jail Keys" },
    { "PROVISION_JEWELRY_BOX", 0, L"Provision Jewelry Box" },
    { "PROVISION_JEWELRYBAG_LG", 0, L"Provision Jewelrybag Lg" },
    { "PROVISION_JEWELRYBAG_SM", 0, L"Provision Jewelrybag Sm" },
    { "PROVISION_LOANSHARK_SKINS", 0, L"Provision Loanshark Skins" },
    { "PROVISION_LOCKET_SILVER", 0, L"Provision Locket Silver" },
    { "PROVISION_LOST_RELIC", 0, L"Provision Lost Relic" },
    { "PROVISION_MARYS_BROOCH", 0, L"Provision Marys Brooch" },
    { "PROVISION_MARYS_FOUNTAIN_PEN", 0, L"Provision Marys Fountain Pen" },
    { "PROVISION_MARYS_RING", 0, L"Provision Marys Ring" },
    { "PROVISION_MOLLYS_POCKET_MIRROR", 0, L"Provision Mollys Pocket Mirror" },
    { "PROVISION_NECKLACE", 0, L"Provision Necklace" },
    { "PROVISION_NECKLACE_GOLD", 0, L"Provision Necklace Gold" },
    { "PROVISION_NECKLACE_PEARL", 0, L"Provision Necklace Pearl" },
    { "PROVISION_NECKLACE_PLATINUM", 0, L"Provision Necklace Platinum" },
    { "PROVISION_NECKLACE_SILVER", 0, L"Provision Necklace Silver" },
    { "PROVISION_PEARSONS_NAVAL_COMPASS", 0, L"Provision Pearsons Naval Compass" },
    { "PROVISION_PEN", 0, L"Provision Pen" },
    { "PROVISION_PENELOPES_BRACELET", 0, L"Provision Penelopes Bracelet" },
    { "PROVISION_PENELOPES_NECKLACE", 0, L"Provision Penelopes Necklace" },
    { "PROVISION_PENELOPES_POSSESSIONS", 0, L"Provision Penelopes Possessions" },
    { "PROVISION_PIRATE_RUM", 0, L"Provision Pirate Rum" },
    { "PROVISION_POCKET_WATCH_GLEAMING_BRASS", 0, L"Provision Pocket Watch Gleaming Brass" },
    { "PROVISION_POCKET_WATCH_GOLD", 0, L"Provision Pocket Watch Gold" },
    { "PROVISION_POCKET_WATCH_PLATINUM", 0, L"Provision Pocket Watch Platinum" },
    { "PROVISION_POCKET_WATCH_REUTLINGE", 0, L"Provision Pocket Watch Reutlinge" },
    { "PROVISION_POCKET_WATCH_SILVER", 0, L"Provision Pocket Watch Silver" },
    { "PROVISION_RC_QUARTZ_CHUNK", 0, L"Provision Rc Quartz Chunk" },
    { "PROVISION_RC_ROCK_STATUE", 0, L"Provision Rc Rock Statue" },
    { "PROVISION_RC_SLVCATCHER_WATCH", 0, L"Provision Rc Slvcatcher Watch" },
    { "PROVISION_RCKITTY_EMERALD", 0, L"Provision Rckitty Emerald" },
    { "PROVISION_RCM_OLD_BELONGINGS", 0, L"Provision Rcm Old Belongings" },
    { "PROVISION_RCM_OLD_GUN", 0, L"Provision Rcm Old Gun" },
    { "PROVISION_RCM_POCKET_WATCH", 0, L"Provision Rcm Pocket Watch" },
    { "PROVISION_RCM_WARVETERAN_HARNESS", 0, L"Provision Rcm Warveteran Harness" },
    { "PROVISION_READING_GLASSES", 0, L"Provision Reading Glasses" },
    { "PROVISION_RF_WOOD_COBALT", 0, L"Provision Rf Wood Cobalt" },
    { "PROVISION_RING_PLATINUM", 0, L"Provision Ring Platinum" },
    { "PROVISION_RING_SILVER", 0, L"Provision Ring Silver" },
    { "PROVISION_RO_FLOWER_ACUNAS_STAR", 0, L"Provision Ro Flower Acunas Star" },
    { "PROVISION_RO_FLOWER_CIGAR", 0, L"Provision Ro Flower Cigar" },
    { "PROVISION_RO_FLOWER_CLAMSHELL", 0, L"Provision Ro Flower Clamshell" },
    { "PROVISION_RO_FLOWER_DRAGONS", 0, L"Provision Ro Flower Dragons" },
    { "PROVISION_RO_FLOWER_GHOST", 0, L"Provision Ro Flower Ghost" },
    { "PROVISION_RO_FLOWER_LADY_OF_NIGHT", 0, L"Provision Ro Flower Lady Of Night" },
    { "PROVISION_RO_FLOWER_LADY_SLIPPER", 0, L"Provision Ro Flower Lady Slipper" },
    { "PROVISION_RO_FLOWER_MOCCASIN", 0, L"Provision Ro Flower Moccasin" },
    { "PROVISION_RO_FLOWER_NIGHT_SCENTED", 0, L"Provision Ro Flower Night Scented" },
    { "PROVISION_RO_FLOWER_QUEENS", 0, L"Provision Ro Flower Queens" },
    { "PROVISION_RO_FLOWER_RAT_TAIL", 0, L"Provision Ro Flower Rat Tail" },
    { "PROVISION_RO_FLOWER_SPARROWS", 0, L"Provision Ro Flower Sparrows" },
    { "PROVISION_RO_FLOWER_SPIDER", 0, L"Provision Ro Flower Spider" },
    { "PROVISION_ROTTEN_MEAT", 0, L"Provision Rotten Meat" },
    { "PROVISION_RS_ABALONE_SHELL_FRAGMENT", 0, L"Provision Rs Abalone Shell Fragment" },
    { "PROVISION_SCRAP_METAL", 0, L"Provision Scrap Metal" },
    { "PROVISION_SHERIFF_STAR", 0, L"Provision Sheriff Star" },
    { "PROVISION_SIGNET_RING", 0, L"Provision Signet Ring" },
    { "PROVISION_SILVER_WEDDING_CHAIN_RING", 0, L"Provision Silver Wedding Chain Ring" },
    { "PROVISION_SILVERTOOTH", 0, L"Provision Silvertooth" },
    { "PROVISION_SQUIRREL_TAIL", 0, L"Provision Squirrel Tail" },
    { "PROVISION_TALISMAN_ALLIGATOR_TOOTH", 0, L"Provision Talisman Alligator Tooth" },
    { "PROVISION_TALISMAN_BEAR_CLAW", 0, L"Provision Talisman Bear Claw" },
    { "PROVISION_TALISMAN_BOAR_TUSK", 0, L"Provision Talisman Boar Tusk" },
    { "PROVISION_TALISMAN_BUFFALO_HORN", 0, L"Provision Talisman Buffalo Horn" },
    { "PROVISION_TALISMAN_EAGLE_TALON", 0, L"Provision Talisman Eagle Talon" },
    { "PROVISION_TALISMAN_RAVEN_CLAW", 0, L"Provision Talisman Raven Claw" },
    { "PROVISION_TH_ANTIQUE_BRASS_COMPASS", 0, L"Provision Th Antique Brass Compass" },
    { "PROVISION_THIMBLEAB", 0, L"Provision Thimbleab" },
    { "PROVISION_TM_SQUIRREL_STATUE", 0, L"Provision Tm Squirrel Statue" },
    { "PROVISION_TRINKET_ALLIGATOR_SKIN", 0, L"Provision Trinket Alligator Skin" },
    { "PROVISION_TRINKET_ASTEROID_CHUNK", 0, L"Provision Trinket Asteroid Chunk" },
    { "PROVISION_TRINKET_BADGER_CLAW", 0, L"Provision Trinket Badger Claw" },
    { "PROVISION_TRINKET_BAT_WING", 0, L"Provision Trinket Bat Wing" },
    { "PROVISION_TRINKET_BEAR_HEART", 0, L"Provision Trinket Bear Heart" },
    { "PROVISION_TRINKET_BEAVER_TOOTH", 0, L"Provision Trinket Beaver Tooth" },
    { "PROVISION_TRINKET_BOBCAT_CLAW", 0, L"Provision Trinket Bobcat Claw" },
    { "PROVISION_TRINKET_BUCK_ANTLER", 0, L"Provision Trinket Buck Antler" },
    { "PROVISION_TRINKET_BULL_GATOR", 0, L"Provision Trinket Bull Gator" },
    { "PROVISION_TRINKET_BULL_HORN", 0, L"Provision Trinket Bull Horn" },
    { "PROVISION_TRINKET_CAT_EYE", 0, L"Provision Trinket Cat Eye" },
    { "PROVISION_TRINKET_CONDOR_BEAK", 0, L"Provision Trinket Condor Beak" },
    { "PROVISION_TRINKET_COUGAR_FANG", 0, L"Provision Trinket Cougar Fang" },
    { "PROVISION_TRINKET_COYOTE_FANG", 0, L"Provision Trinket Coyote Fang" },
    { "PROVISION_TRINKET_CRANE_FEATHER", 0, L"Provision Trinket Crane Feather" },
    { "PROVISION_TRINKET_CROW_BEAK", 0, L"Provision Trinket Crow Beak" },
    { "PROVISION_TRINKET_DEPUTY_STAR", 0, L"Provision Trinket Deputy Star" },
    { "PROVISION_TRINKET_DUCK_FEATHER", 0, L"Provision Trinket Duck Feather" },
    { "PROVISION_TRINKET_ELK_ANTLER", 0, L"Provision Trinket Elk Antler" },
    { "PROVISION_TRINKET_FLOWER_SPUN", 0, L"Provision Trinket Flower Spun" },
    { "PROVISION_TRINKET_FOX_CLAW", 0, L"Provision Trinket Fox Claw" },
    { "PROVISION_TRINKET_FUTURISTIC", 0, L"Provision Trinket Futuristic" },
    { "PROVISION_TRINKET_GHOST_ALLIGATOR", 0, L"Provision Trinket Ghost Alligator" },
    { "PROVISION_TRINKET_GIANT_BOAR", 0, L"Provision Trinket Giant Boar" },
    { "PROVISION_TRINKET_GIANT_RABBIT", 0, L"Provision Trinket Giant Rabbit" },
    { "PROVISION_TRINKET_GILAMONSTER_TOOTH", 0, L"Provision Trinket Gilamonster Tooth" },
    { "PROVISION_TRINKET_HAWK_TALON", 0, L"Provision Trinket Hawk Talon" },
    { "PROVISION_TRINKET_HERON_FEATHER", 0, L"Provision Trinket Heron Feather" },
    { "PROVISION_TRINKET_IGUANA_SCALE", 0, L"Provision Trinket Iguana Scale" },
    { "PROVISION_TRINKET_LION_PAW", 0, L"Provision Trinket Lion Paw" },
    { "PROVISION_TRINKET_MOOSE_ANTLER", 0, L"Provision Trinket Moose Antler" },
    { "PROVISION_TRINKET_ORNATE_GOLD", 0, L"Provision Trinket Ornate Gold" },
    { "PROVISION_TRINKET_OWL_FEATHER", 0, L"Provision Trinket Owl Feather" },
    { "PROVISION_TRINKET_OXEN_HORN", 0, L"Provision Trinket Oxen Horn" },
    { "PROVISION_TRINKET_PANTHER_CLAW", 0, L"Provision Trinket Panther Claw" },
    { "PROVISION_TRINKET_PANTHER_EYE", 0, L"Provision Trinket Panther Eye" },
    { "PROVISION_TRINKET_PRECIOUS_SILVER", 0, L"Provision Trinket Precious Silver" },
    { "PROVISION_TRINKET_PRONGHORN_ANTLER", 0, L"Provision Trinket Pronghorn Antler" },
    { "PROVISION_TRINKET_RABBITS_FOOT", 0, L"Provision Trinket Rabbits Foot" },
    { "PROVISION_TRINKET_RACCOON_CLAW", 0, L"Provision Trinket Raccoon Claw" },
    { "PROVISION_TRINKET_RAM_HORN", 0, L"Provision Trinket Ram Horn" },
    { "PROVISION_TRINKET_SHARK_TOOTH", 0, L"Provision Trinket Shark Tooth" },
    { "PROVISION_TRINKET_SQUIRREL_TOOTH", 0, L"Provision Trinket Squirrel Tooth" },
    { "PROVISION_TRINKET_TOAD_LEG", 0, L"Provision Trinket Toad Leg" },
    { "PROVISION_TRINKET_TURTLE_SHELL", 0, L"Provision Trinket Turtle Shell" },
    { "PROVISION_TRINKET_WOLF_HEART", 0, L"Provision Trinket Wolf Heart" },
    { "PROVISION_TRINKET_WORN_MAYAN", 0, L"Provision Trinket Worn Mayan" },
    { "PROVISION_WATCH", 0, L"Provision Watch" },
    { "PROVISION_WEDDING_RING", 0, L"Provision Wedding Ring" },
    { "REINFORCED_BANDIT_BANDOLIER", 0, L"Reinforced Bandit Bandolier" },
    { "REINFORCED_BANDIT_GUNBELT", 0, L"Reinforced Bandit Gunbelt" },
    { "REINFORCED_BANDIT_HOLSTER", 0, L"Reinforced Bandit Holster" },
    { "REINFORCED_BANDIT_OFFHAND_HOLSTER", 0, L"Reinforced Bandit Offhand Holster" },
    { "REINFORCED_EXPLORER_BANDOLIER", 0, L"Reinforced Explorer Bandolier" },
    { "REINFORCED_EXPLORER_GUNBELT", 0, L"Reinforced Explorer Gunbelt" },
    { "REINFORCED_EXPLORER_HOLSTER", 0, L"Reinforced Explorer Holster" },
    { "REINFORCED_EXPLORER_OFFHAND_HOLSTER", 0, L"Reinforced Explorer Offhand Holster" },
    { "REINFORCED_GAMBLER_BANDOLIER", 0, L"Reinforced Gambler Bandolier" },
    { "REINFORCED_GAMBLER_GUNBELT", 0, L"Reinforced Gambler Gunbelt" },
    { "REINFORCED_GAMBLER_HOLSTER", 0, L"Reinforced Gambler Holster" },
    { "REINFORCED_GAMBLER_OFFHAND_HOLSTER", 0, L"Reinforced Gambler Offhand Holster" },
    { "REINFORCED_HERBALIST_BANDOLIER", 0, L"Reinforced Herbalist Bandolier" },
    { "REINFORCED_HERBALIST_GUNBELT", 0, L"Reinforced Herbalist Gunbelt" },
    { "REINFORCED_HERBALIST_HOLSTER", 0, L"Reinforced Herbalist Holster" },
    { "REINFORCED_HERBALIST_OFFHAND_HOLSTER", 0, L"Reinforced Herbalist Offhand Holster" },
    { "REINFORCED_HORSEMANSHIP_BANDOLIER", 0, L"Reinforced Horsemanship Bandolier" },
    { "REINFORCED_HORSEMANSHIP_GUNBELT", 0, L"Reinforced Horsemanship Gunbelt" },
    { "REINFORCED_HORSEMANSHIP_HOLSTER", 0, L"Reinforced Horsemanship Holster" },
    { "REINFORCED_HORSEMANSHIP_OFFHAND_HOLSTER", 0, L"Reinforced Horsemanship Offhand Holster" },
    { "REINFORCED_MASTER_HUNTER_BANDOLIER", 0, L"Reinforced Master Hunter Bandolier" },
    { "REINFORCED_MASTER_HUNTER_GUNBELT", 0, L"Reinforced Master Hunter Gunbelt" },
    { "REINFORCED_MASTER_HUNTER_HOLSTER", 0, L"Reinforced Master Hunter Holster" },
    { "REINFORCED_MASTER_HUNTER_OFFHAND_HOLSTER", 0, L"Reinforced Master Hunter Offhand Holster" },
    { "REINFORCED_SHARPSHOOTER_BANDOLIER", 0, L"Reinforced Sharpshooter Bandolier" },
    { "REINFORCED_SHARPSHOOTER_GUNBELT", 0, L"Reinforced Sharpshooter Gunbelt" },
    { "REINFORCED_SHARPSHOOTER_HOLSTER", 0, L"Reinforced Sharpshooter Holster" },
    { "REINFORCED_SHARPSHOOTER_OFFHAND_HOLSTER", 0, L"Reinforced Sharpshooter Offhand Holster" },
    { "REINFORCED_SURVIVALIST_BANDOLIER", 0, L"Reinforced Survivalist Bandolier" },
    { "REINFORCED_SURVIVALIST_GUNBELT", 0, L"Reinforced Survivalist Gunbelt" },
    { "REINFORCED_SURVIVALIST_HOLSTER", 0, L"Reinforced Survivalist Holster" },
    { "REINFORCED_SURVIVALIST_OFFHAND_HOLSTER", 0, L"Reinforced Survivalist Offhand Holster" },
    { "REINFORCED_WEAPONS_BANDOLIER", 0, L"Reinforced Weapons Bandolier" },
    { "REINFORCED_WEAPONS_GUNBELT", 0, L"Reinforced Weapons Gunbelt" },
    { "REINFORCED_WEAPONS_HOLSTER", 0, L"Reinforced Weapons Holster" },
    { "REINFORCED_WEAPONS_OFFHAND_HOLSTER", 0, L"Reinforced Weapons Offhand Holster" },
    { "UPGRADE_BANDOLIER", 0, L"Upgrade Bandolier" },
    { "UPGRADE_CAMP_TENT", 0, L"Upgrade Camp Tent" },
    { "UPGRADE_FSH_BAIT_BREAD", 0, L"Upgrade Fsh Bait Bread" },
    { "UPGRADE_FSH_BAIT_CHEESE", 0, L"Upgrade Fsh Bait Cheese" },
    { "UPGRADE_FSH_BAIT_CORN", 0, L"Upgrade Fsh Bait Corn" },
    { "UPGRADE_FSH_BAIT_CRAYFISH", 0, L"Upgrade Fsh Bait Crayfish" },
    { "UPGRADE_FSH_BAIT_CRICKET", 0, L"Upgrade Fsh Bait Cricket" },
    { "UPGRADE_FSH_BAIT_CRICKET_TIN", 0, L"Upgrade Fsh Bait Cricket Tin" },
    { "UPGRADE_FSH_BAIT_LEG_LURE_LAKE", 0, L"Upgrade Fsh Bait Leg Lure Lake" },
    { "UPGRADE_FSH_BAIT_LEG_LURE_RIVER", 0, L"Upgrade Fsh Bait Leg Lure River" },
    { "UPGRADE_FSH_BAIT_LEG_LURE_SWAMP", 0, L"Upgrade Fsh Bait Leg Lure Swamp" },
    { "UPGRADE_FSH_BAIT_LURE_LAKE", 0, L"Upgrade Fsh Bait Lure Lake" },
    { "UPGRADE_FSH_BAIT_LURE_NONE", 0, L"Upgrade Fsh Bait Lure None" },
    { "UPGRADE_FSH_BAIT_LURE_RIVER", 0, L"Upgrade Fsh Bait Lure River" },
    { "UPGRADE_FSH_BAIT_LURE_SWAMP", 0, L"Upgrade Fsh Bait Lure Swamp" },
    { "UPGRADE_FSH_BAIT_SPINNER_FRESHWATER", 0, L"Upgrade Fsh Bait Spinner Freshwater" },
    { "UPGRADE_FSH_BAIT_WORM", 0, L"Upgrade Fsh Bait Worm" },
    { "UPGRADE_FSH_BAIT_WORM_CAN", 0, L"Upgrade Fsh Bait Worm Can" },
    { "UPGRADE_LARGE_QUIVER", 0, L"Upgrade Large Quiver" },
    { "UPGRADE_OFFHAND_HOLSTER", 0, L"Upgrade Offhand Holster" },
    { "UPGRADE_UPG_COFFEE_KIT", 0, L"Upgrade Upg Coffee Kit" },
    { "UPGRADE_UPG_COOKING_SPIT", 0, L"Upgrade Upg Cooking Spit" },
    { "UPGRADE_UPG_MORTAR_PESTLE", 0, L"Upgrade Upg Mortar Pestle" },
    { "WEAPON_BOW", 0, L"Weapon Bow" },
    { "WEAPON_FISHINGROD", 0, L"Weapon Fishingrod" },
    { "WEAPON_KIT_BINOCULARS", 0, L"Weapon Kit Binoculars" },
    { "WEAPON_KIT_CAMERA", 0, L"Weapon Kit Camera" },
    { "WEAPON_KIT_DETECTOR", 0, L"Weapon Kit Detector" },
    { "WEAPON_LASSO", 0, L"Weapon Lasso" },
    { "WEAPON_MELEE_ANCIENT_HATCHET", 0, L"Weapon Melee Ancient Hatchet" },
    { "WEAPON_MELEE_BROKEN_SWORD", 0, L"Weapon Melee Broken Sword" },
    { "WEAPON_MELEE_CLEAVER", 0, L"Weapon Melee Cleaver" },
    { "WEAPON_MELEE_DAVY_LANTERN", 0, L"Weapon Melee Davy Lantern" },
    { "WEAPON_MELEE_ELECTRIC_LANTERN", 0, L"Weapon Melee Electric Lantern" },
    { "WEAPON_MELEE_HATCHET", 0, L"Weapon Melee Hatchet" },
    { "WEAPON_MELEE_HATCHET_DOUBLE_BIT", 0, L"Weapon Melee Hatchet Double Bit" },
    { "WEAPON_MELEE_HATCHET_DOUBLE_BIT_RUSTED", 0, L"Weapon Melee Hatchet Double Bit Rusted" },
    { "WEAPON_MELEE_HATCHET_HEWING", 0, L"Weapon Melee Hatchet Hewing" },
    { "WEAPON_MELEE_HATCHET_HUNTER", 0, L"Weapon Melee Hatchet Hunter" },
    { "WEAPON_MELEE_HATCHET_HUNTER_RUSTED", 0, L"Weapon Melee Hatchet Hunter Rusted" },
    { "WEAPON_MELEE_HATCHET_VIKING", 0, L"Weapon Melee Hatchet Viking" },
    { "WEAPON_MELEE_KNIFE", 0, L"Weapon Melee Knife" },
    { "WEAPON_MELEE_KNIFE_BEAR", 0, L"Weapon Melee Knife Bear" },
    { "WEAPON_MELEE_KNIFE_CIVIL_WAR", 0, L"Weapon Melee Knife Civil War" },
    { "WEAPON_MELEE_KNIFE_JAWBONE", 0, L"Weapon Melee Knife Jawbone" },
    { "WEAPON_MELEE_KNIFE_MINER", 0, L"Weapon Melee Knife Miner" },
    { "WEAPON_MELEE_KNIFE_VAMPIRE", 0, L"Weapon Melee Knife Vampire" },
    { "WEAPON_MELEE_MACHETE", 0, L"Weapon Melee Machete" },
    { "WEAPON_MELEE_TORCH", 0, L"Weapon Melee Torch" },
    { "WEAPON_MOONSHINEJUG", 0, L"Weapon Moonshinejug" },
    { "WEAPON_PISTOL_MAUSER", 0, L"Weapon Pistol Mauser" },
    { "WEAPON_PISTOL_SEMIAUTO", 0, L"Weapon Pistol Semiauto" },
    { "WEAPON_PISTOL_VOLCANIC", 0, L"Weapon Pistol Volcanic" },
    { "WEAPON_REPEATER_CARBINE", 0, L"Weapon Repeater Carbine" },
    { "WEAPON_REPEATER_HENRY", 0, L"Weapon Repeater Henry" },
    { "WEAPON_REPEATER_LANCASTER", 0, L"Weapon Repeater Lancaster" },
    { "WEAPON_REVOLVER_CATTLEMAN", 0, L"Weapon Revolver Cattleman" },
    { "WEAPON_REVOLVER_DOUBLEACTION", 0, L"Weapon Revolver Doubleaction" },
    { "WEAPON_REVOLVER_SCHOFIELD", 0, L"Weapon Revolver Schofield" },
    { "WEAPON_RIFLE_BOLTACTION", 0, L"Weapon Rifle Boltaction" },
    { "WEAPON_RIFLE_SPRINGFIELD", 0, L"Weapon Rifle Springfield" },
    { "WEAPON_RIFLE_VARMINT", 0, L"Weapon Rifle Varmint" },
    { "WEAPON_SHOTGUN_DOUBLEBARREL", 0, L"Weapon Shotgun Doublebarrel" },
    { "WEAPON_SHOTGUN_PUMP", 0, L"Weapon Shotgun Pump" },
    { "WEAPON_SHOTGUN_REPEATING", 0, L"Weapon Shotgun Repeating" },
    { "WEAPON_SHOTGUN_SAWEDOFF", 0, L"Weapon Shotgun Sawedoff" },
    { "WEAPON_SHOTGUN_SEMIAUTO", 0, L"Weapon Shotgun Semiauto" },
    { "WEAPON_SNIPERRIFLE_CARCANO", 0, L"Weapon Sniperrifle Carcano" },
    { "WEAPON_SNIPERRIFLE_ROLLINGBLOCK", 0, L"Weapon Sniperrifle Rollingblock" },
    { "WEAPON_THROWN_ANCIENT_TOMAHAWK", 0, L"Weapon Thrown Ancient Tomahawk" },
    { "WEAPON_THROWN_DYNAMITE", 0, L"Weapon Thrown Dynamite" },
    { "WEAPON_THROWN_HOMING_TOMAHAWK", 0, L"Weapon Thrown Homing Tomahawk" },
    { "WEAPON_THROWN_IMPROVED_TOMAHAWK", 0, L"Weapon Thrown Improved Tomahawk" },
    { "WEAPON_THROWN_MOLOTOV", 0, L"Weapon Thrown Molotov" },
    { "WEAPON_THROWN_THROWING_KNIVES", 0, L"Weapon Thrown Throwing Knives" },
    { "WEAPON_THROWN_THROWING_KNIVES_IMPROVED", 0, L"Weapon Thrown Throwing Knives Improved" },
    { "WEAPON_THROWN_TOMAHAWK", 0, L"Weapon Thrown Tomahawk" },
    { "WEAPON_UNARMED", 0, L"Weapon Unarmed" },
    { "WEAPON_VOLATILE_DYNAMITE", 0, L"Weapon Volatile Dynamite" },
    { "WEAPON_VOLATILE_FIRE_BOTTLE", 0, L"Weapon Volatile Fire Bottle" },
    { "WEAPON_BLANK", 0, L"Weapon Blank" },
    { "WEAPON_DUALWIELD", 0, L"Weapon Dualwield" },
    { "WEAPON_MELEE_KNIFE_JOHN", 0, L"Weapon Melee Knife John" },
    { "WEAPON_PISTOL_MAUSER_DRUNK", 0, L"Weapon Pistol Mauser Drunk" },
    { "WEAPON_REVOLVER_CATTLEMAN_JOHN", 0, L"Weapon Revolver Cattleman John" },
    { "WEAPON_REVOLVER_CATTLEMAN_MEXICAN", 0, L"Weapon Revolver Cattleman Mexican" },
    { "WEAPON_REVOLVER_CATTLEMAN_PIG", 0, L"Weapon Revolver Cattleman Pig" },
    { "WEAPON_REVOLVER_DOUBLEACTION_EXOTIC", 0, L"Weapon Revolver Doubleaction Exotic" },
    { "WEAPON_REVOLVER_DOUBLEACTION_MICAH", 0, L"Weapon Revolver Doubleaction Micah" },
    { "WEAPON_REVOLVER_SCHOFIELD_CALLOWAY", 0, L"Weapon Revolver Schofield Calloway" },
    { "WEAPON_REVOLVER_SCHOFIELD_GOLDEN", 0, L"Weapon Revolver Schofield Golden" },
    { "WEAPON_SHOTGUN_DOUBLEBARREL_EXOTIC", 0, L"Weapon Shotgun Doublebarrel Exotic" },
    { "WEAPON_SNIPERRIFLE_ROLLINGBLOCK_EXOTIC", 0, L"Weapon Sniperrifle Rollingblock Exotic" },
    { "WEAPON_THROWN_THROWING_KNIVES_CONFUSE", 0, L"Weapon Thrown Throwing Knives Confuse" },
    { "WEAPON_THROWN_THROWING_KNIVES_DISORIENT", 0, L"Weapon Thrown Throwing Knives Disorient" },
    { "WEAPON_THROWN_THROWING_KNIVES_DRAIN", 0, L"Weapon Thrown Throwing Knives Drain" },
    { "WEAPON_THROWN_THROWING_KNIVES_POISON", 0, L"Weapon Thrown Throwing Knives Poison" },
    { "WEAPON_THROWN_THROWING_KNIVES_TRAIL", 0, L"Weapon Thrown Throwing Knives Trail" },
    { "WEAPON_THROWN_THROWING_KNIVES_WOUND", 0, L"Weapon Thrown Throwing Knives Wound" },
    { "ANIMAL_ALLIGATOR", 0, L"Animal Alligator" },
    { "ANIMAL_ALLIGATOR_BIG", 0, L"Animal Alligator Big" },
    { "ANIMAL_ALLIGATOR_MEDIUM", 0, L"Animal Alligator Medium" },
    { "ANIMAL_ARMADILLO", 0, L"Animal Armadillo" },
    { "ANIMAL_BADGER", 0, L"Animal Badger" },
    { "ANIMAL_BAT", 0, L"Animal Bat" },
    { "ANIMAL_BEAR", 0, L"Animal Bear" },
    { "ANIMAL_BEAR_BLACK", 0, L"Animal Bear Black" },
    { "ANIMAL_BEAVER", 0, L"Animal Beaver" },
    { "ANIMAL_BIGHORNRAM", 0, L"Animal Bighornram" },
    { "ANIMAL_BIGHORNRAM_F", 0, L"Animal Bighornram F" },
    { "ANIMAL_BLUEJAY", 0, L"Animal Bluejay" },
    { "ANIMAL_BOAR", 0, L"Animal Boar" },
    { "ANIMAL_BOBCAT", 0, L"Animal Bobcat" },
    { "ANIMAL_BUCK", 0, L"Animal Buck" },
    { "ANIMAL_BUFFALO", 0, L"Animal Buffalo" },
    { "ANIMAL_BULL_ANGUS", 0, L"Animal Bull Angus" },
    { "ANIMAL_BULL_DEVON", 0, L"Animal Bull Devon" },
    { "ANIMAL_BULL_HEREFORD", 0, L"Animal Bull Hereford" },
    { "ANIMAL_BULLFROG", 0, L"Animal Bullfrog" },
    { "ANIMAL_CALIFORNIANCONDOR", 0, L"Animal Californiancondor" },
    { "ANIMAL_CARDINAL", 0, L"Animal Cardinal" },
    { "ANIMAL_CAROLINAPARAKEET", 0, L"Animal Carolinaparakeet" },
    { "ANIMAL_CAT", 0, L"Animal Cat" },
    { "ANIMAL_CEDAR_WAXWING", 0, L"Animal Cedar Waxwing" },
    { "ANIMAL_CEDARWAXWING", 0, L"Animal Cedarwaxwing" },
    { "ANIMAL_CHICKEN_DOMINIQUE", 0, L"Animal Chicken Dominique" },
    { "ANIMAL_CHICKEN_JAVA", 0, L"Animal Chicken Java" },
    { "ANIMAL_CHICKEN_LEGHORN", 0, L"Animal Chicken Leghorn" },
    { "ANIMAL_CHIPMUNK", 0, L"Animal Chipmunk" },
    { "ANIMAL_CONDOR", 0, L"Animal Condor" },
    { "ANIMAL_CORMORANT", 0, L"Animal Cormorant" },
    { "ANIMAL_CORMORANT_DOUBLECRESTED", 0, L"Animal Cormorant Doublecrested" },
    { "ANIMAL_CORMORANT_NEOTROPIC", 0, L"Animal Cormorant Neotropic" },
    { "ANIMAL_COUGAR", 0, L"Animal Cougar" },
    { "ANIMAL_COW", 0, L"Animal Cow" },
    { "ANIMAL_COYOTE", 0, L"Animal Coyote" },
    { "ANIMAL_CRAB", 0, L"Animal Crab" },
    { "ANIMAL_CRANE", 0, L"Animal Crane" },
    { "ANIMAL_CRANEWHOOPING_SANDHILL", 0, L"Animal Cranewhooping Sandhill" },
    { "ANIMAL_CRANEWHOOPING_WHOOPING", 0, L"Animal Cranewhooping Whooping" },
    { "ANIMAL_CRAWFISH", 0, L"Animal Crawfish" },
    { "ANIMAL_CROW", 0, L"Animal Crow" },
    { "ANIMAL_DEER", 0, L"Animal Deer" },
    { "ANIMAL_DOG_AMERICANFOXHOUND", 0, L"Animal Dog Americanfoxhound" },
    { "ANIMAL_DOG_AUSTRALIANSHEPHERD", 0, L"Animal Dog Australianshepherd" },
    { "ANIMAL_DOG_BLUETICKCOONHOUND", 0, L"Animal Dog Bluetickcoonhound" },
    { "ANIMAL_DOG_CATAHOULARCUR", 0, L"Animal Dog Catahoularcur" },
    { "ANIMAL_DOG_CHESBAYRETRIEVER", 0, L"Animal Dog Chesbayretriever" },
    { "ANIMAL_DOG_COLLIE", 0, L"Animal Dog Collie" },
    { "ANIMAL_DOG_HOUND", 0, L"Animal Dog Hound" },
    { "ANIMAL_DOG_HUSKY", 0, L"Animal Dog Husky" },
    { "ANIMAL_DOG_LAB", 0, L"Animal Dog Lab" },
    { "ANIMAL_DOG_POODLE", 0, L"Animal Dog Poodle" },
    { "ANIMAL_DOG_STREET", 0, L"Animal Dog Street" },
    { "ANIMAL_DONKEY", 0, L"Animal Donkey" },
    { "ANIMAL_DUCK_MALLARD", 0, L"Animal Duck Mallard" },
    { "ANIMAL_DUCK_PEKIN", 0, L"Animal Duck Pekin" },
    { "ANIMAL_EAGLE", 0, L"Animal Eagle" },
    { "ANIMAL_EAGLE_BALD", 0, L"Animal Eagle Bald" },
    { "ANIMAL_EAGLE_GOLDEN", 0, L"Animal Eagle Golden" },
    { "ANIMAL_EGRET_LITTLE", 0, L"Animal Egret Little" },
    { "ANIMAL_EGRET_REDDISH", 0, L"Animal Egret Reddish" },
    { "ANIMAL_EGRET_SNOWY", 0, L"Animal Egret Snowy" },
    { "ANIMAL_ELK", 0, L"Animal Elk" },
    { "ANIMAL_ELK_ROCKY", 0, L"Animal Elk Rocky" },
    { "ANIMAL_ELK_TULE", 0, L"Animal Elk Tule" },
    { "ANIMAL_FOX_GREY", 0, L"Animal Fox Grey" },
    { "ANIMAL_FOX_RED", 0, L"Animal Fox Red" },
    { "ANIMAL_FOX_SILVER", 0, L"Animal Fox Silver" },
    { "ANIMAL_FROGBULL", 0, L"Animal Frogbull" },
    { "ANIMAL_GILA_MONSTER", 0, L"Animal Gila Monster" },
    { "ANIMAL_GOAT", 0, L"Animal Goat" },
    { "ANIMAL_GOOSECANADA", 0, L"Animal Goosecanada" },
    { "ANIMAL_HAWK_FERRUGINOUS", 0, L"Animal Hawk Ferruginous" },
    { "ANIMAL_HAWK_REDTAILED", 0, L"Animal Hawk Redtailed" },
    { "ANIMAL_HAWK_ROUGHLEGGED", 0, L"Animal Hawk Roughlegged" },
    { "ANIMAL_HERON_GREATBLUE", 0, L"Animal Heron Greatblue" },
    { "ANIMAL_HERON_TRICOLOUR", 0, L"Animal Heron Tricolour" },
    { "ANIMAL_HORSE", 0, L"Animal Horse" },
    { "ANIMAL_IGUANA", 0, L"Animal Iguana" },
    { "ANIMAL_IGUANADESERT", 0, L"Animal Iguanadesert" },
    { "ANIMAL_JAVELINA", 0, L"Animal Javelina" },
    { "ANIMAL_LEGENDARY_BEAR", 0, L"Animal Legendary Bear" },
    { "ANIMAL_LEGENDARY_BEAVER", 0, L"Animal Legendary Beaver" },
    { "ANIMAL_LEGENDARY_BIGHORNRAM", 0, L"Animal Legendary Bighornram" },
    { "ANIMAL_LEGENDARY_BOAR", 0, L"Animal Legendary Boar" },
    { "ANIMAL_LEGENDARY_BUCK", 0, L"Animal Legendary Buck" },
    { "ANIMAL_LEGENDARY_BUFFALO_TAKANTA", 0, L"Animal Legendary Buffalo Takanta" },
    { "ANIMAL_LEGENDARY_BUFFALO_WHITE", 0, L"Animal Legendary Buffalo White" },
    { "ANIMAL_LEGENDARY_COUGAR_GIAGUARO", 0, L"Animal Legendary Cougar Giaguaro" },
    { "ANIMAL_LEGENDARY_COYOTE", 0, L"Animal Legendary Coyote" },
    { "ANIMAL_LEGENDARY_ELK", 0, L"Animal Legendary Elk" },
    { "ANIMAL_LEGENDARY_FOX", 0, L"Animal Legendary Fox" },
    { "ANIMAL_LEGENDARY_MOOSE", 0, L"Animal Legendary Moose" },
    { "ANIMAL_LEGENDARY_PANTHER", 0, L"Animal Legendary Panther" },
    { "ANIMAL_LEGENDARY_PRONGHORN", 0, L"Animal Legendary Pronghorn" },
    { "ANIMAL_LEGENDARY_WOLF", 0, L"Animal Legendary Wolf" },
    { "ANIMAL_LOON_COMMON", 0, L"Animal Loon Common" },
    { "ANIMAL_LOON_PACIFIC", 0, L"Animal Loon Pacific" },
    { "ANIMAL_LOON_YELLOWBILLED", 0, L"Animal Loon Yellowbilled" },
    { "ANIMAL_MOOSE", 0, L"Animal Moose" },
    { "ANIMAL_MOUNTAIN_COW_ELK", 0, L"Animal Mountain Cow Elk" },
    { "ANIMAL_MULE", 0, L"Animal Mule" },
    { "ANIMAL_MUSKRAT", 0, L"Animal Muskrat" },
    { "ANIMAL_OPOSSUM", 0, L"Animal Opossum" },
    { "ANIMAL_ORIOLE_BALTIMORE", 0, L"Animal Oriole Baltimore" },
    { "ANIMAL_ORIOLE_HOODED", 0, L"Animal Oriole Hooded" },
    { "ANIMAL_OWL_CALIFORNIAN", 0, L"Animal Owl Californian" },
    { "ANIMAL_OWL_COASTAL", 0, L"Animal Owl Coastal" },
    { "ANIMAL_OWL_GREAT", 0, L"Animal Owl Great" },
    { "ANIMAL_OX_ANGUS", 0, L"Animal Ox Angus" },
    { "ANIMAL_OX_DEVON", 0, L"Animal Ox Devon" },
    { "ANIMAL_PANTHER", 0, L"Animal Panther" },
    { "ANIMAL_PANTHER_FLORIDA", 0, L"Animal Panther Florida" },
    { "ANIMAL_PARAKEET", 0, L"Animal Parakeet" },
    { "ANIMAL_PARROT_BLUEYELLOW", 0, L"Animal Parrot Blueyellow" },
    { "ANIMAL_PARROT_GREATGREEN", 0, L"Animal Parrot Greatgreen" },
    { "ANIMAL_PARROT_SCARLET", 0, L"Animal Parrot Scarlet" },
    { "ANIMAL_PELICAN_BROWN", 0, L"Animal Pelican Brown" },
    { "ANIMAL_PELICAN_WHITE", 0, L"Animal Pelican White" },
    { "ANIMAL_PHEASANT_CHINESE", 0, L"Animal Pheasant Chinese" },
    { "ANIMAL_PHEASANT_RINGNECK", 0, L"Animal Pheasant Ringneck" },
    { "ANIMAL_PIG_BERKSHIRE", 0, L"Animal Pig Berkshire" },
    { "ANIMAL_PIG_BIGCHINA", 0, L"Animal Pig Bigchina" },
    { "ANIMAL_PIG_OLDSPOT", 0, L"Animal Pig Oldspot" },
    { "ANIMAL_PIGEON", 0, L"Animal Pigeon" },
    { "ANIMAL_PIGEON_BANDTAILED", 0, L"Animal Pigeon Bandtailed" },
    { "ANIMAL_PRAIRIE_CHICKEN", 0, L"Animal Prairie Chicken" },
    { "ANIMAL_PRONGHORN", 0, L"Animal Pronghorn" },
    { "ANIMAL_PRONGHORN_BAJA", 0, L"Animal Pronghorn Baja" },
    { "ANIMAL_PRONGHORN_F", 0, L"Animal Pronghorn F" },
    { "ANIMAL_PRONGHORN_SONARAN", 0, L"Animal Pronghorn Sonaran" },
    { "ANIMAL_QUAIL", 0, L"Animal Quail" },
    { "ANIMAL_RABBIT", 0, L"Animal Rabbit" },
    { "ANIMAL_RACCOON", 0, L"Animal Raccoon" },
    { "ANIMAL_RAM", 0, L"Animal Ram" },
    { "ANIMAL_RAT", 0, L"Animal Rat" },
    { "ANIMAL_RAT_BLACK", 0, L"Animal Rat Black" },
    { "ANIMAL_RAT_BROWN", 0, L"Animal Rat Brown" },
    { "ANIMAL_RAVEN", 0, L"Animal Raven" },
    { "ANIMAL_RED_FOOTED_BOOBY", 0, L"Animal Red Footed Booby" },
    { "ANIMAL_REDFOOTEDBOOBY", 0, L"Animal Redfootedbooby" },
    { "ANIMAL_ROBIN", 0, L"Animal Robin" },
    { "ANIMAL_ROOSTER", 0, L"Animal Rooster" },
    { "ANIMAL_ROOSTER_DOMINIQUE", 0, L"Animal Rooster Dominique" },
    { "ANIMAL_ROSEATE_SPOONBILL", 0, L"Animal Roseate Spoonbill" },
    { "ANIMAL_ROSEATESPOONBILL", 0, L"Animal Roseatespoonbill" },
    { "ANIMAL_SEAGULL", 0, L"Animal Seagull" },
    { "ANIMAL_SEAGULL_HERRING", 0, L"Animal Seagull Herring" },
    { "ANIMAL_SEAGULL_LAUGHING", 0, L"Animal Seagull Laughing" },
    { "ANIMAL_SEAGULL_RING_BILLED", 0, L"Animal Seagull Ring Billed" },
    { "ANIMAL_SHARK_HAMMERHEAD", 0, L"Animal Shark Hammerhead" },
    { "ANIMAL_SHARK_TIGER", 0, L"Animal Shark Tiger" },
    { "ANIMAL_SHEEP", 0, L"Animal Sheep" },
    { "ANIMAL_SKUNK", 0, L"Animal Skunk" },
    { "ANIMAL_SNAKE", 0, L"Animal Snake" },
    { "ANIMAL_SNAKEBLACKTAILRATTLE", 0, L"Animal Snakeblacktailrattle" },
    { "ANIMAL_SNAKECOPPERHEAD", 0, L"Animal Snakecopperhead" },
    { "ANIMAL_SNAKECORAL", 0, L"Animal Snakecoral" },
    { "ANIMAL_SNAKEFERDELANCE", 0, L"Animal Snakeferdelance" },
    { "ANIMAL_SNAKEREDBOA", 0, L"Animal Snakeredboa" },
    { "ANIMAL_SNAKEWATER", 0, L"Animal Snakewater" },
    { "ANIMAL_SNAPPING_TURTLE", 0, L"Animal Snapping Turtle" },
    { "ANIMAL_SONGBIRD", 0, L"Animal Songbird" },
    { "ANIMAL_SONGBIRD_SCARLET", 0, L"Animal Songbird Scarlet" },
    { "ANIMAL_SPARROW", 0, L"Animal Sparrow" },
    { "ANIMAL_SPARROW_EURASIAN", 0, L"Animal Sparrow Eurasian" },
    { "ANIMAL_SPARROW_GOLDEN", 0, L"Animal Sparrow Golden" },
    { "ANIMAL_SQUIRREL", 0, L"Animal Squirrel" },
    { "ANIMAL_SQUIRREL_BLACK", 0, L"Animal Squirrel Black" },
    { "ANIMAL_SQUIRREL_GREY", 0, L"Animal Squirrel Grey" },
    { "ANIMAL_TOAD", 0, L"Animal Toad" },
    { "ANIMAL_TURKEY_EASTERN", 0, L"Animal Turkey Eastern" },
    { "ANIMAL_TURKEY_RIOGRANDE", 0, L"Animal Turkey Riogrande" },
    { "ANIMAL_TURTLE_SEA", 0, L"Animal Turtle Sea" },
    { "ANIMAL_VULTURE_EASTERN", 0, L"Animal Vulture Eastern" },
    { "ANIMAL_VULTURE_WESTERN", 0, L"Animal Vulture Western" },
    { "ANIMAL_WOLF_GRAY", 0, L"Animal Wolf Gray" },
    { "ANIMAL_WOLF_TIMBER", 0, L"Animal Wolf Timber" },
    { "ANIMAL_WOODPECKER_PILEATED", 0, L"Animal Woodpecker Pileated" },
    { "ANIMAL_WOODPECKER_REDBELLIED", 0, L"Animal Woodpecker Redbellied" },
    { "CMPNDM_AMPAINT", 0, L"Cmpndm Ampaint" },
    { "CMPNDM_AMSTDBRED", 0, L"Cmpndm Amstdbred" },
    { "CMPNDM_ANDALUSIAN", 0, L"Cmpndm Andalusian" },
    { "CMPNDM_APPALOOSA", 0, L"Cmpndm Appaloosa" },
    { "CMPNDM_ARABIAN", 0, L"Cmpndm Arabian" },
    { "CMPNDM_ARDENNES", 0, L"Cmpndm Ardennes" },
    { "CMPNDM_BELDRAFT", 0, L"Cmpndm Beldraft" },
    { "CMPNDM_DUTCHWM", 0, L"Cmpndm Dutchwm" },
    { "CMPNDM_HUNGHALF", 0, L"Cmpndm Hunghalf" },
    { "CMPNDM_KYSADDLER", 0, L"Cmpndm Kysaddler" },
    { "CMPNDM_MOFOXTROT", 0, L"Cmpndm Mofoxtrot" },
    { "CMPNDM_MORGAN", 0, L"Cmpndm Morgan" },
    { "CMPNDM_MUSTANG", 0, L"Cmpndm Mustang" },
    { "CMPNDM_NOKOTA", 0, L"Cmpndm Nokota" },
    { "CMPNDM_SHIRE", 0, L"Cmpndm Shire" },
    { "CMPNDM_SUFPUNCH", 0, L"Cmpndm Sufpunch" },
    { "CMPNDM_THOROBRED", 0, L"Cmpndm Thorobred" },
    { "CMPNDM_TNWALKER", 0, L"Cmpndm Tnwalker" },
    { "CMPNDM_TURKOMAN", 0, L"Cmpndm Turkoman" },
    { "FEATHERS_CRAFTING", 0, L"Feathers Crafting" },
    { "FEATHERS_CRAFTING_PREMIUM", 0, L"Feathers Crafting Premium" },
    { "FEATHERS_PLUME", 0, L"Feathers Plume" },
    { "GENERIC_ANIMAL_BEAK", 0, L"Generic Animal Beak" },
    { "GENERIC_ANIMAL_CLAW", 0, L"Generic Animal Claw" },
    { "GENERIC_ANIMAL_FAT", 0, L"Generic Animal Fat" },
    { "GENERIC_ANIMAL_FEATHER_LARGE", 0, L"Generic Animal Feather Large" },
    { "GENERIC_ANIMAL_FEATHER_SMALL", 0, L"Generic Animal Feather Small" },
    { "GENERIC_ANIMAL_HEART", 0, L"Generic Animal Heart" },
    { "GENERIC_ANIMAL_HORN", 0, L"Generic Animal Horn" },
    { "GENERIC_ANIMAL_TOOTH", 0, L"Generic Animal Tooth" },
    { "PROVISION_ARMADILLO_SKIN", 0, L"Provision Armadillo Skin" },
    { "PROVISION_BAT_WING", 0, L"Provision Bat Wing" },
    { "PROVISION_BEAR_FUR", 0, L"Provision Bear Fur" },
    { "PROVISION_BEAR_HEART", 0, L"Provision Bear Heart" },
    { "PROVISION_BEAVER_FUR", 0, L"Provision Beaver Fur" },
    { "PROVISION_BEAVER_SCENTGLAND", 0, L"Provision Beaver Scentgland" },
    { "PROVISION_BEAVER_TAIL", 0, L"Provision Beaver Tail" },
    { "PROVISION_BIRD_FEATHER_FLIGHT", 0, L"Provision Bird Feather Flight" },
    { "PROVISION_BISON_HORN", 0, L"Provision Bison Horn" },
    { "PROVISION_BLACK_BEAR_FUR", 0, L"Provision Black Bear Fur" },
    { "PROVISION_BOAR_SKIN", 0, L"Provision Boar Skin" },
    { "PROVISION_BOBCAT_CLAWS", 0, L"Provision Bobcat Claws" },
    { "PROVISION_BOBCAT_FUR", 0, L"Provision Bobcat Fur" },
    { "PROVISION_BOOBY_FEATHER", 0, L"Provision Booby Feather" },
    { "PROVISION_BUCK_ANTLERS", 0, L"Provision Buck Antlers" },
    { "PROVISION_BUCK_FUR", 0, L"Provision Buck Fur" },
    { "PROVISION_BUFFALO_FUR", 0, L"Provision Buffalo Fur" },
    { "PROVISION_BUFFALO_HORN", 0, L"Provision Buffalo Horn" },
    { "PROVISION_BULL_GATOR_EYE", 0, L"Provision Bull Gator Eye" },
    { "PROVISION_BULL_HIDE", 0, L"Provision Bull Hide" },
    { "PROVISION_BULL_HORNS", 0, L"Provision Bull Horns" },
    { "PROVISION_CORMORANT_FEATHER", 0, L"Provision Cormorant Feather" },
    { "PROVISION_COUGAR_CLAW", 0, L"Provision Cougar Claw" },
    { "PROVISION_COUGAR_FUR", 0, L"Provision Cougar Fur" },
    { "PROVISION_COW_HIDE", 0, L"Provision Cow Hide" },
    { "PROVISION_COYOTE_FANG", 0, L"Provision Coyote Fang" },
    { "PROVISION_COYOTE_FUR", 0, L"Provision Coyote Fur" },
    { "PROVISION_CROW_BEAK", 0, L"Provision Crow Beak" },
    { "PROVISION_CROW_FEATHER", 0, L"Provision Crow Feather" },
    { "PROVISION_DEER_HIDE", 0, L"Provision Deer Hide" },
    { "PROVISION_EAGLE_TALON", 0, L"Provision Eagle Talon" },
    { "PROVISION_ELK_ANTLERS", 0, L"Provision Elk Antlers" },
    { "PROVISION_ELK_FUR", 0, L"Provision Elk Fur" },
    { "PROVISION_FOX_CLAW", 0, L"Provision Fox Claw" },
    { "PROVISION_FOX_FUR", 0, L"Provision Fox Fur" },
    { "PROVISION_FROG_SKIN", 0, L"Provision Frog Skin" },
    { "PROVISION_GOAT_HAIR", 0, L"Provision Goat Hair" },
    { "PROVISION_JAVELINA_SKIN", 0, L"Provision Javelina Skin" },
    { "PROVISION_JAVELINA_TUSK", 0, L"Provision Javelina Tusk" },
    { "PROVISION_LIONS_PAW", 0, L"Provision Lions Paw" },
    { "PROVISION_MEAT_BIG_GAME", 0, L"Provision Meat Big Game" },
    { "PROVISION_MEAT_CRUSTACEAN", 0, L"Provision Meat Crustacean" },
    { "PROVISION_MEAT_EXOTIC_BIRD", 0, L"Provision Meat Exotic Bird" },
    { "PROVISION_MEAT_FLAKEY_FISH", 0, L"Provision Meat Flakey Fish" },
    { "PROVISION_MEAT_GAME", 0, L"Provision Meat Game" },
    { "PROVISION_MEAT_GAMEY_BIRD", 0, L"Provision Meat Gamey Bird" },
    { "PROVISION_MEAT_GRISTLY_MUTTON", 0, L"Provision Meat Gristly Mutton" },
    { "PROVISION_MEAT_GRITTY_FISH", 0, L"Provision Meat Gritty Fish" },
    { "PROVISION_MEAT_HERPTILE", 0, L"Provision Meat Herptile" },
    { "PROVISION_MEAT_MATURE_VENISON", 0, L"Provision Meat Mature Venison" },
    { "PROVISION_MEAT_PLUMP_BIRD", 0, L"Provision Meat Plump Bird" },
    { "PROVISION_MEAT_PRIME_BEEF", 0, L"Provision Meat Prime Beef" },
    { "PROVISION_MEAT_STRINGY", 0, L"Provision Meat Stringy" },
    { "PROVISION_MEAT_SUCCULENT_FISH", 0, L"Provision Meat Succulent Fish" },
    { "PROVISION_MEAT_TENDER_PORK", 0, L"Provision Meat Tender Pork" },
    { "PROVISION_MOOSE_ANTLER", 0, L"Provision Moose Antler" },
    { "PROVISION_MOOSE_FUR", 0, L"Provision Moose Fur" },
    { "PROVISION_MUSKRAT_FUR", 0, L"Provision Muskrat Fur" },
    { "PROVISION_MUSKRAT_SCENTGLAND", 0, L"Provision Muskrat Scentgland" },
    { "PROVISION_OPOSSUM_FUR", 0, L"Provision Opossum Fur" },
    { "PROVISION_OXEN_HIDE", 0, L"Provision Oxen Hide" },
    { "PROVISION_OXEN_HORN", 0, L"Provision Oxen Horn" },
    { "PROVISION_PANTHER_EYE", 0, L"Provision Panther Eye" },
    { "PROVISION_PANTHER_FUR", 0, L"Provision Panther Fur" },
    { "PROVISION_PANTHER_HEART", 0, L"Provision Panther Heart" },
    { "PROVISION_PARROT_FEATHER", 0, L"Provision Parrot Feather" },
    { "PROVISION_PIG_SKIN", 0, L"Provision Pig Skin" },
    { "PROVISION_PRONGHORN_FUR", 0, L"Provision Pronghorn Fur" },
    { "PROVISION_PRONGHORN_HORN", 0, L"Provision Pronghorn Horn" },
    { "PROVISION_RACCOON_FUR", 0, L"Provision Raccoon Fur" },
    { "PROVISION_RAM_HIDE", 0, L"Provision Ram Hide" },
    { "PROVISION_RAM_HORN", 0, L"Provision Ram Horn" },
    { "PROVISION_RAT_FUR", 0, L"Provision Rat Fur" },
    { "PROVISION_RAVEN_CLAW", 0, L"Provision Raven Claw" },
    { "PROVISION_SHEEP_WOOL", 0, L"Provision Sheep Wool" },
    { "PROVISION_SKUNK_FUR", 0, L"Provision Skunk Fur" },
    { "PROVISION_SQUIRREL_TOOTH", 0, L"Provision Squirrel Tooth" },
    { "PROVISION_VULTURE_FEATHER", 0, L"Provision Vulture Feather" },
    { "PROVISION_WHOOPING_CRANE_FEATHER", 0, L"Provision Whooping Crane Feather" },
    { "PROVISION_WOLF_FUR", 0, L"Provision Wolf Fur" },
    { "SATCHEL_NAV_ALL", 0, L"Satchel Nav All" },
    { "SATCHEL_NAV_ANIMALS", 0, L"Satchel Nav Animals" },
    { "SATCHEL_NAV_DOCUMENTS", 0, L"Satchel Nav Documents" },
    { "SATCHEL_NAV_DONATE", 0, L"Satchel Nav Donate" },
    { "SATCHEL_NAV_FIRE", 0, L"Satchel Nav Fire" },
    { "SATCHEL_NAV_FISH_SACK", 0, L"Satchel Nav Fish Sack" },
    { "SATCHEL_NAV_HERBS", 0, L"Satchel Nav Herbs" },
    { "SATCHEL_NAV_HORSE", 0, L"Satchel Nav Horse" },
    { "SATCHEL_NAV_HORSE_ITEMS", 0, L"Satchel Nav Horse Items" },
    { "SATCHEL_NAV_INGREDIENTS", 0, L"Satchel Nav Ingredients" },
    { "SATCHEL_NAV_KIT", 0, L"Satchel Nav Kit" },
    { "SATCHEL_NAV_LOOT", 0, L"Satchel Nav Loot" },
    { "SATCHEL_NAV_MATERIALS", 0, L"Satchel Nav Materials" },
    { "SATCHEL_NAV_PROVISIONS", 0, L"Satchel Nav Provisions" },
    { "SATCHEL_NAV_REMEDIES", 0, L"Satchel Nav Remedies" },
    { "SATCHEL_NAV_SELL", 0, L"Satchel Nav Sell" },
    { "SATCHEL_NAV_SEND", 0, L"Satchel Nav Send" },
    { "SATCHEL_NAV_VALUABLES", 0, L"Satchel Nav Valuables" },
};


static const int g_monitoredItemsCount = sizeof(g_monitoredItems) / sizeof(g_monitoredItems[0]);
static int g_lastInventoryCounts[g_monitoredItemsCount] = { 0 };
static bool g_inventoryInitialized = false;
static bool g_lootHashesInitialized = false;

static void HandleAutoLootAnnounce() {
	if (!g_autoLootAnnounce) {
		g_inventoryInitialized = false;
		return;
	}

	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || ENTITY::IS_ENTITY_DEAD(playerPed)) {
		g_inventoryInitialized = false;
		return;
	}

	if (!g_lootHashesInitialized) {
		DebugLog::log("Initializing monitored loot item hashes:");
		for (int i = 0; i < g_monitoredItemsCount; ++i) {
			if (g_monitoredItems[i].hash == 0) {
				g_monitoredItems[i].hash = GAMEPLAY::GET_HASH_KEY(const_cast<char*>(g_monitoredItems[i].codeName));
			}
			DebugLog::log("  Item %d: %s = 0x%08X", i, g_monitoredItems[i].codeName, g_monitoredItems[i].hash);
		}
		g_lootHashesInitialized = true;
	}

	int invId = invoke<int>(0x13D234A2A3F66E63, playerPed); // _INVENTORY_GET_INVENTORY_ID_FROM_PED
	static int lastLoggedInvId = -999;
	if (invId != lastLoggedInvId) {
		DebugLog::log("Loot Announcer DIAGNOSTIC: invId = %d", invId);
		if (invId != 0) {
			int testCount1 = invoke<int>(0xE787F05DFC977BDE, invId, GAMEPLAY::GET_HASH_KEY("AMMO_REVOLVER"), FALSE);
			int testCount2 = invoke<int>(0xE787F05DFC977BDE, 1, GAMEPLAY::GET_HASH_KEY("AMMO_REVOLVER"), FALSE);
			DebugLog::log("Loot Announcer DIAGNOSTIC: Revolver Ammo Count (with invId=%d) = %d, (with 1) = %d", invId, testCount1, testCount2);
		}
		lastLoggedInvId = invId;
	}
	if (invId == 0) {
		invId = 1; // Fallback to player single player inventory ID
	}

	// Throttle reads to every 300ms to save performance
	static DWORD lastLootCheckTime = 0;
	DWORD now = GetTickCount();
	if (now - lastLootCheckTime < 300) return;
	lastLootCheckTime = now;

	// Track looting target
	static bool s_isLootingActive = false;
	static Entity s_lastLootTarget = 0;
	static DWORD s_lootEndTime = 0;
	static bool s_pendingLootCheck = false;
	static int s_preLootCounts[g_monitoredItemsCount] = { 0 };
	static int s_preLootMoney = 0;

	Entity currentTarget = invoke<Entity>(0x14169FA823679E41, playerPed); // GET_LOOTING_PICKUP_TARGET_ENTITY
	bool isLootingScenario = false;
	if (PED::IS_PED_USING_ANY_SCENARIO(playerPed) || invoke<BOOL>(0x295E3CCEC879CCD7, playerPed)) { // PED_HAS_USE_SCENARIO_TASK
		Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
		int peds[256];
		int count = worldGetAllPeds(peds, 256);
		for (int i = 0; i < count; i++) {
			Ped ped = peds[i];
			if (ENTITY::IS_ENTITY_DEAD(ped) && ped != playerPed) {
				Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(ped, TRUE, FALSE);
				float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z, pedPos.x, pedPos.y, pedPos.z, TRUE);
				if (dist < 4.0f) {
					isLootingScenario = true;
					break;
				}
			}
		}
	}

	bool isLootingCurrent = (currentTarget != 0) || isLootingScenario;
	if (isLootingCurrent) {
		if (!s_isLootingActive) {
			s_isLootingActive = true;
			s_lastLootTarget = currentTarget;
			s_pendingLootCheck = false;
			s_preLootMoney = invoke<int>(0x0C02DABFA3B98176); // _MONEY_GET_CASH_BALANCE
			for (int i = 0; i < g_monitoredItemsCount; ++i) {
				s_preLootCounts[i] = invoke<int>(0xE787F05DFC977BDE, invId, g_monitoredItems[i].hash, FALSE);
			}
			DebugLog::log("Loot Announcer: Loot/Pick/Skin interaction started. target=%d, scenario=%d", currentTarget, isLootingScenario);
		}
	} else {
		if (s_isLootingActive) {
			s_isLootingActive = false;
			s_lootEndTime = GetTickCount();
			s_pendingLootCheck = true;
			DebugLog::log("Loot Announcer: Loot/Pick/Skin interaction finished. Waiting for sync.");
		}
	}

	if (!g_inventoryInitialized) {
		// Initialize the baseline inventory snapshot
		for (int i = 0; i < g_monitoredItemsCount; ++i) {
			g_lastInventoryCounts[i] = invoke<int>(0xE787F05DFC977BDE, invId, g_monitoredItems[i].hash, FALSE);
		}
		g_inventoryInitialized = true;
		s_pendingLootCheck = false; // Reset to avoid false warnings on startup
		return;
	}

	// Regular inventory change detection
	bool itemAdded = false;
    // Custom Tracking for Money
    {
        static int lastMoney = -1;
        int currentMoney = PED::GET_PED_MONEY(PLAYER::PLAYER_PED_ID());
        if (lastMoney != -1 && currentMoney > lastMoney) {
            int diff = currentMoney - lastMoney;
            float dollars = diff / 100.0f;
            wchar_t buf[128];
            swprintf_s(buf, L"Added $%.2f", dollars);
            A11y::speak(buf, false);
            DebugLog::log("Inventory change auto-announcement: Money increased by %d cents", diff);
        }
        lastMoney = currentMoney;
    }

    // Custom Tracking for Ammo
    {
        static Hash ammoTypes[] = {
            0x1BCC57BF, // AMMO_PISTOL
            0x16DB4808, // AMMO_REVOLVER
            0x2035F206, // AMMO_REPEATER
            0x3BEB7AA2, // AMMO_RIFLE
            0x161C6E6E, // AMMO_SHOTGUN
            0x2E6CFEB4, // AMMO_22
            0xCCEE2A82, // AMMO_ARROW
            0x8DE05C8C, // AMMO_DYNAMITE
            0xF45E1B5D, // AMMO_THROWING_KNIVES
            0x65F0FC37  // AMMO_TOMAHAWK
        };
        static const wchar_t* ammoNames[] = {
            L"Pistol Ammo", L"Revolver Ammo", L"Repeater Ammo", L"Rifle Ammo", L"Shotgun Ammo",
            L"Varmint Ammo", L"Arrows", L"Dynamite", L"Throwing Knives", L"Tomahawks"
        };
        static const int ammoTypesCount = 10;
        static int lastAmmoCounts[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
        
        for (int i = 0; i < ammoTypesCount; ++i) {
            int currentAmmo = WEAPON::GET_PED_AMMO_BY_TYPE(PLAYER::PLAYER_PED_ID(), ammoTypes[i]);
            if (lastAmmoCounts[i] != -1 && currentAmmo > lastAmmoCounts[i]) {
                int diff = currentAmmo - lastAmmoCounts[i];
                wchar_t buf[128];
                swprintf_s(buf, L"Added %d %s", diff, ammoNames[i]);
                A11y::speak(buf, false);
                DebugLog::log("Inventory change auto-announcement: %ls increased by %d", ammoNames[i], diff);
            }
            lastAmmoCounts[i] = currentAmmo;
        }
    }


	for (int i = 0; i < g_monitoredItemsCount; ++i) {
		int currentCount = invoke<int>(0xE787F05DFC977BDE, invId, g_monitoredItems[i].hash, FALSE);
		int diff = currentCount - g_lastInventoryCounts[i];
		if (diff > 0) {
			wchar_t buf[256];
			if (diff == 1) {
				swprintf_s(buf, L"Added %s", g_monitoredItems[i].displayName);
			} else {
				swprintf_s(buf, L"Added %d %s", diff, g_monitoredItems[i].displayName);
			}
			A11y::speak(buf, false);
			DebugLog::log("Inventory change auto-announcement: %s increased by %d (total: %d)", g_monitoredItems[i].codeName, diff, currentCount);
			itemAdded = true;
		}
		g_lastInventoryCounts[i] = currentCount;
	}

	// Check if money increased
	int currentMoney = invoke<int>(0x0C02DABFA3B98176); // _MONEY_GET_CASH_BALANCE
	if (currentMoney > s_preLootMoney) {
		itemAdded = true;
	}

	// If a loot action just completed, verify if anything was added
	if (s_pendingLootCheck && (GetTickCount() - s_lootEndTime >= 2000)) {
		s_pendingLootCheck = false;
		// Check if any items actually increased compared to pre-loot snapshot
		bool actuallyAdded = false;
		if (currentMoney > s_preLootMoney) {
			actuallyAdded = true;
		}
		for (int i = 0; i < g_monitoredItemsCount; ++i) {
			int currentCount = invoke<int>(0xE787F05DFC977BDE, invId, g_monitoredItems[i].hash, FALSE);
			if (currentCount > s_preLootCounts[i]) {
				actuallyAdded = true;
			}
		}

		if (!actuallyAdded) {
			// Looting was done but nothing was added!
			A11y::speak(L"No items added. Inventory might be full.", false);
			DebugLog::log("Loot Announcer: Loot completed but nothing added (inventory full or target empty)");
		}
	}
}


// =======================================
// AUTO-AIM SYSTEM (accessibility for blind players)
// =======================================

// Find nearest hostile ped within range and aim at them
static void HandleAutoAim() {
	if (!g_autoAimEnabled) return;

	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || ENTITY::IS_ENTITY_DEAD(playerPed)) {
		g_autoAimTarget = 0;
		return;
	}

	// Only activate when player is aiming (holding aim button)
	// Check raw input to avoid flickering when tasks are assigned
	bool isAiming = (XController::GetLeftTrigger(0) > 0.5f) || ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
	if (!isAiming) {
		g_autoAimTarget = 0;
		return;
	}

	DWORD now = GetTickCount();
	// Scan for target every 100ms
	if (g_autoAimTarget == 0 || !ENTITY::DOES_ENTITY_EXIST(g_autoAimTarget) || ENTITY::IS_ENTITY_DEAD(g_autoAimTarget) || (now - g_lastAutoAimMs) >= 100) {
		g_lastAutoAimMs = now;
		
		Vector3 myPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);

		int peds[1024];
		int count = worldGetAllPeds(peds, 1024);

		Ped bestTarget = 0;
		float bestScore = 999999.0f; // lower is better

		for (int i = 0; i < count; ++i) {
			Ped p = (Ped)peds[i];
			if (!p || p == playerPed) continue;
			if (!ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p)) continue;
			if (IsOurGuard(p)) continue;

			// Must have line of sight
			if (!ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(playerPed, p, 17)) continue;

			Vector3 tPos = ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE);
			float d = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(myPos.x, myPos.y, myPos.z, tPos.x, tPos.y, tPos.z, TRUE);
			
			if (d > 150.0f) continue; // max range

			// Calculate a "score" for the target. Distance is the base.
			float score = d;

			// Priority 1: Hostile Enemies (reduce score massively to prioritize)
			int rel = PED::GET_RELATIONSHIP_BETWEEN_PEDS(p, playerPed);
			bool isHostile = (rel >= 4) || PED::IS_PED_IN_COMBAT(p, playerPed);
			if (isHostile) {
				score -= 1000.0f; 
			} 
			// Priority 2: Animals (Hunting)
			else if (!PED::IS_PED_HUMAN(p)) {
				score -= 500.0f;
			}
			// Civilians remain at base distance score, only targeted if nothing else is near.

			if (score < bestScore) {
				bestScore = score;
				bestTarget = p;
			}
		}

		if (bestTarget && ENTITY::DOES_ENTITY_EXIST(bestTarget)) {
			if (bestTarget != g_autoAimTarget) {
				g_autoAimTarget = bestTarget;
			}
		} else {
			g_autoAimTarget = 0;
		}
	}

	// Apply aim rotation math every frame for the locked target
	if (g_autoAimTarget && ENTITY::DOES_ENTITY_EXIST(g_autoAimTarget) && !ENTITY::IS_ENTITY_DEAD(g_autoAimTarget)) {
		Vector3 myPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
		Vector3 tPos;
		if (PED::IS_PED_HUMAN(g_autoAimTarget)) {
			tPos = PED::GET_PED_BONE_COORDS(g_autoAimTarget, 0x60F2, 0.0f, 0.0f, 0.0f); // SKEL_Spine3 (chest/torso)
		} else {
			tPos = ENTITY::GET_ENTITY_COORDS(g_autoAimTarget, TRUE, FALSE);
			tPos.z += 0.25f; // offset slightly for animals
		}

		float dx = tPos.x - myPos.x;
		float dy = tPos.y - myPos.y;
		float dz = tPos.z - myPos.z;

		float groundDist = sqrtf(dx * dx + dy * dy);

		float targetHeading = atan2f(-dx, dy) * (180.0f / 3.14159265f);
		if (targetHeading < 0.0f) targetHeading += 360.0f;

		float targetPitch = atan2f(dz, groundDist) * (180.0f / 3.14159265f);

		// Face target and snap camera
		ENTITY::SET_ENTITY_HEADING(playerPed, targetHeading);
		CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0.0f, 1.0f);
		CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(targetPitch, 1.0f);

		// Force ped to aim
		AI::TASK_AIM_GUN_AT_ENTITY(playerPed, g_autoAimTarget, -1, FALSE, 0);
	}
}

// =======================================
// AIMING TARGET ANNOUNCEMENTS
// =======================================

static const wchar_t* GetPedAnimalName(Ped ped) {
	Hash model = ENTITY::GET_ENTITY_MODEL(ped);
	for (int i = 0; i < ARRAY_LENGTH(pedModelInfos); i++) {
		if (GAMEPLAY::GET_HASH_KEY(const_cast<char*>(pedModelInfos[i].model.c_str())) == model) {
			static wchar_t nameBuf[128];
			const std::string& n = pedModelInfos[i].name;
			MultiByteToWideChar(CP_UTF8, 0, n.c_str(), -1, nameBuf, 128);
			return nameBuf;
		}
	}
	return nullptr;
}

static void HandleAimingAnnounce() {
	DWORD now = GetTickCount();
	if ((now - g_lastAimCheckMs) < 250) return; // Beep every 250ms

	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

	// Check raw input to avoid flickering when tasks are assigned
	bool isAiming = (XController::GetLeftTrigger(0) > 0.5f) || ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);

	if (!isAiming) {
		g_wasAiming = false;
		g_lastAimedEntity = 0;
		return;
	}

	Entity aimedEntity = 0;
	if (g_autoAimEnabled && g_autoAimTarget && ENTITY::DOES_ENTITY_EXIST(g_autoAimTarget) && !ENTITY::IS_ENTITY_DEAD(g_autoAimTarget)) {
		aimedEntity = g_autoAimTarget;
	} else {
		if (!PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &aimedEntity)) {
			PLAYER::GET_PLAYER_TARGET_ENTITY(player, &aimedEntity);
		}
	}

	if (!aimedEntity || !ENTITY::DOES_ENTITY_EXIST(aimedEntity) || ENTITY::IS_ENTITY_DEAD(aimedEntity)) {
		g_wasAiming = false;
		g_lastAimedEntity = 0;
		return;
	}

	g_wasAiming = true;
	g_lastAimedEntity = aimedEntity;
	g_lastAimCheckMs = now; // Update timer because we are about to play a sound

	if (ENTITY::IS_ENTITY_A_PED(aimedEntity)) {
		Ped targetPed = (Ped)aimedEntity;
		if (PED::IS_PED_HUMAN(targetPed)) {
			// Get head coordinates
			Vector3 headCoords = PED::GET_PED_BONE_COORDS(targetPed, 0x796E, 0, 0, 0); // SKEL_Head
			float screenX = 0, screenY = 0;
			bool onScreen = GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(headCoords.x, headCoords.y, headCoords.z, &screenX, &screenY) ? true : false;
			
			// Screen center is 0.5, 0.5. Check if distance to center is very small (e.g. < 0.05 which is 5% of screen size)
			bool isHeadshot = false;
			if (onScreen) {
				float dx = screenX - 0.5f;
				float dy = screenY - 0.5f;
				float dist = sqrtf(dx * dx + dy * dy);
				if (dist < 0.08f) { // slightly generous hitbox for head audio
					isHeadshot = true;
				}
			}

			if (isHeadshot) {
				A11y::playCustomSound(L"head.wav");
			} else {
				A11y::playCustomSound(L"enemy.wav");
			}
		} else {
			// Animal
			A11y::playCustomSound(L"enemy.wav");
		}
	} else if (ENTITY::IS_ENTITY_A_VEHICLE(aimedEntity)) {
		A11y::playCustomSound(L"vehicle.wav");
	} else if (ENTITY::IS_ENTITY_AN_OBJECT(aimedEntity)) {
		A11y::playCustomSound(L"object.wav");
	}
}

MenuBase *CreateMainMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("RDR  ACCESS"));
	controller->RegisterMenu(menu);

	menu->AddItem(new MenuItemMenu("SQUAD MANAGER", CreateBodyguardMenu(controller)));
	menu->AddItem(new MenuItemMenu("PLAYER", CreatePlayerMenu(controller)));
	menu->AddItem(new MenuItemMenu("ANIMALS", CreateAnimalSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("PEDS", CreateHumanSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("CANNONS", CreateCannonSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("VEHICLES", CreateVehicleSpawnerMenu(controller)));
	menu->AddItem(new MenuItemMenu("WEAPON", CreateWeaponMenu(controller)));
	menu->AddItem(new MenuItemMenu("TIME", CreateTimeMenu(controller)));
	menu->AddItem(new MenuItemMenu("WEATHER", CreateWeatherMenu(controller)));
	menu->AddItem(new MenuItemMenu("MISC", CreateMiscMenu(controller)));
	menu->AddItem(new MenuItemMenu("ACCESSIBILITY", CreateAccessibilityMenu(controller)));
	
	return menu;
}

void main()
{
	// Initialize accessibility layer
	A11y::init();
	A11y::speak(L"R D R access loaded. Ready.", true);
	
	// Initialize controller support
	XController::Init();
	
	auto menuController = new MenuController();
	auto mainMenu = CreateMainMenu(menuController);
	
	bool lastControllerMenuState = false;
	
	// Clean start for squad system
	DismissAllGuards();
	
	while (true)
	{		
			// Pause menu accessibility (always runs)
		HandlePauseMenuAccessibility();

		// Process asynchronous bodyguard spawns
		ProcessPendingSpawns();


		if (!menuController->HasActiveMenu()) {
			// Keyboard input
			if (MenuInput::MenuSwitchPressed()) {
				MenuInput::MenuInputBeep();
				menuController->PushMenu(mainMenu);
			}
			
			// Controller input: R1 + X to open menu
			XController::UpdateState();
			bool r1Held = XController::IsButtonDown(0, XController::BTN_RB);
			bool xJustPressed = XController::IsButtonJustPressed(0, XController::BTN_X);
			
			if (r1Held && xJustPressed) {
				if (!lastControllerMenuState) {
					MenuInput::MenuInputBeep();
					menuController->PushMenu(mainMenu);
					lastControllerMenuState = true;
				}
			} else if (!r1Held) {
				lastControllerMenuState = false;
			}

			// Numpad mode system (Bodyguard / Global / Horse)
			HandleNumpadModes();

			// Diagnostic coordinates key: VK_DECIMAL (Numpad Decimal Point .)
			if (IsKeyJustUp(VK_DECIMAL)) {
				Ped pp = PLAYER::PLAYER_PED_ID();
				if (ENTITY::DOES_ENTITY_EXIST(pp)) {
					Vector3 myPos = ENTITY::GET_ENTITY_COORDS(pp, TRUE, FALSE);
					float heading = ENTITY::GET_ENTITY_HEADING(pp);
					DebugLog::log("DIAGNOSTIC: Player coordinates: (%f, %f, %f), Heading: %f", myPos.x, myPos.y, myPos.z, heading);
					
					wchar_t announceBuf[256];
					swprintf_s(announceBuf, L"Logged position: X %d, Y %d, Z %d, heading %d", (int)myPos.x, (int)myPos.y, (int)myPos.z, (int)heading);
					A11y::speak(announceBuf, false);

					// Scan nearby objects within 8.0 meters
					int objects[256];
					int count = worldGetAllObjects(objects, 256);
					DebugLog::log("DIAGNOSTIC: Scanning nearby objects within 8 meters (found %d total in world):", count);
					int loggedCount = 0;
					for (int i = 0; i < count; ++i) {
						Object obj = objects[i];
						if (!obj || !ENTITY::DOES_ENTITY_EXIST(obj)) continue;
						Vector3 oPos = ENTITY::GET_ENTITY_COORDS(obj, TRUE, FALSE);
						float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(myPos.x, myPos.y, myPos.z, oPos.x, oPos.y, oPos.z, TRUE);
						if (dist <= 8.0f) {
							Hash modelHash = ENTITY::GET_ENTITY_MODEL(obj);
							DebugLog::log("  Object index %d, Hash: 0x%08X, Distance: %f, Position: (%f, %f, %f)", obj, modelHash, dist, oPos.x, oPos.y, oPos.z);
							loggedCount++;
						}
					}
					DebugLog::log("DIAGNOSTIC: Logged %d nearby objects within 8 meters.", loggedCount);

					// Scan player inventory for diagnostics
					int invId = invoke<int>(0x13D234A2A3F66E63, pp); // _INVENTORY_GET_INVENTORY_ID_FROM_PED
					if (invId == 0) invId = 1;
					DebugLog::log("DIAGNOSTIC: Scanning player inventory (invId=%d):", invId);
					int scanCount = 0;
					for (int i = 0; i < g_monitoredItemsCount; ++i) {
						int itemCount = invoke<int>(0xE787F05DFC977BDE, invId, g_monitoredItems[i].hash, FALSE);
						if (itemCount > 0) {
							DebugLog::log("  Item %d: %s (%ls) = %d", i, g_monitoredItems[i].codeName, g_monitoredItems[i].displayName, itemCount);
							scanCount++;
						}
					}
					DebugLog::log("DIAGNOSTIC: Scanned %d items with non-zero count.", scanCount);
					A11y::speak(L"Inventory logged", false);
				}
			}
		}
		
		menuController->Update();
		
		// Squad background maintenance (formation, defense, crowd control)
		SquadFrameMaintenance();

		// Wanted/bounty auto-announcements
		HandleWantedAnnouncements();

		// Accessibility auto-announcements
		HandleAutoMoneyAnnounce();
		HandleAutoHorseStateAnnounce();
		HandleAutoGuardStatusAnnounce();
		HandleAutoAnimalBehaviorAnnounce();
		HandleAutoHonorAnnounce();
		HandleAutoLootAnnounce();

		// Aiming target announcements
		HandleAimingAnnounce();

		// Auto-aim system (lock onto nearest hostile)
		HandleAutoAim();

		// Auto horse whistle/call detection
		if (g_autoHorseCallAnnounce) {
			Ped wpp = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(wpp) && !PED::IS_PED_ON_MOUNT(wpp)) {
				if (!g_horseWhistleActive) {
					// Detect whistle: try game controls + H key + detect horse approaching
					bool whistleDetected = IsKeyJustUp(0x48); // H key fallback
					// Try multiple whistle-related control IDs (varies by context)
					if (!whistleDetected) whistleDetected = CONTROLS::IS_CONTROL_JUST_PRESSED(0, 0x24978A28);
					if (!whistleDetected) whistleDetected = CONTROLS::IS_CONTROL_JUST_PRESSED(0, 0xE7EB9185);
					if (!whistleDetected) whistleDetected = CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 0x24978A28);
					if (!whistleDetected) whistleDetected = CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 0xE7EB9185);

					if (whistleDetected) {
						DebugLog::log("Whistle control or H key detected.");
					}

					// Also detect by monitoring horse: if horse was far and is now running toward player
					if (!whistleDetected) {
						Ped autoHorse = GetPlayerHorse(wpp);
						if (autoHorse && ENTITY::DOES_ENTITY_EXIST(autoHorse) && !ENTITY::IS_ENTITY_DEAD(autoHorse)) {
							float horseSpeed = ENTITY::GET_ENTITY_SPEED(autoHorse);
							if (horseSpeed > 4.0f) { // horse is running/galloping
								Vector3 pPos = ENTITY::GET_ENTITY_COORDS(wpp, TRUE, FALSE);
								Vector3 hPos = ENTITY::GET_ENTITY_COORDS(autoHorse, TRUE, FALSE);
								float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
								// If horse is running and within whistle range, likely was called
								if (dist > 10.0f && dist < 250.0f && g_lastHorseAnnouncedDist > 0 && dist < g_lastHorseAnnouncedDist - 2.0f) {
									whistleDetected = true;
								}
								g_lastHorseAnnouncedDist = dist;
							}
						}
					}

					if (whistleDetected) {
						Ped wHorse = GetPlayerHorse(wpp);
						if (wHorse && ENTITY::DOES_ENTITY_EXIST(wHorse) && !ENTITY::IS_ENTITY_DEAD(wHorse)) {
							Vector3 pPos = ENTITY::GET_ENTITY_COORDS(wpp, TRUE, FALSE);
							Vector3 hPos = ENTITY::GET_ENTITY_COORDS(wHorse, TRUE, FALSE);
							float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
							wchar_t buf[200];
							if (dist > 200.0f)
								swprintf_s(buf, L"Horse too far! %.0f meters", dist);
							else if (dist > 80.0f)
								swprintf_s(buf, L"Horse called. %.0f meters, coming", dist);
							else if (dist > 20.0f)
								swprintf_s(buf, L"Horse called. %.0f meters, approaching", dist);
							else
								swprintf_s(buf, L"Horse nearby. %.0f meters", dist);
							A11y::speak(buf, false);
							g_horseWhistleActive = true;
							g_horseWhistleStartMs = GetTickCount();
							g_lastHorseDistAnnounceMs = GetTickCount();
							g_lastHorseAnnouncedDist = dist;
							g_whistleTargetHorse = wHorse;
						}
					}
				}

				// Passive horse distance tracking when not actively whistling (update baseline)
				if (!g_horseWhistleActive) {
					DWORD now = GetTickCount();
					if ((now - g_lastHorseDistAnnounceMs) >= 2000) {
						g_lastHorseDistAnnounceMs = now;
						Ped pHorse = GetPlayerHorse(wpp);
						if (pHorse && ENTITY::DOES_ENTITY_EXIST(pHorse) && !ENTITY::IS_ENTITY_DEAD(pHorse)) {
							Vector3 pPos = ENTITY::GET_ENTITY_COORDS(wpp, TRUE, FALSE);
							Vector3 hPos = ENTITY::GET_ENTITY_COORDS(pHorse, TRUE, FALSE);
							g_lastHorseAnnouncedDist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
						}
					}
				}
			}
		}

		// Auto compass direction announcement
		if (g_autoCompassAnnounce) {
			DWORD cNow = GetTickCount();
			if ((cNow - g_lastCompassCheckMs) >= 500) {
				g_lastCompassCheckMs = cNow;
				Ped pp = PLAYER::PLAYER_PED_ID();
				if (ENTITY::DOES_ENTITY_EXIST(pp)) {
					float heading = ENTITY::GET_ENTITY_HEADING(pp);
					int dirIdx = GetCompassDirIndex(heading);
					if (dirIdx != g_lastCompassDir && g_lastCompassDir >= 0) {
						A11y::speak(GetCompassDirection(heading), false);
					}
					g_lastCompassDir = dirIdx;
				}
			}
		}

		// Auto horse whistle detection (H key)
		{
			DWORD wNow = GetTickCount();
			if ((wNow - g_lastWeaponCheckMs) >= 800) {
				g_lastWeaponCheckMs = wNow;
				Ped pp = PLAYER::PLAYER_PED_ID();
				if (ENTITY::DOES_ENTITY_EXIST(pp)) {
					Hash curWeapon = 0;
					WEAPON::GET_CURRENT_PED_WEAPON(pp, &curWeapon, TRUE, 0, FALSE);
					if (g_lastWeaponHash == 0) g_lastWeaponHash = curWeapon; // init
					if (curWeapon != g_lastWeaponHash) {
						const wchar_t* name = GetWeaponName(curWeapon);
						A11y::speak(name, false);
						g_lastWeaponHash = curWeapon;
					}
				}
			}
		}

		// Auto mount/dismount + horse proximity announcements
		{
			Ped pp = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(pp)) {
				bool isMounted = PED::IS_PED_ON_MOUNT(pp) ? true : false;
				
				// Mount transition: just got on horse
				if (isMounted && !g_wasPlayerMounted) {
					Ped horse = PED::GET_MOUNT(pp);
					if (horse && ENTITY::DOES_ENTITY_EXIST(horse)) {
						Hash model = ENTITY::GET_ENTITY_MODEL(horse);
						const wchar_t* hName = GetHorseName(model);
						int bond = invoke<int>(0xA4C8E23E29040DE0, horse, 7);
						if (bond < 0) bond = 0; if (bond > 4) bond = 4;
						wchar_t buf[200];
						swprintf_s(buf, L"Mounted %s. Bonding %d", hName, bond);
						A11y::speak(buf, false);
						g_lastAnnouncedHorse = horse;
					}
					g_wasPlayerMounted = true;
				}
				else if (!isMounted && g_wasPlayerMounted) {
					A11y::speak(L"Dismounted", false);
					g_wasPlayerMounted = false;
				}
				
				// Track horse arrival after whistle - continuous distance updates
				if (!isMounted && g_horseWhistleActive && g_whistleTargetHorse) {
					DWORD elapsed = GetTickCount() - g_horseWhistleStartMs;
					if (elapsed > 1500) {
						if (ENTITY::DOES_ENTITY_EXIST(g_whistleTargetHorse) && !ENTITY::IS_ENTITY_DEAD(g_whistleTargetHorse)) {
							Vector3 pPos = ENTITY::GET_ENTITY_COORDS(pp, TRUE, FALSE);
							Vector3 hPos = ENTITY::GET_ENTITY_COORDS(g_whistleTargetHorse, TRUE, FALSE);
							float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(pPos.x, pPos.y, pPos.z, hPos.x, hPos.y, hPos.z, TRUE);
							if (dist < 5.0f) {
								A11y::speak(L"Horse arrived", false);
								g_horseWhistleActive = false;
								g_whistleTargetHorse = 0;
							} else {
								// Announce distance every 3 seconds as horse approaches
								DWORD now = GetTickCount();
								if ((now - g_lastHorseDistAnnounceMs) >= 3000) {
									g_lastHorseDistAnnounceMs = now;
									wchar_t buf[100];
									swprintf_s(buf, L"Horse %.0f meters", dist);
									A11y::speak(buf, false);
									g_lastHorseAnnouncedDist = dist;
								}
							}
						}
						if (elapsed > 30000) {
							A11y::speak(L"Horse whistle timed out", false);
							g_horseWhistleActive = false;
							g_whistleTargetHorse = 0;
						}
					}
				}
			}
		}

		WAIT(0);
	}
	
	// Cleanup on exit
	A11y::shutdown();
}

void ScriptMain()
{
	srand(GetTickCount());
	main();
}
