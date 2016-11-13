//
// Copyright (C) Mei Jun 2011
//

#include "NormalRenderState.h"
#include "Utility.h"

NormalRenderState::NormalRenderState( ID3D11Device* lpDevice, 
	ID3D11DeviceContext* lpDeviceContext )
{
	m_lpDevice				= lpDevice;
	m_lpDeviceContext		= lpDeviceContext;	

	m_lpBlendState			= NULL;
	m_lpSamplerState		= NULL;		
	m_lpRasterizerState		= NULL;
	m_lpDepthStencilState	= NULL;

	if( lpDevice && lpDeviceContext )
		CreateRenderStates();
}


NormalRenderState::~NormalRenderState(void)
{
	SAFE_RELEASE( m_lpRasterizerState );
	SAFE_RELEASE( m_lpDepthStencilState );
	SAFE_RELEASE( m_lpBlendState );
	SAFE_RELEASE( m_lpSamplerState );
}

BOOL NormalRenderState::CreateRenderStates()
{
	if( !CreateBlendState() )
		return FALSE;

	if( !CreateSamplerState() )
		return FALSE;

	if( !CreateRasterizerState() )
		return FALSE;

	if( !CreateDepthStencilState() )
		return FALSE;

	return TRUE;
}

BOOL NormalRenderState::CreateBlendState()
{
	D3D11_BLEND_DESC blend_desc;
	ZeroMemory( &blend_desc, sizeof(D3D11_BLEND_DESC) );
	blend_desc.RenderTarget[0].BlendEnable			 = TRUE;
	blend_desc.RenderTarget[0].SrcBlend				 = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend			 = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp				 = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha		 = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlendAlpha		 = D3D11_BLEND_DEST_ALPHA;
	blend_desc.RenderTarget[0].BlendOpAlpha			 = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	HRESULT hr = m_lpDevice->CreateBlendState( &blend_desc, &m_lpBlendState );	
	if( FAILED(hr) )
		return FALSE;
	else
		return TRUE;
}

BOOL NormalRenderState::CreateSamplerState()
{
	D3D11_SAMPLER_DESC sample_desc;
	ZeroMemory( &sample_desc, sizeof(sample_desc) );
	sample_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sample_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sample_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sample_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sample_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sample_desc.MaxAnisotropy = 16;
	sample_desc.MaxLOD =  FLT_MAX;
	sample_desc.MinLOD = -FLT_MIN;

	HRESULT hr = m_lpDevice->CreateSamplerState( &sample_desc, &m_lpSamplerState );
	if( FAILED(hr) )
		return FALSE;
	else
		return TRUE;
}

BOOL NormalRenderState::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rs_desc;
	ZeroMemory( &rs_desc, sizeof(D3D11_RASTERIZER_DESC) );
	rs_desc.CullMode			  = D3D11_CULL_BACK;
	rs_desc.FillMode			  = D3D11_FILL_SOLID;
	rs_desc.DepthClipEnable		  = TRUE;

	HRESULT hr = m_lpDevice->CreateRasterizerState( &rs_desc, &m_lpRasterizerState );
	if( FAILED(hr) )
		return FALSE;
	else
		return TRUE;
}

BOOL NormalRenderState::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC ds_desc;
	ZeroMemory( &ds_desc, sizeof(D3D11_DEPTH_STENCIL_DESC) );
	ds_desc.DepthEnable    = TRUE;
	ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds_desc.DepthFunc	   = D3D11_COMPARISON_LESS_EQUAL;

	HRESULT hr = m_lpDevice->CreateDepthStencilState( &ds_desc, &m_lpDepthStencilState );	
	if( FAILED(hr) )
		return FALSE;
	else
		return TRUE;
}
