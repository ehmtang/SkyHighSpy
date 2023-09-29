#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER

#include "Play.h"
#include "MainGame.h"

constexpr int DISPLAY_WIDTH{ 1280 };
constexpr int DISPLAY_HEIGHT{ 720 };
constexpr int DISPLAY_SCALE{ 1 };
constexpr int DISPLAY_WINDOW_INC{ 100 };

GameState gameState;

void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	UpdateSpriteOrigins();
	//Play::StartAudioLoop("music");

	CreateAgent8();
}

bool MainGameUpdate(float elapsedTime)
{
	Play::ClearDrawingBuffer(Play::cWhite);
	Play::DrawBackground();

	switch (gameState.mainGameState)
	{
	case STATE_GAME_PLAY:
	{
		EnterPausedState();
		UpdateAndCreateLevel();
		UpdateGamePlay(elapsedTime);
		RestartGame();
		break;
	}
	case STATE_GAME_PAUSE:
	{
		EnterPausedState();
		break;
	}
	}

	DrawGamePlay(elapsedTime);
	return Play::KeyDown(VK_ESCAPE);
}

int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void EnterPausedState()
{
	if (Play::KeyPressed(VK_RETURN))
	{
		if (gameState.mainGameState == STATE_GAME_PLAY)
			gameState.mainGameState = STATE_GAME_PAUSE;
		else if (gameState.mainGameState == STATE_GAME_PAUSE)
			gameState.mainGameState = STATE_GAME_PLAY;
	}
}

void UpdateAndCreateLevel()
{
	if (gameState.gemsRemaining == 0)
	{
		gameState.level += 1;

		if (gameState.level == 1)
		{
			gameState.asteroids.nASTEROID = 3;
			gameState.meteors.nMETEOR = 1;
		}
		else
		{
			gameState.asteroids.nASTEROID += 2;
			gameState.meteors.nMETEOR += 1;
		}
		CreateMeteor(gameState.meteors.nMETEOR);
		CreateAsteroid(gameState.asteroids.nASTEROID);
	}
}

void RestartGame()
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };

	if (gameState.agent8.state == STATE_DEAD)
	{
		if (Play::KeyPressed(VK_SPACE))
		{
			gameState.level = 0;
			gameState.score = 0;
			Play::DestroyGameObjectsByType(TYPE_METEOR);
			Play::DestroyGameObjectsByType(TYPE_ASTEROID);
			Play::DestroyGameObjectsByType(TYPE_ATTACHED);
			Play::DestroyGameObjectsByType(TYPE_GEM);
			UpdateAndCreateLevel();
			agentObj.pos = Point2D(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
			agentObj.rotation = 0;
			gameState.agent8.state = STATE_FLY;
			gameState.agent8.active = true;
			UpdateAgent8Fly();
		}
	}
}

void UpdateSpriteOrigins()
{
	Play::SetSpriteOrigin(Play::GetSpriteId("agent8_left"), gameState.agent8.ATTACHED_OFFSET.x, gameState.agent8.ATTACHED_OFFSET.y);
	Play::SetSpriteOrigin(Play::GetSpriteId("agent8_right"), gameState.agent8.ATTACHED_OFFSET.x, gameState.agent8.ATTACHED_OFFSET.y);
	Play::MoveSpriteOrigin("asteroid", gameState.asteroids.ATTACHED_OFFSET.x, gameState.asteroids.ATTACHED_OFFSET.y);
	Play::MoveSpriteOrigin("meteor", gameState.meteors.ATTACHED_OFFSET.x, gameState.meteors.ATTACHED_OFFSET.y);
}

void UpdateGamePlay(float& elapsedTime)
{
	UpdateAttached();
	UpdatePieces();
	UpdateAsteroid();
	UpdateMeteor();
	UpdateGem(elapsedTime);
	UpdateAgent8();
	UpdateScore(elapsedTime);
}

