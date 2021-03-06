#include "api.h"
#include <chrono>
#include <experimental/filesystem>
#include "tinycon.h"

void printLuaError(sol::error* err)
{
	printf("\033[1;31mLua error:\n%s\033[0m\n\n", err->what());
}

bool noLuaCallError(sol::protected_function_result* res)
{
	if (res->valid()) return true;
	sol::error err = *res;
	printLuaError(&err);
	return false;
}

bool noLuaCallError(sol::load_result* res)
{
	if (res->valid()) return true;
	sol::error err = *res;
	printLuaError(&err);
	return false;
}

void hookAndReset(int reason)
{
	bool noParent = false;
	sol::protected_function func = (*lua)["hook"]["run"];
	if (func != sol::nil)
	{
		auto res = func("ResetGame", reason);
		if (noLuaCallError(&res))
			noParent = (bool)res;
	}
	if (!noParent)
	{
		{
			subhook::ScopedHookRemove remove(&resetgame_hook);
			resetgame();
		}
		if (func != sol::nil)
		{
			auto res = func("PostResetGame", reason);
			noLuaCallError(&res);
		}
	}
}

class tcon : public tinyConsole
{
 public:
	tcon() : tinyConsole() { ; }

	int trigger(std::string s)
	{
		consoleQueue.push(s);
		return 0;
	}
};

void consoleThread()
{
	tcon console;
	console.run();
}

void l_printAppend(const char* str)
{
	printf("%s", str);
}

void l_flagStateForReset(const char* mode)
{
	hookMode = mode;
	shouldReset = true;
}

Vector l_Vector()
{
	return Vector{0.f, 0.f, 0.f};
}

Vector l_Vector_3f(float x, float y, float z)
{
	return Vector{x, y, z};
}

RotMatrix l_RotMatrix(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
{
	return RotMatrix{x1, y1, z1, x2, y2, z2, x3, y3, z3};
}

void l_http_get(const char* host, int port, const char* path, sol::table headers, const char* identifier)
{
	LuaHTTPRequest request{
			LuaRequestType::get,
			host,
			(unsigned short)port,
			path,
			identifier};

	for (const auto& pair : headers)
		request.headers.emplace(pair.first.as<std::string>(), pair.second.as<std::string>());

	requestQueue.push(request);
}

void l_http_post(const char* host, int port, const char* path, sol::table headers, const char* body, const char* contentType, const char* identifier)
{
	LuaHTTPRequest request{
			LuaRequestType::post,
			host,
			(unsigned short)port,
			path,
			identifier,
			contentType,
			body};

	for (const auto& pair : headers)
		request.headers.emplace(pair.first.as<std::string>(), pair.second.as<std::string>());

	requestQueue.push(request);
}

void HTTPThread()
{
	while (true)
	{
		while (!requestQueue.empty())
		{
			auto req = requestQueue.front();
			//printf("new http request\n");

			httplib::Client client(req.host.c_str(), req.port);
			client.set_timeout_sec(6);
			client.set_keep_alive_max_count(0);

			std::shared_ptr<httplib::Response> res;

			req.headers.emplace("Connection", "close");

			switch (req.type)
			{
				case get:
					res = client.Get(req.path.c_str(), req.headers);
					break;
				case post:
					res = client.Post(req.path.c_str(), req.headers, req.body.c_str(), req.contentType.c_str());
					break;
			}

			//printf("yup\n");

			if (res)
			{
				LuaHTTPResponse response{
						req.identifier,
						true,
						res->status,
						res->body,
						res->headers};
				responseQueue.push(response);
			}
			else
			{
				LuaHTTPResponse response{
						req.identifier,
						false};
				responseQueue.push(response);
			}

			requestQueue.pop();

			//printf("popped\n");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

void l_event_sound(int soundType, Vector* pos, float volume, float pitch)
{
	//subhook::ScopedHookRemove remove(&createevent_sound_hook);
	createevent_sound(soundType, pos, volume, pitch);
}

void l_event_soundSimple(int soundType, Vector* pos)
{
	//subhook::ScopedHookRemove remove(&createevent_sound_hook);
	createevent_sound(soundType, pos, 1.0f, 1.0f);
}

void l_event_explosion(Vector* pos)
{
	createevent_explosion(0, pos);
}

void l_event_bulletHit(int hitType, Vector* pos, Vector* normal)
{
	subhook::ScopedHookRemove remove(&createevent_bullethit_hook);
	createevent_bullethit(0, hitType, pos, normal);
}

sol::table l_physics_lineIntersectLevel(Vector* posA, Vector* posB)
{
	sol::table table = lua->create_table();
	int res = lineintersectlevel(posA, posB);
	if (res)
	{
		table["pos"] = lineIntersectResult->pos;
		table["normal"] = lineIntersectResult->normal;
		table["fraction"] = lineIntersectResult->fraction;
	}
	table["hit"] = res != 0;
	return table;
}

sol::table l_physics_lineIntersectHuman(Human* man, Vector* posA, Vector* posB)
{
	sol::table table = lua->create_table();
	subhook::ScopedHookRemove remove(&lineintersecthuman_hook);
	int res = lineintersecthuman(man->getIndex(), posA, posB);
	if (res)
	{
		table["pos"] = lineIntersectResult->pos;
		table["normal"] = lineIntersectResult->normal;
		table["fraction"] = lineIntersectResult->fraction;
		table["bone"] = lineIntersectResult->humanBone;
	}
	table["hit"] = res != 0;
	return table;
}

sol::table l_physics_lineIntersectVehicle(Vehicle* vcl, Vector* posA, Vector* posB)
{
	sol::table table = lua->create_table();
	int res = lineintersectobject(vcl->getIndex(), posA, posB);
	if (res)
	{
		table["pos"] = lineIntersectResult->pos;
		table["normal"] = lineIntersectResult->normal;
		table["fraction"] = lineIntersectResult->fraction;
		table["bone"] = lineIntersectResult->humanBone;
	}
	table["hit"] = res != 0;
	return table;
}

void l_physics_garbageCollectBullets()
{
	bullettimetolive();
}

int l_itemTypes_getCount()
{
	return MAXNUMOFITEMTYPES;
}

sol::table l_itemTypes_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFITEMTYPES; i++)
	{
		arr.add(&itemTypes[i]);
	}
	return arr;
}

ItemType* l_itemTypes_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFITEMTYPES)
		throw std::runtime_error("Index out of range");
	return &itemTypes[idx];
}

