//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <D3Dcompiler.h>

#include <vector>

#include "Utility.h"

class CDirect3D
{
public:
	CDirect3D(void);
	~CDirect3D(void);

public:
	BOOL InitDirect3DDevice( HWND hwnd, DWORD width, DWORD height, 
		BOOL windowed, D3D_DRIVER_TYPE devType );
	BOOL InitDeviceContext();
	BOOL SetupSceneMatrix( FLOAT fRatio );
	BOOL SetupResourcesCallback( BOOL (*pLoadResources)( ID3D11Device* lpDevice, 
		ID3D11DeviceContext* lpContext ),
		BOOL (*pReleaseResources)() );
	BOOL Render( VOID (*pDrawScene)( ID3D11DeviceContext* lpContext ) );
	BOOL Resize( DWORD width, DWORD height );

public:
	BOOL LoadResources( VOID );
	BOOL ReleaseResources( VOID );

public:
	ID3D11Device*			m_lpDevice;
	ID3D11DeviceContext*	m_lpContext;
	IDXGISwapChain*			m_lpSwapChain;
	ID3D11RenderTargetView* m_lpRenderTargetView;
	ID3D11DepthStencilView* m_lpDepthStencilView;

	BOOL				 m_bWindowed;
	BOOL				 m_bLoadResources;
	FLOAT				 m_fRatio;

	DWORD				 m_dwWidth;
	DWORD                m_dwHeight;

	BOOL                (*m_pLoadResources)( ID3D11Device* lpDevice, 
		ID3D11DeviceContext* lpContext );
	BOOL                (*m_pReleaseResources)();
};

