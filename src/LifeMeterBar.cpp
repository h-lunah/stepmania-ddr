#include "global.h"
#include "LifeMeterBar.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "GameState.h"
#include "RageMath.h"
#include "ThemeManager.h"
#include "Song.h"
#include "StatsManager.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "Quad.h"
#include "ActorUtil.h"
#include "StreamDisplay.h"
#include "Steps.h"
#include "Course.h"

#include <vector>
#include <numeric>

static RString LIFE_PERCENT_CHANGE_NAME( size_t i )   { return "LifePercentChange" + ScoreEventToString( (ScoreEvent)i ); }


// @brief DDR life penalty scalar function
double LifeMeterBar::CalculatePenalty(int X) {
	constexpr int beginnerComboThreshold = 8;       // How many misses before the penalties are increased?
	constexpr double beginnerLow = 0.2;              // Lowest lenient penalty multiplier
	constexpr double beginnerHigh = 1.0;             // Highest lenient penalty multiplier
	constexpr double maxMultiplier = 1.8;            // Highest penalty multiplier
	constexpr int consistentPenaltyMultiplier = 3.0; // Consistent performance penalty multiplier

	// Calculate on at least 1 comboed note
	if (X <= 0) {
		X = 1;
	}

	// Declare overrideable values
	double ret;
	double averagePenalty;

	// Determine player performance for scaling
	if (recentMultipliers.size() > 0) { // Don't divide by 0
		averagePenalty = accumulate(recentMultipliers.begin(), recentMultipliers.end(), 0.0) / recentMultipliers.size();
	}
	else {
		averagePenalty = 0;
	}

	// Snap current combo to keep a consistent difficulty
	if (averagePenalty < 0.5 && averagePenalty != 0) {
		X = abs(beginnerComboThreshold / consistentPenaltyMultiplier) + 1; // Consistent bad performance
	}
	else if (averagePenalty >= 1 && averagePenalty != 0) {
		X = abs(beginnerComboThreshold * consistentPenaltyMultiplier) + 1; // Consistent good performance
	}

	// Player is not keeping a combo
	if (X <= beginnerComboThreshold) {
		ret = beginnerLow + (beginnerHigh - beginnerLow) * (static_cast<double>(X) / beginnerComboThreshold);
	}
	// Player is doing well
	else {
		double scaledX = static_cast<double>(X - beginnerComboThreshold) / (maxMultiplier - beginnerHigh);
		ret = beginnerHigh + (maxMultiplier - beginnerHigh) * std::min(scaledX, 1.0); // Don't make life difficulty absurd
	}

	// Ensure performance can be determined on consecutive calls
	recentMultipliers.push_back(ret);
	return ret;
}

LifeMeterBar::LifeMeterBar( PlayerNumber pn )
{
	// Performance should be reset upon loading a lifebar
	ResetPerformanceAdjustments();
	
	DANGER_THRESHOLD.Load	("LifeMeterBar","DangerThreshold");
	DANGER_THRESHOLD_NO_COMMENT.Load("LifeMeterBar", "DangerThresholdNoComment");
	INITIAL_VALUE.Load	("LifeMeterBar","InitialValue");
	HOT_VALUE.Load		("LifeMeterBar","HotValue");
	LIFE_MULTIPLIER.Load	( "LifeMeterBar","LifeMultiplier");
	MIN_STAY_ALIVE.Load	("LifeMeterBar","MinStayAlive");
	FORCE_LIFE_DIFFICULTY_ON_EXTRA_STAGE.Load ("LifeMeterBar","ForceLifeDifficultyOnExtraStage");
	EXTRA_STAGE_LIFE_DIFFICULTY.Load	("LifeMeterBar","ExtraStageLifeDifficulty");
	m_fLifePercentChange.Load( "LifeMeterBar", LIFE_PERCENT_CHANGE_NAME, NUM_ScoreEvent );

	m_pPlayerState = nullptr;


	const RString sType = "LifeMeterBar";

	m_fPassingAlpha = 0;
	m_fHotAlpha = 0;

	// DDR like difficulty
	m_fBaseLifeDifficulty = 1.3f;
	m_fLifeDifficulty = m_fBaseLifeDifficulty;

	// set up progressive lifebar
	m_iProgressiveLifebar = PREFSMAN->m_iProgressiveLifebar;
	m_iMissCombo = 0;
	m_iCombo = 0;

	// set up combotoregainlife
	m_iComboToRegainLife = 0;

	bool bExtra = GAMESTATE->IsAnExtraStage();
	RString sExtra = bExtra ? "extra " : "";

	m_sprUnder.Load( THEME->GetPathG(sType,sExtra+"Under") );
	m_sprUnder->SetName( "Under" );
	ActorUtil::LoadAllCommandsAndSetXY( m_sprUnder, sType );
	this->AddChild( m_sprUnder );

	m_sprDanger.Load( THEME->GetPathG(sType,sExtra+"Danger") );
	m_sprDanger->SetName( "Danger" );
	ActorUtil::LoadAllCommandsAndSetXY( m_sprDanger, sType );
	this->AddChild( m_sprDanger );

	m_pStream = new StreamDisplay;
	m_pStream->Load(bExtra ? "StreamDisplayExtra" : "StreamDisplay", pn);
	m_pStream->SetName( "Stream" );
	ActorUtil::LoadAllCommandsAndSetXY( m_pStream, sType );
	this->AddChild( m_pStream );

	m_sprOver.Load( THEME->GetPathG(sType,sExtra+"Over") );
	m_sprOver->SetName( "Over" );
	ActorUtil::LoadAllCommandsAndSetXY( m_sprOver, sType );
	this->AddChild( m_sprOver );
}

