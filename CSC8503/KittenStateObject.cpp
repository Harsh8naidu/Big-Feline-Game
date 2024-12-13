//#include "KittenStateObject.h"
//#include "PhysicsObject.h"
//
//using namespace NCL;
//using namespace CSC8503;
//
//KittenStateObject::KittenStateObject() : player(nullptr) {}
//
//KittenStateObject::KittenStateObject(GameObject* catObj) : player(catObj) {}
//
//KittenStateObject::~KittenStateObject() {}
//
//void KittenStateObject::Update(float dt) {
//    if (player) {
//        FollowCat(dt); // Make the kitten follow the cat
//    }
//}
//
//void KittenStateObject::FollowCat(float dt) {
//    if (!player) return;
//
//    Vector3 kittenPos = GetTransform().GetPosition();
//    Vector3 catPos = player->GetTransform().GetPosition();
//
//    Vector3 direction = catPos - kittenPos;
//    float distance = Vector::Length(direction);
//
//    // Stop if the kitten is already close to the cat
//    if (distance < 2.0f) { // Adjust as needed
//        GetPhysicsObject()->ClearForces(); // Prevent overshooting
//        return;
//    }
//
//    Vector::Normalise(direction);
//
//    float followSpeed = 3.0f; // Adjust speed as required
//    GetPhysicsObject()->AddForce(direction * followSpeed);
//}