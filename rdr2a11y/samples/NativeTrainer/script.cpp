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
#include <algorithm>
// GTA11Y-like aiming cues additions
#include <thread>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;

#include "scriptinfo.h"

// Last-known stable player money (in cents) as a fallback when direct reads are unreliable
int g_lastKnownMoneyCents = -1;

// Forward declaration (defined at bottom)
static bool TryReadHonor(int& outHonor);

// --- GTA11Y-like aiming cues state ---
static DWORD g_lastAimSoundMs = 0;
static Entity g_lastAimSoundEntity = 0;
static bool g_enableAimingWavCues = true;     // can be wired to a menu later
// (removed) camera pitch beep feature per user request
static DWORD g_lastHeadCueMs = 0;
static Entity g_lastHeadEntity = 0;

// Money announcement timing (debounce + min interval)
// Previously: stable=800ms, minInterval=500ms. Now faster by default.
static int g_moneyStableMs = 50;       // debounce required for value to be considered stable
static int g_moneyMinIntervalMs = 50;  // minimum gap between consecutive money announcements

// Navigation/heading announcements
static bool g_enableAutoZones = true;           // auto announce known/zone names
static bool g_enableAutoHeading = false;        // auto announce cardinal directions
static DWORD g_lastHeadingSpeakMs = 0;          // throttle heading speech
static int g_lastHeadingBucket = -1;            // last 8-way bucket announced
static wchar_t g_lastZone[128] = {0};           // last zone label announced
static DWORD g_lastZoneSpeakMs = 0;
static DWORD g_lastAutoLocationCheckMs = 0;     // throttle auto location checks

// Enhanced location detection function
static bool GetCurrentLocationName(wchar_t* locationResult, size_t bufferSize) {
	Ped me = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(me)) return false;

	Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
	bool locationFound = false;

	// Method 1: Try the original approach
	const char* gxt = reinterpret_cast<const char*>(static_cast<uintptr_t>(ZONE::_0x43AD8FC02B429D33(meC.x, meC.y, meC.z, 0)));
	if (gxt && *gxt) {
		if (UI::DOES_TEXT_LABEL_EXIST(const_cast<char*>(gxt))) {
			const char* text = UI::_GET_LABEL_TEXT(const_cast<char*>(gxt));
			if (text && *text) {
				size_t n = strlen(text); if (n > bufferSize-1) n = bufferSize-1; size_t converted = 0;
				mbstowcs_s(&converted, locationResult, bufferSize, text, n);
				locationResult[n] = L'\0';
				locationFound = true;
			}
		}
	}

	// Method 2: Try alternative ZONE function
	if (!locationFound) {
		Any altResult = ZONE::_0x5BA7A68A346A5A91(meC.x, meC.y, meC.z);
		if (altResult != 0) {
			swprintf_s(locationResult, bufferSize, L"Zone %d", (int)altResult);
			locationFound = true;
		}
	}

	// Method 3: Try MAPREGION functions
	if (!locationFound) {
		Any regionResult = MAPREGION::_0x2B32B11520626229(meC.x, meC.y, meC.z, 100.0f, 0);
		if (regionResult != 0) {
			Vector3 regionCoords = MAPREGION::_0xF70F00013A62F866(regionResult);
			if (regionCoords.x != 0.0f || regionCoords.y != 0.0f) {
				swprintf_s(locationResult, bufferSize, L"Region %.0f %.0f", regionCoords.x, regionCoords.y);
				locationFound = true;
			}
		}
	}

	// Method 4: Enhanced coordinate-based location names
	if (!locationFound) {
		// Saint Denis area (extended range)
		if (meC.x >= 2200 && meC.x <= 2800 && meC.y >= -1350 && meC.y <= -950) {
			wcscpy_s(locationResult, bufferSize, L"Saint Denis");
			locationFound = true;
		}
		// Valentine area
		else if (meC.x >= -350 && meC.x <= -150 && meC.y >= 600 && meC.y <= 900) {
			wcscpy_s(locationResult, bufferSize, L"Valentine");
			locationFound = true;
		}
		// Rhodes area
		else if (meC.x >= 1200 && meC.x <= 1500 && meC.y >= -1350 && meC.y <= -1050) {
			wcscpy_s(locationResult, bufferSize, L"Rhodes");
			locationFound = true;
		}
		// Strawberry area
		else if (meC.x >= -1900 && meC.x <= -1600 && meC.y >= -600 && meC.y <= -350) {
			wcscpy_s(locationResult, bufferSize, L"Strawberry");
			locationFound = true;
		}
		// Blackwater area
		else if (meC.x >= -900 && meC.x <= -600 && meC.y >= -1350 && meC.y <= -1050) {
			wcscpy_s(locationResult, bufferSize, L"Blackwater");
			locationFound = true;
		}
		// Annesburg area
		else if (meC.x >= 2800 && meC.x <= 3100 && meC.y >= 1100 && meC.y <= 1400) {
			wcscpy_s(locationResult, bufferSize, L"Annesburg");
			locationFound = true;
		}
		// Van Horn Trading Post
		else if (meC.x >= 2900 && meC.x <= 3200 && meC.y >= 500 && meC.y <= 800) {
			wcscpy_s(locationResult, bufferSize, L"Van Horn Trading Post");
			locationFound = true;
		}
		// Armadillo
		else if (meC.x >= -3600 && meC.x <= -3300 && meC.y >= -2700 && meC.y <= -2400) {
			wcscpy_s(locationResult, bufferSize, L"Armadillo");
			locationFound = true;
		}
		// Tumbleweed
		else if (meC.x >= -5400 && meC.x <= -5100 && meC.y >= -2900 && meC.y <= -2600) {
			wcscpy_s(locationResult, bufferSize, L"Tumbleweed");
			locationFound = true;
		}
		// MacFarlane's Ranch
		else if (meC.x >= -2400 && meC.x <= -2100 && meC.y >= -2200 && meC.y <= -1900) {
			wcscpy_s(locationResult, bufferSize, L"MacFarlane Ranch");
			locationFound = true;
		}
		// Emerald Ranch
		else if (meC.x >= 1400 && meC.x <= 1700 && meC.y >= 300 && meC.y <= 600) {
			wcscpy_s(locationResult, bufferSize, L"Emerald Ranch");
			locationFound = true;
		}
		// Wallace Station
		else if (meC.x >= -1400 && meC.x <= -1100 && meC.y >= 400 && meC.y <= 700) {
			wcscpy_s(locationResult, bufferSize, L"Wallace Station");
			locationFound = true;
		}
		// Lagras
		else if (meC.x >= 2100 && meC.x <= 2400 && meC.y >= -2050 && meC.y <= -1750) {
			wcscpy_s(locationResult, bufferSize, L"Lagras");
			locationFound = true;
		}
		// General regions based on coordinates
		else if (meC.x >= 2000 && meC.y >= -1500 && meC.y <= -800) {
			wcscpy_s(locationResult, bufferSize, L"Lemoyne region");
			locationFound = true;
		}
		else if (meC.x >= -1000 && meC.x <= 1500 && meC.y >= 0) {
			wcscpy_s(locationResult, bufferSize, L"Heartlands");
			locationFound = true;
		}
		else if (meC.x <= -1500 && meC.y >= -1000) {
			wcscpy_s(locationResult, bufferSize, L"West Elizabeth");
			locationFound = true;
		}
		else if (meC.x <= -2000 && meC.y <= -1500) {
			wcscpy_s(locationResult, bufferSize, L"New Austin");
			locationFound = true;
		}
		else if (meC.y >= 1500) {
			wcscpy_s(locationResult, bufferSize, L"Grizzlies");
			locationFound = true;
		}
	}

	return locationFound;
}

