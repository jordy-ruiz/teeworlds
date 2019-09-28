#pragma once
#include <stdint.h>
#include <base/tl/array.h>
#include <base/tl/sorted_array.h>
#include <engine/graphics.h>
#include <engine/sound.h>
#include <game/duck_collision.h>
#include <game/client/component.h>
#include <generated/protocol.h>

#include "duck_js.h"

// Bridge between teeworlds and duktape

class CRenderTools;
class CGameClient;
class CCharacterCore;
class CTeeRenderInfo;

struct CMultiStackAllocator
{
	struct CStackBuffer
	{
		char* m_pBuffer;
		int m_Cursor;
	};

	enum {
		STACK_BUFFER_CAPACITY=1024*1024*10 // 10 KB
	};

	array<CStackBuffer> m_aStacks;
	int m_CurrentStack;

	CMultiStackAllocator();
	~CMultiStackAllocator();

	void* Alloc(int Size);
	void Clear();
};

struct CDuckBridge : public CComponent
{
	CDuckJs m_Js;

	struct DrawSpace
	{
		enum Enum
		{
			GAME=0,
			HUD,
			_COUNT
		};
	};

	struct CTeeDrawBodyAndFeetInfo
	{
		float m_Size;
		float m_Angle;
		float m_Pos[2]; // vec2
		bool m_IsWalking;
		bool m_IsGrounded;
		bool m_GotAirJump;
		int m_Emote;
	};

	struct CTeeDrawHand
	{
		float m_Size;
		float m_AngleDir;
		float m_AngleOff;
		float m_Pos[2]; // vec2
		float m_Offset[2]; // vec2
	};

	struct CTeeSkinInfo
	{
		int m_aTextures[NUM_SKINPARTS];
		float m_aColors[NUM_SKINPARTS][4];
	};

	struct CTextInfo
	{
		const char* m_pStr;
		float m_aColors[4];
		float m_FontSize;
		float m_aRect[4];
	};

	struct CRenderCmd
	{
		enum TypeEnum
		{
			SET_COLOR=0,
			SET_TEXTURE,
			SET_QUAD_SUBSET,
			SET_QUAD_ROTATION,
			SET_TEE_SKIN,
			SET_FREEFORM_VERTICES,
			DRAW_QUAD,
			DRAW_QUAD_CENTERED,
			DRAW_TEE_BODYANDFEET,
			DRAW_TEE_HAND,
			DRAW_FREEFORM,
			DRAW_TEXT,
		};

		int m_Type;

		union
		{
			float m_Color[4];
			float m_Quad[4]; // POD IGraphics::CQuadItem
			int m_TextureID;
			float m_QuadSubSet[4];
			float m_QuadRotation;
			float m_FreeformPos[2];

			struct {
				const float* m_pFreeformQuads;
				int m_FreeformQuadCount;
			};

			// TODO: this is kinda big...
			CTeeDrawBodyAndFeetInfo m_TeeBodyAndFeet;
			CTeeDrawHand m_TeeHand;
			CTeeSkinInfo m_TeeSkinInfo;
			CTextInfo m_Text;
		};
	};

	struct CRenderSpace
	{
		enum {
			FREEFORM_MAX_COUNT=256
		};

		float m_aWantColor[4];
		float m_aCurrentColor[4];
		float m_aWantQuadSubSet[4];
		float m_aCurrentQuadSubSet[4];
		int m_WantTextureID;
		int m_CurrentTextureID;
		float m_WantQuadRotation;
		float m_CurrentQuadRotation;
		CTeeSkinInfo m_CurrentTeeSkin;
		IGraphics::CFreeformItem m_aFreeformQuads[FREEFORM_MAX_COUNT];
		int m_FreeformQuadCount;

		CRenderSpace()
		{
			mem_zero(m_aWantColor, sizeof(m_aWantColor));
			mem_zero(m_aCurrentColor, sizeof(m_aCurrentColor));
			mem_zero(m_aWantQuadSubSet, sizeof(m_aWantQuadSubSet));
			mem_zero(m_aCurrentQuadSubSet, sizeof(m_aCurrentQuadSubSet));
			m_WantTextureID = -1; // clear by default
			m_CurrentTextureID = 0;
			m_WantQuadRotation = 0; // clear by default
			m_CurrentQuadRotation = -1;
			m_FreeformQuadCount = 0;

			for(int i = 0; i < NUM_SKINPARTS; i++)
			{
				m_CurrentTeeSkin.m_aTextures[i] = -1;
				m_CurrentTeeSkin.m_aColors[i][0] = 1.0f;
				m_CurrentTeeSkin.m_aColors[i][1] = 1.0f;
				m_CurrentTeeSkin.m_aColors[i][2] = 1.0f;
				m_CurrentTeeSkin.m_aColors[i][3] = 1.0f;
			}
		}
	};

	bool m_IsModLoaded;
	CMultiStackAllocator m_FrameAllocator; // holds data for a frame

	int m_CurrentDrawSpace;
	array<CRenderCmd> m_aRenderCmdList[DrawSpace::_COUNT];
	CRenderSpace m_aRenderSpace[DrawSpace::_COUNT];

	struct CTextureHashPair
	{
		uint32_t m_Hash;
		IGraphics::CTextureHandle m_Handle;
	};

	array<CTextureHashPair> m_aTextures;

	CDuckCollision m_Collision;

	struct CHudPartsShown
	{
		// TODO: make those bools?
		int m_Health;
		int m_Armor;
		int m_Ammo;
		int m_Time;
		int m_KillFeed;
		int m_Score;
		int m_Chat;
		int m_Scoreboard;

