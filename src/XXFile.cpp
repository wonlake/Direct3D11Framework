//
// Copyright (C) Mei Jun 2011
//

#include "XXFile.h"
#include "XXModel.h"

CXXFile::CXXFile(void)
{
	m_pModel = NULL;
}


CXXFile::~CXXFile(void)
{
	m_pModel = NULL;
}

bool CXXFile::LoadModelFromMemory( unsigned char* pData,
	size_t uiDataSize, CXXModel* pModel )
{
	m_pModel = pModel;

	size_t uiDataPos = 0;
	char cFrameType = 0;

	memcpy( &cFrameType, pData + uiDataPos, 1 );
	uiDataPos += 26;

	ParseFrame( pData, uiDataSize, uiDataPos, cFrameType, -1 );
	
	//Unknown Data A
	uiDataPos += 4;

	ParseMaterial( pData, uiDataSize, uiDataPos, cFrameType );

	ParseTexture(pData, uiDataSize, uiDataPos, cFrameType );
	
	return true;
}

bool CXXFile::LoadModelFromFile( char* pFileName, CXXModel* pModel )
{
	size_t uiDataSize = 0;
	unsigned char* pData = NULL;

	FILE* pFile = fopen( pFileName, "rb" );
	if( pFile == NULL )
		return false;
	fseek( pFile, 0, SEEK_END );

	uiDataSize = ftell( pFile );
	pData = new unsigned char[uiDataSize];	

	fseek( pFile, 0, SEEK_SET );
	fread( pData, 1, uiDataSize, pFile );
	fclose( pFile );

	bool bSuccess = LoadModelFromMemory( pData, uiDataSize, pModel );	
	SAFE_DELETEARRAY( pData );

	return bSuccess;
}

void CXXFile::ParseName( unsigned char* pData, size_t uiDataSize,
	size_t& uiPos, std::string& strName )
{
	char name[4096] = { 0 };
	int iNameLen = 0;

	memcpy( &iNameLen, pData + uiPos, 4 );
	uiPos += 4;
	if( iNameLen > 0 && iNameLen < 4096 )
	{
		memcpy( name, pData + uiPos, iNameLen );
		for( int i = 0; i < iNameLen; ++i )
			name[i] = ~name[i];
		strName = name;
	}
	uiPos += iNameLen;		
}

void CXXFile::ParseVertex( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
	int iFrameType )
{
	Vertex_1 *pVertert1 = NULL;
	Vertex_2 *pVertert2 = NULL;

	if( iFrameType < 4 )
	{
		pVertert1 = (Vertex_1 *)(pData + uiPos);
		uiPos += sizeof( Vertex_1 );
	}
	else
	{
		pVertert2 = (Vertex_2 *)(pData + uiPos);
		uiPos += sizeof( Vertex_2 );
	}

}

