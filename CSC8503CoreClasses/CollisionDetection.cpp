#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "Window.h"
#include "Maths.h"
#include "Debug.h"

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray&r, const Plane&p, RayCollision& collisions) {
	float ln = Vector::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}
	
	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r,GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume	= object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
		case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume	, collision); break;
		case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume	, collision); break;
		case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume	, collision); break;

		case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray&r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1, -1, -1); // t values for each axis, if the intersection is behind the ray origin, the t value is negative

	for (int i = 0; i < 3; ++i) { // get best 3 intersections
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}

	float bestT = Vector::GetMaxElement(tVals);

	if (bestT < 0.0) {
		return false; //no backwards rays!
	}

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f; // small value to prevent floating point errors
	for (int i = 0; i < 3; ++i) {
		if (intersection[i] + epsilon < boxMin[i] || intersection[i] - epsilon > boxMax[i]) {
			return false; // intersection is outside the box
		}
	}

	collision.collidedAt = intersection;
	collision.rayDistance = bestT;

	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray&r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();

	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray&r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	Matrix3 transform = Quaternion::RotationMatrix<Matrix3>(orientation);
	Matrix3 invTransform = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate()); // undo the rotation using the conjugate

	Vector3 localRayPos = r.GetPosition() - position; // convert the ray position to the local space of the OBB

	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection()); // convert the ray to the local space of the OBB

	bool collided = RayBoxIntersection(tempRay, Vector3(0, 0, 0), volume.GetHalfDimensions(), collision);

	if (collided) {
		collision.collidedAt = transform * collision.collidedAt + position; // convert the collision point back to world space
	}

	return collided;
}

