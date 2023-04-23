#include "Renderer.h"
#include "Helpers.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

//currently just sets up needed variables.
//in the future it could setup shadow mapping or similar things
Renderer::Renderer(
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSamplerOptions,
	unsigned int windowWidth, unsigned int windowHeight)
{
	this->device = device;
	this->context = context;
	this->swapChain = swapChain;
	this->samplerOptions = samplerOptions;
	this->clampSamplerOptions = clampSamplerOptions;

	//call post resize since it recreates render targets
	this->PostResize(backBufferRTV, depthBufferDSV, windowWidth, windowHeight);

	//create random 4x4 texture
	const int textureSize = 4;
	const int totalPixels = textureSize * textureSize;
	XMFLOAT4 randomPixels[totalPixels] = {};
	for (int i = 0; i < totalPixels; i++) {
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), 0, 0);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randomVec));
	}

	D3D11_TEXTURE2D_DESC td = {};
	td.ArraySize = 1;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.MipLevels = 1;
	td.Height = textureSize;
	td.Width = textureSize;
	td.SampleDesc.Count = 1;

	//init data for the texture
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(XMFLOAT4) * 4;

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	device->CreateTexture2D(&td, &data, texture.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = td.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(texture.Get(), &srvDesc, randomsSRV.GetAddressOf());

	//create array of offsets ssao will use
	for (int i = 0; i < numOffsets; i++) {
		ssaoOffsets[i] = XMFLOAT4(
			(float)rand() / RAND_MAX * 2 - 1, //gets range -1 to 1
			(float)rand() / RAND_MAX * 2 - 1,
			(float)rand() / RAND_MAX, //range 0 to 1
			0);

		XMVECTOR offset = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		//scale so that more values are closer to min than max
		float scale = (float)i / numOffsets;
		XMVECTOR acceleratedScale = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1),
			XMVectorSet(1, 1, 1, 1),
			scale * scale);
		XMStoreFloat4(&ssaoOffsets[i], offset * acceleratedScale);
	}

	//create simple shader objects
	fullscreenVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), FixPath(L"FullscreenVS.cso").c_str());
	ssaoCalcPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), FixPath(L"SSAOCalculationsPS.cso").c_str());
	ssaoBlurPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), FixPath(L"SSAOBlurPS.cso").c_str());
	finalCombinePS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), FixPath(L"SSAOCombinePS.cso").c_str());
}

Renderer::~Renderer()
{
}

//create the MRTs needed for ssao post processing
void Renderer::CreateRenderTarget(
	unsigned int width, unsigned int height,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
	DXGI_FORMAT colorFormat)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;

	D3D11_TEXTURE2D_DESC texDesc = {};
	//set width and height of texture
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; //both rendering in, and sampling
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = colorFormat;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;

	device->CreateTexture2D(&texDesc, 0, tex.GetAddressOf());

	// Make the render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // This points to a Texture2D
	rtvDesc.Texture2D.MipSlice = 0;                             // Which mip are we rendering into?
	rtvDesc.Format = texDesc.Format;                // Same format as texture
	device->CreateRenderTargetView(tex.Get(), &rtvDesc, rtv.GetAddressOf());

	// Create the shader resource view using default options 
	device->CreateShaderResourceView(
		tex.Get(),     // Texture resource itself
		0,                   // Null description = default SRV options
		srv.GetAddressOf()); // ComPtr<ID3D11ShaderResourceView>
}

void Renderer::PreResize() 
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();


}

void Renderer::PostResize(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	unsigned int windowWidth, unsigned int windowHeight)
{
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
	
	//reset extra post process render targets
	for (int i = 0; i < RenderTargetType::TYPE_COUNT; i++) {
		mRTVs[i].Reset();
		mSRVs[i].Reset();
	}

	//recreate rtvs for ssao
	for (int i = 0; i < RenderTargetType::TYPE_COUNT; i++) {
		if (i == RenderTargetType::SCENE_DEPTH) {
			CreateRenderTarget(windowWidth, windowHeight,
				mRTVs[i], mSRVs[i], DXGI_FORMAT_R32_FLOAT);
		}
		else {
			CreateRenderTarget(windowWidth, windowHeight,
				mRTVs[i], mSRVs[i]);
		}
	}
}

