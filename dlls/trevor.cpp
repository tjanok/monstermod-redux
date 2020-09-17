/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// human scientist (passive lab worker)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"animation.h"

// Global settings
const char* roboModel = "models/players/robo/robo.mdl";

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================


//=========================================================
// AI Schedules Specific to this monster
//=========================================================
void CMRoboTrevor::StartTask( Task_t* pTask )
{
	switch( pTask->iTask )
	{
	}

	return CMBaseMonster::StartTask( pTask );
}

void CMRoboTrevor::RunTask( Task_t* pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if( MovementIsComplete() )
			TaskComplete();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
	{

		if( m_hEnemy == NULL )
		{
			TaskFail();
		}
		else
		{
			float distance;

			distance = ( m_vecMoveGoal - pev->origin ).Length2D();
			// Re-evaluate when you think your finished, or the target has moved too far
			if( ( distance < pTask->flData ) || ( m_vecMoveGoal - m_hTargetEnt->v.origin ).Length() > pTask->flData * 0.5 )
			{
				m_vecMoveGoal = m_hTargetEnt->v.origin;
				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				FRefreshRoute();
			}

			// Set the appropriate activity based on an overlapping range
			// overlap the range to prevent oscillation
			if( distance < pTask->flData )
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			else if( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
				m_movementActivity = ACT_WALK_SCARED;
			else if( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
				m_movementActivity = ACT_RUN_SCARED;
		}
	}
	break;

	case TASK_HEAL:
		if( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			//if( TargetDistance() > 90 )
				//TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->v.origin - pev->origin );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CMBaseMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMRoboTrevor::Classify( void )
{
	return	CLASS_HUMAN_PASSIVE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMRoboTrevor::SetYawSpeed( void )
{
	int ys;

	ys = 90;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMRoboTrevor::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch( pEvent->event )
	{
	default:
		CMBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CMRoboTrevor::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), roboModel );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMRoboTrevor::Precache( void )
{
	PRECACHE_MODEL( (char*)roboModel );
	CMBaseMonster::Precache();
}

int CMRoboTrevor::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{

	if( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}

	// make sure friends talk about it if player hurts scientist...
	return CMBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CMRoboTrevor::ISoundMask( void )
{
	return 0;
}

//=========================================================
// PainSound
//=========================================================
void CMRoboTrevor::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );
}

//=========================================================
// DeathSound 
//=========================================================
void CMRoboTrevor::DeathSound( void )
{
	PainSound();
}


void CMRoboTrevor::Killed( entvars_t* pevAttacker, int iGib )
{
	SetUse( NULL );
	CMBaseMonster::Killed( pevAttacker, iGib );
}


void CMRoboTrevor::SetActivity( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CMBaseMonster::SetActivity( newActivity );
}


Schedule_t* CMRoboTrevor::GetScheduleOfType( int Type )
{
	Schedule_t* psched;

	switch( Type )
	{
	}

	return CMBaseMonster::GetScheduleOfType( Type );
}

Schedule_t* CMRoboTrevor::GetSchedule( void )
{
	// so we don't keep calling through the EHANDLE stuff
	edict_t* pEnemy = m_hEnemy;
	int relationship;

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( ( pEnemy ) && !UTIL_IsPlayer( pEnemy ) )
		{
			if( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = NULL;
				pEnemy = NULL;
			}
		}

		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Nothing scary, just me and the player
		if( pEnemy != NULL )
		{
			relationship = R_NO;

			if( UTIL_IsPlayer( pEnemy ) )
				relationship = R_AL;  // allies
			else if( pEnemy->v.euser4 != NULL )
			{
				CMBaseMonster* pMonster = GetClassPtr( ( CMBaseMonster* ) VARS( pEnemy ) );
				relationship = IRelationship( pMonster );
			}

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if( relationship != R_DL && relationship != R_HT )
			{
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
					return GetScheduleOfType( SCHED_FEAR );					// React to something scary
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
			}
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		break;

	case MONSTERSTATE_COMBAT:
		/*
		if( HasConditions( bits_COND_NEW_ENEMY ) )
			return slFear;					// Point and scream!
		if( HasConditions( bits_COND_SEE_ENEMY ) )
			return slScientistCover;		// Take Cover

		if( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slScientistCover;			// Run & Cower
		*/
		break;
	}

	return CMBaseMonster::GetSchedule();
}

MONSTERSTATE CMRoboTrevor::GetIdealState( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( HasConditions( bits_COND_NEW_ENEMY ) )
		{
		}
		else if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
		}
		break;

	case MONSTERSTATE_COMBAT:
	{
		edict_t* pEnemy = m_hEnemy;
		if( pEnemy != NULL )
		{
			if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				// Strip enemy when going to alert
				m_IdealMonsterState = MONSTERSTATE_ALERT;
				m_hEnemy = NULL;
				return m_IdealMonsterState;
			}
			// Follow if only scared a little
			if( m_hTargetEnt != NULL )
			{
				m_IdealMonsterState = MONSTERSTATE_ALERT;
				return m_IdealMonsterState;
			}

			if( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				m_fearTime = gpGlobals->time;
				m_IdealMonsterState = MONSTERSTATE_COMBAT;
				return m_IdealMonsterState;
			}

		}
	}
	break;
	}

	return CMBaseMonster::GetIdealState();
}

BOOL CMRoboTrevor::DisregardEnemy( edict_t* pEnemy )
{
	return !UTIL_IsAlive( pEnemy ) || ( gpGlobals->time - m_fearTime ) > 15;
}

