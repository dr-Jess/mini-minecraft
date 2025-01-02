#include "chunk.h"
#include <iostream>

Chunk::Chunk(int x, int z, OpenGLContext* context) : Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
blockMutex(), ready(false),
    loaded(false),
    working(false)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) {
    blockMutex.lock();
    auto p = m_blocks.at(x + 16 * y + 16 * 256 * z);
    blockMutex.unlock();
    return p;
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getLocalBlockAt(int x, int y, int z) {
    return getLocalBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    blockMutex.lock();
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
    blockMutex.unlock();
}

std::unordered_map<BlockType, glm::vec2> Chunk::blockUVs = {
    {GRASS, glm::vec2(2, 15)},
    {DIRT, glm::vec2(2, 15)},
    {STONE, glm::vec2(1, 15)},
    {WATER, glm::vec2(14, 3)},
    {SNOW, glm::vec2(2, 11)},
    {LAVA, glm::vec2(14, 1)},
    // {BEDROCK, glm::vec2(1, 14)}
};

std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec2>> blockUVMap = {
    {GRASS, {
             {XPOS, glm::vec2(3, 15)},
             {XNEG, glm::vec2(3, 15)},
             {YPOS, glm::vec2(8, 13)},
             {YNEG, glm::vec2(2, 15)},
             {ZPOS, glm::vec2(3, 15)},
             {ZNEG, glm::vec2(3, 15)},
             }},
    {DIRT, {
            {XPOS, glm::vec2(2, 15)},
            {XNEG, glm::vec2(2, 15)},
            {YPOS, glm::vec2(2, 15)},
            {YNEG, glm::vec2(2, 15)},
            {ZPOS, glm::vec2(2, 15)},
            {ZNEG, glm::vec2(2, 15)},
            }},
    {STONE, {
             {XPOS, glm::vec2(1, 15)},
             {XNEG, glm::vec2(1, 15)},
             {YPOS, glm::vec2(1, 15)},
             {YNEG, glm::vec2(1, 15)},
             {ZPOS, glm::vec2(1, 15)},
             {ZNEG, glm::vec2(1, 15)},
             }},
    {WATER, {
             {XPOS, glm::vec2(14, 2)},
             {XNEG, glm::vec2(14, 2)},
             {YPOS, glm::vec2(14, 2)},
             {YNEG, glm::vec2(14, 2)},
             {ZPOS, glm::vec2(14, 2)},
             {ZNEG, glm::vec2(14, 2)},
             }},
    {SNOW, {
            {XPOS, glm::vec2(2, 11)},
            {XNEG, glm::vec2(2, 11)},
            {YPOS, glm::vec2(2, 11)},
            {YNEG, glm::vec2(2, 11)},
            {ZPOS, glm::vec2(2, 11)},
            {ZNEG, glm::vec2(2, 11)},
            }},
    {LAVA, {
            {XPOS, glm::vec2(14, 1)},
            {XNEG, glm::vec2(14, 1)},
            {YPOS, glm::vec2(14, 1)},
            {YNEG, glm::vec2(14, 1)},
            {ZPOS, glm::vec2(14, 1)},
            {ZNEG, glm::vec2(14, 1)},
            }},
    {SAND, {
            {XPOS, glm::vec2(0, 4)},
            {XNEG, glm::vec2(0, 4)},
            {YPOS, glm::vec2(0, 4)},
            {YNEG, glm::vec2(0, 4)},
            {ZPOS, glm::vec2(0, 4)},
            {ZNEG, glm::vec2(0, 4)},
            }},
    {WOOD, {
            {XPOS, glm::vec2(4, 14)},
            {XNEG, glm::vec2(4, 14)},
            {YPOS, glm::vec2(5, 14)},
            {YNEG, glm::vec2(5, 14)},
            {ZPOS, glm::vec2(4, 14)},
            {ZNEG, glm::vec2(4, 14)},
            }},
    {LEAVES, {
            {XPOS, glm::vec2(4, 12)},
            {XNEG, glm::vec2(4, 12)},
            {YPOS, glm::vec2(4, 12)},
            {YNEG, glm::vec2(4, 12)},
            {ZPOS, glm::vec2(4, 12)},
            {ZNEG, glm::vec2(4, 12)},
            }},
    {CACTUS, {
              {XPOS, glm::vec2(6, 11)},
              {XNEG, glm::vec2(6, 11)},
              {YPOS, glm::vec2(5, 11)},
              {YNEG, glm::vec2(5, 11)},
              {ZPOS, glm::vec2(6, 11)},
              {ZNEG, glm::vec2(6, 11)},
              }},
    {ICE, {
              {XPOS, glm::vec2(3, 11)},
              {XNEG, glm::vec2(3, 11)},
              {YPOS, glm::vec2(3, 11)},
              {YNEG, glm::vec2(3, 11)},
              {ZPOS, glm::vec2(3, 11)},
              {ZNEG, glm::vec2(3, 11)},
              }},
    {BEDROCK, {
              {XPOS, glm::vec2(1, 14)},
              {XNEG, glm::vec2(1, 14)},
              {YPOS, glm::vec2(1, 14)},
              {YNEG, glm::vec2(1, 14)},
              {ZPOS, glm::vec2(1, 14)},
              {ZNEG, glm::vec2(1, 14)},
              }},
};


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}


