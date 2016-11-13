//
// Copyright (C) Mei Jun 2011
//

#include "KMZLoader.h"
#include "Utility.h"

#include <zzip/zzip.h>

#ifdef _DEBUG
	#pragma comment( lib, "dom_d.lib" )
	#pragma comment( lib, "zlib_d.lib")
	#pragma comment( lib, "zziplib_d.lib")
#else
	#pragma comment( lib, "dom.lib" )
	#pragma comment( lib, "zlib.lib" )
	#pragma comment( lib, "zziplib.lib")
#endif

KMZLoader::ConstantBuffer KMZLoader::s_Buffer;
KMZLoader::PSConstantBuffer KMZLoader::s_PSBuffer;

ID3D11InputLayout* KMZLoader::m_lpInputLayout = NULL;
ID3D11VertexShader* KMZLoader::m_lpVertexShader = NULL;
ID3D11PixelShader* KMZLoader::m_lpPixelShader = NULL;
ID3D11GeometryShader* KMZLoader::m_lpGeometryShader = NULL;
ID3D11Buffer* KMZLoader::m_lpVSConstBufferWorldViewProj = NULL;
DWORD KMZLoader::m_dwShaderBytecodeSize = 0;
VOID* KMZLoader::m_lpShaderBytecodeInput = NULL;
DWORD KMZLoader::m_Reference = 0;

ID3D11InputLayout* KMZLoader::m_lpInputLayoutForNoTexture = NULL;
ID3D11VertexShader*	KMZLoader::m_lpVertexShaderForNoTexture = NULL;
ID3D11PixelShader* KMZLoader::m_lpPixelShaderForNoTexture = NULL;
ID3D11GeometryShader* KMZLoader::m_lpGeometryShaderForNoTexture = NULL;
DWORD KMZLoader::m_dwShaderBytecodeSizeForNoTexture = 0;
VOID* KMZLoader::m_lpShaderBytecodeInputForNoTexture = NULL;
ID3D11Buffer* KMZLoader::m_lpPSConstBufferDiffuse = NULL;

ID3D11RasterizerState* KMZLoader::m_lpRasterizerState = NULL;
ID3D11RasterizerState* KMZLoader::m_lpRasterizerStateForNoCull = NULL;

KMZLoader::KMZLoader( ID3D11Device* lpDevice, ID3D11DeviceContext* lpDeviceContext )
{
	m_lpDevice = lpDevice;
	m_lpDeviceContext = lpDeviceContext;

	m_pRoot		 = NULL;
	m_fScale	 = 1.0f;

	m_lpTexture = NULL;
	m_lpResourceView = NULL;

	m_lpResourceViewExchange = NULL;
	m_lpVertexBuffer = NULL;
	m_lpIndexBuffer = NULL;

	++m_Reference;
	if( m_Reference == 1 )
	{
		SetupShaders();
		SetupShadersForNoTexture();
		SetupInput();
		SetupInputForNoTexture();
		CreateRenderState();
	}

	for( int i = 0; i < 3; ++i )
	{
		m_Max[i] = -FLT_MAX;
		m_Min[i] =  FLT_MAX;
		m_fCenter[i] = 0.0f;
	}	

	m_fTranslateUnit = 1.0f;

	m_pBoundingBox = NULL;
	m_bShowBoundingBox = FALSE;

	static XMMATRIX matIdentity = XMMatrixIdentity();
	memcpy( m_matTransform, &matIdentity, sizeof(XMMATRIX) );
	memcpy( m_matTranslate, &matIdentity, sizeof(XMMATRIX) );
}

KMZLoader::~KMZLoader(void)
{
	DestroyPrivateResource();

	--m_Reference;
	if( m_Reference == 0 )
	{
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
	}
}

VOID KMZLoader::DestroyPrivateResource()
{
	SAFE_DELETE( m_pRoot );
	SAFE_DELETE( m_pBoundingBox );

	//清除Collada模型对象链
	{
		std::list<KMZLoader*>::iterator iter = 
			m_listDaeLoaders.begin();
		while( iter != m_listDaeLoaders.end() )
		{
			SAFE_DELETE(*iter);
			++iter;
		}
		m_listDaeLoaders.clear();
	}

	//清除纹理数据
	{
		std::map<std::string, ID3D11ShaderResourceView*>::iterator iter = 
			m_mapTextures.begin();
		while( iter != m_mapTextures.end() )
		{
			SAFE_RELEASE(iter->second);
			++iter;
		}
		m_mapTextures.clear();
	}

	SAFE_RELEASE( m_lpVertexBuffer );
	SAFE_RELEASE( m_lpIndexBuffer );
	SAFE_RELEASE( m_lpTexture );
	SAFE_RELEASE( m_lpResourceView );	

	//清除几何数据
	{
		std::vector<GEOMETRY>::iterator iter = m_vecGeometries.begin();
		while( iter != m_vecGeometries.end() )
		{
			int numMeshes = (*iter).nNumMeshes;
			for( int j= 0; j < numMeshes; j++ )
			{
				MESH* pMesh = &(*iter).pMeshes[j];
				SAFE_RELEASE( (*iter).pMeshes[j].pVertexBuffer );
			}
			SAFE_DELETEARRAY( (*iter).pMeshes );
			++iter;
		}
	}

	m_fTranslateUnit = 1.0f;

	for( int i = 0; i < 3; ++i )
	{
		m_Max[i] = -FLT_MAX;
		m_Min[i] =  FLT_MAX;
		m_fCenter[i] = 0.0f;
	}

	static XMMATRIX matIdentity = XMMatrixIdentity();
	memcpy( m_matTransform, &matIdentity, sizeof(XMMATRIX) );
	memcpy( m_matTranslate, &matIdentity, sizeof(XMMATRIX) );
}

void KMZLoader::Render( XMMATRIX* pWorldViewMatrix, XMMATRIX* pProjMatrix )
{
	ID3D11Device* lpDevice = m_lpDevice;	

	if( m_listDaeLoaders.size() > 0 )
	{
		static XMMATRIX matIdentity = XMMatrixIdentity();
		std::list<KMZLoader*>::iterator iter =
			m_listDaeLoaders.begin();	
		 
		while( iter != m_listDaeLoaders.end() )
		{
			XMMATRIX matTranslate = matIdentity;
			memcpy( &matTranslate, (*iter)->m_matTranslate, sizeof(XMMATRIX) );
			XMMATRIX matWorldView = *pWorldViewMatrix * matTranslate;
			XMMATRIX matWorldViewProj = matWorldView * *pProjMatrix;

			ID3D11RasterizerState* pOrig = NULL;
			if( m_lpRasterizerState )
			{
				m_lpDeviceContext->RSGetState( &pOrig );
				m_lpDeviceContext->RSSetState( m_lpRasterizerState );
			}
			TraverseSceneGraphForOpaque(lpDevice, *iter, &matWorldViewProj, &matWorldView );
			TraverseSceneGraphForTransparent(lpDevice, *iter, &matWorldViewProj, &matWorldView );
			if( pOrig )
				m_lpDeviceContext->RSSetState( pOrig );

			if( m_bShowBoundingBox )
			{				
				XMMATRIX matTransform = matIdentity;
				memcpy( &matTransform, (*iter)->m_matTransform, sizeof(XMMATRIX) );
				(*iter)->RenderBoundingBox( &( matTransform * matWorldViewProj) );
			}
			++iter;
		}
	}
}

void KMZLoader::RenderBoundingBox( XMMATRIX* pWorldViewProjMatrix )
{
	if( m_pBoundingBox )
		m_pBoundingBox->Render( pWorldViewProjMatrix );
}

