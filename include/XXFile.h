//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "XXModel.h"

class CXXFile
{
public:
	CXXFile();
	~CXXFile(void);

	bool LoadModelFromMemory(
		unsigned char* pData, size_t uiDataSize, CXXModel* pModel );
	bool LoadModelFromFile( char* pFileName, CXXModel* pModel );

private:

	void ParseName( unsigned char* pData, size_t uiDataSize, size_t& uiPos, std::string& strName );

	void ParseVertex( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
		int iFrameType );

	void ParseMesh( unsigned char* pData, size_t uiDataSize, size_t& uiPos, 
		int iFrameType, int iMeshType, CXXModel::MESH *pMesh );

	void ParseBone( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
		int iFrameId );

	void ParseFrame( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
		int iFrameType, int iParentId );

	void ParseMaterial( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
		int iFrameType );

	void ParseTexture( unsigned char* pData, size_t uiDataSize, size_t& uiPos,
		int iFrameType );

private:
	CXXModel*	m_pModel;

public:
	struct Vertex_1 {
		unsigned int	index;
		float			coord[3];
		float			weights[3];
		unsigned char	bones[4];
		float			normal[3];
		float			texcoord[2];
	};

	struct Vertex_2 {
		unsigned short	index;
		float			coord[3];
		float			weights[3];
		unsigned char	bones[4];
		float			normal[3];
		float			texcoord[2];
		unsigned int	unknown[5];
	};

};