LifeMeterBar::~LifeMeterBar()
{
	SAFE_DELETE( m_pStream );
}

void LifeMeterBar::Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats )
{
	LifeMeter::Load( pPlayerState, pPlayerStageStats );

	PlayerNumber pn = pPlayerState->m_PlayerNumber;

	DrainType dtype = pPlayerState->m_PlayerOptions.GetStage().m_DrainType;
	switch( dtype )
	{
		case DrainType_Normal:
			m_fLifePercentage = INITIAL_VALUE;
			break;
			/* These types only go down, so they always start at full. */
		case DrainType_Class:
		case DrainType_Flare1:
		case DrainType_Flare2:
		case DrainType_Flare3:
		case DrainType_Flare4:
		case DrainType_Flare5:
		case DrainType_Flare6:
		case DrainType_Flare7:
		case DrainType_Flare8:
		case DrainType_Flare9:
		case DrainType_FlareEX:
		case DrainType_FloatingFlare:
		case DrainType_NoRecover:
		case DrainType_SuddenDeath:
			m_fLifePercentage = 1.0f;	break;
		default:
			FAIL_M(ssprintf("Invalid DrainType: %i", dtype));
	}

	// Change life difficulty to really easy if merciful beginner on
	m_bMercifulBeginnerInEffect =
		GAMESTATE->m_PlayMode == PLAY_MODE_REGULAR  &&
		GAMESTATE->IsPlayerEnabled( pPlayerState )  &&
		GAMESTATE->m_pCurSteps[pn]->GetDifficulty() == Difficulty_Beginner  &&
		PREFSMAN->m_bMercifulBeginner;

	AfterLifeChanged();
}