void CXXFile::ParseMesh( unsigned char* pData, size_t uiDataSize, size_t& uiPos, 
	int iFrameType, int iMeshType, CXXModel::MESH *pMesh )
{
	char cMatFlags = 0;
	char cSpecial = 0;
	// Seems to be:
	// 00 64 00 00 00 00 00 00 00 00 00 00 00 00 00 00 - normal
	// 04 64 00 00 00 00 00 00 00 00 00 00 00 00 00 00 - hilight
	// 06 64 00 00 00 00 00 00 00 00 00 00 00 00 00 00 - hilight (on top of transparent object?)
	// 08 64 00 00 00 00 00 00 00 00 00 00 00 00 00 00 - shadows
	// 0a 64 00 00 00 00 00 00 00 00 00 00 00 00 00 00 - shadows
	// 0c 64 00 00 00 00 00 00 00 00 00 00 00 00 00 00 - shadows, positive, with transparency
	
	// 0e 64 00 00 00 40 00 00 00 00 00 00 00 00 00 00 - special, each vertex add extra 16 bytes
	// 0c 64 00 00 00 62 00 00 00 00 00 00 00 00 00 00 - special, each vertex add extra 16 bytes
	// 0e 64 00 00 00 62 00 00 00 00 00 00 00 00 00 00 - special, each vertex add extra 16 bytes
	memcpy( &cMatFlags, pData + uiPos, 1 );
	memcpy( &cSpecial, pData + uiPos + 5, 1 );
	uiPos += 16;

	int iMaterialIndex = 0;
	memcpy( &iMaterialIndex, pData + uiPos, 4 );
	uiPos += 4;

	int iNumIndices = 0;
	memcpy( &iNumIndices, pData + uiPos, 4 );
	uiPos += 4;

	pMesh->uiMaterialId = iMaterialIndex;
	pMesh->uiNumIndices = iNumIndices;
	pMesh->pIndexData = new unsigned short[iNumIndices];

	memcpy( pMesh->pIndexData, pData + uiPos, 2 * iNumIndices );
	uiPos += 2 * iNumIndices;

	int iNumVertices = 0;
	memcpy( &iNumVertices, pData + uiPos, 4 );
	uiPos += 4;
	pMesh->uiNumVertices = iNumVertices;
	pMesh->pVertexData = new CXXModel::VERTEX_DATA[iNumVertices];

	for( int i = 0; i < iNumVertices; ++i )
	{
		Vertex_1 *pVertert1 = NULL;
		Vertex_2 *pVertert2 = NULL;

		if( iFrameType < 4 )
		{
			pVertert1 = (Vertex_1 *)(pData + uiPos);
			uiPos += sizeof( Vertex_1 );
		}
		else
		{
			pVertert2 = (Vertex_2 *)(pData + uiPos);
			uiPos += sizeof( Vertex_2 );
		}
		if( pVertert1 )
		{
			pMesh->pVertexData[i].x = pVertert1->coord[0];
			pMesh->pVertexData[i].y = pVertert1->coord[1];
			pMesh->pVertexData[i].z = pVertert1->coord[2];
			pMesh->pVertexData[i].normal[0] = pVertert1->normal[0];
			pMesh->pVertexData[i].normal[1] = pVertert1->normal[1];
			pMesh->pVertexData[i].normal[2] = pVertert1->normal[2];
			pMesh->pVertexData[i].u = pVertert1->texcoord[0];
			pMesh->pVertexData[i].v = pVertert1->texcoord[1];
		}

		if( pVertert2 )
		{
			pMesh->pVertexData[i].x = pVertert2->coord[0];
			pMesh->pVertexData[i].y = pVertert2->coord[1];
			pMesh->pVertexData[i].z = pVertert2->coord[2];
			pMesh->pVertexData[i].normal[0] = pVertert2->normal[0];
			pMesh->pVertexData[i].normal[1] = pVertert2->normal[1];
			pMesh->pVertexData[i].normal[2] = pVertert2->normal[2];
			pMesh->pVertexData[i].u = pVertert2->texcoord[0];
			pMesh->pVertexData[i].v = pVertert2->texcoord[1];
		}
	}

	if( iMeshType != 0 )
	{
		uiPos += iNumVertices * 8;
	}

	if( iMeshType == 2 )
	{
		uiPos += iNumVertices * 8;
	}

	if( cMatFlags == 0x0e )
	{
		if( cSpecial == 0x40 )
			uiPos += iNumVertices * 16;
	}

	if( cMatFlags == 0x0e || cMatFlags == 0x02 )
	{
		if( cSpecial == 0x62 )
			uiPos += iNumVertices * 16;
	}

	if( iFrameType >= 2 )
	{
		// Unknown (frame type >= 2)
		// 100 bytes
		uiPos += 100;
	}

	if( iFrameType >= 3 )
	{
		// Unknwon - MIKDemo, RealGirlfriend
		// 64 bytes
		uiPos += 64;
	}

	if( iFrameType >= 5 )
	{
		// Unknown - Real Girlfriend
		// 20 bytes
		uiPos += 20;
	}
}

