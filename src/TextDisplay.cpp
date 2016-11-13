//
// Copyright (C) Mei Jun 2011
//

#include "TextDisplay.h"
#include "Utility.h"

#include <d3dx11.h>
#include <d3dcompiler.h>

#include <vector>

TextDisplay::ConstantBuffer TextDisplay::s_Buffer;

TextDisplay::TextDisplay( ID3D11Device* lpDevice,
	ID3D11DeviceContext* lpDeviceContext )
{
	m_lpDevice = lpDevice;
	m_lpDeviceContext = lpDeviceContext;
	m_pFontManager = new FontManager();
	
	m_lpResourceView = NULL;
	m_lpInputLayout = NULL;
	m_lpVertexShader = NULL;
	m_lpPixelShader = NULL;
	m_lpGeometryShader = NULL;
	m_dwShaderBytecodeSize = 0;
	m_lpShaderBytecodeInput = NULL;
	m_lpVSConstBufferWorldViewProj = NULL;
	m_lpVertexBuffer = NULL;
	m_lpIndexBuffer = NULL;
	m_lpBlendState = NULL;

	m_bInit = FALSE;
}

TextDisplay::~TextDisplay(void)
{
	SAFE_DELETE( m_pFontManager );

	SAFE_RELEASE( m_lpResourceView );
	SAFE_RELEASE( m_lpVertexBuffer );
	SAFE_RELEASE( m_lpIndexBuffer );
	SAFE_RELEASE( m_lpInputLayout );
	SAFE_RELEASE( m_lpVertexShader );
	SAFE_RELEASE( m_lpPixelShader );
	SAFE_RELEASE( m_lpGeometryShader );
	SAFE_RELEASE( m_lpVSConstBufferWorldViewProj );

	SAFE_RELEASE( m_lpBlendState );

	SAFE_DELETEARRAY( m_lpShaderBytecodeInput );
}

VOID TextDisplay::SetupInput( int iXPos, int iYPos, float Left, float Top,
	float Right, float Bottom )
{	
	//用于文本显示的顶点模板
	UINT numViewPorts = 1;
	D3D11_VIEWPORT viewport;
	m_lpDeviceContext->RSGetViewports( &numViewPorts, &viewport );

	static CUSTOMVERTEX Vertices[] = 
	{
		{ -viewport.Width / 2.0f, viewport.Height / 2.0f, 1.0f, 0.0f, 0.0f },
		{ -viewport.Width / 2.0f, viewport.Height / 2.0f, 1.0f, 1.0f, 0.0f },
		{ -viewport.Width / 2.0f, viewport.Height / 2.0f, 1.0f, 0.0f, 1.0f },
		{ -viewport.Width / 2.0f, viewport.Height / 2.0f, 1.0f, 1.0f, 1.0f },
	};

	//用于文本显示的索引模板
	static WORD Indices[] = 
	{
		0, 1, 2,
		2, 1, 3,
	};

	//文本显示网格
	std::vector<CUSTOMVERTEX>	vecVertices;
	vecVertices.resize( 4 );
	if( m_lpResourceView != NULL )
	{
		for( int i = 0; i < 4; ++i )
		{
			Vertices[i].x = -viewport.Width / 2.0f;
			Vertices[i].y =	viewport.Height / 2.0f;

			vecVertices[i] = Vertices[i];
			if( i % 2 )
			{
				vecVertices[i].x = Vertices[i].x + iXPos + Right - Left;
				if( i / 2 )
				{
					vecVertices[i].y = Vertices[i].y - iYPos + Top - Bottom;
				}
				else
				{
					vecVertices[i].y = Vertices[i].y - iYPos;
				}
			}
			else
			{
				vecVertices[i].x = Vertices[i].x + iXPos;
				if( i / 2 )
				{
					vecVertices[i].y = Vertices[i].y - iYPos + Top - Bottom;
				}
				else
				{
					vecVertices[i].y = Vertices[i].y - iYPos;
				}
			}

			vecVertices[i].z = 1.0f;
		}
	}

	D3D11_BUFFER_DESC bd = { 0 };

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( Vertices );
	bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;
	
	SAFE_RELEASE( m_lpVertexBuffer );
	HRESULT hr = m_lpDevice->CreateBuffer( &bd, NULL, &m_lpVertexBuffer );
	m_lpDeviceContext->UpdateSubresource( m_lpVertexBuffer, 0, NULL, &vecVertices[0], bd.ByteWidth, 0 );
		
	if( m_bInit )
		return;

	bd.ByteWidth			= sizeof( Indices );
	bd.BindFlags			= D3D11_BIND_INDEX_BUFFER;

	SAFE_RELEASE( m_lpIndexBuffer );
	hr = m_lpDevice->CreateBuffer( &bd, NULL, &m_lpIndexBuffer );
	m_lpDeviceContext->UpdateSubresource( m_lpIndexBuffer, 0, NULL, Indices, bd.ByteWidth, 0 );
	
	D3D11_INPUT_ELEMENT_DESC elements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = _countof( elements );

	hr = m_lpDevice->CreateInputLayout( elements, numElements, 
		m_lpShaderBytecodeInput,
		m_dwShaderBytecodeSize,
		&m_lpInputLayout );
	if( FAILED( hr ) )
		return;

	SAFE_DELETEARRAY( m_lpShaderBytecodeInput );	
}