void Renderer::FrameStart()
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	// Clear the back buffer (erases what's on the screen)
	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
	context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

	//also clear any rtvs used for post processing
	for (int i = 0; i < RenderTargetType::TYPE_COUNT; i++) {
		context->ClearRenderTargetView(mRTVs[i].Get(), bgColor);
	}
	const float red[4] = { 1,0,0,0 };
	//clear scene depths to all one
	context->ClearRenderTargetView(mRTVs[RenderTargetType::SCENE_DEPTH].Get(), red);

	// Clear the depth buffer (resets per-pixel occlusion information)
	context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Renderer::FrameEnd(bool vsync)
{
	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	
	//draw each of the render targets
	for (int i = 0; i < RenderTargetType::TYPE_COUNT; i++) {
		ImGui::Image(mSRVs[i].Get(), ImVec2(500, 300));
	}

	// Draw the UI after everything else
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	swapChain->Present(
		vsync ? 1 : 0,
		vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

	// Must re-bind buffers after presenting, as they become unbound
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	
	ID3D11ShaderResourceView* const pSRV[16] = {};
	//unbind all pixel shader slots
	context->PSSetShaderResources(0, 16, pSRV);
}

void Renderer::RenderScene(
	std::vector<std::shared_ptr<GameEntity>> entities,
	std::vector<Light> lights,
	std::shared_ptr<Sky> sky,
	std::shared_ptr<Camera> camera,
	int lightCount)
{
	const int numViews = 4;
	//bind first four render targets needed for ssao
	//create array of render target for easy binding
	ID3D11RenderTargetView* views[numViews] = {
		mRTVs[RenderTargetType::SCENE_COLORS_NO_AMBIENT].Get(),
		mRTVs[RenderTargetType::SCENE_AMBIENT].Get(),
		mRTVs[RenderTargetType::SCENE_NORMALS].Get(),
		mRTVs[RenderTargetType::SCENE_DEPTH].Get()
	};

	context->OMSetRenderTargets(numViews, views, depthBufferDSV.Get());

	// Draw all of the entities
	for (auto& ge : entities)
	{
		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since 
		// we are just using whichever shader the current entity has.  
		// Inefficient!!!
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lightCount);
		ps->SetInt("lightCount", lightCount);
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());

		//IBL vars and texs
		ps->SetInt("SpecIBLTotalMipLevels", sky->GetMipLevels());
		ps->SetShaderResourceView("BrdfLookUpMap", sky->GetBRDFLookUpMap());
		ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceIBLMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->GetConvolvedSpecularIBLMap());

		ps->CopyBufferData("perFrame");

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the sky
	sky->Draw(camera);

	//ID3D11RenderTargetView* nullViews[4] = {};

	//clear render targets so we can read from them
	//context->OMSetRenderTargets(4, nullViews, 0);

	//unbind all pixel shader slots
	//context->PSSetShaderResources(0, 16, pSRV);

	//switch to ssao calculation render target and draw
	views[0] = mRTVs[RenderTargetType::SSAO_RESULTS].Get();
	views[1] = 0;
	views[2] = 0;
	views[3] = 0;

	context->OMSetRenderTargets(numViews, views, 0);

	//call fullscreen vs and ssao calc ps
	fullscreenVS->SetShader();
	ssaoCalcPS->SetShader();
	ssaoCalcPS->SetMatrix4x4("viewMatrix", camera->GetView());
	ssaoCalcPS->SetMatrix4x4("projMatrix", camera->GetProjection());
	ssaoCalcPS->SetMatrix4x4("invProjMatrix", camera->GetInverseProjection());
	ssaoCalcPS->SetData("offsets", ssaoOffsets, ARRAYSIZE(ssaoOffsets) * sizeof(XMFLOAT4));
	ssaoCalcPS->SetFloat("ssaoRadius", ssaoRadius);
	ssaoCalcPS->SetInt("ssaoSamples", numOffsets);
	ssaoCalcPS->SetFloat2("randomTextureScreenScale", XMFLOAT2(windowWidth / 4.0f, windowHeight / 4.0f));

	ssaoCalcPS->SetShaderResourceView("Normals", mSRVs[RenderTargetType::SCENE_NORMALS]);
	ssaoCalcPS->SetShaderResourceView("Depths", mSRVs[RenderTargetType::SCENE_DEPTH]);
	ssaoCalcPS->SetShaderResourceView("Randoms", randomsSRV);

	ssaoCalcPS->SetSamplerState("BasicSampler", samplerOptions);
	ssaoCalcPS->SetSamplerState("ClampSampler", clampSamplerOptions);
	
	ssaoCalcPS->CopyAllBufferData();

	context->Draw(3, 0);


	//ID3D11RenderTargetView* nullView = {};

	//unbind all pixel shader slots
	//context->PSSetShaderResources(0, 16, pSRV);

	//context->OMSetRenderTargets(1, &nullView, 0);//clear
	views[0] = mRTVs[RenderTargetType::SSAO_BLUR].Get();
	//and move to blur stage
	context->OMSetRenderTargets(1, views, 0);

	fullscreenVS->SetShader();
	ssaoBlurPS->SetShader();
	
	ssaoBlurPS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ssaoBlurPS->SetShaderResourceView("SSAO", mSRVs[RenderTargetType::SSAO_RESULTS]);
	ssaoBlurPS->SetSamplerState("ClampSampler", clampSamplerOptions);

	ssaoBlurPS->CopyAllBufferData();

	context->Draw(3, 0);

	//switch to final buffer to put results on screen
	//unbind all pixel shader slots
	//context->PSSetShaderResources(0, 16, pSRV);

	//context->OMSetRenderTargets(1, &nullView, 0);//clear
	views[0] = backBufferRTV.Get();
	//and move to blur stage
	context->OMSetRenderTargets(1, views, 0);

	fullscreenVS->SetShader();
	finalCombinePS->SetShader();

	finalCombinePS->SetShaderResourceView("SceneColorNoAmbient", mSRVs[RenderTargetType::SCENE_COLORS_NO_AMBIENT]);
	finalCombinePS->SetShaderResourceView("Ambient", mSRVs[RenderTargetType::SCENE_AMBIENT]);
	finalCombinePS->SetShaderResourceView("SSAOBlur", mSRVs[RenderTargetType::SSAO_BLUR]);

	finalCombinePS->SetSamplerState("BasicSampler", samplerOptions);

	finalCombinePS->CopyAllBufferData();

	context->Draw(3, 0);

	// Draw the light sources?
	//if (showPointLights)
	//	DrawPointLights();
}
