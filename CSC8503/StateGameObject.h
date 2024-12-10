#pragma once
#include "GameObject.h"

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
        class StateGameObject : public GameObject  {
        public:
            StateGameObject();
            ~StateGameObject();

            StateGameObject(const std::vector<Vector3>& path);

            virtual void Update(float dt);

            void MoveToWaypoint(float dt);

            void Idle(float dt);

        protected:
            void MoveLeft(float dt);
            void MoveRight(float dt);

            StateMachine* stateMachine;

            // Stuff to move the enemy AI
            float counter;
			int waypointIndex;
			std::vector<Vector3> waypoints;
        };
    }
}
