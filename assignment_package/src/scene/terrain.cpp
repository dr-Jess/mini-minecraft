#include "terrain.h"
#include "cube.h"
#include <random>
#include <stdexcept>
#include <iostream>
#include <thread>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(),
      chunkMutex(),
        mp_context(context)
{}

Terrain::~Terrain(){
    for (auto &chunkPair : m_chunks) {
        chunkPair.second->destroyVBOdata();
    }
}


// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getGlobalBlockAt(int x, int y, int z)
{
    chunkMutex.lock();
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        auto temp = c->getLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                                       static_cast<unsigned int>(y),
                                       static_cast<unsigned int>(z - chunkOrigin.y));
        chunkMutex.unlock();
        return temp;
    }
    else {
        chunkMutex.unlock();
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                    std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getGlobalBlockAt(glm::vec3 p) {
    return getGlobalBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    auto p = m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
    return p;
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {

    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}

bool Terrain::hasTerrainAt(int x, int z) {
    if(m_generatedTerrain.count(toKey(x, z)) > 0) {
        return true;
    }
    else {
        return false;
    }
}


// uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
//     int xFloor = static_cast<int>(glm::floor(x / 16.f));
//     int zFloor = static_cast<int>(glm::floor(z / 16.f));
//     return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
// }

void Terrain::setGlobalBlockAt(int x, int y, int z, BlockType t)
{
    chunkMutex.lock();
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                           static_cast<unsigned int>(y),
                           static_cast<unsigned int>(z - chunkOrigin.y),
                           t);
        // c->createVBOdata();
        chunkMutex.unlock();
        c->loaded = false;
    }
    else {
        chunkMutex.unlock();
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

void Terrain::loadChunkVBOs() {
    for (const auto& [key, value] : m_chunks) {
        if(value->ready && !value->loaded) {
            std::cout << "Creating VBO Data" << std::endl;
            value->ready = false;
            value->working = false;
            auto x = value.get();
            auto f = [x]() {
                std::cout << "Beginning VBO data Generation" << std::endl;
                x->generateVBOData();
            };

            auto VBOWorker = std::thread(f);
            VBOWorker.detach();
        }

        if(value->working) { ///DONE WORKING
            // std::cout << "mew" << std::endl;
            value->loadToGPU();
            value->working = false;
            value->loaded = true;
        }
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    chunkMutex.lock();
    uPtr<Chunk> chunk = mkU<Chunk>(x, z, mp_context);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = std::move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    chunkMutex.unlock();
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram, bool opq, bool trans) {

    if (opq) {
        for(int x = minX; x < maxX; x += 16) {
            for(int z = minZ; z < maxZ; z += 16) {
                if (hasChunkAt(x, z)) {
                    const uPtr<Chunk> &chunk = getChunkAt(x, z);
                    if(chunk->loaded) {
                        shaderProgram->drawOpq(*chunk);
                    }
                }
            }
        }
    }
    if (trans) {
        for(int x = minX; x < maxX; x += 16) {
            for(int z = minZ; z < maxZ; z += 16) {
                if (hasChunkAt(x, z)) {
                    const uPtr<Chunk> &chunk = getChunkAt(x, z);
                    if(chunk->loaded) {
                        shaderProgram->drawTrans(*chunk);
                    }
                }
            }
        }
    }
}


float PerlinNoise(float x, float y, float z);
float voronoiNoise(const glm::vec2& position, int seed);
float noise(int x, int y) ;
float distanceToVoronoiEdge(float x, float y, int seed);


void Terrain::GenerateTerrain(int xPos, int zPos)  {

    if(m_generatedTerrain.count(toKey(xPos, zPos)) > 0) {
        return;
    }
    if(m_generatedTerrain.find(toKey(xPos, zPos)) != m_generatedTerrain.end()) {
        return;
    }
    m_generatedTerrain.insert(toKey(xPos, zPos));

    int WinChunks = 4;

    for(int x = xPos; x < 16*WinChunks + xPos; x += 16) {
        for(int z = zPos; z < 16*WinChunks + zPos; z += 16) {
            if(hasChunkAt(x, z)) {
                auto *c = getChunkAt(x, z).get();
                c->ready = false;
            }
        }
    }

    std::cout << "Generating Terrain at " << xPos << ", " << zPos << std::endl;

    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = xPos; x < 16*WinChunks + xPos; x += 16) {
        for(int z = zPos; z < 16*WinChunks + zPos; z += 16) {
            instantiateChunkAt(x, z);
        }
    }

    // Create the basic terrain floor
    for (int x = xPos; x < 16*WinChunks + xPos; x++) {
        for (int z = zPos; z < 16*WinChunks + zPos; z++) {

            const float typeTerrain = 40 * PerlinNoise(x * 0.003, 12, z * 0.003) + 129;
            const float terrain_perlin = PerlinNoise(x * 0.02, 12.23, z * 0.02);
            const float dist = distanceToVoronoiEdge(x * 0.02, z * 0.02, 43);

            for(int y = 0; y < 256; y++) {
                if (y == 0) {
                    setGlobalBlockAt(x, y, z, BEDROCK);
                } else if (y <= 128) {
                    float noise = PerlinNoise(0.1*x, 0.1*z, 0.1*y);
                    // I know instructions say negative but I find this produces a nice looking result
                    if (noise < 0.5) {
                        if (y<25) {
                            setGlobalBlockAt(x, y, z, LAVA);
                        } else {
                            setGlobalBlockAt(x, y, z, EMPTY);
                        }
                    } else {
                        setGlobalBlockAt(x, y, z, STONE);
                    }
                } else if (y == 129) {
                    setGlobalBlockAt(x, y, z, STONE);
                }
                else {
                        if (typeTerrain <= 139) {
                            if (y <= typeTerrain) {
                                setGlobalBlockAt(x, y, z, DIRT);
                            }
                            else if (y > typeTerrain && y <= 139) {
                                setGlobalBlockAt(x, y, z, WATER);
                            }
                        } else {
                            // split into four biomes:
                            float terrainPercent = (typeTerrain - 139) / 30;
                            // 0 to 1
                            if (terrainPercent < 0.333f) {
                                // std::cout << "A" << x << ", " << z << std::endl;

                                float temp = (terrainPercent - (0.333f * 0.5));
                                if (temp < 0) {
                                    temp = -temp;
                                }

                                float amp = (0.333f / 2.0f) - temp;

                                float threshold = (80 * amp * terrain_perlin) + 139;

                                if (y <= threshold) {
                                    if (dist > 0.1) {
                                        setGlobalBlockAt(x, y, z, SAND);
                                        if(y+1 > threshold) {
                                            if (x % 10 == (int)(threshold) % 10 && z % 10 == (int)(threshold) % 10) {
                                                setGlobalBlockAt(x, y+1, z, CACTUS);
                                                setGlobalBlockAt(x, y+2, z, CACTUS);
                                                setGlobalBlockAt(x, y+3, z, CACTUS);
                                                if (x % 10 > 4) {
                                                    setGlobalBlockAt(x, y+4, z, CACTUS);
                                                }
                                            }
                                        }
                                    } else {
                                        setGlobalBlockAt(x, y-1, z, WATER);
                                        setGlobalBlockAt(x, y, z, EMPTY);
                                    }
                                }
                            } else if (terrainPercent < 0.666f) {
                                //     //Grass
                                // std::cout << "B " << x << ", " << z << std::endl;
                                float temp = (terrainPercent - (0.333f * 1.5));
                                if (temp < 0) {
                                    temp = -temp;
                                }

                                float amp = (0.333f / 2.0f) - temp;

                                float threshold = (80 * amp * terrain_perlin) + 139;

                                if (y <= threshold) {
                                    if(y + 1 > threshold) {
                                        if(dist > 0.1) {
                                            setGlobalBlockAt(x, y, z, GRASS);
                                            if (noise(x, z) > 0.97f) {

                                                bool ctd = true;
                                                for(int i = -3; i <= 3; i++) {
                                                    for(int j = -3; j <= 3; j++) {
                                                        if ((x + i) % 16 == 0 || (z + j) % 16 == 0 || noise(x+i, z+j) > 0.99f) {
                                                            ctd = false;
                                                        }
                                                    }
                                                }

                                                if(ctd) {
                                                    setGlobalBlockAt(x, y+1, z, WOOD);
                                                    setGlobalBlockAt(x, y+2, z, WOOD);
                                                    setGlobalBlockAt(x, y+3, z, WOOD);
                                                    setGlobalBlockAt(x, y+4, z, WOOD);
                                                    setGlobalBlockAt(x, y+5, z, WOOD);
                                                    setGlobalBlockAt(x, y+6, z, WOOD);
                                                    setGlobalBlockAt(x, y+7, z, LEAVES);

                                                    for(int i = -2; i <= 2; i++) {
                                                        for(int j = -2; j <= 2; j++) {
                                                            if (i == 0  && j == 0) {
                                                                continue;
                                                            }

                                                            setGlobalBlockAt(x+i, y+4, z+j, LEAVES);
                                                        }
                                                    }

                                                    for(int i = -1; i <= 1; i++) {
                                                        for(int j = -1; j <= 1; j++) {
                                                            if (i == 0  && j == 0) {
                                                                continue;
                                                            }

                                                            setGlobalBlockAt(x+i, y+6, z+j, LEAVES);
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            setGlobalBlockAt(x, y-1, z, WATER);
                                            setGlobalBlockAt(x, y, z, EMPTY);
                                        }
                                    } else {
                                        setGlobalBlockAt(x, y, z, DIRT);
                                    }
                                }
                            } else {
                                //     //Snowy Mountains

                                float temp = terrainPercent - (0.333f * 2.5);
                                if (temp < 0) {
                                    temp = -temp;
                                }
                                float amp = (0.333f / 2.0f) - temp;


                                float threshold = 360 * amp * terrain_perlin + 139;

                                if (y <= threshold) {
                                    if(y + 1 > threshold) {
                                        setGlobalBlockAt(x, y, z, SNOW);

                                        if (dist > 0.1) {
                                            if (noise(x, z) > 0.97f) {
                                                bool ctd = true;
                                                for(int i = -3; i <= 3; i++) {
                                                    for(int j = -3; j <= 3; j++) {
                                                        if ((x + i) % 16 == 0 || (z + j) % 16 == 0 || noise(x+i, z+j) > 0.99f) {
                                                            ctd = false;
                                                        }
                                                    }
                                                }

                                                if(!ctd) {
                                                    break;
                                                }

                                                if(ctd) {
                                                    setGlobalBlockAt(x, y+1, z, WOOD);
                                                    setGlobalBlockAt(x, y+2, z, WOOD);
                                                    setGlobalBlockAt(x, y+3, z, WOOD);
                                                    setGlobalBlockAt(x, y+4, z, WOOD);
                                                    setGlobalBlockAt(x, y+5, z, WOOD);
                                                    setGlobalBlockAt(x, y+6, z, WOOD);
                                                    setGlobalBlockAt(x, y+7, z, LEAVES);

                                                    for(int i = -2; i <= 2; i++) {
                                                        for(int j = -2; j <= 2; j++) {
                                                            if (i == 0  && j == 0) {
                                                                continue;
                                                            }

                                                            setGlobalBlockAt(x+i, y+4, z+j, LEAVES);
                                                        }
                                                    }

                                                    for(int i = -1; i <= 1; i++) {
                                                        for(int j = -1; j <= 1; j++) {
                                                            if (i == 0  && j == 0) {
                                                                continue;
                                                            }

                                                            setGlobalBlockAt(x+i, y+6, z+j, LEAVES);
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            setGlobalBlockAt(x, y-1, z, WATER);
                                            setGlobalBlockAt(x, y, z, EMPTY);
                                        }
                                    } else {
                                        setGlobalBlockAt(x, y, z, ICE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }



    // std::cout << "Part 2 for at " << xPos << ", " << zPos << std::endl;



    // for(int y = 0; y < 255; y++) {
    //     for (int x = xPos; x < 16*WinChunks + xPos; x++) {
    //         for (int z = zPos; z < 16*WinChunks + zPos; z++) {
    //             if (y <= 8 * PerlinNoise(x / 20.37, 100, z /20.37) + 60) {
    //                 setGlobalBlockAt(x, y, z, STONE);
    //             }
    //         }
    //     }
    // }

    // std::cout << "Part 3 for at " << xPos << ", " << zPos << std::endl;

    for(int x = xPos; x < 16*WinChunks + xPos; x += 16) {
        for(int z = zPos; z < 16*WinChunks + zPos; z += 16) {
            auto *c = getChunkAt(x, z).get();
            c->ready = true;
        }
    }


    std::cout << "Success at " << xPos << ", " << zPos << std::endl;
}



float PerlinNoise(float x, float y, float z) {
    // Fade function to smooth the interpolation
    auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };

    // Gradient function (returns a pseudo-random gradient value)
    auto grad = [](int hash, float x, float y, float z) -> float {
        int h = hash & 15;
        float u = (h < 8) ? x : y;
        float v = (h < 4) ? y : (h == 12 || h == 14) ? x : z;
        return (h & 1 ? -u : u) + (h & 2 ? -v : v) + (h & 4 ? -z : z);
    };

    // Permutation table (fixed, deterministic)
    static const int p[] = {
        151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
        140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
        247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
        57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
        74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 109, 32, 41,
        131, 94, 255, 125, 48, 167, 57, 183, 146, 103, 190, 38, 127, 190, 115, 103
    };

    // Hash function to map 2D coordinates to a deterministic pseudo-random value
    auto hash = [&](int x, int y, int z) -> int {
        return p[(x + p[(y + p[z & 255]) & 255]) & 255];
    };

    // Determine grid cell coordinates
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;

    // Relative coordinates in grid cell
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = z - std::floor(z);

    // Fade curves for smooth interpolation
    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    // Hash coordinates of the 4 corners of the unit square
    int aaa = hash(X, Y, Z);
    int aab = hash(X, Y, Z + 1);
    int aba = hash(X, Y + 1, Z);
    int abb = hash(X, Y + 1, Z + 1);
    int baa = hash(X + 1, Y, Z);
    int bab = hash(X + 1, Y, Z + 1);
    int bba = hash(X + 1, Y + 1, Z);
    int bbb = hash(X + 1, Y + 1, Z + 1);

    // Interpolate gradients using fade function
    float gradAAA = grad(aaa, xf, yf, zf);
    float gradAAB = grad(aab, xf, yf, zf - 1);
    float gradABA = grad(aba, xf, yf - 1, zf);
    float gradABB = grad(abb, xf, yf - 1, zf - 1);
    float gradBAA = grad(baa, xf - 1, yf, zf);
    float gradBAB = grad(bab, xf - 1, yf, zf - 1);
    float gradBBA = grad(bba, xf - 1, yf - 1, zf);
    float gradBBB = grad(bbb, xf - 1, yf - 1, zf - 1);

    // Interpolate the results
    float x1 = glm::mix(gradAAA, gradBAA, u);
    float x2 = glm::mix(gradABA, gradBBA, u);
    float y1 = glm::mix(x1, x2, v);
    x1 = glm::mix(gradAAB, gradBAB, u);
    x2 = glm::mix(gradABB, gradBBB, u);
    float y2 = glm::mix(x1, x2, v);

    return glm::mix(y1, y2, w) * 0.5f + 0.5f; // Normalize the result to [0, 1]
}





// terrain functions



float noise(int x, int y) {
    x = (y << 13) ^ x;
    uint32_t hash = (x * (y * x * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return static_cast<float>(hash) / static_cast<float>(0x7fffffff);
}




glm::vec2 hash(const glm::ivec2& gridPoint, int seed) {
    uint32_t hash = gridPoint.x * 73856093 ^ gridPoint.y * 19349663 ^ seed * 83492791;
    return glm::vec2(
        (hash & 0xFFFF) / float(0xFFFF),
        ((hash >> 16) & 0xFFFF) / float(0xFFFF)
        );
}

// Function to compute distance to the edge of Voronoi noise
float distanceToVoronoiEdge(float x, float y, int seed) {
    glm::vec2 pos(x, y);
    glm::ivec2 baseCell(glm::floor(pos));
    float closest = FLT_MAX;
    float secondClosest = FLT_MAX;

    // Iterate through neighboring cells (3x3 grid around the base cell)
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            glm::ivec2 neighborCell = baseCell + glm::ivec2(i, j);
            glm::vec2 randomPoint = hash(neighborCell, seed);
            glm::vec2 neighborPoint = glm::vec2(neighborCell) + randomPoint;

            float dist = glm::length(neighborPoint - pos);

            if (dist < closest) {
                secondClosest = closest;
                closest = dist;
            } else if (dist < secondClosest) {
                secondClosest = dist;
            }
        }
    }

    // Distance to edge is the difference between the closest two distances
    return secondClosest - closest;
}


