/* Actor - Base class for all objects that appear on the screen. */

#ifndef ACTOR_H
#define ACTOR_H

#include "RageTypes.h"
#include "RageUtil_AutoPtr.h"
#include "ActorCommands.h"
#include <map>
struct XNode;
struct lua_State;
class LuaReference;
class LuaClass;
#include "MessageManager.h"


#define DRAW_ORDER_BEFORE_EVERYTHING	-200
#define DRAW_ORDER_UNDERLAY				-100
// normal screen elements go here
#define DRAW_ORDER_OVERLAY				+100
#define DRAW_ORDER_TRANSITIONS			+110
#define DRAW_ORDER_AFTER_EVERYTHING		+200


class Actor : public IMessageSubscriber
{
public:
	Actor();
	Actor( const Actor &cpy );
	virtual ~Actor();
	virtual Actor *Copy() const;
	void UnsubcribeAndClearCommands();
	virtual void Reset();
	void LoadFromNode( const CString& sDir, const XNode* pNode );

	static void SetBGMTime( float fTime, float fBeat );
	static void SetBGMLight( int iLightNumber, float fCabinetLights );

	enum TweenType { 
		TWEEN_LINEAR, 
		TWEEN_ACCELERATE, 
		TWEEN_DECELERATE, 
		TWEEN_BOUNCE_BEGIN, 
		TWEEN_BOUNCE_END,
		TWEEN_SPRING,
	};
	enum Effect { no_effect, effect_lua,
				diffuse_blink,	diffuse_shift,	diffuse_ramp,
				glow_blink,		glow_shift,
				rainbow,
				wag,	bounce,		bob,	pulse,
				spin,	vibrate
				};

	struct TweenState
	{
		// start and end position for tweening
		RageVector3 pos;
		RageVector3 rotation;
		RageVector4 quat;
		RageVector3 scale;
		float		fSkewX;
		RectF		crop;	// 0 = no cropping, 1 = fully cropped
		RectF		fade;	// 0 = no fade
		RageColor   diffuse[4];
		RageColor   glow;
		float		aux;
		CString sCommandName;	// command to execute when this TweenState goes into effect

		void Init();
		static void MakeWeightedAverage( TweenState& average_out, const TweenState& ts1, const TweenState& ts2, float fPercentBetween );
	};

	enum EffectClock
	{
		CLOCK_TIMER,
		CLOCK_BGM_TIME,
		CLOCK_BGM_BEAT,
		CLOCK_LIGHT_1 = 1000,
		CLOCK_LIGHT_LAST = 1100,
		NUM_CLOCKS
	};

	void Draw();						// calls, NeedsDraw, BeginDraw, DrawPrimitives, EndDraw
	virtual bool EarlyAbortDraw() { return false; }	// return true to early abort drawing of this Actor
	virtual void BeginDraw();			// pushes transform onto world matrix stack
	virtual void SetGlobalRenderStates();		// Actor should call this at beginning of their DrawPrimitives()
	virtual void SetTextureRenderStates();		// Actor should call this after setting a texture
	virtual void DrawPrimitives() {};	// Derivitives should override
	virtual void EndDraw();				// pops transform from world matrix stack
	
	// TODO: make Update non virtual and change all classes to override UpdateInternal 
	// instead.
	bool IsFirstUpdate() const;
	virtual void Update( float fDeltaTime );		// this can short circuit UpdateInternal
	virtual void UpdateInternal( float fDeltaTime );	// override this
	void UpdateTweening( float fDeltaTime );
	void CopyTweening( const Actor &from );

	const CString &GetName() const		{ return m_sName; }
	virtual void SetName( const CString &sName ) { m_sName = sName; }

	float GetX() const				{ return m_current.pos.x; };
	float GetY() const				{ return m_current.pos.y; };
	float GetZ() const				{ return m_current.pos.z; };
	float GetDestX()				{ return DestTweenState().pos.x; };
	float GetDestY()				{ return DestTweenState().pos.y; };
	float GetDestZ()				{ return DestTweenState().pos.z; };
	void  SetX( float x )			{ DestTweenState().pos.x = x; };
	void  SetY( float y )			{ DestTweenState().pos.y = y; };
	void  SetZ( float z )			{ DestTweenState().pos.z = z; };
	void  SetXY( float x, float y )	{ DestTweenState().pos.x = x; DestTweenState().pos.y = y; };
	void  AddX( float x )			{ SetX( GetDestX()+x ); }
	void  AddY( float y )			{ SetY( GetDestY()+y ); }
	void  AddZ( float z )			{ SetZ( GetDestZ()+z ); }

