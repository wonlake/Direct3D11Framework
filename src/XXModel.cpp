//
// Copyright (C) Mei Jun 2011
//

#include "XXModel.h"
#include "XXFile.h"
#include "XAFile.h"

#include "TextDisplay.h"

#define FREEIMAGE_LIB
#include <freeimage.h>

#ifdef _DEBUG
#pragma comment( lib, "freeimage_d.lib" )
#else
#pragma comment( lib, "freeimage.lib" )
#endif

CXXModel::ConstantBuffer CXXModel::s_Buffer;
CXXModel::PSConstantBuffer CXXModel::s_PSBuffer;

ID3D11InputLayout* CXXModel::m_lpInputLayout = NULL;
ID3D11VertexShader* CXXModel::m_lpVertexShader = NULL;
ID3D11PixelShader* CXXModel::m_lpPixelShader = NULL;
ID3D11GeometryShader* CXXModel::m_lpGeometryShader = NULL;
ID3D11Buffer* CXXModel::m_lpVSConstBufferWorldViewProj = NULL;
DWORD CXXModel::m_dwShaderBytecodeSize = 0;
VOID* CXXModel::m_lpShaderBytecodeInput = NULL;
DWORD CXXModel::m_Reference = 0;

ID3D11InputLayout* CXXModel::m_lpInputLayoutForNoTexture = NULL;
ID3D11VertexShader*	CXXModel::m_lpVertexShaderForNoTexture = NULL;
ID3D11PixelShader* CXXModel::m_lpPixelShaderForNoTexture = NULL;
ID3D11GeometryShader* CXXModel::m_lpGeometryShaderForNoTexture = NULL;
DWORD CXXModel::m_dwShaderBytecodeSizeForNoTexture = 0;
VOID* CXXModel::m_lpShaderBytecodeInputForNoTexture = NULL;
ID3D11Buffer* CXXModel::m_lpPSConstBufferDiffuse = NULL;

ID3D11RasterizerState* CXXModel::m_lpRasterizerState = NULL;
ID3D11RasterizerState* CXXModel::m_lpRasterizerStateForNoCull = NULL;
ID3D11BlendState* CXXModel::m_lpBlendState = NULL;

CXXModel::CXXModel( ID3D11Device* lpDevice, ID3D11DeviceContext* lpDeviceContext )
{
	m_pXXFile = new CXXFile();
	m_pXAFile = new CXAFile();

	m_lpDevice		  = lpDevice;
	m_lpDeviceContext = lpDeviceContext;

	m_fScale		  = 1.0f;

	m_lpTexture		  = NULL;
	m_lpResourceView  = NULL;

	m_lpResourceViewExchange = NULL;
	m_lpVertexBuffer		 = NULL;
	m_lpIndexBuffer			 = NULL;

	m_iTotalFrames		   = 0;

	m_AnimTime.fTime	   = 0.0f;
	m_AnimTime.uiStartTime = 0;
	m_AnimTime.uiEndTime   = 0;	

	++m_Reference;
	if( m_Reference == 1 )
	{
		FreeImage_Initialise();

		SetupShaders();
		SetupShadersForNoTexture();
		SetupInput();
		SetupInputForNoTexture();
		CreateRenderState();
	}

	for( int i = 0; i < 3; ++i )
	{
		m_Max[i]	 = -FLT_MAX;
		m_Min[i]	 =  FLT_MAX;
		m_fCenter[i] = 0.0f;
	}	

	m_fTranslateUnit   = 1.0f;

	m_pBoundingBox	   = NULL;
	m_bShowBoundingBox = FALSE;

	static XMMATRIX matIdentity = XMMatrixIdentity();
	memcpy( m_matTransform, &matIdentity, sizeof(XMMATRIX) );
	memcpy( m_matTranslate, &matIdentity, sizeof(XMMATRIX) );
}


CXXModel::~CXXModel(void)
{
	DestroyPrivateResource();
	DestroyAnimationData();

	SAFE_DELETE( m_pXXFile );
	SAFE_DELETE( m_pXAFile );

	--m_Reference;
	if( m_Reference == 0 )
	{
		FreeImage_DeInitialise();

		SAFE_RELEASE( m_lpInputLayout );
		SAFE_RELEASE( m_lpVertexShader );
		SAFE_RELEASE( m_lpPixelShader );
		SAFE_RELEASE( m_lpGeometryShader );
		SAFE_RELEASE( m_lpVSConstBufferWorldViewProj );
		SAFE_DELETEARRAY( m_lpShaderBytecodeInput );

		SAFE_RELEASE( m_lpInputLayoutForNoTexture );
		SAFE_RELEASE( m_lpVertexShaderForNoTexture );
		SAFE_RELEASE( m_lpPixelShaderForNoTexture );
		SAFE_RELEASE( m_lpGeometryShaderForNoTexture );
		SAFE_RELEASE( m_lpPSConstBufferDiffuse );
		SAFE_DELETEARRAY( m_lpShaderBytecodeInputForNoTexture );

		SAFE_RELEASE( m_lpRasterizerState );
		SAFE_RELEASE( m_lpRasterizerStateForNoCull );

		SAFE_RELEASE( m_lpBlendState );
	}
}

bool CXXModel::LoadModelFromMemory( unsigned char* pData, size_t uiDataSize )
{
	bool bSuccess = false;
	if( m_pXXFile )
	{
		DestroyPrivateResource();
		bSuccess = m_pXXFile->LoadModelFromMemory( pData, uiDataSize, this );
		if( bSuccess )
		{
			BuildMesh();
		}
	}
	
	return bSuccess;
}

