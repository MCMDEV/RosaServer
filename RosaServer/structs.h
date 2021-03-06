#pragma once

#include <cstring>
#include <string>

#define MAXNUMOFACCOUNTS 32768
#define MAXNUMOFPLAYERS 256
#define MAXNUMOFHUMANS 256
#define MAXNUMOFITEMTYPES 38
#define MAXNUMOFITEMS 1024
#define MAXNUMOFVEHICLES 512
#define MAXNUMOFRIGIDBODIES 2048

/*
	Event types:
	0x1		1	bullethit
	0x2		2	message
	0x3		3	createobject (vehicle)
	0x4		4	updateobject
	0x5		5	updateitem
	0x7		7	updateplayer
	0x8		8	updateplayer_finance
	0x9		9	sound
	0xA		10	updatedoor
	0x11	17	updatehuman
	0x14	20	explosion
*/

//159120 bytes (26D90)
struct Connection
{
	unsigned int address;
	unsigned int port;	//04
	int unk0;						//08
	int adminVisible;		//0c
	int playerID;				//10
	int unk1;						//14
	int bandwidth;			//18
	int timeoutTime;		//1c
	char unk2[159120 - 32];

	const char* getClass() const
	{
		return "Connection";
	}
	std::string getAddress();
	bool getAdminVisible() const
	{
		return adminVisible;
	}
	void setAdminVisible(bool b)
	{
		adminVisible = b;
	}
};

//112 bytes (70)
struct Account
{
	int subRosaID;
	int phoneNumber;			//04
	long long steamID;		//08
	char name[32];				//10
	int unk0;							//30
	int money;						//34
	int corporateRating;	//38
	int criminalRating;		//3c
	char unk1[0x5c - 0x3c - 4];
	int banTime;	//5c
	char unk2[112 - 96];

	const char* getClass() const
	{
		return "Account";
	}
	std::string __tostring() const;
	int getIndex() const;
	char* getName()
	{
		return name;
	}
	std::string getSteamID()
	{
		return std::to_string(steamID);
	}
};

struct Vector
{
	float x, y, z;

	const char* getClass() const
	{
		return "Vector";
	}
	std::string __tostring() const;
	void add(Vector* other);
	void mult(float scalar);
	void set(Vector* other);
	Vector clone() const;
	float dist(Vector* other) const;
	float distSquare(Vector* other) const;
};

struct RotMatrix
{
	float x1, y1, z1;
	float x2, y2, z2;
	float x3, y3, z3;

	const char* getClass() const
	{
		return "RotMatrix";
	}
	std::string __tostring() const;
	void set(RotMatrix* other);
	RotMatrix clone() const;
};

struct RayCastResult
{
	Vector pos;
	Vector normal;	 //0c
	float fraction;	 //18
	float unk0;			 //1c
	int unk1;				 //20
	int unk2;				 //24
	int unk3;				 //28
	int unk4;				 //2c
	int unk5;				 //30
	int humanBone;	 //34
	int unk6;				 //38
	int unk7;				 //3c
	int unk8;				 //40
	int unk9;				 //44
	int unk10;			 //48
	int unk11;			 //4c
	int unk12;			 //50
	int unk13;			 //54
	int blockX;			 //58
	int blockY;			 //5c
	int blockZ;			 //60
	int unk17;			 //64
	int unk18;			 //68
	int matMaybe;		 //6c
	int unk20;			 //70
	int unk21;			 //74
	int unk22;			 //78
	int unk23;			 //7c
	int unk24;			 //80
};

// Forward decl
struct Human;
struct Vehicle;
struct Item;
struct RigidBody;