void UpdateAgent8()
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };

	std::vector<int> meteorIds{ Play::CollectGameObjectIDsByType(TYPE_METEOR) };
	std::vector<int> asteroidIds{ Play::CollectGameObjectIDsByType(TYPE_ASTEROID) };
	std::vector<int> gemIds{ Play::CollectGameObjectIDsByType(TYPE_GEM) };

	if (gameState.agent8.active)
	{
		for (int meteorId : meteorIds)
		{
			GameObject& meteorObj{ Play::GetGameObject(meteorId) };
			if (AABBCollisionTest(agentObj.pos, gameState.agent8.AABB, meteorObj.pos, gameState.meteors.AABB))
			{
				Play::PlayAudio("combust");
				gameState.agent8.state = STATE_DEAD;
			}
		}

		for (int asteroidId : asteroidIds)
		{
			GameObject& asteroidObj{ Play::GetGameObject(asteroidId) };
			if (AABBCollisionTest(agentObj.pos, gameState.agent8.AABB, asteroidObj.pos, gameState.asteroids.AABB)
				&& !gameState.agent8.attached)
			{
				Play::PlayAudio("clang");
				gameState.agent8.attached = true;
				asteroidObj.type = TYPE_ATTACHED;
				gameState.agent8.state = STATE_ATTACHED;
			}
		}

		for (int gemId : gemIds)
		{
			GameObject& gemObj{ Play::GetGameObject(gemId) };
			if (AABBCollisionTest(agentObj.pos, gameState.agent8.AABB, gemObj.pos, gameState.gems.AABB))
			{
				Play::PlayAudio("reward");
				gameState.score += gameState.gems.score;
				DestroyGem(gemObj);
			}
		}
	}

	switch (gameState.agent8.state)
	{
	case STATE_ATTACHED:
	{
		UpdateAgent8Attached();
		break;
	}
	case STATE_FLY:
	{
		UpdateAgent8Fly();
		break;
	}
	case STATE_DEAD:
	{
		UpdateAgent8Dead();
		break;
	}
	}

	WrapObject(agentObj.pos);
	Play::UpdateGameObject(agentObj);
}

void UpdateAgent8Attached()
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };
	GameObject& attachedObj{ Play::GetGameObjectByType(TYPE_ATTACHED) };

	if (!gameState.asteroids.active)
	{
		agentObj.velocity = { 0, 0 };
		agentObj.rotation += PLAY_PI;
		gameState.asteroids.active = true;
		Play::SetSprite(agentObj, "agent8_left", 0.f);
	}

	agentObj.pos = { attachedObj.pos.x, attachedObj.pos.y };

	if (Play::KeyDown(VK_LEFT))
	{
		Play::SetSprite(agentObj, "agent8_left", 0.05f);
		agentObj.rotation -= gameState.agent8.CRAWL_SPEED;
	}

	if (Play::KeyDown(VK_RIGHT))
	{
		Play::SetSprite(agentObj, "agent8_right", 0.05f);
		agentObj.rotation += gameState.agent8.CRAWL_SPEED;
	}

	if (Play::KeyPressed(VK_SPACE))
	{
		Play::SetGameObjectDirection(agentObj, gameState.agent8.FLY_SPEED, agentObj.rotation);
		Vector2f unitVector = agentObj.velocity;
		unitVector.Normalize();
		agentObj.pos += (unitVector * gameState.agent8.JUMP_MAGNITUDE);
		gameState.asteroids.active = false;
		gameState.agent8.attached = false;
		gameState.agent8.state = STATE_FLY;
		Play::PlayAudio("explode");
		DestroyAttached();
	}
}

void UpdateAgent8Fly()
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };
	Play::SetSprite(agentObj, "agent8_fly", 0.0f);

	if (Play::KeyDown(VK_RIGHT))
		agentObj.rotation = agentObj.rotation + gameState.agent8.TURNING_SPEED;

	if (Play::KeyDown(VK_LEFT))
		agentObj.rotation = agentObj.rotation - gameState.agent8.TURNING_SPEED;

	Play::SetGameObjectDirection(agentObj, gameState.agent8.FLY_SPEED, agentObj.rotation);
}

void UpdateAgent8Dead()
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };
	gameState.agent8.active = false;
	Play::SetSprite(agentObj, "agent8_dead", 0.25f);
	Play::SetGameObjectDirection(agentObj, gameState.agent8.FLY_SPEED, agentObj.rotation);
}

void UpdateMeteor()
{
	std::vector<int> meteorIds{ Play::CollectGameObjectIDsByType(TYPE_METEOR) };

	for (int meteorId : meteorIds)
	{
		GameObject& meteorObj{ Play::GetGameObject(meteorId) };
		Play::SetSprite(meteorObj, "meteor", 0.25f);
		WrapObject(meteorObj.pos);
		Play::UpdateGameObject(meteorObj);
	}
}

void UpdateAsteroid()
{
	std::vector<int> asteroidIds{ Play::CollectGameObjectIDsByType(TYPE_ASTEROID) };

	for (int asteroidId : asteroidIds)
	{
		GameObject& asteroidObj{ Play::GetGameObject(asteroidId) };
		Play::SetSprite(asteroidObj, "asteroid", 0.25f);
		WrapObject(asteroidObj.pos);
		Play::UpdateGameObject(asteroidObj);
	}
}

