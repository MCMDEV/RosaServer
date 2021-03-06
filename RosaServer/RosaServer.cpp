﻿#include "RosaServer.h"
#include <sys/mman.h>
#include <cerrno>

#define handle_error(msg)               \
	do                                    \
	{                                     \
		std::cout << __LINE__ << std::endl; \
		perror(msg);                        \
		exit(EXIT_FAILURE);                 \
	} while (0)

bool initialized = false;
bool shouldReset = false;

sol::state* lua;
std::string hookMode;

std::queue<std::string> consoleQueue;
std::queue<LuaHTTPRequest> requestQueue;
std::queue<LuaHTTPResponse> responseQueue;

static Version* version;
static char* serverName;
static unsigned int* serverPort;

static int* isPassworded;
static char* password;

int* gameType;
char* mapName;
char* loadedMapName;
int* gameState;
int* gameTimer;
unsigned int* sunTime;
static int* isLevelLoaded;
static float* gravity;
static float originalGravity;

RayCastResult* lineIntersectResult;

Connection* connections;
Account* accounts;
Player* players;
Human* humans;
Vehicle* vehicles;
ItemType* itemTypes;
Item* items;
Bullet* bullets;
RigidBody* bodies;

unsigned int* numConnections;
unsigned int* numBullets;

static void pryMemory(void* address, size_t numPages)
{
	size_t pageSize = sysconf(_SC_PAGE_SIZE);

	uintptr_t page = (uintptr_t)address;
	page -= (page % pageSize);

	if (mprotect((void*)page, pageSize * numPages, PROT_WRITE | PROT_READ) == 0)
	{
		printf("[RS] Successfully pried open page at %p\n", (void*)page);
	}
	else
	{
		handle_error("mprotect");
	}
}

/*static subhook::Hook _test_hook;
typedef int(*_test_func)(void*, int);
static _test_func _test;

//3f40
int h__test(void* x, int y) {
	printf("test %p %i\n", x, y);
	printf("removing\n");
	subhook::ScopedHookRemove remove(&_test_hook);
	printf("calling\n");
	int ret = _test(x, y);
	printf("done %i\n", ret);
	return ret;
}*/

subhook::Hook resetgame_hook;
void_func resetgame;

subhook::Hook logicsimulation_hook;
void_func logicsimulation;
subhook::Hook logicsimulation_race_hook;
void_func logicsimulation_race;
subhook::Hook logicsimulation_round_hook;
void_func logicsimulation_round;
subhook::Hook logicsimulation_world_hook;
void_func logicsimulation_world;
subhook::Hook logicsimulation_terminator_hook;
void_func logicsimulation_terminator;
subhook::Hook logicsimulation_coop_hook;
void_func logicsimulation_coop;
subhook::Hook logicsimulation_versus_hook;
void_func logicsimulation_versus;

subhook::Hook physicssimulation_hook;
void_func physicssimulation;
subhook::Hook recvpacket_hook;
recvpacket_func recvpacket;
subhook::Hook sendpacket_hook;
void_func sendpacket;
subhook::Hook bulletsimulation_hook;
void_func bulletsimulation;
void_func bullettimetolive;

subhook::Hook createaccount_jointicket_hook;
createaccount_jointicket_func createaccount_jointicket;
// Alex Austin's typo
subhook::Hook server_sendconnectreponse_hook;
server_sendconnectreponse_func server_sendconnectreponse;

scenario_armhuman_func scenario_armhuman;
subhook::Hook linkitem_hook;
linkitem_func linkitem;
subhook::Hook human_applydamage_hook;
human_applydamage_func human_applydamage;
subhook::Hook human_collisionvehicle_hook;
human_collisionvehicle_func human_collisionvehicle;
subhook::Hook human_grabbing_hook;
void_index_func human_grabbing;
subhook::Hook grenadeexplosion_hook;
void_index_func grenadeexplosion;
subhook::Hook chat_hook;
chat_func chat;
subhook::Hook playerai_hook;
void_index_func playerai;
subhook::Hook playerdeathtax_hook;
void_index_func playerdeathtax;

subhook::Hook createplayer_hook;
createplayer_func createplayer;
subhook::Hook deleteplayer_hook;
void_index_func deleteplayer;
subhook::Hook createhuman_hook;
createhuman_func createhuman;
subhook::Hook deletehuman_hook;
void_index_func deletehuman;
subhook::Hook createitem_hook;
createitem_func createitem;
subhook::Hook deleteitem_hook;
void_index_func deleteitem;
createrope_func createrope;
subhook::Hook createobject_hook;
createobject_func createobject;
subhook::Hook deleteobject_hook;
void_index_func deleteobject;

subhook::Hook createevent_message_hook;
createevent_message_func createevent_message;
subhook::Hook createevent_updateplayer_hook;
void_index_func createevent_updateplayer;
subhook::Hook createevent_updateplayer_finance_hook;
void_index_func createevent_updateplayer_finance;
//subhook::Hook createevent_updateitem_hook;
//void_index_func createevent_updateitem;
void_index_func createevent_createobject;
subhook::Hook createevent_updateobject_hook;
createevent_updateobject_func createevent_updateobject;
//subhook::Hook createevent_sound_hook;
createevent_sound_func createevent_sound;
createevent_explosion_func createevent_explosion;
subhook::Hook createevent_bullethit_hook;
createevent_bullethit_func createevent_bullethit;