void KMZLoader::TraverseSceneGraphForOpaque( ID3D11Device* lpDevice,
	 KMZLoader* pDaeLoader, XMMATRIX* pWorldViewProjMatrix, XMMATRIX* pWorldView )
{
	std::list<scene_node_t* >::iterator iter =
		pDaeLoader->m_listRenderableNodes.begin();
	static XMMATRIX matIdentity = XMMatrixIdentity();
	static XMVECTOR vZero = XMVectorZero();

	while( iter != pDaeLoader->m_listRenderableNodes.end() )
	{		
		XMMATRIX matTransform = matIdentity;
		memcpy( &matTransform, (*iter)->_transform, sizeof(XMMATRIX) );
		XMMATRIX matPTransform = matIdentity;
		memcpy( &matPTransform, pDaeLoader->m_matTransform, sizeof(XMMATRIX) );
		XMMATRIX WorldMatrix = matTransform * matPTransform;
	
		int numGeos = (*iter)->_geos.size();
		for( int i = 0; i < numGeos; ++i )
		{
			int numMeshes = (*iter)->_geos[i]->nNumMeshes;
			for( int j= 0; j < numMeshes; j++ )
			{
				DWORD materialID = (*iter)->_geos[i]->pMeshes[j].nMaterialID;

				static BOOL bComplete = FALSE;
				if( materialID != 0xFFFFFFFF )
				{
					if( pDaeLoader->m_vecMaterials[materialID].bIsTransparent )
						continue;
					
					UINT stride = sizeof(VERTEX_DATA);
					UINT offset = 0;

					if( pDaeLoader->m_vecMaterials[materialID].pTexture )
					{
						m_lpDeviceContext->IASetInputLayout( m_lpInputLayout );
						m_lpDeviceContext->IASetVertexBuffers( 0, 1, &(*iter)->_geos[i]->pMeshes[j].pVertexBuffer, &stride, &offset );			
						m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

						memcpy( s_Buffer.matWorldViewProj, &(WorldMatrix * (*pWorldViewProjMatrix)), sizeof(XMMATRIX) );
						D3D11_MAPPED_SUBRESOURCE pData;
						m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
						memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
						m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );

						m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

						m_lpDeviceContext->VSSetShader( m_lpVertexShader, NULL, 0 );
						m_lpDeviceContext->PSSetShader( m_lpPixelShader, NULL, 0 );
						m_lpDeviceContext->GSSetShader( m_lpGeometryShader, NULL, 0 );

						m_lpDeviceContext->PSSetShaderResources( 0, 1, &pDaeLoader->m_vecMaterials[materialID].pTexture );					
					}
					else
					{
						m_lpDeviceContext->IASetInputLayout( m_lpInputLayoutForNoTexture );
						m_lpDeviceContext->IASetVertexBuffers( 0, 1, &(*iter)->_geos[i]->pMeshes[j].pVertexBuffer, &stride, &offset );			
						m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

						memcpy( s_Buffer.matWorldViewProj, &(WorldMatrix * (*pWorldViewProjMatrix)), sizeof(XMMATRIX) );
						memcpy( s_Buffer.matWorldView, &(WorldMatrix * *pWorldView), sizeof(XMMATRIX) );
						XMVECTOR vecDeterminant = vZero;
						XMMATRIX matWorldViewForNormal =
							XMMatrixTranspose( XMMatrixInverse( &vecDeterminant, s_Buffer.matWorldView) ); 
						memcpy( s_Buffer.matWorldViewForNormal, &matWorldViewForNormal, sizeof(XMMATRIX) );

						D3D11_MAPPED_SUBRESOURCE pData;
						m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
						memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
						m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );
						m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

						memcpy( s_PSBuffer.color, 
							pDaeLoader->m_vecMaterials[materialID].Diffuse, sizeof(float) * 4 );
						s_PSBuffer.tranparency = 
							pDaeLoader->m_vecMaterials[materialID].Transparency;
						memcpy( s_PSBuffer.specular, 
							pDaeLoader->m_vecMaterials[materialID].Specular, sizeof(float) * 4 );
						s_PSBuffer.power[0] = 
							pDaeLoader->m_vecMaterials[materialID].Power;

						m_lpDeviceContext->Map( m_lpPSConstBufferDiffuse, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
						memcpy_s( pData.pData, pData.RowPitch, (void*)(&s_PSBuffer), sizeof(PSConstantBuffer) );
						m_lpDeviceContext->Unmap( m_lpPSConstBufferDiffuse, 0 );
						m_lpDeviceContext->PSSetConstantBuffers( 0, 1, &m_lpPSConstBufferDiffuse );

						m_lpDeviceContext->VSSetShader( m_lpVertexShaderForNoTexture, NULL, 0 );
						m_lpDeviceContext->PSSetShader( m_lpPixelShaderForNoTexture, NULL, 0 );
						m_lpDeviceContext->GSSetShader( m_lpGeometryShaderForNoTexture, NULL, 0 );
					}
				}
				else
					continue;
				
				ID3D11RasterizerState* pOrig = NULL;
				if( pDaeLoader->m_vecMaterials[materialID].bDoubleSided )
				{
					m_lpDeviceContext->RSGetState( &pOrig );
					m_lpDeviceContext->RSSetState( m_lpRasterizerStateForNoCull );
				}

				m_lpDeviceContext->Draw(
					(*iter)->_geos[i]->pMeshes[j].nNumVertices, 0 );	
				if( pOrig )
					m_lpDeviceContext->RSSetState( pOrig );
			}
		}
		++iter;		
	}
}

