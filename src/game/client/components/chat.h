/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CHAT_H
#define GAME_CLIENT_COMPONENTS_CHAT_H
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>

class CChat : public CComponent
{
	CLineInput m_Input;

	enum
	{
		MAX_LINES = 25,
	};

	struct CLine
	{
		int64 m_Time;
		float m_YOffset[2];
		float m_YOffset2;
		float m_Blend;
		int m_ClientID;
		int m_Team;
		int m_NameColor;
		char m_aName[64];
		char m_aText[512];
		bool m_Highlighted;
	};

	CLine m_aLines[MAX_LINES];
	int m_CurrentLine;

	// chat
	enum
	{
		MODE_NONE=0,
		MODE_ALL,
		MODE_TEAM,
	};

	int m_Mode;
	bool m_Show;
	bool m_InputUpdate;
	int m_ChatStringOffset;
	int m_OldChatStringLength;
	int m_CompletionChosen;
	char m_aCompletionBuffer[256];
	int m_PlaceholderOffset;
	int m_PlaceholderLength;
	char *m_pHistoryEntry;
	float m_HeightLimit;
	float m_Blend_Box[2];
	int m_LastBlendBox;
	TStaticRingBuffer<char, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;

	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);

public:
	CChat();

	bool IsActive() const { return m_Mode != MODE_NONE; }

	void AddLine(int ClientID, int Team, const char *pLine);

	void EnableMode(int Team);

	void Say(int Team, const char *pLine);

	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);
};
#endif