lineintersectlevel_func lineintersectlevel;
subhook::Hook lineintersecthuman_hook;
lineintersecthuman_func lineintersecthuman;
lineintersectobject_func lineintersectobject;

#define HOOK_FLAGS subhook::HookFlags::HookFlag64BitOffset

struct Server
{
	const int TPS = 60;

	const char* getClass() const
	{
		return "Server";
	}
	int getPort() const
	{
		return *serverPort;
	}
	char* getName() const
	{
		return serverName;
	}
	void setName(const char* newName) const
	{
		strncpy(serverName, newName, 31);
	}
	char* getPassword() const
	{
		return password;
	}
	void setPassword(const char* newPassword) const
	{
		strncpy(password, newPassword, 31);
		*isPassworded = newPassword[0] != 0;
	}
	int getType() const
	{
		return *gameType;
	}
	void setType(int type) const
	{
		*gameType = type;
	}
	char* getLevelName() const
	{
		return mapName;
	}
	void setLevelName(const char* newName) const
	{
		strncpy(mapName, newName, 31);
	}
	char* getLoadedLevelName() const
	{
		return loadedMapName;
	}
	bool getIsLevelLoaded() const
	{
		return *isLevelLoaded;
	}
	void setIsLevelLoaded(bool b) const
	{
		*isLevelLoaded = b;
	}
	float getGravity() const
	{
		return *gravity;
	}
	void setGravity(float g) const
	{
		*gravity = g;
	}
	float getDefaultGravity() const
	{
		return originalGravity;
	}
	int getState() const
	{
		return *gameState;
	}
	void setState(int state) const
	{
		*gameState = state;
	}
	int getTime() const
	{
		return *gameTimer;
	}
	void setTime(int time) const
	{
		*gameTimer = time;
	}
	int getSunTime() const
	{
		return *sunTime;
	}
	void setSunTime(int time) const
	{
		*sunTime = time % 5184000;
	}
	std::string getVersion() const
	{
		std::ostringstream stream;
		stream << version->major << (char)(version->build + 97);
		return stream.str();
	}

	void setConsoleTitle(const char* title) const
	{
		printf("\033]0;%s\007", title);
	}
	void reset() const
	{
		hookAndReset(RESET_REASON_LUACALL);
	}
};
static Server* server;

