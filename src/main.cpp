//
// Copyright (C) Mei Jun 2011
//

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "Direct3D.h"
#include "Camera.h"
#include "Utility.h"
#include "BillboardText.h"
#include "KMZLoader.h"
#include "NormalRenderState.h"
#include "SolidwireShader.h"
#include "NormalShader.h"
#include "Plane.h"
#include "XXModel.h"

#include "resource.h"

#pragma comment( lib, "comdlg32.lib" )

#define ID_FPS	1000

//全局变量
CDirect3D g_Direct3D;
CTrackBallCamera g_Camera;

DWORD g_dwWindowWidth  = 640;
DWORD g_dwWindowHeight = 480;

ID3D11ShaderResourceView*	g_lpSRView1		   = NULL;
ID3D11ShaderResourceView*   g_lpSRView2		   = NULL;
ID3D11ShaderResourceView*	g_lpSRView3		   = NULL;
ID3D11ShaderResourceView*	g_lpSRViewCube	   = NULL;

CBillboardText*				g_pText					  = NULL;
//KMZLoader*					g_pKmzLoader			  = NULL;
CXXModel*					g_pXXModel				  = NULL;
NormalRenderState*			g_pNormalRenderState	  = NULL;
SolidwireShader*			g_pSolidwireShader		  = NULL;
NormalShader*				g_pNormalShader			  = NULL;

#define RT_SCENE	0
#define RT_DEPTH	1
#define RT_NORMAL	2
ScreenQuad*					g_pScreenQuad[3]		  = { NULL };

Plane*						g_pPlaneZ				  = NULL;

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

HWND	SetupWindow( HINSTANCE hInstance, 
					 DWORD dwWidth, DWORD dwHeight );
VOID	SetupD3DFramework( HWND hWnd , CDirect3D* lpDirect3D,
						   DWORD dwWidth, DWORD dwHeight );
INT		SetupMessageLoop( CDirect3D *lpDirect3D );
VOID	DrawScene( ID3D11DeviceContext* lpContext );
BOOL	LoadResources( ID3D11Device* lpDevice, 
					ID3D11DeviceContext* lpContext );
BOOL	ReleaseResources( VOID );

VOID    CreateCubeMapView( ID3D11Device* lpDevice, 
						   ID3D11DeviceContext* lpContext,
						   ID3D11ShaderResourceView** ppSRViewCube );
VOID    CreateTextureArrayView( ID3D11Device* lpDevice, 
							    ID3D11DeviceContext* lpContext,
							    ID3D11ShaderResourceView** ppSRViewArray );

INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd )
{

	HWND hWnd = SetupWindow( hInstance, g_dwWindowWidth, g_dwWindowHeight );
	
	if( hWnd == NULL )
		return 0;

	SetupD3DFramework( hWnd, &g_Direct3D, g_dwWindowWidth, g_dwWindowHeight );

	return SetupMessageLoop( &g_Direct3D );					
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CREATE:
		SetTimer( hWnd, ID_FPS, 1000, NULL );
		break;
	case WM_DESTROY:
		KillTimer( hWnd, ID_FPS );
		PostQuitMessage( 0 );
		break;
	case WM_TIMER:
		{
			if( g_pText )
			{
				DWORD dwFPS = g_pText->GetFPS();
				static TCHAR strFPS[20] = { 0 };
				_stprintf( strFPS, _T("当前帧速:%d"), dwFPS );
				g_pText->UpdateText( strFPS );
			}
			break;
		}
	case WM_LBUTTONDBLCLK:
		{
			OPENFILENAME ofn;
			static TCHAR FileName[MAX_PATH];
			ZeroMemory(&ofn,sizeof(OPENFILENAME));
			ofn.lStructSize     = sizeof(OPENFILENAME);
			ofn.hwndOwner       = hWnd;
			ofn.lpstrFile       = FileName;
			ofn.nMaxFile        = sizeof(FileName);
			//ofn.lpstrFilter     = TEXT(
			//						"KMZ模型文件\0*.kmz\0	\
			//						XX模型文件\0*.xx\0	\
			//						XA动画文件\0*.xa\0"
			//						);
			ofn.lpstrFilter		= TEXT( "模型\0*.kmz;*.xx;*.xa\0" );
			ofn.nFilterIndex    = 0;
			ofn.lpstrFileTitle  = NULL;
			ofn.nMaxFileTitle   = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			ofn.lpstrInitialDir = _T("..\\TestMedia\\KMZ");
			if(GetOpenFileName(&ofn) != TRUE)
				break;

			int type = -1;
			int len = _tcslen( FileName );

			if( len > 2 )
			{
				if( _tcsicmp( FileName + len - 2, _T("xx") ) == 0 )
					type = 1;
				else if( _tcsicmp( FileName + len - 2, _T("xa") ) == 0 )
					type = 2;
			}
			if( len > 3 )
			{
				if( _tcsicmp( FileName + len - 3, _T("kmz") ) == 0 )
					type = 0;
			}
			switch( type )
			{
			case 0:
				{
					SAFE_DELETE( g_pXXModel );
					//if( hwndFrameNameDlg )
					//{
					//	if( IsWindowVisible(hwndFrameNameDlg) )
					//		ShowWindow( hwndFrameNameDlg, SW_HIDE );

					//	if( IsWindowVisible(hwndAnimDlg) )
					//		ShowWindow( hwndAnimDlg, SW_HIDE );
					//}
			/*		if( g_pKmzLoader == NULL )
					{
						g_pKmzLoader = new KMZLoader( 
							g_Direct3D.m_lpDevice, g_Direct3D.m_lpContext );
					}

					if( g_pKmzLoader )
					{
						g_Camera.Reset();
						g_pKmzLoader->LoadColladaMeshFromKMZ( FileName );
						float fUnit = g_pKmzLoader->GetTranslateUnit();
						g_Camera.SetWheelUnit( fUnit );
						g_Camera.SetNearFarPlane( fUnit * 0.5f, fUnit * 100 );
					} */
					break;
				}
			case 1:
				{
					//SAFE_DELETE( g_pKmzLoader );
					if( g_pXXModel == NULL )
					{
						g_pXXModel = new CXXModel( 
							g_Direct3D.m_lpDevice, g_Direct3D.m_lpContext );
					}

					if( g_pXXModel )
					{
						g_Camera.Reset();
						int len = WideCharToMultiByte( CP_ACP, 0, FileName, -1, NULL, 0, NULL, NULL );
						char* szFileName = new char[len];
						WideCharToMultiByte( CP_ACP, 0, FileName, -1, szFileName, len, 0, 0 );
						g_pXXModel->LoadModelFromFile( szFileName );
						SAFE_DELETEARRAY( szFileName );
						float fUnit = g_pXXModel->GetTranslateUnit();
						g_Camera.SetWheelUnit( fUnit );
						g_Camera.SetNearFarPlane( fUnit * 0.5f, fUnit * 100 );
/*						if( hwndFrameNameDlg )
							SetWindowText( hwndFrameNameDlg, FileName );
						SendMessage( hwndFrameNameDlg, WM_FRAMELIST, (WPARAM)hWnd, (LPARAM)g_pXXModel );*/						
					}
					break;
				}
			case 2:
				{
					if( g_pXXModel == NULL )
					{
						g_pXXModel = new CXXModel( 
							g_Direct3D.m_lpDevice, g_Direct3D.m_lpContext );
					}

					if( g_pXXModel )
					{
						int len = WideCharToMultiByte( CP_ACP, 0, FileName, -1, NULL, 0, NULL, NULL );
						char* szFileName = new char[len];
						WideCharToMultiByte( CP_ACP, 0, FileName, -1, szFileName, len, 0, 0 );
						g_pXXModel->LoadAnimationFromFile( szFileName );
						SAFE_DELETEARRAY( szFileName );	
/*						if( hwndAnimDlg )
							SetWindowText( hwndAnimDlg, FileName );
						SendMessage( hwndAnimDlg, WM_ANIMNAMELIST, (WPARAM)hWnd, (LPARAM)g_pXXModel );*/						
					}
					break;
				}
			default:
				break;
			}			

			break;
		}
	case WM_KEYDOWN:
		{
			switch(wParam)
			{
			case VK_ESCAPE:
				DestroyWindow( hWnd );
			}
			break;
		}		
	default:
		{
			if( !g_Camera.HandleMessage( hWnd, uMsg, wParam,lParam ) )
				return DefWindowProc( hWnd, uMsg, wParam, lParam );
		}
	}
	return 0;
}

