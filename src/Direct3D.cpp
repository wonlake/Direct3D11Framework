//
// Copyright (C) Mei Jun 2011
//

#include "Direct3D.h"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dx11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

CDirect3D::CDirect3D()
{
	m_lpDevice			 = NULL;
	m_lpContext			 = NULL;
	m_lpSwapChain		 = NULL;

	m_lpRenderTargetView = NULL;
	m_lpDepthStencilView = NULL;

	m_pLoadResources	 = NULL;
	m_pReleaseResources  = NULL;

	m_bWindowed			 = TRUE;
	m_bLoadResources     = FALSE;

	m_fRatio             = -1.0f;
}

CDirect3D::~CDirect3D()
{
	ReleaseResources();

	m_bWindowed			 = TRUE;
	m_bLoadResources     = FALSE;

	m_fRatio             = -1.0f;

	if( m_lpDevice!= NULL )
	{
		m_lpDevice->Release();
		m_lpDevice = NULL;
	}
	if( m_lpContext != NULL )
	{
		m_lpContext->Release();
		m_lpContext = NULL;
	}
	if( m_lpSwapChain!= NULL )
	{
		m_lpSwapChain->Release();
		m_lpSwapChain = NULL;
	}
	if( m_lpRenderTargetView!= NULL )
	{
		m_lpRenderTargetView->Release();
		m_lpRenderTargetView = NULL;
	}
	if( m_lpDepthStencilView != NULL )
	{
		m_lpDepthStencilView->Release();
		m_lpDepthStencilView = NULL;
	}
}

BOOL CDirect3D::InitDirect3DDevice( HWND hwnd, DWORD width, DWORD height, 
	BOOL windowed, D3D_DRIVER_TYPE driverType )
{
	HRESULT hr = S_OK;

	m_dwWidth  = width;
	m_dwHeight = height;

	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = _countof(featureLevels);
	D3D_FEATURE_LEVEL featureLevelOut;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );

	sd.BufferCount						  = 1;
	sd.BufferDesc.Width					  = width;
	sd.BufferDesc.Height				  = height;
	sd.BufferDesc.Format				  = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator	  = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage						  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow						  = hwnd;
	sd.SampleDesc.Count					  = 4 ;
	sd.SampleDesc.Quality				  = 0;
	sd.Windowed							  = TRUE;
	sd.Flags							  = 2;

	IDXGIFactory1 * pFactory;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory) );

	UINT i = 0; 
	IDXGIAdapter* pAdapter; 
	std::vector<IDXGIAdapter*> vAdapters; 
	while(pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) 
	{ 
		DXGI_ADAPTER_DESC desc = { 0 };
		vAdapters.push_back(pAdapter);
		pAdapter->GetDesc( &desc );
		++i; 
	} 
	
	for( int idx = i - 1; idx >= 0; --idx )
	{
		hr = D3D11CreateDevice( vAdapters[idx], D3D_DRIVER_TYPE_UNKNOWN, NULL,
			0, featureLevels, numFeatureLevels,	D3D11_SDK_VERSION, 
			&m_lpDevice, &featureLevelOut, &m_lpContext );

		if( FAILED(hr) )
			continue;

		m_lpDevice->CheckMultisampleQualityLevels( sd.BufferDesc.Format, 
			sd.SampleDesc.Count, &sd.SampleDesc.Quality );
		sd.SampleDesc.Quality -= 1;

		hr = pFactory->CreateSwapChain( m_lpDevice, &sd, &m_lpSwapChain );
		if( FAILED(hr) )
		{
			SAFE_RELEASE( m_lpDevice );
			SAFE_RELEASE( m_lpDevice );
			continue;
		}
		break;
	}

	for( int idx = 0; idx < i; ++idx )
	{
		SAFE_RELEASE( vAdapters[idx] );
	}
	SAFE_RELEASE( pFactory );

	if( FAILED(hr) )
		return FALSE;
	return TRUE;
};

