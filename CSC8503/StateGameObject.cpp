#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include <GameWorld.h>

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

StateGameObject::StateGameObject()
{
}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	stateMachine->Update(dt);
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
				movingForward = false; // Reverse direction
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

	GetPhysicsObject()->AddForce(direction * 80.0f); // Faster speed when chasing
}

bool StateGameObject::DetectPlayer() {
	

	if (!player || !gameWorld) return false;

	Vector3 currentPos = GetTransform().GetPosition();
	Vector3 playerPos = player->GetTransform().GetPosition();
	Vector3 direction = playerPos - currentPos;

	Ray ray(currentPos, Vector::Normalise(direction));
	RayCollision collision;

	// Check if the ray hits the player
	if (gameWorld->Raycast(ray, collision, true)) {
		
		if (collision.node == player) {
			std::cout << "Detecting player" << std::endl;
			Debug::DrawLine(currentPos, playerPos, Vector4(1, 0, 0, 1)); // Debugging line to visualize the ray
			player->SetActive(false); // Deactivate the player
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