void KMZLoader::TraverseSceneGraphForTransparent( ID3D11Device* lpDevice,
	KMZLoader* pDaeLoader, XMMATRIX* pWorldViewProjMatrix, XMMATRIX* pWorldView )
{
	std::list<scene_node_t* >::iterator iter =
		pDaeLoader->m_listRenderableNodes.begin();
	static XMMATRIX matIdentity = XMMatrixIdentity();
	static XMVECTOR vZero = XMVectorZero();

	while( iter != pDaeLoader->m_listRenderableNodes.end() )
	{		
		XMMATRIX matTransform = matIdentity;
		memcpy( &matTransform, (*iter)->_transform, sizeof(XMMATRIX) );
		XMMATRIX matPTransform = matIdentity;
		memcpy( &matPTransform, pDaeLoader->m_matTransform, sizeof(XMMATRIX) );
		XMMATRIX WorldMatrix = matTransform * matPTransform;

		int numGeos = (*iter)->_geos.size();
		for( int i = 0; i < numGeos; ++i )
		{
			int numMeshes = (*iter)->_geos[i]->nNumMeshes;
			for( int j= 0; j < numMeshes; j++ )
			{
				DWORD materialID = (*iter)->_geos[i]->pMeshes[j].nMaterialID;

				static BOOL bComplete = FALSE;
				if( materialID != 0xFFFFFFFF )
				{
					if( !pDaeLoader->m_vecMaterials[materialID].bIsTransparent )
						continue;

					UINT stride = sizeof(VERTEX_DATA);
					UINT offset = 0;

					if( pDaeLoader->m_vecMaterials[materialID].pTexture )
					{
						m_lpDeviceContext->IASetInputLayout( m_lpInputLayout );
						m_lpDeviceContext->IASetVertexBuffers( 0, 1, &(*iter)->_geos[i]->pMeshes[j].pVertexBuffer, &stride, &offset );			
						m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

						memcpy( s_Buffer.matWorldViewProj, &(WorldMatrix * (*pWorldViewProjMatrix)), sizeof(XMMATRIX) );
						D3D11_MAPPED_SUBRESOURCE pData;
						m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
						memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
						m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );

						m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

						m_lpDeviceContext->VSSetShader( m_lpVertexShader, NULL, 0 );
						m_lpDeviceContext->PSSetShader( m_lpPixelShader, NULL, 0 );
						m_lpDeviceContext->GSSetShader( m_lpGeometryShader, NULL, 0 );

						m_lpDeviceContext->PSSetShaderResources( 0, 1, &pDaeLoader->m_vecMaterials[materialID].pTexture );					
					}
					else
					{
						m_lpDeviceContext->IASetInputLayout( m_lpInputLayoutForNoTexture );
						m_lpDeviceContext->IASetVertexBuffers( 0, 1, &(*iter)->_geos[i]->pMeshes[j].pVertexBuffer, &stride, &offset );			
						m_lpDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

						memcpy( s_Buffer.matWorldViewProj, &(WorldMatrix * (*pWorldViewProjMatrix)), sizeof(XMMATRIX) );
						memcpy( s_Buffer.matWorldView, &(WorldMatrix * *pWorldView), sizeof(XMMATRIX) );
						XMVECTOR vecDeterminant = vZero;
						XMMATRIX matWorldViewForNormal =
							XMMatrixTranspose( XMMatrixInverse( &vecDeterminant, s_Buffer.matWorldView) ); 
						memcpy( s_Buffer.matWorldViewForNormal, &matWorldViewForNormal, sizeof(XMMATRIX) );

						D3D11_MAPPED_SUBRESOURCE pData;
						m_lpDeviceContext->Map( m_lpVSConstBufferWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
						memcpy_s( pData.pData, pData.RowPitch, (void*)( &s_Buffer ), sizeof(ConstantBuffer) );
						m_lpDeviceContext->Unmap( m_lpVSConstBufferWorldViewProj, 0 );
						m_lpDeviceContext->VSSetConstantBuffers( 0, 1, &m_lpVSConstBufferWorldViewProj );

						memcpy( s_PSBuffer.color, 
							pDaeLoader->m_vecMaterials[materialID].Diffuse, sizeof(float) * 4 );
						s_PSBuffer.tranparency = 
							pDaeLoader->m_vecMaterials[materialID].Transparency;
						memcpy( s_PSBuffer.specular, 
							pDaeLoader->m_vecMaterials[materialID].Specular, sizeof(float) * 4 );
						s_PSBuffer.power[0] = 
							pDaeLoader->m_vecMaterials[materialID].Power;

						m_lpDeviceContext->Map( m_lpPSConstBufferDiffuse, 0, D3D11_MAP_WRITE_DISCARD, 0, &pData );
						memcpy_s( pData.pData, pData.RowPitch, (void*)(&s_PSBuffer), sizeof(PSConstantBuffer) );
						m_lpDeviceContext->Unmap( m_lpPSConstBufferDiffuse, 0 );
						m_lpDeviceContext->PSSetConstantBuffers( 0, 1, &m_lpPSConstBufferDiffuse );

						m_lpDeviceContext->VSSetShader( m_lpVertexShaderForNoTexture, NULL, 0 );
						m_lpDeviceContext->PSSetShader( m_lpPixelShaderForNoTexture, NULL, 0 );
						m_lpDeviceContext->GSSetShader( m_lpGeometryShaderForNoTexture, NULL, 0 );
					}
				}
				else
					continue;

				ID3D11RasterizerState* pOrig = NULL;
				if( pDaeLoader->m_vecMaterials[materialID].bDoubleSided )
				{
					m_lpDeviceContext->RSGetState( &pOrig );
					m_lpDeviceContext->RSSetState( m_lpRasterizerStateForNoCull );
				}
				m_lpDeviceContext->Draw(
					(*iter)->_geos[i]->pMeshes[j].nNumVertices, 0 );	
				if( pOrig )
					m_lpDeviceContext->RSSetState( pOrig );
			}
		}
		++iter;		
	}
}

void KMZLoader::LoadColladaMeshFromKMZ( LPCTSTR lpFilename )
{
	DestroyPrivateResource();

	std::map<std::string, DATA_SIZE> file_stream;
	std::list<std::string> dae_list;

	size_t len = WideCharToMultiByte( CP_ACP, 0, lpFilename, -1, NULL, 0, NULL, NULL );
	char* pFileName = new char[len];
	WideCharToMultiByte( CP_ACP, 0, lpFilename, -1, pFileName, len, NULL, NULL );
	ZZIP_DIR* dir = zzip_opendir( pFileName );
	SAFE_DELETEARRAY( pFileName );

	ZZIP_DIRENT* dirrent = NULL;
	while(  dirrent = zzip_readdir( dir ) )
	{
		size_t length = strlen(dirrent->d_name);
		if( length > 0 )
		{
			if( dirrent->d_name[length - 1] != '\\' &&
				dirrent->d_name[length - 1] != '/' )
			{
				ZZIP_FILE* pFile = zzip_file_open( dir, dirrent->d_name, ZZIP_CASELESS );
				if( pFile != NULL )
				{
					ZZIP_STAT sz;
					ZeroMemory( &sz, sizeof(sz) );
					zzip_file_stat( pFile, &sz );
					if( sz.st_size > 0 )
					{
						bool bDae = false;
						if( length > 4 )
						{
							if( !stricmp(dirrent->d_name + length - 4, ".dae") )
							{
								dae_list.push_back( _strlwr(dirrent->d_name) );
								bDae = true;
							}
						}
						char* pBuffer = NULL;
						if( bDae )
						{
							pBuffer = new char[sz.st_size + 1];
							pBuffer[sz.st_size] = 0;
						}
						else
							pBuffer = new char[sz.st_size];

						size_t num = zzip_file_read( pFile, pBuffer, sz.st_size );
						if( bDae )
						{
							char* p = pBuffer;
							while( ++p - pBuffer < sz.st_size )
							{
								if( *p == 0 )
									*p = 0x20;
							}
						}
						DATA_SIZE data_size = { pBuffer, sz.st_size };						

						file_stream.insert( std::make_pair( _strlwr(dirrent->d_name), data_size) );
					}
					zzip_file_close( pFile );
				}
			}
		}
	}
	zzip_closedir( dir );

	//if( false )
	{
		std::list<std::string>::iterator iter = 
			dae_list.begin();
		while( iter != dae_list.end() )
		{
			KMZLoader* pDaeLoader = new KMZLoader( m_lpDevice, m_lpDeviceContext );
			pDaeLoader->LoadColladaMeshFromMemory( file_stream, iter->c_str() );
			m_listDaeLoaders.push_back(pDaeLoader );
			++iter;
			if( pDaeLoader )
				m_fTranslateUnit = pDaeLoader->m_fTranslateUnit;
		}
	}

	{
		std::map<std::string, DATA_SIZE>::iterator iter = 
			file_stream.begin();
		while( iter != file_stream.end() )
		{
			SAFE_DELETEARRAY(iter->second.pData);
			++iter;
		}
	}
}	

void KMZLoader::LoadColladaMeshFromMemory( std::map<std::string, DATA_SIZE>& filelist, 
	LPCSTR lpDaeName )
{
	std::map<std::string, DATA_SIZE>::iterator file_iter = 
		filelist.find(lpDaeName);
	if( file_iter == filelist.end() )
		return;

	DAE* pDae = new DAE;
	domCOLLADA* pDom = pDae->openFromMemory( std::string(lpDaeName), file_iter->second.pData );

	m_fScale = 1.0f;
	if( pDom->getAsset()->getUnit() )
	{
		m_fScale = pDom->getAsset()->getUnit()->getMeter();
	}

	ReadImages( pDom, filelist );
	ReadEffects( pDom );
	ReadMaterials( pDom );
	ReadGeometries( pDom );
	ReadNodes( pDom );
	ReadScene( pDom );
	ComputeBoundingBox();

	pDae->cleanup();
	SAFE_DELETE(pDae);
}

