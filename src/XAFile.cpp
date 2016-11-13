//
// Copyright (C) Mei Jun 2011
//

#include "XAFile.h"

CXAFile::CXAFile(void)
{
	m_pModel = NULL;
}


CXAFile::~CXAFile(void)
{
	m_pModel = NULL;
}

bool CXAFile::LoadModelFromMemory( unsigned char* pData, size_t uiDataSize, CXXModel* pModel )
{
	m_pModel = pModel;

	size_t uiDataPos = 0;
	unsigned char cFormatType = 0;
	memcpy( &cFormatType, pData + uiDataPos, 1 );

	if( cFormatType == 0x03 ) //SB3
	{
		uiDataPos += 4;

		uiDataPos += 1;
	}
	else
		return false;

	// Material
	unsigned char cMat1 = 0;	
	memcpy( &cMat1, pData + uiDataPos, 1 );
	uiDataPos += 1;	
	if( cMat1 == 0x01 )
	{
		return false;
	}
	// Section 2
	unsigned char cMat2 = 0;
	memcpy( &cMat2, pData + uiDataPos, 1 );
	uiDataPos += 1;
	if( cMat2 == 0x01 )
	{
		return false;
	}
	// Morph targets
	unsigned char cMat3 = 0;
	memcpy( &cMat3, pData + uiDataPos, 1 );
	uiDataPos += 1;
	if( cMat3 == 0x01 )
	{
		return false;
	}

	if( cFormatType == 0x03 )
	{
		unsigned char cMat4 = 0;
		memcpy( &cMat4, pData + uiDataPos, 1 );
		uiDataPos += 1;
		if( cMat4 == 0x01 )
		{
			return false;
		}
	}

	unsigned char cMat5 = 0;
	memcpy( &cMat5, pData + uiDataPos, 1 );
	uiDataPos += 1;
	if( cMat5 == 0x01 )
	{
		ParseSequence( pData, uiDataSize, uiDataPos, cFormatType );		
		ParseAnimation( pData, uiDataSize, uiDataPos, cFormatType );
	}

	return true;
}

bool CXAFile::LoadModelFromFile( char* pFileName, CXXModel* pModel )
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

void CXAFile::ParseName( unsigned char* pData, size_t uiDataSize,
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

void CXAFile::ParseXAName( unsigned char* pData, size_t uiDataSize,
	size_t& uiPos, std::string& strName )
{
	char name[64] = { 0 };
	int iNameLen = 64;

	memcpy( name, pData + uiPos, iNameLen - 1 );
	for( int i = 0; i < iNameLen; ++i )
		name[i] = ~name[i];
	strName = name;

	uiPos += iNameLen;		
}

void CXAFile::ParseSequence( unsigned char* pData, size_t uiDataSize,
	size_t& uiDataPos, int iFormatType )
{
	int iLoop = 512;
	if( iFormatType == 0x03 )
	{
		iLoop = 1024;
	}

	for( int i = 0; i < iLoop; ++i )
	{
		std::string strSeqName;
		ParseXAName( pData, uiDataSize, uiDataPos, strSeqName );

		float fSpeed = 0.0f;
		memcpy( &fSpeed, pData + uiDataPos, 4 );
		uiDataPos += 4;

		//Unknown Data
		uiDataPos += 4;

		float fStartTime = 0.0f;
		memcpy( &fStartTime, pData + uiDataPos, 4 );
		uiDataPos += 4;

		float fEndTime = 0.0f;
		memcpy( &fEndTime, pData + uiDataPos, 4 );
		uiDataPos += 4;

		//Unknown Data
		uiDataPos += 1;
		//Unknown Data
		uiDataPos += 1;
		//Unknown Data
		uiDataPos += 1;

		//Next Seq Index;
		int iNextSeq = 0;
		memcpy( &iNextSeq, pData + uiDataPos, 4 );
		uiDataPos += 4;

		//Unknown Data
		uiDataPos += 1;
		//Unknown Data
		uiDataPos += 4;
		//Unknown Data
		uiDataPos += 16;

		if( fStartTime < fEndTime )
		{
			CXXModel::SEQUENCE seq;
			seq.strSequenceName = strSeqName;
			seq.fSpeed = fSpeed;
			seq.uiStartTime = fStartTime;
			seq.uiEndTime = fEndTime;
			m_pModel->m_vecSequences.push_back( seq );
			//int len = MultiByteToWideChar( CP_ACP, 0, strSeqName.c_str(), -1, NULL, 0 );
			//WCHAR* pwName = new WCHAR[len];
			//MultiByteToWideChar( CP_ACP, 0, strSeqName.c_str(), -1, pwName, len );
			//MessageBox( NULL, pwName, NULL, MB_OK );
			//delete[] pwName;
			//setlocale( 0, "japanese" );
			//printf( "%s\n", strSeqName.c_str() );
			//setlocale( 0, "" );

		}
	}
}

void CXAFile::ParseAnimation( unsigned char* pData, size_t uiDataSize,
	size_t& uiDataPos, int iFormatType )
{
	static XMVECTOR vZero = XMVectorZero();
	static XMMATRIX matIdentity = XMMatrixIdentity();

	int iNumAnims = 0;
	memcpy( &iNumAnims, pData + uiDataPos, 4 );
	uiDataPos += 4;

	m_pModel->m_vecBoneAnims.resize( iNumAnims );
	for( int i = 0; i < iNumAnims; ++i )
	{
		std::string strBoneName;
		ParseName( pData, uiDataSize, uiDataPos, strBoneName );

		m_pModel->m_vecBoneAnims[i].strBoneName = strBoneName;
		memcpy( m_pModel->m_vecBoneAnims[i].matRot, &matIdentity, sizeof(XMMATRIX) );
		memcpy( m_pModel->m_vecBoneAnims[i].matTrans, &matIdentity, sizeof(XMMATRIX) );

		int iNumFrames = 0;
		memcpy( &iNumFrames, pData + uiDataPos, 4 );
		uiDataPos += 4;

		//Unknown Data
		uiDataPos += 4;

		// animation [4] keyframe data
		// 0 QUATERNION(YZXW) 0 0 POSITION(YZX) 1 1
		m_pModel->m_vecBoneAnims[i].vecKeyframes.resize( iNumFrames );
		if( m_pModel->m_iTotalFrames < iNumFrames )
			m_pModel->m_iTotalFrames = iNumFrames;
		for( int j = 0; j < iNumFrames; ++j )
		{
			XMVECTOR quat = vZero;
			XMVECTOR pos  = vZero;
			memcpy( &quat, pData + uiDataPos + 4, 4 * 4 );			
			memcpy( &pos, pData + uiDataPos + 4 * 7, 4 * 3 );
			memcpy( m_pModel->m_vecBoneAnims[i].vecKeyframes[j].quaternion, &quat, sizeof(XMVECTOR) );
			memcpy( m_pModel->m_vecBoneAnims[i].vecKeyframes[j].translate, &quat, sizeof(XMVECTOR) );

			//fprintf( pOut, "\tÐý×ª:%f %f %f %f\n", quat.x, quat.y, quat.z, quat.w );

			uiDataPos += 52;
		}
	}
}