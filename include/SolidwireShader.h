//
// Copyright (C) Mei Jun 2011
//

#pragma once

#include "ShaderHelper.h"

class SolidwireShader : public ShaderHelper
{
public:
	BOOL CreateShaders( ID3D11Device* lpDevice,
		ID3D11DeviceContext* lpContext );
};