// A11y hotkey modes (cycled by NumPad 5)
enum class A11yMode { 
	Global = 0, 
	Horse = 1, 
	Wolves = 2, 
	Bodyguard = 3, 
	BlackDeath = 4     // Black Death comprehensive mode
};
static A11yMode g_a11yMode = A11yMode::Global;

// Plague system states (within BlackDeath mode)
enum class PlagueType {
	FullPlague = 0,      // Full plague: cough then death
	DiseaseOnly = 1,     // Disease only: cough without death
	InstantKill = 2      // Instant kill mode
};
static PlagueType g_plagueType = PlagueType::FullPlague;
static bool g_plagueSystemEnabled = false;
static DWORD g_lastPlagueScanMs = 0;

static inline const wchar_t* ModeName(A11yMode m) {
	switch (m) {
	case A11yMode::Global: return L"global";
	case A11yMode::Horse: return L"horse";
	case A11yMode::Wolves: return L"wolves";
	case A11yMode::Bodyguard: return L"bodyguard";
	case A11yMode::BlackDeath: return L"black death";
	default: return L"";
	}
}

static inline const wchar_t* PlagueTypeName(PlagueType p) {
	switch (p) {
	case PlagueType::FullPlague: return L"full plague";
	case PlagueType::DiseaseOnly: return L"disease only";
	case PlagueType::InstantKill: return L"instant kill";
	default: return L"";
	}
}

static inline int HeadingBucket(float h) {
	// Normalize 0..360, map to 8 buckets (N, NE, E, SE, S, SW, W, NW)
	while (h < 0) h += 360.0f; while (h >= 360.0f) h -= 360.0f;
	float step = 360.0f / 8.0f; int b = int((h + step/2) / step) % 8; return b;
}
static inline const wchar_t* BucketName8(int b) {
	static const wchar_t* names[8] = { L"north", L"north east", L"east", L"south east", L"south", L"south west", L"west", L"north west" };
	if (b < 0 || b > 7) return L""; return names[b];
}

static inline void PlayWavAsync(const wchar_t* fileName)
{
	if (!fileName || !*fileName) return;
	// Non-blocking, OS-mixed playback; relies on wavs sitting next to the ASI
	PlaySoundW(fileName, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

static inline void BeepAsync(int freq, int durMs)
{
	if (freq < 37) freq = 37; // Beep limits
	if (freq > 32767) freq = 32767;
	if (durMs < 5) durMs = 5; if (durMs > 200) durMs = 200;
	std::thread([freq, durMs]() {
		// Beep is blocking; run in a short-lived detached thread
		Beep(freq, durMs);
	}).detach();
}

// Best-effort wallet reader: prefer CASH natives, then fall back to PED::GET_PED_MONEY
static int ReadPlayerMoneyCents(Ped me)
{
	int best = -1;
	// Try CASH getters (names unknown in headers; using hashes via wrappers)
	// These return Any; interpret as int cents if plausible
	{
		int v0 = (int)CASH::_0x0C02DABFA3B98176();
		if (v0 > best) best = v0;
	}
	{
		int v1 = (int)CASH::_0xA46FD001D1BE896C();
		if (v1 > best) best = v1;
	}
	{
		int v2 = (int)CASH::_0x282D36FF103D78DF();
		if (v2 > best) best = v2;
	}
	{
		int v3 = (int)CASH::_0x8A67120DBC299525();
		if (v3 > best) best = v3;
	}
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

// Accessibility: master speech toggle and targeting cue toggle
class MenuItemA11yMasterSwitch : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		A11y::setEnabled(newState);
		SetState(newState);
	}
public:
	MenuItemA11yMasterSwitch(string caption)
		: MenuItemSwitchable(caption)
	{
		SetState(A11y::isEnabled());
	}
};

class MenuItemA11yAimingCues : public MenuItemSwitchable
{
	virtual void OnSelect()
	{
		bool newState = !GetState();
		g_enableAimingWavCues = newState;
		SetState(newState);
	}
public:
	MenuItemA11yAimingCues(string caption)
		: MenuItemSwitchable(caption)
	{
		SetState(g_enableAimingWavCues);
	}
};

class MenuItemA11yAutoZones : public MenuItemSwitchable
{
	virtual void OnSelect() { bool ns = !GetState(); g_enableAutoZones = ns; SetState(ns); }
public:
	MenuItemA11yAutoZones(string caption) : MenuItemSwitchable(caption) { SetState(g_enableAutoZones); }
};

class MenuItemA11yAutoHeading : public MenuItemSwitchable
{
	virtual void OnSelect() { bool ns = !GetState(); g_enableAutoHeading = ns; SetState(ns); }
public:
	MenuItemA11yAutoHeading(string caption) : MenuItemSwitchable(caption) { SetState(g_enableAutoHeading); }
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

MenuBase *CreateAccessibilityMenu(MenuController *controller)
{
	auto menu = new MenuBase(new MenuItemTitle("ACCESSIBILITY"));
	controller->RegisterMenu(menu);

	// Top-level master speech switch (default ON)
	menu->AddItem(new MenuItemA11yMasterSwitch("Enable accessibility (speech)"));

	// Group: Aiming
	menu->AddItem(new MenuItemTitle("AIMING"));
	menu->AddItem(new MenuItemA11yAimingCues("Target type sound cues"));

	// Group: Navigation
	menu->AddItem(new MenuItemTitle("NAVIGATION"));
	menu->AddItem(new MenuItemA11yAutoZones("Auto place/zone announcements"));
	menu->AddItem(new MenuItemA11yAutoHeading("Auto heading announcements"));

	// Future groups can be added here (navigation, combat assist, etc.)
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
	menu->AddItem(new MenuItemMenu("ACCESSIBILITY", CreateAccessibilityMenu(controller)));
	
	return menu;
}

// ===== PLAGUE SYSTEMS =====

// Disease Only Mode: Make people cough, sneeze, vomit, and fall down (no death)
static void DiseaseOnlySystem(DWORD& lastScanMs)
{
	DWORD now = GetTickCount();
	if ((now - lastScanMs) < 2000) return; // scan every 2 seconds
	lastScanMs = now;
	
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed)) return;
	
	// Find nearby peds within 50m
	int packed[33] = { 32 };
	int count = PED::GET_PED_NEARBY_PEDS(playerPed, packed, -1, 0);
	if (count <= 0) return;
	
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	int lim = packed[0]; if (lim > 32) lim = 32;
	
	for (int i = 1; i <= lim; ++i) {
		Ped target = (Ped)packed[i];
		if (!target || target == playerPed) continue;
		if (!ENTITY::DOES_ENTITY_EXIST(target) || ENTITY::IS_ENTITY_DEAD(target)) continue;
		if (PED::IS_PED_A_PLAYER(target)) continue;
		
		Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, TRUE, FALSE);
		float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z,
		                                                       targetPos.x, targetPos.y, targetPos.z, TRUE);
		if (distance > 50.0f) continue;
		
		// Make them sick with animations
		STREAMING::REQUEST_ANIM_DICT((char*)"amb_misc@world_human_coughing");
		if (STREAMING::HAS_ANIM_DICT_LOADED((char*)"amb_misc@world_human_coughing")) {
			AI::TASK_PLAY_ANIM(target, (char*)"amb_misc@world_human_coughing", (char*)"coughing", 4.0f, -8.0f, 4000, 1, 0.0f, FALSE, FALSE, FALSE, 0, FALSE);
		}
		
		// Random chance for vomiting or falling
		int action = rand() % 3;
		if (action == 0) {
			// Coughing (already done above)
		} else if (action == 1) {
			// Vomiting animation
			STREAMING::REQUEST_ANIM_DICT((char*)"creatures@rottweiler@amb@world_dog_sitting@base");
			if (STREAMING::HAS_ANIM_DICT_LOADED((char*)"creatures@rottweiler@amb@world_dog_sitting@base")) {
				AI::TASK_PLAY_ANIM(target, (char*)"creatures@rottweiler@amb@world_dog_sitting@base", (char*)"base", 4.0f, -8.0f, 3000, 1, 0.0f, FALSE, FALSE, FALSE, 0, FALSE);
			}
		} else if (action == 2) {
			// Fall down weakly
			PED::SET_PED_TO_RAGDOLL(target, 2000, 4000, 0, TRUE, TRUE, FALSE);
		}
	}
}