BOOL CDirect3D::InitDeviceContext()
{
	ID3D11Texture2D* pRenderTargetTexture;
	HRESULT hr = m_lpSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pRenderTargetTexture );
	if( FAILED(hr) )
		return hr;

	hr = m_lpDevice->CreateRenderTargetView( pRenderTargetTexture, NULL, &m_lpRenderTargetView );

	if( FAILED(hr) )
		return FALSE;

	pRenderTargetTexture->Release();

	DXGI_SWAP_CHAIN_DESC swap_desc;
	m_lpSwapChain->GetDesc( &swap_desc );

	ID3D11Texture2D *pDepthStencil = NULL;
	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width				 = m_dwWidth;
	descDepth.Height			 = m_dwHeight;
	descDepth.MipLevels			 = 1;
	descDepth.ArraySize			 = 1;
	descDepth.Format			 = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count	 = swap_desc.SampleDesc.Count;
	descDepth.SampleDesc.Quality = swap_desc.SampleDesc.Quality;

	descDepth.Usage				 = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags			 = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags	 = 0;
	descDepth.MiscFlags			 = 0;

	hr = m_lpDevice->CreateTexture2D( &descDepth, NULL, &pDepthStencil );
	if( FAILED(hr) )
		return FALSE;

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format			   = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension	   = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	hr = m_lpDevice->CreateDepthStencilView( pDepthStencil, NULL, &m_lpDepthStencilView );
	pDepthStencil->Release();

	if( FAILED(hr) )
		return FALSE;

	D3D11_VIEWPORT vp;
	vp.Width	= m_dwWidth;
	vp.Height	= m_dwHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_lpContext->RSSetViewports( 1, &vp );

	m_lpContext->OMSetRenderTargets( 1, &m_lpRenderTargetView, m_lpDepthStencilView );

	return TRUE;
}

BOOL CDirect3D::SetupResourcesCallback( BOOL (*pLoadResources)( ID3D11Device* lpDevice, 
	ID3D11DeviceContext* lpContext ),
	BOOL (*pReleaseResources)() )
{
	if( pLoadResources != NULL )
		m_pLoadResources = pLoadResources;
	if( pReleaseResources != NULL )
		m_pReleaseResources = pReleaseResources;
	return TRUE;
};

BOOL CDirect3D::Render( VOID (*pDrawScene)( ID3D11DeviceContext* lpContext) )
{
	if( m_lpContext == NULL )
		return FALSE;

	if( !m_bLoadResources )
		if( !LoadResources() )
			return FALSE;

	if( pDrawScene != NULL )
	{
		float ClearColor[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
		m_lpContext->ClearRenderTargetView( m_lpRenderTargetView, ClearColor );
		m_lpContext->ClearDepthStencilView( m_lpDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
		pDrawScene( m_lpContext );
		m_lpSwapChain->Present( 0, 0 );
	}
	return TRUE;
}

BOOL CDirect3D::LoadResources( VOID )
{
	if( m_pLoadResources != NULL )
	{
		m_bLoadResources = m_pLoadResources( m_lpDevice, m_lpContext );
		return m_bLoadResources;
	}
	m_bLoadResources = TRUE;	
	return TRUE;
}

BOOL CDirect3D::ReleaseResources( VOID )
{
	m_bLoadResources = FALSE;
	if( m_pReleaseResources != NULL )
		return m_pReleaseResources();
	return TRUE;
}

BOOL CDirect3D::Resize( DWORD width, DWORD height )
{
	if( m_lpSwapChain )
	{
		if( m_lpRenderTargetView!= NULL )
		{
			m_lpRenderTargetView->Release();
			m_lpRenderTargetView = NULL;
		}
		if( m_lpDepthStencilView != NULL )
		{
			m_lpDepthStencilView->Release();
			m_lpDepthStencilView = NULL;
		}

		m_lpSwapChain->ResizeBuffers( 1, width, height, 
			DXGI_FORMAT_R8G8B8A8_UNORM, 2 );
		m_dwWidth = width;
		m_dwHeight = height;
		
		return InitDeviceContext();
	}
	return FALSE;
}