BOOL TextDisplay::SetupShaders()
{
	static CHAR strVS[] =
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4x4 matWorldViewProj;\n"
		"};\n"
		"struct VS_INPUT\n"
		"{\n"
		"float4 position : POSITION;\n"
		"float2 texcoord : TEXCOORD;\n"
		"};\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"	float4 position : SV_POSITION;\n"
		"	float2 texcoord : TEXCOORD;\n"
		"};\n"
		"void main( in  VS_INPUT  vert,\n"
		"           out VS_OUTPUT o )\n"
		"{\n"   
		"    o.position = mul( matWorldViewProj, vert.position );\n"
		"    o.texcoord = vert.texcoord;\n"  
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"Texture2D testMap : register(t0);\n"
		"SamplerState samLinear : register(s0);\n"
		"struct PS_INPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"	float2 texcoord : TEXCOORD;\n"
		"};\n"
		"float4 main( in PS_INPUT vert ) : SV_TARGET\n"
		"{\n"
		"	float4 color = testMap.Sample(samLinear, vert.texcoord);\n"
		"	if( color.a > 0.1 )\n"
		"	{\n"
		"		//color.a = 1.0;\n"
		"		color.r *= 100.0f;\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		color.r = color.g = color.b = 1.0;\n"
		"	}\n"
		"	return color;"
		"}\n"
	};

	HRESULT hr = S_OK;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
	ID3D10Blob* pBlobError = NULL;

	//////////////////////////////创建Vertex Shader/////////////////////////
	{
		ID3D10Blob* pBlobVS = NULL;		

		hr = D3DCompile( strVS, lstrlenA( strVS ) + 1, NULL, NULL, NULL, "main", 
			"vs_4_0", dwShaderFlags, 0, &pBlobVS, &pBlobError );

		if( FAILED( hr ) )
		{
			if( pBlobError != NULL )
			{ 
				MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "VertexShader", NULL );
				pBlobError->Release();
			}
			return FALSE;
		}
		hr = m_lpDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), 
			NULL, &m_lpVertexShader );
		if( FAILED( hr ) )
			return FALSE;

		SAFE_DELETEARRAY( m_lpShaderBytecodeInput );

		m_dwShaderBytecodeSize	= pBlobVS->GetBufferSize();
		m_lpShaderBytecodeInput = new BYTE[m_dwShaderBytecodeSize];
		memcpy( m_lpShaderBytecodeInput, pBlobVS->GetBufferPointer(), m_dwShaderBytecodeSize );		

		pBlobVS->Release();
	}

	////////////////////创建Vertex Shader常量缓冲区/////////////////////////
	{
		D3D11_BUFFER_DESC cb;
		cb.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
		cb.ByteWidth			= sizeof( ConstantBuffer );
		cb.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
		cb.MiscFlags			= 0;
		cb.StructureByteStride	= 0;
		cb.Usage				= D3D11_USAGE_DYNAMIC;

		hr = m_lpDevice->CreateBuffer( &cb, NULL, &m_lpVSConstBufferWorldViewProj );
		if( FAILED(hr) )
			return FALSE;
	}

	//////////////////////////////创建Pixel Shader/////////////////////////
	{
		ID3D10Blob* pBlobPS = NULL;
		hr = D3DCompile( strPS, lstrlenA( strPS ) + 1, NULL, NULL, NULL, "main", 
			"ps_4_0", dwShaderFlags, 0, &pBlobPS, &pBlobError );
		if( FAILED( hr ) )
		{
			if( pBlobError != NULL )
			{
				MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "PixelShader", NULL );
				pBlobError->Release();
			}
			return hr;
		}
		hr = m_lpDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), 
			NULL, &m_lpPixelShader );
		if( FAILED( hr ) )
			return FALSE;

		pBlobPS->Release();
	}

	return TRUE;
}

