#pragma once
#include "GameObject.h"

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
		class GameWorld;
		class GameObject;
		class NetworkPlayer;
        class StateGameObject : public GameObject  {
        public:
            StateGameObject();
            ~StateGameObject();

            StateGameObject(const std::vector<Vector3>& path, GameObject* playerObj, GameWorld* world);

            virtual void Update(float dt);

            void MoveToWaypoint(float dt);

            void ChasePlayer(float dt);

            bool DetectPlayer();

            void TogglePlayerWithDelay(NetworkPlayer* player);

            void Idle(float dt);

        protected:
            void MoveLeft(float dt);
            void MoveRight(float dt);

            StateMachine* stateMachine;

            // Stuff to move the enemy AI
            float counter;
			int waypointIndex;
			std::vector<Vector3> waypoints;
			bool movingForward;

			GameObject* player;
			GameWorld* gameWorld;

            
        };
    }
}
