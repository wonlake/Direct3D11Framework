//
// Copyright (C) Mei Jun 2011
//

#include "BoundingBox.h"

DWORD BoundingBox::m_Reference = 0;
BoundingBox::ConstantBuffer BoundingBox::s_Buffer;
ID3D11InputLayout* BoundingBox::m_lpInputLayout = NULL;
ID3D11VertexShader* BoundingBox::m_lpVertexShader = NULL;
ID3D11PixelShader* BoundingBox::m_lpPixelShader = NULL;
ID3D11GeometryShader* BoundingBox::m_lpGeometryShader = NULL;
ID3D11Buffer* BoundingBox::m_lpVSConstBufferWorldViewProj = NULL;
DWORD BoundingBox::m_dwShaderBytecodeSize = 0;
VOID* BoundingBox::m_lpShaderBytecodeInput = NULL;
ID3D11RasterizerState* BoundingBox::m_lpRasterizerState = NULL;

BoundingBox::BoundingBox( ID3D11Device* lpDevice, 
	ID3D11DeviceContext* lpDeviceContext )
{
	m_lpDevice = lpDevice;
	m_lpDeviceContext = lpDeviceContext;

	m_lpVertexBuffer		= NULL;
	m_lpIndexBuffer			= NULL;

	m_IndexFormat			= DXGI_FORMAT_R16_UINT;
	m_NumIndices			= 0;

	++m_Reference;
	if( m_Reference == 1 )
	{
		SetupShaders();
		SetupInput();
		CreateRenderState();
	}
}


BoundingBox::~BoundingBox(void)
{
	--m_Reference;
	if( m_Reference == 0 )
	{
		SAFE_RELEASE( m_lpRasterizerState );
		SAFE_RELEASE( m_lpInputLayout );
		SAFE_RELEASE( m_lpVertexShader );
		SAFE_RELEASE( m_lpPixelShader );
		SAFE_RELEASE( m_lpGeometryShader );
		SAFE_RELEASE( m_lpVSConstBufferWorldViewProj );
		SAFE_DELETEARRAY( m_lpShaderBytecodeInput );
	}
	SAFE_RELEASE( m_lpVertexBuffer );
	SAFE_RELEASE( m_lpIndexBuffer );
}

void BoundingBox::SetupInput()
{
	D3D11_INPUT_ELEMENT_DESC elements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = _countof( elements );

	//if( false )
	{
		HRESULT hr = m_lpDevice->CreateInputLayout( elements, numElements, 
			m_lpShaderBytecodeInput,
			m_dwShaderBytecodeSize,
			&m_lpInputLayout );
		if( FAILED( hr ) )
			return;
	}

	SAFE_DELETEARRAY( m_lpShaderBytecodeInput );	
}

BOOL BoundingBox::SetupShaders()
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
		"};\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"	float4 position : SV_POSITION;\n"
		"};\n"
		"void main( in  VS_INPUT  vert,\n"
		"           out VS_OUTPUT o )\n"
		"{\n"   
		"    o.position = mul( matWorldViewProj, vert.position );\n"  
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"struct PS_INPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"};\n"
		"float4 main( in PS_INPUT vert ) : SV_TARGET\n"
		"{\n"
		"	return float4(0.7968, 0.7969, 0.0, 0.3);\n"
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

		if( pBlobError != NULL )
		{
			MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "VertexShader", NULL );
			pBlobError->Release();
		}
		if( FAILED( hr ) )
			return FALSE;

		SAFE_RELEASE(m_lpVertexShader);
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

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem			= &s_Buffer;
		data.SysMemPitch		= 0;
		data.SysMemSlicePitch	= 0;

		hr = m_lpDevice->CreateBuffer( &cb, NULL, &m_lpVSConstBufferWorldViewProj );
		if( FAILED(hr) )
			return FALSE;
	}

	//////////////////////////////创建Pixel Shader/////////////////////////
	{
		ID3D10Blob* pBlobPS = NULL;
		hr = D3DCompile( strPS, lstrlenA( strPS ) + 1, NULL, NULL, NULL, "main", 
			"ps_4_0", dwShaderFlags, 0, &pBlobPS, &pBlobError );

		if( pBlobError != NULL )
		{
			MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "PixelShader", NULL );
			pBlobError->Release();
		}
		if( FAILED( hr ) )
			return FALSE;

		SAFE_RELEASE(m_lpPixelShader);
		hr = m_lpDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), 
			NULL, &m_lpPixelShader );
		if( FAILED( hr ) )
			return FALSE;

		pBlobPS->Release();
	}

	return TRUE;
}