void LifeMeterBar::ChangeLife( TapNoteScore score )
{
	float fDeltaLife=0.f;
	// this was previously if( IsHot()  &&  score < TNS_GOOD ) in 3.9... -freem
	if(PREFSMAN->m_HarshHotLifePenalty && IsHot()  &&  fDeltaLife < 0)
		fDeltaLife = min( fDeltaLife, -0.10f );		// make it take a while to get back to "hot"

	DrainType dtype = m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType;
	switch(dtype)
	{
	DEFAULT_FAIL(m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType);
	case DrainType_Normal:
		switch (score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W1);	break;
		case TNS_W2:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W2);	break;
		case TNS_W3:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W3);	break;
		case TNS_W4:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W4);	break;
		case TNS_Miss:		fDeltaLife = m_fLifePercentChange.GetValue(SE_Miss);	break;
		case TNS_HitMine:	fDeltaLife = m_fLifePercentChange.GetValue(SE_HitMine);	break;
		case TNS_None:		fDeltaLife = m_fLifePercentChange.GetValue(SE_Miss);	break;
		case TNS_CheckpointHit:	fDeltaLife = m_fLifePercentChange.GetValue(SE_CheckpointHit);	break;
		case TNS_CheckpointMiss:fDeltaLife = m_fLifePercentChange.GetValue(SE_CheckpointMiss);	break;
		}

		if (fDeltaLife >= 0) {
			m_iCombo += 1;
			m_iMissCombo = 0;
			if (m_iCombo > 10)
				double _ = CalculatePenalty(min(m_iCombo, 30));  // XXX: unused when called with TNS_W4 or better, but allows the vector to grow
		}
		else {
			if (m_iCombo <= 8) {
				m_iCombo = 0;
			}
			else {
				m_iCombo /= 2;
			}
			m_iMissCombo += 1;
			// Scale based on performance
			fDeltaLife *= CalculatePenalty(min(m_iCombo, 30));
			// Punish more for consecutive misses
			fDeltaLife *= min((1 + (m_iMissCombo * 0.2)), 3);
		}

		break;
	case DrainType_Class:
		switch (score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W1);	break;
		case TNS_W2:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W2);	break;
		case TNS_W3:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W3);	break;
		case TNS_W4:		fDeltaLife = m_fLifePercentChange.GetValue(SE_W4);	break;
		case TNS_Miss:		fDeltaLife = m_fLifePercentChange.GetValue(SE_Miss);	break;
		case TNS_HitMine:	fDeltaLife = m_fLifePercentChange.GetValue(SE_HitMine);	break;
		case TNS_None:		fDeltaLife = m_fLifePercentChange.GetValue(SE_Miss);	break;
		case TNS_CheckpointHit:	fDeltaLife = m_fLifePercentChange.GetValue(SE_CheckpointHit);	break;
		case TNS_CheckpointMiss:fDeltaLife = m_fLifePercentChange.GetValue(SE_CheckpointMiss);	break;
		}

        // Count misses and scale penalties up
		if ( fDeltaLife < 0 ) {
			m_iMissCombo += 1;
		} else {
			m_iMissCombo = 0;
		}
		// Dan is an endurance course, halve the deltas
		fDeltaLife *= 0.5f;
		fDeltaLife *= min((1 + (m_iMissCombo * 0.2)), 3);
	case DrainType_Flare1:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[0]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[0]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[0]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[0]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[0]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[0]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[0]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[0]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[0]; break;
		}
		break;
	case DrainType_Flare2:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[1]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[1]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[1]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[1]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[1]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[1]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[1]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[1]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[1]; break;
		}
		break;
	case DrainType_Flare3:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[2]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[2]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[2]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[2]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[2]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[2]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[2]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[2]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[2]; break;
		}
		break;
	case DrainType_Flare4:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[3]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[3]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[3]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[3]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[3]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[3]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[3]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[3]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[3]; break;
		}
		break;
	case DrainType_Flare5:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[4]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[4]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[4]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[4]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[4]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[4]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[4]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[4]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[4]; break;
		}
		break;
	case DrainType_Flare6:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[5]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[5]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[5]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[5]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[5]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[5]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[5]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[5]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[5]; break;
		}
		break;
	case DrainType_Flare7:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[6]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[6]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[6]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[6]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[6]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[6]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[6]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[6]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[6]; break;
		}
		break;
	case DrainType_Flare8:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[7]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[7]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[7]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[7]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[7]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[7]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[7]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[7]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[7]; break;
		}
		break;
	case DrainType_Flare9:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[8]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[8]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[8]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[8]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[8]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[8]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[8]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[8]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[8]; break;
		}
		break;
	case DrainType_FlareEX:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1: fDeltaLife = FlareJudgmentsW1[9]; break;
		case TNS_W2: fDeltaLife = FlareJudgmentsW2[9]; break;
		case TNS_W3: fDeltaLife = FlareJudgmentsW3[9]; break;
		case TNS_W4: fDeltaLife = FlareJudgmentsW4[9]; break;
		case TNS_Miss: fDeltaLife = FlareJudgmentsMiss[9]; break;
		case TNS_HitMine: fDeltaLife = FlareJudgmentsMiss[9]; break;
		case TNS_None: fDeltaLife = FlareJudgmentsW1[9]; break;
		case TNS_CheckpointHit: fDeltaLife = FlareJudgmentsW1[9]; break;
		case TNS_CheckpointMiss: fDeltaLife = FlareJudgmentsW1[9]; break;
		}
		break;
	case DrainType_FloatingFlare:
		switch(score)
		{
		DEFAULT_FAIL(score);
		case TNS_W1:
			if ( m_fLifePercentage + FlareJudgmentsW1[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsW1[currFloatingFlareIndex];
			} else {
				if ( currFloatingFlareIndex > 0 )
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsW1[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsW1[currFloatingFlareIndex];
			}
			break;
		case TNS_W2:
			if ( m_fLifePercentage + FlareJudgmentsW2[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsW2[currFloatingFlareIndex];
			} else {
				if ( currFloatingFlareIndex > 0 )
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsW2[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsW2[currFloatingFlareIndex];
			}
			break;
		case TNS_W3:
			if ( m_fLifePercentage + FlareJudgmentsW3[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsW3[currFloatingFlareIndex];
			} else {
				if ( currFloatingFlareIndex > 0 )
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsW3[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsW3[currFloatingFlareIndex];
			}
			break;
		case TNS_W4:
			if ( m_fLifePercentage + FlareJudgmentsW4[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsW4[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsW4[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsW4[currFloatingFlareIndex];
			}
			break;
		case TNS_Miss:
			if ( m_fLifePercentage + FlareJudgmentsMiss[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsMiss[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsMiss[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsMiss[currFloatingFlareIndex];
			}
			break;
		case TNS_HitMine:
			if ( m_fLifePercentage + FlareJudgmentsMiss[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsMiss[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsMiss[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsMiss[currFloatingFlareIndex];
			}
			break;
		case TNS_CheckpointHit:
			if ( m_fLifePercentage + FlareJudgmentsW1[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsW1[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsW1[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsW1[currFloatingFlareIndex];
			}
			break;
		case TNS_CheckpointMiss:
			if ( m_fLifePercentage + FlareJudgmentsW1[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsW1[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsW1[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsW1[currFloatingFlareIndex];
			}
			break;
		}
	case DrainType_NoRecover:
		fDeltaLife = min( fDeltaLife, 0 );
		break;
	case DrainType_SuddenDeath:
		if( score < MIN_STAY_ALIVE )
			fDeltaLife = -1.0f;
		else
			fDeltaLife = 0;
		break;
	}

	ChangeLife( fDeltaLife );
}

void LifeMeterBar::ChangeLife( HoldNoteScore score, TapNoteScore tscore )
{
	float fDeltaLife=0.f;
	DrainType dtype = m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType;

	switch( dtype )
	{
	case DrainType_Normal:
		switch( score )
		{
		case HNS_Held:		fDeltaLife = m_fLifePercentChange.GetValue(SE_Held);	break;
		case HNS_LetGo:	fDeltaLife = m_fLifePercentChange.GetValue(SE_LetGo);	break;
		case HNS_Missed:	fDeltaLife = m_fLifePercentChange.GetValue(SE_Missed);	break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		if(PREFSMAN->m_HarshHotLifePenalty && IsHot()  &&  score == HNS_LetGo)
			fDeltaLife = -0.10f;		// make it take a while to get back to "hot"

		if (fDeltaLife >= 0) {
			m_iCombo += 1;
			m_iMissCombo = 0;
			if ( m_iCombo > 10 )
				double _ = CalculatePenalty(min(m_iCombo, 30));  // XXX: unused when called with TNS_W4 or better, but allows the vector to grow
		}
		else {
			if (m_iCombo <= 8) {
				m_iCombo = 0;
			}
			else {
				m_iCombo /= 2;
			}
			// Scale based on performance
			fDeltaLife *= CalculatePenalty(min(m_iCombo, 30));
			// Punish more for consecutive misses
			fDeltaLife *= min((1 + (m_iMissCombo * 0.2)), 3);
		}

		break;
	case DrainType_Class:
		switch( score )
		{
		case HNS_Held: fDeltaLife = m_fLifePercentChange.GetValue(SE_Held); break;
		case HNS_LetGo: fDeltaLife = m_fLifePercentChange.GetValue(SE_LetGo); break;
		case HNS_Missed: fDeltaLife = m_fLifePercentChange.GetValue(SE_Missed); break;
        default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		// Count misses and scale penalties up
		if ( fDeltaLife < 0 ) {
			m_iMissCombo += 1;
		} else {
			m_iMissCombo = 0;
		}
		// Dan is an endurance course, halve the deltas
	    fDeltaLife *= 0.5f;                                               
	    fDeltaLife *= min((1 + (m_iMissCombo * 0.2)), 3);
	    break;
	case DrainType_Flare1:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[0]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[0]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[0]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare2:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[1]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[1]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[1]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare3:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[2]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[2]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[2]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare4:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[3]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[3]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[3]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare5:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[4]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[4]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[4]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare6:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[5]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[5]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[5]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare7:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[6]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[6]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[6]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare8:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[7]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[7]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[7]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_Flare9:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[8]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[8]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[8]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
	    break;
	case DrainType_FlareEX:
		switch(score)
		{
		case HNS_Held: fDeltaLife = FlareJudgmentsHeld[9]; break;
		case HNS_LetGo: fDeltaLife = FlareJudgmentsLetGo[9]; break;
		case HNS_Missed: fDeltaLife = FlareJudgmentsHeld[9]; break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
        break;
	case DrainType_FloatingFlare:
		switch (score)
		{
		case HNS_Held:
			if ( m_fLifePercentage + FlareJudgmentsHeld[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsHeld[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsHeld[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsHeld[currFloatingFlareIndex];
			}
			break;
		case HNS_LetGo:
			if ( m_fLifePercentage + FlareJudgmentsLetGo[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsLetGo[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsLetGo[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsLetGo[currFloatingFlareIndex];
			}
			break;
		case HNS_Missed:
			if ( m_fLifePercentage + FlareJudgmentsHeld[currFloatingFlareIndex] > 0 )
			{
				fDeltaLife = FlareJudgmentsHeld[currFloatingFlareIndex];
			} else {
				if (currFloatingFlareIndex > 0)
				{
					--currFloatingFlareIndex;
					fDeltaLife = 0;
					SetLife(1.0f - FlareJudgmentsMissed[currFloatingFlareIndex]);
					break;
				} else fDeltaLife = FlareJudgmentsHeld[currFloatingFlareIndex];
			}
			break;
		}
	case DrainType_NoRecover:
		switch( score )
		{
		case HNS_Held:		fDeltaLife = 0;	break;
		case HNS_LetGo:	fDeltaLife = m_fLifePercentChange.GetValue(SE_LetGo);	break;
		case HNS_Missed:		fDeltaLife = 0;	break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	case DrainType_SuddenDeath:
		switch( score )
		{
		case HNS_Held:		fDeltaLife = 0;	break;
		case HNS_LetGo:	fDeltaLife = -1.0f;	break;
		case HNS_Missed:	fDeltaLife = 0;	break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
		}
		break;
	default:
		FAIL_M(ssprintf("Invalid DrainType: %i", dtype));
	}

	ChangeLife( fDeltaLife );
}

void LifeMeterBar::ChangeLife( float fDeltaLife )
{
	bool bUseMercifulDrain = m_bMercifulBeginnerInEffect || PREFSMAN->m_bMercifulDrain;
	if( bUseMercifulDrain  &&  fDeltaLife < 0 )
		fDeltaLife *= SCALE( m_fLifePercentage, 0.f, 1.f, 0.5f, 1.f);

	// handle progressiveness and ComboToRegainLife here
	if( fDeltaLife >= 0 )
	{
		m_iMissCombo = 0;
		m_iComboToRegainLife = max( m_iComboToRegainLife-1, 0 );
		if ( m_iComboToRegainLife > 0 )
			fDeltaLife = 0.0f;
	}
	else
	{
		fDeltaLife *= 1 + (float)m_iProgressiveLifebar/8 * m_iMissCombo;
		// do this after; only successive miss will increase the amount of life lost.
		m_iMissCombo++;
		/* Increase by m_iRegenComboAfterMiss; never push it beyond m_iMaxRegenComboAfterMiss
		 * but don't reduce it if it's already past. */
		const int NewComboToRegainLife = 0;

		m_iComboToRegainLife = max( m_iComboToRegainLife, NewComboToRegainLife );
	}

	// If we've already failed, there's no point in letting them fill up the bar again.
	if( m_pPlayerStageStats->m_bFailed )
		return;

	switch(m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType)
	{
		case DrainType_Normal:
		case DrainType_Class:
			if( fDeltaLife > 0 )
				fDeltaLife *= m_fLifeDifficulty;
			else
				fDeltaLife /= m_fLifeDifficulty;
			break;
		case DrainType_Flare1:
		case DrainType_Flare2:
		case DrainType_Flare3:
		case DrainType_Flare4:
		case DrainType_Flare5:
		case DrainType_Flare6:
		case DrainType_Flare7:
		case DrainType_Flare8:
		case DrainType_Flare9:
		case DrainType_FlareEX:
			// Substract like Flare, don't increase if > 0.
			if (fDeltaLife < 0)
				fDeltaLife = fDeltaLife;
			else
				fDeltaLife = 0;
			break;
		case DrainType_FloatingFlare:
			// Substract like Floating Flare, allow 1.0f to reset to a new Flare Gauge
			if (fDeltaLife < 0 || fDeltaLife == 1.0f)
				fDeltaLife = fDeltaLife;
			else
				fDeltaLife = 0;
			break;
		case DrainType_NoRecover:
			if( fDeltaLife > 0 )
				fDeltaLife *= m_fLifeDifficulty;
			else
				fDeltaLife /= m_fLifeDifficulty;
			break;
		case DrainType_SuddenDeath:
			// This should always -1.0f;
			if( fDeltaLife < 0 )
				fDeltaLife = -1.0f;
			else
				fDeltaLife = 0;
			break;
		default:
		break;
	}

	m_fLifePercentage += fDeltaLife;
	CLAMP( m_fLifePercentage, 0, LIFE_MULTIPLIER );
	AfterLifeChanged();
}

void LifeMeterBar::SetLife(float value)
{
	m_fLifePercentage= value;
	CLAMP( m_fLifePercentage, 0, LIFE_MULTIPLIER );
	AfterLifeChanged();
}

extern ThemeMetric<bool> PENALIZE_TAP_SCORE_NONE;
void LifeMeterBar::HandleTapScoreNone()
{
	if( PENALIZE_TAP_SCORE_NONE )
		ChangeLife( TNS_None );
}

void LifeMeterBar::AfterLifeChanged()
{
	m_pStream->SetPercent( m_fLifePercentage );

	Message msg( "LifeChanged" );
	msg.SetParam( "Player", m_pPlayerState->m_PlayerNumber );
	msg.SetParam( "LifeMeter", LuaReference::CreateFromPush(*this) );
	MESSAGEMAN->Broadcast( msg );
}

bool LifeMeterBar::IsHot() const
{
	return m_fLifePercentage >= HOT_VALUE;
}

bool LifeMeterBar::IsInDanger() const
{
    // Floating Flare: FLARE I danger
	if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_DrainType == DrainType_FloatingFlare && currFloatingFlareIndex == 0) {
		return m_fLifePercentage < DANGER_THRESHOLD;
	}
	// Danger should not be visible when Floating Flare is not FLARE I
	else if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_DrainType == DrainType_FloatingFlare && currFloatingFlareIndex > 0) {
		return false;
	}
	// Show danger for normal gauges
	else {
		return m_fLifePercentage < DANGER_THRESHOLD;
	}
}

/* @brief Shows Danger state but doesn't give Danger comments */
bool LifeMeterBar::DangerShouldComment() const {
	if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_DrainType == DrainType_FloatingFlare && currFloatingFlareIndex == 0) {
		return m_fLifePercentage > DANGER_THRESHOLD && m_fLifePercentage < DANGER_THRESHOLD_NO_COMMENT;
	}
	// Danger should not be visible when Floating Flare is not FLARE I
	else if (m_pPlayerState->m_PlayerOptions.GetCurrent().m_DrainType == DrainType_FloatingFlare && currFloatingFlareIndex > 0) {
		return false;
	}
	// Show danger for normal gauges
	else {
		return m_fLifePercentage > DANGER_THRESHOLD && m_fLifePercentage < DANGER_THRESHOLD_NO_COMMENT;
	}
}

bool LifeMeterBar::IsFailing() const
{
	return m_fLifePercentage <= m_pPlayerState->m_PlayerOptions.GetCurrent().m_fPassmark;
}


void LifeMeterBar::Update( float fDeltaTime )
{
	LifeMeter::Update( fDeltaTime );

	m_fPassingAlpha += !IsFailing() ? +fDeltaTime*2 : -fDeltaTime*2;
	CLAMP( m_fPassingAlpha, 0, 1 );

	m_fHotAlpha  += IsHot() ? + fDeltaTime*2 : -fDeltaTime*2;
	CLAMP( m_fHotAlpha, 0, 1 );

	m_pStream->SetPassingAlpha( m_fPassingAlpha );
	m_pStream->SetHotAlpha( m_fHotAlpha );

	if( m_pPlayerState->m_HealthState == HealthState_Danger && m_fLifePercentage != 0 )
		m_sprDanger->SetVisible ( true );
	if (m_pPlayerState->m_HealthState == HealthState_DangerNoComment && m_fLifePercentage != 0)
		m_sprDanger->SetVisible ( true );
	else
		m_sprDanger->SetVisible( false );
}


void LifeMeterBar::UpdateNonstopLifebar()
{
	int iCleared, iTotal, iProgressiveLifebarDifficulty;

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_REGULAR:
		if( GAMESTATE->IsEventMode() || GAMESTATE->m_bDemonstrationOrJukebox )
			return;

		iCleared = GAMESTATE->m_iCurrentStageIndex;
		iTotal = PREFSMAN->m_iSongsPerPlay;
		iProgressiveLifebarDifficulty = PREFSMAN->m_iProgressiveStageLifebar;
		break;
	case PLAY_MODE_NONSTOP:
		iCleared = GAMESTATE->GetCourseSongIndex();
		iTotal = GAMESTATE->m_pCurCourse->GetEstimatedNumStages();
		iProgressiveLifebarDifficulty = PREFSMAN->m_iProgressiveNonstopLifebar;
		break;
	default:
		return;
	}

//	if (iCleared > iTotal) iCleared = iTotal; // clear/iTotal <= 1
//	if (iTotal == 0) iTotal = 1;  // no division by 0

	if( GAMESTATE->IsAnExtraStage() && FORCE_LIFE_DIFFICULTY_ON_EXTRA_STAGE )
	{
		// extra stage is its own thing, should not be progressive
		// and it should be as difficult as life 4
		// (e.g. it should not depend on life settings)

		m_iProgressiveLifebar = 0;
		m_fLifeDifficulty = EXTRA_STAGE_LIFE_DIFFICULTY;
		return;
	}

	if( iTotal > 1 )
		m_fLifeDifficulty = m_fBaseLifeDifficulty - 0.2f * (int)(iProgressiveLifebarDifficulty * iCleared / (iTotal - 1));
	else
		m_fLifeDifficulty = m_fBaseLifeDifficulty - 0.2f * iProgressiveLifebarDifficulty;

	if( m_fLifeDifficulty >= 0.4f )
		return;

	/* Approximate deductions for a miss
	 * Life 1 :    5   %
	 * Life 2 :    5.7 %
	 * Life 3 :    6.6 %
	 * Life 4 :    8   %
	 * Life 5 :   10   %
	 * Life 6 :   13.3 %
	 * Life 7 :   20   %
	 * Life 8 :   26.6 %
	 * Life 9 :   32   %
	 * Life 10:   40   %
	 * Life 11:   50   %
	 * Life 12:   57.1 %
	 * Life 13:   66.6 %
	 * Life 14:   80   %
	 * Life 15:  100   %
	 * Life 16+: 200   %
	 *
	 * Everything past 7 is intended mainly for nonstop mode.
	 */

	// the lifebar is pretty harsh at 0.4 already (you lose
	// about 20% of your lifebar); at 0.2 it would be 40%, which
	// is too harsh at one difficulty level higher.  Override.
	int iLifeDifficulty = int( (1.8f - m_fLifeDifficulty)/0.2f );

	// first eight values don't matter
	float fDifficultyValues[16] = {0,0,0,0,0,0,0,0,
		0.3f, 0.25f, 0.2f, 0.16f, 0.14f, 0.12f, 0.10f, 0.08f};

	if( iLifeDifficulty >= 16 )
	{
		// judge 16 or higher
		m_fLifeDifficulty = 0.04f;
		return;
	}

	m_fLifeDifficulty = fDifficultyValues[iLifeDifficulty];
	return;
}

void LifeMeterBar::FillForHowToPlay( int NumW2s, int NumMisses )
{
	m_iProgressiveLifebar = 0;  // disable progressive lifebar

	float AmountForW2	= NumW2s * m_fLifeDifficulty * 0.008f;
	float AmountForMiss	= NumMisses / m_fLifeDifficulty * 0.08f;

	m_fLifePercentage = AmountForMiss - AmountForW2;
	CLAMP( m_fLifePercentage, 0.0f, 1.0f );
	AfterLifeChanged();
}

/*
 * (c) 2001-2004 Chris Danford
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
