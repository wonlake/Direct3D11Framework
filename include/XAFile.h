//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "XXModel.h"

class CXAFile
{
public:
	CXAFile(void);
	~CXAFile(void);

public:
	bool LoadModelFromMemory(
		unsigned char* pData, size_t uiDataSize, CXXModel* pModel );
	bool LoadModelFromFile( char* pFileName, CXXModel* pModel );

protected:
	void ParseName( unsigned char* pData, size_t uiDataSize,
		size_t& uiPos, std::string& strName );

	void ParseXAName( unsigned char* pData, size_t uiDataSize,
		size_t& uiPos, std::string& strName );

	void ParseSequence( unsigned char* pData, size_t uiDataSize,
		size_t& uiPos, int iFormatType );

	void ParseAnimation( unsigned char* pData, size_t uiDataSize,
		size_t& uiPos, int iFormatType );

private:
	CXXModel* m_pModel;
};

