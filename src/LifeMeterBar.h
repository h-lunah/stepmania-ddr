#ifndef LIFEMETERBAR_H
#define LIFEMETERBAR_H

#include "LifeMeter.h"
#include "Sprite.h"
#include "AutoActor.h"
#include "Quad.h"
#include "ThemeMetric.h"
class StreamDisplay;

/** @brief The player's life represented as a bar. */
class LifeMeterBar : public LifeMeter
{
public:
	LifeMeterBar();
	~LifeMeterBar();

	virtual void Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats );

	virtual void Update( float fDeltaTime );

	virtual void ChangeLife( TapNoteScore score );
	virtual void ChangeLife( HoldNoteScore score, TapNoteScore tscore  );
	virtual void ChangeLife( float fDeltaLifePercent );
	virtual void SetLife(float value);
	virtual void HandleTapScoreNone();
	virtual void AfterLifeChanged();
	virtual bool IsInDanger() const;
	virtual bool IsHot() const;
	virtual bool IsFailing() const;
	virtual float GetLife() const { return m_fLifePercentage; }

	void UpdateNonstopLifebar();
	void FillForHowToPlay(int NumT2s, int NumMisses);
	// this function is solely for HowToPlay

private:
	ThemeMetric<float> DANGER_THRESHOLD;
	ThemeMetric<float> INITIAL_VALUE;
	ThemeMetric<float> HOT_VALUE;
	ThemeMetric<float> LIFE_MULTIPLIER;
	ThemeMetric<bool> FORCE_LIFE_DIFFICULTY_ON_EXTRA_STAGE;
	ThemeMetric<TapNoteScore>   MIN_STAY_ALIVE;
	ThemeMetric<float>	EXTRA_STAGE_LIFE_DIFFICULTY;

	ThemeMetric1D<float> m_fLifePercentChange;

	AutoActor		m_sprUnder;
	AutoActor		m_sprDanger;
	StreamDisplay*	m_pStream;
	AutoActor		m_sprOver;

	float		m_fLifePercentage;

	float		m_fPassingAlpha;
	float		m_fHotAlpha;

	bool		m_bMercifulBeginnerInEffect;
	float		m_fBaseLifeDifficulty;
	float		m_fLifeDifficulty;		// essentially same as pref

	int			m_iProgressiveLifebar;		// cached from prefs
	/** @brief The current number of progressive miss rankings. */
	int			m_iMissCombo;
	/** @brief The combo needed before the life bar starts to fill up after a Player failed. */
	int			m_iComboToRegainLife;
	/** @brief Current Flare Gauge active in Floating Flare mode. */
	int 		currFloatingFlareIndex = 9;

    /** @brief Penalty values for Marvelous judgments */
	float 		FlareJudgmentsW1[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	/** @brief Penalty values for Perfect judgments */
	float 		FlareJudgmentsW2[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, -0.01f};
	/** @brief Penalty values for Great judgments */
	float 		FlareJudgmentsW3[10] = {-0.002f, -0.0029f, -0.0038f, -0.0056f, -0.0074f, -0.0092f, -0.0128f, -0.0164f, -0.02f, -0.02f};
	/** @brief Penalty values for Good judgments */
	float 		FlareJudgmentsW4[10] = {-0.01f, -0.0145f, -0.019f, -0.028f, -0.038f, -0.045f, -0.064f, -0.082f, -0.1f, -0.1f};
	/** @brief Penalty values for misses */
	float 		FlareJudgmentsMiss[10] = {-0.10f, -0.11f, -0.12f, -0.14f, -0.16f, -0.18f, -0.22f, -0.26f, -0.3f, -0.3f};

	/** @brief Penalty values for OKs */
	float 		FlareJudgmentsHeld[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	/** @brief Penalty values for missed holds, not used as the Miss penalty applies here. */
	float 		FlareJudgmentsMissed[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	/** @brief Penalty values for NGs */
	float 		FlareJudgmentsLetGo[10] = {-0.10f, -0.11f, -0.12f, -0.14f, -0.16f, -0.18f, -0.22f, -0.26f, -0.3f, -0.3f};
};

#endif

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