bool CollisionDetection::RaySphereIntersection(const Ray&r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	// Get the direction from the ray's position to the sphere's position
	Vector3 dir = (spherePos - r.GetPosition());

	// Project the sphere's position onto the ray direction vector
	float sphereProj = Vector::Dot(dir, r.GetDirection());

	if (sphereProj < 0.0f) {
		return false; // Point is behind the ray
	}

	// Get the closest point on the ray to the sphere
	Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);

	// Get the distance from the sphere to the closest point on the ray
	float sphereDist = Vector::Length(point - spherePos);

	if (sphereDist > sphereRadius) {
		return false;
	}

	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));

	collision.rayDistance = sphereProj - (offset);
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);

	return true;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	return false;
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	//Two AABBs
	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Spheres
	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	//Two OBBs
	if (pairType == VolumeType::OBB) {
		return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Capsules

	//AABB vs Sphere pairs
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//OBB vs sphere pairs
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//Capsule vs other interactions
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((CapsuleVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	if (volB->type == VolumeType::Capsule && volA->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((CapsuleVolume&)*volB, transformB, (AABBVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Plane && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return PlaneAABBIntersection((Plane&)*volA, (AABBVolume&)*volB, transformB, collisionInfo);
	}

	if (volA->type == VolumeType::Plane && volB->type == VolumeType::Sphere) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return PlaneSphereIntersection((Plane&)*volA, (SphereVolume&)*volB, transformB, collisionInfo);
	}

	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Plane) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return PlaneSphereIntersection((Plane&)*volB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	return false;
}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = halfSizeA + halfSizeB;

	if (abs(delta.x) < totalSize.x &&
		abs(delta.y) < totalSize.y &&
		abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

bool CollisionDetection::PlaneSphereIntersection(const Plane& plane, const SphereVolume& volume, const Transform& worldTransform, CollisionInfo& collisionInfo) {
	// Get the sphere's position and radius
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	// Calculate the signed distance from the sphere center to the plane
	float sphereToPlane = Vector::Dot(spherePos, plane.GetNormal()) - plane.GetDistance();

	// Check if the sphere intersects with the plane
	if (std::abs(sphereToPlane) <= sphereRadius) {
		// Compute the collision normal and penetration depth
		Vector3 collisionNormal = plane.GetNormal();
		float penetrationDepth = sphereRadius - std::abs(sphereToPlane);

		// Assume the contact point is the sphere center projected onto the plane
		Vector3 localA = -collisionNormal * sphereToPlane;
		Vector3 localB = localA; // Contact is the same point relative to the sphere

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetrationDepth);
		return true;
	}

	return false; // No collision
}

bool CollisionDetection::PlaneAABBIntersection(const Plane& plane, const AABBVolume& volume, const Transform& worldTransform, CollisionInfo& collisionInfo) {
	// Get the AABB's position and half-dimensions
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxHalfSize = volume.GetHalfDimensions();

	// Project the AABB's half-dimensions onto the plane's normal
	float r = boxHalfSize.x * std::abs(plane.GetNormal().x) +
		boxHalfSize.y * std::abs(plane.GetNormal().y) +
		boxHalfSize.z * std::abs(plane.GetNormal().z);

	// Calculate the signed distance from the box center to the plane
	float boxToPlane = Vector::Dot(boxPos, plane.GetNormal()) - plane.GetDistance();

	// Check if the AABB intersects with the plane
	if (std::abs(boxToPlane) <= r) {
		// Compute the collision normal and penetration depth
		Vector3 collisionNormal = plane.GetNormal();
		float penetrationDepth = r - std::abs(boxToPlane);

		// Assume the contact point is the box center projected onto the plane
		Vector3 localA = boxPos - collisionNormal * boxToPlane;
		Vector3 localB = localA; // Contact is the same point relative to the AABB

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetrationDepth);
		return true;
	}

	return false; // No collision
}


//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();

	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);

	if (overlap) {
		static const Vector3 faces[6] = {
			Vector3(-1, 0, 0), Vector3(1, 0, 0),
			Vector3(0, -1, 0), Vector3(0, 1, 0),
			Vector3(0, 0, -1), Vector3(0, 0, 1)
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;

		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] = {
			(maxB.x - minA.x), //distance of box B to the left of box A
			(maxA.x - minB.x), //distance of box B to the right of box A
			(maxB.y - minA.y), //distance of box B below box A
			(maxA.y - minB.y), //distance of box B above box A
			(maxB.z - minA.z), //distance of box B behind box A
			(maxA.z - minB.z)  //distance of box B in front of box A
		};

		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; ++i) {
			if (distances[i] < 0) {
				return false; //no collision!
			}
			else if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		return true;
	}

	return false;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	
	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	float deltaLength = Vector::Length(delta);

	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);
		Vector3 normal = Vector::Normalise(delta);
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true; //We have a collision!
	}
	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeA.GetHalfDimensions();

	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	Vector3 closestPointOnBox = Vector::Clamp(delta, -boxSize, boxSize);

	Vector3 localPoint = delta - closestPointOnBox;
	float distance = Vector::Length(localPoint);

	if (distance < volumeB.GetRadius()) { // We have a collision!
		Vector3 collisionNormal = Vector::Normalise(localPoint);
		float penetration = (volumeB.GetRadius() - distance);

		Vector3 localA = Vector3();
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}
	return false;
}

bool  CollisionDetection::OBBSphereIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	// Get the center of the OBB and the sphere
	Vector3 obbCenter = worldTransformA.GetPosition();
	Vector3 sphereCenter = worldTransformB.GetPosition();

	// Get the orientation and half-dimensions of the OBB
	Matrix3 obbOrientation = worldTransformA.GetOrientation().ToMatrix3();
	Vector3 obbHalfDimensions = volumeA.GetHalfDimensions();

	// Transform the sphere's center into the OBB's local space
	Vector3 localSphereCenter = Matrix::Transpose(obbOrientation) * (sphereCenter - obbCenter);

	// Clamp the sphere center to the OBB's local extents to find the closest point
	Vector3 closestPoint = Vector::Clamp(localSphereCenter, -obbHalfDimensions, obbHalfDimensions);

	// Compute the vector from the closest point to the sphere center
	Vector3 delta = localSphereCenter - closestPoint;

	// Calculate the squared distance and compare it to the squared sphere radius
	float distanceSquared = Vector::LengthSquared(delta);
	float sphereRadius = volumeB.GetRadius();

	if (distanceSquared < sphereRadius * sphereRadius) {
		float distance = sqrt(distanceSquared);
		Vector3 collisionNormal = (distance > 0.0f) ? Vector::Normalise(delta) : Vector3(1, 0, 0); // Default normal in case of overlap
		float penetration = sphereRadius - distance;

		// Calculate local contact points for the collision
		Vector3 localA = closestPoint;
		Vector3 localB = sphereCenter - collisionNormal * sphereRadius;

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}

	return false;
}