void TextDisplay::Display()
{
	UINT numViewPorts = 1;
	D3D11_VIEWPORT viewport;

	m_lpDeviceContext->RSGetViewports( &numViewPorts, &viewport );
	m_lpDeviceContext->IASetIndexBuffer(
		m_lpIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	UINT stride = sizeof(CUSTOMVERTEX);
	UINT offset = 0;

	m_lpDeviceContext->IASetInputLayout( m_lpInputLayout );
	m_lpDeviceContext->IASetVertexBuffers( 0, 1, &m_lpVertexBuffer, &stride, &offset );			
	m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

	if( m_lpResourceView )
		m_lpDeviceContext->PSSetShaderResources( 0, 1, &m_lpResourceView );
	else
		return;

	m_lpDeviceContext->VSSetShader( m_lpVertexShader, NULL, 0 );
	m_lpDeviceContext->PSSetShader( m_lpPixelShader, NULL, 0 );
	m_lpDeviceContext->GSSetShader( m_lpGeometryShader, NULL, 0 );
		
	m_lpDeviceContext->RSGetViewports( &numViewPorts, &viewport );

	XMMATRIX matProj = XMMatrixOrthographicLH( 
		viewport.Width, viewport.Height, 1.0f, 1000.0f );

	memcpy( s_Buffer.matWorldViewProj, &matProj, sizeof(XMMATRIX) );
	D3D11_MAPPED_SUBRESOURCE pData;
	m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
	memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
	m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );	

	ID3D11BlendState* lpOrigBlendState = NULL;
	FLOAT origColor[4]; 
	UINT uiOrigSampleMask = 0xFFFFFFFF;
	if( m_lpBlendState )
	{
		FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_lpDeviceContext->OMGetBlendState( &lpOrigBlendState, origColor, &uiOrigSampleMask );
		m_lpDeviceContext->OMSetBlendState( m_lpBlendState, color, 0xFFFFFFFF );
		
	}
	m_lpDeviceContext->DrawIndexed( 6, 0, 0 );	
	if( lpOrigBlendState )
		m_lpDeviceContext->OMSetBlendState( lpOrigBlendState, origColor, uiOrigSampleMask );
}

