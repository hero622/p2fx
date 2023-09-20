#pragma once

#define BONE_USED_MASK 0x0007FF00
#define BONE_USED_BY_ANYTHING 0x0007FF00
#define BONE_USED_BY_HITBOX 0x00000100
#define BONE_USED_BY_ATTACHMENT 0x00000200
#define BONE_USED_BY_VERTEX_MASK 0x0003FC00
#define BONE_USED_BY_VERTEX_LOD0 0x00000400
#define BONE_USED_BY_VERTEX_LOD1 0x00000800
#define BONE_USED_BY_VERTEX_LOD2 0x00001000
#define BONE_USED_BY_VERTEX_LOD3 0x00002000
#define BONE_USED_BY_VERTEX_LOD4 0x00004000
#define BONE_USED_BY_VERTEX_LOD5 0x00008000
#define BONE_USED_BY_VERTEX_LOD6 0x00010000
#define BONE_USED_BY_VERTEX_LOD7 0x00020000
#define BONE_USED_BY_BONE_MERGE 0x00040000

struct mstudiobone_t {
	int sznameindex;
	int parent;
	int bonecontroller[6];
	Vector pos;
	Quaternion quat;
	Vector rot; // this is not a vector but its basically the same
	Vector posscale;
	Vector rotscale;
	matrix3x4_t poseToBone;
	Quaternion qAlignment;
	int flags;
	int proctype;
	int procindex;
	mutable int physicsbone;
	int surfacepropidx;
	int contents;
	int surfacepropLookup;
	int unused[7];
};

struct studiohdr_t {
	int id;
	int version;
	long checksum;
	char name[64];
	int length;
	Vector eyeposition;
	Vector illumposition;
	Vector hull_min;
	Vector hull_max;
	Vector view_bbmin;
	Vector view_bbmax;
	int flags;
	int numbones;
	int boneindex;
	inline mstudiobone_t *pBone(int i) const {
		return (mstudiobone_t *)(((byte *)this) + boneindex) + i;
	}
	int numbonecontrollers;
	int bonecontrollerindex;
	int numhitboxsets;
	int hitboxsetindex;
	int numlocalanim;
	int localanimindex;
	int numlocalseq;
	int localseqindex;
	mutable int activitylistversion;
	mutable int eventsindexed;
	int numtextures;
	int textureindex;
	int numcdtextures;
	int cdtextureindex;
	int numskinref;
	int numskinfamilies;
	int skinindex;
	int numbodyparts;
	int bodypartindex;
	int numlocalattachments;
	int localattachmentindex;
	int numlocalnodes;
	int localnodeindex;
	int localnodenameindex;
	int numflexdesc;
	int flexdescindex;
	int numflexcontrollers;
	int flexcontrollerindex;
	int numflexrules;
	int flexruleindex;
	int numikchains;
	int ikchainindex;
	int nummouths;
	int mouthindex;
	int numlocalposeparameters;
	int localposeparamindex;
	int surfacepropindex;
	int keyvalueindex;
	int keyvaluesize;
	int numlocalikautoplaylocks;
	int localikautoplaylockindex;
	float mass;
	int contents;
	int numincludemodels;
	int includemodelindex;
	mutable void *virtualModel;
	int szanimblocknameindex;
	int numanimblocks;
	int animblockindex;
	mutable void *animblockModel;
	int bonetablebynameindex;
	void *pVertexBase;
	void *pIndexBase;
	byte constdirectionallightdot;
	byte rootLOD;
	byte numAllowedRootLODs;
	byte unused[1];
	int unused4;
	int numflexcontrollerui;
	int flexcontrolleruiindex;
	float flVertAnimFixedPointScale;
	mutable int surfacepropLookup;
	int studiohdr2index;
	int unused2[1];
};

class CStudioHdr {
public:
	mutable const studiohdr_t *m_pStudioHdr;
	inline int numbones(void) const { return m_pStudioHdr->numbones; };
	inline mstudiobone_t *pBone(int i) const { return m_pStudioHdr->pBone(i); };
};