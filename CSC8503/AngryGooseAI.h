#pragma once

namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class StateGameObject;

		class AngryGoose {
		public:
			~AngryGoose();

			AngryGoose(StateGameObject* gooseEnemy);

			void Update(float dt);

			void MoveAngryGooseAI(StateGameObject* gooseEnemy);
		};
	}
}