void KMZLoader::ReadImages( domCOLLADA* pDom,
	std::map<std::string, DATA_SIZE>& filelist )
{
	int num = pDom->getLibrary_images_array().getCount();

	for( int idx = 0; idx < num; ++idx )
	{
		domLibrary_imagesRef lib = pDom->getLibrary_images_array()[idx];
		size_t numImages = lib->getImage_array().getCount();

		ID3D11Device* lpDevice = m_lpDevice;
		for( size_t i = 0; i < numImages; ++i )
		{
			domImage* pImage = lib->getImage_array()[i];
			const domImage::domInit_fromRef initFrom = pImage->getInit_from();
			const char* pImageFilename = initFrom->getValue().getURI();
			const char* pBaseURI = pDom->getDAE()->getBaseURI().getOriginalURI();
			size_t name_len = strlen(pImageFilename);
			char* pFileName = new char[name_len + 1];
			memcpy( pFileName, pImageFilename, name_len + 1 );
			for( size_t j = 0; j < name_len; ++j )
			{
				if( pFileName[j] == '\\' )
					pFileName[j] = '/';
			}

			size_t Buffer_Len = strlen(pBaseURI) - 2;
			std::map<std::string, DATA_SIZE>::iterator file_iter = 
				filelist.find(_strlwr(pFileName + Buffer_Len));
			SAFE_DELETEARRAY( pFileName );

			ID3D11ShaderResourceView* lpTexture = NULL;
			//if( false )
			{
				if( file_iter != filelist.end() )
				{		
					D3DX11_IMAGE_LOAD_INFO loadInfo;
					ZeroMemory( &loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO) );
					loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					loadInfo.Format = DXGI_FORMAT_BC1_UNORM;

					D3DX11CreateShaderResourceViewFromMemory( lpDevice, file_iter->second.pData,
						file_iter->second.dwSize, &loadInfo, NULL, &lpTexture, NULL );
				}

				m_mapTextures.insert( std::make_pair(pImage->getID(), lpTexture) );
			}
		}
		break;
	}
}

void KMZLoader::ReadEffects( domCOLLADA* pDom )
{
	size_t num = pDom->getLibrary_effects_array().getCount();

	for( size_t idx = 0; idx < num; ++idx )
	{
		domLibrary_effectsRef lib = pDom->getLibrary_effects_array()[idx];
		size_t numEffects = lib->getEffect_array().getCount();
		for( size_t i = 0; i < numEffects; ++i )
		{
			domEffect* pEffect = lib->getEffect_array()[i];
			const char* pEffectId = pEffect->getId();
			size_t numProfiles = pEffect->getFx_profile_abstract_array().getCount();
			for( size_t j = 0; j < numProfiles; ++j )
			{
				domFx_profile_abstract* pProfile = 
					pEffect->getFx_profile_abstract_array()[j];
				const char* pTypeName = pProfile->getTypeName();
				if( !stricmp("profile_COMMON", pTypeName) )
				{
					domProfile_COMMON* pCommon =
						(domProfile_COMMON*)pProfile;
					domProfile_COMMON::domTechniqueRef pTech = pCommon->getTechnique();

					domProfile_COMMON::domTechnique::domLambert* pLambert = 
						pTech->getLambert();
					domProfile_COMMON::domTechnique::domPhong* pPhong = 
						pTech->getPhong();
					
					daeElement* pLightModel = NULL;
					domCommon_color_or_texture_type_complexType::domTextureRef pTexture = NULL;
					if( pLambert )
					{
						pLightModel = pLambert;
						pTexture = pLambert->getDiffuse()->getTexture();
					}
					else if( pPhong )
					{
						pLightModel = pPhong;
						pTexture = pPhong->getDiffuse()->getTexture();
					}

					std::pair<daeElement*, std::list<domExtraRef> > pairLightModelExtra;
					pairLightModelExtra.first = pLightModel;
					size_t numExtras = pCommon->getExtra_array().getCount();
					for( size_t k = 0; k < numExtras; ++k )
					{
						domExtraRef pExtra = pCommon->getExtra_array()[k];
						pairLightModelExtra.second.push_back( pExtra );
					}

					numExtras = pEffect->getExtra_array().getCount();
					for( size_t k = 0; k < numExtras; ++k )
					{
						domExtraRef pExtra = pEffect->getExtra_array()[k];
						pairLightModelExtra.second.push_back( pExtra );
					}

					m_mapEffects.insert(std::make_pair(pEffectId, pairLightModelExtra));

					ID3D11ShaderResourceView* lpTexture = NULL;
					if( pTexture != NULL )
					{
						const char* pImageId = pTexture->getTexture();
						size_t numNewparams = pCommon->getNewparam_array().getCount();
						for( size_t k = 0; k < numNewparams; ++k )
						{
							domCommon_newparam_typeRef pNewparam =
								pCommon->getNewparam_array()[k];
							if( !strcmp( pNewparam->getSid(), pImageId ) )
							{
								for( size_t l = 0; l < numNewparams; ++l )
								{
									domCommon_newparam_typeRef pNewparam2 =
										pCommon->getNewparam_array()[l];
									if( pNewparam->getSampler2D()->getSource() != NULL )
									{
										const char* pSourceId =
											pNewparam->getSampler2D()->getSource()->getValue();
										if( pSourceId != NULL )
										{
											if( !strcmp( pNewparam2->getSid(), 
												pSourceId) )
											{
												size_t numContents = pNewparam2->getSurface()->getContents().getCount();
												for( size_t m = 0; m < numContents; ++m )
												{
													const char* pElemName = 
														pNewparam2->getSurface()->getContents()[m]->getElementName();
													if( !stricmp( pElemName, "init_from" ) )
													{
														daeElementRef pElem = pNewparam2->getSurface()->getContents()[m];
														std::string TextureId;
														pElem->getCharData(TextureId);
														std::map<std::string, ID3D11ShaderResourceView*>::iterator iter =
															m_mapTextures.find(TextureId);
														if( iter != m_mapTextures.end() )
														{
															lpTexture = iter->second;
														}
														break;
													}													
												}												
												break;
											}
										}
									}
								}
								break;
							}
						}
					}


					m_mapEffectTextures.insert(std::make_pair(pEffectId, lpTexture) );
				}
			}
		}
		break;
	}
}

