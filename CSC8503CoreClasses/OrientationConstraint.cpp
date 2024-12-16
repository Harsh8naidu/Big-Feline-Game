#include "OrientationConstraint.h"
#include "GameObject.h"
#include "PhysicsObject.h"
using namespace NCL;
using namespace Maths;
using namespace CSC8503;

OrientationConstraint::OrientationConstraint(GameObject* a, GameObject* b)
{
	objectA = a;
	objectB = b;
}

OrientationConstraint::~OrientationConstraint()
{

}

void OrientationConstraint::UpdateConstraint(float dt) {
	// Get the current orientations as quaternions
	Quaternion orientationA = objectA->GetTransform().GetOrientation();
	Quaternion orientationB = objectB->GetTransform().GetOrientation();

	// Compute the relative orientation between the two objects
	Quaternion relativeOrientation = orientationB.Conjugate() * orientationA;

	// Convert the relative orientation to an axis-angle representation
	Vector3 axis;
	float angle;
	Quaternion::QuaternionToAxisAngle(relativeOrientation, axis, angle);

	// If there's no significant angular difference, exit early
	if (abs(angle) < 0.01f) {
		return;
	}

	// Normalize the axis
	axis = Vector::Normalise(axis);

	// Get the physics objects
	PhysicsObject* physA = objectA->GetPhysicsObject();
	PhysicsObject* physB = objectB->GetPhysicsObject();

	// Compute the constraint mass (inverse mass of angular components)
	float constraintMass = physA->GetInverseInertia() + physB->GetInverseInertia();

	if (constraintMass > 0.0f) {
		// Compute the corrective angular velocity
		float biasFactor = 0.01f;
		float bias = -(biasFactor / dt) * angle;

		// Compute the corrective impulse scalar
		float lambda = bias / constraintMass;

		// Compute the corrective torques for each object
		Vector3 aTorque = axis * lambda;
		Vector3 bTorque = -axis * lambda;

		// Apply the torques
		physA->ApplyAngularImpulse(aTorque);
		physB->ApplyAngularImpulse(bTorque);
	}
}
