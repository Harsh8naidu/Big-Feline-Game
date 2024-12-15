#include "../NCLCoreClasses/KeyboardMouseController.h"

#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"

#include "StateGameObject.h"
#include "NavigationGrid.h"
#include <vector>
//#include "KittenStateObject.h"

namespace NCL {
	namespace CSC8503 {
		class NetworkPlayer;
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

			void GenerateMaze(NavigationGrid& grid);

			//KittenStateObject* AddKittensT//oWorld(const Vector3& position);

			void AddFlyingStairs();

			const std::vector<Vector3>& GetWallPositions() const {
				return wallPositions;
			}

			StateGameObject* AddAngryGooseToWorld(const Vector3& position, const std::vector<Vector3>& path, GameObject* player, GameWorld* world);

			

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);

			void InitDefaultFloor();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, bool isHollow = true);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);

			GameObject* AddOBBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);

			

			NetworkPlayer* AddPlayerToWorld(const Vector3& position, std::string playerName);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			void BridgeConstraintTest();

			StateGameObject* AddStateObjectToWorld(const Vector3& position);

			
			StateGameObject* testStateObject;
			StateGameObject* angryGoose;

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			KeyboardMouseController controller;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			Mesh*	capsuleMesh = nullptr;
			Mesh*	cubeMesh	= nullptr;
			Mesh*	sphereMesh	= nullptr;

			Texture*	basicTex	= nullptr;
			Shader*		basicShader = nullptr;

			Texture* catTex = nullptr;

			Texture* planeTex = nullptr;

			Texture* capsuleTex = nullptr;

			Texture* bonusTex = nullptr;

			//Coursework Meshes
			Mesh*	catMesh		= nullptr;
			Mesh*	kittenMesh	= nullptr;
			Mesh*	enemyMesh	= nullptr;
			Mesh*	bonusMesh	= nullptr;

			Mesh* planeMesh = nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 4, -10);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;

			NetworkPlayer* player = nullptr; // Player object cat

			NetworkPlayer* player2 = nullptr;

			GameObject* bonus1 = nullptr;
			GameObject* bonus2 = nullptr;
			GameObject* bonus3 = nullptr;
			GameObject* bonus4 = nullptr;
			GameObject* bonus5 = nullptr;
			GameObject* bonus6 = nullptr;

			GameObject* capsule = nullptr;
			

			GameObject* plane = nullptr;
			GameObject* plane2 = nullptr;
			GameObject* door = nullptr;
			GameObject* AddDoorToWorld(const Vector3& position);
			GameObject* AddBlackObstacleToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);

			GameObject* AddPlaneToWorld(const Vector3& position, const Vector3& scale);

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass);

			//KittenStateObject* kitten1 = nullptr;

			bool isCameraLocked = false; // Toggle camera lock
			bool isRotatingAroundObject = false;
			float rotationAngle = 0.0f; // Angle in radians
			float rotationSpeed = 1.0f; // Rotation speed
			float distanceFromCat = 10.0f; // Distance from the cat


			std::vector<Vector3> wallPositions;

			Vector3 lockedAngle = Vector3(0, 0, 0);

			bool isGameStart = false;
			bool isGamePaused = false;

			float gameTimer = 0.0f; // Track elapsed time
			bool isGameEnd = false;

			int score = 0;
		};
	}
}