int l_items_getCount()
{
	int count = 0;
	for (int i = 0; i < MAXNUMOFITEMS; i++)
	{
		if ((&items[i])->active) count++;
	}
	return count;
}

sol::table l_items_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFITEMS; i++)
	{
		auto item = &items[i];
		if (!item->active) continue;
		arr.add(item);
	}
	return arr;
}

Item* l_items_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFITEMS)
		throw std::runtime_error("Index out of range");
	return &items[idx];
}

Item* l_items_create(int itemType, Vector* pos, RotMatrix* rot)
{
	subhook::ScopedHookRemove remove(&createitem_hook);
	int id = createitem(itemType, pos, nullptr, rot);
	return id == -1 ? nullptr : &items[id];
}

Item* l_items_createVel(int itemType, Vector* pos, Vector* vel, RotMatrix* rot)
{
	subhook::ScopedHookRemove remove(&createitem_hook);
	int id = createitem(itemType, pos, vel, rot);
	return id == -1 ? nullptr : &items[id];
}

Item* l_items_createRope(Vector* pos, RotMatrix* rot)
{
	int id = createrope(pos, rot);
	return id == -1 ? nullptr : &items[id];
}

int l_vehicles_getCount()
{
	int count = 0;
	for (int i = 0; i < MAXNUMOFVEHICLES; i++)
	{
		if ((&vehicles[i])->active) count++;
	}
	return count;
}

sol::table l_vehicles_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFVEHICLES; i++)
	{
		auto vcl = &vehicles[i];
		if (!vcl->active) continue;
		arr.add(vcl);
	}
	return arr;
}

Vehicle* l_vehicles_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFVEHICLES)
		throw std::runtime_error("Index out of range");
	return &vehicles[idx];
}

Vehicle* l_vehicles_create(int type, Vector* pos, RotMatrix* rot, int color)
{
	subhook::ScopedHookRemove remove(&createobject_hook);
	int id = createobject(type, pos, nullptr, rot, color);
	return id == -1 ? nullptr : &vehicles[id];
}