bool CollisionDetection::AABBCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxHalfSize = volumeB.GetHalfDimensions();
	Vector3 boxCenter = worldTransformB.GetPosition();
	Vector3 capsuleStart = worldTransformA.GetPosition() - Vector3(0, volumeA.GetHalfHeight(), 0);
	Vector3 capsuleEnd = worldTransformA.GetPosition() + Vector3(0, volumeA.GetHalfHeight(), 0);

	// Find the closest point on the capsule line segment to the AABB center
	Vector3 closestPoint = Vector::ClosestPointOnLineSegment(boxCenter, capsuleStart, capsuleEnd);

	// Clamp the closest point to within the AABB
	Vector3 delta = closestPoint - boxCenter;
	Vector3 clampedPoint = Vector::Clamp(delta, -boxHalfSize, boxHalfSize);

	// Compute the distance between the capsule surface and the clamped point
	Vector3 penetrationVector = closestPoint - (boxCenter + clampedPoint);
	float distanceSquared = Vector::LengthSquared(penetrationVector);
	float capsuleRadius = volumeA.GetRadius();

	if (distanceSquared < capsuleRadius * capsuleRadius) {
		float distance = sqrt(distanceSquared);
		Vector3 collisionNormal = Vector::Normalise(penetrationVector);
		float penetrationDepth = capsuleRadius - distance;

		Vector3 localA = closestPoint - worldTransformA.GetPosition();
		Vector3 localB = boxCenter + clampedPoint - worldTransformB.GetPosition();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetrationDepth);
		return true;
	}

	return false;
}

bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 capsuleStart = worldTransformA.GetPosition() - Vector3(0, volumeA.GetHalfHeight(), 0);
	Vector3 capsuleEnd = worldTransformA.GetPosition() + Vector3(0, volumeA.GetHalfHeight(), 0);

	Vector3 sphereCenter = worldTransformB.GetPosition();
	
	// Find the closest point on the capsule line segment to the sphere center
	Vector3 closestPoint = Vector::ClosestPointOnLineSegment(sphereCenter, capsuleStart, capsuleEnd);

	// Compute the distance between the sphere center and the closest point
	Vector3 penetrationVector = sphereCenter - closestPoint;
	float distanceSquared = Vector::LengthSquared(penetrationVector);

	float combinedRadius = volumeA.GetRadius() + volumeB.GetRadius();

	if (distanceSquared < combinedRadius * combinedRadius) {
		float distance = sqrt(distanceSquared);
		Vector3 collisionNormal = Vector::Normalise(penetrationVector);
		float penetrationDepth = combinedRadius - distance;

		Vector3 localA = closestPoint - worldTransformA.GetPosition();
		Vector3 localB = sphereCenter - worldTransformB.GetPosition();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetrationDepth);
		return true;
	}


	return false;
}

bool CollisionDetection::OBBIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
}

Matrix4 GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix::Translation(position) *
		Matrix::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	float negDepth = nearPlane - farPlane;

	float invNegDepth = negDepth / (2 * (farPlane * nearPlane));

	Matrix4 m;

	float h = 1.0f / tan(fov*PI_OVER_360);

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = invNegDepth;//// +PI_OVER_360;
	m.array[3][2] = -1.0f;
	m.array[3][3] = (0.5f / nearPlane) + (0.5f / farPlane);

	return m;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const PerspectiveCamera& cam) {
	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = Window::GetWindow()->GetScreenAspect();
	float fov		= cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane  = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	Matrix4 proj  = cam.BuildProjectionMatrix(aspect);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const PerspectiveCamera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2i screenSize	= Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c = Vector::Normalise(c);

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov*PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f*(nearPlane*farPlane) / neg_depth;

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = 1.0f / d;

	m.array[3][2] = 1.0f / e;
	m.array[3][3] = -c / (d * e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix::Translation(position) *
		Matrix::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const PerspectiveCamera& c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());


	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

