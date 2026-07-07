#pragma once

#include "RendererAPI.h"

namespace Hydra
{

    class RenderCommand
    {
    public:
        static void Init()
        {            
            s_RendererAPI->Init();
        }

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RendererAPI->SetViewport(x, y, width, height);
        }

        static void SetClearColor(const glm::vec4& color)
        {
            s_RendererAPI->SetClearColor(color);
        }
         
        static void Clear()
        {
            s_RendererAPI->Clear();
        }

        static void DrawIndexed(const Ref<VertexArray>& VertexArray, uint32_t indexCount = 0)
        {
            s_RendererAPI->DrawIndexed(VertexArray, indexCount);
        }

        static void DrawLines(const Ref<VertexArray>& VertexArray, uint32_t vertexCount = 0)
        {
            s_RendererAPI->DrawLines(VertexArray, vertexCount);
        }

        static void SetLineWidth(float width)
        {
            s_RendererAPI->SetLineWidth(width);
        }

    private:
        static Scope<RendererAPI> s_RendererAPI;
    };

}