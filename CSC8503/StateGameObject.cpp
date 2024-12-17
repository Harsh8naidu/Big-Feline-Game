#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include <GameWorld.h>
#include "NetworkPlayer.h"
#include "Window.h"
#include "TutorialGame.h"

using namespace NCL;
using namespace CSC8503;

StateGameObject::StateGameObject(const std::vector<Vector3>& path, GameObject* playerObj, GameWorld* world) {
	counter = 0.0f;
	waypointIndex = 0;
	waypoints = path;
	movingForward = true; // Start by moving forward along the path
	player = playerObj;
	gameWorld = world;

	stateMachine = new StateMachine();

	State* moveState = new State([&](float dt)->void {
		this->MoveToWaypoint(dt);
		});

	// Chasing state
	State* chaseState = new State([&](float dt)->void {
		this->ChasePlayer(dt);
		});

	stateMachine->AddState(moveState);
	stateMachine->AddState(chaseState);

	// Transition from path-following to chasing
	stateMachine->AddTransition(new StateTransition(moveState, chaseState, [&]()->bool {
		return this->DetectPlayer();
		}));

	// Transition from chasing back to path-following
	stateMachine->AddTransition(new StateTransition(chaseState, moveState, [&]()->bool {
		return !this->DetectPlayer();
		}));
}

StateGameObject::StateGameObject(GameObject* playerObj, GameWorld* world, GameObject* kittenHome) {
	player = playerObj;
	gameWorld = world;
	home = kittenHome;

	stateMachine = new StateMachine();

	State* followState = new State([&](float dt)->void {
		this->FollowPlayer(dt);
		});

	stateMachine->AddState(followState); // Only one state for following
}

StateGameObject::StateGameObject()
{
}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	if (stateMachine != nullptr) {
		stateMachine->Update(dt);
	}
	
}

void StateGameObject::MoveToWaypoint(float dt) {
	if (waypointIndex < 0 || waypointIndex >= waypoints.size()) return;

	Vector3 currentPos = GetTransform().GetPosition();
	Vector3 targetPos = waypoints[waypointIndex];

	Vector3 direction = targetPos - currentPos;
	float distance = Vector::Length(direction);

	if (distance < 11.0f) {
		// Move to the next or previous waypoint depending on direction
		if (movingForward) {
			waypointIndex++;
			if (waypointIndex >= waypoints.size()) {
				movingForward = false; // Reverse directionas
				waypointIndex = waypoints.size() - 2; // Go back one step
			}
		}
		else {
			waypointIndex--;
			if (waypointIndex < 0) {
				movingForward = true; // Reverse direction
				waypointIndex = 1; // Start from second waypoint
			}
		}
	}
	else {
		Vector::Normalise(direction);
		GetPhysicsObject()->AddForce(direction * 1.0f);
	}
}

void StateGameObject::ChasePlayer(float dt) {
	if (!player) return;

	Vector3 currentPos = GetTransform().GetPosition();
	Vector3 playerPos = player->GetTransform().GetPosition();

	Vector3 direction = playerPos - currentPos;
	Vector::Normalise(direction);

	GetPhysicsObject()->AddForce(direction * 5.0f); // Faster speed when chasing
}

void StateGameObject::FollowPlayer(float dt) {
	if (!player || !gameWorld || !home) return;

	Vector3 currentPos = GetTransform().GetPosition();
	Vector3 playerPos = player->GetTransform().GetPosition();
	Vector3 direction = playerPos - currentPos;

	float distance = Vector::Length(direction);
	float detectionRange = 10.0f; // Adjust this value as needed

	if (distance > detectionRange) {
		return; // Player is too far to follow
	}

	Ray ray(currentPos, Vector::Normalise(direction));
	RayCollision collision;

	if (gameWorld->Raycast(ray, collision, true)) {
		Debug::DrawLine(currentPos, playerPos, Vector4(0, 1, 0, 1)); // Visualize the ray in green
		if (collision.node == home) {
			// Stop following and clear forces
			GetPhysicsObject()->ClearForces();
			stateMachine->Update(0.0f); // Reset state machine or pause updates
			std::cout << "Kitten has reached home and stopped following the player." << std::endl;
			return;
		} else if (collision.node == player) {
			//if (distance < 10.0f) { // Stay close to the player
			//	return;
			//}
			Vector::Normalise(direction);
			GetPhysicsObject()->AddForce(direction * 3.0f); // Moderate speed when following
		}
		
		else {
			GetPhysicsObject()->ClearForces(); // Slow down if the player is not in sight
		}
	}
	else {
		GetPhysicsObject()->ClearForces(); // Slow down if the player is not in sight
	}
}

bool StateGameObject::DetectPlayer() {
	if (!player || !gameWorld) return false;

	Vector3 currentPos = GetTransform().GetPosition();
	Vector3 playerPos = player->GetTransform().GetPosition();
	Vector3 direction = playerPos - currentPos;

	float distance = Vector::Length(direction); // Calculate the distance between the objects
	float detectionRange = 10.0f; // Adjust this value to set the detection range

	if (distance > detectionRange) { // Check if the player is too far
		return false;
	}

	Ray ray(currentPos, Vector::Normalise(direction));
	RayCollision collision;

	// Check if the ray hits the player
	if (gameWorld->Raycast(ray, collision, true)) {
		std::cout << "Collision node: " << collision.node << std::endl;
		Debug::DrawLine(currentPos, playerPos, Vector4(1, 0, 0, 1)); // Debugging line to visualize the ray
		if (collision.node == player) {
			std::cout << "collision exectued" << std::endl;

			static float elapsedTime = 0.0f;
			float dt = Window::GetWindow()->GetTimer().GetTimeDeltaSeconds();

			if (player->IsActive()) {
				player->SetActive(false); // Deactivate the player
				elapsedTime = 0.0f;       // Reset the timer
			}
			else if (!player->IsActive()) {
				elapsedTime += dt;
				std::cout << "Player detected! " << elapsedTime << std::endl;
				if (elapsedTime >= 0.8f) { // Check if 5 seconds have passed
					player->GetTransform().SetPosition(Vector3(0, 2, -30)); // Reset the player position
					player->SetActive(true); // Activate the player
					elapsedTime = 0.0f;      // Reset the timer for future use
				}
			}
			return true;
		}

	}
	return false;
}


void StateGameObject::Idle(float dt) {
	// Optional idle behavior, not interested in this for now
	std::cout << "Idle State" << std::endl;
}

void StateGameObject::MoveLeft(float dt) {
	GetPhysicsObject()->AddForce({ -50, 0, 0 });
	counter += dt;
}

void StateGameObject::MoveRight(float dt) {
	GetPhysicsObject()->AddForce({50, 0, 0 });
	counter -= dt;
}