void KMZLoader::ReadMaterials( domCOLLADA* pDom )
{
	size_t num = pDom->getLibrary_materials_array().getCount();

	for( size_t idx = 0; idx < num; ++idx )
	{
		domLibrary_materialsRef lib = pDom->getLibrary_materials_array()[idx];
		size_t numMaterials = lib->getMaterial_array().getCount();
		m_vecMaterials.resize(numMaterials);

		for( size_t i = 0; i < numMaterials; ++i )
		{
			domMaterial* pMaterial = lib->getMaterial_array()[i];
			domInstance_effectRef pEffect = pMaterial->getInstance_effect();
			const char* pEffectId = pEffect->getUrl().getOriginalURI();

			std::map<std::string, std::pair<daeElement*, std::list<domExtraRef> > >::iterator iter = 
				m_mapEffects.find( pEffectId + 1);
			std::map<std::string, ID3D11ShaderResourceView*>::iterator iter2 =
				m_mapEffectTextures.find( pEffectId + 1);

			MATERIAL material;
			if( iter != m_mapEffects.end() )
			{
				float AmbientR = 1.0f;
				float AmbientG = 1.0f;
				float AmbientB = 1.0f;
				float AmbientA = 1.0f;

				float DiffuseR = 1.0f;
				float DiffuseG = 1.0f;
				float DiffuseB = 1.0f;
				float DiffuseA = 1.0f;

				float SpecularR = 0.0f;
				float SpecularG = 0.0f;
				float SpecularB = 0.0f;
				float SpecularA = 1.0f;

				float EmissionR = 0.0f;
				float EmissionG = 0.0f;
				float EmissionB = 0.0f;
				float EmissionA = 1.0f;

				float Power = 0.0f;

				domProfile_COMMON::domTechnique::domLambert* pLambert = NULL;
				domProfile_COMMON::domTechnique::domPhong* pPhong = NULL;

				if( iter->second.first->typeID() == 743 )
				{
					pLambert = (domProfile_COMMON::domTechnique::domLambert*)iter->second.first;
				}

				if( iter->second.first->typeID() == 744 )
				{
					pPhong = (domProfile_COMMON::domTechnique::domPhong*)iter->second.first;
				}

				ZeroMemory( &material, sizeof(material));

				if( pLambert )
				{
					if( pLambert->getAmbient() )
					{
						if( pLambert->getAmbient()->getColor() != NULL )
						{
							AmbientR = pLambert->getAmbient()->getColor()->getValue().get(0);
							AmbientG = pLambert->getAmbient()->getColor()->getValue().get(1);
							AmbientB = pLambert->getAmbient()->getColor()->getValue().get(2);
							AmbientA = pLambert->getAmbient()->getColor()->getValue().get(3);
						}
					}

					if( pLambert->getDiffuse() )
					{
						if( pLambert->getDiffuse()->getColor() != NULL )
						{
							DiffuseR = pLambert->getDiffuse()->getColor()->getValue().get(0);
							DiffuseG = pLambert->getDiffuse()->getColor()->getValue().get(1);
							DiffuseB = pLambert->getDiffuse()->getColor()->getValue().get(2);
							DiffuseA = pLambert->getDiffuse()->getColor()->getValue().get(3);
						}				
					}

					if( pLambert->getEmission() )
					{
						if( pLambert->getEmission()->getColor() != NULL )
						{
							EmissionR = pLambert->getEmission()->getColor()->getValue().get(0);
							EmissionG = pLambert->getEmission()->getColor()->getValue().get(1);
							EmissionB = pLambert->getEmission()->getColor()->getValue().get(2);
							EmissionA = pLambert->getEmission()->getColor()->getValue().get(3);
						}
					}

					material.Ambient[0] = AmbientR;
					material.Ambient[1] = AmbientG;
					material.Ambient[2] = AmbientB;
					material.Ambient[3] = AmbientA;

					material.Diffuse[0] = DiffuseR;
					material.Diffuse[1] = DiffuseG;
					material.Diffuse[2] = DiffuseB;
					material.Diffuse[3] = DiffuseA;

					material.Specular[0] = SpecularR;
					material.Specular[1] = SpecularG;
					material.Specular[2] = SpecularB;
					material.Specular[3] = SpecularA;

					material.Emissive[0] = EmissionR;
					material.Emissive[1] = EmissionB;
					material.Emissive[2] = EmissionG;
					material.Emissive[3] = EmissionA;

					material.Power = Power;

					material.pTexture = iter2->second;

					material.bIsTransparent = FALSE;

					if( pLambert->getTransparency() )
					{
						material.Transparency =
							(float)pLambert->getTransparency()->getFloat()->getValue();
						if( material.Transparency < 0.001f )
							material.Transparency = 1.0f;						
					}
					else
						material.Transparency = 1.0f;
				}
				else if( pPhong )
				{
					if( pPhong->getAmbient() )
					{
						if( pPhong->getAmbient()->getColor() != NULL )
						{
							AmbientR = pPhong->getAmbient()->getColor()->getValue().get(0);
							AmbientG = pPhong->getAmbient()->getColor()->getValue().get(1);
							AmbientB = pPhong->getAmbient()->getColor()->getValue().get(2);
							AmbientA = pPhong->getAmbient()->getColor()->getValue().get(3);
						}
					}

					if( pPhong->getDiffuse() )
					{
						if( pPhong->getDiffuse()->getColor() != NULL )
						{
							DiffuseR = pPhong->getDiffuse()->getColor()->getValue().get(0);
							DiffuseG = pPhong->getDiffuse()->getColor()->getValue().get(1);
							DiffuseB = pPhong->getDiffuse()->getColor()->getValue().get(2);
							DiffuseA = pPhong->getDiffuse()->getColor()->getValue().get(3);
						}
					}

					if( pPhong->getSpecular() )
					{
						if( pPhong->getSpecular()->getColor() != NULL )
						{
							SpecularR = pPhong->getSpecular()->getColor()->getValue().get(0);
							SpecularG = pPhong->getSpecular()->getColor()->getValue().get(1);
							SpecularB = pPhong->getSpecular()->getColor()->getValue().get(2);
							SpecularA = pPhong->getSpecular()->getColor()->getValue().get(3);
						}
					}

					if( pPhong->getEmission() )
					{
						if( pPhong->getEmission()->getColor() != NULL )
						{
							EmissionR = pPhong->getEmission()->getColor()->getValue().get(0);
							EmissionG = pPhong->getEmission()->getColor()->getValue().get(1);
							EmissionB = pPhong->getEmission()->getColor()->getValue().get(2);
							EmissionA = pPhong->getEmission()->getColor()->getValue().get(3);
						}
					}

					if( pPhong->getShininess() )
					{
						Power = (float)pPhong->getShininess()->getFloat()->getValue();
					}
					
					material.Ambient[0] = AmbientR;
					material.Ambient[1] = AmbientG;
					material.Ambient[2] = AmbientB;
					material.Ambient[3] = AmbientA;

					material.Diffuse[0] = DiffuseR;
					material.Diffuse[1] = DiffuseG;
					material.Diffuse[2] = DiffuseB;
					material.Diffuse[3] = DiffuseA;

					material.Specular[0] = SpecularR;
					material.Specular[1] = SpecularG;
					material.Specular[2] = SpecularB;
					material.Specular[3] = SpecularA;

					material.Emissive[0] = EmissionR;
					material.Emissive[1] = EmissionB;
					material.Emissive[2] = EmissionG;
					material.Emissive[3] = EmissionA;

					material.Power = Power;

					material.pTexture = iter2->second;

					material.bIsTransparent = FALSE;

					if( pPhong->getTransparency() )
					{
						material.Transparency = 
							(float)pPhong->getTransparency()->getFloat()->getValue();
						if( material.Transparency < 0.001f )
							material.Transparency = 1.0f;
					}
					else
						material.Transparency = 1.0f;
				}
			}

			if( material.Transparency < 1.0f || material.Diffuse[3] < 1.0f )
				material.bIsTransparent = TRUE;

			std::list<domExtraRef>::iterator iterExtra = iter->second.second.begin();
			while( iterExtra != iter->second.second.end() )
			{
				size_t uiNumTechs = (*iterExtra)->getTechnique_array().getCount();
				for( size_t l = 0; l < uiNumTechs; ++l )
				{
					domTechniqueRef pTech = (*iterExtra)->getTechnique_array()[l];
					size_t uiNumContents = pTech->getContents().getCount();
					for( size_t m = 0; m < uiNumContents; ++m )
					{
						daeElementRef pElem = pTech->getContents()[m];
						std::string strElemName = pElem->getElementName();
						if( strcmp(strElemName.c_str(), "double_sided") == 0 )
						{
							std::string strValue;
							pElem->getCharData( strValue );
							if( strcmp(strValue.c_str(), "1") == 0 )
								material.bDoubleSided = TRUE;
						}
					}
				}
				++iterExtra;
			}
			m_vecMaterials[i] = material;

			if( pMaterial->getId() )
			{
				m_mapMaterials[pMaterial->getId()] = i;
			}
		}
		break;
	}
}

