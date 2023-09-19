#pragma once
#pragma warning(suppress : 26495)
#include "Offsets.hpp"

#include "Color.hpp"
#include "Handle.hpp"
#include "Trace.hpp"
#include "UtlMemory.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>


struct cmdalias_t {
	cmdalias_t *next;
	char name[32];
	char *value;
};

struct GameOverlayActivated_t {
	uint8_t m_bActive;
};

enum PaintMode_t {
	PAINT_UIPANELS = (1 << 0),
	PAINT_INGAMEPANELS = (1 << 1),
};


struct PaintPowerInfo_t {
	void *vtable;
	Vector m_SurfaceNormal;
	Vector m_ContactPoint;
	int m_PaintPowerType;
	CBaseHandle m_HandleToOther;
	int m_State;
	bool m_IsOnThinSurface;
};


enum {
	PORTAL_COND_TAUNTING = 0,
	PORTAL_COND_POINTING,
	PORTAL_COND_DROWNING,
	PORTAL_COND_DEATH_CRUSH,
	PORTAL_COND_DEATH_GIB,
	PORTAL_COND_LAST
};


class IMaterial;

struct CFontAmalgam {
	struct TFontRange {
		int lowRange;
		int highRange;
		void *pFont;
	};

	CUtlVector<TFontRange> m_Fonts;
	int m_iMaxWidth;
	int m_iMaxHeight;
};


struct FcpsTraceAdapter {
	void (*traceFunc)(const Ray_t &ray, CGameTrace *result, FcpsTraceAdapter *adapter);
	bool (*pointOutsideWorldFunc)(const Vector &test, FcpsTraceAdapter *adapter);
	ITraceFilter *traceFilter;
	unsigned mask;
};

class CPlayerState {
public:
	void *vtable;
	void *m_hTonemapController;
	QAngle v_angle;
};

typedef unsigned int HTOOLHANDLE;
enum {
	HTOOLHANDLE_INVALID = 0
};

class CBoneList {
public:
	uint16_t m_nBones : 15;
	Vector m_vecPos[128];
	Quaternion m_quatRot[128];
};

struct BaseEntityRecordingState_t {
	float m_flTime;
	const char *m_pModelName;
	int m_nOwner;
	int m_fEffects;
	bool m_bVisible : 1;
	bool m_bRecordFinalVisibleSample : 1;
	Vector m_vecRenderOrigin;
	QAngle m_vecRenderAngles;
	int m_nFollowEntity;
	int m_numEffects;
};

struct BaseAnimatingHighLevelRecordingState_t {
	bool m_bClearIkTargets;
	bool m_bIsRagdoll;
	bool m_bShouldCreateIkContext;
	int m_nNumPoseParams;
	float m_flCycle;
	float m_flPlaybackRate;
	float m_flCycleRate;
	int m_nFrameCount;
	float m_flPoseParameter[24];
	bool m_bInterpEffectActive;
};

struct BaseAnimatingRecordingState_t {
	BaseAnimatingHighLevelRecordingState_t m_highLevelState;

	int m_nSkin;
	int m_nBody;
	int m_nSequence;
	CBoneList *m_pBoneList;
};