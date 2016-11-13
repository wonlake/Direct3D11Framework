//
// Copyright (C) Mei Jun 2011
//

#include "SolidwireShader.h"

BOOL SolidwireShader::CreateShaders( ID3D11Device* lpDevice,
	ID3D11DeviceContext* lpContext )
{
	static CHAR strVS[] =
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4x4 matWorldViewProj;\n"
		"	float4x4 matWorldView;\n"
		"	float4x4 matWorldViewForNormal;\n"
		"};\n"
		"\n"
		"struct VS_INPUT\n"
		"{\n"
		"	float3 position  : POSITION;\n"
		"	float3 normal	 : NORMAL;\n"
		"	float3 texcoord  : TEXCOORD;\n"
		"};\n"
		"\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"	float4 position	 : SV_POSITION;\n"
		"	float4 PosV	 : TEXCOORD0;\n"
		"};\n"
		"\n"
		"void main( in  VS_INPUT  vert,\n"
		"	   out VS_OUTPUT o )\n"
		"{\n"
		"	o.position   = mul( matWorldViewProj, float4(vert.position, 1.0) );\n"
		"	o.PosV	     = mul( matWorldView, float4(vert.position, 1.0) );	\n"
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"Texture2D testMap : register(t0);\n"
		"Texture2D treeMap : register(t1);\n"
		"Texture2DArray arrayMap : register(t2);\n"
		"TextureCube cubeMap : register(t3);\n"
		"\n"
		"static float LineWidth = 5.0;\n"
		"static float4 WireColor = float4(1, 1, 1, 1);\n"
		"\n"
		"SamplerState samLinear : register(s0);\n"
		"\n"
		"struct PS_INPUT\n"
		"{	\n"
		"	float4 position : SV_Position;\n"
		"	float4 Col	: TEXCOORD;\n"
		"	float3 Heights	: TEXCOORD1;\n"
		"};\n"
		"\n"
		"float evalMinDistanceToEdges(in PS_INPUT input)\n"
		"{\n"
		"    float dist;\n"
		"    float3 ddxHeights = ddx( input.Heights );\n"
		"    float3 ddyHeights = ddy( input.Heights );\n"
		"    float3 ddHeights2 = ddxHeights*ddxHeights + ddyHeights*ddyHeights;\n"
		"	\n"
		"    float3 pixHeights2 = input.Heights *  input.Heights / ddHeights2 ;\n"
		"    \n"
		"    dist = sqrt( min ( min (pixHeights2.x, pixHeights2.y), pixHeights2.z) );\n"
		"    \n"
		"    return dist;\n"
		"}\n"
		"\n"
		"float4 main( PS_INPUT input) : SV_Target\n"
		"{\n"
		"    // Compute the shortest distance between the fragment and the edges.\n"
		"    float dist = evalMinDistanceToEdges(input);\n"
		"    \n"
		"    // Cull fragments too far from the edge.\n"
		"    if (dist > 0.5*LineWidth+1) discard;\n"
		"   \n"
		"    // Map the computed distance to the [0,2] range on the border of the line.\n"
		"    dist = clamp((dist - (0.5*LineWidth - 1)), 0, 2);\n"
		"    // Alpha is computed from the function exp2(-2(x)^2).\n"
		"    dist *= dist;\n"
		"    float alpha = exp2(-2*dist);\n"
		"\n"
		"    // Standard wire color\n"
		"    float4 color = WireColor;\n"
		"    color.a *= alpha;\n"
		"\n"
		"    return color;\n"
		"}\n"
	};

	static CHAR strGS[] = 
	{
		"struct GS_INPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"	float4 PosV 	: TEXCOORD;\n"
		"};\n"
		"\n"
		"struct GS_OUTPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"	float4 Col	: TEXCOORD;\n"
		"	float3 Heights	: TEXCOORD1;\n"
		"};\n"
		"\n"
		"static float4 FillColor = float4(0.1, 0.2, 0.4, 1);\n"
		"static float4 LightVector = float4( 0, 0, 1, 0);\n"
		"\n"
		"float3 faceNormal(in float3 posA, in float3 posB, in float3 posC)\n"
		"{\n"
		"    return normalize( cross(normalize(posB - posA), normalize(posC - posA)) );\n"
		"}\n"
		"\n"
		"float4 shadeFace(in float4 verA, in float4 verB, in float4 verC)\n"
		"{\n"
		"    // Compute the triangle face normal in view frame\n"
		"    float3 normal = faceNormal((float3)verA, (float3)verB, (float3)verC);\n"
		"    \n"
		"    // Then the color of the face.\n"
		"    float shade = 0.5*abs( dot(normal, (float3)LightVector) );\n"
		"    \n"
		"    return float4(FillColor.xyz*shade, 1);\n"
		"}\n"
		"\n"
		"[maxvertexcount(3)]\n"
		"void main( triangle GS_INPUT input[3],\n"
		"                         inout TriangleStream<GS_OUTPUT> outStream )\n"
		"{\n"
		"    GS_OUTPUT output;\n"
		"\n"
		"    // Shade and colour face.\n"
		"    output.Col = shadeFace( input[0].PosV, input[1].PosV, input[2].PosV );\n"
		"\n"
		"    // Emit the 3 vertices\n"
		"    // The Height attribute is based on the constant\n"
		"    output.position = input[0].position;\n"
		"    output.Heights = float3( 1, 0, 0 );\n"
		"    outStream.Append( output );\n"
		"\n"
		"    output.position = input[1].position;\n"
		"    output.Heights = float3( 0, 1, 0 );\n"
		"    outStream.Append( output );\n"
		"\n"
		"    output.position = input[2].position;\n"
		"    output.Heights = float3( 0, 0, 1 );\n"
		"    outStream.Append( output );\n"
		"\n"
		"    outStream.RestartStrip();\n"
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
		m_lpShaderBytecode		= new BYTE[m_dwShaderBytecodeSize];
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

	return TRUE;
}