	// height and width vary depending on zoom
	float GetUnzoomedWidth()		{ return m_size.x; }
	float GetUnzoomedHeight()		{ return m_size.y; }
	float GetZoomedWidth()			{ return m_size.x * m_baseScale.x * DestTweenState().scale.x; }
	float GetZoomedHeight()			{ return m_size.y * m_baseScale.y * DestTweenState().scale.y; }
	void  SetWidth( float width )	{ m_size.x = width; }
	void  SetHeight( float height )	{ m_size.y = height; }

	// Base
	float GetBaseZoomX()				{ return m_baseScale.x;	}
	void  SetBaseZoomX( float zoom )	{ m_baseScale.x = zoom;	}
	void  SetBaseZoomY( float zoom )	{ m_baseScale.y = zoom; }
	void  SetBaseZoomZ( float zoom )	{ m_baseScale.z = zoom; }
	void  SetBaseZoom( const RageVector3 &zoom ) { m_baseScale = zoom; }
	void  SetBaseRotationX( float rot )	{ m_baseRotation.x = rot; }
	void  SetBaseRotationY( float rot )	{ m_baseRotation.y = rot; }
	void  SetBaseRotationZ( float rot )	{ m_baseRotation.z = rot; }
	void  SetBaseRotation( const RageVector3 &rot )	{ m_baseRotation = rot; }
	virtual void  SetBaseAlpha( float fAlpha )	{ m_fBaseAlpha = fAlpha; }


	float GetZoom()					{ return DestTweenState().scale.x; }	// not accurate in some cases
	float GetZoomX()				{ return DestTweenState().scale.x; }
	float GetZoomY()				{ return DestTweenState().scale.y; }
	float GetZoomZ()				{ return DestTweenState().scale.z; }
	void  SetZoom( float zoom )		{ DestTweenState().scale.x = zoom;	DestTweenState().scale.y = zoom; DestTweenState().scale.z = zoom; }
	void  SetZoomX( float zoom )	{ DestTweenState().scale.x = zoom;	}
	void  SetZoomY( float zoom )	{ DestTweenState().scale.y = zoom; }
	void  SetZoomZ( float zoom )	{ DestTweenState().scale.z = zoom; }
	void  ZoomTo( float fX, float fY )	{ ZoomToWidth(fX); ZoomToHeight(fY); }
	void  ZoomToWidth( float zoom )	{ SetZoomX( zoom / GetUnzoomedWidth() ); }
	void  ZoomToHeight( float zoom ){ SetZoomY( zoom / GetUnzoomedHeight() ); }

	float GetRotationX()			{ return DestTweenState().rotation.x; }
	float GetRotationY()			{ return DestTweenState().rotation.y; }
	float GetRotationZ()			{ return DestTweenState().rotation.z; }
	void  SetRotationX( float rot )	{ DestTweenState().rotation.x = rot; }
	void  SetRotationY( float rot )	{ DestTweenState().rotation.y = rot; }
	void  SetRotationZ( float rot )	{ DestTweenState().rotation.z = rot; }
	void  AddRotationH( float rot );
	void  AddRotationP( float rot );
	void  AddRotationR( float rot );

	void SetSkewX( float fAmount )	{ DestTweenState().fSkewX = fAmount; }
	float GetSkewX( float fAmount )	{ return DestTweenState().fSkewX; }

	float GetCropLeft()					{ return DestTweenState().crop.left; }
	float GetCropTop()					{ return DestTweenState().crop.top;	}
	float GetCropRight()				{ return DestTweenState().crop.right;}
	float GetCropBottom()				{ return DestTweenState().crop.bottom;}
	void  SetCropLeft( float percent )	{ DestTweenState().crop.left = percent; }
	void  SetCropTop( float percent )	{ DestTweenState().crop.top = percent;	}
	void  SetCropRight( float percent )	{ DestTweenState().crop.right = percent;}
	void  SetCropBottom( float percent ){ DestTweenState().crop.bottom = percent;}