bool CXXModel::LoadModelFromFile( char* pFileName )
{
	bool bSuccess = false;
	if( m_pXXFile )
	{
		DestroyPrivateResource();
		bSuccess = m_pXXFile->LoadModelFromFile( pFileName, this );
		if( bSuccess )
		{
			BuildMesh();
		}
	}
	
	return bSuccess;
}

bool CXXModel::LoadAnimationFromMemory( unsigned char* pData, size_t uiDataSize )
{
	bool bSuccess = false;
	if( m_pXAFile )
	{
		DestroyAnimationData();
		bSuccess = m_pXAFile->LoadModelFromMemory( pData, uiDataSize, this );
		if( bSuccess )
		{
			BuildAnimation();
		}
	}

	return bSuccess;
}

bool CXXModel::LoadAnimationFromFile( char* pFileName )
{
	bool bSuccess = false;
	if( m_pXAFile )
	{
		DestroyAnimationData();
		bSuccess = m_pXAFile->LoadModelFromFile( pFileName, this );
		if( bSuccess )
		{
			BuildAnimation();
		}
	}

	return bSuccess;
}


VOID CXXModel::DestroyPrivateResource()
{
	SAFE_DELETE( m_pBoundingBox );	

	SAFE_RELEASE( m_lpVertexBuffer );
	SAFE_RELEASE( m_lpIndexBuffer );
	SAFE_RELEASE( m_lpTexture );
	SAFE_RELEASE( m_lpResourceView );	
	
	m_fTranslateUnit = 1.0f;

	for( int i = 0; i < 3; ++i )
	{
		m_Max[i]	 = -FLT_MAX;
		m_Min[i]	 =  FLT_MAX;
		m_fCenter[i] = 0.0f;
	}	
	
	static XMMATRIX matIdentity = XMMatrixIdentity();
	memcpy( m_matTransform, &matIdentity, sizeof(XMMATRIX) );
	memcpy( m_matTranslate, &matIdentity, sizeof(XMMATRIX) );

	{
		std::vector<MESHINFO*>::iterator iter = m_vecMeshes.begin();
		while( iter != m_vecMeshes.end() )
		{
			std::vector<MESH*>::iterator inIter = (*iter)->vecMeshes.begin();
			while( inIter != (*iter)->vecMeshes.end() )
			{
				SAFE_DELETEARRAY( (*inIter)->pIndexData );
				SAFE_DELETEARRAY( (*inIter)->pVertexData );
				SAFE_RELEASE( (*inIter)->pIndexBuffer );
				SAFE_RELEASE( (*inIter)->pVertexBuffer );
				SAFE_DELETE( *inIter );
				++inIter;
			}
			SAFE_DELETE( *iter );
			++iter;
		}
		m_vecMeshes.clear();
	}

	{
		std::vector<MATERIAL>::iterator iter =
			m_vecMaterials.begin();
		while( iter != m_vecMaterials.end() )
		{
			SAFE_DELETEARRAY( iter->ppTextures );			
			++iter;
		}
		m_vecMaterials.clear();
	}	

	{
		std::map<std::string, TEXTURE>::iterator iter = m_mapTexture.begin();
		while( iter != m_mapTexture.end() )
		{
			SAFE_RELEASE( iter->second.pTexture );
			SAFE_DELETEARRAY( iter->second.pTextureData );			
			++iter;
		}
		m_mapTexture.clear();
	}

	{
		m_vecFrames.clear();
	}
}