glm::vec2 Chunk::getUV(BlockType t, Direction dir) {
    glm::vec2 baseUV = blockUVMap[t][dir];
    float offset = 0.0625f;
    return baseUV * offset;
}

bool isAnimated(BlockType t) {
    return t == WATER || t == LAVA;
}

void Chunk::updateVBO(std::vector<glm::vec4>& interleavedData, Direction dir, const glm::vec4& pos, BlockType t, int vC, std::vector<GLuint>& indices) {
    glm::vec4 vertices[4];
    glm::vec4 color;
    glm::vec4 normal;

    switch (dir) {
    case XPOS: vertices[0] = glm::vec4(1, 0, 0, 1); vertices[1] = glm::vec4(1, 1, 0, 1); vertices[2] = glm::vec4(1, 1, 1, 1); vertices[3] = glm::vec4(1, 0, 1, 1); normal = glm::vec4(1, 0, 0, 0); break;
    case XNEG: vertices[0] = glm::vec4(0, 0, 0, 1); vertices[1] = glm::vec4(0, 1, 0, 1); vertices[2] = glm::vec4(0, 1, 1, 1); vertices[3] = glm::vec4(0, 0, 1, 1); normal = glm::vec4(-1, 0, 0, 0); break;
    case YPOS: vertices[0] = glm::vec4(0, 1, 0, 1); vertices[1] = glm::vec4(1, 1, 0, 1); vertices[2] = glm::vec4(1, 1, 1, 1); vertices[3] = glm::vec4(0, 1, 1, 1); normal = glm::vec4(0, 1, 0, 0); break;
    case YNEG: vertices[0] = glm::vec4(0, 0, 0, 1); vertices[1] = glm::vec4(1, 0, 0, 1); vertices[2] = glm::vec4(1, 0, 1, 1); vertices[3] = glm::vec4(0, 0, 1, 1); normal = glm::vec4(0, -1, 0, 0); break;
    case ZPOS: vertices[0] = glm::vec4(0, 0, 1, 1); vertices[1] = glm::vec4(1, 0, 1, 1); vertices[2] = glm::vec4(1, 1, 1, 1); vertices[3] = glm::vec4(0, 1, 1, 1); normal = glm::vec4(0, 0, 1, 0); break;
    case ZNEG: vertices[0] = glm::vec4(0, 0, 0, 1); vertices[1] = glm::vec4(1, 0, 0, 1); vertices[2] = glm::vec4(1, 1, 0, 1); vertices[3] = glm::vec4(0, 1, 0, 1); normal = glm::vec4(0, 0, -1, 0);  break;
    }

    // if (t == WATER && dir != YPOS) {
    //     return;
    // }

    glm::vec2 baseUV = getUV(t, dir);
    std::vector<glm::vec2> faceUVs = {
        baseUV + glm::vec2(0, 0),
        baseUV + glm::vec2(0, 0.0625f),
        baseUV + glm::vec2(0.0625f, 0.0625f),
        baseUV + glm::vec2(0.0625f, 0)
    };

    if (dir == XPOS) {
        faceUVs = {
            baseUV + glm::vec2(0.0625f, 0),
            baseUV + glm::vec2(0.0625f, 0.0625f),
            baseUV + glm::vec2(0, 0.0625f),
            baseUV + glm::vec2(0, 0),
        };
    } else if (dir == XNEG) {
        faceUVs = {
            baseUV + glm::vec2(0, 0),
            baseUV + glm::vec2(0, 0.0625f),
            baseUV + glm::vec2(0.0625f, 0.0625f),
            baseUV + glm::vec2(0.0625f, 0),
        };
    } else if (dir == ZPOS) {
        faceUVs = {
            baseUV + glm::vec2(0.0625f, 0),
            baseUV + glm::vec2(0, 0),
            baseUV + glm::vec2(0, 0.0625f),
            baseUV + glm::vec2(0.0625f, 0.0625f),
        };
    } else if (dir == ZNEG) {
        faceUVs = {
            baseUV + glm::vec2(0.0625f, 0),
            baseUV + glm::vec2(0, 0),
            baseUV + glm::vec2(0, 0.0625f),
            baseUV + glm::vec2(0.0625f, 0.0625f),
        };
    }

    for (int i = 0; i < 4; ++i) {
        interleavedData.push_back(glm::vec4(minX, 0, minZ, 0) + pos + vertices[i]);
        interleavedData.push_back(normal);
        if (t == WATER) {
         interleavedData.push_back(glm::vec4(faceUVs[i], 1.0f, 0.0f));
        } else if (t == LAVA) {
          interleavedData.push_back(glm::vec4(faceUVs[i], 2.0f, 0.0f));
        } else {
            interleavedData.push_back(glm::vec4(faceUVs[i], 0.0f, 0.0f));
        }
    }
    indices.push_back(vC + 0);
    indices.push_back(vC + 1);
    indices.push_back(vC + 2);
    indices.push_back(vC + 0);
    indices.push_back(vC + 2);
    indices.push_back(vC + 3);
}

