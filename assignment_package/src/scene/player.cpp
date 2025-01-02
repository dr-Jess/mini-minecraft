#include "player.h"
#include <QString>

const glm::vec3 Player::corners[12] = {
    0.9f * glm::vec3(-0.5f, 0.f, -0.5f),
    0.9f * glm::vec3(-0.5f, 0.f, 0.5f),
   0.9f *  glm::vec3(-0.5f, 2.0f, -0.5f),
    0.9f * glm::vec3(-0.5f, 2.0f, 0.5f),
    0.9f * glm::vec3(-0.5f, 1.0f, -0.5f),
    0.9f * glm::vec3(-0.5f, 1.0f, 0.5f),
   0.9f *  glm::vec3(0.5f, 0.f, -0.5f),
    0.9f * glm::vec3(0.5f, 0.f, 0.5f),
   0.9f *  glm::vec3(0.5f, 2.0f, -0.5f),
   0.9f *  glm::vec3(0.5f, 2.0f, 0.5f),
    0.9f * glm::vec3(0.5f, 1.0f, -0.5f),
    0.9f * glm::vec3(0.5f, 1.0f, 0.5f)
};

Player::Player(glm::vec3 pos, Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera), m_movementMode(MovementMode::FLYING),
      m_mouseSensitivity(0.1f)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.
    // first, update look direction

    // clamp up and down look
    // compute theoretical up/down angle

    float pitch = glm::degrees(glm::asin(m_forward.y));
    if (pitch + inputs.mouseY * m_mouseSensitivity > 90.f) {
        this->rotateOnRightLocal(89.9f - pitch);
    } else if (pitch + inputs.mouseY * m_mouseSensitivity < -90.f) {
        this->rotateOnRightLocal(-89.9f - pitch);
    } else {
        this->rotateOnRightLocal(inputs.mouseY * m_mouseSensitivity);
    }
    this->rotateOnUpGlobal(inputs.mouseX * m_mouseSensitivity);

    this->m_acceleration = glm::vec3(0,0,0);
    // update acceleration
    switch (this->m_movementMode) {
        case MovementMode::WALKING:
        {
            bool grounded = isGrounded();
            bool submerged = isSubmerged();
            if (inputs.wPressed) {
                this->m_acceleration += glm::normalize(glm::vec3(this->m_forward.x, 0, this->m_forward.z));
            }
            if (inputs.sPressed) {
                this->m_acceleration -= glm::normalize(glm::vec3(this->m_forward.x, 0, this->m_forward.z));
            }
            if (inputs.dPressed) {
                this->m_acceleration += this->m_right;
            }
            if (inputs.aPressed) {
                this->m_acceleration -= this->m_right;
            }
            if (glm::length(this->m_acceleration) > 0) {
                this->m_acceleration = glm::normalize(this->m_acceleration);
                this->m_acceleration *= grounded ? 50.f : 15.f;
            }
            if (submerged) {
                this->m_acceleration *= 0.33f;
                if (inputs.spacePressed) {
                    this->m_velocity.y = 3.f;
                }
            } else {
                if (inputs.spacePressed && grounded) {
                    this->m_velocity.y = 9.f;
                }
            }
            this->m_acceleration.y = submerged ? -8.f : -25.f;
            break;
        }
        case MovementMode::FLYING:
            if (inputs.wPressed) {
                this->m_acceleration += this->m_forward;
            }
            if (inputs.sPressed) {
                this->m_acceleration -= this->m_forward;
            }
            if (inputs.dPressed) {
                this->m_acceleration += this->m_right;
            }
            if (inputs.aPressed) {
                this->m_acceleration -= this->m_right;
            }
            if (inputs.qPressed) {
                this->m_acceleration -= this->m_up;
            }
            if (inputs.ePressed) {
                this->m_acceleration += this->m_up;
            }
            if (glm::length(this->m_acceleration) > 0) {
                this->m_acceleration = glm::normalize(this->m_acceleration);
                this->m_acceleration *= 30.f;
            }
    }
}

void Player::move(glm::vec3 pos) {
    this->m_position = pos;
}


