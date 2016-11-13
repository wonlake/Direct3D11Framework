//
// Copyright (C) Mei Jun 2011
//

#include "BillboardText.h"

#include <tchar.h>

#pragma comment( lib, "winmm.lib" )

CBillboardText::CBillboardText( 
	ID3D11Device* lpDevice, ID3D11DeviceContext* lpDeviceContext )
{
	m_dwSize	= 20;
	m_TextColor = RGB(255, 0, 0);
	m_BackColor = RGB(255, 255, 255);

	m_lpDevice	= lpDevice;
	m_lpDeviceContext = lpDeviceContext;

	memset( m_vPos, 0, sizeof(m_vPos) );
	m_dwFPS = 0;
	m_pTextDisplay = NULL;

	m_mapFontNames["����"]		= "C:\\Windows\\Fonts\\simsun.ttc";
	m_mapFontNames["����"]		= "C:\\Windows\\Fonts\\simkai.ttf";
	m_mapFontNames["����"]		= "C:\\Windows\\Fonts\\simfang.ttf";	
	m_mapFontNames["����"]		= "C:\\Windows\\Fonts\\simhei.ttf";
	m_mapFontNames["�·���"]		= "C:\\Windows\\Fonts\\simsun.ttc";
	m_mapFontNames["΢���ź�"]	= "C:\\Windows\\Fonts\\msyh.ttf";
}

CBillboardText::~CBillboardText(void)
{	
	SAFE_DELETE( m_pTextDisplay );
}

ID3D11ShaderResourceView* CBillboardText::CreateTextTexture( LPCTSTR lpText, 
	LPCTSTR lpFontName, DWORD dwSize, COLORREF TextColor, COLORREF BackColor )
{
	size_t text_length = _tcslen( lpText );
	if( text_length < 1 )
		return NULL;

	DWORD dwImageWidth = text_length * dwSize;
	DWORD dwImageHeight = dwSize;
	int i = 0;
	while( dwImageWidth >> ++i > 0 );
	dwImageWidth = pow( 2.0f, i );
	i = 0;
	while( dwImageHeight >> ++i > 0 );
	dwImageHeight = pow( 2.0f, i );

	//����ߴ����ֵ���ܳ���4096
	if( dwImageWidth > 4096 )
		dwImageWidth = 4096;
	if( dwImageHeight > 4096 )
		dwImageHeight = 4096;

	// ͨ����ǰ���洴���豸����HDC
	HDC hDC = ::CreateCompatibleDC( NULL );                    

	//�����ı�������ͼƬ������ɫ������������ʾ�ı����Ч��
	COLORREF tempColor = BackColor;
	BYTE* pRed = (BYTE*)&tempColor;
	if( *pRed == 0 )
		*pRed = 2;
	*pRed -= 1;

	// ���ñ�����ɫ�����ֵ���ɫ
	SetTextColor( hDC, tempColor );                         
	SetBkMode( hDC, 1 );
	SetMapMode( hDC, MM_TEXT );

	//�����߼�����
	LOGFONT lf;
	ZeroMemory( &lf, sizeof(LOGFONT));

	lf.lfHeight			= dwSize;
	lf.lfWeight			= FW_SEMIBOLD;
	lf.lfOutPrecision	= OUT_TT_ONLY_PRECIS;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	lf.lfQuality		= ANTIALIASED_QUALITY;
	_tcscpy( lf.lfFaceName, lpFontName );

	//����λͼ�ṹ
	DWORD* pBitmapBits;                                                        //  ����һ��λͼ
	BITMAPINFO bitmapInfo;
	ZeroMemory( &bitmapInfo.bmiHeader, sizeof(BITMAPINFOHEADER));
	bitmapInfo.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth		= dwImageWidth;
	bitmapInfo.bmiHeader.biHeight		= dwImageHeight;
	bitmapInfo.bmiHeader.biPlanes		= 1;
	bitmapInfo.bmiHeader.biCompression	= BI_RGB;
	bitmapInfo.bmiHeader.biBitCount		= 32;
	//����λͼ
	HBITMAP hBitmap = CreateDIBSection( hDC, &bitmapInfo, DIB_RGB_COLORS, (VOID**)&pBitmapBits, NULL, 0);
	SelectObject( hDC, hBitmap);                        
	// ��������
	HFONT hFont = CreateFontIndirect(&lf);

	SelectObject( hDC, hFont);

	RECT rc = { 0, 0, bitmapInfo.bmiHeader.biWidth, bitmapInfo.bmiHeader.biHeight };
	//����������ˢ
	LOGBRUSH brush;
	brush.lbColor = BackColor;
	brush.lbHatch = HS_HORIZONTAL;
	brush.lbStyle = BS_SOLID;
	HBRUSH hBrush = CreateBrushIndirect( &brush );
	//��䱳��
	FillRect( hDC, &rc, hBrush );
	DeleteObject( hBrush );
	//����ı���ʹ����б������
	TextOut( hDC, 0, 0, lpText, text_length );
	TextOut( hDC, 2, 0, lpText, text_length );
	TextOut( hDC, 0, 2, lpText, text_length );
	TextOut( hDC, 2, 2, lpText, text_length );
	SetTextColor( hDC, TextColor );
	TextOut( hDC, 1, 1, lpText, text_length );

	//��ȡλͼ����
	BITMAP bmp;
	GetObject( hBitmap, sizeof(BITMAP), &bmp);
	BYTE* buffer;   
	int  length;                                  //������hBitMap�ļ��ֽڳ���
	length = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp.bmWidthBytes * bmp.bmHeight;
	buffer = new BYTE[length];

	BITMAPINFOHEADER bi;
	bi.biSize                    =  sizeof(BITMAPINFOHEADER);  
	bi.biWidth                   =  bmp.bmWidth;  
	bi.biHeight                  =  bmp.bmHeight;  
	bi.biPlanes                  =  1;  
	bi.biBitCount                =  bmp.bmBitsPixel;  
	bi.biCompression             =  BI_RGB;  
	bi.biSizeImage               =  0;  
	bi.biXPelsPerMeter           =  0;  
	bi.biYPelsPerMeter           =  0;  
	bi.biClrImportant            =  0;  
	bi.biClrUsed                 =  0;

	BITMAPFILEHEADER bmfHdr;
	bmfHdr.bfType		=  0x4D42;  //  "BM"    
	bmfHdr.bfSize		=  length;    
	bmfHdr.bfReserved1  =  0;    
	bmfHdr.bfReserved2  =  0;    
	bmfHdr.bfOffBits	=  (DWORD)sizeof(BITMAPFILEHEADER)  +  (DWORD)sizeof(BITMAPINFOHEADER);

	memcpy( buffer, &bmfHdr, sizeof(BITMAPFILEHEADER));
	memcpy( &buffer[sizeof(BITMAPFILEHEADER)], &bi, sizeof(BITMAPINFOHEADER));
	memcpy( &buffer[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)],
		bmp.bmBits, length - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER));

	//��������
	ID3D11Texture2D* lpTexture = NULL;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Width = bitmapInfo.bmiHeader.biWidth;
	desc.Height = bitmapInfo.bmiHeader.biHeight;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.SampleDesc.Count = 1;
	desc.MipLevels = 1;

	HRESULT hr = m_lpDevice->CreateTexture2D(&desc, NULL, &lpTexture); 

	if( FAILED(hr) )
	{
		return NULL;
	}

	//����λͼ���������������

	BYTE *pSrcData = (BYTE*)&buffer[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];

	{
		DWORD BufferIndex = 0;
		DWORD color_key = ((BackColor & 0x000000FF) << 16) + 
			((BackColor & 0x00FF0000) >> 16) + 
			(BackColor & 0x0000FF00);	//���ݱ���ɫ��������ɫ

		for(DWORD Y = 0; Y < bitmapInfo.bmiHeader.biHeight; Y++)
		{
			for(DWORD X = 0; X < bitmapInfo.bmiHeader.biWidth; X++)
			{
				DWORD* pSrc = (DWORD*)(pSrcData + (Y * bitmapInfo.bmiHeader.biWidth + X) * 4);
				//�ǹ���ɫ����ʾ
				if( *pSrc != color_key )	
					*pSrc |= 0xFF000000;	
			}
		}			
	}

	m_lpDeviceContext->UpdateSubresource( lpTexture, 0, NULL, pSrcData, bitmapInfo.bmiHeader.biWidth * 4, 0 );

	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
	ZeroMemory( &view_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );

	lpTexture->GetDesc( &desc );

	view_desc.Format					  = desc.Format;
	view_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MostDetailedMip = 0;
	view_desc.Texture2D.MipLevels		  = desc.MipLevels;

	ID3D11ShaderResourceView* lpResourceView = NULL;
	m_lpDevice->CreateShaderResourceView( lpTexture, &view_desc, &lpResourceView );
	SAFE_RELEASE( lpTexture );

	delete[] buffer;
	DeleteObject( hFont );
	DeleteObject( hBitmap);
	DeleteDC( hDC );

	return lpResourceView;
}


