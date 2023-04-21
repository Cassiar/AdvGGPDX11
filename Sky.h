#pragma once

#include <memory>

#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <wrl/client.h> // Used for ComPtr

class Sky
{
public:

	// Constructor that loads a DDS cube map file
	Sky(
		const wchar_t* cubemapDDSFile, 
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 	
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> irradiancePS,
		std::shared_ptr<SimplePixelShader> specConvPS,
		std::shared_ptr<SimplePixelShader> brdfLookupPS
	);

	// Constructor that loads 6 textures and makes a cube map
	Sky(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back,
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> irradiancePS,
		std::shared_ptr<SimplePixelShader> specConvPS,
		std::shared_ptr<SimplePixelShader> brdfLookupPS
	);

	~Sky();

	void Draw(std::shared_ptr<Camera> camera);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIrradianceIBLMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetConvolvedSpecularIBLMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBRDFLookUpMap();
	int GetMipLevels();

private:

	void InitRenderStates();

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	// Skybox related resources
	std::shared_ptr<SimpleVertexShader> skyVS;
	std::shared_ptr<SimplePixelShader> skyPS;
	
	std::shared_ptr<Mesh> skyMesh;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	//================= IBL variables ================
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceIBLMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> convolvedSpecularIBLMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brdfLookUpMap;

	//how many mip levels the IBL map has
	int mipLevels;

	//skip the last three mip levels because detail is too small
	const unsigned int numMipLevelsToSkip = 3;
	//num pixels square for each face
	const unsigned int cubeFaceSize = 256; 
	//num pixels square of the look up textures
	const unsigned int brdfSize = 256;

	void CreateIBLMaps(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> irradiancePS,
		std::shared_ptr<SimplePixelShader> specConvPS,
		std::shared_ptr<SimplePixelShader> brdfLookupPS);

	//private functions to help create IBL maps
	void IBLCreateIrradianceMap(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> irradiancePS);
	void IBLCreateConvolvedSpecularMap(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> specConvPS);
	void IBLCreateBRDFLookUpTexture(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> brdfLookupPS);
};