Vehicle* l_vehicles_createVel(int type, Vector* pos, Vector* vel, RotMatrix* rot, int color)
{
	subhook::ScopedHookRemove remove(&createobject_hook);
	int id = createobject(type, pos, vel, rot, color);
	return id == -1 ? nullptr : &vehicles[id];
}

/*void l_vehicles_createTraffic(int density) {
	scenario_createtraffic3(density);
}*/

void l_chat_announce(const char* message)
{
	subhook::ScopedHookRemove remove(&createevent_message_hook);
	createevent_message(0, (char*)message, -1, 0);
}

void l_chat_tellAdmins(const char* message)
{
	subhook::ScopedHookRemove remove(&createevent_message_hook);
	createevent_message(4, (char*)message, -1, 0);
}

void l_chat_addRaw(int type, const char* message, int speakerID, int distance)
{
	subhook::ScopedHookRemove remove(&createevent_message_hook);
	createevent_message(type, (char*)message, speakerID, distance);
}

int l_accounts_getCount()
{
	int count = 0;
	while (true)
	{
		Account* acc = &accounts[count];
		if (!acc->subRosaID) break;
		count++;
	}
	return count;
}

sol::table l_accounts_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0;; i++)
	{
		Account* acc = &accounts[i];
		if (!acc->subRosaID) break;
		arr.add(acc);
	}
	return arr;
}

Account* l_accounts_getByPhone(int phone)
{
	for (int i = 0;; i++)
	{
		Account* acc = &accounts[i];
		if (!acc->subRosaID) break;
		if (acc->phoneNumber == phone)
			return acc;
	}
	return nullptr;
}

Account* l_accounts_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFACCOUNTS)
		throw std::runtime_error("Index out of range");
	return &accounts[idx];
}

int l_players_getCount()
{
	int count = 0;
	for (int i = 0; i < MAXNUMOFPLAYERS; i++)
	{
		if ((&players[i])->active) count++;
	}
	return count;
}

sol::table l_players_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFPLAYERS; i++)
	{
		auto ply = &players[i];
		if (!ply->active) continue;
		arr.add(ply);
	}
	return arr;
}

Player* l_players_getByPhone(int phone)
{
	for (int i = 0; i < MAXNUMOFPLAYERS; i++)
	{
		auto ply = &players[i];
		if (!ply->active) continue;
		if (ply->phoneNumber == phone)
			return ply;
	}
	return nullptr;
}

sol::table l_players_getNonBots()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFPLAYERS; i++)
	{
		auto ply = &players[i];
		if (!ply->active || !ply->subRosaID || ply->isBot) continue;
		arr.add(ply);
	}
	return arr;
}

Player* l_players_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFPLAYERS)
		throw std::runtime_error("Index out of range");
	return &players[idx];
}

Player* l_players_createBot()
{
	subhook::ScopedHookRemove remove(&createplayer_hook);
	int playerID = createplayer();
	if (playerID == -1) return nullptr;
	auto ply = &players[playerID];
	ply->subRosaID = 0;
	ply->isBot = 1;
	ply->team = 6;
	ply->setName("Bot");
	return ply;
}

int l_humans_getCount()
{
	int count = 0;
	for (int i = 0; i < MAXNUMOFHUMANS; i++)
	{
		if ((&humans[i])->active) count++;
	}
	return count;
}

sol::table l_humans_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFHUMANS; i++)
	{
		auto man = &humans[i];
		if (!man->active) continue;
		arr.add(man);
	}
	return arr;
}

Human* l_humans_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFHUMANS)
		throw std::runtime_error("Index out of range");
	return &humans[idx];
}

Human* l_humans_create(Vector* pos, RotMatrix* rot, Player* ply)
{
	int playerID = ply->getIndex();
	if (ply->humanID != -1)
	{
		subhook::ScopedHookRemove remove(&deletehuman_hook);
		deletehuman(ply->humanID);
	}
	int humanID;
	{
		subhook::ScopedHookRemove remove(&createhuman_hook);
		humanID = createhuman(pos, rot, playerID);
	}
	if (humanID == -1)
		return nullptr;
	auto man = &humans[humanID];
	man->playerID = playerID;
	ply->humanID = humanID;
	return man;
}