bool isTransparent(BlockType t) {
    return t == WATER || t == CACTUS || t == ICE;
}

void Chunk::generateVBOData() {
    std::cout << "Generating Data" << std::endl;

    opq_interleavedData.clear();
    trans_interleavedData.clear();
    opq_indices.clear();
    trans_indices.clear();

    int opq_faceCount = 0;
    int opq_vertexCount = 0;

    int trans_faceCount = 0;
    int trans_vertexCount = 0;

    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 256; ++y) {
            for (int z = 0; z < 16; ++z) {
                BlockType t = getLocalBlockAt(x, y, z);
                glm::vec4 blockPos(x, y, z, 0);

                if (t != EMPTY) {
                    BlockType x_pos = (x < 15) ? getLocalBlockAt(x + 1, y, z) : (m_neighbors[XPOS] ? m_neighbors[XPOS]->getLocalBlockAt(0, y, z) : EMPTY);
                    BlockType x_neg = (x > 0) ? getLocalBlockAt(x - 1, y, z) : (m_neighbors[XNEG] ? m_neighbors[XNEG]->getLocalBlockAt(15, y, z) : EMPTY);
                    BlockType y_pos = (y < 255) ? getLocalBlockAt(x, y + 1, z) : EMPTY;
                    BlockType y_neg = (y > 0) ? getLocalBlockAt(x, y - 1, z) : EMPTY;
                    BlockType z_pos = (z < 15) ? getLocalBlockAt(x, y, z + 1) : (m_neighbors[ZPOS] ? m_neighbors[ZPOS]->getLocalBlockAt(x, y, 0) : EMPTY);
                    BlockType z_neg = (z > 0) ? getLocalBlockAt(x, y, z - 1) : (m_neighbors[ZNEG] ? m_neighbors[ZNEG]->getLocalBlockAt(x, y, 15) : EMPTY);

                    if (x_pos == EMPTY || ((isTransparent(x_pos) || x_pos == LAVA) && x_pos != t)) {
                        if (!isTransparent(t)) {
                            updateVBO(opq_interleavedData, XPOS, blockPos, t, opq_vertexCount, opq_indices); opq_faceCount ++; opq_vertexCount += 4;
                        } else {
                            updateVBO(trans_interleavedData, XPOS, blockPos, t, trans_vertexCount, trans_indices); trans_faceCount ++; trans_vertexCount += 4;
                        }
                    }
                    if (x_neg == EMPTY || ((isTransparent(x_neg) || x_neg == LAVA) && x_neg != t)) {
                        if (!isTransparent(t)) {
                            updateVBO(opq_interleavedData, XNEG, blockPos, t, opq_vertexCount, opq_indices); opq_faceCount ++; opq_vertexCount += 4;;
                        } else {
                            updateVBO(trans_interleavedData, XNEG, blockPos, t, trans_vertexCount, trans_indices); trans_faceCount ++; trans_vertexCount += 4;
                        }
                    }
                    if (y_pos == EMPTY || ((isTransparent(y_pos) || y_pos == LAVA) && y_pos != t)) {
                        if (!isTransparent(t)) {
                            updateVBO(opq_interleavedData, YPOS, blockPos, t, opq_vertexCount, opq_indices); opq_faceCount ++; opq_vertexCount += 4;
                        } else {
                            updateVBO(trans_interleavedData, YPOS, blockPos, t, trans_vertexCount, trans_indices); trans_faceCount ++; trans_vertexCount += 4;
                        }
                    }
                    if (y_neg == EMPTY || ((isTransparent(y_neg)|| y_neg == LAVA) && y_neg != t)) {
                        if (!isTransparent(t)) {
                            updateVBO(opq_interleavedData, YNEG, blockPos, t, opq_vertexCount, opq_indices); opq_faceCount ++; opq_vertexCount += 4;
                        } else {
                            updateVBO(trans_interleavedData, YNEG, blockPos, t, trans_vertexCount, trans_indices); trans_faceCount ++; trans_vertexCount += 4;
                        }
                    }
                    if (z_pos == EMPTY || ((isTransparent(z_pos) || z_pos == LAVA) && z_pos != t)) {
                        if (!isTransparent(t)) {
                            updateVBO(opq_interleavedData, ZPOS, blockPos, t, opq_vertexCount, opq_indices); opq_faceCount ++; opq_vertexCount += 4;
                        } else {
                            updateVBO(trans_interleavedData, ZPOS, blockPos, t, trans_vertexCount, trans_indices); trans_faceCount ++; trans_vertexCount += 4;
                        }
                    }
                    if (z_neg == EMPTY || ((isTransparent(z_neg) || z_neg == LAVA) && z_neg != t)) {
                        if (!isTransparent(t)) {
                            updateVBO(opq_interleavedData, ZNEG, blockPos, t, opq_vertexCount, opq_indices); opq_faceCount ++; opq_vertexCount += 4;
                        } else {
                            updateVBO(trans_interleavedData, ZNEG, blockPos, t, trans_vertexCount, trans_indices); trans_faceCount ++; trans_vertexCount += 4;
                        }
                    }
                }
            }
        }
    }

    /*
        std::cout << "debug: face count: " << trans_faceCount << std::endl;
        std::cout << "debug: vertex count: " << trans_vertexCount << std::endl;
    */

    working = true;
}



