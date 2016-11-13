//
// Copyright (C) Mei Jun 2011
//

#include "NormalShader.h"

BOOL NormalShader::CreateShaders( ID3D11Device* lpDevice,
	ID3D11DeviceContext* lpContext )

{
	static CHAR strVS[] =
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4x4 matWorldViewProj;\n"
		"	float4x4 matWorldView;\n"
		"};\n"
		"\n"
		"static shared float3 FresRatio;\n"
		"static shared float3 lightDir; \n"
		"static shared float3 eyeDir;\n"
		"static shared float3 reflectDir;\n"
		"static shared float3 refractDir;\n"
		"\n"
		"struct VS_INPUT\n"
		"{\n"
		"	float4 position  : POSITION;\n"
		"	float3 normal	 : NORMAL;\n"
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
		"	float3 norm1 = normalize( mul(vert.normal, (float3x3)matWorldView) );\n"
		"	float3 norm  = norm1.xyz;\n"
		"	o.texcoord   = vert.texcoord;\n"
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"Texture2D testMap : register(t0);\n"
		"Texture2D treeMap : register(t1);\n"
		"Texture2DArray arrayMap : register(t2);\n"
		"TextureCube cubeMap : register(t3);\n"
		"\n"
		"SamplerState samLinear : register(s0);\n"
		"\n"
		"struct PS_INPUT\n"
		"{\n"
		"	float4 position	 : SV_POSITION;\n"
		"	float2 texcoord  : TEXCOORD;\n"
		"};\n"
		"\n"
		"float4 main( in PS_INPUT vert ) : SV_TARGET\n"
		"{\n"
		"	float3 coord1 = float3( vert.texcoord, 0 );\n"
		"	float3 coord2 = float3( vert.texcoord, 1 );\n"
		"	float3 coord3 = float3( -0.5, 0.5 - vert.texcoord.y, vert.texcoord.x - 0.5 );\n"
		"\n"
		"	float4 color1 = testMap.Sample( samLinear, vert.texcoord );\n"
		"	float4 color2 = treeMap.Sample( samLinear, vert.texcoord );\n"
		"	float4 color3 = arrayMap.Sample( samLinear, coord1 );\n"
		"	float4 color4 = arrayMap.Sample( samLinear, coord2 );\n"
		"	float4 color5 = cubeMap.Sample( samLinear, coord3 );\n"
		"	float4 color6 = color1 * 0.8 + color2 * 0.3 + color3 * 0.1 + color4 * 0.1;\n"
		"	color5 = color6;\n"
		"	return color5;\n"
		"}\n"
	};

	static CHAR strGS[] = 
	{
		"struct GS_INPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"	float2 texcoord : TEXCOORD0;\n"
		"	float3 normal	: TEXCOORD1;\n"
		"};\n"
		"struct GS_OUTPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"	float2 texcoord : TEXCOORD;\n"
		"	float3 normal	: TEXCOORD1;\n"
		"};\n"
		"[maxvertexcount(3)]\n"
		"void main( triangle GS_INPUT input[3],\n"
		"			inout TriangleStream<GS_OUTPUT> triStream )\n"
		"{\n"
		"	GS_OUTPUT output;\n"
		"	for( uint i = 0; i < 3; i++ )\n"
		"	{\n"
		"		output.position = input[i].position;\n"
		"		output.texcoord = input[i].texcoord;\n"
		"		output.normal	= input[i].normal;\n"
		"		triStream.Append( output );\n"
		"	}\n"
		"	triStream.RestartStrip();\n"
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
		hr = lpDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), 
			NULL, &m_lpPixelShader );
		if( FAILED( hr ) )
			return FALSE;

		pBlobPS->Release();
	}

	//////////////////////////////创建Geometry Shader/////////////////////////
	{
		ID3D10Blob* pBlobGS = NULL;

		hr = D3DCompile( strGS, lstrlenA( strGS ) + 1, NULL, NULL, NULL, "main", 
			"gs_4_0", dwShaderFlags, 0, &pBlobGS, &pBlobError );

		if( pBlobError != NULL )
		{
			MessageBoxA( NULL, (LPCSTR)(pBlobError->GetBufferPointer()), "GeometryShader", NULL );
			pBlobError->Release();
		}
		if( FAILED( hr ) )
			return FALSE;

		SAFE_RELEASE(m_lpGeometryShader);
		hr = lpDevice->CreateGeometryShader( pBlobGS->GetBufferPointer(), pBlobGS->GetBufferSize(), 
			NULL, &m_lpGeometryShader );
		if( FAILED( hr ) )
			return FALSE;

		pBlobGS->Release();
	}

	if( m_lpVertexConstantBuffer )
		return TRUE;
	else
		return CreateConstantBuffers( lpDevice, lpContext );
}

BOOL NormalShader::CreateConstantBuffers( ID3D11Device* lpDevice,
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