void CXXFile::ParseBone( unsigned char* pData, size_t uiDataSize, size_t& uiPos, int iFrameId )
{
	static XMMATRIX matIdentity = XMMatrixIdentity();

	std::string strBoneName;
	ParseName( pData, uiDataSize, uiPos, strBoneName );

	int iBoneIndex = 0;
	memcpy( &iBoneIndex, pData + uiPos, 4 );
	uiPos += 4;

	XMMATRIX mat = matIdentity;
	memcpy( &mat, pData + uiPos, 4 * 16 );
	uiPos += 64;

	CXXModel::BONE bone;
	bone.iFrameId = iFrameId;
	memcpy( bone.matCurrent, &mat, sizeof(XMMATRIX) );
	memcpy( bone.matrix, &mat, sizeof(XMMATRIX) );

	bone.strBoneName = strBoneName;
	m_pModel->m_vecBones.push_back( bone );
}

void CXXFile::ParseFrame( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
	int iFrameType, int iParentId )
{	
	static XMMATRIX matIdentity = XMMatrixIdentity();

	std::string strFrameName;
	ParseName( pData, uiDataSize, uiPos, strFrameName );
	
	CXXModel::FRAME frame;
	frame.iFrameId = m_pModel->m_vecFrames.size();
	frame.iParentId = iParentId;
	frame.strFrameName = strFrameName;
	frame.iAnimBoneIndex = -1;

	int iNumChildren = 0;
	memcpy( &iNumChildren, pData + uiPos, 4 );
	uiPos += 4;

	XMMATRIX mat = matIdentity;
	memcpy( &mat, pData + uiPos, 4 * 16 );
	uiPos += 64;
	
	memcpy( frame.matCurrent, &mat, sizeof(XMMATRIX) );
	memcpy( frame.matrix, &mat, sizeof(XMMATRIX) );
	m_pModel->m_vecFrames.push_back( frame );

	//Unknown Data A
	uiPos += 16;

	int iNumMeshes = 0;
	memcpy( &iNumMeshes, pData + uiPos, 4 );
	uiPos += 4;

	float fMin[3], fMax[3];
	memcpy( fMin, pData + uiPos, 4 * 3 );
	uiPos += 12;
	memcpy( fMax, pData + uiPos, 4 * 3 );
	uiPos += 12;

	//Unknown Data B
	uiPos += 16;

	if( iNumMeshes > 0 )
	{
		char cMeshType = 0;
		memcpy( &cMeshType, pData + uiPos, 1 );
		uiPos += 1;

		CXXModel::MESHINFO* pMeshInfo = new CXXModel::MESHINFO();		
		pMeshInfo->bVisible = TRUE;
		pMeshInfo->strFrameName = strFrameName;
		pMeshInfo->uiFrameIndex = frame.iFrameId;

		for( int i = 0; i < iNumMeshes; ++i )
		{
			CXXModel::MESH* pMesh = new CXXModel::MESH();
			pMesh->bVisible = TRUE;			
			pMeshInfo->vecMeshes.push_back( pMesh );
			ParseMesh( pData, uiDataSize, uiPos, iFrameType, cMeshType, pMesh );
		}
		m_pModel->m_vecMeshes.push_back( pMeshInfo );

		short iNumPrivateVerts = 0;
		memcpy( &iNumPrivateVerts, pData + uiPos, 2 );
		uiPos += 2;

		//Unknown Data C
		uiPos += 8;

		for( int j = 0; j < iNumPrivateVerts; ++j )
			ParseVertex( pData, uiDataSize, uiPos, iFrameType );

		int iNumBones = 0;
		memcpy( &iNumBones, pData + uiPos, 4 );
		uiPos += 4;

		for( int j = 0; j < iNumBones; ++j )
			ParseBone( pData, uiDataSize, uiPos, frame.iFrameId );
	}
	
	for( int i = 0; i < iNumChildren; ++i )
	{
		ParseFrame( pData, uiDataSize, uiPos, iFrameType, frame.iFrameId );
	}
}