	void  SetFadeLeft( float percent )	{ DestTweenState().fade.left = percent; }
	void  SetFadeTop( float percent )	{ DestTweenState().fade.top = percent;	}
	void  SetFadeRight( float percent )	{ DestTweenState().fade.right = percent;}
	void  SetFadeBottom( float percent ){ DestTweenState().fade.bottom = percent;}

	void SetGlobalDiffuseColor( RageColor c );
	void SetGlobalX( float x );

	virtual void SetDiffuse( RageColor c ) { for(int i=0; i<4; i++) DestTweenState().diffuse[i] = c; };
	virtual void SetDiffuseAlpha( float f ) { for(int i = 0; i < 4; ++i) { RageColor c = GetDiffuses( i ); c.a = f; SetDiffuses( i, c ); } }
	void SetDiffuseColor( RageColor c );
	void SetDiffuses( int i, RageColor c )		{ DestTweenState().diffuse[i] = c; };
	void SetDiffuseUpperLeft( RageColor c )		{ DestTweenState().diffuse[0] = c; };
	void SetDiffuseUpperRight( RageColor c )	{ DestTweenState().diffuse[1] = c; };
	void SetDiffuseLowerLeft( RageColor c )		{ DestTweenState().diffuse[2] = c; };
	void SetDiffuseLowerRight( RageColor c )	{ DestTweenState().diffuse[3] = c; };
	void SetDiffuseTopEdge( RageColor c )		{ DestTweenState().diffuse[0] = DestTweenState().diffuse[1] = c; };
	void SetDiffuseRightEdge( RageColor c )		{ DestTweenState().diffuse[1] = DestTweenState().diffuse[3] = c; };
	void SetDiffuseBottomEdge( RageColor c )	{ DestTweenState().diffuse[2] = DestTweenState().diffuse[3] = c; };
	void SetDiffuseLeftEdge( RageColor c )		{ DestTweenState().diffuse[0] = DestTweenState().diffuse[2] = c; };
	RageColor GetDiffuse()						{ return DestTweenState().diffuse[0]; };
	RageColor GetDiffuses( int i )				{ return DestTweenState().diffuse[i]; };
	void SetGlow( RageColor c )					{ DestTweenState().glow = c; };
	RageColor GetGlow()							{ return DestTweenState().glow; };

	void SetAux( float f )						{ DestTweenState().aux = f; }
	float GetAux() const						{ return m_current.aux; }

	void BeginTweening( float time, TweenType tt = TWEEN_LINEAR );
	void StopTweening();
	void Sleep( float time );
	void QueueCommand( const CString& sCommandName );
	void QueueMessage( const CString& sMessageName );
	virtual void FinishTweening();
	virtual void HurryTweening( float factor );
	// Let ActorFrame and BGAnimation override
	virtual float GetTweenTimeLeft() const;	// Amount of time until all tweens have stopped
	TweenState& DestTweenState()	// where Actor will end when its tween finish
	{
		if( m_pTempState != NULL ) // effect_lua running
			return *m_pTempState;
		else if( m_Tweens.empty() )	// not tweening
			return m_current;
		else
			return m_Tweens.back()->state;
	}

	
	enum StretchType { fit_inside, cover };

	void ScaleToCover( const RectF &rect )		{ ScaleTo( rect, cover ); }
	void ScaleToFitInside( const RectF &rect )	{ ScaleTo( rect, fit_inside); };
	void ScaleTo( const RectF &rect, StretchType st );

	void StretchTo( const RectF &rect );


	//
	// Alignment settings.  These need to be virtual for BitmapText
	//
	enum HorizAlign { align_left, align_center, align_right };
	virtual void SetHorizAlign( HorizAlign ha ) { m_HorizAlign = ha; }
	virtual void SetHorizAlignString( const CString &s );	// convenience

	enum VertAlign { align_top, align_middle, align_bottom };
	virtual void SetVertAlign( VertAlign va ) { m_VertAlign = va; }
	virtual void SetVertAlignString( const CString &s );	// convenience


	//
	// effects
	//
	void SetEffectNone()						{ m_Effect = no_effect; }
	Effect GetEffect() const					{ return m_Effect; }
	float GetSecsIntoEffect() const				{ return m_fSecsIntoEffect; }
	float GetEffectDelta() const				{ return m_fEffectDelta; }