void TextDisplay::SetText( int iXPos, int iYPos, int iFontSize, 
	LPCWSTR lpText, unsigned int uiColor )
{
	int dwSize = iFontSize;	
	size_t uiTextLength = wcslen( lpText );

	DWORD dwImageWidth = uiTextLength * dwSize;
	float rows = dwImageWidth / 1024.0f;
	DWORD dwImageHeight = ceil( rows ) * dwSize * 2;
	if( rows > 1.0f )
		dwImageWidth = 1024;
	if( dwImageHeight / 512.0f > 1.0f )
		dwImageHeight = 512;

	int iBytesPerPixel = 4;
	size_t uiImageDataSize = dwImageHeight * dwImageWidth * iBytesPerPixel;
	BYTE* pImageData = new BYTE[uiImageDataSize];
	ZeroMemory( pImageData, uiImageDataSize );
	BYTE* pData = NULL;
	size_t XOffset = 0;
	size_t YOffset = dwSize;
	size_t dwMaxHeight = 0;
	
	BYTE *pBlue = (BYTE*)&uiColor;
	BYTE* pGreen = pBlue + 1;
	BYTE* pRed = pBlue + 2;

	for( int i = 0; i < uiTextLength; ++i )
	{		
		FontManager::BMP_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		pData = m_pFontManager->CreateFontTexture(
			lpText[i], dwSize, &desc );
		size_t width = desc.dwWidth;
		size_t height = desc.dwHeight;

		int dwPitch = width * iBytesPerPixel;
		while( dwPitch % 4 != 0 )
			dwPitch++;
		if( XOffset + desc.iXAdvance > 1024 )
		{
			XOffset = 0;			
			YOffset += dwSize;
			dwMaxHeight = 0;
		}
		if( height > dwMaxHeight )
		{
			dwMaxHeight = height;
		}
		for( int h = 0; h < height; ++h )
		{
			for( int w = 0; w < width; ++w )
			{
				int X = (XOffset + desc.iXOffset + w) * iBytesPerPixel;
				int Y = (YOffset - desc.iYOffset + h) * dwImageWidth * iBytesPerPixel;
				if( Y + X >= uiImageDataSize - iBytesPerPixel )
					continue;
				{
					pImageData[Y + X]	  = *pBlue;
					pImageData[Y + X + 1] = *pGreen;
					pImageData[Y + X + 2] = *pRed;
				}
				if( iBytesPerPixel > 3 )
					pImageData[Y + X + 3] = pData[dwPitch * (height - h - 1) + w * iBytesPerPixel + 3];
			}
		}
		XOffset += desc.iXAdvance;			
	}	

	if( TRUE )
	{
		//创建纹理
		ID3D11Texture2D* lpTexture = NULL;
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.ArraySize	= 1;
		desc.BindFlags	= D3D11_BIND_SHADER_RESOURCE;
		desc.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Width		= dwImageWidth;
		desc.Height		= dwImageHeight;
		desc.Usage		= D3D11_USAGE_DEFAULT;
		desc.MipLevels	= 1;
		desc.SampleDesc.Count = 1;

		HRESULT hr = m_lpDevice->CreateTexture2D(&desc, NULL, &lpTexture); 

		//根据位图数据填充纹理数据

		BYTE *pSrcData = (BYTE*)pImageData;

		{
			DWORD BufferIndex = 0;

			for(DWORD Y = 0; Y < dwImageHeight; Y++)
			{
				for(DWORD X = 0; X < dwImageWidth; X++)
				{
					pRed = (pSrcData + (Y * dwImageWidth + X) * 4);
					pBlue = pRed + 2;
					BYTE temp = *pBlue;
					*pBlue = *pRed;
					*pRed = temp;
				}
			}			
		}

		m_lpDeviceContext->UpdateSubresource( lpTexture, 0, NULL, pSrcData, dwImageWidth * 4, 0 );

		D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
		ZeroMemory( &view_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );

		lpTexture->GetDesc( &desc );

		view_desc.Format					= desc.Format;
		view_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		view_desc.Texture2D.MostDetailedMip = 0;
		view_desc.Texture2D.MipLevels		= desc.MipLevels;

		SAFE_RELEASE( m_lpResourceView );
		m_lpDevice->CreateShaderResourceView( lpTexture, &view_desc, &m_lpResourceView );
		SAFE_RELEASE( lpTexture );
	}
	if( FALSE )
	{			
		size_t uiBitmapSize = RawToBitmap( pImageData, uiImageDataSize, NULL, 0,
			dwImageWidth, dwImageHeight, iBytesPerPixel );
		BYTE* pBitmapBuffer = new BYTE[uiBitmapSize];
		RawToBitmap( pImageData, uiImageDataSize, pBitmapBuffer, uiBitmapSize,
			dwImageWidth, dwImageHeight, iBytesPerPixel );

		FILE* pFile = fopen( "E:/test.bmp", "wb" );
		fwrite( pBitmapBuffer, 1, uiBitmapSize, pFile );
		fclose( pFile );

		D3DX11_IMAGE_LOAD_INFO loadInfo;
		ZeroMemory( &loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO) );
		loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		
		SAFE_RELEASE( m_lpResourceView );
		HRESULT hr = D3DX11CreateShaderResourceViewFromMemory( m_lpDevice,
			pBitmapBuffer, uiBitmapSize, &loadInfo, NULL, &m_lpResourceView, NULL );
		
		SAFE_DELETEARRAY( pBitmapBuffer );
	}
	SAFE_DELETEARRAY( pImageData );

	if( !m_bInit )
	{
		SetupShaders();
		CreateBlendState();				
	}

	SetupInput( iXPos, iYPos, 0, 0,
		dwImageWidth, dwImageHeight );	
	m_bInit = TRUE;	
}

