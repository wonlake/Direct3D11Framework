//
// Copyright (C) Mei Jun 2011
//

#include "Plane.h"
#include "Utility.h"
#include <D3DX11.h>
#include <xnamath.h>

Plane::Plane()
{
	m_lpInputLayout		= NULL;
	m_lpVertexBuffer	= NULL;
	m_lpIndexBuffer		= NULL;

	m_uiElementSize		= 0;
	m_uiNumIndices		= 0;
	m_uiNumVertices		= 0;
}

Plane::~Plane()
{
	SAFE_RELEASE( m_lpInputLayout );
	SAFE_RELEASE( m_lpVertexBuffer );
	SAFE_RELEASE( m_lpIndexBuffer );
}

BOOL Plane::CreateInputLayout( ID3D11Device* lpDevice, 
	ID3D11DeviceContext* lpContext, ShaderHelper* pShader )
{
	D3D11_INPUT_ELEMENT_DESC elements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{   "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = _countof( elements );

	HRESULT hr = lpDevice->CreateInputLayout( elements, numElements, 
		pShader->m_lpShaderBytecode,
		pShader->m_dwShaderBytecodeSize,
		&m_lpInputLayout );

	if( FAILED( hr ) )
		return FALSE;

	return TRUE;
}

BOOL Plane::CreatePlaneX( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
	ShaderHelper* pShader, float yLength, float zLength,
	float xOffset, float yOffset, float zOffset )
{
	m_uiElementSize = sizeof( CUSTOMVERTEX );
	m_uiNumIndices	= 6;
	m_uiNumVertices = 4;

	CUSTOMVERTEX vertices[4] =
	{
		{ 0.0f,  yLength / 2.0f,  zLength / 2.0f, 1.0f, 0.0f, 0.0f, 0.0, 0.0 },
		{ 0.0f,  yLength / 2.0f, -zLength / 2.0f, 1.0f, 0.0f, 0.0f, 1.0, 0.0 },
		{ 0.0f, -yLength / 2.0f,  zLength / 2.0f, 1.0f, 0.0f, 0.0f, 0.0, 1.0 },
		{ 0.0f, -yLength / 2.0f, -zLength / 2.0f, 1.0f, 0.0f, 0.0f, 1.0, 1.0 },
	};

	for( int i = 0; i < 4; ++i )
	{
		vertices[i].x += xOffset;
		vertices[i].y += yOffset;
		vertices[i].z += zOffset;
	}

	D3D11_BUFFER_DESC		bd		 = { 0 };
	D3D11_SUBRESOURCE_DATA	initData = { 0 };

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( vertices );
	bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags	   = 0;
	bd.MiscFlags		   = 0;
	bd.StructureByteStride = 0;

	initData.pSysMem	   = vertices;

	HRESULT hr = lpDevice->CreateBuffer( &bd, &initData, &m_lpVertexBuffer );

	if( FAILED( hr ) )
		return FALSE;

	WORD indices[6] =
	{
		0, 1, 2,
		2, 1, 3,
	};

	ZeroMemory( &bd, sizeof(bd) );
	ZeroMemory( &initData, sizeof(initData) );

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( indices );
	bd.BindFlags		   = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags	   = 0;
	bd.MiscFlags		   = 0;
	bd.StructureByteStride = 0;

	initData.pSysMem	   = indices;

	hr = lpDevice->CreateBuffer( &bd, &initData, &m_lpIndexBuffer );

	if( FAILED( hr ) )
		return FALSE;

	if( m_lpInputLayout )
		return TRUE;
	else
		return CreateInputLayout( lpDevice, lpContext, pShader );
}

BOOL Plane::CreatePlaneY( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
	ShaderHelper* pShader, float xLength, float zLength,
	float xOffset, float yOffset, float zOffset )
{
	m_uiElementSize = sizeof( CUSTOMVERTEX );
	m_uiNumIndices	= 6;
	m_uiNumVertices = 4;
	
	CUSTOMVERTEX vertices[4] =
	{
		{ -xLength / 2.0f, 0.0f,  zLength / 2.0f, 0.0f, 1.0f, 0.0f, 0.0, 0.0 },
		{  xLength / 2.0f, 0.0f,  zLength / 2.0f, 0.0f, 1.0f, 0.0f, 1.0, 0.0 },
		{ -xLength / 2.0f, 0.0f, -zLength / 2.0f, 0.0f, 1.0f, 0.0f, 0.0, 1.0 },
		{  xLength / 2.0f, 0.0f, -zLength / 2.0f, 0.0f, 1.0f, 0.0f, 1.0, 1.0 },
	};

	for( int i = 0; i < 4; ++i )
	{
		vertices[i].x += xOffset;
		vertices[i].y += yOffset;
		vertices[i].z += zOffset;
	}

	D3D11_BUFFER_DESC		bd		 = { 0 };
	D3D11_SUBRESOURCE_DATA	initData = { 0 };

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( vertices );
	bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags	   = 0;
	bd.MiscFlags		   = 0;
	bd.StructureByteStride = 0;

	initData.pSysMem	   = vertices;

	HRESULT hr = lpDevice->CreateBuffer( &bd, &initData, &m_lpVertexBuffer );

	if( FAILED( hr ) )
		return FALSE;

	WORD indices[6] =
	{
		0, 1, 2,
		2, 1, 3,
	};

	ZeroMemory( &bd, sizeof(bd) );
	ZeroMemory( &initData, sizeof(initData) );

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( indices );
	bd.BindFlags		   = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags	   = 0;
	bd.MiscFlags		   = 0;
	bd.StructureByteStride = 0;

	initData.pSysMem	   = indices;

	hr = lpDevice->CreateBuffer( &bd, &initData, &m_lpIndexBuffer );

	if( FAILED( hr ) )
		return FALSE;

	if( m_lpInputLayout )
		return TRUE;
	else
		return CreateInputLayout( lpDevice, lpContext, pShader );
}

BOOL Plane::CreatePlaneZ( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
	ShaderHelper* pShader, float xLength, float yLength,
	float xOffset, float yOffset, float zOffset )
{
	m_uiElementSize = sizeof( CUSTOMVERTEX );
	m_uiNumIndices	= 6;
	m_uiNumVertices = 4;

	CUSTOMVERTEX vertices[4] =
	{
		{ -xLength / 2.0f,  yLength / 2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0, 0.0 },
		{  xLength / 2.0f,  yLength / 2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0, 0.0 },
		{ -xLength / 2.0f, -yLength / 2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0, 1.0 },
		{  xLength / 2.0f, -yLength / 2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0, 1.0 },
	};

	for( int i = 0; i < 4; ++i )
	{
		vertices[i].x += xOffset;
		vertices[i].y += yOffset;
		vertices[i].z += zOffset;
	}

	D3D11_BUFFER_DESC		bd		 = { 0 };
	D3D11_SUBRESOURCE_DATA	initData = { 0 };

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( vertices );
	bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags	   = 0;
	bd.MiscFlags		   = 0;
	bd.StructureByteStride = 0;

	initData.pSysMem	   = vertices;

	HRESULT hr = lpDevice->CreateBuffer( &bd, &initData, &m_lpVertexBuffer );

	if( FAILED( hr ) )
		return FALSE;

	WORD indices[6] =
	{
		0, 1, 2,
		2, 1, 3,
	};

	ZeroMemory( &bd, sizeof(bd) );
	ZeroMemory( &initData, sizeof(initData) );

	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof( indices );
	bd.BindFlags		   = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags	   = 0;
	bd.MiscFlags		   = 0;
	bd.StructureByteStride = 0;

	initData.pSysMem	   = indices;

	hr = lpDevice->CreateBuffer( &bd, &initData, &m_lpIndexBuffer );

	if( FAILED( hr ) )
		return FALSE;

	if( m_lpInputLayout )
		return TRUE;
	else
		return CreateInputLayout( lpDevice, lpContext, pShader );
}

VOID Plane::Render( ID3D11DeviceContext* lpContext )
{
	if( m_lpIndexBuffer && m_lpVertexBuffer && m_lpInputLayout )
	{
		UINT offset = 0;
		lpContext->IASetInputLayout( m_lpInputLayout );
		lpContext->IASetVertexBuffers( 0, 1,
			&m_lpVertexBuffer, &m_uiElementSize, &offset );
		lpContext->IASetIndexBuffer( m_lpIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
		lpContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		lpContext->DrawIndexed( m_uiNumIndices, 0, 0 );
	}
}

ScreenQuad::ScreenQuad()
{
	m_pOrthoShader = NULL;
}

ScreenQuad::~ScreenQuad()
{
	SAFE_DELETE( m_pOrthoShader );
}

BOOL ScreenQuad::Create( ID3D11Device* lpDevice, ID3D11DeviceContext* lpContext, 
	int dwWindowWidth, int dwWindowHeight, float xLength, float yLength,
	float xOffset, float yOffset, float zOffset )
{
	if( m_pOrthoShader == NULL )
	{
		m_pOrthoShader = new OrthoShader();
		m_pOrthoShader->CreateShaders( lpDevice, lpContext );
		XMMATRIX matOrtho = XMMatrixOrthographicLH( dwWindowWidth, dwWindowHeight, 1.0f, 100.0f );
		memcpy( m_pOrthoShader->m_VertexConstantBuffer.matWorldViewProj, &matOrtho, sizeof(XMMATRIX) );
	}

	return CreatePlaneZ( lpDevice, lpContext, m_pOrthoShader, xLength, yLength,
		xOffset, yOffset, zOffset );
}

VOID ScreenQuad::RenderQuad( ID3D11DeviceContext* lpContext )
{
	lpContext->VSSetShader( m_pOrthoShader->m_lpVertexShader, NULL, 0 );
	lpContext->PSSetShader( m_pOrthoShader->m_lpPixelShader, NULL, 0 );
	lpContext->GSSetShader( m_pOrthoShader->m_lpGeometryShader, NULL, 0 );

	D3D11_MAPPED_SUBRESOURCE pData;
	lpContext->Map( m_pOrthoShader->m_lpVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
	memcpy_s( pData.pData, pData.RowPitch, &m_pOrthoShader->m_VertexConstantBuffer, 
		sizeof(m_pOrthoShader->m_VertexConstantBuffer) );
	lpContext->Unmap( m_pOrthoShader->m_lpVertexConstantBuffer, 0 );

	lpContext->VSSetConstantBuffers( 0, 1, &m_pOrthoShader->m_lpVertexConstantBuffer );

	Render( lpContext );
}

BOOL ScreenQuad::OrthoShader::CreateShaders( ID3D11Device* lpDevice,
	ID3D11DeviceContext* lpContext )
{
	static CHAR strVS[] =
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4x4 matWorldViewProj;\n"
		"};\n"
		"\n"
		"struct VS_INPUT\n"
		"{\n"
		"	float4 position  : POSITION;\n"
		"	float2 texcoord  : TEXCOORD;\n"
		"};\n"
		"\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"	float4 position	 : SV_POSITION;\n"
		"	float2 texcoord  : TEXCOORD0;\n"
		"};\n"
		"\n"
		"void main( in  VS_INPUT  vert,\n"
		"	   out VS_OUTPUT o )\n"
		"{\n"
		"	o.position   = mul( matWorldViewProj, vert.position );\n"
		"	o.texcoord   = vert.texcoord;\n"
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"Texture2D SceneMap : register(t0);\n"
		"\n"
		"sampler samLinear : register(s0);\n"
		"\n"
		"struct PS_INPUT\n"
		"{\n"
		"	float4 position	 : SV_POSITION;\n"
		"	float2 texcoord  : TEXCOORD;\n"
		"};\n"
		"\n"
		"float4 main( in PS_INPUT vert ) : SV_TARGET\n"
		"{\n"
		"	return SceneMap.Sample( samLinear, vert.texcoord );\n"
		"}\n"
	};
	
	HRESULT hr				= S_OK;
	DWORD dwShaderFlags		= D3D10_SHADER_ENABLE_STRICTNESS;
	ID3D10Blob* pBlobError	= NULL;

	//////////////////////////////创建Vertex Shader/////////////////////////
	{
		ID3D10Blob* pBlobVS = NULL;	

		hr = D3DCompile( strVS, lstrlenA( strVS ) + 1, NULL, NULL, NULL, "main", 
			"vs_4_0", dwShaderFlags, 0, &pBlobVS, &pBlobError );
		//hr = D3DX11CompileFromFile( TEXT("effect/ortho.vsh"), NULL, NULL, 
		//	"main", "vs_4_0", 0, 0, NULL, 
		//	&pBlobVS, &pBlobError, NULL );

		if( pBlobError != NULL )
		{
			MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "VertexShader", NULL );
			pBlobError->Release();
		}
		if( FAILED( hr ) )
			return FALSE;

		SAFE_RELEASE(m_lpVertexShader);
		hr = lpDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), 
			NULL, &m_lpVertexShader );
		if( FAILED( hr ) )
			return FALSE;

		SAFE_DELETEARRAY( m_lpShaderBytecode );
		m_dwShaderBytecodeSize	= pBlobVS->GetBufferSize();
		m_lpShaderBytecode = new BYTE[m_dwShaderBytecodeSize];
		memcpy( m_lpShaderBytecode, pBlobVS->GetBufferPointer(), m_dwShaderBytecodeSize );		

		pBlobVS->Release();
	}	

	//////////////////////////////创建Pixel Shader/////////////////////////
	{
		ID3D10Blob* pBlobPS = NULL;

		hr = D3DCompile( strPS, lstrlenA( strPS ) + 1, NULL, NULL, NULL, 
			"main", "ps_4_0", dwShaderFlags, 0, &pBlobPS, &pBlobError );

		if( pBlobError != NULL )
		{
			MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "PixelShader", NULL );
			pBlobError->Release();
		}
		if( FAILED( hr ) )
			return FALSE;

		SAFE_RELEASE(m_lpPixelShader);
		hr = lpDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), 
			NULL, &m_lpPixelShader );
		if( FAILED( hr ) )
			return FALSE;

		pBlobPS->Release();
	}

	if( m_lpVertexConstantBuffer )
		return TRUE;
	else
		return CreateConstantBuffers( lpDevice, lpContext );
}

BOOL ScreenQuad::OrthoShader::CreateConstantBuffers( ID3D11Device* lpDevice,
	ID3D11DeviceContext* lpContext )
{
	////////////////////创建Vertex Shader常量缓冲区/////////////////////////
	D3D11_BUFFER_DESC cb;
	cb.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
	cb.ByteWidth			= sizeof( ConstantBuffer );
	cb.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
	cb.MiscFlags			= 0;
	cb.StructureByteStride	= 0;
	cb.Usage				= D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem			= &m_VertexConstantBuffer;
	data.SysMemPitch		= sizeof( ConstantBuffer );
	data.SysMemSlicePitch	= 0;

	HRESULT hr = lpDevice->CreateBuffer( &cb, &data, &m_lpVertexConstantBuffer );

	if( FAILED(hr) )
		return FALSE;
	else
		return TRUE;
}