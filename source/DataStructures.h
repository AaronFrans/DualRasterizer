#pragma once
#include "Vector3.h"
#include "ColorRGB.h"
#include <d3dx11effect.h>

namespace dae
{
	class Effect;
	class Texture;


	struct Vertex_In
	{
		Vector3 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 tangent{};
		Vector3 normal{};
		Vector3 viewDirection{};
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	class Mesh final
	{
	public:
		Mesh(ID3D11Device* pDevice, const std::vector<Vertex_In>& vertices, const std::vector<uint32_t>& indices, Effect* effect);
		~Mesh();

		Mesh(const Mesh& other) = delete;
		Mesh& operator=(const Mesh& other) = delete;
		Mesh(Mesh&& other) = delete;
		Mesh& operator=(Mesh&& other) = delete;

		void Render(ID3D11DeviceContext* pDeviceContext, Matrix worldView, Matrix invView) const;

		void SetSampleState(ID3D11SamplerState* pSampler);
		void SetRasterizerState(ID3D11RasterizerState* pRasterizer);

		Matrix GetWorldMatrix() const;
		void  SetWorldMatrix(const Matrix& m_WorldMatrix);

		void RotateMesh(float rotation);


		PrimitiveTopology GetPrimitiveTopoligy() const { return m_PrimitiveTopology; };
		std::vector<Vertex_In> GetVerticesIn() const { return m_VerticesIn; };
		std::vector<Vertex_Out>& GetVerticesOutReference() { return m_VerticesOut; };
		std::vector<uint32_t> GetIndeces() const { return m_Indices; };

	private:
		//SHARED
		Matrix m_WorldMatrix{};

		//HARDWARE
		Effect* m_pEffect{};

		ID3D11InputLayout* m_pInputLayout{};

		ID3D11Buffer* m_pVertexBuffer{};

		uint32_t m_NumIndices{};
		ID3D11Buffer* m_pIndexBuffer{};


		void InitHardware(ID3D11Device* pDevice);

		//SOFTWARE
		std::vector<Vertex_In> m_VerticesIn{};
		std::vector<uint32_t> m_Indices{};
		PrimitiveTopology m_PrimitiveTopology{ PrimitiveTopology::TriangleList };
		std::vector<Vertex_Out> m_VerticesOut{};

		void InitSoftware(const std::vector<Vertex_In>& vertices, const std::vector<uint32_t>& indices);
	};
}