void TextDisplay::CreateBlendState()
{
	D3D11_BLEND_DESC blend_desc;
	ZeroMemory( &blend_desc, sizeof(D3D11_BLEND_DESC) );
	blend_desc.RenderTarget[0].BlendEnable			 = TRUE;
	blend_desc.RenderTarget[0].SrcBlend				 = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend			 = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp				 = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha		 = D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha		 = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha			 = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		
	m_lpDevice->CreateBlendState( &blend_desc, &m_lpBlendState );	
}

size_t TextDisplay::RawToBitmap( void* pImageData, size_t uiImageDataSize, 
	void* pBitmap, size_t uiBitmapSize,
	size_t uiWidth, size_t uiHeight, int iBytesPerPixel )
{
	if( (pImageData == NULL) || (iBytesPerPixel < 3) )
		return 0;

	int dwRowPitch = uiWidth * iBytesPerPixel;
	while( dwRowPitch % 4 != 0 )
		dwRowPitch++;
	if( uiBitmapSize == 0 )
	{
		uiBitmapSize = sizeof(BITMAPFILEHEADER) +
			sizeof(BITMAPINFOHEADER) + dwRowPitch * uiHeight;
		return uiBitmapSize;
	}

	if( pBitmap == NULL )
		return 0;

	BYTE* pBitmapBuffer = (BYTE*)pBitmap;

	BITMAPINFOHEADER bi;
	bi.biSize                    =  sizeof(BITMAPINFOHEADER);  
	bi.biWidth                   =  uiWidth;  
	bi.biHeight                  =  uiHeight;  
	bi.biPlanes                  =  1;  
	bi.biBitCount                =  iBytesPerPixel * 8;  
	bi.biCompression             =  BI_RGB;  
	bi.biSizeImage               =  0;  
	bi.biXPelsPerMeter           =  0;  
	bi.biYPelsPerMeter           =  0;  
	bi.biClrImportant            =  0;  
	bi.biClrUsed                 =  0;

	BITMAPFILEHEADER bmfHdr;
	bmfHdr.bfType		=  0x4D42;  //  "BM"    
	bmfHdr.bfSize		=  uiBitmapSize;    
	bmfHdr.bfReserved1  =  0;    
	bmfHdr.bfReserved2  =  0;    
	bmfHdr.bfOffBits	=  (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	memcpy( pBitmapBuffer, &bmfHdr, sizeof(BITMAPFILEHEADER));
	memcpy( pBitmapBuffer + sizeof(BITMAPFILEHEADER),
		&bi, sizeof(BITMAPINFOHEADER));
	
	BYTE* pData = pBitmapBuffer + sizeof(BITMAPFILEHEADER) +
		sizeof(BITMAPINFOHEADER);
	BYTE* pSrcData = (BYTE*)pImageData;

	size_t uiRowWidth = uiWidth * iBytesPerPixel;
	for( int Y = 0; Y < uiHeight; ++Y )
	{
		for( int X = 0; X < uiRowWidth; ++X )
		{
			*pData++ = *pSrcData++;

		}
		pData = pBitmapBuffer + bmfHdr.bfOffBits + dwRowPitch * (Y + 1);
	}

	return uiBitmapSize;
}

void TextDisplay::SetFontFile( const char* pFontFile )
{
	if( m_pFontManager )
		m_pFontManager->SetFontFile( pFontFile );
}