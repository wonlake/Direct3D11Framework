//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "ShaderHelper.h"

class NormalShader : public ShaderHelper
{
public:
	struct ConstantBuffer
	{
		float matWorldViewProj[16];
		float matWorldView[16];
	};

	ConstantBuffer m_VertexConstantBuffer;

public:
	BOOL CreateShaders( ID3D11Device* lpDevice,
		ID3D11DeviceContext* lpContext );

	BOOL CreateConstantBuffers( ID3D11Device* lpDevice,
		ID3D11DeviceContext* lpContext );
};