void KMZLoader::ReadGeometries( domCOLLADA* pDom )
{
	size_t num = pDom->getLibrary_geometries_array().getCount();

	for( size_t idx = 0; idx < num; ++idx )
	{
		domLibrary_geometriesRef lib = pDom->getLibrary_geometries_array()[idx];
		size_t numGeometries = lib->getGeometry_array().getCount();
		m_vecGeometries.resize(numGeometries);

		for( size_t i = 0; i < numGeometries; ++i )
		{
			domGeometryRef pGeometry = lib->getGeometry_array()[i];
			domMeshRef pMesh = pGeometry->getMesh();

			if( pMesh != NULL )
			{
				size_t numMeshes = 
					pMesh->getTriangles_array().getCount();
				size_t numSources = 
					pMesh->getSource_array().getCount();

				std::map<std::string, domSourceRef> mapSources;
				std::map<std::string, domSourceRef> mapVertexInputs;				

				for( size_t j = 0; j < numSources; ++j )
				{
					domSourceRef pSource = pMesh->getSource_array()[j];
					if( pSource->getID() != NULL )
					{
						mapSources.insert( 
							std::make_pair(pSource->getID(), pSource) );
					}
				}

				if( pMesh->getVertices() != NULL )
				{
					size_t numInputs = 
						pMesh->getVertices()->getInput_array().getCount();

					for( size_t j = 0; j < numInputs; ++j )
					{						
						domInputLocalRef pInput = 
							pMesh->getVertices()->getInput_array()[j];
						const char* pUri = pInput->getSource().getOriginalURI();
						std::map<std::string, domSourceRef>::iterator iter = 
							mapSources.find(pUri + 1);

						if( iter != mapSources.end() )
						{
							if( !stricmp( pInput->getSemantic(), "POSITION") )
							{
								mapVertexInputs["POSITION"] = iter->second;								
							}
							else if( !stricmp( pInput->getSemantic(), "NORMAL"))
							{
								mapVertexInputs["NORMAL"] = iter->second;

							}
							else if( !stricmp( pInput->getSemantic(), "TEXCOORD"))
							{
								mapVertexInputs["TEXCOORD"] = iter->second;
							}
						}
					}
					mapSources[pMesh->getVertices()->getId()] = NULL;					
				}

				m_vecGeometries[i].nNumMeshes	= numMeshes;
				m_vecGeometries[i].pMeshes		= new MESH[numMeshes];
				m_vecGeometries[i].type			= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				m_vecGeometries[i].index		= i;

				for( size_t j = 0; j < numMeshes; ++j )
				{
					std::map<std::string, domSourceRef> mapTypeSources;

					domTrianglesRef pTriangle = 
						pMesh->getTriangles_array()[j];
					size_t numInputs = 
						pTriangle->getInput_array().getCount();

					bool bHasNormal = false;
					bool bHasTexcoord = false;

					domSourceRef pNormal = NULL;
					domSourceRef pTexcoord = NULL;
					domSourceRef pPosition = NULL;

					bool bNormalInVertex = false;
					bool bTexcoordInVertex = false;

					std::map<std::string, int> mapInputOrder;
					size_t curOrderIndex = 0;

					for( size_t k = 0; k < numInputs; ++k )
					{
						domInputLocalOffsetRef pInput =
							pTriangle->getInput_array()[k];
						const char* pUri =
							pInput->getSource().getOriginalURI();
						std::map<std::string, domSourceRef>::iterator iter = 
							mapSources.find(pUri + 1);
						if( iter != mapSources.end() )
						{
							if( !stricmp( pInput->getSemantic(), "VERTEX" ) )
							{
								std::map<std::string, domSourceRef>::iterator iterPosition =
									mapVertexInputs.find("POSITION");
								std::map<std::string, domSourceRef>::iterator iterNormal =
									mapVertexInputs.find("NORMAL");
								std::map<std::string, domSourceRef>::iterator iterTexcoord =
									mapVertexInputs.find("TEXCOORD");

								if( iterPosition != mapVertexInputs.end() )
								{
									pPosition = iterPosition->second;
								}
								if( iterNormal != mapVertexInputs.end() )
								{
									bHasNormal = true;
									bNormalInVertex = true;
									pNormal = iterNormal->second;									
								}
								if( iterTexcoord != mapVertexInputs.end() )
								{
									bHasTexcoord = true;
									bTexcoordInVertex = true;
									pTexcoord = iterTexcoord->second;	
								}
							}
							else if( !stricmp( pInput->getSemantic(), "NORMAL") )
							{
								pNormal = iter->second;
								bHasNormal = true;
							}
							else if( !stricmp( pInput->getSemantic(), "TEXCOORD") )
							{
								pTexcoord = iter->second;
								bHasTexcoord = true;
							}
						}
						mapInputOrder[pInput->getSemantic()] = curOrderIndex;
						++curOrderIndex;
					}

					domPRef pP = pTriangle->getP();

					if( pP != NULL )
					{						
						size_t numPArray	= pP->getValue().getCount();
						size_t indexCounter = 0;
						size_t numFaces		= pTriangle->getCount();

						m_vecGeometries[i].pMeshes[j].bHasNormal	= bHasNormal;
						m_vecGeometries[i].pMeshes[j].bHasTexcoord	= bHasTexcoord;
						m_vecGeometries[i].pMeshes[j].nMaterialID	= 0xFFFFFFFF;
						m_vecGeometries[i].pMeshes[j].materialName  = pTriangle->getMaterial(); 
						m_vecGeometries[i].pMeshes[j].nNumVertices	= 3 * numFaces;
						m_vecGeometries[i].pMeshes[j].pVertices = new VERTEX_DATA[3 * numFaces];
						if( pGeometry->getName() )
							m_vecGeometries[i].pMeshes[j].name = pGeometry->getName();
						int stride = 2;
						if( pTexcoord )
						{
							domSource::domTechnique_commonRef pTech = 
								pTexcoord->getTechnique_common();
							if( pTech )
							{
								domAccessorRef pAccessor = pTech->getAccessor();
								if( pAccessor )
								{
									stride = pAccessor->getStride();
								}
							}
						}

						size_t numIndices = pP->getValue().getCount();

						int offsetVertex = 0;
						int offsetNormal = 0;
						int offsetTexcoord = 0;

						if( pPosition )
						{
							offsetVertex = mapInputOrder["VERTEX"];
							offsetNormal = offsetVertex;
							offsetTexcoord = offsetNormal;
						}

						for( size_t n = 0; 
							n < m_vecGeometries[i].pMeshes[j].nNumVertices; 
							n++ )
						{
							if( pPosition != NULL )
							{
								size_t index = pP->getValue()[n * numInputs + offsetVertex];
								m_vecGeometries[i].pMeshes[j].pVertices[n].x =
									pPosition->getFloat_array()->getValue().get(index * 3);
								m_vecGeometries[i].pMeshes[j].pVertices[n].y = 
									pPosition->getFloat_array()->getValue().get(index * 3 + 1);
								m_vecGeometries[i].pMeshes[j].pVertices[n].z = 
									pPosition->getFloat_array()->getValue().get(index * 3 + 2);
							}

							if( pNormal != NULL )
							{
								if( !bNormalInVertex )
									offsetNormal = mapInputOrder["NORMAL"];

								size_t index = pP->getValue()[n * numInputs + offsetNormal];
								m_vecGeometries[i].pMeshes[j].pVertices[n].normal[0] =
									pNormal->getFloat_array()->getValue().get(index * 3);
								m_vecGeometries[i].pMeshes[j].pVertices[n].normal[1] = 
									pNormal->getFloat_array()->getValue().get(index * 3 + 1);
								m_vecGeometries[i].pMeshes[j].pVertices[n].normal[2] = 
									pNormal->getFloat_array()->getValue().get(index * 3 + 2);
							}

							if( pTexcoord != NULL )
							{	
								if( !bTexcoordInVertex )									
									offsetTexcoord = mapInputOrder["TEXCOORD"];

								size_t index = pP->getValue()[n * numInputs + offsetTexcoord];
								m_vecGeometries[i].pMeshes[j].pVertices[n].u = pTexcoord->getFloat_array()->getValue().get(index * stride);
								m_vecGeometries[i].pMeshes[j].pVertices[n].v = 1.0 - pTexcoord->getFloat_array()->getValue().get(index * stride + 1);								
							}
						}
					}
				}

				if( pGeometry->getID() )
					m_mapGeomtries[pGeometry->getID()] = &m_vecGeometries[i];
			}
		}
		break;
	}
}

