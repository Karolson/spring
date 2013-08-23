/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BeamLaser.h"
#include "PlasmaRepulser.h"
#include "WeaponDef.h"
#include "WeaponDefHandler.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Matrix44f.h"

#define SWEEPFIRE_ENABLED false

CR_BIND_DERIVED(CBeamLaser, CWeapon, (NULL, NULL));

CR_REG_METADATA(CBeamLaser,(
	CR_MEMBER(color),
	CR_MEMBER(oldDir),
	CR_MEMBER(lastSweepFirePos),
	CR_MEMBER(lastSweepFireDir),
	CR_MEMBER(salvoDamageMult),
	CR_MEMBER(sweepFiring)
));

CBeamLaser::CBeamLaser(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	salvoDamageMult = 1.0f;
	sweepFiring = false;

	color = def->visuals.color;
}



void CBeamLaser::Init()
{
	if (!weaponDef->beamburst) {
		salvoDelay = 0;
		salvoSize = int(weaponDef->beamtime * GAME_SPEED);
		salvoSize = std::max(salvoSize, 1);

		// multiply damage with this on each shot so the total damage done is correct
		salvoDamageMult = 1.0f / salvoSize;
	}

	CWeapon::Init();

	muzzleFlareSize = 0.0f;
}

void CBeamLaser::Update()
{
	if (targetType != Target_None) {
		weaponPos =
			owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos =
			owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		if (!onlyForward) {
			wantedDir = (targetPos - weaponPos).SafeNormalize();
		}

		if (!weaponDef->beamburst) {
			predict = salvoSize / 2;
		} else {
 			// beamburst tracks the target during the burst so there's no need to lead
			predict = 0;
		}
	}

	CWeapon::Update();

	// sweeping always happens between targets
	if (targetType == Target_None)
		return;
	if (!weaponDef->sweepFire)
		return;
	#if (!SWEEPFIRE_ENABLED)
	return;
	#endif

	CUnit* u = NULL;
	CFeature* f = NULL;

	const float3 sweepFireDir = GetFireDir(true);
	const float3 sweepFirePos = weaponMuzzlePos + sweepFireDir * TraceRay::TraceRay(weaponMuzzlePos, sweepFireDir, range, collisionFlags, owner, u, f);

	if (sweepFirePos == lastSweepFirePos) {
		sweepFiring = false;
		return;
	}

	if (teamHandler->Team(owner->team)->metal < metalFireCost) { return; }
	if (teamHandler->Team(owner->team)->energy < energyFireCost) { return; }

	owner->UseEnergy(energyFireCost / salvoSize);
	owner->UseMetal(metalFireCost / salvoSize);

	lastSweepFirePos = sweepFirePos;
	lastSweepFireDir = sweepFireDir;

	FireInternal(sweepFiring = true);
}

float3 CBeamLaser::GetFireDir(bool sweepFire)
{
	float3 dir;

	if (!sweepFire) {
		if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType) != NULL) {
			// [?] StrafeAirMovetype cannot align itself properly, change back when that is fixed
			dir = owner->frontdir;
		} else {
			if (salvoLeft == salvoSize - 1) {
				dir = (targetPos - weaponMuzzlePos).SafeNormalize();
				oldDir = dir;
			} else if (weaponDef->beamburst) {
				dir = (targetPos - weaponMuzzlePos).SafeNormalize();
			} else {
				dir = oldDir;
			}
		}

		dir += SalvoErrorExperience();
		dir.SafeNormalize();

		// NOTE:
		//  on units with (extremely) long weapon barrels the muzzle
		//  can be on the far side of the target unit such that <dir>
		//  would point away from it
		if ((targetPos - weaponMuzzlePos).dot(targetPos - owner->aimPos) < 0.0f) {
			dir = -dir;
		}
	} else {
		const int weaponPiece = owner->script->QueryWeapon(weaponNum);
		const CMatrix44f weaponMat = owner->script->GetPieceMatrix(weaponPiece);

		const float3 relWeaponPos = weaponMat.GetPos();
		const float3 newWeaponDir =
			owner->frontdir * weaponMat[10] +
			owner->updir    * weaponMat[ 6] +
			owner->rightdir * weaponMat[ 2];

		weaponPos =
			owner->pos +
			owner->frontdir * -relWeaponPos.z +
			owner->updir    *  relWeaponPos.y +
			owner->rightdir * -relWeaponPos.x;

		// FIXME:
		//   this way of implementing sweep-fire is extremely bugged
		//   the intersection points traced by rays from the turning
		//   weapon piece do NOT describe a fluid arc segment between
		//   old and new target positions (nor even a straight line)
		dir = newWeaponDir;
	}

	return dir;
}

void CBeamLaser::FireImpl()
{
	// sweepfire must exclude regular fire (!)
	if (sweepFiring)
		return;

	FireInternal(false);
}