void UpdateAttached()
{
	GameObject& attachedObj{ Play::GetGameObjectByType(TYPE_ATTACHED) };
	Play::SetSprite(attachedObj, "asteroid", 0.25f);
	WrapObject(attachedObj.pos);
	Play::UpdateGameObject(attachedObj);
}

void UpdatePieces()
{
	std::vector<int> pieceIds{ Play::CollectGameObjectIDsByType(TYPE_PIECE) };

	for (int pieceId : pieceIds)
	{
		GameObject& pieceObj{ Play::GetGameObject(pieceId) };

		if (!Play::IsVisible(pieceObj))
			Play::DestroyGameObject(pieceId);

		Play::UpdateGameObject(pieceObj);
	}
}

void UpdateGem(float& elapsedTime)
{
	std::vector<int> gemIds{ Play::CollectGameObjectIDsByType(TYPE_GEM) };
	gameState.gems.splitTime += elapsedTime;

	if (gameState.gems.splitTime > gameState.gems.wobblePeriod)
	{
		gameState.gems.wobbleDirection *= -1.0f;
		gameState.gems.splitTime = 0;
	}

	for (int gemId : gemIds)
	{
		GameObject& gemObj{ Play::GetGameObject(gemId) };
		float wobble = gameState.gems.wobbleSpeed * gameState.gems.wobbleDirection * sin(elapsedTime * PLAY_PI);
		gemObj.rotation += wobble;
		Play::UpdateGameObject(gemObj);
	}
}

void UpdateScore(float& elapsedTime)
{
	std::vector<int> attachedIds{ Play::CollectGameObjectIDsByType(TYPE_ATTACHED) };
	std::vector<int> asteroidIds{ Play::CollectGameObjectIDsByType(TYPE_ASTEROID) };
	std::vector<int> gemIds{ Play::CollectGameObjectIDsByType(TYPE_GEM) };

	switch (gameState.agent8.state)
	{
	case STATE_DEAD:
	{
		gameState.score;
		gameState.gemsRemaining;
		break;
	}
	case STATE_FLY:
	{
		if (gameState.score < 1)
			gameState.score = 0;
		else
			gameState.score -= elapsedTime;
		gameState.gemsRemaining = asteroidIds.size() + gemIds.size() + attachedIds.size();
		if (gameState.hiScore < gameState.score)
			gameState.hiScore = gameState.score;
		break;
	}
	default:
	{
		gameState.gemsRemaining = asteroidIds.size() + gemIds.size() + attachedIds.size();
		break;
	}
	}
}

void DrawGamePlay(float& elapsedTime)
{
	DrawParticle(elapsedTime);
	DrawBlueRing(elapsedTime);
	DrawGameObjects(TYPE_PIECE);
	DrawGameObjects(TYPE_ASTEROID);
	DrawGameObjects(TYPE_METEOR);
	DrawGameObjects(TYPE_GEM);
	DrawGameObjects(TYPE_ATTACHED);
	DrawGameObjects(TYPE_AGENT8);
	DrawGUI();
	Play::PresentDrawingBuffer();
}

void DrawGameObjects(int type)
{
	std::vector<int> objIds{ Play::CollectGameObjectIDsByType(type) };

	for (int objId : objIds)
	{
		Play::DrawObjectRotated(Play::GetGameObject(objId));
	}
}

void DrawParticle(float& elapsedTime)
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };

	switch (gameState.agent8.state)
	{
	case STATE_FLY:
	{
		gameState.particleEmitter.splitTime += elapsedTime;

		if (gameState.particleEmitter.splitTime > gameState.particleEmitter.emitPeriod)
		{
			AddParticleToEmitter(agentObj);
			gameState.particleEmitter.splitTime = 0;
		}

		if (!gameState.particleEmitter.vParticle.empty())
		{
			UpdateParticleLifeTime(elapsedTime);
		}
		break;
	}
	default:
	{
		if (!gameState.particleEmitter.vParticle.empty())
		{
			UpdateParticleLifeTime(elapsedTime);
		}
		break;
	}
	}
}

void AddParticleToEmitter(GameObject& agentObj)
{
	Particle particle;

	for (int i = 0; i < gameState.particleEmitter.emitParticles; ++i)
	{
		Vector2D randomPos = { Play::RandomRollRange(-particle.posRANDOMNESS, particle.posRANDOMNESS), Play::RandomRollRange(-particle.posRANDOMNESS, particle.posRANDOMNESS) };
		gameState.particleEmitter.vParticle.push_back(particle);
		gameState.particleEmitter.vParticle.back().pos = agentObj.pos + randomPos;
	}

	if (gameState.particleEmitter.vParticle.size() > gameState.particleEmitter.maxParticles)
		gameState.particleEmitter.vParticle.erase(gameState.particleEmitter.vParticle.begin());
}