HWND SetupWindow( HINSTANCE hInstance, DWORD dwWidth, DWORD dwHeight )
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hCursor		 = (HCURSOR)LoadCursor( NULL, IDC_ARROW );
	wc.hIcon		 = (HICON)LoadIcon( hInstance, (LPCTSTR)IDI_ICON1 );
	wc.hbrBackground = NULL;
	wc.lpfnWndProc	 = (WNDPROC)WndProc;
	wc.lpszClassName = TEXT("Direct3D11");
	wc.lpszMenuName  = NULL;

	if( !RegisterClass( &wc ) )
	{
		MessageBox( NULL, TEXT("注册窗口失败！"), NULL, MB_OK );
		return NULL;
	}

	RECT rc;
	SetRect( &rc, 0, 0, g_dwWindowWidth, g_dwWindowHeight );        
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );

	HWND hWnd = CreateWindow( TEXT("Direct3D11"), 
		TEXT("Direct3D11Framework"), 
		WS_OVERLAPPEDWINDOW,
		100, 100, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, hInstance, NULL );

	if( !hWnd )
	{
		MessageBox( NULL, TEXT("创建窗口失败！"), NULL, MB_OK );
		return NULL;
	}

	ShowWindow( hWnd, SW_SHOW );
	UpdateWindow( hWnd );

	return hWnd;
}

VOID SetupD3DFramework( HWND hWnd , CDirect3D* lpDirect3D, 
					    DWORD dwWidth, DWORD dwHeight )
{
	g_Camera.Init( dwWidth, dwHeight );
	lpDirect3D->InitDirect3DDevice( hWnd, dwWidth, dwHeight,
		TRUE, D3D_DRIVER_TYPE_HARDWARE );
	lpDirect3D->InitDeviceContext();
	lpDirect3D->SetupResourcesCallback( LoadResources, ReleaseResources );
}

INT SetupMessageLoop( CDirect3D *lpDirect3D )
{
	MSG msg;
	ZeroMemory( &msg, sizeof(MSG) );
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			lpDirect3D->Render( DrawScene );
			Sleep( 10 );
		}
	}
	return 0;
}

