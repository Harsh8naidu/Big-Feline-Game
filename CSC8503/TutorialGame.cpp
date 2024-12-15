#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"
#include "NetworkedGame.h"
#include <NavigationGrid.h>
#include "NetworkPlayer.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame() : controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()) {
	world		= new GameWorld();
#ifdef USEVULKAN
	renderer	= new GameTechVulkanRenderer(*world);
	renderer->Init();
	renderer->InitStructures();
#else 
	renderer = new GameTechRenderer(*world);
#endif
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	world->GetMainCamera().SetController(controller);

	controller.MapAxis(0, "Sidestep");
	controller.MapAxis(1, "UpDown");
	controller.MapAxis(2, "Forward");

	controller.MapAxis(3, "XLook");
	controller.MapAxis(4, "YLook");

	testStateObject = nullptr;

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	catMesh		= renderer->LoadMesh("ORIGAMI_Chat.msh");
	kittenMesh	= renderer->LoadMesh("Kitten.msh");

	//enemyMesh	= renderer->LoadMesh("Keeper.msh");
	enemyMesh = renderer->LoadMesh("goose.msh");
	bonusMesh	= renderer->LoadMesh("19463_Kitten_Head_v1.msh");
	capsuleMesh = renderer->LoadMesh("Capsule.msh");

	planeMesh = renderer->LoadMesh("Plane.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");

	catTex = renderer->LoadTexture("GoatBeige.png");

	planeTex = renderer->LoadTexture("GoatBlack.png");
	capsuleTex = renderer->LoadTexture("Default.png");

	bonusTex = renderer->LoadTexture("goldtex.png");


	InitCamera();
	//InitWorld(); //This is now done in the network tutorial to allow for server/client selection
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete catMesh;
	delete kittenMesh;
	delete enemyMesh;
	delete bonusMesh;
	delete capsuleMesh;
	delete planeMesh;

	delete basicTex;
	delete basicShader;
	delete catTex;
	delete planeTex;
	delete capsuleTex;

	delete bonusTex;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	if (Window::GetKeyboard()->KeyPressed(NCL::KeyCodes::P)) {
		// Pause the game
		isGamePaused = !isGamePaused;
	}

	if (isGamePaused) {
		Debug::Print("Game Paused", Vector2(40, 50), Debug::RED);
		return;
	}

	if (!inSelectionMode) {
		world->GetMainCamera().UpdateCamera(dt);
	}

	if (lockedObject == nullptr && isCameraLocked) {
		lockedObject = (GameObject*)player; // lock the camera to the player
		selectionObject = lockedObject;
		
	}
	
	if (Window::GetKeyboard()->KeyPressed(NCL::KeyCodes::C)) {
		
		lockedObject = nullptr; // release the camera from the player
		isCameraLocked = !isCameraLocked;
		isGameStart = true;
		useGravity = true;
		physics->UseGravity(useGravity);

		// Reset the timer when the game starts
		gameTimer = 0.0f;
	}
	else if (!isGameStart) {
		Debug::Print("Press C to start the Game", Vector2(25, 30), Debug::CYAN);
		Debug::Print("Press P to pause the Game", Vector2(25, 40), Debug::CYAN);

		float yPosOfUI = 50;
		// Controls
		Debug::Print("Controls", Vector2(25, 45), Debug::YELLOW);
		Debug::Print("Press W to move forward", Vector2(25, yPosOfUI), Debug::BLUE);
		Debug::Print("Press S to move backward", Vector2(25, yPosOfUI + 10), Debug::BLUE);
		Debug::Print("Press A to move left", Vector2(25, yPosOfUI + 20), Debug::BLUE);
		Debug::Print("Press D to move right", Vector2(25, yPosOfUI + 30), Debug::BLUE);
		Debug::Print("Press SPACE to jump", Vector2(25, yPosOfUI + 40), Debug::BLUE);

		if (isGameEnd) {
			Debug::Print("Game Over", Vector2(45, 10), Debug::RED);
		}
	}
	//Debug::Print("Game Over", Vector2(45, 10), Debug::RED);

	score = player->GetScore();

	Debug::Print("Score: " + std::to_string(score) + "/6", Vector2(75, 15), Debug::GREEN);

	if (isGameEnd) {
		Debug::Print("High Score: " + std::to_string(score), Vector2(40, 20), Debug::RED);
	}

	if (isGameStart) {
		// Update the timer
		gameTimer += dt;

		// Format and display the timer as minutes:seconds
		int minutes = static_cast<int>(gameTimer) / 60;
		int seconds = static_cast<int>(gameTimer) % 60;
		std::string timerText = "Timer: " + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds) + "/5:00";

		Debug::Print(timerText, Vector2(35, 10), Debug::GREEN);

		// End the game after 5 minutes
		if (gameTimer >= 300.0f || score == 6) { // 300 seconds = 5 minutes

			// Logic to handle game end (e.g., restart, quit, show end screen)
			isGameStart = false;
			gameTimer = 0.0f; // Reset timer
			isGameEnd = true;
		}
	}

	

	if (lockedObject != nullptr) {
		// Get the position of the locked object
		Vector3 objPos = lockedObject->GetTransform().GetPosition();

		// Update camera rotation based on controller input
		lockedAngle.x -= controller.GetNamedAxis("XLook") * 100.0f * dt;
		lockedAngle.y -= controller.GetNamedAxis("YLook") * 100.0f * dt;
		lockedAngle.y = std::max(-89.0f, std::min(89.0f, lockedAngle.y)); // Clamp vertical angle

		// Adjust zoom level with the mouse wheel
		float scroll = Window::GetMouse()->GetWheelMovement() * 0.1f;
		lockedOffset -= lockedOffset * scroll;

		// Calculate the offset from the locked object's position
		Vector3 offset = Quaternion::EulerAnglesToQuaternion(lockedAngle.y, lockedAngle.x, lockedAngle.z) * lockedOffset;

		// Determine the camera's intended position
		Vector3 camPos = objPos + offset;

		// Handle camera collision with environment
		Vector3 direction = Vector::Normalise(camPos - objPos);
		Ray ray = Ray(objPos, direction);
		RayCollision closestCollision;
		bool hit = world->Raycast(ray, closestCollision, true, lockedObject);

		if (hit && Vector::Distance(closestCollision.collidedAt, objPos) < Vector::Distance(camPos, objPos)) {
			// Apply a bias to prevent near-plane clipping
			float bias = 0.5f;
			camPos = closestCollision.collidedAt - direction * bias;
		}

		// Calculate view matrix and update camera
		Matrix4 temp = Matrix::View(camPos, objPos, Vector3(0, 1, 0));
		Matrix4 modelMat = Matrix::Inverse(temp);

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); // Convert to Euler angles

		world->GetMainCamera().SetPosition(camPos);
		world->GetMainCamera().SetPitch(angles.x);
		world->GetMainCamera().SetYaw(angles.y);
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
	}

	RayCollision closestCollision;
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::K) && selectionObject) {
		Vector3 rayPos;
		Vector3 rayDir;

		rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

		rayPos = selectionObject->GetTransform().GetPosition();

		Ray r = Ray(rayPos, rayDir);

		if (world->Raycast(r, closestCollision, true, selectionObject)) {
			if (objClosest) {
				objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			}
			objClosest = (GameObject*)closestCollision.node;

			objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		}
	}

	//Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));

	SelectObject();
	MoveSelectedObject();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	renderer->Render();
	Debug::UpdateRenderables(dt);

	if (testStateObject) {
		testStateObject->Update(dt);
	}

	bonus1->GameObjectUpdate(dt);

	plane->GameObjectUpdate(dt);
	door->GameObjectUpdate(dt);
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::G)) {
		//useGravity = !useGravity; //Toggle gravity!
		//physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F7)) {
		//world->ShuffleObjects(true);
		player->SetActive(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F8)) {
		//world->ShuffleObjects(false);
		player->SetActive(false);
	}


	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera().BuildViewMatrix();
	Matrix4 camWorld	= Matrix::Inverse(view);

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis = Vector::Normalise(fwdAxis);

	if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::NEXT)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyCodes::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera().SetNearPlane(0.1f);
	world->GetMainCamera().SetFarPlane(500.0f);
	world->GetMainCamera().SetPitch(-15.0f);
	world->GetMainCamera().SetYaw(315.0f);
	world->GetMainCamera().SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	// Create a simple constraint to test the system
	BridgeConstraintTest();

	InitMixedGridWorld(15, 15, 3.5f, 3.5f);

	InitGameExamples();
	InitDefaultFloor();

	testStateObject = AddStateObjectToWorld(Vector3(0, 10, -10));
}