void CXXModel::RenderObject( MESH* pMesh, int iMatId,
	XMMATRIX* pWorldViewProjMatrix, XMMATRIX* pWorldView )
{
	if( pMesh == NULL )
		return;

	UINT stride = sizeof(VERTEX_DATA);
	UINT offset = 0;

	m_lpDeviceContext->IASetIndexBuffer(
		pMesh->pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	memcpy( s_Buffer.matWorldViewProj, pWorldViewProjMatrix, sizeof(XMMATRIX) );
	memcpy( s_Buffer.matWorldView, pWorldView, sizeof(XMMATRIX) );
	static XMVECTOR vecDeterminant = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	XMMATRIX matWorldViewForNormal = XMMatrixTranspose( XMMatrixInverse( &vecDeterminant, *pWorldView) ); 
	memcpy( s_Buffer.matWorldViewForNormal, &matWorldViewForNormal, sizeof(XMMATRIX) );
		
	D3D11_MAPPED_SUBRESOURCE pData;
	m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
	memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
	m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );
	m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

	if( m_vecMaterials[iMatId].uiNumTextures == 0 )
	{
		m_lpDeviceContext->IASetInputLayout( m_lpInputLayoutForNoTexture );
		m_lpDeviceContext->IASetVertexBuffers( 0, 1, &pMesh->pVertexBuffer, &stride, &offset );			
		m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		memcpy( s_PSBuffer.color, m_vecMaterials[iMatId].Diffuse, 16 );
		s_PSBuffer.tranparency[0] = m_vecMaterials[iMatId].Transparency;

		m_lpDeviceContext->Map( m_lpPSConstBufferDiffuse, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
		memcpy_s( pData.pData, pData.RowPitch, (void*)(&s_PSBuffer), sizeof(PSConstantBuffer) );
		m_lpDeviceContext->Unmap( m_lpPSConstBufferDiffuse, 0 );
		m_lpDeviceContext->PSSetConstantBuffers( 0, 1, &m_lpPSConstBufferDiffuse );

		m_lpDeviceContext->VSSetShader( m_lpVertexShaderForNoTexture, NULL, 0 );
		m_lpDeviceContext->PSSetShader( m_lpPixelShaderForNoTexture, NULL, 0 );
		m_lpDeviceContext->GSSetShader( m_lpGeometryShaderForNoTexture, NULL, 0 );
	}
	else
	{			
		m_lpDeviceContext->IASetInputLayout( m_lpInputLayout );
		m_lpDeviceContext->IASetVertexBuffers( 0, 1, &pMesh->pVertexBuffer, &stride, &offset );			
		m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

		s_PSBuffer.iNumMeshes = m_vecMaterials[iMatId].uiNumTextures;

		m_lpDeviceContext->Map( m_lpPSConstBufferDiffuse, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
		memcpy_s( pData.pData, pData.RowPitch, (void*)(&s_PSBuffer), sizeof(PSConstantBuffer) );
		m_lpDeviceContext->Unmap( m_lpPSConstBufferDiffuse, 0 );
		m_lpDeviceContext->PSSetConstantBuffers( 0, 1, &m_lpPSConstBufferDiffuse );

		m_lpDeviceContext->VSSetShader( m_lpVertexShader, NULL, 0 );
		m_lpDeviceContext->PSSetShader( m_lpPixelShader, NULL, 0 );
		m_lpDeviceContext->GSSetShader( m_lpGeometryShader, NULL, 0 );

		m_lpDeviceContext->PSSetShaderResources( 0, 
			m_vecMaterials[iMatId].uiNumTextures, 
			m_vecMaterials[iMatId].ppTextures );
	}

	ID3D11RasterizerState* pOrig = NULL;

	m_lpDeviceContext->DrawIndexed(	pMesh->uiNumIndices, 0, 0 );	
	if( pOrig )
		m_lpDeviceContext->RSSetState( pOrig );
}

void CXXModel::Render( XMMATRIX* pWorldViewMatrix, XMMATRIX* pProjMatrix )
{
	if( m_vecMeshes.size() < 1 )
		return;

	static XMMATRIX matIdentity = XMMatrixIdentity();

	ID3D11Device* lpDevice = m_lpDevice;

	XMMATRIX WorldView = *pWorldViewMatrix * m_matTranslate;
	XMMATRIX WorldViewProj = WorldView * *pProjMatrix;

	ID3D11RasterizerState* pOrigRasterization = NULL;
	ID3D11BlendState* pOrigBlend = NULL;
	FLOAT OrigBlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	UINT OrigSampleMask = 0xFFFFFFFF;

	if( m_lpRasterizerState )
	{
		m_lpDeviceContext->RSGetState( &pOrigRasterization );		
		m_lpDeviceContext->RSSetState( m_lpRasterizerState );
	}

	if( m_lpBlendState )
	{
		m_lpDeviceContext->OMGetBlendState( &pOrigBlend, OrigBlendFactor, &OrigSampleMask );
		FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_lpDeviceContext->OMSetBlendState( m_lpBlendState, color, 0xFFFFFFFF );
	}
		
	for( int count = 1; count > -1; --count )
	{
		//std::vector<MESHINFO*>::iterator MeshIter = 
		//	m_vecMeshes.begin();
		//if( MeshIter == m_vecMeshes.end() )
		//	break;

		int iFrameMeshIndex = m_vecMeshes.size() - 1;
		if( iFrameMeshIndex < 0 )
			break;

		do
		{
			MESHINFO* pMeshInfo = m_vecMeshes[iFrameMeshIndex];
			std::vector<MESH*>::iterator iter = pMeshInfo->vecMeshes.begin();
			if( iter == pMeshInfo->vecMeshes.end() )
				continue;

			do
			{
				if( !(*iter)->bVisible )
					continue;
				
				int iMatId = (*iter)->uiMaterialId;

				if( iMatId >= m_vecMaterials.size() )
					continue;


				if( m_vecMaterials[iMatId].bIsTransparent != count  )
				{
					int iFrameId			= pMeshInfo->uiFrameIndex;
					XMMATRIX matCurrent		= matIdentity;
					XMMATRIX matTransform	= matIdentity;
					memcpy( &matCurrent, m_vecFrames[iFrameId].matCurrent, sizeof(XMMATRIX) );
					memcpy( &matTransform, m_matTransform, sizeof(XMMATRIX) );
					XMMATRIX matObject =  matCurrent * matTransform;

					RenderObject( *iter, iMatId, 
						&(matObject * WorldViewProj),
						&(matObject * WorldView) );
				}

			} while( ++iter != pMeshInfo->vecMeshes.end() );

		} while( --iFrameMeshIndex > -1 );
	}
	
	if( pOrigRasterization )
		m_lpDeviceContext->RSSetState( pOrigRasterization );

	if( pOrigBlend )
		m_lpDeviceContext->OMSetBlendState( pOrigBlend, OrigBlendFactor, OrigSampleMask );

	if( m_bShowBoundingBox )
	{
		XMMATRIX matTransform	= matIdentity;
		memcpy( &matTransform, m_matTransform, sizeof(XMMATRIX) );
		RenderBoundingBox( &(matTransform * WorldViewProj) );
	}
}


void CXXModel::RenderBoundingBox( XMMATRIX* pWorldViewProjMatrix )
{
	if( m_pBoundingBox )
		m_pBoundingBox->Render( pWorldViewProjMatrix );
}

BOOL CXXModel::SetupInput()
{
	D3D11_INPUT_ELEMENT_DESC elements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{   "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = _countof( elements );

	HRESULT hr = m_lpDevice->CreateInputLayout( elements, numElements, 
		m_lpShaderBytecodeInput,
		m_dwShaderBytecodeSize,
		&m_lpInputLayout );
	if( FAILED( hr ) )
		return FALSE;

	SAFE_DELETEARRAY( m_lpShaderBytecodeInput );
	return TRUE;
}

BOOL CXXModel::SetupInputForNoTexture()
{
	D3D11_INPUT_ELEMENT_DESC elements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{   "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = _countof( elements );

	HRESULT hr = m_lpDevice->CreateInputLayout( elements, numElements, 
		m_lpShaderBytecodeInputForNoTexture,
		m_dwShaderBytecodeSizeForNoTexture,
		&m_lpInputLayoutForNoTexture );
	if( FAILED( hr ) )
		return FALSE;

	SAFE_DELETEARRAY( m_lpShaderBytecodeInputForNoTexture );
	return TRUE;
}

BOOL CXXModel::SetupShadersForNoTexture()
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
		"	float4 position  : POSITION;\n"
		"	float3 normal	 : NORMAL;\n"
		"};\n"
		"\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"	float4 position	 : SV_POSITION;\n"
		"	float3 normal    : TEXCOORD0;\n"
		"};\n"
		"\n"
		"void main( in  VS_INPUT  vert,\n"
		"	   out VS_OUTPUT o )\n"
		"{\n"
		"	o.position   = mul( matWorldViewProj, vert.position );\n"
		"	float3 norm1 = normalize( mul((float3x3)matWorldViewForNormal, vert.normal) );\n"
		"	o.normal = norm1.xyz;\n"
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4 diffuse;\n"
		"	float  iNumTextures;\n"
		"	float  tranparency;\n"
		"};\n"
		"\n"
		"struct PS_INPUT\n"
		"{\n"
		"	float4 position	 : SV_POSITION;\n"
		"	float3 normal	 : TEXCOORD0;\n"
		"};\n"
		"\n"
		"float4 main( in PS_INPUT vert ) : SV_TARGET\n"
		"{\n"
		"	if( diffuse.a < 0.01f )\n"
		"	{\n"
		"		discard;\n"
		"	}\n"
		"	float3 vLightDir = float3(0.0f, 0.0f, -1.0f);\n"
		"	float fLightColor = saturate( dot(vert.normal, vLightDir) );\n"		
		"	float4 color = diffuse * fLightColor;	\n"
		"	color.a = diffuse.a * tranparency;\n"
		"	return color;\n"
		"}\n"
	};

	HRESULT hr = S_OK;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
	ID3D10Blob* pBlobError = NULL;

	//////////////////////////////创建Vertex Shader/////////////////////////
	{
		ID3D10Blob* pBlobVS = NULL;

		//hr = D3DX11CompileFromFile( TEXT("E:\\Projects\\Direct3D11Tutorial\\D3D11_Effect\\kmz.vsh"), NULL, NULL, 
		//	"main", "vs_4_0", 0, 0, NULL, 
		//	&pBlobVS, &pBlobError, NULL );

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

		SAFE_RELEASE( m_lpVertexShaderForNoTexture );
		hr = m_lpDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), 
			NULL, &m_lpVertexShaderForNoTexture );
		if( FAILED( hr ) )
			return FALSE;

		SAFE_DELETEARRAY( m_lpShaderBytecodeInputForNoTexture )
			m_dwShaderBytecodeSizeForNoTexture = pBlobVS->GetBufferSize();
		if( m_dwShaderBytecodeSizeForNoTexture > 0 )
		{
			m_lpShaderBytecodeInputForNoTexture = 
				new BYTE[m_dwShaderBytecodeSizeForNoTexture];
			memcpy( m_lpShaderBytecodeInputForNoTexture,
				pBlobVS->GetBufferPointer(),
				m_dwShaderBytecodeSizeForNoTexture );
		}

		pBlobVS->Release();
	}

	//////////////////////////////创建Pixel Shader/////////////////////////
	{
		ID3D10Blob* pBlobPS = NULL;

		//hr = D3DX11CompileFromFile( TEXT("E:\\Projects\\Direct3D11Tutorial\\D3D11_Effect\\kmz.psh"), NULL, NULL, 
		//	"main", "ps_4_0", 0, 0, NULL, 
		//	&pBlobPS, &pBlobError, NULL );

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

		SAFE_RELEASE( m_lpPixelShaderForNoTexture );
		hr = m_lpDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), 
			NULL, &m_lpPixelShaderForNoTexture );
		if( FAILED( hr ) )
			return FALSE;

		pBlobPS->Release();
	}

	return TRUE;
}