	void SetEffectColor1( RageColor c )			{ m_effectColor1 = c; }
	void SetEffectColor2( RageColor c )			{ m_effectColor2 = c; }
	void SetEffectPeriod( float fSecs )			{ m_fEffectPeriodSeconds = fSecs; } 
	void SetEffectDelay( float fTime )			{ m_fEffectDelay = fTime; }
	void SetEffectOffset( float fPercent )		{ m_fEffectOffset = fPercent; }
	void SetEffectClock( EffectClock c )		{ m_EffectClock = c; }
	void SetEffectClockString( const CString &s );	// convenience

	void SetEffectMagnitude( RageVector3 vec )	{ m_vEffectMagnitude = vec; }
	RageVector3 GetEffectMagnitude() const		{ return m_vEffectMagnitude; }

	void SetEffectLua( const CString &sCommand );
	void SetEffectDiffuseBlink( 
		float fEffectPeriodSeconds = 1.0f,
		RageColor c1 = RageColor(0.5f,0.5f,0.5f,1), 
		RageColor c2 = RageColor(1,1,1,1) );
	void SetEffectDiffuseShift( float fEffectPeriodSeconds = 1.f,
		RageColor c1 = RageColor(0,0,0,1), 
		RageColor c2 = RageColor(1,1,1,1) );
	void SetEffectDiffuseRamp( float fEffectPeriodSeconds = 1.f,
		RageColor c1 = RageColor(0,0,0,1), 
		RageColor c2 = RageColor(1,1,1,1) );
	void SetEffectGlowBlink( float fEffectPeriodSeconds = 1.f,
		RageColor c1 = RageColor(1,1,1,0.2f),
		RageColor c2 = RageColor(1,1,1,0.8f) );
	void SetEffectGlowShift( 
		float fEffectPeriodSeconds = 1.0f,
		RageColor c1 = RageColor(1,1,1,0.2f),
		RageColor c2 = RageColor(1,1,1,0.8f) );
	void SetEffectRainbow( 
		float fEffectPeriodSeconds = 2.0f );
	void SetEffectWag( 
		float fPeriod = 2.f, 
		RageVector3 vect = RageVector3(0,0,20) );
	void SetEffectBounce( 
		float fPeriod = 2.f, 
		RageVector3 vect = RageVector3(0,0,20) );
	void SetEffectBob( 
		float fPeriod = 2.f, 
		RageVector3 vect = RageVector3(0,0,20) );
	void SetEffectPulse( 
		float fPeriod = 2.f,
		float fMinZoom = 0.5f,
		float fMaxZoom = 1.f );
	void SetEffectSpin( 
		RageVector3 vect = RageVector3(0,0,180) );
	void SetEffectVibrate( 
		RageVector3 vect = RageVector3(10,10,10) );


	//
	// other properties
	//
	bool GetVisible() const				{ return m_bVisible; }
	bool GetHidden() const				{ return !m_bVisible; }
	void SetVisible( bool b )			{ m_bVisible = b; }
	void SetHidden( bool b )			{ m_bVisible = !b; }
	void SetShadowLength( float fLength );
	// TODO: Implement hibernate as a tween type?
	void SetHibernate( float fSecs )	{ m_fHibernateSecondsLeft = fSecs; }
	void SetDrawOrder( int iOrder )		{ m_iDrawOrder = iOrder; }
	int GetDrawOrder() const			{ return m_iDrawOrder; }

	virtual void EnableAnimation( bool b ) 		{ m_bIsAnimating = b; }	// Sprite needs to overload this
	void StartAnimating()		{ this->EnableAnimation(true); };
	void StopAnimating()		{ this->EnableAnimation(false); };


	//
	// render states
	//
	void SetBlendMode( BlendMode mode )			{ m_BlendMode = mode; } 
	void SetBlendModeString( const CString &s );	// convenience
	void SetTextureWrapping( bool b ) 			{ m_bTextureWrapping = b; } 
	void SetClearZBuffer( bool b ) 				{ m_bClearZBuffer = b; } 
	void SetUseZBuffer( bool b ) 				{ SetZTestMode(b?ZTEST_WRITE_ON_PASS:ZTEST_OFF); SetZWrite(b); } 
	virtual void SetZTestMode( ZTestMode mode ) { m_ZTestMode = mode; } 
	void SetZTestModeString( const CString &s );	// convenience
	virtual void SetZWrite( bool b ) 			{ m_bZWrite = b; } 
	void SetZBias( float f )					{ m_fZBias = f; }
	virtual void SetCullMode( CullMode mode ) 	{ m_CullMode = mode; } 
	void SetCullModeString( const CString &s );	// convenience

