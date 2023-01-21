#pragma once

struct SDL_Window;
struct SDL_Surface;


#include "Camera.h"
#include "DataStructures.h"

namespace dae
{

	class Mesh;
	class Texture;

	class Renderer final
	{

#pragma region Enums
		enum class RenderMode
		{
			Hardware,
			Software
		};

		enum class BufferMode
		{
			Texture,
			Depth,
		};

		enum class ColorMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined,
		};

		enum class CullMode
		{
			Back,
			Front,
			None,
		};

		enum class SampleState
		{
			Point,
			Linear,
			Anisotropic
		};
#pragma endregion

	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render();


		void ToggleSampleState();
		void ToggleDepthBuffer();
		void ToggleColorMode();
		void ToggleNormals();
		void ToggleRotation();
		void ToggleRenderMode();
		void ToggleClearColor();
		void ToggleCullMode();
		void ToggleFire();
		void ToggleDrawBoundingBox();

	private:

		//SHARED
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		bool m_UseNormalMap{ true };
		bool m_IsRotating{ true };
		bool m_UseUniformColor{ false };
		bool m_DrawBoundingBox{ false };

		std::vector<Mesh*> m_pMeshes{};

		Camera m_Camera;

		RenderMode m_CurrentRenderMode{ RenderMode::Hardware };
		CullMode m_CurrentCullMode{ CullMode::Back };

		void InitMeshes();
		Matrix MakeWorldMatrix();

		//DIRECTX
		ID3D11Device* m_pDevice{ nullptr };
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDepthStencilView{ nullptr };
		ID3D11Resource* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_pRenderTargetView{ nullptr };

		ID3D11SamplerState* m_pSampler;
		ID3D11RasterizerState* m_pRasterizer;

		Texture* m_pDiffuseMap{ };
		Texture* m_pFireDiffuseMap{ };
		Texture* m_pSpecularMap{ };
		Texture* m_pNormalMap{ };
		Texture* m_pGlossinessMap{ };



		SampleState m_SampleState{ SampleState::Point };
		bool m_RenderFire{ true };

		void InitHardware() ;
		HRESULT InitializeDirectX();

		void RenderHardware() const;



		//SOFTWARE
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		BufferMode m_CurrentBufferMode{ BufferMode::Texture };
		ColorMode m_CurrentColorMode{ ColorMode::Combined };

		void InitSoftware();

		void VertexTransformationFunction();
		void RenderTraingle(int i0, int i1, int i2, std::vector<Vector2>& screenVertices,
			std::vector<Vertex_Out>& verticesOut, const std::vector<uint32_t>& indeces);
		bool PositionOutsideFrustrum(const Vector4& v) const;
		ColorRGB PixelShading(const Vertex_Out& v);
		void RenderSoftware();

	};
}