BOOL CXXModel::SetupShaders()
{
	static CHAR strVS[] =
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4x4 matWorldViewProj;\n"
		"	float4x4 matWorldView;\n"
		"	float4x4 matWorldViewForNormal;\n"
		"};\n"
		"struct VS_INPUT\n"
		"{\n"
		"	float4 position : POSITION;\n"
		"	float3 normal	: NORMAL;\n"
		"	float2 texcoord : TEXCOORD;\n"
		"};\n"
		"struct VS_OUTPUT\n"
		"{\n"
		"	float4 position : SV_POSITION;\n"
		"	float2 texcoord : TEXCOORD0;\n"
		"	float3 normal   : TEXCOORD1;\n"
		"};\n"
		"void main( in  VS_INPUT  vert,\n"
		"           out VS_OUTPUT o )\n"
		"{\n"   
		"   o.position = mul( matWorldViewProj, vert.position );\n"
		"   o.texcoord = vert.texcoord;\n" 
		"	float3 norm1 = normalize( mul((float3x3)matWorldViewForNormal, vert.normal) );\n"
		"	o.normal = norm1.xyz;\n"
		"}\n"
	};

	static CHAR strPS[] = 
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4 diffuse;\n"
		"	float iNumTextures;\n"
		"};\n"
		"Texture2D testMap : register(t0);\n"
		"Texture2D texture2 : register(t1);\n"
		"Texture2D texture3 : register(t2);\n"
		"Texture2D texture4 : register(t3);\n"
		"SamplerState samLinear : register(s0);\n"
		"struct PS_INPUT\n"
		"{\n"
		"	float4 position : SV_Position;\n"
		"	float2 texcoord : TEXCOORD0;\n"
		"	float3 normal	: TEXCOORD1;\n"
		"};\n"
		"float4 main( in PS_INPUT vert ) : SV_TARGET\n"
		"{\n"
		"	float4 color1 = testMap.Sample(samLinear, vert.texcoord);\n"
		"	float4 color2 = texture2.Sample(samLinear, vert.texcoord);\n"
		"	if( iNumTextures > 1.0f )\n"
		"	{\n"
		"		color1 *= color2.a;\n"
		"	}\n"
		"	if( color1.a < 0.01f )\n"
		"		discard;"
		"	float4 color3 = texture3.Sample(samLinear, vert.texcoord);\n"
		"	float4 color4 = texture4.Sample(samLinear, vert.texcoord);\n"
		"	float3 vLightDir = float3(0.0f, 0.0f, -1.0f);\n"
		"	float3 vLightDir2 = float3( 0.0f, 1.0f, 0.0f );\n"
		"	float fLightColor1 = saturate( dot(vert.normal, vLightDir) );\n"
		"	float fLightColor2 = saturate( dot(vert.normal, vLightDir) );\n"
		"	float alpha = color1.a;\n"
		"	float4 color = color1 * (fLightColor1 + fLightColor2 * 0.15f);\n"
		"	color.a = alpha;\n"		
		"	return color;\n"
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

	////////////////////创建Pixel Shader常量缓冲区/////////////////////////
	{
		D3D11_BUFFER_DESC cb;
		cb.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
		cb.ByteWidth			= sizeof( PSConstantBuffer );
		cb.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
		cb.MiscFlags			= 0;
		cb.StructureByteStride	= 0;
		cb.Usage				= D3D11_USAGE_DYNAMIC;

		hr = m_lpDevice->CreateBuffer( &cb, NULL, &m_lpPSConstBufferDiffuse );
		if( FAILED(hr) )
			return FALSE;
	}

	return TRUE;
}