void CBeamLaser::FireInternal(bool sweepFire)
{
	// fix negative damage when hitting big spheres
	float actualRange = range;
	float rangeMod = 1.0f;

	if (!owner->unitDef->IsImmobileUnit()) {
		// help units fire while chasing
		rangeMod = 1.3f;
	}
	if (owner->fpsControlPlayer != NULL) {
		rangeMod = 0.95f;
	}

	float maxLength = range * rangeMod;
	float curLength = 0.0f;

	float3 curPos = weaponMuzzlePos;
	float3 curDir = GetFireDir(sweepFire);
	float3 hitPos;
	float3 newDir;

	curDir += (gs->randVector() * SprayAngleExperience() * (1 - sweepFire));
	curDir.SafeNormalize();

	bool tryAgain = true;
	bool doDamage = true;


	// increase range if targets are searched for in a cylinder
	if (cylinderTargeting > 0.01f) {
		const float verticalDist = owner->radius * cylinderTargeting * curDir.y;
		const float maxLengthModSq = maxLength * maxLength + verticalDist * verticalDist;

		maxLength = math::sqrt(maxLengthModSq);
	}

	// increase range if targetting edge of hitsphere
	if (targetType == Target_Unit && targetUnit != NULL && targetBorder != 0) {
		maxLength += (targetUnit->radius * targetBorder);
	}


	// unit at the end of the beam
	CUnit* hitUnit = NULL;
	CFeature* hitFeature = NULL;
	CPlasmaRepulser* hitShield = NULL;
	CollisionQuery hitColQuery;

	for (int tries = 0; tries < 5 && tryAgain; ++tries) {
		float beamLength = TraceRay::TraceRay(curPos, curDir, maxLength - curLength, collisionFlags, owner, hitUnit, hitFeature, &hitColQuery);

		if (hitUnit != NULL && teamHandler->AlliedTeams(hitUnit->team, owner->team) && sweepFire) {
			// never damage friendlies with sweepfire
			doDamage = false; break;
		}

		if (!weaponDef->waterweapon) {
			// terminate beam at water surface if necessary
			if ((curDir.y < 0.0f) && ((curPos.y + curDir.y * beamLength) <= 0.0f)) {
				beamLength = curPos.y / -curDir.y;
			}
		}

		// if the beam gets intercepted, this modifies newDir
		//
		// we do more than one trace-iteration and set dir to
		// newDir only in the case there is a shield in our way
		const float shieldLength = interceptHandler.AddShieldInterceptableBeam(this, curPos, curDir, beamLength, newDir, hitShield);

		if (shieldLength < beamLength) {
			beamLength = shieldLength;
			tryAgain = hitShield->BeamIntercepted(this, salvoDamageMult);
		} else {
			tryAgain = false;
		}

		// same as hitColQuery.GetHitPos() if no water or shield in way
		hitPos = curPos + curDir * beamLength;

		{
			const float baseAlpha  = weaponDef->intensity * 255.0f;
			const float startAlpha = (1.0f - (curLength             ) / (range * 1.3f)) * baseAlpha;
			const float endAlpha   = (1.0f - (curLength + beamLength) / (range * 1.3f)) * baseAlpha;

			ProjectileParams params = GetProjectileParams();
			params.pos = curPos;
			params.end = hitPos;
			params.ttl = std::max(1, weaponDef->beamLaserTTL);
			params.startAlpha = startAlpha;
			params.endAlpha = endAlpha;

			WeaponProjectileFactory::LoadProjectile(params);
		}

		curPos = hitPos;
		curDir = newDir;
		curLength += beamLength;
	}

	if (!doDamage)
		return;

	if (hitUnit != NULL) {
		hitUnit->SetLastAttackedPiece(hitColQuery.GetHitPiece(), gs->frameNum);

		if (targetBorder > 0.0f) {
			actualRange += (hitUnit->radius * targetBorder);
		}
	}

	if (curLength < maxLength) {
		const DamageArray& baseDamages = (weaponDef->dynDamageExp <= 0.0f)?
			weaponDef->damages:
			weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				weaponMuzzlePos,
				curPos,
				(weaponDef->dynDamageRange > 0.0f)?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp,
				weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);

		// make it possible to always hit with some minimal intensity (melee weapons have use for that)
		const float hitIntensity = std::max(minIntensity, 1.0f - (curLength) / (actualRange * 2.0f));

		const DamageArray damages = baseDamages * (hitIntensity * salvoDamageMult);
		const CGameHelper::ExplosionParams params = {
			hitPos,
			curDir,
			damages,
			weaponDef,
			owner,
			hitUnit,
			hitFeature,
			craterAreaOfEffect,
			damageAreaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			1.0f,                                             // gfxMod
			weaponDef->impactOnly,
			weaponDef->noExplode || weaponDef->noSelfDamage,  // ignoreOwner
			true,                                             // damageGround
			-1u                                               // projectileID
		};

		helper->Explosion(params);
	}
}
