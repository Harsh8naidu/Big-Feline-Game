#pragma once
#include "Transform.h"
#include "CollisionVolume.h"

// Header included later
#include "Layer.h"
#include "Debug.h"
#include <string>

using std::vector;

namespace NCL::CSC8503 {
	class NetworkObject;
	class RenderObject;
	class PhysicsObject;

	class GameObject	{
	public:
		GameObject(const std::string& name = "");
		~GameObject();

		// Getter and setter for layer
		void SetLayer(Layer layer) {
			this->layer = static_cast<int>(layer); // Convert to int using static_cast
		}

		Layer GetLayer() const {
			return static_cast<Layer>(layer);
		}

		void SetBoundingVolume(CollisionVolume* vol) {
			boundingVolume = vol;
		}

		const CollisionVolume* GetBoundingVolume() const {
			return boundingVolume;
		}

		bool IsActive() const {
			return isActive;
		}

		Transform& GetTransform() {
			return transform;
		}

		RenderObject* GetRenderObject() const {
			return renderObject;
		}

		PhysicsObject* GetPhysicsObject() const {
			return physicsObject;
		}

		NetworkObject* GetNetworkObject() const {
			return networkObject;
		}

		void SetRenderObject(RenderObject* newObject) {
			renderObject = newObject;
		}

		void SetPhysicsObject(PhysicsObject* newObject) {
			physicsObject = newObject;
		}

		void SetActive(bool active) {
			isActive = active;
		}

		const std::string& GetName() const {
			return name;
		}

		int GetScore() const {
			return score;
		}

		void SetCollected(bool state) { collected = state; }
		bool IsCollected() const { return collected; }

		virtual void OnCollisionBegin(GameObject* otherObject) {
			//std::cout << "OnCollisionBegin event occured!\n";
			if (name == "player" && otherObject->GetName() == "bonus") {
				if (!otherObject->IsCollected()) {
					otherObject->SetCollected(true); // Mark it as collected
					otherObject->SetActive(false);  // Deactivate the object
					std::cout << "Score: " << ++score << std::endl;
				}
			}
		}

		virtual void OnCollisionEnd(GameObject* otherObject) {
			//std::cout << "OnCollisionEnd event occured!\n";
		}

		bool GetBroadphaseAABB(Vector3&outsize) const;

		void UpdateBroadphaseAABB();

		void SetWorldID(int newID) {
			worldID = newID;
		}

		int		GetWorldID() const {
			return worldID;
		}

		virtual void GameObjectUpdate(float dt);

	protected:
		Transform			transform;

		CollisionVolume*	boundingVolume;
		PhysicsObject*		physicsObject;
		RenderObject*		renderObject;
		NetworkObject*		networkObject;

		bool		isActive;
		int			worldID;
		std::string	name;

		Vector3 broadphaseAABB;

		int layer;
		int score;
		bool collected = false;
	};
}