unsigned int l_bullets_getCount()
{
	return *numBullets;
}

sol::table l_bullets_getAll()
{
	auto arr = lua->create_table();
	for (unsigned int i = 0; i < *numBullets; i++)
	{
		Bullet* bul = &bullets[i];
		arr.add(bul);
	}
	return arr;
}

int l_rigidBodies_getCount()
{
	int count = 0;
	for (int i = 0; i < MAXNUMOFRIGIDBODIES; i++)
	{
		if ((&bodies[i])->active) count++;
	}
	return count;
}

sol::table l_rigidBodies_getAll()
{
	auto arr = lua->create_table();
	for (int i = 0; i < MAXNUMOFRIGIDBODIES; i++)
	{
		auto body = &bodies[i];
		if (!body->active) continue;
		arr.add(body);
	}
	return arr;
}

RigidBody* l_rigidBodies_getByIndex(sol::table self, unsigned int idx)
{
	if (idx >= MAXNUMOFRIGIDBODIES)
		throw std::runtime_error("Index out of range");
	return &bodies[idx];
}

sol::table l_os_listDirectory(const char* path)
{
	auto arr = lua->create_table();
	for (const auto& entry : std::experimental::filesystem::directory_iterator(path))
	{
		auto table = lua->create_table();
		auto path = entry.path();
		table["isDirectory"] = std::experimental::filesystem::is_directory(path);
		table["name"] = path.filename().string();
		table["stem"] = path.stem().string();
		table["extension"] = path.extension().string();
		arr.add(table);
	}
	return arr;
}

double l_os_clock()
{
	auto now = std::chrono::steady_clock::now();
	auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto epoch = ms.time_since_epoch();
	auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
	return value.count() / 1000.;
}

std::string addressFromInteger(unsigned int address)
{
	unsigned char* bytes = (unsigned char*)(&address);

	char buf[16];
	sprintf(buf, "%i.%i.%i.%i",
					(int)bytes[3],
					(int)bytes[2],
					(int)bytes[1],
					(int)bytes[0]);

	return buf;
}

std::string Connection::getAddress()
{
	return addressFromInteger(address);
}

std::string Account::__tostring() const
{
	char buf[32];
	sprintf(buf, "Account(%i)", getIndex());
	return buf;
}

int Account::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)accounts) / sizeof(*this);
}

std::string Vector::__tostring() const
{
	char buf[64];
	sprintf(buf, "Vector(%f, %f, %f)", x, y, z);
	return buf;
}

void Vector::add(Vector* other)
{
	x += other->x;
	y += other->y;
	z += other->z;
}

void Vector::mult(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
}

void Vector::set(Vector* other)
{
	x = other->x;
	y = other->y;
	z = other->z;
}

Vector Vector::clone() const
{
	return Vector{x, y, z};
}

float Vector::dist(Vector* other) const
{
	float dx = x - other->x;
	float dy = y - other->y;
	float dz = z - other->z;
	return sqrt(dx * dx + dy * dy + dz * dz);
}

float Vector::distSquare(Vector* other) const
{
	float dx = x - other->x;
	float dy = y - other->y;
	float dz = z - other->z;
	return dx * dx + dy * dy + dz * dz;
}

std::string RotMatrix::__tostring() const
{
	char buf[256];
	sprintf(buf, "RotMatrix(%f, %f, %f, %f, %f, %f, %f, %f, %f)", x1, y1, z1, x2, y2, z2, x3, y3, z3);
	return buf;
}

void RotMatrix::set(RotMatrix* other)
{
	x1 = other->x1;
	y1 = other->y1;
	z1 = other->z1;

	x2 = other->x2;
	y2 = other->y2;
	z2 = other->z2;

	x3 = other->x3;
	y3 = other->y3;
	z3 = other->z3;
}

RotMatrix RotMatrix::clone() const
{
	return RotMatrix{x1, y1, z1, x2, y2, z2, x3, y3, z3};
}

std::string Player::__tostring() const
{
	char buf[16];
	sprintf(buf, "Player(%i)", getIndex());
	return buf;
}

int Player::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)players) / sizeof(*this);
}