VOID CXXModel::ComputeBoundingBox()
{
	//计算包围盒
#ifndef _TEST
	{
		for( int i = m_vecMeshes.size() - 1; i > - 1; --i )
		{
			std::vector<MESH*>::iterator iter = m_vecMeshes[i]->vecMeshes.begin();
			int iFrameId = m_vecMeshes[i]->uiFrameIndex;
			XMMATRIX matObject = m_vecFrames[iFrameId].matCurrent;
			while( iter != m_vecMeshes[i]->vecMeshes.end() )
			{	
				for( int j = 0; j < (*iter)->uiNumVertices; ++j )
				{
					XMVECTOR vecIn = XMVectorSet( 
						(*iter)->pVertexData[j].x,
						(*iter)->pVertexData[j].y,
						(*iter)->pVertexData[j].z,
						0.0f );
					XMVECTOR vecOut = XMVector3Transform( vecIn, matObject );				

					FLOAT vecOutX = XMVectorGetX( vecOut );
					FLOAT vecOutY = XMVectorGetY( vecOut );
					FLOAT vecOutZ = XMVectorGetZ( vecOut );

					if( vecOutX < m_Min[0] )
						m_Min[0] = vecOutX;
					if( vecOutY < m_Min[1] )
						m_Min[1] = vecOutY;
					if( vecOutZ < m_Min[2] )
						m_Min[2] = vecOutZ;

					if( vecOutX > m_Max[0] )
						m_Max[0] = vecOutX;
					if( vecOutY > m_Max[1] )
						m_Max[1] = vecOutY;
					if( vecOutZ > m_Max[2] )
						m_Max[2] = vecOutZ;
				}
				++iter;
			}					
		}
	}
#endif
	//创建顶点缓存
	{
		for( int i = m_vecMeshes.size() - 1; i > - 1; --i )
		{
			std::vector<MESH*>::iterator iter = m_vecMeshes[i]->vecMeshes.begin();
			while( iter != m_vecMeshes[i]->vecMeshes.end() )
			{
				MESH* pMesh = *iter;

				if( pMesh->uiNumIndices > 0 && pMesh->uiNumVertices )
				{
					D3D11_BUFFER_DESC bd = { 0 };

					bd.Usage			   = D3D11_USAGE_DEFAULT;
					bd.ByteWidth		   = sizeof(VERTEX_DATA) * pMesh->uiNumVertices;
					bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;

					HRESULT hr = m_lpDevice->CreateBuffer( &bd, NULL,
						&pMesh->pVertexBuffer );
					m_lpDeviceContext->UpdateSubresource( pMesh->pVertexBuffer, 0, NULL,
						pMesh->pVertexData, bd.ByteWidth, 0 );
					//SAFE_DELETEARRAY( pMesh->pVertexData );

					bd.Usage			  = D3D11_USAGE_DEFAULT;
					bd.ByteWidth		  = sizeof(short) * pMesh->uiNumIndices;
					bd.BindFlags		  = D3D11_BIND_INDEX_BUFFER;

					hr = m_lpDevice->CreateBuffer( &bd, NULL, &pMesh->pIndexBuffer );
					m_lpDeviceContext->UpdateSubresource( pMesh->pIndexBuffer, 0, NULL,
						pMesh->pIndexData, bd.ByteWidth, 0 );
					//SAFE_DELETEARRAY( pMesh->pIndexData );
				}
				else
				{
					pMesh->pVertexBuffer = NULL;
					pMesh->pIndexBuffer = NULL;
				}

				++iter;
			}	
		}
	}

	//创建包围盒顶点缓存
	{
		VERTEX_DATA vertices[8];

		vertices[0].x = m_Min[0];
		vertices[0].y = m_Min[1];
		vertices[0].z = m_Min[2];

		vertices[1].x = m_Max[0];
		vertices[1].y = m_Min[1];
		vertices[1].z = m_Min[2];

		vertices[2].x = m_Min[0];
		vertices[2].y = m_Max[1];
		vertices[2].z = m_Min[2];

		vertices[3].x = m_Max[0];
		vertices[3].y = m_Max[1];
		vertices[3].z = m_Min[2];

		vertices[4].x = m_Min[0];
		vertices[4].y = m_Min[1];
		vertices[4].z = m_Max[2];

		vertices[5].x = m_Max[0];
		vertices[5].y = m_Min[1];
		vertices[5].z = m_Max[2];

		vertices[6].x = m_Min[0];
		vertices[6].y = m_Max[1];
		vertices[6].z = m_Max[2];

		vertices[7].x = m_Max[0];
		vertices[7].y = m_Max[1];
		vertices[7].z = m_Max[2];

		WORD index[] = { 
			0, 1, 2,		//back
			2, 1, 3, 
			6, 2, 7,		//top
			7, 2, 3,
			4, 6, 5,		//front
			5, 6, 7,
			4, 5, 0,		//bottom
			0, 5, 1,
			7, 3, 5,		//right
			5, 3, 1,
			0, 2, 4,		//left
			4, 2, 6
		};

		m_pBoundingBox = new BoundingBox( m_lpDevice, m_lpDeviceContext );
		m_pBoundingBox->SetBuffer( 36, index, sizeof(WORD), 8, vertices, sizeof(VERTEX_DATA) );
	}

	FLOAT Length[3];
	for( int i = 0; i < 3; ++i )
	{
		m_fCenter[i] = (m_Min[i] + m_Max[i]) / 2.0f;
		Length[i] = (m_Max[i] - m_Min[i]) / 2.0f;
	}

	FLOAT Radius = XMVectorGetX( XMVector3Length( XMVectorSet(Length[0], Length[1], Length[2], 0.0f) ) );

	XMMATRIX matTrans;
	XMMATRIX matScale;

	if( Radius < 10.0f )
	{
		if( m_fScale > 0.0f && m_fScale < 1.0f )
			m_fScale = 1.0f;
		else if( m_fScale > -1.0f && m_fScale < 0.0f )
			m_fScale = -1.0f;
	}

	matScale = XMMatrixScaling( m_fScale, m_fScale, -m_fScale );
	matTrans = XMMatrixTranslation( -m_fCenter[0], -m_fCenter[1], -m_fCenter[2] );
	float fPositiveScale = fabs( m_fScale );

	XMMATRIX matTransform = matTrans * matScale;
	memcpy( m_matTransform, &matTransform, sizeof(XMMATRIX) );
	m_fTranslateUnit = Radius * fPositiveScale / 20.0f;

	XMMATRIX matTranslate = XMMatrixTranslation( 0.0f, 0.0f, 
		Radius * (1.0f / sin(XM_PI / 8.0f) - 1.0f) * m_fScale );
	memcpy( m_matTranslate, &matTranslate, sizeof(XMMATRIX) );
}