// Instant Kill Mode: Kill immediately when detected
static void InstantKillSystem(DWORD& lastScanMs)
{
	DWORD now = GetTickCount();
	if ((now - lastScanMs) < 500) return; // scan every 0.5 seconds for quick response
	lastScanMs = now;
	
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed)) return;
	
	// Find nearby peds within 50m
	int packed[33] = { 32 };
	int count = PED::GET_PED_NEARBY_PEDS(playerPed, packed, -1, 0);
	if (count <= 0) return;
	
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	int lim = packed[0]; if (lim > 32) lim = 32;
	
	for (int i = 1; i <= lim; ++i) {
		Ped target = (Ped)packed[i];
		if (!target || target == playerPed) continue;
		if (!ENTITY::DOES_ENTITY_EXIST(target) || ENTITY::IS_ENTITY_DEAD(target)) continue;
		if (PED::IS_PED_A_PLAYER(target)) continue;
		
		Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, TRUE, FALSE);
		float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z,
		                                                       targetPos.x, targetPos.y, targetPos.z, TRUE);
		if (distance > 50.0f) continue;
		
		// Instant death - no delay
		ENTITY::SET_ENTITY_HEALTH(target, 0, 0);
	}
}

// Black Death Mode: Cough, sneeze, get sick for 6-10 seconds, then die
static void BlackDeathSystem(DWORD& lastScanMs)
{
	static std::unordered_map<Ped, DWORD> infectedPeds; // Track infection times
	
	DWORD now = GetTickCount();
	if ((now - lastScanMs) < 1500) return; // scan every 1.5 seconds
	lastScanMs = now;
	
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed)) return;
	
	// Find nearby peds within 50m
	int packed[33] = { 32 };
	int count = PED::GET_PED_NEARBY_PEDS(playerPed, packed, -1, 0);
	if (count > 0) {
		Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
		int lim = packed[0]; if (lim > 32) lim = 32;
		
		for (int i = 1; i <= lim; ++i) {
			Ped target = (Ped)packed[i];
			if (!target || target == playerPed) continue;
			if (!ENTITY::DOES_ENTITY_EXIST(target) || ENTITY::IS_ENTITY_DEAD(target)) continue;
			if (PED::IS_PED_A_PLAYER(target)) continue;
			
			Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, TRUE, FALSE);
			float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z,
			                                                       targetPos.x, targetPos.y, targetPos.z, TRUE);
			if (distance > 50.0f) continue;
			
			// If not already infected, infect them
			if (infectedPeds.find(target) == infectedPeds.end()) {
				infectedPeds[target] = now;
				
				// Start coughing animation
				STREAMING::REQUEST_ANIM_DICT((char*)"amb_misc@world_human_coughing");
				if (STREAMING::HAS_ANIM_DICT_LOADED((char*)"amb_misc@world_human_coughing")) {
					AI::TASK_PLAY_ANIM(target, (char*)"amb_misc@world_human_coughing", (char*)"coughing", 4.0f, -8.0f, -1, 1, 0.0f, FALSE, FALSE, FALSE, 0, FALSE);
				}
			}
		}
	}
	
	// Check infected peds for death timing (6-10 seconds after infection)
	auto it = infectedPeds.begin();
	while (it != infectedPeds.end()) {
		Ped infected = it->first;
		DWORD infectionTime = it->second;
		
		if (!ENTITY::DOES_ENTITY_EXIST(infected) || ENTITY::IS_ENTITY_DEAD(infected)) {
			it = infectedPeds.erase(it);
			continue;
		}
		
		DWORD sickDuration = now - infectionTime;
		DWORD deathTime = 6000 + (rand() % 4000); // 6-10 seconds
		
		if (sickDuration > deathTime) {
			// Time to die
			ENTITY::SET_ENTITY_HEALTH(infected, 0, 0);
			it = infectedPeds.erase(it);
		} else {
			++it;
		}
	}
}

// Plague System Manager: Central handler for all plague types
static void PlagueSystemManager(DWORD& lastScanMs)
{
	if (!g_plagueSystemEnabled || g_a11yMode != A11yMode::BlackDeath) return;
	
	switch (g_plagueType) {
	case PlagueType::FullPlague:
		BlackDeathSystem(lastScanMs);
		break;
	case PlagueType::DiseaseOnly:
		DiseaseOnlySystem(lastScanMs);
		break;
	case PlagueType::InstantKill:
		InstantKillSystem(lastScanMs);
		break;
	default:
		break;
	}
}

// Toggle Plague System: Switches between plague types and manages enable/disable
static void TogglePlagueSystem(PlagueType newType)
{
	if (g_a11yMode != A11yMode::BlackDeath) {
		A11y::speak(L"Plague system only available in Black Death mode", true);
		return;
	}
	
	bool wasEnabled = g_plagueSystemEnabled;
	bool wasCurrentType = (g_plagueType == newType);
	
	if (wasCurrentType) {
		// Toggle current type on/off
		g_plagueSystemEnabled = !g_plagueSystemEnabled;
	} else {
		// Switch to new type and enable
		g_plagueType = newType;
		g_plagueSystemEnabled = true;
	}
	
	const wchar_t* typeName = PlagueTypeName(g_plagueType);
	wchar_t buf[80];
	swprintf_s(buf, L"%s %s", typeName, g_plagueSystemEnabled ? L"enabled" : L"disabled");
	A11y::speak(buf, true);
}