void luaInit(bool redo)
{
	printf("\033[36m");
	if (redo)
	{
		printf("\n[RS] Resetting state...\n");
		delete server;
		delete lua;
	}
	else
	{
		printf("\n[RS] Initializing state...\n");
	}

	lua = new sol::state();
	lua->open_libraries(sol::lib::base);
	lua->open_libraries(sol::lib::package);
	lua->open_libraries(sol::lib::coroutine);
	lua->open_libraries(sol::lib::string);
	lua->open_libraries(sol::lib::os);
	lua->open_libraries(sol::lib::math);
	lua->open_libraries(sol::lib::table);
	lua->open_libraries(sol::lib::debug);
	lua->open_libraries(sol::lib::bit32);
	lua->open_libraries(sol::lib::io);
	lua->open_libraries(sol::lib::ffi);
	lua->open_libraries(sol::lib::jit);

	{
		auto meta = lua->new_usertype<Server>("new", sol::no_constructor);
		meta["TPS"] = &Server::TPS;

		meta["class"] = sol::property(&Server::getClass);
		meta["port"] = sol::property(&Server::getPort);
		meta["name"] = sol::property(&Server::getName, &Server::setName);
		meta["password"] = sol::property(&Server::getPassword, &Server::setPassword);
		meta["type"] = sol::property(&Server::getType, &Server::setType);
		meta["levelToLoad"] = sol::property(&Server::getLevelName, &Server::setLevelName);
		meta["loadedLevel"] = sol::property(&Server::getLoadedLevelName);
		meta["isLevelLoaded"] = sol::property(&Server::getIsLevelLoaded, &Server::setIsLevelLoaded);
		meta["gravity"] = sol::property(&Server::getGravity, &Server::setGravity);
		meta["defaultGravity"] = sol::property(&Server::getDefaultGravity);
		meta["state"] = sol::property(&Server::getState, &Server::setState);
		meta["time"] = sol::property(&Server::getTime, &Server::setTime);
		meta["sunTime"] = sol::property(&Server::getSunTime, &Server::setSunTime);
		meta["version"] = sol::property(&Server::getVersion);

		meta["setConsoleTitle"] = &Server::setConsoleTitle;
		meta["reset"] = &Server::reset;
	}

	server = new Server();
	(*lua)["server"] = server;

	{
		auto meta = lua->new_usertype<Connection>("new", sol::no_constructor);
		meta["port"] = &Connection::port;
		meta["timeoutTime"] = &Connection::timeoutTime;

		meta["class"] = sol::property(&Connection::getClass);
		meta["address"] = sol::property(&Connection::getAddress);
		meta["adminVisible"] = sol::property(&Connection::getAdminVisible, &Connection::setAdminVisible);
	}

	{
		auto meta = lua->new_usertype<Account>("new", sol::no_constructor);
		meta["subRosaID"] = &Account::subRosaID;
		meta["phoneNumber"] = &Account::phoneNumber;
		meta["money"] = &Account::money;
		meta["corporateRating"] = &Account::corporateRating;
		meta["criminalRating"] = &Account::criminalRating;
		meta["banTime"] = &Account::banTime;

		meta["class"] = sol::property(&Account::getClass);
		meta["__tostring"] = &Account::__tostring;
		meta["index"] = sol::property(&Account::getIndex);
		meta["name"] = sol::property(&Account::getName);
		meta["steamID"] = sol::property(&Account::getSteamID);
	}

	{
		auto meta = lua->new_usertype<Vector>("new", sol::no_constructor);
		meta["x"] = &Vector::x;
		meta["y"] = &Vector::y;
		meta["z"] = &Vector::z;

		meta["class"] = sol::property(&Vector::getClass);
		meta["__tostring"] = &Vector::__tostring;
		meta["add"] = &Vector::add;
		meta["mult"] = &Vector::mult;
		meta["set"] = &Vector::set;
		meta["clone"] = &Vector::clone;
		meta["dist"] = &Vector::dist;
		meta["distSquare"] = &Vector::distSquare;
	}

	{
		auto meta = lua->new_usertype<RotMatrix>("new", sol::no_constructor);
		meta["x1"] = &RotMatrix::x1;
		meta["y1"] = &RotMatrix::y1;
		meta["z1"] = &RotMatrix::z1;
		meta["x2"] = &RotMatrix::x2;
		meta["y2"] = &RotMatrix::y2;
		meta["z2"] = &RotMatrix::z2;
		meta["x3"] = &RotMatrix::x3;
		meta["y3"] = &RotMatrix::y3;
		meta["z3"] = &RotMatrix::z3;

		meta["class"] = sol::property(&RotMatrix::getClass);
		meta["__tostring"] = &RotMatrix::__tostring;
		meta["set"] = &RotMatrix::set;
		meta["clone"] = &RotMatrix::clone;
	}

	{
		auto meta = lua->new_usertype<Player>("new", sol::no_constructor);
		meta["subRosaID"] = &Player::subRosaID;
		meta["phoneNumber"] = &Player::phoneNumber;
		meta["money"] = &Player::money;
		meta["corporateRating"] = &Player::corporateRating;
		meta["criminalRating"] = &Player::criminalRating;
		meta["team"] = &Player::team;
		meta["teamSwitchTimer"] = &Player::teamSwitchTimer;
		meta["stocks"] = &Player::stocks;
		meta["menuTab"] = &Player::menuTab;
		meta["gender"] = &Player::gender;
		meta["skinColor"] = &Player::skinColor;
		meta["hairColor"] = &Player::hairColor;
		meta["hair"] = &Player::hair;
		meta["eyeColor"] = &Player::eyeColor;
		meta["model"] = &Player::model;
		meta["suitColor"] = &Player::suitColor;
		meta["tieColor"] = &Player::tieColor;
		meta["head"] = &Player::head;
		meta["necklace"] = &Player::necklace;

		meta["class"] = sol::property(&Player::getClass);
		meta["__tostring"] = &Player::__tostring;
		meta["index"] = sol::property(&Player::getIndex);
		meta["isActive"] = sol::property(&Player::getIsActive, &Player::setIsActive);
		meta["name"] = sol::property(&Player::getName, &Player::setName);
		meta["isAdmin"] = sol::property(&Player::getIsAdmin, &Player::setIsAdmin);
		meta["isReady"] = sol::property(&Player::getIsReady, &Player::setIsReady);
		meta["isBot"] = sol::property(&Player::getIsBot, &Player::setIsBot);
		meta["human"] = sol::property(&Player::getHuman);
		meta["connection"] = sol::property(&Player::getConnection);
		meta["account"] = sol::property(&Player::getAccount, &Player::setAccount);
		meta["botDestination"] = sol::property(&Player::getBotDestination, &Player::setBotDestination);

		meta["update"] = &Player::update;
		meta["updateFinance"] = &Player::updateFinance;
		meta["remove"] = &Player::remove;
		meta["sendMessage"] = &Player::sendMessage;
	}

	{
		auto meta = lua->new_usertype<Human>("new", sol::no_constructor);
		meta["vehicleSeat"] = &Human::vehicleSeat;
		meta["despawnTime"] = &Human::despawnTime;
		meta["movementState"] = &Human::movementState;
		meta["zoomLevel"] = &Human::zoomLevel;
		meta["damage"] = &Human::damage;
		meta["viewYaw"] = &Human::viewYaw;
		meta["viewPitch"] = &Human::viewPitch;
		meta["strafeInput"] = &Human::strafeInput;
		meta["walkInput"] = &Human::walkInput;
		meta["inputFlags"] = &Human::inputFlags;
		meta["lastInputFlags"] = &Human::lastInputFlags;
		meta["health"] = &Human::health;
		meta["bloodLevel"] = &Human::bloodLevel;
		meta["chestHP"] = &Human::chestHP;
		meta["headHP"] = &Human::headHP;
		meta["leftArmHP"] = &Human::leftArmHP;
		meta["rightArmHP"] = &Human::rightArmHP;
		meta["leftLegHP"] = &Human::leftLegHP;
		meta["rightLegHP"] = &Human::rightLegHP;
		meta["gender"] = &Human::gender;
		meta["head"] = &Human::head;
		meta["skinColor"] = &Human::skinColor;
		meta["hairColor"] = &Human::hairColor;
		meta["hair"] = &Human::hair;
		meta["eyeColor"] = &Human::eyeColor;

		meta["class"] = sol::property(&Human::getClass);
		meta["__tostring"] = &Human::__tostring;
		meta["index"] = sol::property(&Human::getIndex);
		meta["isActive"] = sol::property(&Human::getIsActive, &Human::setIsActive);
		meta["isAlive"] = sol::property(&Human::getIsAlive, &Human::setIsAlive);
		meta["isImmortal"] = sol::property(&Human::getIsImmortal, &Human::setIsImmortal);
		meta["isOnGround"] = sol::property(&Human::getIsOnGround);
		meta["isStanding"] = sol::property(&Human::getIsStanding);
		meta["isBleeding"] = sol::property(&Human::getIsBleeding, &Human::setIsBleeding);
		meta["player"] = sol::property(&Human::getPlayer);
		meta["vehicle"] = sol::property(&Human::getVehicle, &Human::setVehicle);
		meta["rightHandItem"] = sol::property(&Human::getRightHandItem);
		meta["leftHandItem"] = sol::property(&Human::getLeftHandItem);
		meta["rightHandGrab"] = sol::property(&Human::getRightHandGrab, &Human::setRightHandGrab);
		meta["leftHandGrab"] = sol::property(&Human::getLeftHandGrab, &Human::setLeftHandGrab);

		meta["remove"] = &Human::remove;
		meta["getPos"] = &Human::getPos;
		meta["setPos"] = &Human::setPos;
		meta["speak"] = &Human::speak;
		meta["arm"] = &Human::arm;
		meta["getBone"] = &Human::getBone;
		meta["getRigidBody"] = &Human::getRigidBody;
		meta["setVelocity"] = &Human::setVelocity;
		meta["addVelocity"] = &Human::addVelocity;
		meta["mountItem"] = &Human::mountItem;
		meta["applyDamage"] = &Human::applyDamage;
	}

	{
		auto meta = lua->new_usertype<ItemType>("new", sol::no_constructor);
		meta["price"] = &ItemType::price;
		meta["mass"] = &ItemType::mass;
		meta["fireRate"] = &ItemType::fireRate;
		meta["bulletType"] = &ItemType::bulletType;
		meta["bulletVelocity"] = &ItemType::bulletVelocity;
		meta["bulletSpread"] = &ItemType::bulletSpread;
		meta["numHands"] = &ItemType::numHands;
		meta["rightHandPos"] = &ItemType::rightHandPos;
		meta["leftHandPos"] = &ItemType::leftHandPos;

		meta["class"] = sol::property(&ItemType::getClass);
		meta["__tostring"] = &ItemType::__tostring;
		meta["index"] = sol::property(&ItemType::getIndex);
		meta["name"] = sol::property(&ItemType::getName, &ItemType::setName);
		meta["isGun"] = sol::property(&ItemType::getIsGun, &ItemType::setIsGun);
	}

	{
		auto meta = lua->new_usertype<Item>("new", sol::no_constructor);
		meta["type"] = &Item::type;
		meta["despawnTime"] = &Item::despawnTime;
		meta["parentSlot"] = &Item::parentSlot;
		meta["pos"] = &Item::pos;
		meta["vel"] = &Item::vel;
		meta["rot"] = &Item::rot;
		meta["bullets"] = &Item::bullets;

		meta["class"] = sol::property(&Item::getClass);
		meta["__tostring"] = &Item::__tostring;
		meta["index"] = sol::property(&Item::getIndex);
		meta["isActive"] = sol::property(&Item::getIsActive, &Item::setIsActive);
		meta["hasPhysics"] = sol::property(&Item::getHasPhysics, &Item::setHasPhysics);
		meta["physicsSettled"] = sol::property(&Item::getPhysicsSettled, &Item::setPhysicsSettled);
		meta["rigidBody"] = sol::property(&Item::getRigidBody);
		meta["parentHuman"] = sol::property(&Item::getParentHuman);
		meta["parentItem"] = sol::property(&Item::getParentItem);

		meta["remove"] = &Item::remove;
		meta["mountItem"] = &Item::mountItem;
		meta["unmount"] = &Item::unmount;
		meta["speak"] = &Item::speak;
		meta["explode"] = &Item::explode;
	}

	{
		auto meta = lua->new_usertype<Vehicle>("new", sol::no_constructor);
		meta["type"] = &Vehicle::type;
		meta["controllableState"] = &Vehicle::controllableState;
		meta["health"] = &Vehicle::health;
		meta["color"] = &Vehicle::color;
		meta["pos"] = &Vehicle::pos;
		meta["pos2"] = &Vehicle::pos2;
		meta["rot"] = &Vehicle::rot;
		meta["vel"] = &Vehicle::vel;
		// Messy but faster than using a table or some shit
		meta["windowState0"] = &Vehicle::windowState0;
		meta["windowState1"] = &Vehicle::windowState1;
		meta["windowState2"] = &Vehicle::windowState2;
		meta["windowState3"] = &Vehicle::windowState3;
		meta["windowState4"] = &Vehicle::windowState4;
		meta["windowState5"] = &Vehicle::windowState5;
		meta["windowState6"] = &Vehicle::windowState6;
		meta["windowState7"] = &Vehicle::windowState7;
		meta["gearX"] = &Vehicle::gearX;
		meta["steerControl"] = &Vehicle::steerControl;
		meta["gearY"] = &Vehicle::gearY;
		meta["gasControl"] = &Vehicle::gasControl;
		meta["bladeBodyID"] = &Vehicle::bladeBodyID;

		meta["class"] = sol::property(&Vehicle::getClass);
		meta["__tostring"] = &Vehicle::__tostring;
		meta["index"] = sol::property(&Vehicle::getIndex);
		meta["isActive"] = sol::property(&Vehicle::getIsActive, &Vehicle::setIsActive);
		meta["lastDriver"] = sol::property(&Vehicle::getLastDriver);
		meta["rigidBody"] = sol::property(&Vehicle::getRigidBody);

		meta["updateType"] = &Vehicle::updateType;
		meta["updateDestruction"] = &Vehicle::updateDestruction;
		meta["remove"] = &Vehicle::remove;
	}

	{
		auto meta = lua->new_usertype<Bullet>("new", sol::no_constructor);
		meta["type"] = &Bullet::type;
		meta["time"] = &Bullet::time;
		meta["lastPos"] = &Bullet::lastPos;
		meta["pos"] = &Bullet::pos;
		meta["vel"] = &Bullet::vel;

		meta["class"] = sol::property(&Bullet::getClass);
		meta["player"] = sol::property(&Bullet::getPlayer);
	}

	{
		auto meta = lua->new_usertype<Bone>("new", sol::no_constructor);
		meta["pos"] = &Bone::pos;
		meta["pos2"] = &Bone::pos2;

		meta["class"] = sol::property(&Bone::getClass);
	}

	{
		auto meta = lua->new_usertype<RigidBody>("new", sol::no_constructor);
		meta["type"] = &RigidBody::type;
		meta["unk0"] = &RigidBody::unk0;
		meta["mass"] = &RigidBody::mass;
		meta["pos"] = &RigidBody::pos;
		meta["vel"] = &RigidBody::vel;
		meta["rot"] = &RigidBody::rot;
		meta["rot2"] = &RigidBody::rot2;

		meta["class"] = sol::property(&RigidBody::getClass);
		meta["__tostring"] = &RigidBody::__tostring;
		meta["index"] = sol::property(&RigidBody::getIndex);
		meta["isActive"] = sol::property(&RigidBody::getIsActive, &RigidBody::setIsActive);
		meta["isSettled"] = sol::property(&RigidBody::getIsSettled, &RigidBody::setIsSettled);
	}

	(*lua)["printAppend"] = l_printAppend;
	(*lua)["flagStateForReset"] = l_flagStateForReset;

	(*lua)["hook"] = lua->create_table();
	(*lua)["hook"]["persistentMode"] = hookMode;

	(*lua)["Vector"] = sol::overload(l_Vector, l_Vector_3f);
	(*lua)["RotMatrix"] = l_RotMatrix;

	(*lua)["http"] = lua->create_table();
	(*lua)["http"]["get"] = l_http_get;
	(*lua)["http"]["post"] = l_http_post;

	(*lua)["event"] = lua->create_table();
	(*lua)["event"]["sound"] = sol::overload(l_event_sound, l_event_soundSimple);
	(*lua)["event"]["explosion"] = l_event_explosion;
	(*lua)["event"]["bulletHit"] = l_event_bulletHit;

	(*lua)["physics"] = lua->create_table();
	(*lua)["physics"]["lineIntersectLevel"] = l_physics_lineIntersectLevel;
	(*lua)["physics"]["lineIntersectHuman"] = l_physics_lineIntersectHuman;
	(*lua)["physics"]["lineIntersectVehicle"] = l_physics_lineIntersectVehicle;
	(*lua)["physics"]["garbageCollectBullets"] = l_physics_garbageCollectBullets;

	(*lua)["chat"] = lua->create_table();
	(*lua)["chat"]["announce"] = l_chat_announce;
	(*lua)["chat"]["tellAdmins"] = l_chat_tellAdmins;
	(*lua)["chat"]["addRaw"] = l_chat_addRaw;

	(*lua)["accounts"] = lua->create_table();
	(*lua)["accounts"]["getCount"] = l_accounts_getCount;
	(*lua)["accounts"]["getAll"] = l_accounts_getAll;
	(*lua)["accounts"]["getByPhone"] = l_accounts_getByPhone;
	{
		sol::table _meta = lua->create_table();
		(*lua)["accounts"][sol::metatable_key] = _meta;
		_meta["__index"] = l_accounts_getByIndex;
	}

	(*lua)["players"] = lua->create_table();
	(*lua)["players"]["getCount"] = l_players_getCount;
	(*lua)["players"]["getAll"] = l_players_getAll;
	(*lua)["players"]["getByPhone"] = l_players_getByPhone;
	(*lua)["players"]["getNonBots"] = l_players_getNonBots;
	(*lua)["players"]["createBot"] = l_players_createBot;
	{
		sol::table _meta = lua->create_table();
		(*lua)["players"][sol::metatable_key] = _meta;
		_meta["__index"] = l_players_getByIndex;
	}

	(*lua)["humans"] = lua->create_table();
	(*lua)["humans"]["getCount"] = l_humans_getCount;
	(*lua)["humans"]["getAll"] = l_humans_getAll;
	(*lua)["humans"]["create"] = l_humans_create;
	{
		sol::table _meta = lua->create_table();
		(*lua)["humans"][sol::metatable_key] = _meta;
		_meta["__index"] = l_humans_getByIndex;
	}

	(*lua)["itemTypes"] = lua->create_table();
	(*lua)["itemTypes"]["getCount"] = l_itemTypes_getCount;
	(*lua)["itemTypes"]["getAll"] = l_itemTypes_getAll;
	{
		sol::table _meta = lua->create_table();
		(*lua)["itemTypes"][sol::metatable_key] = _meta;
		_meta["__index"] = l_itemTypes_getByIndex;
	}

	(*lua)["items"] = lua->create_table();
	(*lua)["items"]["getCount"] = l_items_getCount;
	(*lua)["items"]["getAll"] = l_items_getAll;
	(*lua)["items"]["create"] = sol::overload(l_items_create, l_items_createVel);
	(*lua)["items"]["createRope"] = l_items_createRope;
	{
		sol::table _meta = lua->create_table();
		(*lua)["items"][sol::metatable_key] = _meta;
		_meta["__index"] = l_items_getByIndex;
	}

	(*lua)["vehicles"] = lua->create_table();
	(*lua)["vehicles"]["getCount"] = l_vehicles_getCount;
	(*lua)["vehicles"]["getAll"] = l_vehicles_getAll;
	(*lua)["vehicles"]["create"] = sol::overload(l_vehicles_create, l_vehicles_createVel);
	//(*lua)["vehicles"]["createTraffic"] = l_vehicles_createTraffic;
	{
		sol::table _meta = lua->create_table();
		(*lua)["vehicles"][sol::metatable_key] = _meta;
		_meta["__index"] = l_vehicles_getByIndex;
	}

	(*lua)["bullets"] = lua->create_table();
	(*lua)["bullets"]["getCount"] = l_bullets_getCount;
	(*lua)["bullets"]["getAll"] = l_bullets_getAll;

	(*lua)["rigidBodies"] = lua->create_table();
	(*lua)["rigidBodies"]["getCount"] = l_rigidBodies_getCount;
	(*lua)["rigidBodies"]["getAll"] = l_rigidBodies_getAll;
	{
		sol::table _meta = lua->create_table();
		(*lua)["rigidBodies"][sol::metatable_key] = _meta;
		_meta["__index"] = l_rigidBodies_getByIndex;
	}

	//(*lua)["os"]["setClipboard"] = l_os_setClipboard;
	(*lua)["os"]["listDirectory"] = l_os_listDirectory;
	(*lua)["os"]["clock"] = l_os_clock;

	(*lua)["RESET_REASON_BOOT"] = RESET_REASON_BOOT;
	(*lua)["RESET_REASON_ENGINECALL"] = RESET_REASON_ENGINECALL;
	(*lua)["RESET_REASON_LUARESET"] = RESET_REASON_LUARESET;
	(*lua)["RESET_REASON_LUACALL"] = RESET_REASON_LUACALL;

	(*lua)["STATE_PREGAME"] = 1;
	(*lua)["STATE_GAME"] = 2;
	(*lua)["STATE_RESTARTING"] = 3;

	(*lua)["TYPE_DRIVING"] = 1;
	(*lua)["TYPE_RACE"] = 2;
	(*lua)["TYPE_ROUND"] = 3;
	(*lua)["TYPE_WORLD"] = 4;
	(*lua)["TYPE_TERMINATOR"] = 5;
	(*lua)["TYPE_COOP"] = 6;
	(*lua)["TYPE_VERSUS"] = 7;

	printf("[RS] Running init.lua...\033[0m\n");

	sol::load_result load = lua->load_file("main/init.lua");
	if (noLuaCallError(&load))
	{
		sol::protected_function_result res = load();
		if (noLuaCallError(&res))
		{
			printf("\033[32m[RS] Ready!\033[0m\n");
		}
	}
}

