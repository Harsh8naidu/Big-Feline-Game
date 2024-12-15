#include "PhysicsObject.h"
#include "PhysicsSystem.h"
#include "Transform.h"
using namespace NCL;
using namespace CSC8503;

PhysicsObject::PhysicsObject(Transform* parentTransform, const CollisionVolume* parentVolume)	{
	transform	= parentTransform;
	volume		= parentVolume;

	inverseMass = 1.0f;
	elasticity	= 0.8f;
	friction	= 0.8f;
}

PhysicsObject::~PhysicsObject()	{

}

void PhysicsObject::SetElasticity(float newElasticity) {
	elasticity = newElasticity;
}


void PhysicsObject::ApplyAngularImpulse(const Vector3& force) {
	angularVelocity += inverseInteriaTensor * force;
}

void PhysicsObject::ApplyLinearImpulse(const Vector3& force) {
	linearVelocity += force * inverseMass;
}

void PhysicsObject::AddForce(const Vector3& addedForce) {
	force += addedForce;
}

void PhysicsObject::StopForce(const Vector3& addedForce) {
	force = addedForce;
}

void PhysicsObject::AddForceAtPosition(const Vector3& addedForce, const Vector3& position) {
	Vector3 localPos = position - transform->GetPosition();

	force  += addedForce;
	torque += Vector::Cross(localPos, addedForce);
}

void PhysicsObject::AddTorque(const Vector3& addedTorque) {
	torque += addedTorque;
}

void PhysicsObject::ClearForces() {
	force				= Vector3();
	torque				= Vector3();
}

void PhysicsObject::InitCubeInertia() {
	Vector3 dimensions	= transform->GetScale();

	Vector3 fullWidth = dimensions * 2.0f;

	Vector3 dimsSqr		= fullWidth * fullWidth;

	inverseInertia.x = (12.0f * inverseMass) / (dimsSqr.y + dimsSqr.z);
	inverseInertia.y = (12.0f * inverseMass) / (dimsSqr.x + dimsSqr.z);
	inverseInertia.z = (12.0f * inverseMass) / (dimsSqr.x + dimsSqr.y);
}

void PhysicsObject::InitSphereInertia() {

	float radius	= Vector::GetMaxElement(transform->GetScale());
	float i			= 2.5f * inverseMass / (radius*radius);

	inverseInertia	= Vector3(i, i, i);
}

void PhysicsObject::InitHollowSphereInertia() {
	float radius = Vector::GetMaxElement(transform->GetScale());
	float i = (3.0f / 2.0f) * inverseMass / (radius * radius);

	// Since it's a hollow sphere, the inertia tensor is uniform
	inverseInertia = Vector3(i, i, i);
}

void PhysicsObject::InitCapsuleInertia() {
	float radius = transform->GetScale().x; // Assuming uniform scaling for radius
	float halfHeight = transform->GetScale().y; // Half the height of the cylindrical part

	float cylinderMass = inverseMass * (halfHeight / (halfHeight + (4.0f / 3.0f) * radius));
	float sphereMass = inverseMass - cylinderMass; // Remaining mass for the hemispheres

	// Cylinder inertia components
	float cylinderIxx = (1.0f / 12.0f) * cylinderMass * (3 * radius * radius + 4 * halfHeight * halfHeight);
	float cylinderIyy = (0.5f) * cylinderMass * radius * radius;
	float cylinderIzz = cylinderIxx;

	// Sphere inertia components (treating as a full sphere, then halving)
	float sphereI = (2.0f / 5.0f) * sphereMass * radius * radius; // Full sphere inertia
	sphereI *= 0.5f; // Half for hemispheres

	// Combine contributions from cylinder and hemispheres
	inverseInertia.x = 1.0f / (cylinderIxx + sphereI);
	inverseInertia.y = 1.0f / (cylinderIyy + sphereI);
	inverseInertia.z = 1.0f / (cylinderIzz + sphereI);
}


void PhysicsObject::UpdateInertiaTensor() {
	Quaternion q = transform->GetOrientation();

	Matrix3 invOrientation = Quaternion::RotationMatrix<Matrix3>(q.Conjugate());
	Matrix3 orientation = Quaternion::RotationMatrix<Matrix3>(q);

	inverseInteriaTensor = orientation * Matrix::Scale3x3(inverseInertia) *invOrientation;
}