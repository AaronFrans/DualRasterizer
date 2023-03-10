#include "pch.h"
#include "Effect.h"
#include "Texture.h"

namespace dae
{
	Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
		:m_pEffect{ LoadEffect(pDevice, assetFile) }
	{
#pragma region Techniques
		m_pRenderTechnique = m_pEffect->GetTechniqueByName("RenderTechnique");
		if (!m_pRenderTechnique->IsValid()) std::wcout << L"Technique not valid\n";
#pragma endregion

#pragma region Matrix
		m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
		if (!m_pMatWorldViewProjVariable->IsValid()) std::wcout << L"WorldViewProj not valid\n";
#pragma endregion

#pragma region Shaders
		m_pDiffuseMapResourceVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapResourceVariable->IsValid()) std::wcout << L"DiffuseMap not valid\n";
#pragma endregion

#pragma region Other
		m_pTextureSampler = m_pEffect->GetVariableByName("gTextureSampler")->AsSampler();
		if (!m_pTextureSampler->IsValid()) std::wcout << L"DiffuseMap not valid\n";

		m_pRasterizer = m_pEffect->GetVariableByName("gRasterizerState")->AsRasterizer();
		if (!m_pTextureSampler->IsValid()) std::wcout << L"DiffuseMap not valid\n";
#pragma endregion

	}

	Effect::~Effect()
	{
		if (m_pEffect)
			m_pEffect->Release();
	}

	ID3DX11Effect* Effect::GetEffect() const
	{
		return m_pEffect;
	}

	ID3DX11EffectTechnique* Effect::GetTechnique() const
	{
		return m_pRenderTechnique;
	}

	void Effect::SetWorldViewProjMatrixData(Matrix pWorldViewProjectionMatrix)
	{
		m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<const float*>(&pWorldViewProjectionMatrix));
	}

	void dae::Effect::SetWorldMatrixData(Matrix worldMatrix)
	{
		if(m_pMatWorldMatrixVariable)
		m_pMatWorldMatrixVariable->SetMatrix(reinterpret_cast<const float*>(&worldMatrix));
	}

	void dae::Effect::SetInvViewMatrixData(Matrix invViewMatrix)
	{
		if(m_pMatInvViewMatrixVariable)
		m_pMatInvViewMatrixVariable->SetMatrix(reinterpret_cast<const float*>(&invViewMatrix));
	}

	void Effect::SetDiffuseMap(Texture* pTexture)
	{
		if (m_pDiffuseMapResourceVariable)
			m_pDiffuseMapResourceVariable->SetResource(pTexture->GetShaderResourceView());
	}

	void Effect::SetSampleState(ID3D11SamplerState* pSampler)
	{
		m_pTextureSampler->SetSampler(0, pSampler);
	}

	void Effect::SetRasterizerState(ID3D11RasterizerState* pRasterizer)
	{
		m_pRasterizer->SetRasterizerState(0, pRasterizer);
	}

	ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
	{
		HRESULT result;
		ID3D10Blob* pErrorBlob{ nullptr };
		ID3DX11Effect* pEffect;

		DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
		shaderFlags |= D3DCOMPILE_DEBUG;
		shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		result = D3DX11CompileEffectFromFile
		(
			assetFile.c_str(),
			nullptr,
			nullptr,
			shaderFlags,
			0,
			pDevice,
			&pEffect,
			&pErrorBlob
		);

		if (FAILED(result))
		{
			if (pErrorBlob != nullptr)
			{
				const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

				std::wstringstream ss;
				for (unsigned int i{}; i < pErrorBlob->GetBufferSize(); ++i)
				{
					ss << pErrors[i];
				}

				OutputDebugStringW(ss.str().c_str());
				pErrorBlob->Release();
				pErrorBlob = nullptr;

				std::wcout << ss.str() << "\n";
			}
			else
			{
				std::wstringstream ss;
				ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
				std::wcout << ss.str() << "\n";
				return nullptr;
			}
		}

		return pEffect;
	}

}
