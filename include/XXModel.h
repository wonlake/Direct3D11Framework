//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "BoundingBox.h"
#include "Camera.h"

#include <vector>
#include <map>
#include <string>

class CXXModel
{
public:
	struct VERTEX_DATA
	{
		FLOAT x, y, z;	      // 世界坐标
		FLOAT normal[3];	  // 法线
		FLOAT u, v, s;        // 纹理坐标
		FLOAT weight[4];
	};

	struct MESH
	{
		BOOL bVisible;
		unsigned int uiMaterialId;
		unsigned short* pIndexData;
		VERTEX_DATA* pVertexData;	
		ID3D11Buffer* pVertexBuffer;
		ID3D11Buffer* pIndexBuffer;
		unsigned int uiNumIndices;
		unsigned int uiNumVertices;
	};

	struct MATERIAL
	{
		BOOL			   bDoubleSided;
		BOOL			   bIsTransparent;
		FLOAT			   Transparency;
		FLOAT			   Diffuse[4];
		FLOAT			   Ambient[4];
		FLOAT			   Specular[4];
		FLOAT			   Emissive[4];
		FLOAT			   Shininess;
		std::vector<std::string> vecTextureNames;
		unsigned int	   uiNumTextures;	
		ID3D11ShaderResourceView** ppTextures;
	};

	struct TEXTURE
	{
		unsigned int uiTextureFormat;
		unsigned int uiTextureSize;
		unsigned int uiTextureWidth;
		unsigned int uiTextureHeight;
		unsigned char* pTextureData;
		ID3D11ShaderResourceView* pTexture;	
	};

	struct ConstantBuffer
	{
		float matWorldViewProj[16];
		float matWorldView[16];
		float matWorldViewForNormal[16];
	};

	struct PSConstantBuffer
	{
		float color[4];
		float iNumMeshes;
		float tranparency[3];
				
	};

	struct MESHINFO
	{
		BOOL bVisible;
		unsigned int uiFrameIndex;
		std::string strFrameName;
		std::vector<CXXModel::MESH*> vecMeshes;
	};

	struct FRAME
	{
		int iParentId;
		int iFrameId;
		int iAnimBoneIndex;
		float matrix[16];
		float matCurrent[16];
		std::string strFrameName;
	};

	struct SEQUENCE
	{		
		float		 fSpeed;
		unsigned int uiStartTime;
		unsigned int uiEndTime;
		std::string  strSequenceName;
	};

	struct KEYFRAME
	{
		float quaternion[4];
		float translate[4];
	};
	
	struct BONE
	{
		int iFrameId;
		float matrix[16];
		float matCurrent[16];
		std::string strBoneName;
	};

	struct BONEANIMINFO
	{		
		float matRot[16];
		float matTrans[16];
		std::string strBoneName;
		std::vector<KEYFRAME> vecKeyframes;
	};

	struct ANIMTIME
	{
		float fTime;
		unsigned int uiStartTime;
		unsigned int uiEndTime;
	};

public:
	class MeshFrameNameIterator
	{
	public:
		MeshFrameNameIterator(CXXModel* pModel )
		{
			iter = pModel->m_vecMeshes.begin();
			iter_end = pModel->m_vecMeshes.end();
		}

	public:
		std::vector<MESHINFO*>::iterator iter;
		std::vector<MESHINFO*>::iterator iter_end;

	public:
		const char* GetNext()
		{
			if( iter == iter_end )
			{
				return NULL;
			}
			const char* pName = (*iter)->strFrameName.c_str();
			++iter;
			return pName;
		}
	};

public:
	CXXModel( ID3D11Device* lpDevice, ID3D11DeviceContext* lpDeviceContext );
	~CXXModel(void);
	 
public:
	bool LoadModelFromMemory( unsigned char* pData, size_t uiDataSize );

	bool LoadModelFromFile( char* pFileName );
	
	bool LoadAnimationFromMemory( unsigned char* pData, size_t uiDataSize );

	bool LoadAnimationFromFile( char* pFileName );

	void SetTransform( XMMATRIX* pMatTransform, XMMATRIX* pMatTranlate );
	
	void GetTransform( XMMATRIX* pMatTransform, XMMATRIX* pMatTranlate );

	void Render( XMMATRIX* pWorldViewMatrix, XMMATRIX* pProjMatrix );

