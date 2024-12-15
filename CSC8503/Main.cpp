#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

#include "TestPacketReceiver.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>

void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void {
			std::cout << "In State A!" << std::endl;
			data++;
		}
	);

	State* B = new State([&](float dt)->void {
		std::cout << "In State B!" << std::endl;
		data--;
		}
	);

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool {
		return data > 10;
	});

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool {
		return data < 0;
	});

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}
}



class PauseScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::U)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Press U to unpause game" << std::endl;
	}
};

class GameScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		pauseReminder -= dt;
		if (pauseReminder < 0.0f) {
			std::cout << "Coins mined: " << coinsMined << std::endl;
			std::cout << "Press P to pause game, or F1 to return to main menu" << std::endl;
			pauseReminder += 1.0f;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::P)) {
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
			std::cout << "Returning to main menu..." << std::endl;
			return PushdownResult::Pop;
		}
		if (rand() % 7 == 0) {
			coinsMined++;
		}
		return PushdownResult::NoChange;
	};

	void OnAwake() override {
		std::cout << "Preparing to mine coins!" << std::endl;
	}
protected:
	int coinsMined = 0;
	float pauseReminder = 1.0f;
};

class IntroScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
			*newState = new GameScreen();
			std::cout << "Starting game!" << std::endl;
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			std::cout << "Quitting game!" << std::endl;
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	};
	void OnAwake() override {
		std::cout << "Welcome to a really awesome game!" << std::endl;
		std::cout << "Press SPACE to start, or ESC to quit" << std::endl;
	}
};

void TestPushdownAutomata(Window* w) {
	PushdownMachine machine(new IntroScreen());
	while (w->UpdateWindow()) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (!machine.Update(dt)) {
			return;
		}
	}
}


void RunGame(Window* w, bool isServer) {
	NetworkBase::Initialise();

	NetworkedGame* game = new NetworkedGame(isServer);

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue;
		}

		game->UpdateGame(dt);

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			break;
		}
	}

	delete game;
	NetworkBase::Destroy();
	std::cout << "Client shut down successfully." << std::endl;
}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/

int main(int argc, char** argv) {
	WindowInitialisation initInfo;
	initInfo.width		= 2560;
	initInfo.height		= 1440;
	initInfo.windowTitle = "CSC8503 Game technology!";

	Window*w = Window::CreateGameWindow(initInfo);

	//TestPushdownAutomata(w);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	// Determine mode based on command line arguments
	bool isServer = argc != 1;

	if (isServer) {
		std::cout << "Running as server" << std::endl;
		//RunServer();
	}
	else {
		std::cout << "Running as client" << std::endl;
		//RunClient(w);
	}
	RunGame(w, isServer);

	//NetworkedGame* g = new NetworkedGame(isServer);
	//w->GetTimer().GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!

	//TestPathfinding();
	//TestBehaviourTree();

	//while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
	//	float dt = w->GetTimer().GetTimeDeltaSeconds();
	//	if (dt > 0.1f) {
	//		std::cout << "Skipping large time delta" << std::endl;
	//		continue; //must have hit a breakpoint or something to have a 1 second frame time!
	//	}
	//	if (Window::GetKeyboard()->KeyPressed(KeyCodes::PRIOR)) {
	//		w->ShowConsole(true);
	//	}
	//	if (Window::GetKeyboard()->KeyPressed(KeyCodes::NEXT)) {
	//		w->ShowConsole(false);
	//	}

	//	if (Window::GetKeyboard()->KeyPressed(KeyCodes::T)) {
	//		w->SetWindowPosition(0, 0);
	//	}

	//	//TestStateMachine();
	//	DisplayPathfinding();

	//	//TestNetworking();

	//	w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

	//	g->UpdateGame(dt);
	//}
	//Window::DestroyGameWindow();
}