GameObject* TutorialGame::AddDoorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("door");

	Vector3 doorSize = Vector3(4, 5.5, 0.2f);
	AABBVolume* volume = new AABBVolume(Vector3(4, 5, 0.2f) * 10.0f);
	//floor->SetBoundingVolume((CollisionVolume*)volume);
	
	floor->GetTransform()
		.SetScale(doorSize * 6.0f)
		.SetPosition(position)
		.SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 90));

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	floor->SetLayer(Layer::IgnoreRaycast); // optional, but useful for the raycast to ignore this object

	world->AddGameObject(floor);

	//Debug::DrawAxisLines(floor->GetTransform().GetMatrix(), 5.0f, 120.0f);

	return floor;
}

GameObject* TutorialGame::AddBlackObstacleToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject("plane");

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2.0f);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, planeTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetPhysicsObject()->SetElasticity(1.0f); // Different elasticity for the plane

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddPlaneToWorld(const Vector3& position, const Vector3& scale) {
	GameObject* plane = new GameObject();

	Plane* volume = new Plane();
	plane->SetBoundingVolume((CollisionVolume*)volume);

	plane->GetTransform()
		.SetPosition(position)
		.SetOrientation(Quaternion::Identity()) // Plane will stay flat (no rotation)
		.SetScale(scale);

	plane->SetRenderObject(new RenderObject(&plane->GetTransform(), planeMesh, capsuleTex, basicShader));
	plane->SetPhysicsObject(new PhysicsObject(&plane->GetTransform(), plane->GetBoundingVolume()));
	plane->GetPhysicsObject()->SetInverseMass(1);
	plane->GetPhysicsObject()->SetElasticity(0.7);

	world->AddGameObject(plane);

	return plane;
}


GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(radius * 2, halfHeight * 2, radius * 2));

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, capsuleTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCapsuleInertia();
	capsule->GetPhysicsObject()->SetElasticity(0.5f); // Different elasticity for the capsule

	world->AddGameObject(capsule);

	Debug::DrawLine(position, position + Vector3(0, 5, 0), Vector4(1, 0, 0, 1), 120);

	return capsule;
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(200, 2, 200);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2.0f)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	floor->SetLayer(Layer::IgnoreRaycast); // optional, but useful for the raycast to ignore this object

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, bool isHollow) {
	GameObject* sphere = new GameObject("sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	if (isHollow) {
		sphere->GetPhysicsObject()->InitHollowSphereInertia();
	}
	else {
		sphere->GetPhysicsObject()->InitSphereInertia();
	}

	sphere->GetPhysicsObject()->SetInverseMass(30);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2.0f);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddOBBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	//AABBVolume* volume = new AABBVolume(dimensions);
	OBBVolume* volume = new OBBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2.0f);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

void TutorialGame::GenerateMaze(NavigationGrid& grid) {
	// loop through the grid and find all wall tiles ('x')
	for (int y = 0; y < grid.GetGridHeight(); ++y) {
		for (int x = 0; x < grid.GetGridWidth(); ++x) {
			// access each grid node
			GridNode& node = grid.GetGridCount()[y * grid.GetGridWidth() + x];
			
			// check if the node is a wall ('x')
			if (node.type == 'x') {
				// create a position vector for where the wall is
				Vector3 position = node.position;

				wallPositions.push_back(position);

				Vector3 cubeSize = Vector3(8, 8, 8);

				//AddCubeToWorld(position, cubeSize);

				//Debug::DrawLine(position, position + Vector3(0, 5, 0), Vector4(1, 0, 0, 1), 120);
			}
		}
	}
}

//KittenStateObject* TutorialGame::AddKittensToWorld(const Vector3& position) {
//	KittenStateObject* kitten = new KittenStateObject();
//
//	SphereVolume* volume = new SphereVolume(0.5f);
//	kitten->SetBoundingVolume((CollisionVolume*)volume);
//	kitten->GetTransform()
//		.SetScale(Vector3(2, 2, 2))
//		.SetPosition(position);
//
//	kitten->SetRenderObject(new RenderObject(&kitten->GetTransform(), kittenMesh, nullptr, basicShader));
//	kitten->SetPhysicsObject(new PhysicsObject(&kitten->GetTransform(), kitten->GetBoundingVolume()));
//
//	kitten->GetPhysicsObject()->SetInverseMass(1.0f);
//	kitten->GetPhysicsObject()->InitSphereInertia();
//
//	world->AddGameObject(kitten);
//
//	return kitten;
//}

void TutorialGame::AddFlyingStairs() {
	Vector3 cubeSize = Vector3(5, 5, 5);
	Vector3 position = Vector3(0, 0, 0);
	for (int i = 0; i < 12; ++i) {
		if (i < 6) {
			// Ascending stairs
			position = Vector3(-50, 5 + (i * 10), i * 10);
		}
		else {
			// Descending stairs
			int descendIndex = i - 6;
			position = Vector3(-50, 55 - (descendIndex * 10), (6 + descendIndex) * 10);
		}
		AddCubeToWorld(position, cubeSize, 0);
	}
}



NetworkPlayer* TutorialGame::AddPlayerToWorld(const Vector3& position, std::string playerName) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	NetworkPlayer* character = new NetworkPlayer(playerName);
	SphereVolume* volume  = new SphereVolume(1.0f);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), catMesh, catTex, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	//player = (NetworkPlayer*)character;

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("bonus");

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, bonusTex, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

