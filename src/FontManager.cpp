//
// Copyright (C) Mei Jun 2011
//

#include "FontManager.h"
#include "Utility.h"

#ifdef _DEBUG
#pragma comment( lib, "FreeType_d.lib" )
#else
#pragma comment( lib, "FreeType.lib" )
#endif

FontManager::FontManager()
{
}


FontManager::~FontManager(void)
{
	std::map<wchar_t, BMP_DESC>::iterator iter = 
		m_mapFonts.begin();
	while( iter != m_mapFonts.end() )
	{
		SAFE_DELETEARRAY( iter->second.pBuffer );
		++iter;
	}
	m_mapFonts.clear();
}

unsigned char* FontManager::CreateFontTexture( 
	wchar_t iUnicode, size_t uiFontSize, BMP_DESC* pDesc )
{
	FT_Library lib;
	FT_Face face;

	int iBytesPerPixel = 4;
	std::map<wchar_t, BMP_DESC>::iterator iter = 
		m_mapFonts.find( iUnicode );
	if( iter != m_mapFonts.end() )
	{
		*pDesc = iter->second;		
		return iter->second.pBuffer;
	}
	FT_Error error = FT_Init_FreeType( &lib );

	if( error )
		return NULL;

	error = FT_New_Face( lib, m_strFontFileName.c_str(), 0, &face );
	if( error )
		error = FT_New_Face( lib, "C:\\Windows\\Fonts\\simkai.ttf", 0, &face );
		
	if( error )
	{
		FT_Done_FreeType( lib );
		return NULL;
	}
	

	do 
	{
		if( error = FT_Set_Pixel_Sizes( face, 0, uiFontSize ) )
			break;
		FT_UInt glyph_index = FT_Get_Char_Index( face, iUnicode );
		if( error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT ) )
			break;
		if( error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL ) )
			break;
		break;
	} while ( false );

	if( error )
	{
		FT_Done_Face( face );
		FT_Done_FreeType( lib );
	}
	
	int width = face->glyph->bitmap.width;
	int height = face->glyph->bitmap.rows;
	int dwPitch = width * iBytesPerPixel;
	while( dwPitch % 4 != 0 )
		dwPitch++;
	
	size_t uiBufferSize = dwPitch * height;
	unsigned char* pData = new unsigned char[uiBufferSize];
	memset( pData, 0, uiBufferSize );

	unsigned char *pColor = face->glyph->bitmap.buffer;
	int pitch = face->glyph->bitmap.pitch;		

	for( int h = height - 1; h >= 0; --h )
	{		
		for ( int w = 0; w < width; ++w )
		{							
			unsigned char uiTargetColor = 0;
			switch( face->glyph->bitmap.pixel_mode )
			{
			case FT_PIXEL_MODE_MONO:
				{
					unsigned char c = 128;
					int offset = w % 8;
					int mask = c >> offset;
					unsigned char target = *(pColor + w / 8);
					if( target & mask )
					{
						uiTargetColor = 0xFF;
					}
					break;
				}				
			case FT_PIXEL_MODE_GRAY:
				{
					uiTargetColor = *(pColor + w);
					break;
				}
			default:
				break;
			}

			pData[dwPitch * h + w * iBytesPerPixel + iBytesPerPixel - 1] = uiTargetColor;						
		}
		pColor += pitch;
	}

	pDesc->dwWidth = width;
	pDesc->dwHeight = height;
	pDesc->pBuffer = pData;
	pDesc->iXOffset = face->glyph->metrics.horiBearingX >> 6;
	pDesc->iYOffset = face->glyph->metrics.horiBearingY >> 6;
	pDesc->iXAdvance = face->glyph->metrics.horiAdvance >> 6;
	pDesc->iYAdvance = face->glyph->metrics.vertAdvance >> 6;

	m_mapFonts.insert( std::make_pair(iUnicode, *pDesc) );
	
	FT_Done_Face( face );
	FT_Done_FreeType( lib );

	return pData;
}

void FontManager::SetFontFile( const char* pFontFile )
{
	m_strFontFileName = pFontFile;
}