void Player::update() const
{
	subhook::ScopedHookRemove remove(&createevent_updateplayer_hook);
	createevent_updateplayer(getIndex());
}

void Player::updateFinance() const
{
	subhook::ScopedHookRemove remove(&createevent_updateplayer_finance_hook);
	createevent_updateplayer_finance(getIndex());
}

void Player::remove() const
{
	subhook::ScopedHookRemove remove(&deleteplayer_hook);
	deleteplayer(getIndex());
}

void Player::sendMessage(const char* message) const
{
	subhook::ScopedHookRemove remove(&createevent_message_hook);
	createevent_message(6, (char*)message, getIndex(), 0);
}

Human* Player::getHuman()
{
	if (humanID == -1)
		return nullptr;
	return &humans[humanID];
}

Connection* Player::getConnection()
{
	int id = getIndex();
	for (unsigned int i = 0; i < *numConnections; i++)
	{
		auto con = &connections[i];
		if (con->playerID == id)
			return con;
	}
	return nullptr;
}

Account* Player::getAccount()
{
	return &accounts[accountID];
}

void Player::setAccount(Account* account)
{
	if (account == nullptr)
		throw std::runtime_error("Cannot set account to nil value");
	else
		accountID = account->getIndex();
}

const Vector* Player::getBotDestination() const
{
	if (!botHasDestination)
		return nullptr;
	return &botDestination;
}

void Player::setBotDestination(Vector* vec)
{
	if (vec == nullptr)
		botHasDestination = false;
	else
	{
		botHasDestination = true;
		botDestination = *vec;
	}
}

std::string Human::__tostring() const
{
	char buf[16];
	sprintf(buf, "Human(%i)", getIndex());
	return buf;
}

int Human::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)humans) / sizeof(*this);
}

void Human::remove() const
{
	subhook::ScopedHookRemove remove(&deletehuman_hook);
	deletehuman(getIndex());
}

Player* Human::getPlayer() const
{
	if (playerID == -1)
		return nullptr;
	return &players[playerID];
}

Vehicle* Human::getVehicle() const
{
	if (vehicleID == -1)
		return nullptr;
	return &vehicles[vehicleID];
}

void Human::setVehicle(Vehicle* vcl)
{
	if (vcl == nullptr)
		vehicleID = -1;
	else
		vehicleID = vcl->getIndex();
}

Vector Human::getPos() const
{
	return pos;
};

void Human::setPos(Vector* vec)
{
	float offX = vec->x - pos.x;
	float offY = vec->y - pos.y;
	float offZ = vec->z - pos.z;

	Bone* bone;
	RigidBody* body;
	for (int i = 0; i < 16; i++)
	{
		bone = &bones[i];
		bone->pos.x += offX;
		bone->pos.y += offY;
		bone->pos.z += offZ;
		bone->pos2.x += offX;
		bone->pos2.y += offY;
		bone->pos2.z += offZ;

		body = &bodies[bone->bodyID];
		body->pos.x += offX;
		body->pos.y += offY;
		body->pos.z += offZ;
	}
};

void Human::speak(const char* message, int distance) const
{
	subhook::ScopedHookRemove remove(&createevent_message_hook);
	createevent_message(1, (char*)message, getIndex(), distance);
}

void Human::arm(int weapon, int magCount) const
{
	scenario_armhuman(getIndex(), weapon, magCount);
}

Bone* Human::getBone(unsigned int idx)
{
	if (idx > 15)
		throw std::runtime_error("Index out of range");

	return &bones[idx];
}

RigidBody* Human::getRigidBody(unsigned int idx) const
{
	if (idx > 15)
		throw std::runtime_error("Index out of range");

	return &bodies[bones[idx].bodyID];
}

Item* Human::getRightHandItem() const
{
	if (!rightHandOccupied)
		return nullptr;
	return &items[rightHandItemID];
}

Item* Human::getLeftHandItem() const
{
	if (!leftHandOccupied)
		return nullptr;
	return &items[leftHandItemID];
}

Human* Human::getRightHandGrab() const
{
	if (!isGrabbingRight)
		return nullptr;
	return &humans[grabbingRightHumanID];
}

void Human::setRightHandGrab(Human* man)
{
	if (man == nullptr)
		isGrabbingRight = 0;
	else
	{
		isGrabbingRight = 1;
		grabbingRightHumanID = man->getIndex();
		grabbingRightBone = 0;
	}
}

