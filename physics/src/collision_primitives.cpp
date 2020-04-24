#include "../include/collision_primitives.h"

#include "../include/rigid_body.h"

#include <algorithm>
#include <stdlib.h>

dphysics::Collision::Collision() : ysObject("Collision") {
    m_penetration = 0.0f;
    m_normal = ysMath::Constants::Zero;
    m_body1 = nullptr;
    m_body2 = nullptr;

    m_collisionObject1 = nullptr;
    m_collisionObject2 = nullptr;

    m_sensor = false;

    m_dynamicFriction = 0.0f;
    m_staticFriction = 0.0f;
    m_restitution = 0.0f;
}

dphysics::Collision::Collision(Collision &collision) : ysObject("Collision") {
    m_penetration = collision.m_penetration;
    m_normal = collision.m_normal;
    m_position = collision.m_position;

    m_body1 = collision.m_body1;
    m_body2 = collision.m_body2;

    m_collisionObject1 = collision.m_collisionObject1;
    m_collisionObject2 = collision.m_collisionObject2;

    m_sensor = collision.m_sensor;

    m_relativePosition[0] = collision.m_relativePosition[0];
    m_relativePosition[1] = collision.m_relativePosition[1];

    m_dynamicFriction = collision.m_dynamicFriction;
    m_staticFriction = collision.m_staticFriction;
    m_restitution = collision.m_restitution;
}

dphysics::Collision::~Collision() {
    /* void */
}

void dphysics::Collision::UpdateInternals(float timestep) {
    CalculateContactSpace();

    for (int i = 0; i < 2; i++) {
        if (m_bodies[i] != nullptr) {
            m_relativePosition[i] = ysMath::Sub(m_position, m_bodies[i]->Transform.GetWorldPosition());
        }
    }

    m_contactVelocity = CalculateLocalVelocity(0, timestep);

    if (m_bodies[1] != nullptr) {
        m_contactVelocity = ysMath::Sub(m_contactVelocity, CalculateLocalVelocity(1, timestep));
    }

    CalculateDesiredDeltaVelocity(timestep);
}

dphysics::Collision &dphysics::Collision::operator=(dphysics::Collision &collision) {
    m_penetration = collision.m_penetration;
    m_normal = collision.m_normal;
    m_position = collision.m_position;

    m_body1 = collision.m_body1;
    m_body2 = collision.m_body2;

    m_collisionObject1 = collision.m_collisionObject1;
    m_collisionObject2 = collision.m_collisionObject2;

    m_sensor = collision.m_sensor;

    m_relativePosition[0] = collision.m_relativePosition[0];
    m_relativePosition[1] = collision.m_relativePosition[1];

    return *this;
}

bool dphysics::Collision::IsGhost() const {
    if (m_collisionObject1 != nullptr && m_collisionObject1->GetParent()->IsGhost()) return true;
    if (m_collisionObject2 != nullptr && m_collisionObject2->GetParent()->IsGhost()) return true;
    return false;
}

void dphysics::Collision::CalculateDesiredDeltaVelocity(float timestep) {
    const static float VelocityLimit = 0.25f;

    ///>NewVelocityCalculation
        // Calculate the acceleration induced velocity accumulated this frame
    //real velocityFromAcc = body[0]->getLastFrameAcceleration() * duration * contactNormal;
    float velocityFromAcc = ysMath::GetScalar(ysMath::Dot(m_bodies[0]->GetAcceleration(), m_normal)) * timestep;

    if (m_bodies[1] != nullptr) {
        velocityFromAcc -= ysMath::GetScalar(ysMath::Dot(m_bodies[1]->GetAcceleration(), m_normal)) * timestep;;
    }

    // If the velocity is very slow, limit the restitution
    float thisRestitution = m_restitution;
    if (std::abs(ysMath::GetX(m_contactVelocity)) < VelocityLimit) {
        thisRestitution = 0.0f;
    }

    // Combine the bounce velocity with the removed
    // acceleration velocity.
    m_desiredDeltaVelocity =
        -ysMath::GetX(m_contactVelocity)
        - thisRestitution * (ysMath::GetX(m_contactVelocity) - velocityFromAcc);
}

ysVector dphysics::Collision::CalculateLocalVelocity(int bodyIndex, float timestep) {
    RigidBody *body = m_bodies[bodyIndex];
    ysVector position = ysMath::Add(m_relativePosition[bodyIndex], body->Transform.GetWorldPosition());

    ysVector velocity = body->GetVelocityAtWorldPoint(position);
    ysVector contactVelocity = ysMath::MatMult(ysMath::OrthogonalInverse(m_contactSpace), velocity);

    return contactVelocity;
}

void dphysics::Collision::CalculateContactSpace() {
    ysVector contactTangent0;
    ysVector contactTangent1;

    if (std::abs(ysMath::GetX(m_normal)) > std::abs(ysMath::GetY(m_normal))) {
        contactTangent0 = ysMath::Normalize(
            ysMath::Cross(m_normal, ysMath::Constants::YAxis));
        contactTangent1 = ysMath::Cross(m_normal, contactTangent0);
    }
    else {
        contactTangent0 = ysMath::Normalize(
            ysMath::Cross(m_normal, ysMath::Constants::XAxis));
        contactTangent1 = ysMath::Cross(m_normal, contactTangent0);
    }

    m_contactSpace = ysMath::Transpose(ysMath::LoadMatrix(
        m_normal,
        contactTangent0,
        contactTangent1,
        ysMath::Constants::IdentityRow4
    ));
}

void dphysics::BoxPrimitive::GetBounds(ysVector &minPoint, ysVector &maxPoint) const {
    ysVector point1 = ysMath::Add(ysMath::QuatTransform(Orientation, ysMath::LoadVector(HalfWidth, HalfHeight)), Position);
    ysVector point2 = ysMath::Add(ysMath::QuatTransform(Orientation, ysMath::LoadVector(-HalfWidth, HalfHeight)), Position);
    ysVector point3 = ysMath::Add(ysMath::QuatTransform(Orientation, ysMath::LoadVector(HalfWidth, -HalfHeight)), Position);
    ysVector point4 = ysMath::Add(ysMath::QuatTransform(Orientation, ysMath::LoadVector(-HalfWidth, -HalfHeight)), Position);

    maxPoint = ysMath::ComponentMax(point1, point2);
    maxPoint = ysMath::ComponentMax(maxPoint, point3);
    maxPoint = ysMath::ComponentMax(maxPoint, point4);

    minPoint = ysMath::ComponentMin(point1, point2);
    minPoint = ysMath::ComponentMin(minPoint, point3);
    minPoint = ysMath::ComponentMin(minPoint, point4);
}

void dphysics::CirclePrimitive::GetBounds(ysVector &minPoint, ysVector &maxPoint) const {
    maxPoint = ysMath::Add(Position, ysMath::LoadVector(Radius, Radius));
    minPoint = ysMath::Add(Position, ysMath::LoadVector(-Radius, -Radius));
}