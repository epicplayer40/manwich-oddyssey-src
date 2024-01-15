#ifndef FADINGDECALFIX_H
#define FADINGDECALFIX_H

#include "cbase.h"
void DecalShootFixed(int textureIndex, int entity,
	const model_t* model, const Vector& model_origin, const QAngle& model_angles,
	const Vector& position, const Vector* saxis, int flags);

#endif // !FADINGDECALFIX_H