void KMZLoader::TraverseAndSetupTransform( domNodeRef pNode,
	scene_node_t* pSceneNode )
{
	static XMMATRIX matIdentity = XMMatrixIdentity();
	static XMVECTOR vZero = XMVectorZero();

	if( pSceneNode != NULL )
	{
		memcpy( pSceneNode->_transform, &matIdentity, sizeof(XMMATRIX) );
	}
	if( pNode == NULL )
		return;

	size_t numTranslation	= pNode->getTranslate_array().getCount();
	size_t numRotation		= pNode->getRotate_array().getCount();
	size_t numScaling		= pNode->getScale_array().getCount();
	size_t numMatrices		= pNode->getMatrix_array().getCount();

	BOOL bIsTranslationExist	= FALSE;
	BOOL bIsRotationExist		= FALSE;
	BOOL bIsScalingExist		= FALSE;
	BOOL bIsMatrixExist			= FALSE;

	XMVECTOR vecTranslation;

	XMVECTOR vecAxis;
	FLOAT RotAngle;

	XMVECTOR vecScale;

	XMMATRIX matRot;
	XMMATRIX matScale;
	XMMATRIX matTrans;

	XMMATRIX tempMatrix = matIdentity;

	for( size_t i = 0; i < numRotation; ++i )
	{
		domRotateRef pRotation = pNode->getRotate_array()[i];
		float axis_x = pRotation->getValue().get(0);
		float axis_y = pRotation->getValue().get(1);
		float axis_z = pRotation->getValue().get(2);
		float angle = pRotation->getValue().get(3);
		vecAxis = XMVectorSet(axis_x, axis_y, axis_z, 0.0f);
		RotAngle = angle;
		bIsRotationExist = TRUE;
	}

	for( size_t i = 0; i < numTranslation; ++i )
	{
		domTranslateRef pTranslation = pNode->getTranslate_array()[i];
		float translation_x = pTranslation->getValue().get(0);
		float translation_y = pTranslation->getValue().get(1);
		float translation_z = pTranslation->getValue().get(2);
		vecTranslation = XMVectorSet(translation_x, translation_y, translation_z, 0.0f);
		bIsTranslationExist = TRUE;
	}

	for( size_t i = 0; i < numScaling; ++i )
	{
		domScaleRef pScaling = pNode->getScale_array()[i];
		float scaling_x = pScaling->getValue().get(0);
		float scaling_y = pScaling->getValue().get(1);
		float scaling_z = pScaling->getValue().get(2);
		vecScale = XMVectorSet(scaling_x, scaling_y, scaling_z, 0.0f);
		bIsScalingExist = TRUE;
	}

	for( size_t i = 0; i < numMatrices; ++i )
	{
		domMatrixRef pMatrix = pNode->getMatrix_array()[i];

		tempMatrix._11 = pMatrix->getValue().get(0);
		tempMatrix._21 = pMatrix->getValue().get(1);
		tempMatrix._31 = pMatrix->getValue().get(2);
		tempMatrix._41 = pMatrix->getValue().get(3);

		tempMatrix._12  = pMatrix->getValue().get(4);
		tempMatrix._22  = pMatrix->getValue().get(5);
		tempMatrix._32  = pMatrix->getValue().get(6);
		tempMatrix._42 = pMatrix->getValue().get(7);

		tempMatrix._13 = pMatrix->getValue().get(8);
		tempMatrix._23 = pMatrix->getValue().get(9);
		tempMatrix._33 = pMatrix->getValue().get(10);
		tempMatrix._43 = pMatrix->getValue().get(11);

		tempMatrix._14 = pMatrix->getValue().get(12);
		tempMatrix._24 = pMatrix->getValue().get(13);
		tempMatrix._34 = pMatrix->getValue().get(14);
		tempMatrix._44 = pMatrix->getValue().get(15);

		bIsMatrixExist = TRUE;
	}

	if( bIsTranslationExist )
	{
		matTrans = XMMatrixTranslationFromVector( vecTranslation );
		XMMATRIX matNodeTransform = matIdentity;
		memcpy( &matNodeTransform, pSceneNode->_transform, sizeof(XMMATRIX) );
		matNodeTransform = matTrans * matNodeTransform;
		memcpy( pSceneNode->_transform, &matNodeTransform, sizeof(XMMATRIX) );
	}

	if( bIsRotationExist )
	{
		matRot = XMMatrixRotationAxis( vecAxis, RotAngle );
		XMMATRIX matNodeTransform = matIdentity;
		memcpy( &matNodeTransform, pSceneNode->_transform, sizeof(XMMATRIX) );
		matNodeTransform = matRot * matNodeTransform;
		memcpy( pSceneNode->_transform, &matNodeTransform, sizeof(XMMATRIX) );
	}

	if( bIsScalingExist )
	{
		matScale = XMMatrixScalingFromVector( vecScale );
		XMMATRIX matNodeTransform = matIdentity;
		memcpy( &matNodeTransform, pSceneNode->_transform, sizeof(XMMATRIX) );
		matNodeTransform = matScale * matNodeTransform;
		memcpy( pSceneNode->_transform, &matNodeTransform, sizeof(XMMATRIX) );
	}

	if( bIsMatrixExist )
		memcpy( pSceneNode->_transform, &tempMatrix, sizeof(XMMATRIX) );

	if( pSceneNode->_pparent != NULL )
	{
		XMMATRIX matNodeTransform = matIdentity;
		memcpy( &matNodeTransform, pSceneNode->_transform, sizeof(XMMATRIX) );
		XMMATRIX matParentTransform = matIdentity;
		memcpy( &matParentTransform, pSceneNode->_pparent->_transform, sizeof(XMMATRIX) );
		matNodeTransform *= matParentTransform;
		memcpy( pSceneNode->_transform, &matNodeTransform, sizeof(XMMATRIX) );
	}
	else
	{
		FLOAT scale = 1.0f;

		XMMATRIX WorldMatrix;
		XMMATRIX TransMatrix;
		XMMATRIX RotMatrix, RotMatrixX, RotMatrixY;
		XMMATRIX ScalingMatrix;

		ScalingMatrix = XMMatrixScaling( scale, scale, scale );

		TransMatrix = XMMatrixTranslation( 0.0f, 0.0f, 0.0f );

		RotMatrixY = XMMatrixRotationY( 0.0f );
		RotMatrixX = XMMatrixRotationX( -XM_PI / 2.0f );
		RotMatrix = RotMatrixY * RotMatrixX;
		WorldMatrix = ScalingMatrix * RotMatrix * TransMatrix;

		XMMATRIX matNodeTransform = matIdentity;
		memcpy( &matNodeTransform, pSceneNode->_transform, sizeof(XMMATRIX) );
		matNodeTransform *= WorldMatrix;
		memcpy( pSceneNode->_transform, &matNodeTransform, sizeof(XMMATRIX) );
	}

	if( !pSceneNode->_bAdded )
	{
		m_listRenderableNodes.push_back( pSceneNode );
		pSceneNode->_bAdded = TRUE;
	}

	int numInstanceGeometry = pNode->getInstance_geometry_array().getCount();
	for( int i = 0; i < numInstanceGeometry; ++i )
	{
		domInstance_geometryRef pGeometry = pNode->getInstance_geometry_array()[i];
		const char* pUri = pGeometry->getUrl().getOriginalURI();
		std::map<std::string, GEOMETRY*>::iterator iter =
			m_mapGeomtries.find(pUri + 1);
		if( iter != m_mapGeomtries.end() )
		{
			pSceneNode->_geos.push_back( iter->second );
			domBind_materialRef pBindMaterial = pGeometry->getBind_material();
			domBind_material::domTechnique_commonRef pTech = 
				pBindMaterial->getTechnique_common();
			std::map<std::string, std::string> mapSymbolTargets;
			int numInstanceMaterials = 
				pTech->getInstance_material_array().getCount();
			for( int j = 0; j < numInstanceMaterials; ++j )
			{
				domInstance_materialRef pMaterial = 
					pTech->getInstance_material_array()[j];
				if( pMaterial->getSymbol() )
					mapSymbolTargets[pMaterial->getSymbol()] = 
					pMaterial->getTarget().getOriginalURI() + 1;
			}
			for( size_t j = 0; j < iter->second->nNumMeshes; ++j )
			{
				MESH* pMesh = &iter->second->pMeshes[j];
				std::map<std::string, std::string>::iterator iterSymbol = 
					mapSymbolTargets.find(pMesh->materialName);
				if( iterSymbol != mapSymbolTargets.end() )
				{
					std::map<std::string, int>::iterator iterMaterial =
						m_mapMaterials.find(iterSymbol->second);
					if( iterMaterial != m_mapMaterials.end() )
					{
						pMesh->nMaterialID = iterMaterial->second;
					}
				}
			}		
		}
	}

	size_t numInstanceNode = pNode->getInstance_node_array().getCount();
	size_t numNodes = pNode->getNode_array().getCount();
	size_t numTotalNodes = numInstanceNode + numNodes;
	if( numTotalNodes > 0 )
		pSceneNode->_children.resize(numTotalNodes);

	for( size_t i = 0; i < numInstanceNode; ++i )
	{
		domInstance_nodeRef pInstanceNode = 
			pNode->getInstance_node_array()[i];
		const char* pUri = pInstanceNode->getUrl().getOriginalURI();
		std::map<std::string, domNodeRef>::iterator iter =
			m_mapNodes.find( pUri + 1 );
		if( iter != m_mapNodes.end() )
		{
			domNodeRef pChildNode = iter->second;
			scene_node_t pMyNode;
			pMyNode._pparent = pSceneNode;
			pMyNode._bAdded	= FALSE;
			pSceneNode->_children[i] = pMyNode;
			TraverseAndSetupTransform( pChildNode, &(pSceneNode->_children[i]) );
		}
	}

	for( size_t i = numInstanceNode; i < numTotalNodes ; ++i )
	{
		domNodeRef pChildNode = pNode->getNode_array()[i];
		scene_node_t pMyNode;
		pMyNode._pparent = pSceneNode;
		pMyNode._bAdded	= FALSE;
		pSceneNode->_children[i] = pMyNode;
		TraverseAndSetupTransform( pChildNode, &(pSceneNode->_children[i]) );
	}
}