VOID DrawScene( ID3D11DeviceContext* lpContext )
{
	static XMMATRIX matIdentity = XMMatrixIdentity();

	XMMATRIX matW = XMMatrixTranslation( 0.0f, 0.0f, 2.0f );
	XMMATRIX matV = matIdentity;
	g_Camera.GetViewMatrix( &matV );

	XMMATRIX matP = XMMatrixPerspectiveFovLH( 0.7855f, 
		(float)g_dwWindowWidth / (float)g_dwWindowHeight, 1.0f, 1000.0f );

	XMMATRIX matVW = matV * matW;

	if( g_pNormalRenderState )
	{		
		FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		lpContext->RSSetState( g_pNormalRenderState->m_lpRasterizerState );
		lpContext->PSSetSamplers( 0, 1, &g_pNormalRenderState->m_lpSamplerState );		
		lpContext->OMSetBlendState( g_pNormalRenderState->m_lpBlendState, color, 0xFFFFFFFF );
		lpContext->OMSetDepthStencilState( g_pNormalRenderState->m_lpDepthStencilState, 0 );
	}

	//if( g_pKmzLoader )
	if(false)
	{
		if( GetAsyncKeyState('B') & 0x8000f )
		{
		//	g_pKmzLoader->ShowBoundingBox( TRUE );
		}
		if( GetAsyncKeyState('N') & 0x8000f )
		{
		//	g_pKmzLoader->ShowBoundingBox( FALSE );
		}
		//g_pKmzLoader->Render( &matVW, &matP );
	}
	else if( g_pXXModel )
	{
		static DWORD last = timeGetTime();
		static DWORD cur = timeGetTime();
		static DWORD dwDiff = 0;
		static DWORD dwSinceLastUpdate = timeGetTime();
		if( cur - dwSinceLastUpdate > 40 )
		{
			dwSinceLastUpdate = cur;
			dwDiff = 40;
			g_pXXModel->UpdateBoneMatrix( (float)dwDiff / 1000.0f );
		}

		g_pXXModel->Render( &matVW, &matP );

		//for( int i = 0; i < g_vecXXModels.size(); ++i )
		//	g_vecXXModels[i]->Render( &g_Buffer.matWorldView, &g_Buffer.matProj );

		if( GetAsyncKeyState('B') & 0x8000f )
		{
			g_pXXModel->ShowBoundingBox( TRUE );
		}
		if( GetAsyncKeyState('N') & 0x8000f )
		{
			g_pXXModel->ShowBoundingBox( FALSE );
		}
		cur = timeGetTime();
	}
	else
	{
		ID3D11ShaderResourceView* view[4];
		view[0] = g_lpSRView1;
		view[1] = g_lpSRView2;
		view[2] = g_lpSRView3;
		view[3] = g_lpSRViewCube;
		lpContext->PSSetShaderResources( 0, 2, view );

		{
			lpContext->VSSetShader( g_pNormalShader->m_lpVertexShader, NULL, 0 );
			lpContext->PSSetShader( g_pNormalShader->m_lpPixelShader, NULL, 0 );
			lpContext->GSSetShader( g_pNormalShader->m_lpGeometryShader, NULL, 0 );

			memcpy( g_pNormalShader->m_VertexConstantBuffer.matWorldViewProj, &(matVW * matP), sizeof(XMMATRIX) );
			memcpy( g_pNormalShader->m_VertexConstantBuffer.matWorldView, &matVW, sizeof(XMMATRIX) );

			D3D11_MAPPED_SUBRESOURCE pData;
			lpContext->Map( g_pNormalShader->m_lpVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
			memcpy_s( pData.pData, pData.RowPitch, &g_pNormalShader->m_VertexConstantBuffer, 
				sizeof(g_pNormalShader->m_VertexConstantBuffer) );
			lpContext->Unmap( g_pNormalShader->m_lpVertexConstantBuffer, 0 );

			lpContext->VSSetConstantBuffers( 0, 1, &g_pNormalShader->m_lpVertexConstantBuffer );

			g_pPlaneZ->Render( lpContext );
		}
	}
	for( int i = 1; i < 3; ++i )
	{
		if( i == 2 )
			lpContext->PSSetShaderResources( 0, 1, &g_lpSRView2 );
		g_pScreenQuad[i]->RenderQuad( lpContext );
	}

	if( g_pText )
		g_pText->Render();
}

VOID CreateCubeMapView( ID3D11Device* lpDevice, 
						ID3D11DeviceContext* lpContext,
						ID3D11ShaderResourceView** ppSRViewCube )
{
	ID3D11Texture2D* lpCubeTexture = NULL;
	ID3D11Texture2D* lpTempTexture = NULL;
	D3D11_TEXTURE2D_DESC TempDesc;
	TCHAR* strFileNames[6] = { 
								TEXT("..\\TestMedia\\cubemap\\posx.bmp"),
								TEXT("..\\TestMedia\\cubemap\\negx.bmp"),
								TEXT("..\\TestMedia\\cubemap\\posy.bmp"),
								TEXT("..\\TestMedia\\cubemap\\negy.bmp"),
								TEXT("..\\TestMedia\\cubemap\\posz.bmp"),
								TEXT("..\\TestMedia\\cubemap\\negz.bmp")
							 };
	for( int face = 0; face < 6; ++face )
	{		
		ZeroMemory( &TempDesc, sizeof(D3D11_TEXTURE2D_DESC) );
		TempDesc.MipLevels		= 1;
		TempDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		D3DX11_IMAGE_LOAD_INFO TempLoadInfo;
		ZeroMemory( &TempLoadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO) );

		TempLoadInfo.Width			= D3DX11_FROM_FILE;
		TempLoadInfo.Height			= D3DX11_FROM_FILE;
		TempLoadInfo.Depth			= D3DX11_FROM_FILE;
		TempLoadInfo.FirstMipLevel	= 0;
		TempLoadInfo.MipLevels		= D3DX11_FROM_FILE;
		TempLoadInfo.Usage			= D3D11_USAGE_STAGING;
		TempLoadInfo.BindFlags		= 0;
		TempLoadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		TempLoadInfo.MiscFlags		= 0;
		TempLoadInfo.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
		TempLoadInfo.Filter			= D3DX11_FILTER_NONE;
		TempLoadInfo.MipFilter		= D3DX11_FILTER_NONE;

		HRESULT hr = D3DX11CreateTextureFromFile( lpDevice, strFileNames[face], &TempLoadInfo, NULL, (ID3D11Resource**)&lpTempTexture, NULL );
		lpTempTexture->GetDesc( &TempDesc );

		if( g_lpSRViewCube == NULL )
		{
			TempDesc.Usage			= D3D11_USAGE_DEFAULT;
			TempDesc.BindFlags		= D3D11_BIND_SHADER_RESOURCE;
			TempDesc.CPUAccessFlags = 0;
			TempDesc.MiscFlags		= D3D11_RESOURCE_MISC_TEXTURECUBE;
			TempDesc.ArraySize		= 6;

			hr = lpDevice->CreateTexture2D( &TempDesc, NULL, (ID3D11Texture2D**)&lpCubeTexture );

			D3D11_SHADER_RESOURCE_VIEW_DESC srvMultiDesc;
			ZeroMemory( &srvMultiDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );

			srvMultiDesc.Format = TempDesc.Format;
			srvMultiDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvMultiDesc.TextureCube.MipLevels = TempDesc.MipLevels;
			srvMultiDesc.TextureCube.MostDetailedMip = 0;
			hr = lpDevice->CreateShaderResourceView( lpCubeTexture, &srvMultiDesc, &g_lpSRViewCube );
		}
		//////////////////////更新纹理//////////////////////////////////////////
		D3D11_MAPPED_SUBRESOURCE pData;
		for( int i = 0; i < TempDesc.MipLevels; ++i )
		{
			UINT subRes1 = D3D11CalcSubresource( i, 0, TempDesc.MipLevels );
			UINT subRes2 = D3D11CalcSubresource( i, face, TempDesc.MipLevels );
			lpContext->Map( lpTempTexture, subRes1, D3D11_MAP_READ, 0, &pData );		
			lpContext->UpdateSubresource( lpCubeTexture, subRes2, NULL, 
				pData.pData, pData.RowPitch, 0 );

			lpContext->Unmap( lpTempTexture, subRes1 );
		}
		lpTempTexture->Release();
	}
	lpCubeTexture->Release();
}

