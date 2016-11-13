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
	//�����ı�
	//
	VOID UpdateText( LPCTSTR lpText );
	//
	//������������
	//
	VOID SetFontProperty( LPCSTR lpFontName, DWORD dwSize,
		COLORREF TextColor, COLORREF BackColor );
	//
	//������ʾλ��
	//
	VOID SetDisplayPos( XMVECTOR vPos );
	//
	//��ȡFPS
	//
	DWORD GetFPS();

private:
	//
	//�����ı�����
	//
	ID3D11ShaderResourceView* CreateTextTexture( LPCTSTR lpText, 
		LPCTSTR lpFontName, DWORD dwSize, COLORREF TextColor, COLORREF BackColor );

protected:
	//
	//�����С
	//
	DWORD						m_dwSize;
	//
	//�ı���ɫ
	//
	COLORREF					m_TextColor;
	//
	//������ɫ
	//
	COLORREF					m_BackColor;

	ID3D11Device*				m_lpDevice;

	ID3D11DeviceContext*		m_lpDeviceContext;
	
	DWORD						m_dwFPS;

	FLOAT						m_vPos[4];

	TextDisplay*				m_pTextDisplay;

	std::map<std::string, std::string> m_mapFontNames;
};