		CHudPartsShown() {
			m_Health = 1;
			m_Armor = 1;
			m_Ammo = 1;
			m_Time = 1;
			m_KillFeed = 1;
			m_Score = 1;
			m_Chat = 1;
			m_Scoreboard = 1;
		}
	};

	CHudPartsShown m_HudPartsShown;
	CMsgPacker m_CurrentPacket;
	int m_CurrentPacketFlags;

	struct CSkinPartName
	{
		char m_aName[24];
		int m_Type;
	};

	array<CSkinPartName> m_aSkinPartsToUnload;

	struct CWeaponCustomJs
	{
		int WeaponID;
		char aTexWeapon[64];
		char aTexCursor[64];
		float WeaponX;
		float WeaponY;
		float WeaponSizeX;
		float WeaponSizeY;
		float HandX;
		float HandY;
		float HandAngle;
		float Recoil;
	};

	struct CWeaponCustom
	{
		int WeaponID;
		IGraphics::CTextureHandle TexWeaponHandle;
		IGraphics::CTextureHandle TexCursorHandle;
		vec2 WeaponPos;
		vec2 WeaponSize;
		vec2 HandPos;
		float HandAngle;
		float Recoil;

		bool operator < (const CWeaponCustom& other) {
			return WeaponID < other.WeaponID;
		}

		bool operator == (const CWeaponCustom& other) {
			return WeaponID == other.WeaponID;
		}
	};

	sorted_array<CWeaponCustom> m_aWeapons;

	struct CSoundHashPair
	{
		uint32_t m_Hash;
		ISound::CSampleHandle m_Handle;
	};

	array<CSoundHashPair> m_aSounds;

	void DrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo, const CTeeSkinInfo& SkinInfo);
	void DrawTeeHand(const CTeeDrawHand& Hand, const CTeeSkinInfo& SkinInfo);

	CDuckBridge() : m_CurrentPacket(0, 0) {} // We have to do this, CMsgPacker can't be uninitialized apparently...

	void Init(CDuckJs* pDuckJs);
	void Reset();

	void QueueSetColor(const float* pColor);
	void QueueSetTexture(int TextureID);
	void QueueSetQuadSubSet(const float* pSubSet);
	void QueueSetQuadRotation(float Angle);
	void QueueSetTeeSkin(const CTeeSkinInfo& SkinInfo);
	void QueueSetFreeform(const IGraphics::CFreeformItem* pFreeform, int FreeformCount);
	void QueueDrawQuad(IGraphics::CQuadItem Quad);
	void QueueDrawQuadCentered(IGraphics::CQuadItem Quad);
	void QueueDrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo);
	void QueueDrawTeeHand(const CTeeDrawHand& Hand);
	void QueueDrawFreeform(vec2 Pos);
	void QueueDrawText(const char* pStr, float FontSize, float *pRect, float *pColors);

	void SetHudPartsShown(CHudPartsShown hps);

	bool LoadTexture(const char* pTexturePath, const char *pTextureName);
	IGraphics::CTextureHandle GetTextureFromName(const char* pTextureName);

	void PacketCreate(int NetID, int Flags);
	void PacketPackFloat(float f);
	void PacketPackInt(int i);
	void PacketPackString(const char* pStr, int SizeLimit);
	void SendPacket();

	void AddSkinPart(const char* pPart, const char* pName, IGraphics::CTextureHandle Handle);
	void AddWeapon(const CWeaponCustomJs& Wc);
	CWeaponCustom* FindWeapon(int WeaponID);

	void PlaySoundAt(const char* pSoundName, float x, float y);
	void PlaySoundGlobal(const char* pSoundName);
	void PlayMusic(const char* pSoundName);

	CUIRect GetUiScreenRect();

	// "entries"
	void RenderDrawSpace(DrawSpace::Enum Space);
	void CharacterCorePreTick(CCharacterCore** apCharCores);
	void CharacterCorePostTick(CCharacterCore** apCharCores);
	void Predict(CWorldCore *pWorld);
	void RenderPlayerWeapon(int WeaponID, vec2 Pos, float AttachAngle, float Angle, CTeeRenderInfo *pRenderInfo, float RecoilAlpha);
	void RenderWeaponCursor(int WeaponID, vec2 Pos);
	void RenderWeaponAmmo(int WeaponID, vec2 Pos);

	// mod installation
	bool IsModAlreadyInstalled(const SHA256_DIGEST* pModSha256);
	bool ExtractAndInstallModZipBuffer(const CGrowBuffer* pHttpZipData, const SHA256_DIGEST* pModSha256);
	bool ExtractAndInstallModCompressedBuffer(const void* pCompBuff, int CompBuffSize, const SHA256_DIGEST* pModSha256);
	bool LoadModFilesFromDisk(const SHA256_DIGEST* pModSha256);
	bool StartDuckModHttpDownload(const char* pModUrl, const SHA256_DIGEST* pModSha256);
	bool TryLoadInstalledDuckMod(const SHA256_DIGEST* pModSha256);
	bool InstallAndLoadDuckModFromZipBuffer(const void* pBuffer, int BufferSize, const SHA256_DIGEST* pModSha256);

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnRender();
	virtual void OnMessage(int Msg, void *pRawMsg);
	void OnSnapItem(int Msg, void *pRawMsg);
	virtual void OnStateChange(int NewState, int OldState);
	virtual bool OnInput(IInput::CEvent e);
	void OnModReset();
	void OnModUnload();

	inline bool IsLoaded() const { return m_Js.m_pDukContext != 0 && m_IsModLoaded; }

	friend class CDuckJs;
};
