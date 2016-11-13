//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "Camera.h"
#include "Utility.h"

#include <d3d11.h>
#include <d3dx11.h>
#include <D3Dcompiler.h>

class BoundingBox
{
public:
	BoundingBox( ID3D11Device* lpDevice, 
		ID3D11DeviceContext* lpDeviceContext );
	~BoundingBox(void);

	void SetupInput();
	BOOL SetupShaders();

	VOID SetBuffer( UINT NumIndices, VOID* pIndexBuffer, UINT IndexDataStride,
		UINT NumVertices, VOID* pVertexBuffer, UINT VertexDataStride );

	VOID Render( XMMATRIX* pWorldViewProjMatrix );

	BOOL CreateRenderState();

public:

	ID3D11Device*				m_lpDevice;

	ID3D11DeviceContext*		m_lpDeviceContext;

	ID3D11Buffer*				m_lpVertexBuffer;

	ID3D11Buffer*				m_lpIndexBuffer;

	DXGI_FORMAT					m_IndexFormat;

	UINT						m_NumIndices;

public:
	static ID3D11InputLayout*			m_lpInputLayout;

	static ID3D11VertexShader*			m_lpVertexShader;

	static ID3D11PixelShader*			m_lpPixelShader;

	static ID3D11GeometryShader*		m_lpGeometryShader;

	static ID3D11Buffer*				m_lpVSConstBufferWorldViewProj;

	static DWORD						m_dwShaderBytecodeSize;

	static VOID*						m_lpShaderBytecodeInput;

	static DWORD						m_Reference;

	static ID3D11RasterizerState*		m_lpRasterizerState;

public:
	struct ConstantBuffer
	{
		float matWorldViewProj[16];
		float matWorldView[16];
	};

	struct VERTEX_DATA
	{
		float x, y, z;
	};

	static ConstantBuffer s_Buffer;
};

