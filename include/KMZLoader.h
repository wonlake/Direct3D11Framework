//
// Copyright (C) Mei Jun 2011
//

#pragma once

/*
#define NO_BOOST
#include <dae.h>
#include <dom.h>
#include <dom/domCollada.h>
#include <dom/domImage.h>
#include <dom/domProfile_COMMON.h>

#include "BoundingBox.h"
#include "Camera.h"

class KMZLoader
{
protected:
	struct VERTEX_DATA
	{
		FLOAT x, y, z;	      // 世界坐标
		FLOAT normal[3];	  // 法线
		FLOAT u, v, s;        // 纹理坐标
	};

	struct MESH
	{
		BOOL		  bHasNormal;
		BOOL		  bHasTexcoord;
		DWORD		  nNumVertices;
		DWORD		  nMaterialID;
		VERTEX_DATA*  pVertices;
		ID3D11Buffer* pVertexBuffer;
		std::wstring  semantic;
		std::string	  materialName;
		std::string	  name;
	};

	struct GEOMETRY
	{
		DWORD nNumMeshes;
		MESH* pMeshes;
		DWORD index;
		D3D11_PRIMITIVE_TOPOLOGY type;
	};

	struct scene_node_t
	{
		std::vector<GEOMETRY*>		_geos;
		std::vector<scene_node_t>	_children;
		scene_node_t*				_pparent;
		float						_transform[16];
		BOOL						_bAdded;
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
		FLOAT			   Power;
		std::string		   strMaterialName;
		ID3D11ShaderResourceView* pTexture;	
	};

	struct DATA_SIZE
	{
		char* pData;
		DWORD dwSize;
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
		float specular[4];
		float tranparency;
		float power[3];
	};

public:
	KMZLoader( ID3D11Device* lpDevice, ID3D11DeviceContext* lpDeviceContext );
	~KMZLoader(void);

	void ReadImages( domCOLLADA* pDom, 
		std::map<std::string, DATA_SIZE>& filelist );

	void ReadEffects( domCOLLADA* pDom );

	void ReadMaterials( domCOLLADA* pDom );

	void ReadGeometries( domCOLLADA* pDom );

	void ReadNodes( domCOLLADA* pDom );

	void ReadScene( domCOLLADA* pDom );

	void LoadColladaMeshFromKMZ( LPCTSTR lpFilename );

	void LoadColladaMeshFromMemory( std::map<std::string, DATA_SIZE>& filelist, 
		LPCSTR lpDaeName );

	void TraverseAndSetupTransform( domNodeRef pNode,
		scene_node_t* pSceneNode );

	void TraverseSceneGraphForOpaque( ID3D11Device* lpDevice,
		KMZLoader* pDaeLoader, XMMATRIX* pWorldViewProjMatrix, XMMATRIX* pWorldView );
	void TraverseSceneGraphForTransparent( ID3D11Device* lpDevice,
		KMZLoader* pDaeLoader, XMMATRIX* pWorldViewProjMatrix, XMMATRIX* pWorldView );

	void SetTransform( XMMATRIX* pMatrix );
	void Render( XMMATRIX* pWorldViewMatrix, XMMATRIX* pProjMatrix );
	VOID ComputeBoundingBox();
	void RenderBoundingBox( XMMATRIX* pWorldViewProjMatrix );
	BOOL SetupInput();
	BOOL SetupInputForNoTexture();
	BOOL SetupShaders();
	BOOL SetupShadersForNoTexture();
	VOID DestroyPrivateResource();
	VOID ShowBoundingBox( BOOL bShow );
	float GetTranslateUnit();
	BOOL CreateRenderState();

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

public:
	std::vector<GEOMETRY>								m_vecGeometries;
	std::vector<std::string>							m_vecGeoNames;
	std::map<std::string, ID3D11ShaderResourceView*>	m_mapTextures;
	std::vector<MATERIAL>								m_vecMaterials;
	std::list<scene_node_t*>							m_listRenderableNodes;
	std::list<KMZLoader*>								m_listDaeLoaders;

	std::map<std::string, GEOMETRY*>					m_mapGeomtries;
	std::map<std::string, int>							m_mapMaterials;

	std::map<std::string, std::pair<daeElement*, std::list<domExtraRef> > > m_mapEffects;
	std::map<std::string, ID3D11ShaderResourceView*>	m_mapEffectTextures;
	std::map<std::string, domNodeRef>					m_mapNodes;

public:

	FLOAT			m_fScale;
	scene_node_t*	m_pRoot;
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
*/