void Chunk::loadToGPU() {

    // std::cout << "Loading to GPU" << std::endl;

    generateBuffer(OPQ_INTERLEAVED);
    generateBuffer(OPQ_INDEX);

    if (bindBuffer(OPQ_INTERLEAVED)) {
        mp_context->glBufferData(GL_ARRAY_BUFFER, opq_interleavedData.size() * sizeof(glm::vec4), opq_interleavedData.data(), GL_STATIC_DRAW);
    }

    if (bindBuffer(OPQ_INDEX)) {
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, opq_indices.size() * sizeof(GLuint), opq_indices.data(), GL_STATIC_DRAW);
    }
    // std::cout << "debug: 1 index count " << this->elemCount(INDEX) << std::endl;


    if (!opq_indices.empty()) {
        indexCounts[OPQ_INDEX] = opq_indices.size();
    } else {
        indexCounts[OPQ_INDEX] = 0;
    }

    if (!opq_interleavedData.empty()) {
        indexCounts[OPQ_INTERLEAVED] = opq_interleavedData.size();
    } else {
        indexCounts[OPQ_INTERLEAVED] = 0;
    }

    // std::cout << "debug: INTERLEAVED count: " << indexCounts[OPQ_INTERLEAVED] << std::endl;

    generateBuffer(TRANS_INTERLEAVED);
    generateBuffer(TRANS_INDEX);

    if (bindBuffer(TRANS_INTERLEAVED)) {
        mp_context->glBufferData(GL_ARRAY_BUFFER, trans_interleavedData.size() * sizeof(glm::vec4), trans_interleavedData.data(), GL_STATIC_DRAW);
    }

    if (bindBuffer(TRANS_INDEX)) {
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, trans_indices.size() * sizeof(GLuint), trans_indices.data(), GL_STATIC_DRAW);
    }

    if (!trans_indices.empty()) {
        indexCounts[TRANS_INDEX] = trans_indices.size();
    } else {
        indexCounts[TRANS_INDEX] = 0;
    }

    if (!trans_interleavedData.empty()) {
        indexCounts[TRANS_INTERLEAVED] = trans_interleavedData.size();
    } else {
        indexCounts[TRANS_INTERLEAVED] = 0;
    }


    // std::cout << "debug: interleaved count " << this->elemCount(INTERLEAVED) << std::endl;
    // std::cout << "debug: 2 index count " << this->elemCount(INDEX) << std::endl;
}


void Chunk::createVBOdata() {
    generateVBOData();
    // loadToGPU();
}

void Chunk::create() {
    createVBOdata();
}
