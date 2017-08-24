#ifndef __3DENGINE_H_INCLUDED
#define __3DENGINE_H_INCLUDED

//This header is just a convinient way to include all necessary files to use the engine.

#include "components\freeLook.h"
#include "components\freeMove.h"
#include "components/meshRenderer.h"
#include "core/game.h"
#include "core/util.h"

//SDL2 defines a main macro, which can prevent certain compilers from finding the main function.
#undef main

#endif // 3DENGINE_H_INCLUDED