VOID CreateTextureArrayView( ID3D11Device* lpDevice, 
							 ID3D11DeviceContext* lpContext,
							 ID3D11ShaderResourceView** ppSRViewArray )
{
	ID3D11Texture2D* lpTempTexture  = NULL;
	ID3D11Texture2D* lpMultiTexture = NULL;
	UINT TextureArraySize		    = 2;
	D3D11_TEXTURE2D_DESC TempDesc;
	TCHAR* strFileNames[2] = {
								TEXT("..\\GameMedia\\textures\\Water01.jpg"),
								TEXT("..\\GameMedia\\textures\\Water02.jpg")
							 };

	for( int size = 0; size < TextureArraySize; ++size )
	{
		ZeroMemory( &TempDesc, sizeof(D3D11_TEXTURE2D_DESC) );
		TempDesc.MipLevels		= 1;
		TempDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		D3DX11_IMAGE_LOAD_INFO TempLoadInfo;
		ZeroMemory( &TempLoadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO) );

		TempLoadInfo.Width			= D3DX11_FROM_FILE;
		TempLoadInfo.Height			= D3DX11_FROM_FILE;
		TempLoadInfo.Depth			= D3DX11_FROM_FILE;
		TempLoadInfo.FirstMipLevel  = 0;
		TempLoadInfo.MipLevels		= D3DX11_FROM_FILE;
		TempLoadInfo.Usage			= D3D11_USAGE_STAGING;
		TempLoadInfo.BindFlags		= 0;
		TempLoadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		TempLoadInfo.MiscFlags		= 0;
		TempLoadInfo.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
		TempLoadInfo.Filter			= D3DX11_FILTER_NONE;
		TempLoadInfo.MipFilter		= D3DX11_FILTER_NONE;

		HRESULT hr = D3DX11CreateTextureFromFile( lpDevice, strFileNames[size], &TempLoadInfo,
			NULL, (ID3D11Resource**)&lpTempTexture, NULL );
		lpTempTexture->GetDesc( &TempDesc );

		//////////////////////创建纹理数组////////////////////////////////////
		if( g_lpSRView3 == NULL )
		{
			TempDesc.Usage			= D3D11_USAGE_DEFAULT;
			TempDesc.BindFlags		= D3D11_BIND_SHADER_RESOURCE;
			TempDesc.CPUAccessFlags = 0;
			TempDesc.ArraySize		= TextureArraySize;

			hr = lpDevice->CreateTexture2D( &TempDesc, NULL, (ID3D11Texture2D**)&lpMultiTexture );

			D3D11_SHADER_RESOURCE_VIEW_DESC srvMultiDesc;
			ZeroMemory( &srvMultiDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );

			srvMultiDesc.Format					  = TempDesc.Format;
			srvMultiDesc.ViewDimension			  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvMultiDesc.Texture2DArray.MipLevels = TempDesc.MipLevels;
			srvMultiDesc.Texture2DArray.ArraySize = TextureArraySize;
			lpDevice->CreateShaderResourceView( lpMultiTexture, &srvMultiDesc, &g_lpSRView3 );
		}
		D3D11_MAPPED_SUBRESOURCE pData;
		for( int i = 0; i < TempDesc.MipLevels; ++i )
		{
			UINT subRes1 = D3D11CalcSubresource( i, 0, TempDesc.MipLevels );
			UINT subRes2 = D3D11CalcSubresource( i, size, TempDesc.MipLevels );
			lpContext->Map( lpTempTexture, subRes1, D3D11_MAP_READ, 0, &pData );		
			lpContext->UpdateSubresource( lpMultiTexture, subRes2, NULL, 
				pData.pData, pData.RowPitch, 0 );

			lpContext->Unmap( lpTempTexture, subRes1 );
		}
		lpTempTexture->Release();
	}
	lpMultiTexture->Release();
}

