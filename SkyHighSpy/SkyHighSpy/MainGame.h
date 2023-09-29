enum MainGameState
{
	STATE_GAME_PLAY,
	STATE_GAME_PAUSE,
};

enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_ASTEROID,
	TYPE_ATTACHED,
	TYPE_PIECE,
	TYPE_METEOR,
	TYPE_GEM,
	TYPE_DESTROYED,
};

enum Agent8State
{
	STATE_ATTACHED = 0,
	STATE_FLY,
	STATE_DEAD,
};

struct Agent8
{
	const float FLY_SPEED{ 10.0f };
	const float CRAWL_SPEED{ 0.04f };
	const float TURNING_SPEED{ 0.04f };
	const int JUMP_MAGNITUDE{ 70 };
	const Vector2D AABB{ 38.f, 38.f };
	bool active{ false };
	bool attached{ false };
	Agent8State state = STATE_FLY;
	const Point2D ATTACHED_OFFSET{ 64, 95 };
};

struct AsteroidInfo			// rename asteroid, meteor structs (not instances! but object information)
{
	int nASTEROID;
	const int PIECES_SPEED{ 5 };
	const int MIN_SPEED{ 2 };
	const int MAX_SPEED{ 5 };
	const Vector2D AABB{ 55.f, 55.f };
	bool active{ false };
	const Point2D ATTACHED_OFFSET{ 0, -15 };
};

struct MeteorInfo
{
	int nMETEOR;
	const int MIN_SPEED{ 5 };
	const int MAX_SPEED{ 8 };
	const Vector2D AABB{ 40.f, 40.f };
	const Point2D ATTACHED_OFFSET{ 0, -55 };
};

struct GemInfo
{
	const int score{ 1000 };
	float splitTime{ 0 };
	const float wobbleSpeed{ 0.25f };
	float wobbleDirection{ 1.0f };
	const float wobblePeriod{ 0.5f };
	const Vector2D AABB{ 10.f, 10.f };
};

struct BlueRing
{
	float currentLifetime{ 0 };
	float lifetime{ 2.0f };
	float decayConstant{ 2.0f };
	int nRings{ 3 };
	float baseScale{ 1.0f };
	Point2D pos;
};


struct BlueRingEmitter
{
	std::vector<BlueRing> vBlueRing;
};

struct Particle
{
	float currentLifetime{ 0 };
	float lifetime{ 1.f };
	float baseOpacity{ 1.0f };
	float opacityThreshold{ 0.005f };
	float decayConstant{ 1.0f };
	int posRANDOMNESS{ 20 };
	Point2D pos;
};


struct ParticleEmitter
{
	std::vector<Particle> vParticle;
	float splitTime{ 0.0f };
	float emitPeriod{ 0.1f };
	const int emitParticles{ 4 };
	const int maxParticles{ 50 };
};

struct GameState
{
	int level{ 0 };
	int score{ 0 };
	int hiScore{ 0 };
	int gemsRemaining;
	float timeTaken{ 0.f };
	MainGameState mainGameState = STATE_GAME_PLAY;
	Agent8 agent8;
	GemInfo gems;
	MeteorInfo meteors;
	AsteroidInfo asteroids;
	BlueRingEmitter blueRingEmitter;
	ParticleEmitter particleEmitter;
};


void EnterPausedState();
void UpdateAndCreateLevel();
void RestartGame();


void UpdateSpriteOrigins();
void UpdateGamePlay(float& elapsedTime);
void UpdateAgent8();
void UpdateAgent8Attached();
void UpdateAgent8Fly();
void UpdateAgent8Dead();
void UpdateMeteor();
void UpdateAsteroid();
void UpdateAttached();
void UpdatePieces();
void UpdateGem(float& elapsedTime);
void UpdateScore(float& elapsedTime);

void DrawGamePlay(float& elapsedTime);
void DrawGameObjects(int type);
void DrawParticle(float& elapsedTime);
void AddParticleToEmitter(GameObject& agentObj);
void UpdateParticleLifeTime(float& elapsedTime);
void DrawBlueRing(float& elapsedTime);
void DrawGUI();

void CreateAgent8();
void CreateMeteor(int& numberOfMeteor);
void CreateAsteroid(int& numberOfAsteroid);
void CreatePieces(Point2D& attachedPos);
void CreateGem(Point2D& attachedPos);
void DestroyAttached();
void DestroyGem(GameObject& gemObj);

void WrapObject(Point2D& objPos);
void WrapObjectBetweenScreen(GameObject& obj);
bool AABBCollisionTest(const Point2D& aPos, const Vector2D& aAABB, const Point2D& bPos, const Vector2D& bAABB);
float exponentialDecay(float A0, float lambda, float time);