	void RenderObject( MESH* pMesh, int iMatId,
		XMMATRIX* pWorldViewProjMatrix, XMMATRIX* pWorldView );
	
	VOID ComputeBoundingBox();

	void RenderBoundingBox( XMMATRIX* pWorldViewProjMatrix );

	BOOL SetupInput();

	BOOL SetupInputForNoTexture();

	BOOL SetupShaders();

	BOOL SetupShadersForNoTexture();

	VOID DestroyPrivateResource();

	VOID DestroyAnimationData();

	VOID ShowBoundingBox( BOOL bShow );

	float GetTranslateUnit();

	BOOL CreateRenderState();
	
	VOID BuildMesh();

	VOID BuildAnimation();

	MeshFrameNameIterator GetMeshFrameNameIterator()
	{
		return MeshFrameNameIterator( this );
	}

	VOID SetFrameVisible( unsigned int index, BOOL bVisible );

	BOOL GetFrameVisible( unsigned int index );

	BOOL GetTexture( unsigned char* pData, unsigned int uiDataSize, ID3D11ShaderResourceView** ppSRV );

	VOID BuildFrameMatrix();

	VOID UpdateBoneMatrix( float fTimeDiff );

	VOID BuildBoneMatrix();

	const CXXModel::SEQUENCE* GetSequenceByID( int iIndex );

	int GetNumSequences();

	void SetSequenceByIndex( int iIndex );

public:
	friend class CXXFile;
	CXXFile*							m_pXXFile;
	friend class CXAFile;
	CXAFile*							m_pXAFile;

	std::vector<MESHINFO*>				m_vecMeshes;
	std::vector<MATERIAL>				m_vecMaterials;
	std::map<std::string, TEXTURE>		m_mapTexture;
	std::vector<FRAME>					m_vecFrames;
	std::vector<BONE>					m_vecBones;

	std::vector<BONEANIMINFO>			m_vecBoneAnims;
	std::vector<SEQUENCE>				m_vecSequences;

	int									m_iTotalFrames;
	ANIMTIME							m_AnimTime;

public:
	ID3D11Device*				m_lpDevice;

	ID3D11DeviceContext*		m_lpDeviceContext;

	ID3D11Texture2D*			m_lpTexture;	

	ID3D11Buffer*				m_lpVertexBuffer;

	ID3D11Buffer*				m_lpIndexBuffer;

	ID3D11ShaderResourceView*	m_lpResourceView;

	ID3D11ShaderResourceView*	m_lpResourceViewExchange;

public:
	static ID3D11InputLayout*			m_lpInputLayout;

	static ID3D11VertexShader*			m_lpVertexShader;

	static ID3D11PixelShader*			m_lpPixelShader;

	static ID3D11GeometryShader*		m_lpGeometryShader;

	static ID3D11Buffer*				m_lpVSConstBufferWorldViewProj;

	static DWORD						m_dwShaderBytecodeSize;

	static VOID*						m_lpShaderBytecodeInput;

	static DWORD						m_Reference;

	static ID3D11InputLayout*			m_lpInputLayoutForNoTexture;

	static ID3D11VertexShader*			m_lpVertexShaderForNoTexture;

	static ID3D11PixelShader*			m_lpPixelShaderForNoTexture;

	static ID3D11GeometryShader*		m_lpGeometryShaderForNoTexture;

	static ID3D11Buffer*				m_lpPSConstBufferDiffuse;

	static DWORD						m_dwShaderBytecodeSizeForNoTexture;

	static VOID*						m_lpShaderBytecodeInputForNoTexture;

	static ID3D11RasterizerState*		m_lpRasterizerState;

	static ID3D11RasterizerState*		m_lpRasterizerStateForNoCull;

	static ID3D11BlendState*			m_lpBlendState;

public:

	FLOAT			m_fScale;
	FLOAT			m_Min[3];
	FLOAT			m_Max[3];
	FLOAT			m_fCenter[3];
	BOOL			m_bShowBoundingBox;
	float			m_fTranslateUnit;

	BoundingBox* m_pBoundingBox;
	float		 m_matTransform[16];
	float		 m_matTranslate[16];

public:

	static ConstantBuffer s_Buffer;

	static PSConstantBuffer s_PSBuffer;
};

