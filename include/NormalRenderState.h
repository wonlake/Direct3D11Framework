//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include <d3d11.h>
#include <float.h>

class NormalRenderState
{
public:
	NormalRenderState( ID3D11Device* lpDevice, 
		ID3D11DeviceContext* lpDeviceContext );
	~NormalRenderState(void);

public:
	BOOL CreateRenderStates();
	BOOL CreateBlendState();
	BOOL CreateSamplerState();
	BOOL CreateRasterizerState();
	BOOL CreateDepthStencilState();

public:
	ID3D11Device*				m_lpDevice;
	ID3D11DeviceContext*		m_lpDeviceContext;	

	ID3D11BlendState*			m_lpBlendState;
	ID3D11SamplerState*			m_lpSamplerState;
	ID3D11RasterizerState*		m_lpRasterizerState;
	ID3D11DepthStencilState*	m_lpDepthStencilState;
};