void CXXModel::ShowBoundingBox( BOOL bShow )
{
	m_bShowBoundingBox = bShow;
}

float CXXModel::GetTranslateUnit()
{
	return m_fTranslateUnit;
}

BOOL CXXModel::CreateRenderState()
{
	D3D11_RASTERIZER_DESC rs_desc;
	ZeroMemory( &rs_desc, sizeof(D3D11_RASTERIZER_DESC) );
	rs_desc.CullMode			  = D3D11_CULL_FRONT;
	rs_desc.FillMode			  = D3D11_FILL_SOLID;
	rs_desc.DepthClipEnable		  = TRUE;

	HRESULT hr = m_lpDevice->CreateRasterizerState( &rs_desc, &m_lpRasterizerState );
	if( FAILED(hr) )
		return FALSE;

	rs_desc.CullMode = D3D11_CULL_NONE;
	hr = m_lpDevice->CreateRasterizerState( &rs_desc, &m_lpRasterizerStateForNoCull );
	if( FAILED(hr) )
		return FALSE;

	D3D11_BLEND_DESC blend_desc;
	ZeroMemory( &blend_desc, sizeof(D3D11_BLEND_DESC) );
	//blend_desc.AlphaToCoverageEnable = TRUE;
	blend_desc.RenderTarget[0].BlendEnable			 = TRUE;
	blend_desc.RenderTarget[0].SrcBlend				 = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend			 = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp				 = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha		 = D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha		 = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha			 = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = m_lpDevice->CreateBlendState( &blend_desc, &m_lpBlendState );
	if( FAILED(hr) )
		return FALSE;
	else
		return TRUE;
}

