//
// Copyright (C) Mei Jun 2011
//

#pragma once

#define SAFE_RELEASE(p) if(p != NULL) { (p)->Release(); (p) = NULL; };
#define SAFE_DELETE(p) if(p != NULL) { delete (p); (p) = NULL; };
#define SAFE_DELETEARRAY(p) if(p != NULL) { delete[] (p); (p) = NULL; };