//14372 bytes (0x3824)
struct Player
{
	int active;
	char name[32];							 //04
	int unk0;										 //24
	int unk1;										 //28
	unsigned int subRosaID;			 //2c
	unsigned int phoneNumber;		 //30
	int isAdmin;								 //34
	unsigned int adminAttempts;	 //38
	unsigned int accountID;			 //3C
	char unk2[0x48 - 0x3C - 4];
	int isReady;					//48
	int money;						//4C
	int corporateRating;	//50
	int criminalRating;		//54
	char unk3[0x7C - 0x54 - 4];
	unsigned int team;						 //7C
	unsigned int teamSwitchTimer;	 //80
	int stocks;										 //84
	int unk4[2];
	int humanID;	//90
	char unk5[0x158 - 0x90 - 4];
	//0 = none, 1-19 = shop, 2X = base
	int menuTab;	//158
	char unk6[0x2d0c - 0x158 - 4];
	int isBot;	//2d0c
	char unk7a[0x2d28 - 0x2d0c - 4];
	int botHasDestination;	//2d28
	Vector botDestination;	//2d2c
	char unk7[0x354c - 0x2d2c - 12];
	int botState;	 //354c
	int unk8;
	int botEnemyID;	 //3554
	char unk9[0x379c - 0x3554 - 4];
	int gender;			//379c
	int skinColor;	//37a0
	int hairColor;	//37a4
	int hair;				//37a8
	int eyeColor;		//37ac
	//0 = casual, 1 = suit
	int model;			//37b0
	int suitColor;	//37b4
	//0 = no tie
	int tieColor;	 //37b8
	int unk10;		 //37bc
	int head;			 //37c0
	int necklace;	 //37c4
	char unk11[14372 - 14280];

	const char* getClass() const
	{
		return "Player";
	}
	std::string __tostring() const;
	int getIndex() const;
	bool getIsActive() const
	{
		return active;
	}
	void setIsActive(bool b)
	{
		active = b;
	}
	char* getName()
	{
		return name;
	}
	void setName(const char* newName)
	{
		std::strncpy(name, newName, 31);
	}
	bool getIsAdmin() const
	{
		return isAdmin;
	}
	void setIsAdmin(bool b)
	{
		isAdmin = b;
	}
	bool getIsReady() const
	{
		return isReady;
	}
	void setIsReady(bool b)
	{
		isReady = b;
	}
	bool getIsBot() const
	{
		return isBot;
	}
	void setIsBot(bool b)
	{
		isBot = b;
	}
	Human* getHuman();
	Connection* getConnection();
	Account* getAccount();
	void setAccount(Account* account);
	const Vector* getBotDestination() const;
	void setBotDestination(Vector* vec);

	void update() const;
	void updateFinance() const;
	void remove() const;
	void sendMessage(const char* message) const;
};

//312 bytes (138)
struct Bone
{
	int bodyID;
	Vector pos;
	Vector pos2;
	Vector vel;
	Vector vel2;
	RotMatrix rot;
	char unk[312 - 88];

	const char* getClass() const
	{
		return "Bone";
	}
};

//14152 bytes (3748)
struct Human
{
	int active;
	unsigned int unk00;				//04
	int playerID;							//08
	int unk0;									//0c
	int unk1;									//10
	int unk2;									//14
	int unk3;									//18
	int unk4;									//1c
	int vehicleID;						//20
	int vehicleSeat;					//24
	int lastVehicleID;				//28
	int lastVehicleCooldown;	//2c
	//counts down after death
	unsigned int despawnTime;	 //30
	int oldHealth;						 //34
	//eliminator
	int isImmortal;								 //38
	int unk10;										 //3c
	int unk11;										 //40
	int unk12;										 //44
	unsigned int spawnProtection;	 //48
	int isOnGround;								 //4c
	/*
	0=normal
	1=jumping/falling
	2=sliding
	5=getting up?
	*/
	int movementState;	//50
	int unk13;					//54
	int zoomLevel;			//58
	int unk14;					//5c
	int unk15;					//60
	int unk16;					//64
	int unk17;					//68
	int unk18;					//6c
	//max 60
	int damage;				//70
	int isStanding;		//74
	Vector pos;				//78
	Vector pos2;			//84
	float viewYaw;		//90
	float viewPitch;	//94
	char unk19[0x124 - 0x94 - 4];
	float strafeInput;	//124
	float unk20;
	float walkInput;	//12c
	char unk21[0x1f4 - 0x12c - 4];
	/*
	mouse1 = 1		1 << 0
	mouse2 = 2		1 << 1
	space = 4		1 << 2
	ctrl = 8		1 << 3
	shift = 16		1 << 4

	Q = 32			1 << 5
	e = 2048		1 << 11
	r = 4096		1 << 12
	f = 8192		1 << 13

	del = 262144	1 << 18
	z = 524288		1 << 19
	*/
	unsigned int inputFlags;			//1f4
	unsigned int lastInputFlags;	//1f8
	char unk22[0x200 - 0x1f8 - 4];
	Bone bones[16];	 //200
	char unk23[0x323c - (0x200 + (312 * 16))];
	int rightHandOccupied;	//323c
	int rightHandItemID;		//3240
	char unk24[0x3264 - 0x3240 - 4];
	int leftHandOccupied;	 //3264
	int leftHandItemID;		 //3268
	char unk25[0x338c - 0x3268 - 4];
	int isGrabbingRight;			 //338c
	int grabbingRightHumanID;	 //3390
	int unk26_1;
	int grabbingRightBone;	//3398
	char unk26_2[0x33b8 - 0x3398 - 4];
	int isGrabbingLeft;				//33b8
	int grabbingLeftHumanID;	//33bc
	int unk26_3;
	int grabbingLeftBone;	 //33c4
	char unk26_4[0x34a4 - 0x33c4 - 4];
	int health;			 //34a4
	int bloodLevel;	 //34a8
	int isBleeding;	 //34ac
	int chestHP;		 //34b0
	int unk26;			 //34b4
	int headHP;			 //34b8
	int unk27;			 //34bc
	int leftArmHP;	 //34c0
	int unk28;			 //34c4
	int rightArmHP;	 //34c8
	int unk29;			 //34cc
	int leftLegHP;	 //34d0
	int unk30;			 //34d4
	int rightLegHP;	 //34d8
	char unk31[0x36d4 - 0x34d8 - 4];
	int gender;			//36d4
	int head;				//36d8
	int skinColor;	//36dc
	int hairColor;	//36e0
	int hair;				//36e4
	int eyeColor;		//36e8
	int model;			//36ec
	int suitColor;	//36f0
	int tieColor;		//36f4
	char unk32[14152 - 0x36f4 - 4];

