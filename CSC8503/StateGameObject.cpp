#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

StateGameObject::StateGameObject(const std::vector<Vector3>& path) {
	counter = 0.0f;
	waypointIndex = 0;
	waypoints = path;
	movingForward = true; // Start by moving forward along the path

	stateMachine = new StateMachine();

	/*State* stateA = new State([&](float dt)->void {
		this->MoveLeft(dt);
	});

	State* stateB = new State([&](float dt)->void {
		this->MoveRight(dt);
	});*/

	State* moveState = new State([&](float dt)->void {
		this->MoveToWaypoint(dt);
		});

	//State* idleState = new State([&](float dt)->void {
	//	// Idle state for when the path is complete
	//	this->Idle(dt);
	//	});

	stateMachine->AddState(moveState);
	//stateMachine->AddState(idleState);

	/*stateMachine->AddTransition(new StateTransition(moveState, idleState, [&]()->bool {
		return waypointIndex >= waypoints.size();
	}));*/

	stateMachine->AddTransition(new StateTransition(moveState, moveState, [&]()->bool {
		// Switch direction when we reach the end or the start
		return waypointIndex >= waypoints.size() || waypointIndex < 0;
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