/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SCOREBOARD_H
#define GAME_CLIENT_COMPONENTS_SCOREBOARD_H
#include <game/client/component.h>

class CScoreboard : public CComponent
{
	void RenderGoals(float x, float y, float w);
	void RenderSpectators(float x, float y, float w);
	int ScoreboardNumPlayers(int Team);	// _my_scoreboard
	float ScoreboardHeight(int Team);	// _my_scoreboard
	float RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, float h = 0);	// _my_scoreboard
	void RenderRecordingNotification(float x);

	static void ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData);

	const char *GetClanName(int Team);

	bool m_Active;

public:
	CScoreboard();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnRelease();

	void ScoreToChat();	// _my_score2chat

	bool Active();
};

#endif