void Player::computePhysics(float dT, Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    switch (this->m_movementMode) {
        case MovementMode::WALKING:
        {
            bool grounded = isGrounded();
            this->m_velocity.x *= grounded ? glm::pow(0.005f, dT) : glm::pow(0.15f, dT);
            this->m_velocity.z *= grounded ? glm::pow(0.005f, dT) : glm::pow(0.15f, dT);
            this->m_velocity.y *= glm::pow(0.67f, dT);

            this->m_velocity += this->m_acceleration * dT;
            // volume cast along each corner of the player
            glm::vec3 dist = this->m_velocity * dT;
            for (int i = 0; i < 12; i++) {
                glm::vec3 corner = this->m_position + corners[i];
                // differing logic for front/back
                int nextX;
                if (corners[i].x > 0) {
                    nextX = dist.x >= 0 ? glm::ceil(corner.x-0.05) : glm::floor(corner.x-0.01);
                } else {
                    nextX = dist.x >= 0 ? glm::ceil(corner.x+0.01) : glm::floor(corner.x+0.05);
                }
                while (glm::abs(nextX - corner.x) <= glm::abs(dist.x)) {
                    int xCoord = dist.x >= 0 ? nextX : nextX-1;
                    if (terrain.hasChunkAt(xCoord, corner.z)) {
                        BlockType block = terrain.getGlobalBlockAt(xCoord, corner.y, corner.z);
                        if (!((block == EMPTY)||(block == WATER)||(block == LAVA))) {
                            dist.x = nextX - corner.x;
                            this->m_velocity.x = 0;
                        }
                        break;
                    }
                    nextX += dist.x >= 0 ? 1 : -1;
                }
                // differing logic for left/right
                int nextZ;
                if (corners[i].z > 0) {
                    nextZ = dist.z >= 0 ? glm::ceil(corner.z-0.05) : glm::floor(corner.z-0.01);
                } else {
                    nextZ = dist.z >= 0 ? glm::ceil(corner.z+0.01) : glm::floor(corner.z+0.05);
                }
                while (glm::abs(nextZ - corner.z) <= glm::abs(dist.z)) {
                    int zCoord = dist.z >= 0 ? nextZ : nextZ-1;
                    if (terrain.hasChunkAt(corner.x, zCoord)) {
                        BlockType block = terrain.getGlobalBlockAt(corner.x, corner.y, zCoord);
                        if (!((block == EMPTY)||(block == WATER)||(block == LAVA))) {
                            dist.z = nextZ - corner.z;
                            this->m_velocity.z = 0;
                        }
                        break;
                    }
                    nextZ += dist.z >= 0 ? 1 : -1;
                }
                // differing logic for up/down
                int nextY;
                if (corners[i].y > 1) {
                    nextY = dist.y >= 0 ? glm::ceil(corner.y-0.05) : glm::floor(corner.y-0.01);
                } else {
                    nextY = dist.y >= 0 ? glm::ceil(corner.y+0.01) : glm::floor(corner.y+0.05);
                }
                while (glm::abs(nextY - corner.y) <= glm::abs(dist.y)) {
                    int yCoord = dist.y >= 0 ? nextY : nextY-1;
                    if (terrain.hasChunkAt(corner.x, corner.z)) {
                        BlockType block = terrain.getGlobalBlockAt(corner.x, yCoord, corner.z);
                        if (!((block == EMPTY)||(block == WATER)||(block == LAVA))) {
                            dist.y = nextY - corner.y;
                            this->m_velocity.y = 0;
                        }
                        break;
                    }
                    nextY += dist.y >= 0 ? 1 : -1;
                }
            }
            this->moveAlongVector(dist);
            break;
        }
        case MovementMode::FLYING:
            // first, apply drag
            this->m_velocity *= glm::pow(0.1f, dT);
            // then, apply acceleration
            this->m_velocity += this->m_acceleration * dT;
            // then, apply velocity
            if (glm::length(this->m_velocity) > 23.f) {
                this->m_velocity = glm::normalize(this->m_velocity);
                this->m_velocity *= 23.f;
            }
            this->moveAlongVector(this->m_velocity * dT);
            break;
    }
}

bool Player::isGrounded() const {
    int feet[4] = {0, 1, 6, 7};
    for (int foot: feet) {
        glm::vec3 corner = this->m_position + corners[foot];
        int yPos = glm::floor(corner.y-0.01f);
        if (this->mcr_terrain.hasChunkAt(corner.x, corner.z)) {
            BlockType block = this->mcr_terrain.getGlobalBlockAt(corner.x, yPos, corner.z);
            if (!((block == EMPTY)||(block == WATER)||(block == LAVA))) {
                return true;
            }
        }
    }
    return false;
}

bool Player::isSubmerged() const {
    for (glm::vec3 corner: corners) {
        glm::vec3 pos = this->m_position + corner;
        if (this->mcr_terrain.hasChunkAt(pos.x, pos.z)) {
            BlockType block = this->mcr_terrain.getGlobalBlockAt(pos.x, pos.y, pos.z);
            if ((block == WATER)||(block == LAVA)) {
                return true;
            }
        }
    }
    return false;
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
