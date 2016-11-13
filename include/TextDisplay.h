//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "FontManager.h"
#include "Camera.h"
#include <d3d11.h>

class TextDisplay
{
public:
	TextDisplay( ID3D11Device* lpDevice,
		ID3D11DeviceContext* lpDeviceContext );
	~TextDisplay(void);
public:
	FontManager*				m_pFontManager;

	BOOL						m_bInit;

public:
	ID3D11Device*				m_lpDevice;

	ID3D11DeviceContext*		m_lpDeviceContext;

	ID3D11Buffer*				m_lpVertexBuffer;

	ID3D11Buffer*				m_lpIndexBuffer;

	ID3D11ShaderResourceView*	m_lpResourceView;

	ID3D11InputLayout*			m_lpInputLayout;

	ID3D11VertexShader*			m_lpVertexShader;

	ID3D11PixelShader*			m_lpPixelShader;

	ID3D11GeometryShader*		m_lpGeometryShader;

	DWORD						m_dwShaderBytecodeSize;

	VOID*						m_lpShaderBytecodeInput;

	ID3D11Buffer*				m_lpVSConstBufferWorldViewProj;
		
	ID3D11BlendState*			m_lpBlendState;

public:
	void Display();

	void SetText( int iXPos, int iYPos, int iFontSize, 
		LPCWSTR lpText, unsigned int uiColor );

	VOID SetupInput( int iXPos, int iYPos, float Left, float Top,
		float Right, float Bottom );

	BOOL SetupShaders();

	void CreateBlendState();

	static size_t RawToBitmap( void* pImageData, size_t uiImageDataSize, 
		void* pBitmap, size_t uiBitmapSize,
		size_t uiWidth, size_t uiHeight, int iBytesPerPixel );

	void SetFontFile( const char* pFontFile );

public:
	// 自定义顶点格式	
	struct CUSTOMVERTEX
	{
		FLOAT x, y, z;	   // 世界坐标
		FLOAT u, v;        // 纹理坐标
	};

	struct ConstantBuffer
	{
		float matWorldViewProj[16];
		float matWorldView[16];
	};

	static ConstantBuffer s_Buffer;
};