void TutorialGame::BridgeConstraintTest()
{
	Vector3 cubeSize = Vector3(4, 4, 4);

	float invCubeMass = 1; // how heavy the middle pieces are
	int numLinks = 5; // how many links in the chain
	float maxDistance = 13; // how far apart the links are
	float cubeDistance = 10; // how far apart the cubes are

	Vector3 startPos = Vector3(-130, 56, 60);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0); // add the first cube
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0); // add the last cube

	std::cout << "start" << start << " , " << "end" << end << std::endl;

	GameObject* previous = start; // the previous cube is the start cube

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass); // add a cube
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance); // create a constraint between the previous cube and the new cube
		world->AddConstraint(constraint); // add the constraint to the world

		// Draw debug line between the cubes
		//Debug::DrawLine(previous->GetTransform().GetPosition(), block->GetTransform().GetPosition(), Vector4(0, 0, 1, 1), 120);

		previous = block; // the previous cube is now the new cube
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance); // create a constraint between the last cube and the end cube
	world->AddConstraint(constraint); // add the constraint to the world

	// Draw debug line between the last cube and the end cube
	//Debug::DrawLine(previous->GetTransform().GetPosition(), end->GetTransform().GetPosition(), Vector4(0, 0, 1, 1), 120);
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position)
{
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* TutorialGame::AddAngryGooseToWorld(const Vector3& position, const std::vector<Vector3>& path, GameObject* player, GameWorld* world) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	StateGameObject* character = new StateGameObject(path, player, world);

	//AABBVolume* volume = new AABBVolume(Vector3(0.2f, 0.5f, 0.2f) * meshSize);
	// SphereVolume worked better for the goose
	SphereVolume* volume = new SphereVolume(0.5f);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position + Vector3(0, 4, 0));

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetLinearDamping(0.2f);

	character->GetPhysicsObject()->SetAngularDamping(0.2f);

	character->GetPhysicsObject()->SetInverseMass(0.1);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	std::cout << "Goose added to the world" << std::endl;

	return character;
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -20, 0));
}

void TutorialGame::InitGameExamples() {
	//AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		//Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::Left)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true, nullptr, IgnoreRaycastLayerMask)) {
				selectionObject = (GameObject*)closestCollision.node;

				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				Debug::DrawLine(ray.GetPosition(), closestCollision.collidedAt, Vector4(1, 0, 0, 1), 2);
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyCodes::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		//Debug::Print("Press Q to change to select mode!", Vector2(5, 85));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	//Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return; // We haven't selected anything!
	}

	// Timer and speed setup
	float dt = Window::GetWindow()->GetTimer().GetTimeDeltaSeconds();
	float speed = 10.0f;
	Vector3 moveDirection = Vector3(0, 0, 0); // Accumulates movement forces

	// Grounded check using raycast
	Ray groundRay(selectionObject->GetTransform().GetPosition(), Vector3(0, -1, 0));
	RayCollision groundCollision;
	bool isGrounded = world->Raycast(groundRay, groundCollision, true) && groundCollision.rayDistance < 0.1f;

	// Keyboard input handling
	if (Window::GetKeyboard()->KeyHeld(NCL::KeyCodes::W)) {
		moveDirection += Vector3(0, 0, speed);
		selectionObject->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 0));
	}
	if (Window::GetKeyboard()->KeyHeld(NCL::KeyCodes::S)) {
		moveDirection += Vector3(0, 0, -speed);
		selectionObject->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 180));
	}
	if (Window::GetKeyboard()->KeyHeld(NCL::KeyCodes::A)) {
		moveDirection += Vector3(speed, 0, 0);
		selectionObject->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 90));
	}
	if (Window::GetKeyboard()->KeyHeld(NCL::KeyCodes::D)) {
		moveDirection += Vector3(-speed, 0, 0);
		selectionObject->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -90));
	}
	if (Window::GetKeyboard()->KeyHeld(NCL::KeyCodes::NUM2)) {
		moveDirection += Vector3(0, -speed, 0);
	}

	// Jump logic
	if (Window::GetKeyboard()->KeyPressed(NCL::KeyCodes::SPACE) && isGrounded) {
		selectionObject->GetPhysicsObject()->ApplyLinearImpulse(Vector3(0, 20.0f, 0));
	}

	// Apply accumulated forces for movement
	if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
		selectionObject->GetPhysicsObject()->AddForce(moveDirection);
	}
	else {
		// Stop the object if no movement keys are pressed
		selectionObject->GetPhysicsObject()->ClearForces();
		selectionObject->GetPhysicsObject()->SetLinearVelocity(Vector3(0, selectionObject->GetPhysicsObject()->GetLinearVelocity().y, 0));
		selectionObject->GetPhysicsObject()->SetAngularVelocity(Vector3(0, 0, 0));
	}

	// Gravity handling:
	if (!isGrounded) {
		// Ensure gravity is applied when the object is in the air
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -30.8f, 0));  // Assuming standard gravity
	}

	// Push the selected object using the mouse
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::Right)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}