	//
	// Lua
	//
	virtual void PushSelf( lua_State *L );

	//
	// Commands
	//
	void AddCommand( const CString &sCmdName, apActorCommands apac );
	bool HasCommand( const CString &sCmdName );
	const apActorCommands& GetCommand( const CString &sCommandName ) const;
	virtual void PlayCommand( const CString &sCommandName, Actor *pParent = NULL );
	virtual void RunCommands( const LuaReference& cmds, Actor *pParent = NULL );
	void RunCommands( const apActorCommands& cmds, Actor *pParent = NULL ) { this->RunCommands( *cmds, pParent ); }	// convenience
	// If we're a leaf, then execute this command.
	virtual void RunCommandsOnLeaves( const LuaReference& cmds, Actor *pParent = NULL ) { RunCommands(cmds,pParent); }

	static float GetCommandsLengthSeconds( const LuaReference& cmds );
	static float GetCommandsLengthSeconds( const apActorCommands& cmds ) { return GetCommandsLengthSeconds( *cmds ); }	// convenience

	//
	// Messages
	//
	void SubscribeToMessage( Message message ); // will automatically unsubscribe
	void SubscribeToMessage( const CString &sMessageName ); // will automatically unsubscribe
	virtual void HandleMessage( const CString& sMessage );


	//
	// Animation
	//
	virtual int GetNumStates() const { return 1; }
	virtual void SetState( int iNewState ) {}
	virtual float GetAnimationLengthSeconds() const { return 0; }
	virtual void SetSecondsIntoAnimation( float fSeconds ) {}
	virtual void SetUpdateRate( float fRate ) {}

	HiddenPtr<LuaClass> m_pLuaInstance;

protected:
	CString m_sName;

	struct TweenInfo
	{
		// counters for tweening
		TweenType	m_TweenType;
		float		m_fTimeLeftInTween;	// how far into the tween are we?
		float		m_fTweenTime;		// seconds between Start and End positions/zooms
	};


	RageVector3	m_baseRotation;
	RageVector3	m_baseScale;
	float m_fBaseAlpha;


	RageVector2	m_size;
	TweenState	m_current;
	TweenState	m_start;
	struct TweenStateAndInfo
	{
		TweenState state;
		TweenInfo info;
	};
	vector<TweenStateAndInfo *>	m_Tweens;

	//
	// Temporary variables that are filled just before drawing
	//
	TweenState *m_pTempState;

	bool	m_bFirstUpdate;

	//
	// Stuff for alignment
	//
	HorizAlign	m_HorizAlign;
	VertAlign	m_VertAlign;


	//
	// Stuff for effects
	//
	Effect m_Effect;
	CString m_sEffectCommand; // effect_lua
	float m_fSecsIntoEffect, m_fEffectDelta;
	float m_fEffectPeriodSeconds;
	float m_fEffectDelay;
	float m_fEffectOffset;
	EffectClock m_EffectClock;

	/* This can be used in lieu of the fDeltaTime parameter to Update() to
	 * follow the effect clock.  Actor::Update must be called first. */
	float GetEffectDeltaTime() const { return m_fEffectDelta; }

	RageColor   m_effectColor1;
	RageColor   m_effectColor2;
	RageVector3 m_vEffectMagnitude;


	//
	// other properties
	//
	bool	m_bVisible;
	float	m_fHibernateSecondsLeft;
	float	m_fShadowLength;	// 0 == no shadow
	bool	m_bIsAnimating;
	int		m_iDrawOrder;

	//
	// render states
	//
	bool		m_bTextureWrapping;
	BlendMode	m_BlendMode;
	bool		m_bClearZBuffer;
	ZTestMode	m_ZTestMode;
	bool		m_bZWrite;
	float		m_fZBias; // 0 = no bias; 1 = full bias
	CullMode	m_CullMode;

	//
	// global state
	//
	static float g_fCurrentBGMTime, g_fCurrentBGMBeat;

private:
	//
	// commands
	//
	map<CString, apActorCommands> m_mapNameToCommands;
	vector<CString> m_vsSubscribedTo;
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
