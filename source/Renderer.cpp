#include "pch.h"
#include "Renderer.h"
#include "DataStructures.h"
#include "EffectShader.h"
#include "EffectTransparency.h"
#include "Texture.h"
#include "Utils.h"

#define USE_OBJ

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		SDL_GetWindowSize(m_pWindow, &m_Width, &m_Height);

		m_Camera.Initialize(45.f, { .0f,.0f, 0.f }, static_cast<float>(m_Width) / m_Height);

		InitHardware();

		m_pDiffuseMap = Texture::LoadFromFile("Resources/vehicle_diffuse.png", m_pDevice);
		m_pFireDiffuseMap = Texture::LoadFromFile("Resources/fireFX_diffuse.png", m_pDevice);
		m_pGlossinessMap = Texture::LoadFromFile("Resources/vehicle_gloss.png", m_pDevice);
		m_pNormalMap = Texture::LoadFromFile("Resources/vehicle_normal.png", m_pDevice);
		m_pSpecularMap = Texture::LoadFromFile("Resources/vehicle_specular.png", m_pDevice);

		InitMeshes();

		InitSoftware();

	}

	Renderer::~Renderer()
	{
		for (auto& mesh : m_pMeshes)
		{
			delete mesh;
		}

		delete m_pDiffuseMap;
		delete m_pFireDiffuseMap;
		delete m_pSpecularMap;
		delete m_pNormalMap;
		delete m_pGlossinessMap;

		if (m_pRenderTargetView)
		{
			m_pRenderTargetView->Release();
		}
		if (m_pRenderTargetBuffer)
		{
			m_pRenderTargetBuffer->Release();
		}
		if (m_pDepthStencilView)
		{
			m_pDepthStencilView->Release();
		}
		if (m_pDepthStencilBuffer)
		{
			m_pDepthStencilBuffer->Release();
		}
		if (m_pSwapChain)
		{
			m_pSwapChain->Release();
		}
		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}
		if (m_pDevice)
		{
			m_pDevice->Release();
		}


		delete[] m_pDepthBufferPixels;

	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_Camera.Update(pTimer);

		if (m_IsRotating)
		{
			const float meshRotation{ 45.0f * pTimer->GetElapsed() * TO_RADIANS };
			for (auto& mesh : m_pMeshes)
			{
				mesh->RotateMesh(meshRotation);
			}
		}
	}

	void Renderer::Render()
	{
		switch (m_CurrentRenderMode)
		{
		case dae::Renderer::RenderMode::Hardware:
			RenderHardware();
			break;
		case dae::Renderer::RenderMode::Software:
			RenderSoftware();
			break;
		}

	}

	void Renderer::ToggleSampleState()
	{
		if (m_CurrentRenderMode == RenderMode::Software)
			return;

		m_SampleState = static_cast<SampleState>((static_cast<int>(m_SampleState) + 1) % (static_cast<int>(SampleState::Anisotropic) + 1));

		D3D11_FILTER filter{};
		switch (m_SampleState)
		{
		case SampleState::Point:
			filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			std::cout << "Current Sample State: Point\n";
			break;
		case SampleState::Linear:
			filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			std::cout << "Current Sample State: Linear\n";
			break;
		case SampleState::Anisotropic:
			filter = D3D11_FILTER_ANISOTROPIC;
			std::cout << "Current Sample State: Anisotropic\n";
			break;
		}

		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.Filter = filter;

		if (m_pSampler) m_pSampler->Release();
		m_pDevice->CreateSamplerState(&samplerDesc, &m_pSampler);

		for (auto& mesh : m_pMeshes)
		{
			mesh->SetSampleState(m_pSampler);
		}

	}

	void Renderer::ToggleDepthBuffer()
	{
		if (m_CurrentRenderMode == RenderMode::Hardware)
			return;
		m_CurrentBufferMode = static_cast<BufferMode>((static_cast<int>(m_CurrentBufferMode) + 1) % (static_cast<int>(BufferMode::Depth) + 1));
		switch (m_CurrentBufferMode)
		{
		case dae::Renderer::BufferMode::Texture:
			std::cout << "Show Depth Buffer: OFF\n";
			break;
		case dae::Renderer::BufferMode::Depth:
			std::cout << "Show Depth Buffer: ON\n";
			break;
		}
	}

	void Renderer::ToggleColorMode()
	{
		if (m_CurrentRenderMode == RenderMode::Hardware)
			return;
		m_CurrentColorMode = static_cast<ColorMode>((static_cast<int>(m_CurrentColorMode) + 1) % (static_cast<int>(ColorMode::Combined) + 1));
		switch (m_CurrentColorMode)
		{
		case dae::Renderer::ColorMode::ObservedArea:
			std::cout << "Current Shading Mode: Observed Area\n";
			break;
		case dae::Renderer::ColorMode::Diffuse:
			std::cout << "Current Shading Mode: Diffuse\n";
			break;
		case dae::Renderer::ColorMode::Specular:
			std::cout << "Current Shading Mode: Specular\n";
			break;
		case dae::Renderer::ColorMode::Combined:
			std::cout << "Current Shading Mode: Combined\n";
			break;
		}
	}

	void Renderer::ToggleNormals()
	{
		if (m_CurrentRenderMode == RenderMode::Hardware)
			return;
		m_UseNormalMap = !m_UseNormalMap;
		switch (m_UseNormalMap)
		{
		case true:
			std::cout << "Use Normal Map: ON\n";
			break;
		case false:
			std::cout << "Use Normal Map: OFF\n";
			break;
		}
	}

	void Renderer::ToggleRotation()
	{
		m_IsRotating = !m_IsRotating;
		switch (m_IsRotating)
		{
		case true:
			std::cout << "Rotate Mesh: ON\n";
			break;
		case false:
			std::cout << "Rotate Mesh: OFF\n";
			break;
		}
	}

	void Renderer::ToggleRenderMode()
	{
		m_CurrentRenderMode = static_cast<RenderMode>((static_cast<int>(m_CurrentRenderMode) + 1) % (static_cast<int>(RenderMode::Software) + 1));
		m_Camera.ToggleCameraMode();
		switch (m_CurrentRenderMode)
		{
		case dae::Renderer::RenderMode::Hardware:
			std::cout << "Current Render Mode: Hardware\n";
			break;
		case dae::Renderer::RenderMode::Software:
			std::cout << "Current Render Mode: Software\n";
			break;
		}
	}

	void Renderer::ToggleClearColor()
	{
		m_UseUniformColor = !m_UseUniformColor;
		switch (m_UseUniformColor)
		{
		case true:
			std::cout << "Use uniform color: ON\n";
			break;
		case false:
			std::cout << "Use uniform color: OFF\n";
			break;
		}
	}

	void Renderer::ToggleCullMode()
	{

		m_CurrentCullMode = static_cast<CullMode>((static_cast<int>(m_CurrentCullMode) + 1) % (static_cast<int>(CullMode::None) + 1));

		D3D11_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.FrontCounterClockwise = false;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.ScissorEnable = false;
		rasterizerDesc.MultisampleEnable = false;
		rasterizerDesc.AntialiasedLineEnable = false;

		switch (m_CurrentCullMode)
		{
		case dae::Renderer::CullMode::Back:
			rasterizerDesc.CullMode = D3D11_CULL_BACK;
			std::cout << "Current Cull Mode: Back\n";
			break;
		case dae::Renderer::CullMode::Front:
			rasterizerDesc.CullMode = D3D11_CULL_FRONT;
			std::cout << "Current Cull Mode: Front\n";
			break;
		case dae::Renderer::CullMode::None:
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			std::cout << "Current Cull Mode: None\n";
			break;
		}


		if (m_pRasterizer)m_pRasterizer->Release();

		m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizer);


		m_pMeshes[0]->SetRasterizerState(m_pRasterizer);

	}

	void Renderer::ToggleFire()
	{
		if (m_CurrentRenderMode == RenderMode::Software)
			return;

		m_RenderFire = !m_RenderFire;
		switch (m_RenderFire)
		{
		case true:
			std::cout << "Render Fire FX: ON\n";
			break;
		case false:
			std::cout << "Render Fire FX: OFF\n";
			break;
		}

	}

	void Renderer::ToggleDrawBoundingBox()
	{
		if (m_CurrentRenderMode == RenderMode::Hardware)
			return;
		m_DrawBoundingBox = !m_DrawBoundingBox;
		switch (m_DrawBoundingBox)
		{
		case true:
			std::cout << "Draw Bounding Boxes: ON\n";
			break;
		case false:
			std::cout << "Draw Bounding Boxes: OFF\n";
			break;
		}

	}

	void Renderer::InitHardware()
	{
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";

		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}
	}

	HRESULT Renderer::InitializeDirectX()
	{
		//Creat device context
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);

		if (FAILED(result))
			return result;

		//Create DXGI Factory

		IDXGIFactory* pDXGIFactory{};

		result = CreateDXGIFactory1(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&pDXGIFactory));

		if (FAILED(result))
			return result;

		//Create Swapchain

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;


		//Get The Handle (HWND) from SDL Backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_VERSION(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);

		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		result = pDXGIFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);

		if (FAILED(result))
			return result;


		//Create DepthStencil & DepthStencilView

		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;


		//View

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);

		if (FAILED(result))
			return result;

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);

		if (FAILED(result))
			return result;

		// Create Rendertarget

		//resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));

		if (FAILED(result))
			return result;

		//view
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);

		if (FAILED(result))
			return result;

		//Bind RTV & DSV to Output Merger Stage

		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//Set Viewport

		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		if (pDXGIFactory)
		{
			pDXGIFactory->Release();
		}

		return S_OK;
	}

	Matrix Renderer::MakeWorldMatrix()
	{
		const Vector3 position{ m_Camera.origin + Vector3{ 0, 0, 50 } };
		const Vector3 rotation{ };
		const Vector3 scale{ Vector3{ 1, 1, 1 } };
		return Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);
	}

	void Renderer::InitMeshes()
	{
#ifdef USE_OBJ


		const Matrix worldMatrix = MakeWorldMatrix();

#pragma region Vehicle
		std::vector<uint32_t> indecesVehicle;
		std::vector<Vertex_In> verticesVehicle;
		Utils::ParseOBJ("Resources/vehicle.obj", verticesVehicle, indecesVehicle);
		EffectShader* pShader = new EffectShader(m_pDevice, L"Resources/Shader.fx");

		pShader->SetDiffuseMap(m_pDiffuseMap);
		pShader->SetNormalMap(m_pNormalMap);
		pShader->SetGlossinessMap(m_pGlossinessMap);
		pShader->SetSpecularMap(m_pSpecularMap);

		Mesh* pMeshVehicle = new Mesh(m_pDevice, verticesVehicle, indecesVehicle, pShader);


		pMeshVehicle->SetWorldMatrix(worldMatrix);

		m_pMeshes.emplace_back(pMeshVehicle);
#pragma endregion

#pragma region Fire
		std::vector<uint32_t> indecesFire;
		std::vector<Vertex_In> verticesFire;
		Utils::ParseOBJ("Resources/fireFX.obj", verticesFire, indecesFire);

		EffectTransparency* pTransparent = new EffectTransparency(m_pDevice, L"Resources/Transparency.fx");
		pTransparent->SetDiffuseMap(m_pFireDiffuseMap);

		Mesh* pMeshFire = new Mesh(m_pDevice, verticesFire, indecesFire, pTransparent);

		pMeshFire->SetWorldMatrix(worldMatrix);

		m_pMeshes.emplace_back(pMeshFire);
#pragma endregion




#else
		std::vector<Vertex_In> vertices{
		Vertex_In{
			{-3.f,3.f,-2.f},
			{1},
			{0, 0}
		},
		Vertex_In{
			{0.f,3.f,-2.f},
			{1},
			{0.5f, 0}},
		Vertex_In{
			{3.f,3.f,-2.f},
			{1},
			{1, 0}},
		Vertex_In{
			{-3.f,0.f,-2.f},
			{1},
			{0, 0.5f}},
		Vertex_In{
			{0.f,0.f,-2.f},
			{1},
			{0.5f, 0.5f}},
		Vertex_In{
			{3.f,0.f,-2.f},
			{1},
			{1, 0.5f}},
		Vertex_In{
			{-3.f,-3.f,-2.f},
			{1},
			{0, 1}},
		Vertex_In{
			{0.f,-3.f,-2.f},
			{1},
			{0.5f, 1}},
		Vertex_In{
			{3.f,-3.f,-2.f},
			{1},
			{1,1}}
		};


		std::vector<uint32_t> indices{
			3,0,1,	1,4,3,	4,1,2,
			2,5,4,	6,3,4,	4,7,6,
			7,4,5,	7,5,8
		};

		m_pMesh = new Mesh{ m_pDevice, vertices, indices };
#endif // DEBUG



	}

	void Renderer::RenderHardware() const
	{
		if (!m_IsInitialized)
			return;


		ColorRGB clearColor;
		m_UseUniformColor ? clearColor = { 0.1f, 0.1f, 0.1f } : clearColor = { .39f, .59f, .93f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		const Matrix worldViewProjectionMatrix{ m_pMeshes[0]->GetWorldMatrix() * m_Camera.viewMatrix * m_Camera.projectionMatrix };

		m_pMeshes[0]->Render(m_pDeviceContext, worldViewProjectionMatrix, m_Camera.invViewMatrix);
		if (m_RenderFire)
		{
			m_pMeshes[1]->Render(m_pDeviceContext, worldViewProjectionMatrix, m_Camera.invViewMatrix);
		}


		m_pSwapChain->Present(0, 0);

	}

	void Renderer::InitSoftware()
	{
		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(m_pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

	}

	void Renderer::VertexTransformationFunction()
	{
		std::vector<Vertex_Out>& verticesOut = m_pMeshes[0]->GetVerticesOutReference();
		verticesOut.clear();
		verticesOut.reserve(m_pMeshes[0]->GetVerticesIn().size());

		const Matrix meshWorldMatrix = m_pMeshes[0]->GetWorldMatrix();

		const Matrix worldViewProjectionMatrix{ meshWorldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

		for (const Vertex_In& vertex : m_pMeshes[0]->GetVerticesIn())
		{
			Vertex_Out vertexOut{ {}, vertex.color, vertex.uv, vertex.normal, vertex.normal };
			vertexOut.position = worldViewProjectionMatrix.TransformPoint({ vertex.position, 1.0f });


			vertexOut.viewDirection = Vector3{ vertexOut.position.x, vertexOut.position.y, vertexOut.position.z }.Normalized();

			vertexOut.normal = meshWorldMatrix.TransformVector(vertex.normal);
			vertexOut.tangent = meshWorldMatrix.TransformVector(vertex.tangent);


			vertexOut.position.x /= vertexOut.position.w;
			vertexOut.position.y /= vertexOut.position.w;
			vertexOut.position.z /= vertexOut.position.w;

			verticesOut.emplace_back(vertexOut);
		}
	}

	void Renderer::RenderTraingle(int i0, int i1, int i2, std::vector<Vector2>& screenVertices,
		std::vector<Vertex_Out>& verticesOut, const std::vector<uint32_t>& indeces)
	{

		if (PositionOutsideFrustrum(verticesOut[indeces[i0]].position) ||
			PositionOutsideFrustrum(verticesOut[indeces[i1]].position) ||
			PositionOutsideFrustrum(verticesOut[indeces[i2]].position))
			return;

		const Vector2& vertex0{ screenVertices[indeces[i0]] };
		const Vector2& vertex1{ screenVertices[indeces[i1]] };
		const Vector2& vertex2{ screenVertices[indeces[i2]] };

		const Vector2 edge0{ vertex1 - vertex0 };
		const Vector2 edge1{ vertex2 - vertex1 };
		const Vector2 edge2{ vertex0 - vertex2 };

		const float triangleArea{ Vector2::Cross(edge0, edge1) };


		const Vector2 minBB{ Vector2::Min(vertex0, Vector2::Min(vertex1, vertex2)) };
		const Vector2 maxBB{ Vector2::Max(vertex0, Vector2::Max(vertex1, vertex2)) };


		const int startX{ std::clamp(static_cast<int>(minBB.x) - 1, 0, m_Width) };
		const int startY{ std::clamp(static_cast<int>(minBB.y) - 1, 0, m_Height) };
		const int endX{ std::clamp(static_cast<int>(maxBB.x) + 1, 0, m_Width) };
		const int endY{ std::clamp(static_cast<int>(maxBB.y) + 1, 0, m_Height) };


		for (int px{ startX }; px < endX; ++px)
		{
			for (int py{ startY }; py < endY; ++py)
			{
				const int pixelIndex{ px + py * m_Width };
				const Vector2 currentPixel{ static_cast<float>(px), static_cast<float>(py) };

				if (m_DrawBoundingBox)
				{
					m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(255),
						static_cast<uint8_t>(255),
						static_cast<uint8_t>(255));
					continue;
				}

				const Vector2 vertex0ToPixel{ currentPixel - vertex0 };
				const Vector2 vertex1ToPixel{ currentPixel - vertex1 };
				const Vector2 vertex2ToPixel{ currentPixel - vertex2 };

				const float edge0PixelCross{ Vector2::Cross(edge0, vertex0ToPixel) };
				const float edge1PixelCross{ Vector2::Cross(edge1, vertex1ToPixel) };
				const float edge2PixelCross{ Vector2::Cross(edge2, vertex2ToPixel) };


				switch (m_CurrentCullMode)
				{
				case dae::Renderer::CullMode::Back:
				{
					const bool doesHitFront{ edge0PixelCross > 0 && edge1PixelCross > 0 && edge2PixelCross > 0 };
					if (!doesHitFront)
						continue;
				}
				break;
				case dae::Renderer::CullMode::Front:
				{
					const bool doesHitBack{ edge0PixelCross < 0 && edge1PixelCross < 0 && edge2PixelCross < 0 };
					if (!doesHitBack)
						continue;
				}
				break;
				case dae::Renderer::CullMode::None:
				{
					const bool doesHitFront{ edge0PixelCross > 0 && edge1PixelCross > 0 && edge2PixelCross > 0 };
					const bool doesHitBack{ edge0PixelCross < 0 && edge1PixelCross < 0 && edge2PixelCross < 0 };
					if (!doesHitBack && !doesHitFront)
						continue;
				}
				break;
				}


				const float weightV0{ edge1PixelCross / triangleArea };
				const float weightV1{ edge2PixelCross / triangleArea };
				const float weightV2{ edge0PixelCross / triangleArea };


				const float interpolatedZDepth
				{
					1.0f /
						(weightV0 / verticesOut[indeces[i0]].position.z +
						weightV1 / verticesOut[indeces[i1]].position.z +
						weightV2 / verticesOut[indeces[i2]].position.z)
				};


				if (m_pDepthBufferPixels[pixelIndex] < interpolatedZDepth)
					continue;

				m_pDepthBufferPixels[pixelIndex] = interpolatedZDepth;


				switch (m_CurrentBufferMode)
				{
				case dae::Renderer::BufferMode::Texture:
				{

					const Vertex_Out& v0 = verticesOut[indeces[i0]];
					const Vertex_Out& v1 = verticesOut[indeces[i1]];
					const Vertex_Out& v2 = verticesOut[indeces[i2]];

					Vertex_Out interpolatedVertex{};

					const float interpolatedWWeight
					{
						1.0f / (
							weightV0 / v0.position.w +
							weightV1 / v1.position.w +
							weightV2 / v2.position.w
							)
					};

					// uv


					const Vector2 uvInterpolated0{ weightV0 * (v0.uv / v0.position.w) };
					const Vector2 uvInterpolated1{ weightV1 * (v1.uv / v1.position.w) };
					const Vector2 uvInterpolated2{ weightV2 * (v2.uv / v2.position.w) };

					interpolatedVertex.uv = { (uvInterpolated0 + uvInterpolated1 + uvInterpolated2) * interpolatedWWeight };


					//color
					interpolatedVertex.color = v0.color * weightV0 + v1.color * weightV1 + v2.color * weightV2;

					//normal
					const Vector3 normalInterpolated0{ weightV0 * (v0.normal / v0.position.w) };
					const Vector3 normalInterpolated1{ weightV1 * (v1.normal / v1.position.w) };
					const Vector3 normalInterpolated2{ weightV2 * (v2.normal / v2.position.w) };

					interpolatedVertex.normal = {
						(
						(normalInterpolated0 + normalInterpolated1 + normalInterpolated2)
						* interpolatedWWeight
						).Normalized() };

					//tangent
					const Vector3 tangentInterpolated0{ weightV0 * (v0.tangent / v0.position.w) };
					const Vector3 tangentInterpolated1{ weightV1 * (v1.tangent / v1.position.w) };
					const Vector3 tangentInterpolated2{ weightV2 * (v2.tangent / v2.position.w) };

					interpolatedVertex.tangent = {
						(
						(tangentInterpolated0 + tangentInterpolated1 + tangentInterpolated2)
						* interpolatedWWeight
						).Normalized() };;



					//viewDir
					const Vector3 viewDirInterpolated0{ weightV0 * (v0.viewDirection / v0.position.w) };
					const Vector3 viewDirInterpolated1{ weightV1 * (v1.viewDirection / v1.position.w) };
					const Vector3 viewDirInterpolated2{ weightV2 * (v2.viewDirection / v2.position.w) };

					interpolatedVertex.viewDirection = {
						(
						(viewDirInterpolated0 + viewDirInterpolated1 + viewDirInterpolated2)
						* interpolatedWWeight
						).Normalized() };;


					ColorRGB finalColor = PixelShading(interpolatedVertex);

					finalColor.MaxToOne();

					m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
				break;

				break;
				case dae::Renderer::BufferMode::Depth:
				{
					float depthVal = Utils::Remap(interpolatedZDepth, 0.997f, 1.0f);

					const ColorRGB finalColor{ depthVal, depthVal, depthVal };

					m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
				break;
				}




			}
		}
	}

	bool Renderer::PositionOutsideFrustrum(const Vector4& v) const
	{
		return v.x < -1.0f || v.x > 1.0f || v.y < -1.0f || v.y > 1.0f || v.z < 0.0f || v.z > 1.0f;
	}

	ColorRGB Renderer::PixelShading(const Vertex_Out& v)
	{

		Vector3 pixelNormal{ v.normal };


		if (m_UseNormalMap)
		{
			const Vector3 binormal = Vector3::Cross(v.normal, v.tangent);

			const Matrix tangentSpaceAxis = Matrix{ v.tangent, binormal, v.normal, Vector3::Zero };

			const ColorRGB currentNormalMap{ 2.0f * m_pNormalMap->Sample(v.uv) - ColorRGB{ 1.0f, 1.0f, 1.0f } };

			const Vector3 normalMapSample{ currentNormalMap.r, currentNormalMap.g, currentNormalMap.b };

			pixelNormal = tangentSpaceAxis.TransformVector(normalMapSample).Normalized();
		}

		const Vector3 lightDirection = Vector3{ .577f, -.577f, .577f }.Normalized();

		const float observedArea{ std::max(Vector3::Dot(pixelNormal, -lightDirection), 0.f) };
		const float lightIntensity{ 7.0f };
		const float glossiness{ 25.0f };


		switch (m_CurrentColorMode)
		{
		case dae::Renderer::ColorMode::ObservedArea:
			return ColorRGB{ observedArea, observedArea, observedArea };
			break;
		case dae::Renderer::ColorMode::Diffuse:
		{

			const ColorRGB lambert{ BRDF_Utils::Lambert(1.0f, m_pDiffuseMap->Sample(v.uv)) };

			return (lightIntensity * lambert) * observedArea;
		}
		break;
		case dae::Renderer::ColorMode::Specular:
		{
			const float phongExponent{ m_pGlossinessMap->Sample(v.uv).r * glossiness };

			return m_pSpecularMap->Sample(v.uv) * BRDF_Utils::Phong(1.0f, phongExponent, -lightDirection, v.viewDirection, pixelNormal);
		}
		break;
		case dae::Renderer::ColorMode::Combined:
		{
			const ColorRGB lambert{ BRDF_Utils::Lambert(1.0f, m_pDiffuseMap->Sample(v.uv)) };

			const float phongExponent{ m_pGlossinessMap->Sample(v.uv).r * glossiness };

			const ColorRGB specular{ m_pSpecularMap->Sample(v.uv) * BRDF_Utils::Phong(1.0f, phongExponent, -lightDirection, v.viewDirection, pixelNormal) };

			return (lightIntensity * lambert + specular) * observedArea;
		}
		break;
		}
	}

	void Renderer::RenderSoftware()
	{
		//@START
	//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		//clear background
		if (m_UseUniformColor)
		{
			const int color{ static_cast<int>(0.1f * 255) };
			SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, color, color, color));
		}
		else
		{
			const int color{ static_cast <int>(0.39f * 255) };
			SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, color, color, color));
		}

		//reset buffer
		const int nrPixels{ m_Width * m_Height };
		std::fill_n(m_pDepthBufferPixels, nrPixels, FLT_MAX);

		//Rasterization
		VertexTransformationFunction();

		std::vector<Vector2> screenVertices{};
		std::vector<Vertex_Out>& verticesOut = m_pMeshes[0]->GetVerticesOutReference();
		const std::vector<uint32_t> indeces = m_pMeshes[0]->GetIndeces();

		screenVertices.reserve(verticesOut.size());
		for (auto& vertex : verticesOut)
		{
			screenVertices.push_back({
				(vertex.position.x + 1) * 0.5f * m_Width,
				(1.0f - vertex.position.y) * 0.5f * m_Height
				});
		}

		//RENDER LOGIC
		switch (m_pMeshes[0]->GetPrimitiveTopoligy())
		{
		case PrimitiveTopology::TriangleList:

			for (int i{}; i < indeces.size(); i += 3)
			{
				const int index0{ i }, index1{ i + 1 }, index2{ i + 2 };

				RenderTraingle(index0, index1, index2, screenVertices, verticesOut, indeces);

			}
			break;
		case PrimitiveTopology::TriangleStrip:

			for (int i{}; i < m_pMeshes[0]->GetIndeces().size() - 2; ++i)
			{

				const int index0{ i };
				int index1{}, index2{};
				// if n&1 is 1, then odd, else even

				const bool swapIndeces = i % 2;


				index1 = i + !swapIndeces * 1 + swapIndeces * 2;
				index2 = i + !swapIndeces * 2 + swapIndeces * 1;

				RenderTraingle(index0, index1, index2, screenVertices, verticesOut, indeces);
			}
			break;
		}



		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}
}