// Helper function to assign kill task to a bodyguard
static void AssignKillTask(Ped bodyguard, Ped target)
{
	if (!bodyguard || !target || !ENTITY::DOES_ENTITY_EXIST(bodyguard) || !ENTITY::DOES_ENTITY_EXIST(target)) return;
	
	// Clear current tasks and move to target
	AI::CLEAR_PED_TASKS_IMMEDIATELY(bodyguard, TRUE, TRUE);
	AI::TASK_GO_TO_ENTITY(bodyguard, target, -1, 1.5f, 2.0f, 1073741824, 0);
	
	// Set bodyguard to be aggressive
	PED::SET_PED_COMBAT_ABILITY(bodyguard, 2); // professional
	PED::SET_PED_COMBAT_RANGE(bodyguard, 2); // far
	
	// Force combat with target
	AI::TASK_COMBAT_PED(bodyguard, target, 0, 16);
}

// Enhanced Bodyguard Protection System: Kill anyone in 30m radius with smart target distribution
static void BodyguardProtectionSystem(bool protectionEnabled, DWORD& lastScanMs, Ped primaryBodyguard)
{
	if (!protectionEnabled || !primaryBodyguard || !ENTITY::DOES_ENTITY_EXIST(primaryBodyguard) || ENTITY::IS_ENTITY_DEAD(primaryBodyguard)) {
		return;
	}
	
	DWORD now = GetTickCount();
	if ((now - lastScanMs) < 1500) return; // scan every 1.5 seconds
	lastScanMs = now;
	
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed) || ENTITY::IS_ENTITY_DEAD(playerPed)) return;
	
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
	
	// Get all available bodyguards
	std::vector<Ped> availableBodyguards;
	availableBodyguards.push_back(primaryBodyguard);
	
	// Find additional bodyguards nearby (allies of the primary bodyguard)
	int packedAllies[33] = { 32 };
	int allyCount = PED::GET_PED_NEARBY_PEDS(primaryBodyguard, packedAllies, -1, 0);
	if (allyCount > 0) {
		int allyLim = packedAllies[0]; if (allyLim > 32) allyLim = 32;
		for (int i = 1; i <= allyLim && availableBodyguards.size() < 5; ++i) { // max 5 bodyguards
			Ped ally = (Ped)packedAllies[i];
			if (ally && ally != playerPed && ENTITY::DOES_ENTITY_EXIST(ally) && !ENTITY::IS_ENTITY_DEAD(ally)) {
				float allyDist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z,
					ENTITY::GET_ENTITY_COORDS(ally, TRUE, FALSE).x,
					ENTITY::GET_ENTITY_COORDS(ally, TRUE, FALSE).y,
					ENTITY::GET_ENTITY_COORDS(ally, TRUE, FALSE).z, TRUE);
				if (allyDist < 50.0f && !PED::IS_PED_IN_COMBAT(ally, playerPed)) {
					availableBodyguards.push_back(ally);
				}
			}
		}
	}
	
	// Collect all targets within 30m radius
	std::vector<std::pair<Ped, float>> targets; // target and distance
	int packed[33] = { 32 };
	int count = PED::GET_PED_NEARBY_PEDS(playerPed, packed, -1, 0);
	if (count > 0) {
		int lim = packed[0]; if (lim > 32) lim = 32;
		for (int i = 1; i <= lim; ++i) {
			Ped target = (Ped)packed[i];
			if (!target || target == playerPed) continue;
			if (!ENTITY::DOES_ENTITY_EXIST(target) || ENTITY::IS_ENTITY_DEAD(target)) continue;
			if (PED::IS_PED_A_PLAYER(target)) continue; // never target players
			
			// Skip if target is one of our bodyguards
			bool isBodyguard = false;
			for (Ped bg : availableBodyguards) {
				if (target == bg) { isBodyguard = true; break; }
			}
			if (isBodyguard) continue;
			
			// Check distance (30 meter kill radius)
			Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, TRUE, FALSE);
			float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z, 
			                                                       targetPos.x, targetPos.y, targetPos.z, TRUE);
			if (distance <= 30.0f) {
				targets.push_back(std::make_pair(target, distance));
			}
		}
	}
	
	if (targets.empty()) return;
	
	// Sort targets by distance (closest first)
	std::sort(targets.begin(), targets.end(), [](const std::pair<Ped, float>& a, const std::pair<Ped, float>& b) {
		return a.second < b.second;
	});
	
	// Assign targets to bodyguards intelligently
	int numBodyguards = availableBodyguards.size();
	int numTargets = targets.size();
	
	if (numBodyguards == 1) {
		// One bodyguard: target closest, then next closest after finishing
		Ped bodyguard = availableBodyguards[0];
		if (!PED::IS_PED_IN_COMBAT(bodyguard, 0)) { // if not already busy
			Ped target = targets[0].first;
			AssignKillTask(bodyguard, target);
		}
	}
	else if (numBodyguards >= 2) {
		// Multiple bodyguards: assign closest targets first
		int assignedTargets = 0;
		for (int i = 0; i < numBodyguards && assignedTargets < numTargets; ++i) {
			Ped bodyguard = availableBodyguards[i];
			if (!PED::IS_PED_IN_COMBAT(bodyguard, 0)) { // if not already busy
				Ped target = targets[assignedTargets].first;
				AssignKillTask(bodyguard, target);
				assignedTargets++;
			}
		}
	}
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
	// Bodyguard protection system
	static bool g_protectionEnabled = false;    // auto-protection toggle
	static DWORD g_lastProtectionScanMs = 0;    // last scan time
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

		// NumPad 1 (Global): announce player's health
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && IsKeyJustUp(VK_NUMPAD1))
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

		// NumPad 2 (Global): announce what you're riding/driving
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && IsKeyJustUp(VK_NUMPAD2))
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
				if (!modelName.empty()) { size_t n = modelName.size(); if (n > 95) n = 95; size_t cv = 0; mbstowcs_s(&cv, wmodel, 96, modelName.c_str(), n); wmodel[n] = L'\0'; }
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
		// NumPad 4: feed horse (Horse mode)
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Horse && IsKeyJustUp(VK_NUMPAD4))
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

	// NumPad 5: Cycle A11y mode (Global -> Horse -> Wolves -> Bodyguard -> BlackDeath)
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_NUMPAD5))
		{
			int m = (int)g_a11yMode;
			m = (m + 1) % 5;  // Now we have 5 main modes
			g_a11yMode = (A11yMode)m;
			const wchar_t* name = ModeName(g_a11yMode);
			wchar_t buf[64]; swprintf_s(buf, L"%s", name);
			A11y::speak(buf, true);
			
			// Disable plague systems when changing mode
			g_plagueSystemEnabled = false;
		}

	// Horse mode: NEW LAYOUT - 1: quick status, 2: health details, 3: bond/calm, 4: summon/dismiss, 6: speed boost, 7: detailed stats, 8: horse heading
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Horse)
		{
			// NumPad 1: Quick horse status
			if (IsKeyJustUp(VK_NUMPAD1)) {
				Ped me = PLAYER::PLAYER_PED_ID();
				if (PED::IS_PED_ON_MOUNT(me)) {
					Ped h = PED::GET_MOUNT(me);
					if (ENTITY::DOES_ENTITY_EXIST(h) && !ENTITY::IS_ENTITY_DEAD(h)) {
						int hp = ENTITY::GET_ENTITY_HEALTH(h);
						int mh = PED::GET_PED_MAX_HEALTH(h); if (mh <= 0) mh = 1; if (hp > mh) hp = mh;
						int pct = (int)((hp * 100.0f) / (float)mh + 0.5f);
						wchar_t buf[96]; swprintf_s(buf, L"horse health %d%%, stamina ready", pct);
						A11y::speak(buf, true);
					} else A11y::speak(L"no valid horse", true);
				} else A11y::speak(L"not on horse", true);
			}
			
			// NumPad 2: Detailed health information
			if (IsKeyJustUp(VK_NUMPAD2)) {
				Ped me = PLAYER::PLAYER_PED_ID();
				if (PED::IS_PED_ON_MOUNT(me)) {
					Ped h = PED::GET_MOUNT(me);
					if (ENTITY::DOES_ENTITY_EXIST(h) && !ENTITY::IS_ENTITY_DEAD(h)) {
						int hp = ENTITY::GET_ENTITY_HEALTH(h);
						int mh = PED::GET_PED_MAX_HEALTH(h); if (mh <= 0) mh = 1; if (hp > mh) hp = mh;
						int pct = (int)((hp * 100.0f) / (float)mh + 0.5f);
						wchar_t buf[120]; swprintf_s(buf, L"horse health %d of %d (%d%%), stamina 100%%", hp, mh, pct);
						A11y::speak(buf, true);
					} else A11y::speak(L"no valid horse", true);
				} else A11y::speak(L"not on horse", true);
			}
			
			// NumPad 3: Bond level and calm horse
			if (IsKeyJustUp(VK_NUMPAD3)) {
				Ped playerPed = PLAYER::PLAYER_PED_ID();
				if (PED::IS_PED_ON_MOUNT(playerPed)) {
					Ped horse = PED::GET_MOUNT(playerPed);
					if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse)) { 
						PED::CLEAR_PED_WETNESS(horse); 
						A11y::speak(L"horse calmed and bonded", true); 
					}
					else { A11y::speak(L"no valid horse", true); }
				} else { A11y::speak(L"not on horse", true); }
			}
			
			// NumPad 4: Feed horse (heal and restore stamina)
			if (IsKeyJustUp(VK_NUMPAD4)) {
				Ped playerPed = PLAYER::PLAYER_PED_ID();
				if (PED::IS_PED_ON_MOUNT(playerPed)) {
					Ped horse = PED::GET_MOUNT(playerPed);
					if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse)) {
						int h = ENTITY::GET_ENTITY_HEALTH(horse);
						int mh = PED::GET_PED_MAX_HEALTH(horse); if (mh <= 0) mh = 1; if (h > mh) h = mh;
						int add = mh / 4; if (add < 10) add = 10; int newH = h + add; if (newH > mh) newH = mh; ENTITY::SET_ENTITY_HEALTH(horse, newH, FALSE);
						PED::SET_PED_STAMINA(horse, 100.0);
						int pct = static_cast<int>((newH * 100.0f) / static_cast<float>(mh) + 0.5f); if (pct > 100) pct = 100; if (pct < 0) pct = 0;
						wchar_t buf[128]; swprintf_s(buf, L"fed horse, health %d (%d%%), stamina 100%%", newH, pct);
						A11y::speak(buf, true);
					} else { A11y::speak(L"no valid horse", true); }
				} else { A11y::speak(L"not on horse", true); }
			}
			
			// NumPad 7: Detailed horse statistics
			if (IsKeyJustUp(VK_NUMPAD7)) {
				Ped me = PLAYER::PLAYER_PED_ID();
				if (PED::IS_PED_ON_MOUNT(me)) {
					Ped horse = PED::GET_MOUNT(me);
					if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse)) {
						int hp = ENTITY::GET_ENTITY_HEALTH(horse);
						int mh = PED::GET_PED_MAX_HEALTH(horse); if (mh <= 0) mh = 1; if (hp > mh) hp = mh;
						int hp_pct = (int)((hp * 100.0f) / (float)mh + 0.5f);
						float speed = ENTITY::GET_ENTITY_SPEED(horse) * 3.6f;
						
						wchar_t buf[120]; swprintf_s(buf, L"horse: health %d%%, speed %.0f kmh", hp_pct, speed);
						A11y::speak(buf, true);
					} else A11y::speak(L"no valid horse", true);
				} else A11y::speak(L"not on horse", true);
			}
			
			// NumPad 8: Horse heading direction
			if (IsKeyJustUp(VK_NUMPAD8)) {
				Ped me = PLAYER::PLAYER_PED_ID();
				if (PED::IS_PED_ON_MOUNT(me)) {
					Ped horse = PED::GET_MOUNT(me);
					if (ENTITY::DOES_ENTITY_EXIST(horse) && !ENTITY::IS_ENTITY_DEAD(horse)) {
						float heading = ENTITY::GET_ENTITY_HEADING(horse);
						int b = HeadingBucket(heading);
						const wchar_t* wname = BucketName8(b);
						if (wname && *wname) { 
							wchar_t buf[80]; swprintf_s(buf, L"horse facing %s", wname); 
							A11y::speak(buf, true); 
						} else A11y::speak(L"horse heading", true);
					} else A11y::speak(L"no valid horse", true);
				} else A11y::speak(L"not on horse", true);
			}
		}

		// Removed on-demand camera scan (NumPad 0); automatic announcements occur while aiming only

	// NumPad 3 (Global): on-demand current weapon info (name only)
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && IsKeyJustUp(VK_NUMPAD3))
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
			if (!uname.empty()) { size_t n = uname.size(); if (n > 95) n = 95; size_t cv = 0; mbstowcs_s(&cv, wname, 96, uname.c_str(), n); wname[n] = L'\0'; }
		if (wname[0]) A11y::speak(wname, true);
		else A11y::speak(L"weapon", true);
			}
			else
			{
				A11y::speak(L"unarmed", true);
			}
			after_num3:;
		}

	// NumPad 6 (Global): on-demand money total (USD dollars)
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && IsKeyJustUp(VK_NUMPAD6))
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

	// Bodyguard mode: NEW LAYOUT - 1: status, 2: teleport to player, 3: follow close, 4: toggle bodyguard, 6: protection toggle, 7: detailed status
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Bodyguard)
		{
			// NumPad 1: Quick bodyguard status
			if (IsKeyJustUp(VK_NUMPAD1)) {
				if (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard) && !ENTITY::IS_ENTITY_DEAD(g_bodyguard)) {
					int hp = ENTITY::GET_ENTITY_HEALTH(g_bodyguard);
					int mh = PED::GET_PED_MAX_HEALTH(g_bodyguard); if (mh<=0) mh=1; if (hp>mh) hp=mh; int pct=(int)((hp*100.0f)/(float)mh+0.5f);
					wchar_t buf[80]; swprintf_s(buf, L"bodyguard health %d%%", pct); A11y::speak(buf, true);
				} else A11y::speak(L"no bodyguard", true);
			}
			
			// NumPad 2: Teleport bodyguard to player
			if (IsKeyJustUp(VK_NUMPAD2)) {
				if (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard)) {
					Ped me = PLAYER::PLAYER_PED_ID(); Vector3 p = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(me, 0.0f, -1.4f, 0.0f);
					ENTITY::SET_ENTITY_COORDS(g_bodyguard, p.x, p.y, p.z, FALSE, FALSE, FALSE, TRUE);
					A11y::speak(L"bodyguard here", true);
				} else A11y::speak(L"no bodyguard", true);
			}
			
			// NumPad 3: Order close follow
			if (IsKeyJustUp(VK_NUMPAD3)) {
				if (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard)) {
					Ped me = PLAYER::PLAYER_PED_ID(); AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_bodyguard, me, 0.0f, -1.4f, 0.0f, 3.0f, -1, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
					g_bodyguardLastTaskMs = GetTickCount(); A11y::speak(L"following", true);
				} else A11y::speak(L"no bodyguard", true);
			}
			
			// NumPad 4: Toggle bodyguard
			if (IsKeyJustUp(VK_NUMPAD4)) {
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
			
			// NumPad 6: Toggle auto-protection (choke and burn hostiles)
			if (IsKeyJustUp(VK_NUMPAD6)) {
				g_protectionEnabled = !g_protectionEnabled;
				if (g_protectionEnabled) {
					A11y::speak(L"Protection activated. Guards will eliminate all threats.", true);
				} else {
					A11y::speak(L"Protection deactivated. Guards on standby.", true);
				}
			}
			
			// NumPad 7: Detailed bodyguard status
			if (IsKeyJustUp(VK_NUMPAD7)) {
				if (g_bodyguard && ENTITY::DOES_ENTITY_EXIST(g_bodyguard) && !ENTITY::IS_ENTITY_DEAD(g_bodyguard)) {
					int hp = ENTITY::GET_ENTITY_HEALTH(g_bodyguard);
					int mh = PED::GET_PED_MAX_HEALTH(g_bodyguard); if (mh<=0) mh=1; if (hp>mh) hp=mh; int pct=(int)((hp*100.0f)/(float)mh+0.5f);
					bool inCombat = PED::IS_PED_IN_COMBAT(g_bodyguard, 0) ? true : false;
					wchar_t buf[120]; swprintf_s(buf, L"bodyguard health %d of %d (%d%%), %s", hp, mh, pct, inCombat ? L"in combat" : L"ready");
					A11y::speak(buf, true);
				} else A11y::speak(L"no bodyguard", true);
			}
		}

		// Wolves mode: NEW LAYOUT - 1: status, 2: gather, 3: call/regroup, 4: toggle wolf, 6: attack, 7: increase pack, 8: decrease pack, 9: pack status, 0: clear pack
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Wolves)
		{
			// NumPad 1: Wolf status
			if (IsKeyJustUp(VK_NUMPAD1)) {
				if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf)) {
					int hp = ENTITY::GET_ENTITY_HEALTH(g_wolf); int mh = PED::GET_PED_MAX_HEALTH(g_wolf); if(mh<=0) mh=1; if(hp>mh) hp=mh; int pct=(int)((hp*100.0f)/(float)mh+0.5f);
					wchar_t buf[64]; swprintf_s(buf, L"wolf health %d (%d%%)", hp, pct); A11y::speak(buf, true);
				} else A11y::speak(L"no wolf", true);
			}
			
			// NumPad 2: Gather pack
			if (IsKeyJustUp(VK_NUMPAD2)) {
				if (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf)) {
					Ped me = PLAYER::PLAYER_PED_ID(); if (ENTITY::DOES_ENTITY_EXIST(me)) {
						AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_wolf, me, -0.5f, -1.6f, 0.0f, 3.2f, 2500, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						for (int i=0;i<g_wolfPackSize;++i){ Ped wp=g_wolfPack[i]; if(!wp||ENTITY::IS_ENTITY_DEAD(wp)) continue; AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(wp, me, -0.6f, -2.0f, 0.0f, 3.0f, 2500, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);} A11y::speak(L"gather", true);
					}
				} else A11y::speak(L"no wolf", true);
			}
			
			// NumPad 3: Call/regroup near player
			if (IsKeyJustUp(VK_NUMPAD3)) {
				if (!(g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf))) { A11y::speak(L"no wolf", true); }
				else {
					Ped me = PLAYER::PLAYER_PED_ID();
					bool didCall = TryPlayerCall(me);
					TryFrontendBeep();
					g_forcedHowlAll = true; g_forcedHowlAtMs = GetTickCount() + 500;
					int maxAllowed = (g_targetPackSize > 0 ? g_targetPackSize : MAX_WOLF_PACK);
					if (g_wolfPackSize < maxAllowed) { g_howlsRemaining = (g_howlsRemaining < 2 ? 2 : g_howlsRemaining); g_nextHowlAtMs = GetTickCount(); }
					A11y::speak(didCall ? L"calling" : L"call", true);
					if (ENTITY::DOES_ENTITY_EXIST(me)) {
						AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(g_wolf, me, -0.5f, -1.6f, 0.0f, 3.2f, 2000, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						g_wolfLastTaskMs = GetTickCount();
						for (int i = 0; i < g_wolfPackSize; ++i) {
							Ped wp = g_wolfPack[i]; if (!wp || ENTITY::IS_ENTITY_DEAD(wp)) continue;
							AI::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(wp, me, -0.6f, -2.0f, 0.0f, 3.2f, 2000, 1.5f, TRUE, FALSE, FALSE, FALSE, FALSE);
						}
					}
				}
			}
			
			// NumPad 4: Toggle wolf companion
			if (IsKeyJustUp(VK_NUMPAD4)) {
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
			
			// NumPad 7: Increase pack size (+2)
			if (IsKeyJustUp(VK_NUMPAD7)) {
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
			
			// NumPad 8: Decrease pack size (-1)
			if (IsKeyJustUp(VK_NUMPAD8)) {
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
				}
				wchar_t buf[64]; swprintf_s(buf, L"pack %d", g_targetPackSize);
				A11y::speak(buf, true);
			}
			
			// NumPad 9: Pack status
			if (IsKeyJustUp(VK_NUMPAD9)) {
				wchar_t buf[64]; swprintf_s(buf, L"pack %d", g_wolfPackSize);
				A11y::speak(buf, true);
			}
			
			// NumPad 6: Command attack
			if (IsKeyJustUp(VK_NUMPAD6)) {
				Entity t = 0; Player pl = PLAYER::PLAYER_ID();
				if (!PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(pl, &t) || !ENTITY::DOES_ENTITY_EXIST(t)) {
					Ped me = PLAYER::PLAYER_PED_ID(); Ped mt = PED::GET_MELEE_TARGET_FOR_PED(me); if (mt && ENTITY::DOES_ENTITY_EXIST(mt)) t = mt;
					if ((!t || !ENTITY::DOES_ENTITY_EXIST(t)) && lastAimedEntity && ENTITY::DOES_ENTITY_EXIST(lastAimedEntity)) t = lastAimedEntity;
				}
				if (t && ENTITY::DOES_ENTITY_EXIST(t) && ENTITY::IS_ENTITY_A_PED(t)) {
					bool wolfReady = (g_wolf && ENTITY::DOES_ENTITY_EXIST(g_wolf) && !ENTITY::IS_ENTITY_DEAD(g_wolf));
					if (wolfReady) {
						PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(g_wolf, TRUE);
						PED::SET_PED_COMBAT_ABILITY(g_wolf, 2);
						PED::SET_PED_COMBAT_MOVEMENT(g_wolf, 2);
						PED::SET_PED_FLEE_ATTRIBUTES(g_wolf, 0, FALSE);
						AI::TASK_COMBAT_PED(g_wolf, (Ped)t, 0, 0);
						g_pendingAttackTarget = t; A11y::speak(L"wolf attacking", true);
					} else { A11y::speak(L"no wolf", true); }
				} else { A11y::speak(L"no target", true); }
			}
			
			// NumPad 0: Clear/dismiss helper pack (keep main wolf)
			if (IsKeyJustUp(VK_NUMPAD0)) {
				g_targetPackSize = 0;
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
				A11y::speak(L"pack 0", true);
			}
		}

		// NumPad 4 (Global): On-demand location using enhanced detection
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && IsKeyJustUp(VK_NUMPAD4))
		{
			wchar_t locationResult[256] = L"";
			if (GetCurrentLocationName(locationResult, 256)) {
				A11y::speak(locationResult, true);
			} else {
				A11y::speak(L"location unknown", true);
			}
		}

		// NumPad 7/Home (Global): on-demand honor + bounty/wanted status (Home when NumLock is off)
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && (IsKeyJustUp(VK_NUMPAD7) || IsKeyJustUp(VK_HOME)))
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

	// NumPad 7: Wolves mode — make wolf howl and call pack (increase desired pack)
	if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Wolves && IsKeyJustUp(VK_NUMPAD7))
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

		// NumPad .: Global -> speak heading only
		if (!menuController->HasActiveMenu() && IsKeyJustUp(VK_DECIMAL))
		{
			if (g_a11yMode == A11yMode::Global) {
				Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
				int b = HeadingBucket(camRot.z);
				const wchar_t* wname = BucketName8(b);
				if (wname && *wname) A11y::speak(wname, true); else A11y::speak(L"heading", true);
			}
		}

		// NumPad 0: Clear/dismiss helper pack (keep main wolf)
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Wolves && IsKeyJustUp(VK_NUMPAD0))
		{
			g_targetPackSize = 0;
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
			A11y::speak(L"pack 0", true);
		}

		// Global: also allow heading on Numpad 8 for sequential layout lovers
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::Global && IsKeyJustUp(VK_NUMPAD8))
		{
			Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
			int b = HeadingBucket(camRot.z);
			const wchar_t* wname = BucketName8(b);
			if (wname && *wname) A11y::speak(wname, true); else A11y::speak(L"heading", true);
		}

		// Auto-navigation announcements: zone/place and heading
		if (!menuController->HasActiveMenu())
		{
			DWORD now = GetTickCount();
			// Zone/place change announcement
			if (g_enableAutoZones)
			{
				Ped me = PLAYER::PLAYER_PED_ID();
				if (ENTITY::DOES_ENTITY_EXIST(me)) {
					Vector3 meC = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
					// Call hashed native directly to avoid modifying headers
					const char* gxt = reinterpret_cast<const char*>(static_cast<uintptr_t>(ZONE::_0x43AD8FC02B429D33(meC.x, meC.y, meC.z, 0)));
					const char* text = nullptr;
					if (gxt && *gxt) {
						if (UI::DOES_TEXT_LABEL_EXIST(const_cast<char*>(gxt))) {
							text = UI::_GET_LABEL_TEXT(const_cast<char*>(gxt));
						} else {
							text = gxt;
						}
					}
					if (text && *text) {
						wchar_t wlab[128]; wlab[0] = L'\0';
						size_t n = strlen(text); if (n > 127) n = 127; size_t converted=0; mbstowcs_s(&converted, wlab, 128, text, n);
						if (wlab[0]) {
							if (wcscmp(wlab, g_lastZone) != 0 && (now - g_lastZoneSpeakMs) > 600) {
								wcsncpy_s(g_lastZone, wlab, _TRUNCATE);
								g_lastZoneSpeakMs = now;
								A11y::speak(wlab, true);
							}
						}
					}
				}
			}
			// Heading announcements (8-way cardinal) on bucket change
			if (g_enableAutoHeading)
			{
				// Use camera heading for player orientation
				Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
				float heading = camRot.z; // degrees
				int b = HeadingBucket(heading);
				if (b != g_lastHeadingBucket && (now - g_lastHeadingSpeakMs) > 450) {
					g_lastHeadingBucket = b; g_lastHeadingSpeakMs = now;
					const wchar_t* wname = BucketName8(b);
					if (wname && *wname) A11y::speak(wname, true);
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
					bool stable = (now - moneyCandidateSinceMs >= g_moneyStableMs);
					if (stable && cur != lastMoneyCents && (now - lastMoneyAnnounceMs > g_moneyMinIntervalMs)) {
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
							if (attributed && src[0]) swprintf_s(buf, L"%s gave you %d.%02d$ - You have %d.%02d$", src, d_dollars, d_cents, t_dollars, t_cents);
							else swprintf_s(buf, L"You received %d.%02d$ - You have %d.%02d$", d_dollars, d_cents, t_dollars, t_cents);
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
						if (!uname.empty()) { size_t n = uname.size(); if (n > 95) n = 95; size_t cv = 0; mbstowcs_s(&cv, wname, 96, uname.c_str(), n); wname[n] = L'\0'; }
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
					// If aiming at a ped, check if crosshair aligns with the head bone
					if (ENTITY::IS_ENTITY_A_PED(target) && !ENTITY::IS_ENTITY_DEAD(target))
					{
						// Resolve head bone index (try common names)
						int headIdx = -1;
						if (headIdx < 0) headIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(target, const_cast<char*>("SKEL_Head"));
						if (headIdx < 0) headIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(target, const_cast<char*>("HEAD"));
						if (headIdx >= 0)
						{
							Vector3 headPos = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(target, headIdx);
							Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
							Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
							float radX = camRot.x * static_cast<float>(3.14159265358979323846 / 180.0);
							float radZ = camRot.z * static_cast<float>(3.14159265358979323846 / 180.0);
							Vector3 dir = { -sinf(radZ) * cosf(radX), cosf(radZ) * cosf(radX), sinf(radX) };
							// Normalize dir
							float dlen = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
							if (dlen > 0.0001f) { dir.x/=dlen; dir.y/=dlen; dir.z/=dlen; }
							// Vector from cam to head
							Vector3 w = { headPos.x - camPos.x, headPos.y - camPos.y, headPos.z - camPos.z };
							float proj = w.x*dir.x + w.y*dir.y + w.z*dir.z; // along-ray distance
							if (proj > 0.0f)
							{
								// Perpendicular distance to ray = |w x dir|
								Vector3 cross = { w.y*dir.z - w.z*dir.y, w.z*dir.x - w.x*dir.z, w.x*dir.y - w.y*dir.x };
								float dist = sqrtf(cross.x*cross.x + cross.y*cross.y + cross.z*cross.z);
								// Threshold ~22cm feels like head-width tolerance
								if (dist < 0.22f)
								{
									DWORD now = GetTickCount();
									if (target != g_lastHeadEntity || (now - g_lastHeadCueMs) > 220)
									{
										// Optional distinct cue; keep it short
										BeepAsync(1450, 28);
										A11y::speak(L"head", true);
										g_lastHeadEntity = target;
										g_lastHeadCueMs = now;
									}
								}
							}
						}
					}

					// GTA11Y-like short ping for target class (ped/vehicle/prop)
					if (g_enableAimingWavCues)
					{
						DWORD now = GetTickCount();
						if (now - g_lastAimSoundMs > 180 || target != g_lastAimSoundEntity)
						{
							wchar_t wavPath[MAX_PATH]; wavPath[0] = L'\0';
							// WAVs are next to NativeTrainer.asi in release/bin; try local relative names first
							if (ENTITY::IS_ENTITY_A_PED(target) && !ENTITY::IS_ENTITY_DEAD(target))
								wcsncpy_s(wavPath, L"tped.wav", _TRUNCATE);
							else if (ENTITY::IS_ENTITY_A_VEHICLE(target) && !ENTITY::IS_ENTITY_DEAD(target))
								wcsncpy_s(wavPath, L"tvehicle.wav", _TRUNCATE);
							else {
								// type 3 == object/prop; filter roughly by can-be-damaged
								int t = ENTITY::GET_ENTITY_TYPE(target);
								if (t == 3) wcsncpy_s(wavPath, L"tprop.wav", _TRUNCATE);
							}
							if (wavPath[0]) PlayWavAsync(wavPath);
							g_lastAimSoundMs = now; g_lastAimSoundEntity = target;
						}
					}

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
							if (!name.empty()) { size_t n = name.size(); if (n > 127) n = 127; size_t cv = 0; mbstowcs_s(&cv, nbuf, 128, name.c_str(), n); nbuf[n] = L'\0'; }
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
						if (!name.empty()) { size_t n = name.size(); if (n > 127) n = 127; size_t cv = 0; mbstowcs_s(&cv, wname, 128, name.c_str(), n); wname[n] = L'\0'; swprintf_s(wbuf, L"aiming at %s, distance %d meters, health %d (%d%%)", wname, distM, hp, hpPct); }
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

			// (camera pitch beep removed)
		}
		
		// Call bodyguard protection system if enabled
		if (g_protectionEnabled) {
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			// Find a bodyguard ped
			Ped bodyguard = 0;
			// Look for bodyguards in nearby area
			int peds[1024];
			int count = worldGetAllPeds(peds, 1024);
			for (int i = 0; i < count; i++) {
				if (ENTITY::DOES_ENTITY_EXIST(peds[i]) && peds[i] != playerPed) {
					Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true, false);
					Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(peds[i], true, false);
					float distance = SYSTEM::VDIST(playerPos.x, playerPos.y, playerPos.z, pedPos.x, pedPos.y, pedPos.z);
					if (distance < 50.0f && !PED::IS_PED_IN_COMBAT(peds[i], playerPed)) {
						bodyguard = peds[i];
						break;
					}
				}
			}
			if (bodyguard != 0) {
				BodyguardProtectionSystem(g_protectionEnabled, g_lastProtectionScanMs, bodyguard);
			}
		}
		
		// === PLAGUE SYSTEM (BlackDeath Mode only) ===
		
		// Black Death Mode - NEW LAYOUT: 1: Full Plague, 2: Disease Only, 3: Instant Kill, 4: System Toggle, 6: Targeted Plague, 7: Statistics, 8: Clear Area, 9: Spread Plague, 0: Stop All
		if (!menuController->HasActiveMenu() && g_a11yMode == A11yMode::BlackDeath) {
			// NumPad 1: Toggle Full Plague
			if (IsKeyJustUp(VK_NUMPAD1)) {
				TogglePlagueSystem(PlagueType::FullPlague);
			}
			
			// NumPad 2: Toggle Disease Only
			if (IsKeyJustUp(VK_NUMPAD2)) {
				TogglePlagueSystem(PlagueType::DiseaseOnly);
			}
			
			// NumPad 3: Toggle Instant Kill
			if (IsKeyJustUp(VK_NUMPAD3)) {
				TogglePlagueSystem(PlagueType::InstantKill);
			}
			
			// NumPad 4: Toggle entire plague system
			if (IsKeyJustUp(VK_NUMPAD4)) {
				g_plagueSystemEnabled = !g_plagueSystemEnabled;
				if (g_plagueSystemEnabled) {
					A11y::speak(L"Plague system activated", true);
				} else {
					A11y::speak(L"Plague system deactivated", true);
				}
			}
			
			// NumPad 6: Targeted plague (on aimed entity)
			if (IsKeyJustUp(VK_NUMPAD6)) {
				Entity target = 0; Player pl = PLAYER::PLAYER_ID();
				if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(pl, &target) && ENTITY::DOES_ENTITY_EXIST(target) && ENTITY::IS_ENTITY_A_PED(target)) {
					// Apply targeted plague effect
					Ped targetPed = (Ped)target;
					if (!ENTITY::IS_ENTITY_DEAD(targetPed)) {
						ENTITY::SET_ENTITY_HEALTH(targetPed, 0, FALSE);
						A11y::speak(L"Target infected", true);
					} else {
						A11y::speak(L"Target already dead", true);
					}
				} else {
					A11y::speak(L"No valid target", true);
				}
			}
			
			// NumPad 7: Plague statistics
			if (IsKeyJustUp(VK_NUMPAD7)) {
				wchar_t buf[120]; 
				swprintf_s(buf, L"Plague system %s, last scan %d seconds ago", 
					g_plagueSystemEnabled ? L"active" : L"inactive",
					(GetTickCount() - g_lastPlagueScanMs) / 1000);
				A11y::speak(buf, true);
			}
			
			// NumPad 8: Clear area of plague effects
			if (IsKeyJustUp(VK_NUMPAD8)) {
				// This would clear fire/smoke effects in the area
				A11y::speak(L"Area cleansed", true);
			}
			
			// NumPad 9: Force plague spread
			if (IsKeyJustUp(VK_NUMPAD9)) {
				if (g_plagueSystemEnabled) {
					g_lastPlagueScanMs = 0; // Force immediate scan
					A11y::speak(L"Plague spreading", true);
				} else {
					A11y::speak(L"Plague system inactive", true);
				}
			}
			
			// NumPad 0: Stop all plague types
			if (IsKeyJustUp(VK_NUMPAD0)) {
				g_plagueSystemEnabled = false;
				A11y::speak(L"All plague effects stopped", true);
			}
		}
		
		// Run plague system manager (handles all plague types centrally)
		PlagueSystemManager(g_lastPlagueScanMs);
		
		// Auto-announce location changes (with anti-spam protection)
		if (g_enableAutoZones) {
			DWORD now = GetTickCount();
			// Check for location changes every 1 second to avoid performance impact
			if (now - g_lastAutoLocationCheckMs > 1000) {
				g_lastAutoLocationCheckMs = now;
				
				wchar_t currentLocation[128] = L"";
				if (GetCurrentLocationName(currentLocation, 128)) {
					// Only announce if location has changed and enough time has passed since last announcement
					if (wcscmp(currentLocation, g_lastZone) != 0 && (now - g_lastZoneSpeakMs) > 3000) {
						wcscpy_s(g_lastZone, 128, currentLocation);
						g_lastZoneSpeakMs = now;
						A11y::speak(currentLocation, true);
					}
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