void KMZLoader::ReadNodes( domCOLLADA* pDom )
{
	size_t num = pDom->getLibrary_nodes_array().getCount();
	for( size_t idx = 0; idx < num; ++idx )
	{
		domLibrary_nodesRef lib = pDom->getLibrary_nodes_array()[idx];
		size_t numNodes = lib->getNode_array().getCount();
		for( size_t i = 0; i < numNodes; ++i )
		{
			domNodeRef pNode = lib->getNode_array()[i];
			if( pNode->getId() )
			{
				m_mapNodes[pNode->getId()] = pNode;
			}
		}
		break;
	}
}

void KMZLoader::ReadScene( domCOLLADA* pDom )
{
	m_pRoot = new scene_node_t;
	m_pRoot->_pparent = NULL;
	m_pRoot->_bAdded  = FALSE;

	domCOLLADA::domSceneRef pScene = pDom->getScene();
	daeElement* pElement = 
		pScene->getInstance_visual_scene()->getUrl().getElement();

	if( pElement != NULL )
	{		
		domVisual_sceneRef pVisualScene = (domVisual_scene*)pElement;
		size_t numNodes = pVisualScene->getNode_array().getCount();

		for( size_t i = 0; i < numNodes; ++i )
		{
			domNodeRef pNode = pVisualScene->getNode_array()[i];
			TraverseAndSetupTransform( pNode, m_pRoot );
		}
	}
}

BOOL KMZLoader::SetupInput()
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

BOOL KMZLoader::SetupInputForNoTexture()
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

BOOL KMZLoader::SetupShadersForNoTexture()
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
		"	float4 specular;\n"
		"	float  tranparency;\n"
		"	float  power;\n"
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
		"	float3 vLightDir = float3(0.0f, 0.0f, -1.0f);\n"
		"	float fLightColor = saturate( dot(vert.normal, vLightDir) );\n"
		"	float fLightFactor = saturate( dot(vert.normal, normalize(float3(0.0f, 0.0f, -2.0f))));"
		"	float4 color = diffuse * fLightColor + specular * pow(fLightFactor, power);	\n"
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

BOOL KMZLoader::SetupShaders()
{
	static CHAR strVS[] =
	{
		"cbuffer main : register(b0)\n"
		"{\n"
		"	float4x4 matWorldViewProj;\n"
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
		"	return testMap.Sample(samLinear, vert.texcoord);\n"
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

VOID KMZLoader::ComputeBoundingBox()
{
	//计算包围盒
	{
		std::list<scene_node_t* >::iterator iter =
			m_listRenderableNodes.begin();

		while( iter != m_listRenderableNodes.end() )
		{		
			XMMATRIX WorldMatrix = (*iter)->_transform;

			int numGeos = (*iter)->_geos.size();
			for( int i = 0; i < numGeos; ++i )
			{
				size_t numMeshes = (*iter)->_geos[i]->nNumMeshes;
				for( size_t j= 0; j < numMeshes; j++ )
				{
					for( size_t k = 0; k < (*iter)->_geos[i]->pMeshes[j].nNumVertices; ++k )
					{
						XMVECTOR vecIn = XMVectorSet(
							(*iter)->_geos[i]->pMeshes[j].pVertices[k].x,
							(*iter)->_geos[i]->pMeshes[j].pVertices[k].y,
							(*iter)->_geos[i]->pMeshes[j].pVertices[k].z,
							0.0f );

						XMVECTOR vecOut = XMVector3Transform( vecIn, (*iter)->_transform );
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
				}
			}
			++iter;		
		}
	}
	//创建顶点缓存
	{
		std::vector<GEOMETRY>::iterator iter =
			m_vecGeometries.begin();
		while( iter != m_vecGeometries.end() )
		{
			int numMeshes = (*iter).nNumMeshes;
			for( int j= 0; j < numMeshes; j++ )
			{
				MESH* pMesh = &(*iter).pMeshes[j];
				if( pMesh->nNumVertices > 0 )
				{
					D3D11_BUFFER_DESC bd = { 0 };

					bd.Usage			   = D3D11_USAGE_DEFAULT;
					bd.ByteWidth		   = sizeof(VERTEX_DATA) * pMesh->nNumVertices;
					bd.BindFlags		   = D3D11_BIND_VERTEX_BUFFER;

					HRESULT hr = m_lpDevice->CreateBuffer( &bd, NULL,
						&pMesh->pVertexBuffer );
					m_lpDeviceContext->UpdateSubresource( pMesh->pVertexBuffer, 0, NULL,
						pMesh->pVertices, bd.ByteWidth, 0 );
					SAFE_DELETEARRAY( pMesh->pVertices );
				}
				else
					pMesh->pVertexBuffer = NULL;
			}
			++iter;
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

	matScale = XMMatrixScalingFromVector( XMVectorSet(m_fScale,	m_fScale, -m_fScale, 0.0f) );	
	matTrans = XMMatrixTranslationFromVector( XMVectorSet(-m_fCenter[0], -m_fCenter[1], -m_fCenter[2], 0.0f) );
	float fPositiveScale = fabs( m_fScale );

	XMMATRIX matTransform = matTrans * matScale;
	memcpy( m_matTransform, &matTransform, sizeof(XMMATRIX) );
	m_fTranslateUnit = Radius * fPositiveScale / 20.0f;

	XMMATRIX matTranslate = XMMatrixTranslationFromVector( XMVectorSet(0.0f, 0.0f, 
		Radius * (1.0f / sin(XM_PI / 8.0f) - 1.0f) * m_fScale, 0.0f) );
	memcpy( m_matTranslate, &matTranslate, sizeof(XMMATRIX) );
}

void KMZLoader::ShowBoundingBox( BOOL bShow )
{
	m_bShowBoundingBox = bShow;
}

float KMZLoader::GetTranslateUnit()
{
	return m_fTranslateUnit;
}

BOOL KMZLoader::CreateRenderState()
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
	else
		return TRUE;	
}