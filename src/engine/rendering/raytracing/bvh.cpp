#include "bvh.hpp"

#include <algorithm>
#include <limits>

namespace Pulse::Engine::Rendering::Raytracing {

    namespace {

        constexpr int kSAHBins = 12;
        constexpr uint32_t kMaxLeafTriangles = 4;

        float SurfaceArea(const glm::vec3& boundsMin, const glm::vec3& boundsMax)
        {
            glm::vec3 e = boundsMax - boundsMin;
            if (e.x < 0.0f || e.y < 0.0f || e.z < 0.0f)
                return 0.0f;
            return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
        }

        void UpdateBounds(BVHNode& node, const std::vector<BVHPrimitive>& prims, const std::vector<uint32_t>& order)
        {
            glm::vec3 boundsMin(std::numeric_limits<float>::max());
            glm::vec3 boundsMax(std::numeric_limits<float>::lowest());

            for (uint32_t i = 0; i < node.triCount; i++)
            {
                const BVHPrimitive& p = prims[order[node.leftFirst + i]];
                boundsMin = glm::min(boundsMin, p.boundsMin);
                boundsMax = glm::max(boundsMax, p.boundsMax);
            }

            node.boundsMin = boundsMin;
            node.boundsMax = boundsMax;
        }

        // Binned SAH : bins primitive centroids along each axis into kSAHBins buckets, then evaluates
        // the surface-area-heuristic cost for every candidate plane between buckets and keeps the best
        // one found across all 3 axes. Returns false if no axis has any centroid spread at all (every
        // primitive in the node shares the same centroid - can't usefully split further).
        bool FindBestSplit(const BVHNode& node, const std::vector<BVHPrimitive>& prims, const std::vector<uint32_t>& order,
            int& outAxis, float& outSplitPos, float& outCost)
        {
            outCost = std::numeric_limits<float>::max();
            bool found = false;

            for (int axis = 0; axis < 3; axis++)
            {
                float centroidMin = std::numeric_limits<float>::max();
                float centroidMax = std::numeric_limits<float>::lowest();

                for (uint32_t i = 0; i < node.triCount; i++)
                {
                    float c = prims[order[node.leftFirst + i]].centroid[axis];
                    centroidMin = std::min(centroidMin, c);
                    centroidMax = std::max(centroidMax, c);
                }

                if (centroidMax - centroidMin < 1e-8f)
                    continue;

                struct Bin
                {
                    glm::vec3 boundsMin{ std::numeric_limits<float>::max() };
                    glm::vec3 boundsMax{ std::numeric_limits<float>::lowest() };
                    uint32_t count = 0;
                };

                Bin bins[kSAHBins];
                float scale = kSAHBins / (centroidMax - centroidMin);

                for (uint32_t i = 0; i < node.triCount; i++)
                {
                    const BVHPrimitive& p = prims[order[node.leftFirst + i]];
                    int binIdx = std::min(kSAHBins - 1, (int)((p.centroid[axis] - centroidMin) * scale));
                    bins[binIdx].count++;
                    bins[binIdx].boundsMin = glm::min(bins[binIdx].boundsMin, p.boundsMin);
                    bins[binIdx].boundsMax = glm::max(bins[binIdx].boundsMax, p.boundsMax);
                }

                // Sweep left->right and right->left to get, for each of the (kSAHBins - 1) candidate
                // planes, the cumulative count/surface-area of everything to its left and to its right.
                float leftArea[kSAHBins - 1];
                float rightArea[kSAHBins - 1];
                uint32_t leftCount[kSAHBins - 1];
                uint32_t rightCount[kSAHBins - 1];

                glm::vec3 lMin(std::numeric_limits<float>::max());
                glm::vec3 lMax(std::numeric_limits<float>::lowest());
                uint32_t lCount = 0;

                for (int i = 0; i < kSAHBins - 1; i++)
                {
                    lCount += bins[i].count;
                    lMin = glm::min(lMin, bins[i].boundsMin);
                    lMax = glm::max(lMax, bins[i].boundsMax);
                    leftCount[i] = lCount;
                    leftArea[i] = lCount > 0 ? SurfaceArea(lMin, lMax) : 0.0f;
                }

                glm::vec3 rMin(std::numeric_limits<float>::max());
                glm::vec3 rMax(std::numeric_limits<float>::lowest());
                uint32_t rCount = 0;

                for (int i = kSAHBins - 1; i >= 1; i--)
                {
                    rCount += bins[i].count;
                    rMin = glm::min(rMin, bins[i].boundsMin);
                    rMax = glm::max(rMax, bins[i].boundsMax);
                    rightCount[i - 1] = rCount;
                    rightArea[i - 1] = rCount > 0 ? SurfaceArea(rMin, rMax) : 0.0f;
                }

                float binWidth = (centroidMax - centroidMin) / kSAHBins;

                for (int i = 0; i < kSAHBins - 1; i++)
                {
                    float cost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
                    if (cost < outCost)
                    {
                        outCost = cost;
                        outAxis = axis;
                        outSplitPos = centroidMin + binWidth * (i + 1);
                        found = true;
                    }
                }
            }

            return found;
        }