VOID CBillboardText::UpdateText( LPCTSTR lpText )
{
	if( m_pTextDisplay )
	{
		m_pTextDisplay->SetText( m_vPos[0], m_vPos[1],
			m_dwSize, lpText, m_TextColor );
	}	
}

VOID CBillboardText::SetFontProperty( LPCSTR lpFontName, DWORD dwSize, 
	COLORREF TextColor, COLORREF BackColor )
{
	std::map<std::string, std::string>::iterator iter = 
		m_mapFontNames.find( lpFontName );
	if( iter != m_mapFontNames.end() )
	{
		if( m_pTextDisplay == NULL )
		{
			if( m_lpDevice && m_lpDeviceContext )
				m_pTextDisplay = new TextDisplay( m_lpDevice, m_lpDeviceContext );
		}
		m_pTextDisplay->SetFontFile( iter->second.c_str() );
	}

	m_dwSize	= dwSize;
	m_TextColor = TextColor;
	m_BackColor = BackColor;

}

void CBillboardText::Render()
{	
	static DWORD last = 0;
	static DWORD cur = timeGetTime();
	static DWORD frame = 0;
	static DWORD uFrame = 0;

	if( cur - last > 1000 )
	{
		last = cur;
		m_dwFPS = frame;
		frame = 0;
	}
	cur = timeGetTime();
	++frame;
			
	if( m_pTextDisplay != NULL )
		m_pTextDisplay->Display();
}

VOID CBillboardText::SetDisplayPos( XMVECTOR vPos )
{
	memcpy( m_vPos, &vPos, sizeof(vPos) );
}

DWORD CBillboardText::GetFPS()
{
	return m_dwFPS;
}