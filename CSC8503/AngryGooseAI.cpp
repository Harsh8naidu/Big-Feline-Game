#include "AngryGooseAI.h"
#include <iostream>
#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"
#include "StateGameObject.h"

using namespace NCL;
using namespace CSC8503;

AngryGoose::AngryGoose(StateGameObject* gooseEnemy)
{
	MoveAngryGooseAI(gooseEnemy);

}

void AngryGoose::Update(float dt)
{
}

void AngryGoose::MoveAngryGooseAI(StateGameObject* gooseEnemy)
{
	StateMachine* angryGooseAI = new StateMachine();

	State* angryGooseState = new State([&](float dt) {
		std::cout << "Angry Goose is moving" << std::endl;
		});

	angryGooseAI->AddState(angryGooseState);
	State* angryGooseState2 = new State([&](float dt) {
		std::cout << "Angry Goose is attacking" << std::endl;
		});

	angryGooseAI->AddState(angryGooseState2);
	StateTransition* angryGooseTransition = new StateTransition(angryGooseState, angryGooseState2, [&]() {
		return true;
		});

	angryGooseAI->AddTransition(angryGooseTransition);

	angryGooseAI->Update(1.0f);
}