VOID CXXModel::BuildMesh()
{
	BuildFrameMatrix();

	ComputeBoundingBox();
	
	BuildBoneMatrix();

	{
		std::map<std::string, TEXTURE>::iterator iter = m_mapTexture.begin();
		while( iter != m_mapTexture.end() )
		{
			BOOL bSuccess = GetTexture( iter->second.pTextureData, 
				iter->second.uiTextureSize, &iter->second.pTexture );
			if( !bSuccess )
				iter->second.pTexture = NULL;

			++iter;
		}
	}

	{
		for( int i = 0; i < m_vecMaterials.size(); ++i )
		{
			int counter = 0;
			if( m_vecMaterials[i].uiNumTextures == 0 )
			{
				m_vecMaterials[i].ppTextures = NULL;
				continue;
			}

			m_vecMaterials[i].ppTextures = 
					new ID3D11ShaderResourceView*[m_vecMaterials[i].uiNumTextures];
			
			for( int j = 0; j < m_vecMaterials[i].uiNumTextures; ++j )
			{
				std::map<std::string, TEXTURE>::iterator iter = 
					m_mapTexture.find(m_vecMaterials[i].vecTextureNames[j]);
				if( iter != m_mapTexture.end() && iter->second.pTexture )
				{					
					m_vecMaterials[i].ppTextures[counter] = iter->second.pTexture;
					if( iter->second.uiTextureFormat == 0x00000002 )
						m_vecMaterials[i].bIsTransparent = TRUE;

					++counter;
				}
			}
			m_vecMaterials[i].uiNumTextures = counter;
		}
	}	
}

VOID CXXModel::BuildAnimation()
{
	BuildBoneMatrix();
}

VOID CXXModel::SetFrameVisible( unsigned int uiFrameIndex, BOOL bVisible )
{
	if( uiFrameIndex >= m_vecMeshes.size() )
	{
		for( int i = 0; i < m_vecMeshes.size(); ++i )
		{
			std::vector<MESH*>::iterator iter = 
				m_vecMeshes[i]->vecMeshes.begin();
			while( iter != m_vecMeshes[i]->vecMeshes.end() )
			{
				(*iter)->bVisible = bVisible;
				++iter;
			}
			m_vecMeshes[i]->bVisible = bVisible;
		}		
	}
	else
	{
		std::vector<MESH*>::iterator iter = 
			m_vecMeshes[uiFrameIndex]->vecMeshes.begin();
		while( iter != m_vecMeshes[uiFrameIndex]->vecMeshes.end() )
		{
			(*iter)->bVisible = bVisible;
			++iter;
		}
		m_vecMeshes[uiFrameIndex]->bVisible = bVisible;
	}	
}

BOOL CXXModel::GetFrameVisible( unsigned int uiFrameIndex )
{	
	if( uiFrameIndex >= m_vecMeshes.size() )
		return FALSE;
	else
	{
		return m_vecMeshes[uiFrameIndex]->bVisible;
	}
}

BOOL CXXModel::GetTexture( unsigned char* pData, unsigned int uiDataSize,
	ID3D11ShaderResourceView** ppSRV )
{
	FIBITMAP         *pFiBitmap			= NULL;
	FIMULTIBITMAP    *pFiMultiBitmap	= NULL;
	DWORD             dwIndex			= 0;
	BOOL              bMultiBitmap		= FALSE;
	
	FIMEMORY* fi_mem			= FreeImage_OpenMemory( pData, uiDataSize );
	FREE_IMAGE_FORMAT FiFormat  = FreeImage_GetFileTypeFromMemory( fi_mem, uiDataSize );

	if( FiFormat == FIF_UNKNOWN )
	{
		FreeImage_CloseMemory( fi_mem );
		return FALSE;
	}

	if(FiFormat == FIF_JPEG)
		pFiBitmap = FreeImage_LoadFromMemory( FiFormat, fi_mem, JPEG_CMYK );
	else if( FiFormat == FIF_GIF )
		pFiBitmap = FreeImage_LoadFromMemory( FiFormat, fi_mem, GIF_PLAYBACK );
	else if( FiFormat == FIF_ICO )
		pFiBitmap = FreeImage_LoadFromMemory( FiFormat, fi_mem, ICO_MAKEALPHA );
	else
		pFiBitmap = FreeImage_LoadFromMemory( FiFormat, fi_mem, 0 );

	FIMEMORY* PngMem = FreeImage_OpenMemory();
	FreeImage_SaveToMemory( FIF_PNG, pFiBitmap, PngMem );
	FreeImage_SeekMemory( PngMem, 0, SEEK_END );
	int iPngSize = FreeImage_TellMemory( PngMem );
	unsigned char* pPngData = new unsigned char[iPngSize];
	FreeImage_SeekMemory( PngMem, 0, SEEK_SET );
	FreeImage_ReadMemory( pPngData, 1, iPngSize, PngMem );

	D3DX11_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory( &loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO) );
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	HRESULT hr = D3DX11CreateShaderResourceViewFromMemory( m_lpDevice, 
		pPngData, iPngSize, &loadInfo, NULL, ppSRV, NULL );

	FreeImage_CloseMemory( PngMem );

	if( bMultiBitmap )
	{
		FreeImage_UnlockPage( pFiMultiBitmap, pFiBitmap, FALSE );
		FreeImage_CloseMultiBitmap( pFiMultiBitmap, 0 );
	}
	else
	{
		FreeImage_Unload( pFiBitmap );	
	}
	FreeImage_CloseMemory( fi_mem );

	return TRUE;
}

