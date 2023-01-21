#pragma once

namespace dae
{
	class Texture;
	class Effect
	{
	public:

		Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~Effect();

		Effect(const Effect& other) = delete;
		Effect& operator=(const Effect& other) = delete;
		Effect(Effect&& other) = delete;
		Effect& operator=(Effect&& other) = delete;

		ID3DX11Effect* GetEffect() const;
		ID3DX11EffectTechnique* GetTechnique() const;

		void SetWorldViewProjMatrixData(Matrix worldViewProjectionMatrix);
		void SetWorldMatrixData(Matrix worldMatrix);
		void SetInvViewMatrixData(Matrix invViewMatrix);

		void SetDiffuseMap(Texture* pTexture);

		void SetSampleState(ID3D11SamplerState* pSampler);
		void SetRasterizerState(ID3D11RasterizerState* pRasterizer);

	protected:


		ID3DX11Effect* m_pEffect{};
		ID3DX11EffectTechnique* m_pRenderTechnique{};

		ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{};
		ID3DX11EffectMatrixVariable* m_pMatWorldMatrixVariable{};
		ID3DX11EffectMatrixVariable* m_pMatInvViewMatrixVariable{};

		ID3DX11EffectRasterizerVariable* m_pRasterizer;
		ID3DX11EffectSamplerVariable* m_pTextureSampler;


		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapResourceVariable{};

		ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
	};
}

