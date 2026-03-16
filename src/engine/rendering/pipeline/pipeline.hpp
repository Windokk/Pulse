#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>

namespace Pulse::Engine::Rendering{

    class Shader;
    struct RenderPass;
    enum class ShaderDataType;

    enum class PrimitiveTopology
    {
        Points,
        Lines,
        LineStrip,

        Triangles,
        TriangleStrip,
        TriangleFan
    };

    enum class CullMode
    {
        None,
        Back,
        Front
    };

    enum class PolygonMode
    {
        Fill,
        Line
    };

    enum class DepthCompareOp
    {
        Less,
        LessOrEqual,
        Greater,
        Always,
        Never
    };

    enum class BlendFactor
    {
        Zero,
        One,
        SrcAlpha,
        OneMinusSrcAlpha
    };

    enum class BlendOp
    {
        Add,
        Subtract
    };

    struct VertexElement
    {
        std::string name;
        ShaderDataType type;
        uint32_t location = 0;
        uint32_t offset = 0;
        uint32_t Size() const;
    };

    class VertexLayout
    {
    public:

        VertexLayout() = default;

        VertexLayout(std::initializer_list<VertexElement> elements)
            : m_Elements(elements)
        {
            CalculateStride();
        }

        const std::vector<VertexElement>& GetElements() const
        {
            return m_Elements;
        }

        uint32_t GetStride() const
        {
            return m_Stride;
        }

    private:

        void CalculateStride()
        {
            m_Stride = 0;
            uint32_t offset = 0;

            for (auto& element : m_Elements)
            {
                element.offset = offset;
                offset += element.Size();
            }

            m_Stride = offset;
        }

        std::vector<VertexElement> m_Elements;
        uint32_t m_Stride = 0;
    };

    struct PipelineSpecifications
    {
        std::shared_ptr<Shader> shader;

        VertexLayout vertexLayout;

        PrimitiveTopology topology = PrimitiveTopology::Triangles;

        CullMode cullMode = CullMode::Back;
        PolygonMode polygonMode = PolygonMode::Fill;

        bool depthTest = true;
        bool depthWrite = true;
        DepthCompareOp depthCompare = DepthCompareOp::Less;

        bool blending = false;
        BlendFactor srcBlend = BlendFactor::SrcAlpha;
        BlendFactor dstBlend = BlendFactor::OneMinusSrcAlpha;
        BlendOp blendOp = BlendOp::Add;

        std::string debugName;
    };

    class Pipeline
    {
        public:

            virtual ~Pipeline() = default;

            const PipelineSpecifications& GetSpecifications() const
            {
                return m_Specifications;
            };

            virtual void Invalidate() = 0;

            virtual void Bind() = 0;

            static std::shared_ptr<Pipeline> Create(const PipelineSpecifications& specs);

        protected:
            PipelineSpecifications m_Specifications;
    };
}