	const char* getClass() const
	{
		return "Human";
	}
	std::string __tostring() const;
	int getIndex() const;
	bool getIsActive() const
	{
		return active;
	}
	void setIsActive(bool b)
	{
		active = b;
	}
	bool getIsAlive() const
	{
		return oldHealth > 0;
	}
	void setIsAlive(bool b)
	{
		oldHealth = b ? 100 : 0;
	}
	bool getIsImmortal() const
	{
		return isImmortal;
	}
	void setIsImmortal(bool b)
	{
		isImmortal = b;
	}
	bool getIsOnGround() const
	{
		return isOnGround;
	}
	bool getIsStanding() const
	{
		return isStanding;
	}
	bool getIsBleeding() const
	{
		return isBleeding;
	}
	void setIsBleeding(bool b)
	{
		isBleeding = b;
	}
	Player* getPlayer() const;
	Vehicle* getVehicle() const;
	void setVehicle(Vehicle* vcl);
	Bone* getBone(unsigned int idx);
	RigidBody* getRigidBody(unsigned int idx) const;
	Item* getRightHandItem() const;
	Item* getLeftHandItem() const;
	Human* getRightHandGrab() const;
	void setRightHandGrab(Human* man);
	Human* getLeftHandGrab() const;
	void setLeftHandGrab(Human* man);

	void remove() const;
	Vector getPos() const;
	void setPos(Vector* vec);
	void speak(const char* message, int distance) const;
	void arm(int weapon, int magCount) const;
	void setVelocity(Vector* vel) const;
	void addVelocity(Vector* vel) const;
	bool mountItem(Item* childItem, unsigned int slot) const;
	void applyDamage(int bone, int damage) const;
};

//3944 bytes (F68)
struct ItemType
{
	int price;
	float mass;					 //04
	int isGun;					 //08
	int fuckedUpAiming;	 //0c
	//in ticks per bullet
	int fireRate;	 //10
	//?
	int bulletType;				 //14
	int unk0;							 //18
	int unk1;							 //1c
	float bulletVelocity;	 //20
	float bulletSpread;		 //24
	char name[64];				 //28
	char unk2[0x74 - 0x28 - 64];
	int numHands;					//74
	Vector rightHandPos;	//78
	Vector leftHandPos;		//84
	char unk3[3944 - 0x84 - 12];

	const char* getClass() const
	{
		return "ItemType";
	}
	std::string __tostring() const;
	int getIndex() const;
	char* getName()
	{
		return name;
	}
	void setName(const char* newName)
	{
		std::strncpy(name, newName, 63);
	}
	bool getIsGun() const
	{
		return isGun;
	}
	void setIsGun(bool b)
	{
		isGun = b;
	}
};