        void Subdivide(std::vector<BVHNode>& nodes, uint32_t& nodesUsed, uint32_t nodeIdx,
            const std::vector<BVHPrimitive>& prims, std::vector<uint32_t>& order)
        {
            BVHNode& node = nodes[nodeIdx];

            if (node.triCount <= kMaxLeafTriangles)
                return;

            int axis = 0;
            float splitPos = 0.0f, splitCost = 0.0f;
            if (!FindBestSplit(node, prims, order, axis, splitPos, splitCost))
                return;

            // Only split if it's actually cheaper than leaving this node as one big leaf.
            float parentCost = node.triCount * SurfaceArea(node.boundsMin, node.boundsMax);
            if (splitCost >= parentCost)
                return;

            // Partition [leftFirst, leftFirst + triCount) in-place by centroid vs. the chosen plane.
            auto rangeBegin = order.begin() + node.leftFirst;
            auto rangeEnd = rangeBegin + node.triCount;
            auto mid = std::partition(rangeBegin, rangeEnd, [&](uint32_t primIdx) {
                return prims[primIdx].centroid[axis] < splitPos;
            });

            uint32_t leftCount = (uint32_t)std::distance(rangeBegin, mid);
            if (leftCount == 0 || leftCount == node.triCount)
                return; // degenerate split (e.g. all remaining centroids landed on one side) - keep as leaf

            uint32_t leftFirst = node.leftFirst;
            uint32_t rightFirst = leftFirst + leftCount;
            uint32_t rightCount = node.triCount - leftCount;

            uint32_t leftIdx = nodesUsed++;
            uint32_t rightIdx = nodesUsed++;

            nodes[leftIdx].leftFirst = leftFirst;
            nodes[leftIdx].triCount = leftCount;
            nodes[rightIdx].leftFirst = rightFirst;
            nodes[rightIdx].triCount = rightCount;

            // node.leftFirst now refers to a node index (not a triangle index) - reinterpreted per the
            // triCount == 0 convention.
            node.leftFirst = leftIdx;
            node.triCount = 0;

            UpdateBounds(nodes[leftIdx], prims, order);
            UpdateBounds(nodes[rightIdx], prims, order);

            Subdivide(nodes, nodesUsed, leftIdx, prims, order);
            Subdivide(nodes, nodesUsed, rightIdx, prims, order);
        }

    }

    std::vector<BVHNode> BVHBuilder::Build(const std::vector<BVHPrimitive>& primitives, std::vector<uint32_t>& outOrder)
    {
        size_t n = primitives.size();

        outOrder.resize(n);
        for (size_t i = 0; i < n; i++)
            outOrder[i] = (uint32_t)i;

        if (n == 0)
            return { BVHNode{ glm::vec3(0.0f), 0, glm::vec3(0.0f), 0 } };

        // Upper bound on total node count for a binary tree over n leaves : each split adds exactly 2
        // nodes and there are at most n - 1 splits, so 2n - 1 nodes covers every case. Allocated
        // up front (fixed size, no push_back) so the BVHNode& references taken during Subdivide are
        // never invalidated by a reallocation.
        std::vector<BVHNode> nodes(2 * n);
        uint32_t nodesUsed = 1;

        nodes[0].leftFirst = 0;
        nodes[0].triCount = (uint32_t)n;
        UpdateBounds(nodes[0], primitives, outOrder);

        Subdivide(nodes, nodesUsed, 0, primitives, outOrder);

        nodes.resize(nodesUsed);
        return nodes;
    }

}
