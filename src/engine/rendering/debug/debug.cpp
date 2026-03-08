#include "debug.hpp"

#include "engine/rendering/opengl/opengl.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering{
    
    constexpr int LAT_SEGMENTS = 12;
    constexpr int LONG_SEGMENTS = 24;

    void SetupGLBuffers(GLuint &VAO, GLuint &VBO, GLuint &EBO, const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices) {
        Core::GetEngine().GetGL()->GenVertexArrays(1, &VAO);
        Core::GetEngine().GetGL()->BindVertexArray(VAO);

        Core::GetEngine().GetGL()->GenBuffers(1, &VBO);
        Core::GetEngine().GetGL()->BindBuffer(GL_ARRAY_BUFFER, VBO);
        Core::GetEngine().GetGL()->BufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        Core::GetEngine().GetGL()->GenBuffers(1, &EBO);
        Core::GetEngine().GetGL()->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        Core::GetEngine().GetGL()->BufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        Core::GetEngine().GetGL()->EnableVertexAttribArray(0); // position
        Core::GetEngine().GetGL()->VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        Core::GetEngine().GetGL()->EnableVertexAttribArray(1); // normal
        Core::GetEngine().GetGL()->VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        Core::GetEngine().GetGL()->EnableVertexAttribArray(2); // color
        Core::GetEngine().GetGL()->VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        Core::GetEngine().GetGL()->BindVertexArray(0);
    }

    DebugBox::DebugBox(glm::vec3 halfExtent, COL_RGBA color)
    {
        std::vector<glm::vec3> localCorners = {
            {-halfExtent.x, -halfExtent.y, -halfExtent.z},
            { halfExtent.x, -halfExtent.y, -halfExtent.z},
            { halfExtent.x,  halfExtent.y, -halfExtent.z},
            {-halfExtent.x,  halfExtent.y, -halfExtent.z},
            {-halfExtent.x, -halfExtent.y,  halfExtent.z},
            { halfExtent.x, -halfExtent.y,  halfExtent.z},
            { halfExtent.x,  halfExtent.y,  halfExtent.z},
            {-halfExtent.x,  halfExtent.y,  halfExtent.z}
        };

        vertices.resize(8);
        for (int i = 0; i < 8; ++i) {
            glm::vec3 worldPos = localCorners[i];
            vertices[i].position = worldPos;
            vertices[i].normal = glm::normalize(worldPos);
            vertices[i].color = color;
            vertices[i].texCoord = glm::vec2(0,0);
            vertices[i].tangent = glm::vec3(0);
        }

        std::vector<GLuint> indices = {
            0,1, 1,2, 2,3, 3,0, // bottom square edges
            4,5, 5,6, 6,7, 7,4, // top square edges
            0,4, 1,5, 2,6, 3,7  // vertical edgess
        };

        SetupGLBuffers(VAO, VBO, EBO, vertices, indices);
        
        indexCount = indices.size();
    }

    DebugSphere::DebugSphere(float radius, COL_RGBA color)
    {
        const int SEGMENTS = 32;
        std::vector<GLuint> indices;

        for (int i = 0; i < SEGMENTS; i++)
        {
            float angle = glm::two_pi<float>() * i / SEGMENTS;
            float c = cos(angle);
            float s = sin(angle);

            // XY ring
            vertices.push_back({ glm::vec3(radius * c, radius * s, 0), glm::vec3(0), color, glm::vec2(0) });

            // XZ ring
            vertices.push_back({ glm::vec3(radius * c, 0, radius * s), glm::vec3(0), color, glm::vec2(0) });

            // YZ ring
            vertices.push_back({ glm::vec3(0, radius * c, radius * s), glm::vec3(0), color, glm::vec2(0) });
        }

        for (int i = 0; i < SEGMENTS; i++)
        {
            int next = (i + 1) % SEGMENTS;

            int xy0 = i * 3;
            int xy1 = next * 3;

            int xz0 = i * 3 + 1;
            int xz1 = next * 3 + 1;

            int yz0 = i * 3 + 2;
            int yz1 = next * 3 + 2;

            // XY
            indices.push_back(xy0);
            indices.push_back(xy1);

            // XZ
            indices.push_back(xz0);
            indices.push_back(xz1);

            // YZ
            indices.push_back(yz0);
            indices.push_back(yz1);
        }

        SetupGLBuffers(VAO, VBO, EBO, vertices, indices);
        indexCount = indices.size();
    }

    DebugCapsule::DebugCapsule(float radius, float halfHeight, COL_RGBA color)
    {
        const int SEGMENTS = 24;
        std::vector<GLuint> indices;

        // --- circles (top & bottom) ---
        for (int i = 0; i < SEGMENTS; i++)
        {
            float angle = glm::two_pi<float>() * i / SEGMENTS;
            float c = cos(angle);
            float s = sin(angle);

            glm::vec3 dir(c * radius, 0, s * radius);

            // bottom ring
            vertices.push_back({ dir + glm::vec3(0,-halfHeight,0), glm::vec3(0), color, glm::vec2(0) });

            // top ring
            vertices.push_back({ dir + glm::vec3(0, halfHeight,0), glm::vec3(0), color, glm::vec2(0) });
        }

        for (int i = 0; i < SEGMENTS; i++)
        {
            int next = (i + 1) % SEGMENTS;

            // bottom circle
            indices.push_back(i*2);
            indices.push_back(next*2);

            // top circle
            indices.push_back(i*2+1);
            indices.push_back(next*2+1);
        }

        // --- vertical lines ---
        vertices.push_back({ glm::vec3(radius,-halfHeight,0), glm::vec3(0), color, glm::vec2(0) });
        vertices.push_back({ glm::vec3(radius, halfHeight,0), glm::vec3(0), color, glm::vec2(0) });

        vertices.push_back({ glm::vec3(-radius,-halfHeight,0), glm::vec3(0), color, glm::vec2(0) });
        vertices.push_back({ glm::vec3(-radius, halfHeight,0), glm::vec3(0), color, glm::vec2(0) });

        vertices.push_back({ glm::vec3(0,-halfHeight,radius), glm::vec3(0), color, glm::vec2(0) });
        vertices.push_back({ glm::vec3(0, halfHeight,radius), glm::vec3(0), color, glm::vec2(0) });

        vertices.push_back({ glm::vec3(0,-halfHeight,-radius), glm::vec3(0), color, glm::vec2(0) });
        vertices.push_back({ glm::vec3(0, halfHeight,-radius), glm::vec3(0), color, glm::vec2(0) });

        int base = vertices.size() - 8;

        for(int i = 0; i < 8; i += 2)
        {
            indices.push_back(base + i);
            indices.push_back(base + i + 1);
        }

        // ----------------------------------------------------
        // HALF CIRCLES (caps)
        // ----------------------------------------------------

        const int ARC_SEGMENTS = SEGMENTS / 2;

        auto addHalfCircle = [&](glm::vec3 center, bool topCap, bool xAxis)
        {
            int start = vertices.size();

            for(int i = 0; i <= ARC_SEGMENTS; i++)
            {
                float angle = glm::pi<float>() * i / ARC_SEGMENTS;

                float x = cos(angle) * radius;
                float y = sin(angle) * radius;

                if(!topCap) y = -y;

                glm::vec3 p;

                if(xAxis)
                    p = center + glm::vec3(x, y, 0);
                else
                    p = center + glm::vec3(0, y, x);

                vertices.push_back({ p, glm::vec3(0), color, glm::vec2(0) });

                if(i > 0)
                {
                    indices.push_back(start + i - 1);
                    indices.push_back(start + i);
                }
            }
        };

        // top hemisphere arcs
        addHalfCircle(glm::vec3(0, halfHeight, 0), true, true);
        addHalfCircle(glm::vec3(0, halfHeight, 0), true, false);

        // bottom hemisphere arcs
        addHalfCircle(glm::vec3(0,-halfHeight, 0), false, true);
        addHalfCircle(glm::vec3(0,-halfHeight, 0), false, false);

        SetupGLBuffers(VAO, VBO, EBO, vertices, indices);
        indexCount = indices.size();
    }

    DebugCylinder::DebugCylinder(float radius, float halfHeight, COL_RGBA color)
    {
        constexpr int SEGMENTS = 18; // minimal segment count for wireframe
        std::vector<GLuint> indices;

        // Vertex generation (top and bottom circles)
        for (int i = 0; i < SEGMENTS; ++i) {
            float angle = glm::two_pi<float>() * i / SEGMENTS;
            float x = cos(angle), z = sin(angle);
            glm::vec3 dir = glm::vec3(x, 0, z);

            // bottom ring
            vertices.push_back({ dir * radius + glm::vec3(0, -halfHeight, 0), dir, color, glm::vec2(0,0)});
            // top ring
            vertices.push_back({ dir * radius + glm::vec3(0, +halfHeight, 0), dir, color, glm::vec2(0,0)});
        }

        // Index generation
        for (int i = 0; i < SEGMENTS; ++i) {
            int next = (i + 1) % SEGMENTS;

            int b0 = i * 2;       // bottom current
            int t0 = b0 + 1;      // top current
            int b1 = next * 2;    // bottom next
            int t1 = b1 + 1;      // top next

            // Bottom circle
            indices.push_back(b0);
            indices.push_back(b1);

            // Top circle
            indices.push_back(t0);
            indices.push_back(t1);

            // Vertical lines
            indices.push_back(b0);
            indices.push_back(t0);
        }

        SetupGLBuffers(VAO, VBO, EBO, vertices, indices);
        indexCount = indices.size();
    }
    
    DebugShape::~DebugShape()
    {
        Core::GetEngine().GetGL()->DeleteVertexArrays(1, &VAO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &VBO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &EBO);
    }
}