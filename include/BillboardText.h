//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "Direct3D.h"
#include "TextDisplay.h"

class CBillboardText
{
public:
	CBillboardText( ID3D11Device* lpDevice, ID3D11DeviceContext* lpDeviceContext );
	~CBillboardText(void);
	
	void Render();	
	//
	//更新文本
	//
	VOID UpdateText( LPCTSTR lpText );
	//
	//设置字体属性
	//
	VOID SetFontProperty( LPCSTR lpFontName, DWORD dwSize,
		COLORREF TextColor, COLORREF BackColor );
	//
	//设置显示位置
	//
	VOID SetDisplayPos( XMVECTOR vPos );
	//
	//获取FPS
	//
	DWORD GetFPS();

private:
	//
	//创建文本纹理
	//
	ID3D11ShaderResourceView* CreateTextTexture( LPCTSTR lpText, 
		LPCTSTR lpFontName, DWORD dwSize, COLORREF TextColor, COLORREF BackColor );

protected:
	//
	//字体大小
	//
	DWORD						m_dwSize;
	//
	//文本颜色
	//
	COLORREF					m_TextColor;
	//
	//背景颜色
	//
	COLORREF					m_BackColor;

	ID3D11Device*				m_lpDevice;

	ID3D11DeviceContext*		m_lpDeviceContext;
	
	DWORD						m_dwFPS;

	FLOAT						m_vPos[4];

	TextDisplay*				m_pTextDisplay;

	std::map<std::string, std::string> m_mapFontNames;
};