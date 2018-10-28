#pragma once

#define SAFE_RELEASE(obj) { if ( obj ) { obj->Release(); obj = NULL; } }