//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include<d3d11.h>
#include<D3Dcompiler.h>

#include "Utility.h"

class ShaderHelper
{
public:
	ID3D11VertexShader*		m_lpVertexShader;
	ID3D11PixelShader*		m_lpPixelShader;
	ID3D11GeometryShader*	m_lpGeometryShader;
	
	ID3D11Buffer*			m_lpVertexConstantBuffer;
	ID3D11Buffer*			m_lpPixelConstantBuffer;
	ID3D11Buffer*			m_lpGeometryConstantBuffer;

	BYTE*					m_lpShaderBytecode;
	DWORD					m_dwShaderBytecodeSize;

public:
	ShaderHelper()
	{
		m_lpVertexShader			= NULL;
		m_lpPixelShader				= NULL;
		m_lpGeometryShader			= NULL;
		
		m_lpVertexConstantBuffer	= NULL;
		m_lpPixelConstantBuffer		= NULL;
		m_lpGeometryConstantBuffer  = NULL;

		m_lpShaderBytecode			= NULL;
		m_dwShaderBytecodeSize		= 0;
	}

	virtual ~ShaderHelper()
	{
		SAFE_RELEASE( m_lpVertexShader );
		SAFE_RELEASE( m_lpPixelShader );
		SAFE_RELEASE( m_lpGeometryShader );

		SAFE_RELEASE( m_lpVertexConstantBuffer );
		SAFE_RELEASE( m_lpPixelConstantBuffer );
		SAFE_RELEASE( m_lpGeometryConstantBuffer );

		SAFE_DELETEARRAY( m_lpShaderBytecode );
		m_dwShaderBytecodeSize = 0;
	}
};