void UpdateParticleLifeTime(float& elapsedTime)
{
	Particle particle;

	for (int i = 0; i < gameState.particleEmitter.vParticle.size(); ++i)
	{
		gameState.particleEmitter.vParticle[i].currentLifetime += elapsedTime;
		float opacity = exponentialDecay(gameState.particleEmitter.vParticle[i].baseOpacity, gameState.particleEmitter.vParticle[i].decayConstant, gameState.particleEmitter.vParticle[i].currentLifetime);
		Play::DrawSpriteTransparent(Play::GetSpriteId("particle"), gameState.particleEmitter.vParticle[i].pos, 1.0f, opacity);

		if (opacity < particle.opacityThreshold)
			gameState.particleEmitter.vParticle.erase(gameState.particleEmitter.vParticle.begin() + i);
	}
}

void DrawBlueRing(float& elapsedTime)
{
	BlueRing blueRing;

	if (gameState.blueRingEmitter.vBlueRing.empty())
		return;

	for (int k = 0; k < gameState.blueRingEmitter.vBlueRing.size(); ++k)
	{
		if (gameState.blueRingEmitter.vBlueRing[k].currentLifetime > gameState.blueRingEmitter.vBlueRing[k].lifetime)
			gameState.blueRingEmitter.vBlueRing.erase(gameState.blueRingEmitter.vBlueRing.begin() + k);
	}

	for (BlueRing& blueRingElement : gameState.blueRingEmitter.vBlueRing)
	{
		blueRingElement.currentLifetime += elapsedTime;

		float opacity = exponentialDecay(1.0f, blueRingElement.decayConstant, blueRingElement.currentLifetime);

		for (int i = 0; i < blueRingElement.nRings; ++i)
		{
			float scale = blueRing.baseScale + (blueRing.baseScale * 0.1f * i);
			Play::DrawSpriteRotated(Play::GetSpriteId("blue_ring"), blueRingElement.pos, 0, 0, scale * blueRingElement.currentLifetime, opacity);
		}
	}
}

void DrawGUI()
{
	Play::DrawFontText("64px", "Gems Remaining: " + std::to_string(gameState.gemsRemaining), Point2D(50, 100), Play::LEFT);
	
	if (gameState.level == 0)
		Play::DrawFontText("64px", "Level: 1", Point2D(DISPLAY_WIDTH / 2, 100), Play::CENTRE);
	else
		Play::DrawFontText("64px", "Level: " + std::to_string(gameState.level), Point2D(DISPLAY_WIDTH / 2, 100), Play::CENTRE);
	
	Play::DrawFontText("64px", "High Score: " + std::to_string(gameState.hiScore), Point2D(DISPLAY_WIDTH - 50, 100), Play::RIGHT);
	Play::DrawFontText("64px", "Score: " + std::to_string(gameState.score), Point2D(DISPLAY_WIDTH - 50, 150), Play::RIGHT);

	if (gameState.agent8.state == STATE_DEAD)
	{
		Play::DrawFontText("105px", "GAME OVER", Point2D(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2), Play::CENTRE);
		Play::DrawFontText("64px", "Press SPACE to restart", Point2D(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30), Play::CENTRE);
	}
}