VOID LoadPSShaderResources( ID3D11Device* lpDevice, 
							ID3D11DeviceContext* lpContext )
{
	//////////////////////////////创建Shader Resource View/////////////////////////

	ID3D11Texture2D* lpTexture = NULL;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory( &srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	D3DX11CreateTextureFromFile( lpDevice, TEXT("..\\Res\\NineGrids.png"), NULL, NULL, (ID3D11Resource**)&lpTexture, NULL );
	lpTexture->GetDesc( &desc );

	srvDesc.Format					  = desc.Format;
	srvDesc.ViewDimension			  = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels		  = desc.MipLevels;
	lpDevice->CreateShaderResourceView( lpTexture, &srvDesc, &g_lpSRView1 );

	lpTexture->Release();
	////////////////////////////第二张纹理///////////////////////////////
	D3DX11_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory( &loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO) );
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.Format	   = DXGI_FORMAT_R8G8B8A8_UNORM;

	HRESULT hr = D3DX11CreateShaderResourceViewFromFile( 
		lpDevice, TEXT("..\\GameMedia\\textures\\droplet.png"), &loadInfo, NULL, &g_lpSRView2, NULL );

	//////////////////////////////////////////////////////////////////////////

	//CreateTextureArrayView( lpDevice, lpContext, &g_lpSRView3 );
	//CreateCubeMapView( lpDevice, lpContext, &g_lpSRViewCube );
}