static void Attach()
{
	printf("[RS] Assuming 36b...\n");

	std::ifstream file("/proc/self/maps");
	std::string line;
	// First line
	std::getline(file, line);
	auto pos = line.find("-");
	auto truncated = line.substr(0, pos);

	printf("[RS] Base address is 0x%s...\n", truncated.c_str());

	auto base = std::stoul(truncated, nullptr, 16);

	// Locate everything

	version = (Version*)(base + 0x2BED08);
	serverName = (char*)(base + 0x1D158694);
	serverPort = (unsigned int*)(base + 0x1D158AA0);
	isPassworded = (int*)(base + 0x1D158AA4);
	password = (char*)(base + 0x14F24CEC);

	gameType = (int*)(base + 0x314CBDA8);
	mapName = (char*)(base + 0x314CBDAC);
	loadedMapName = (char*)(base + 0x2FD6E424);
	gameState = (int*)(base + 0x314CBFC4);
	gameTimer = (int*)(base + 0x314CBFCC);
	sunTime = (unsigned int*)(base + 0xC5D6AA0);
	isLevelLoaded = (int*)(base + 0x2FD6E420);
	gravity = (float*)(base + 0xB1F04);
	pryMemory(gravity, 1);
	originalGravity = *gravity;

	lineIntersectResult = (RayCastResult*)(base + 0x39B5B460);

	connections = (Connection*)(base + 0x36F640);
	accounts = (Account*)(base + 0x2A6F3F0);
	players = (Player*)(base + 0x11E82140);
	humans = (Human*)(base + 0x7226328);
	vehicles = (Vehicle*)(base + 0x190A6B80);
	itemTypes = (ItemType*)(base + 0x3DD351E0);
	items = (Item*)(base + 0x6CF6FE0);
	bullets = (Bullet*)(base + 0x307376A0);
	bodies = (RigidBody*)(base + 0x2C3620);

	numConnections = (unsigned int*)(base + 0x32255B68);
	numBullets = (unsigned int*)(base + 0x32255940);

	//_test = (_test_func)(base + 0x3f40);
	//pryMemory(&_test, 2);

	resetgame = (void_func)(base + 0x9D4C0);

	logicsimulation = (void_func)(base + 0xA39C0);
	logicsimulation_race = (void_func)(base + 0x9F750);
	logicsimulation_round = (void_func)(base + 0x9FEC0);
	logicsimulation_world = (void_func)(base + 0xA3270);
	logicsimulation_terminator = (void_func)(base + 0xA0E20);
	logicsimulation_coop = (void_func)(base + 0x9F510);
	logicsimulation_versus = (void_func)(base + 0xA26D0);

	physicssimulation = (void_func)(base + 0x932A0);
	recvpacket = (recvpacket_func)(base + 0xAC0C0);
	sendpacket = (void_func)(base + 0xA9360);
	bulletsimulation = (void_func)(base + 0x870A0);
	bullettimetolive = (void_func)(base + 0x15E90);

	createaccount_jointicket = (createaccount_jointicket_func)(base + 0x5b20);
	server_sendconnectreponse = (server_sendconnectreponse_func)(base + 0xa4bd0);

	scenario_armhuman = (scenario_armhuman_func)(base + 0x46030);
	linkitem = (linkitem_func)(base + 0x23520);
	human_applydamage = (human_applydamage_func)(base + 0x1B010);
	human_collisionvehicle = (human_collisionvehicle_func)(base + 0x699E0);
	human_grabbing = (void_index_func)(base + 0x6BB30);
	grenadeexplosion = (void_index_func)(base + 0x22F20);
	chat = (chat_func)(base + 0x94160);
	playerai = (void_index_func)(base + 0x85610);
	playerdeathtax = (void_index_func)(base + 0x29D0);

	createplayer = (createplayer_func)(base + 0x3A5E0);
	deleteplayer = (void_index_func)(base + 0x3A890);
	createhuman = (createhuman_func)(base + 0x58DE0);
	deletehuman = (void_index_func)(base + 0x3750);
	createitem = (createitem_func)(base + 0x457F0);
	deleteitem = (void_index_func)(base + 0x23820);
	createrope = (createrope_func)(base + 0x45CD0);
	createobject = (createobject_func)(base + 0x48E00);
	deleteobject = (void_index_func)(base + 0x39B0);

	createevent_message = (createevent_message_func)(base + 0x2550);
	createevent_updateplayer = (void_index_func)(base + 0x2850);
	createevent_updateplayer_finance = (void_index_func)(base + 0x2960);
	//pryMemory(&createevent_updateplayer_finance, 2);
	//createevent_updateitem = (void_index_func)(base + 0x27B0);
	createevent_createobject = (void_index_func)(base + 0x2670);
	createevent_updateobject = (createevent_updateobject_func)(base + 0x26D0);
	createevent_sound = (createevent_sound_func)(base + 0x2AA0);
	createevent_explosion = (createevent_explosion_func)(base + 0x3110);
	createevent_bullethit = (createevent_bullethit_func)(base + 0x24A0);

	lineintersecthuman = (lineintersecthuman_func)(base + 0x20B70);
	lineintersectlevel = (lineintersectlevel_func)(base + 0x6D680);
	lineintersectobject = (lineintersectobject_func)(base + 0x83C20);

	// Hooks

	//_test_hook.Install((void*)_test, (void*)h__test, HOOK_FLAGS);
	resetgame_hook.Install((void*)resetgame, (void*)h_resetgame, HOOK_FLAGS);

	logicsimulation_hook.Install((void*)logicsimulation, (void*)h_logicsimulation, HOOK_FLAGS);
	logicsimulation_race_hook.Install((void*)logicsimulation_race, (void*)h_logicsimulation_race, HOOK_FLAGS);
	logicsimulation_round_hook.Install((void*)logicsimulation_round, (void*)h_logicsimulation_round, HOOK_FLAGS);
	logicsimulation_world_hook.Install((void*)logicsimulation_world, (void*)h_logicsimulation_world, HOOK_FLAGS);
	logicsimulation_terminator_hook.Install((void*)logicsimulation_terminator, (void*)h_logicsimulation_terminator, HOOK_FLAGS);
	logicsimulation_coop_hook.Install((void*)logicsimulation_coop, (void*)h_logicsimulation_coop, HOOK_FLAGS);
	logicsimulation_versus_hook.Install((void*)logicsimulation_versus, (void*)h_logicsimulation_versus, HOOK_FLAGS);

	physicssimulation_hook.Install((void*)physicssimulation, (void*)h_physicssimulation, HOOK_FLAGS);
	recvpacket_hook.Install((void*)recvpacket, (void*)h_recvpacket, HOOK_FLAGS);
	sendpacket_hook.Install((void*)sendpacket, (void*)h_sendpacket, HOOK_FLAGS);
	bulletsimulation_hook.Install((void*)bulletsimulation, (void*)h_bulletsimulation, HOOK_FLAGS);

	createaccount_jointicket_hook.Install((void*)createaccount_jointicket, (void*)h_createaccount_jointicket, HOOK_FLAGS);
	server_sendconnectreponse_hook.Install((void*)server_sendconnectreponse, (void*)h_server_sendconnectreponse, HOOK_FLAGS);

	linkitem_hook.Install((void*)linkitem, (void*)h_linkitem, HOOK_FLAGS);
	human_applydamage_hook.Install((void*)human_applydamage, (void*)h_human_applydamage, HOOK_FLAGS);
	human_collisionvehicle_hook.Install((void*)human_collisionvehicle, (void*)h_human_collisionvehicle, HOOK_FLAGS);
	human_grabbing_hook.Install((void*)human_grabbing, (void*)h_human_grabbing, HOOK_FLAGS);
	grenadeexplosion_hook.Install((void*)grenadeexplosion, (void*)h_grenadeexplosion, HOOK_FLAGS);
	chat_hook.Install((void*)chat, (void*)h_chat, HOOK_FLAGS);
	playerai_hook.Install((void*)playerai, (void*)h_playerai, HOOK_FLAGS);
	playerdeathtax_hook.Install((void*)playerdeathtax, (void*)h_playerdeathtax, HOOK_FLAGS);

	createplayer_hook.Install((void*)createplayer, (void*)h_createplayer, HOOK_FLAGS);
	deleteplayer_hook.Install((void*)deleteplayer, (void*)h_deleteplayer, HOOK_FLAGS);
	createhuman_hook.Install((void*)createhuman, (void*)h_createhuman, HOOK_FLAGS);
	deletehuman_hook.Install((void*)deletehuman, (void*)h_deletehuman, HOOK_FLAGS);
	createitem_hook.Install((void*)createitem, (void*)h_createitem, HOOK_FLAGS);
	deleteitem_hook.Install((void*)deleteitem, (void*)h_deleteitem, HOOK_FLAGS);
	createobject_hook.Install((void*)createobject, (void*)h_createobject, HOOK_FLAGS);
	deleteobject_hook.Install((void*)deleteobject, (void*)h_deleteobject, HOOK_FLAGS);

	createevent_message_hook.Install((void*)createevent_message, (void*)h_createevent_message, HOOK_FLAGS);
	createevent_updateplayer_hook.Install((void*)createevent_updateplayer, (void*)h_createevent_updateplayer, HOOK_FLAGS);
	//createevent_updateplayer_finance_hook.Install((void*)createevent_updateplayer_finance, (void*)h_createevent_updateplayer_finance, HOOK_FLAGS);
	//createevent_updateitem_hook.Install((void*)createevent_updateitem, (void*)h_createevent_updateitem, HOOK_FLAGS);
	createevent_updateobject_hook.Install((void*)createevent_updateobject, (void*)h_createevent_updateobject, HOOK_FLAGS);
	//createevent_sound_hook.Install((void*)createevent_sound, (void*)h_createevent_sound, HOOK_FLAGS);
	createevent_bullethit_hook.Install((void*)createevent_bullethit, (void*)h_createevent_bullethit, HOOK_FLAGS);

	lineintersecthuman_hook.Install((void*)lineintersecthuman, (void*)h_lineintersecthuman, HOOK_FLAGS);
}

int __attribute__((constructor)) Entry()
{
	std::thread mainThread(Attach);
	mainThread.detach();
	return 0;
}

int __attribute__((destructor)) Destroy()
{
	if (lua != nullptr)
	{
		delete lua;
		lua = nullptr;
	}
}