Human* Human::getLeftHandGrab() const
{
	if (!isGrabbingLeft)
		return nullptr;
	return &humans[grabbingLeftHumanID];
}

void Human::setLeftHandGrab(Human* man)
{
	if (man == nullptr)
		isGrabbingLeft = 0;
	else
	{
		isGrabbingLeft = 1;
		grabbingLeftHumanID = man->getIndex();
		grabbingLeftBone = 0;
	}
}

void Human::setVelocity(Vector* vel) const
{
	for (int i = 0; i < 16; i++)
	{
		auto body = getRigidBody(i);
		body->vel.set(vel);
	}
}

void Human::addVelocity(Vector* vel) const
{
	for (int i = 0; i < 16; i++)
	{
		auto body = getRigidBody(i);
		body->vel.add(vel);
	}
}

bool Human::mountItem(Item* childItem, unsigned int slot) const
{
	subhook::ScopedHookRemove remove(&linkitem_hook);
	return linkitem(childItem->getIndex(), -1, getIndex(), slot);
}

void Human::applyDamage(int bone, int damage) const
{
	subhook::ScopedHookRemove remove(&human_applydamage_hook);
	human_applydamage(getIndex(), bone, 0, damage);
}

std::string ItemType::__tostring() const
{
	char buf[16];
	sprintf(buf, "ItemType(%i)", getIndex());
	return buf;
}

int ItemType::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)itemTypes) / sizeof(*this);
}

std::string Item::__tostring() const
{
	char buf[16];
	sprintf(buf, "Item(%i)", getIndex());
	return buf;
}

int Item::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)items) / sizeof(*this);
}

void Item::remove() const
{
	subhook::ScopedHookRemove remove(&deleteitem_hook);
	deleteitem(getIndex());
}

Human* Item::getParentHuman() const
{
	return parentHumanID == -1 ? nullptr : &humans[parentHumanID];
}

Item* Item::getParentItem() const
{
	return parentItemID == -1 ? nullptr : &items[parentItemID];
}

RigidBody* Item::getRigidBody() const
{
	return &bodies[bodyID];
}

bool Item::mountItem(Item* childItem, unsigned int slot) const
{
	subhook::ScopedHookRemove remove(&linkitem_hook);
	return linkitem(getIndex(), childItem->getIndex(), -1, slot);
}

bool Item::unmount() const
{
	subhook::ScopedHookRemove remove(&linkitem_hook);
	return linkitem(getIndex(), -1, -1, 0);
}

void Item::speak(const char* message, int distance) const
{
	subhook::ScopedHookRemove remove(&createevent_message_hook);
	createevent_message(2, (char*)message, getIndex(), distance);
}

void Item::explode() const
{
	subhook::ScopedHookRemove remove(&grenadeexplosion_hook);
	grenadeexplosion(getIndex());
}

std::string Vehicle::__tostring() const
{
	char buf[16];
	sprintf(buf, "Vehicle(%i)", getIndex());
	return buf;
}

int Vehicle::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)vehicles) / sizeof(*this);
}

void Vehicle::updateType() const
{
	createevent_createobject(getIndex());
}

void Vehicle::updateDestruction(int updateType, int partID, Vector* pos, Vector* normal) const
{
	subhook::ScopedHookRemove remove(&createevent_updateobject_hook);
	createevent_updateobject(getIndex(), updateType, partID, pos, normal);
}

void Vehicle::remove() const
{
	deleteobject(getIndex());
}

Player* Vehicle::getLastDriver() const
{
	if (lastDriverPlayerID == -1)
		return nullptr;
	return &players[lastDriverPlayerID];
}

RigidBody* Vehicle::getRigidBody() const
{
	return &bodies[bodyID];
}

Player* Bullet::getPlayer() const
{
	if (playerID == -1)
		return nullptr;
	return &players[playerID];
}

std::string RigidBody::__tostring() const
{
	char buf[16];
	sprintf(buf, "RigidBody(%i)", getIndex());
	return buf;
}

int RigidBody::getIndex() const
{
	return ((uintptr_t)this - (uintptr_t)bodies) / sizeof(*this);
}