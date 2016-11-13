//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include <d3d11.h>
#include "ShaderHelper.h"

class Plane
{
public:
	struct CUSTOMVERTEX
	{
		FLOAT x, y, z;	   // 世界坐标
		FLOAT normal[3];   // 法线
		FLOAT u, v;        // 纹理坐标
	};
	ID3D11InputLayout*	m_lpInputLayout;
	ID3D11Buffer*		m_lpVertexBuffer;
	ID3D11Buffer*		m_lpIndexBuffer;

	UINT m_uiElementSize;
	UINT m_uiNumIndices;
	UINT m_uiNumVertices;

public:
	Plane();
	~Plane();

	BOOL CreatePlaneX( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
		ShaderHelper* pShader, float yLength, float zLength,
		float xOffset = 0, float yOffset = 0, float zOffset = 0 );

	BOOL CreatePlaneY( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
		ShaderHelper* pShader, float xLength, float zLength,
		float xOffset = 0, float yOffset = 0, float zOffset = 0 );

	BOOL CreatePlaneZ( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
		ShaderHelper* pShader, float xLength, float yLength,
		float xOffset = 0, float yOffset = 0, float zOffset = 0 );

	BOOL CreateInputLayout( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext,
		ShaderHelper* pShader );

	VOID Render( ID3D11DeviceContext* lpContext );
};

class ScreenQuad : public Plane
{
public:
	class OrthoShader : public ShaderHelper
	{
	public:
		BOOL CreateShaders( ID3D11Device* lpDevice,
			ID3D11DeviceContext* lpContext );

	public:
		BOOL CreateConstantBuffers( ID3D11Device* lpDevice,
			ID3D11DeviceContext* lpContext );

		struct ConstantBuffer
		{
			float matWorldViewProj[16];
		};

		ConstantBuffer m_VertexConstantBuffer;
	};

public:
	OrthoShader* m_pOrthoShader;

public:
	ScreenQuad();
	~ScreenQuad();

	BOOL Create( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
		int dwWindowWidth, int dwWindowHeight, float xLength, float yLength,
		float xOffset = 0, float yOffset = 0, float zOffset = 0 );

	VOID RenderQuad( ID3D11DeviceContext* lpContext );
};