VOID CXXModel::BuildFrameMatrix()
{
	static XMMATRIX matIdentity = XMMatrixIdentity();

	for( int i = 0; i < m_vecFrames.size(); ++i )
	{
		if( m_vecFrames[i].iParentId > -1 && m_vecFrames[i].iParentId < m_vecFrames.size() )
		{
			int iAnimIndex = m_vecFrames[i].iAnimBoneIndex;
			if( iAnimIndex != -1 )
			{
				XMMATRIX matRot			  = matIdentity;
				XMMATRIX matTrans		  = matIdentity;
				XMMATRIX matParentCurrent = matIdentity;
				memcpy( &matRot, m_vecBoneAnims[iAnimIndex].matRot, sizeof(XMMATRIX) );
				memcpy( &matTrans, m_vecBoneAnims[iAnimIndex].matTrans, sizeof(XMMATRIX) );
				memcpy( &matParentCurrent, m_vecFrames[m_vecFrames[i].iParentId].matCurrent, sizeof(XMMATRIX) );
				XMMATRIX matCurrent = matRot * matTrans * matParentCurrent;
				memcpy( m_vecFrames[i].matCurrent, &matCurrent, sizeof(XMMATRIX) );
			}
			else
			{
				XMMATRIX mat = matIdentity;
				XMMATRIX matParentCurrent = matIdentity;
				memcpy( &mat, m_vecFrames[i].matrix, sizeof(XMMATRIX) );
				memcpy( &matParentCurrent, m_vecFrames[m_vecFrames[i].iParentId].matCurrent, sizeof(XMMATRIX) );
				XMMATRIX matCurrent = mat * matParentCurrent;
				memcpy( m_vecFrames[i].matCurrent, &matCurrent, sizeof(XMMATRIX) );
			}
		}
		else
			memcpy( m_vecFrames[i].matCurrent, m_vecFrames[i].matrix, sizeof(XMMATRIX) );
	}	
}

VOID CXXModel::DestroyAnimationData()
{
	m_iTotalFrames = 0.0f;

	m_vecBoneAnims.clear();
	m_vecSequences.clear();
}

VOID CXXModel::UpdateBoneMatrix( float fTimeDiff )
{
	static XMVECTOR vZero = XMVectorZero();
	static XMMATRIX matIdentity = XMMatrixIdentity();

	if( m_iTotalFrames == 0 && m_vecSequences.size() > 0 )
	{
		for( int i = 0; i < m_vecSequences.size(); i++ )
		{
			if( m_vecSequences[i].uiStartTime == m_vecSequences[i].uiEndTime  )
				continue;
			m_AnimTime.uiStartTime = m_vecSequences[i].uiStartTime;
			m_AnimTime.uiEndTime   = m_vecSequences[i].uiEndTime;
			break;
		}

	}

	if( m_iTotalFrames == 0 || m_AnimTime.uiStartTime == m_AnimTime.uiEndTime )
		return;

	m_AnimTime.fTime += fTimeDiff * 10;
	if( (m_AnimTime.uiEndTime <= m_iTotalFrames) && (m_AnimTime.uiStartTime >= 0) )
	{
		if( m_AnimTime.fTime <= m_AnimTime.uiStartTime )
			m_AnimTime.fTime = (float)m_AnimTime.uiStartTime + fTimeDiff;
		if( m_AnimTime.fTime >= m_AnimTime.uiEndTime )
			m_AnimTime.fTime = m_AnimTime.uiStartTime;
	}
	else
	{
		if( m_AnimTime.fTime > m_iTotalFrames )
			m_AnimTime.fTime = 0;
	}

	for( int i = 0; i < m_vecBoneAnims.size(); ++i )
	{
		unsigned int index = m_AnimTime.fTime;
		if( index > m_vecBoneAnims[i].vecKeyframes.size() - 1 )
			index = m_vecBoneAnims[i].vecKeyframes.size() - 1;

		XMVECTOR quat  = vZero;
		XMVECTOR trans = vZero;
		memcpy( &quat, m_vecBoneAnims[i].vecKeyframes[index].quaternion, sizeof(XMVECTOR) );
		memcpy( &trans, m_vecBoneAnims[i].vecKeyframes[index].translate, sizeof(XMVECTOR) );
		XMMATRIX matRot = XMMatrixRotationQuaternion( quat );
		XMMATRIX matTrans = XMMatrixTranslationFromVector( trans );
		memcpy( m_vecBoneAnims[i].matRot, &matRot, sizeof(XMMATRIX) );
		memcpy( m_vecBoneAnims[i].matTrans, &matTrans, sizeof(XMMATRIX) );
	}
	if( m_vecBoneAnims.size() > 0 )
		BuildFrameMatrix();

}

int CXXModel::GetNumSequences()
{
	return m_vecSequences.size();
}

const CXXModel::SEQUENCE* CXXModel::GetSequenceByID( int iIndex )
{
	if( iIndex > m_vecSequences.size() - 1 )
		return NULL;
	else
		return &m_vecSequences[iIndex];
}

void CXXModel::SetSequenceByIndex( int iIndex )
{
	if( iIndex > m_vecSequences.size() - 1 )
		return;
	else
	{
		m_AnimTime.uiStartTime = m_vecSequences[iIndex].uiStartTime;
		m_AnimTime.uiEndTime = m_vecSequences[iIndex].uiEndTime;
	}
}

VOID CXXModel::BuildBoneMatrix()
{
	for( int i = 0; i < m_vecFrames.size(); ++i )
	{
		bool bFound = false;
		for( int j = 0; j < m_vecBoneAnims.size(); ++j )
		{
			if( m_vecFrames[i].strFrameName == m_vecBoneAnims[j].strBoneName )
			{
				m_vecFrames[i].iAnimBoneIndex = j;
				bFound = true;
				break;
			}			
		}
		if( !bFound )
			m_vecFrames[i].iAnimBoneIndex = -1;
	}
}

void CXXModel::SetTransform( XMMATRIX* pMatTransform, XMMATRIX* pMatTranlate )
{
	memcpy( m_matTransform, pMatTransform, sizeof(XMMATRIX) );
	memcpy( m_matTranslate, pMatTranlate, sizeof(XMMATRIX) );
}

void CXXModel::GetTransform( XMMATRIX* pMatTransform, XMMATRIX* pMatTranlate )
{
	memcpy( pMatTransform, m_matTransform, sizeof(XMMATRIX) );
	memcpy( pMatTranlate, m_matTranslate, sizeof(XMMATRIX) );
}