void CreateAgent8()
{
	Play::CreateGameObject(TYPE_AGENT8, { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, 0, "agent8_fly");
	gameState.agent8.active = true;
}

void CreateMeteor(int& numberOfMeteors)
{
	for (int i = 0; i < numberOfMeteors; ++i)
	{
		int tempId = Play::CreateGameObject(TYPE_METEOR, { 0, 0 }, 0, "meteor");
		GameObject& meteorObj{ Play::GetGameObject(tempId) };
		meteorObj.pos = { Play::RandomRollRange(0, DISPLAY_WIDTH), Play::RandomRollRange(0, DISPLAY_HEIGHT) };
		meteorObj.rotation = Play::DegToRad(Play::RandomRollRange(0, 360));
		int speed = Play::RandomRollRange(gameState.meteors.MIN_SPEED, gameState.meteors.MAX_SPEED);
		Play::SetGameObjectDirection(meteorObj, speed, meteorObj.rotation);
	}
}

void CreateAsteroid(int& numberOfAsteroids)
{
	for (int i = 0; i < numberOfAsteroids; ++i)
	{
		int tempId = Play::CreateGameObject(TYPE_ASTEROID, { 0, 0 }, 0, "asteroid");
		GameObject& asteroidObj{ Play::GetGameObject(tempId) };
		asteroidObj.pos = { Play::RandomRollRange(0, DISPLAY_WIDTH), Play::RandomRollRange(0, DISPLAY_HEIGHT) };
		asteroidObj.rotation = Play::DegToRad(Play::RandomRollRange(0, 360));
		int speed = Play::RandomRollRange(gameState.asteroids.MIN_SPEED, gameState.asteroids.MAX_SPEED);
		Play::SetGameObjectDirection(asteroidObj, speed, asteroidObj.rotation);
	}
}

void CreatePieces(Point2D& attachedPos)
{
	GameObject& agentObj{ Play::GetGameObjectByType(TYPE_AGENT8) };
	GameObject& attachedObj{ Play::GetGameObjectByType(TYPE_ATTACHED) };

	for (int i = 0; i < 3; ++i)
	{
		int tempId = Play::CreateGameObject(TYPE_PIECE, attachedPos, 0, "asteroid_pieces");
		GameObject& pieceObj{ Play::GetGameObject(tempId) };
		pieceObj.frame = i;

		float angle = Play::DegToRad(-120 * i) + agentObj.rotation;
		pieceObj.rotation = agentObj.rotation;
		Play::SetGameObjectDirection(pieceObj, gameState.asteroids.PIECES_SPEED, angle);
	}
}

void CreateGem(Point2D& attachedPos)
{
	Point2D gemPos;

	if (attachedPos.x > 0 && attachedPos.x < DISPLAY_WIDTH && attachedPos.y > 0 && attachedPos.y < DISPLAY_HEIGHT)
		gemPos = attachedPos;

	else
		gemPos = { Play::RandomRollRange(DISPLAY_WINDOW_INC, DISPLAY_WIDTH), Play::RandomRollRange(DISPLAY_WINDOW_INC, DISPLAY_HEIGHT) };

	Play::CreateGameObject(TYPE_GEM, gemPos, 0, "gem");
}

void DestroyAttached()
{
	GameObject& attachedObj{ Play::GetGameObjectByType(TYPE_ATTACHED) };
	CreatePieces(attachedObj.pos);
	CreateGem(attachedObj.pos);
	Play::DestroyGameObject(attachedObj.GetId());
}

void DestroyGem(GameObject& gemObj)
{
	BlueRing blueRing;
	blueRing.pos = gemObj.pos;
	gameState.blueRingEmitter.vBlueRing.push_back(blueRing);
	Play::DestroyGameObject(gemObj.GetId());
}

void WrapObject(Point2D& objPos)
{
	const float minX = 0 - DISPLAY_WINDOW_INC;
	const float maxX = DISPLAY_WIDTH + DISPLAY_WINDOW_INC;
	const float minY = 0 - DISPLAY_WINDOW_INC;
	const float maxY = DISPLAY_HEIGHT + DISPLAY_WINDOW_INC;

	if (objPos.x < minX)
		objPos.x = maxX;

	else if (objPos.x > maxX)
		objPos.x = minX;

	if (objPos.y < minY)
		objPos.y = maxY;

	else if (objPos.y > maxY)
		objPos.y = minY;
}

void WrapObjectBetweenScreen(GameObject& obj)
{
	const float minX = 0 - DISPLAY_WINDOW_INC;
	const float maxX = DISPLAY_WIDTH + DISPLAY_WINDOW_INC;
	const float minY = 0 - DISPLAY_WINDOW_INC;
	const float maxY = DISPLAY_HEIGHT + DISPLAY_WINDOW_INC;

	if (obj.pos.x < minX)
		obj.pos.x = maxX;

	else if (obj.pos.x > maxX)
		obj.pos.x = minX;

	if (obj.pos.y < minY)
		obj.pos.y = maxY;

	else if (obj.pos.y > maxY)
		obj.pos.y = minY;
}

bool AABBCollisionTest(const Point2D& aPos, const Vector2D& aAABB, const Point2D& bPos, const Vector2D& bAABB)
{
	return (aPos.x - aAABB.x < bPos.x + bAABB.x
		&& aPos.x + aAABB.x > bPos.x - bAABB.x
		&& aPos.y - aAABB.y < bPos.y + bAABB.y
		&& aPos.y + aAABB.y > bPos.y - bAABB.y);
}

float exponentialDecay(float A0, float lambda, float time)
{
	return A0 * exp(-lambda * time);
}