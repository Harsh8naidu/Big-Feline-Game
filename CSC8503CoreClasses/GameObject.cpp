#include "GameObject.h"
#include "CollisionDetection.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "NetworkObject.h"
#include "Maths.h"

using namespace NCL::CSC8503;

GameObject::GameObject(const std::string& objectName)	{
	name			= objectName;
	worldID			= -1;
	isActive		= true;
	boundingVolume	= nullptr;
	physicsObject	= nullptr;
	renderObject	= nullptr;
	networkObject	= nullptr;
}

GameObject::~GameObject()	{
	delete boundingVolume;
	delete physicsObject;
	delete renderObject;
	delete networkObject;
}

bool GameObject::GetBroadphaseAABB(Vector3&outSize) const {
	if (!boundingVolume) {
		return false;
	}
	outSize = broadphaseAABB;
	return true;
}

void GameObject::UpdateBroadphaseAABB() {
	if (!boundingVolume) {
		return;
	}
	if (boundingVolume->type == VolumeType::AABB) {
		broadphaseAABB = ((AABBVolume&)*boundingVolume).GetHalfDimensions();
	}
	else if (boundingVolume->type == VolumeType::Sphere) {
		float r = ((SphereVolume&)*boundingVolume).GetRadius();
		broadphaseAABB = Vector3(r, r, r);
	}
	else if (boundingVolume->type == VolumeType::OBB) {
		Matrix3 mat = Quaternion::RotationMatrix<Matrix3>(transform.GetOrientation());
		mat = Matrix::Absolute(mat);
		Vector3 halfSizes = ((OBBVolume&)*boundingVolume).GetHalfDimensions();
		broadphaseAABB = mat * halfSizes;
	}
}

/* This would allow you to write subclasses to encapsulate any
of the object construction or logic required for your game,
whether it uses state machines, or its own custom logic just
within its overridden Update method. */

void NCL::CSC8503::GameObject::GameObjectUpdate(float dt)
{
	if (this->GetName() == "bonus") {
		Quaternion currentOrientation = this->GetTransform().GetOrientation();
		
		float rotationSpeed = 3000.0f;
		float angle = rotationSpeed * dt;

		Quaternion yRotation = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::DegreesToRadians(angle));
			
		currentOrientation = currentOrientation * yRotation;

		currentOrientation.Normalise();

		this->GetTransform().SetOrientation(currentOrientation);
	}

}