void CXXFile::ParseMaterial( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
	int iFrameType )
{
	int iNumMaterials = 0;
	memcpy( &iNumMaterials, pData + uiPos, 4 );
	uiPos += 4;

	m_pModel->m_vecMaterials.resize( iNumMaterials );
	for( int i = 0; i < iNumMaterials; ++i )
	{
		std::string strMaterialName;
		ParseName( pData, uiDataSize, uiPos, strMaterialName );

		float			   Diffuse[4];
		float			   Ambient[4];
		float			   Specular[4];
		float			   Emissive[4];
		float			   Shininess;
		memcpy( Diffuse, pData + uiPos, 4 * 4 );
		uiPos += 16;
		memcpy( Ambient, pData + uiPos, 4 * 4 );
		uiPos += 16;
		memcpy( Specular, pData + uiPos, 4 * 4 );
		uiPos += 16;
		memcpy( Emissive, pData + uiPos, 4 * 4 );
		uiPos += 16;
		memcpy( &Shininess, pData + uiPos, 4 );
		uiPos += 4;

		for( int j = 0; j < 4; ++j )
		{
			m_pModel->m_vecMaterials[i].Diffuse[j]  = -Diffuse[j];
			m_pModel->m_vecMaterials[i].Ambient[j]  = -Ambient[j];
			m_pModel->m_vecMaterials[i].Specular[j] = -Specular[j];
			m_pModel->m_vecMaterials[i].Emissive[j] = -Emissive[j];			
		}
		m_pModel->m_vecMaterials[i].Shininess = -Shininess;
		m_pModel->m_vecMaterials[i].uiNumTextures = 0;
		m_pModel->m_vecMaterials[i].bIsTransparent = FALSE;
		m_pModel->m_vecMaterials[i].Transparency = 1.0f;
		m_pModel->m_vecMaterials[i].bDoubleSided = FALSE;

		for( int j = 0; j < 4; ++j )
		{
			std::string strTextureName;
			ParseName( pData, uiDataSize, uiPos, strTextureName );
			if( !strTextureName.empty() )
			{
				++m_pModel->m_vecMaterials[i].uiNumTextures;
				m_pModel->m_vecMaterials[i].vecTextureNames.push_back( strTextureName );
			}

			//Unknown Data A
			uiPos += 16;
		}

		//Unknown Data B
		uiPos += 88;
	}
}

void CXXFile::ParseTexture( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
	int iFrameType )
{
	int iNumTexture = 0;
	memcpy( &iNumTexture, pData + uiPos, 4 );
	uiPos += 4;

	if( iNumTexture > 0 )
	{
		CXXModel::TEXTURE texture;

		for( int i = 0; i < iNumTexture; ++i )
		{
			std::string strTextureName;
			ParseName( pData, uiDataSize, uiPos, strTextureName  );

			//Unknown Data A
			uiPos += 4;

			int iImageWidth = 0;
			int iImageHeight = 0;

			memcpy( &iImageWidth, pData + uiPos, 4 );
			uiPos += 4;
			memcpy( &iImageHeight, pData + uiPos, 4 );
			uiPos += 4;

			// Unknown
			// BMP: 1 1 20|41 3 0
			// TGA: 1 1 21 3 2
			int iFormat[5];
			memcpy( iFormat, pData + uiPos, 4 * 5 );
			uiPos += 20;

			// Checksum
			// u8 checksum = *(cdata + cpos);
			uiPos += 1;

			int iImageDataSize = 0;
			memcpy( &iImageDataSize, pData + uiPos, 4 );
			uiPos += 4;
			
			texture.uiTextureWidth = iImageWidth;
			texture.uiTextureHeight = iImageHeight;
			texture.uiTextureSize = iImageDataSize;
			texture.pTextureData = new unsigned char[iImageDataSize];

			if( iFormat[4] == 0 )
			{
				texture.uiTextureFormat = 0x00000001;
				memcpy( texture.pTextureData, "BM", 2 );
				memcpy( texture.pTextureData + 2, pData + uiPos + 2, iImageDataSize - 2 );
			}
			else
			{
				texture.uiTextureFormat = 0x00000002;
				memcpy( texture.pTextureData, pData + uiPos, iImageDataSize );
			}
			m_pModel->m_mapTexture.insert(
				std::make_pair(strTextureName, texture) );
			
			uiPos += iImageDataSize;
		}
	}
}