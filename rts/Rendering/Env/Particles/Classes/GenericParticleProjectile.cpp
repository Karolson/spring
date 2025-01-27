/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "GenericParticleProjectile.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/creg/DefTypes.h"

CR_BIND_DERIVED(CGenericParticleProjectile, CProjectile, )

CR_REG_METADATA(CGenericParticleProjectile,(
	CR_MEMBER(gravity),
	CR_IGNORED(texture),
	CR_MEMBER(colorMap),
	CR_MEMBER(directional),
	CR_MEMBER(life),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(airdrag),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod),
	CR_SERIALIZER(Serialize)
))


CGenericParticleProjectile::CGenericParticleProjectile(const CUnit* owner, const float3& pos, const float3& speed)
	: CProjectile(pos, speed, owner, false, false, false)

	, gravity(ZeroVector)
	, texture(nullptr)
	, colorMap(nullptr)
	, directional(false)
	, life(0.0f)
	, decayrate(0.0f)
	, size(0.0f)
	, airdrag(0.0f)
	, sizeGrowth(0.0f)
	, sizeMod(0.0f)
{
	// set fields from super-classes
	useAirLos = true;
	checkCol  = false;
	deleteMe  = false;
}

void CGenericParticleProjectile::Serialize(creg::ISerializer* s)
{
	std::string name;
	if (s->IsWriting())
		name = projectileDrawer->textureAtlas->GetTextureName(texture);
	creg::GetType(name)->Serialize(s, &name);
	if (!s->IsWriting())
		texture = projectileDrawer->textureAtlas->GetTexturePtr(name);
}

void CGenericParticleProjectile::Update()
{
	SetPosition(pos + speed);
	SetVelocityAndSpeed((speed + gravity) * airdrag);

	life += decayrate;
	size = size * sizeMod + sizeGrowth;

	deleteMe |= (life > 1.0f);
}

void CGenericParticleProjectile::Draw()
{
	float3 dir1 = camera->GetRight();
	float3 dir2 = camera->GetUp();
	if (directional) {
		float3 dif(pos - camera->GetPos());
		dif.ANormalize();
		dir1 = dif.cross(speed).ANormalize();
		dir2 = dif.cross(dir1);
	}

	unsigned char color[4];
	colorMap->GetColor(color, life);
	AddEffectsQuad(
		{ drawPos + (-dir1 - dir2) * size, texture->xstart, texture->ystart, color },
		{ drawPos + (-dir1 + dir2) * size, texture->xend,   texture->ystart, color },
		{ drawPos + ( dir1 + dir2) * size, texture->xend,   texture->yend,   color },
		{ drawPos + ( dir1 - dir2) * size, texture->xstart, texture->yend,   color }
	);
}

int CGenericParticleProjectile::GetProjectilesCount() const
{
	return 1;
}