VOID BoundingBox::SetBuffer( UINT NumIndices, VOID* pIndexBuffer, UINT IndexDataStride,
	UINT NumVertices, VOID* pVertexBuffer, UINT VertexDataStride )
{
	VERTEX_DATA* pVertices = new VERTEX_DATA[NumVertices];
	FLOAT* pData = (FLOAT*)pVertexBuffer;
	for( size_t i = 0; i < NumVertices; ++i )
	{		
		pVertices[i].x = *pData;
		pVertices[i].y = *(pData + 1);
		pVertices[i].z = *(pData + 2);
		pData += VertexDataStride / sizeof(FLOAT);
	}


	D3D11_BUFFER_DESC bd = { 0 };
	
	bd.Usage			   = D3D11_USAGE_DEFAULT;
	bd.ByteWidth		   = sizeof(VERTEX_DATA) * NumVertices; 
	bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;

	SAFE_RELEASE( m_lpVertexBuffer );
	HRESULT hr = m_lpDevice->CreateBuffer( &bd, NULL, &m_lpVertexBuffer );
	m_lpDeviceContext->UpdateSubresource( m_lpVertexBuffer, 0, NULL,
		pVertices, bd.ByteWidth, 0 ); 
	SAFE_DELETEARRAY( pVertices );

	bd.ByteWidth		   = NumIndices * IndexDataStride;
	bd.BindFlags		   = D3D11_BIND_INDEX_BUFFER;

	if( IndexDataStride == 4 )
		m_IndexFormat = DXGI_FORMAT_R32_UINT;

	SAFE_RELEASE( m_lpIndexBuffer );
	hr = m_lpDevice->CreateBuffer( &bd, NULL, &m_lpIndexBuffer );
	m_lpDeviceContext->UpdateSubresource( m_lpIndexBuffer, 0, NULL,
		pIndexBuffer, bd.ByteWidth, 0 );

	m_NumIndices = NumIndices;
}

VOID BoundingBox::Render( XMMATRIX* pWorldViewProjMatrix )
{
	if( m_lpVertexBuffer && m_lpIndexBuffer )
	{
		m_lpDeviceContext->IASetIndexBuffer(
			m_lpIndexBuffer, m_IndexFormat, 0 );

		UINT stride = sizeof(VERTEX_DATA);
		UINT offset = 0;

		m_lpDeviceContext->IASetInputLayout( m_lpInputLayout );
		m_lpDeviceContext->IASetVertexBuffers( 0, 1, &m_lpVertexBuffer, &stride, &offset );			
		m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

		ID3D11ShaderResourceView* pView = NULL;
		m_lpDeviceContext->PSSetShaderResources( 0, 1, &pView );

		m_lpDeviceContext->VSSetShader( m_lpVertexShader, NULL, 0 );
		m_lpDeviceContext->PSSetShader( m_lpPixelShader, NULL, 0 );
		m_lpDeviceContext->GSSetShader( m_lpGeometryShader, NULL, 0 );

		D3D11_MAPPED_SUBRESOURCE pData;		
		memcpy( s_Buffer.matWorldViewProj, pWorldViewProjMatrix, sizeof(XMMATRIX) );
		m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
		memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
		m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );	

		ID3D11RasterizerState* lpOrig = NULL;
		m_lpDeviceContext->RSGetState( &lpOrig );
		if( m_lpRasterizerState )
			m_lpDeviceContext->RSSetState( m_lpRasterizerState );
		m_lpDeviceContext->DrawIndexed( m_NumIndices, 0, 0 );	
		if( lpOrig )
			m_lpDeviceContext->RSSetState( lpOrig );
	}
}

BOOL BoundingBox::CreateRenderState()
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