BOOL LoadResources( ID3D11Device* lpDevice, 
					ID3D11DeviceContext* lpContext )
{
	g_pSolidwireShader = new SolidwireShader();
	g_pSolidwireShader->CreateShaders( lpDevice, lpContext );

	g_pNormalShader	= new NormalShader();
	g_pNormalShader->CreateShaders( lpDevice, lpContext );
	
	g_pPlaneZ = new Plane();
	g_pPlaneZ->CreatePlaneZ( lpDevice, lpContext, g_pNormalShader, 1.0f, 1.0f );

	g_pScreenQuad[RT_SCENE] = new ScreenQuad();
	g_pScreenQuad[RT_SCENE]->Create( lpDevice, lpContext, g_dwWindowWidth, g_dwWindowHeight,
		g_dwWindowWidth, g_dwWindowHeight, 0.0f, 0.0f, 1.0f );

	g_pScreenQuad[RT_DEPTH] = new ScreenQuad();
	g_pScreenQuad[RT_DEPTH]->Create( lpDevice, lpContext, g_dwWindowWidth, g_dwWindowHeight,
		g_dwWindowWidth / 4.0f, g_dwWindowHeight / 4.0f,
		-(float)g_dwWindowWidth * 3 / 8.0f, (float)g_dwWindowHeight * 3 / 8.0f - 30, 
		1.0f );
	
	g_pScreenQuad[RT_NORMAL] = new ScreenQuad();
	g_pScreenQuad[RT_NORMAL]->Create( lpDevice, lpContext, g_dwWindowWidth, g_dwWindowHeight,
		g_dwWindowWidth / 4.0f, g_dwWindowHeight / 4.0f,
		-(float)g_dwWindowWidth * 3 / 8.0f, (float)g_dwWindowHeight * 1 / 8.0f - 45, 
		1.0f );

	LoadPSShaderResources( lpDevice, lpContext );

	g_pText = new CBillboardText( lpDevice, lpContext );
	g_pText->SetDisplayPos( XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f) );
	g_pText->SetFontProperty( "楷体", 20, 0xFF00FF00, 0xFFFFFFFF );
	g_pText->UpdateText( _T("当前帧速:0") );

	g_pNormalRenderState = new NormalRenderState( lpDevice, lpContext );
	g_pNormalRenderState->CreateRenderStates();

	return TRUE;
}

BOOL ReleaseResources( VOID )
{
	SAFE_RELEASE( g_lpSRViewCube );
	SAFE_RELEASE( g_lpSRView3 );
	SAFE_RELEASE( g_lpSRView2 );
	SAFE_RELEASE( g_lpSRView1 );

	SAFE_DELETE( g_pText );
	//SAFE_DELETE( g_pKmzLoader );
	SAFE_DELETE( g_pXXModel );
	SAFE_DELETE( g_pNormalRenderState );

	SAFE_DELETE( g_pSolidwireShader );
	SAFE_DELETE( g_pNormalShader );
	for( int i = 0; i < 3; ++i )
		SAFE_DELETE( g_pScreenQuad[i] );
	SAFE_DELETE( g_pPlaneZ );

	return TRUE;
}