//592 bytes (250)
struct Item
{
	int active;
	int physicsSim;			 //04
	int physicsSettled;	 //08
	int unk1;						 //0c
	int type;						 //10
	int unk2;						 //14
	int despawnTime;		 //18
	int unk4;						 //1c
	int parentHumanID;	 //20
	int parentItemID;		 //24
	int parentSlot;			 //28
	char unk5[0x54 - 0x28 - 4];
	int bodyID;			//54
	Vector pos;			//58
	Vector pos2;		//64
	Vector vel;			//70
	Vector vel2;		//7c
	Vector vel3;		//88
	Vector vel4;		//94
	RotMatrix rot;	//a0
	char unk6[0x124 - 0xa0 - 36];
	int bullets;	//124
	char unk7[592 - 296];

	const char* getClass() const
	{
		return "Item";
	}
	std::string __tostring() const;
	int getIndex() const;
	bool getIsActive() const
	{
		return active;
	}
	void setIsActive(bool b)
	{
		active = b;
	}
	bool getHasPhysics() const
	{
		return physicsSim;
	}
	void setHasPhysics(bool b)
	{
		physicsSim = b;
	}
	bool getPhysicsSettled() const
	{
		return physicsSettled;
	}
	void setPhysicsSettled(bool b)
	{
		physicsSettled = b;
	}

	void remove() const;
	Human* getParentHuman() const;
	Item* getParentItem() const;
	RigidBody* getRigidBody() const;
	bool mountItem(Item* childItem, unsigned int slot) const;
	bool unmount() const;
	void speak(const char* message, int distance) const;
	void explode() const;
};

//20548 bytes (5044)
struct Vehicle
{
	int active;
	unsigned int type;			//04
	int controllableState;	//08
	//default 100
	int health;							 //0c
	int unk1;								 //10
	int lastDriverPlayerID;	 //14
	unsigned int color;			 //18
	//-1 = won't despawn
	short despawnTime;	 //1c
	short spawnedState;	 //1e
	int isLocked;				 //20
	int unk3;						 //24
	int bodyID;					 //28
	Vector pos;					 //2c
	Vector pos2;				 //38
	RotMatrix rot;			 //44
	Vector vel;					 //68
	char unk5[0x27c0 - 0x68 - 12];
	int windowState0;	 //27c0
	int windowState1;	 //27c4
	int windowState2;	 //27c8
	int windowState3;	 //27cc
	int windowState4;	 //27d0
	int windowState5;	 //27d4
	int windowState6;	 //27d8
	int windowState7;	 //27dc
	char unk6[0x35c4 - 0x27dc - 4];
	float gearX;				 //35c4
	float steerControl;	 //35c8
	float gearY;				 //35cc
	float gasControl;		 //35d0
	char unk7[0x4e88 - 0x35d0 - 4];
	int bladeBodyID;	//4e88
	char unk8[20548 - 20108];

	const char* getClass() const
	{
		return "Vehicle";
	}
	std::string __tostring() const;
	int getIndex() const;
	bool getIsActive() const
	{
		return active;
	}
	void setIsActive(bool b)
	{
		active = b;
	}
	Player* getLastDriver() const;
	RigidBody* getRigidBody() const;

	void updateType() const;
	void updateDestruction(int updateType, int partID, Vector* pos, Vector* normal) const;
	void remove() const;
};

//92 bytes (5C)
struct Bullet
{
	unsigned int type;
	int time;				 //04
	int playerID;		 //08
	float unk0;			 //0c
	float unk1;			 //10
	Vector lastPos;	 //14
	Vector pos;			 //20
	Vector vel;			 //2c
	char unk2[92 - 56];

	const char* getClass() const
	{
		return "Bullet";
	}
	Player* getPlayer() const;
};

//172 bytes (AC)
struct RigidBody
{
	int active;
	/*
	0 = human bone
	1 = car body
	2 = wheel
	3 = item
	*/
	int type;					//04
	int settled;			//08
	int unk0;					//0c
	float mass;				//10
	Vector pos;				//14
	Vector vel;				//20
	Vector startVel;	//? 2C
	RotMatrix rot;		//38
	RotMatrix rot2;		//5c
	char unk3[172 - 128];

	const char* getClass() const
	{
		return "RigidBody";
	}
	std::string __tostring() const;
	int getIndex() const;
	bool getIsActive() const
	{
		return active;
	}
	void setIsActive(bool b)
	{
		active = b;
	}
	bool getIsSettled() const
	{
		return settled;
	}
	void setIsSettled(bool b)
	{
		settled = b;
	}
};