/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>	// _my_scoreboard
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/localization.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/chat.h>	// _my_score2chat

#include "scoreboard.h"


CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CScoreboard::OnReset()
{
	m_Active = false;
}

void CScoreboard::OnRelease()
{
	m_Active = false;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
}

void CScoreboard::RenderGoals(float x, float y, float w)
{
// _my_scoreboard START
	float LinesNum = 3;
	float LineHeight = 36.0f;
	float FontSize = 24.0f;

	float h = 20 + LineHeight*LinesNum;
	y = y-h+10;
	float xL = x+10.0f;
	float xR = x+w-35;
	float xC = (xL+xR)/2;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.65f);
	RenderTools()->DrawRoundRectExt(x-10.f, y-10.f, w, h, 10.0f, CUI::CORNER_T);
	Graphics()->QuadsEnd();

	// init
	float tw = 0.0f;
	char aBuf[64];
	mem_zero(aBuf, sizeof(aBuf));

	// render server name
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	float FontSizeResize = FontSize;
	while(TextRender()->TextWidth(0, FontSizeResize, CurrentServerInfo.m_aName, -1) > w-20)
		--FontSizeResize;
	tw = TextRender()->TextWidth(0, FontSizeResize, CurrentServerInfo.m_aName, -1);
	TextRender()->TextColor(0.8f,0.8f,0.8f,1);
	TextRender()->Text(0, xC-tw/2, y+(FontSize-FontSizeResize)/2, FontSizeResize, CurrentServerInfo.m_aName, -1);

	y += LineHeight;

	// render goals
	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		// render score limit
		str_format(aBuf, sizeof(aBuf), "%s: ", Localize("Score limit"));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->Text(0, xL, y, FontSize, aBuf, -1);
		if (m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit == 0) {
			str_format(aBuf, sizeof(aBuf), "-");
		} else {
			TextRender()->TextColor(0.5f,1,0.5f,1);
			str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit);
		}
		TextRender()->Text(0, xL+tw, y, FontSize, aBuf, -1);

		// render time limit
		str_format(aBuf, sizeof(aBuf), "%s: ", Localize("Time limit"));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->TextColor(0.8f,0.8f,0.8f,1);
		TextRender()->Text(0, xC-70, y, FontSize, aBuf, -1);
		if (m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit == 0) {
			str_format(aBuf, sizeof(aBuf), "-");
			TextRender()->Text(0, xC-70+tw, y, FontSize, aBuf, -1);
		} else {
			str_format(aBuf, sizeof(aBuf), "%d %s", m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit, Localize("min"));
			TextRender()->Text(0, xC-70+tw, y, FontSize, aBuf, -1);
			TextRender()->TextColor(0.5f,1,0.5f,1);
			str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit);
			TextRender()->Text(0, xC-70+tw, y, FontSize, aBuf, -1);
		}

		// render rounds
		if (m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum == 0) {
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Round"), m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent);
		} else {
			str_format(aBuf, sizeof(aBuf), "%s: %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent, m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum);
		}
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->TextColor(0.8f,0.8f,0.8f,1);
		TextRender()->Text(0, xR-tw, y, FontSize, aBuf, -1);
	}

	y += LineHeight;

	// fetch server info
	if(m_pClient->m_Snap.m_pLocalInfo)
	{
		// count players for server info-box
		int NumPlayers = 0;
		for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
		{
			IClient::CSnapItem Item;
			Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

			if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
			{
				NumPlayers++;
			}
		}

		// render game type
		str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Game type"), CurrentServerInfo.m_aGameType);
		TextRender()->TextColor(0.8f,0.8f,0.8f,1);
		TextRender()->Text(0, xL, y, FontSize, aBuf, -1);

		// render players
		str_format(aBuf, sizeof(aBuf), "%s: ", Localize("Players"));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->Text(0, xC-70, y, FontSize, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%d/%d", m_pClient->m_Snap.m_NumPlayers, CurrentServerInfo.m_MaxPlayers);
		TextRender()->Text(0, xC-70+tw, y, FontSize, aBuf, -1);
		TextRender()->TextColor(0.5f,1,0.5f,1);
		str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_NumPlayers);
		TextRender()->Text(0, xC-70+tw, y, FontSize, aBuf, -1);
		TextRender()->TextColor(0.8f,0.8f,0.8f,1);

		// render map
		FontSizeResize = FontSize;
		str_format(aBuf, sizeof(aBuf), "%s: ", Localize("Map"));
		float tag_tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		while(TextRender()->TextWidth(0, FontSizeResize, CurrentServerInfo.m_aMap, -1) > 200-tag_tw && FontSizeResize > 16)
			--FontSizeResize;
		tw = TextRender()->TextWidth(0, FontSizeResize, CurrentServerInfo.m_aMap, -1);
		if (tw > 200.0f-tag_tw) {
			tw = 200.0f-tag_tw;
		}
		TextRender()->Text(0, xR-tw-tag_tw, y, FontSize, aBuf, -1);
		TextRender()->Text(0, xR-tw, y+(FontSize-FontSizeResize)/2, FontSizeResize, CurrentServerInfo.m_aMap, -1);
	}
	TextRender()->TextColor(1,1,1,1);
// _my_scoreboard END
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 140.0f;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// Headline
	y += 10.0f;
	TextRender()->Text(0, x+10.0f, y, 28.0f, Localize("Spectators"), w-20.0f);

	// spectator names
	y += 30.0f;
	char aBuffer[1024*4];
	aBuffer[0] = 0;
	bool Multiple = false;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			str_append(aBuffer, ", ", sizeof(aBuffer));
		str_append(aBuffer, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, sizeof(aBuffer));
		Multiple = true;
	}
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x+10.0f, y, 22.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = w-20.0f;
	Cursor.m_MaxLines = 4;
	TextRender()->TextEx(&Cursor, aBuffer, -1);
}

// _my_score2chat BEGIN
void CScoreboard::ScoreToChat()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
		{
			char aBuf[128];
			mem_zero(aBuf, sizeof(aBuf));

			int Score1 = m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed;
			int Score2 = m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue;

			str_format(aBuf, sizeof(aBuf), Localize("Game over with score  %d : %d"), Score1, Score2);
			m_pClient->m_pChat->AddLine(-1,0,aBuf);
		}
	}
}
// _my_score2chat END

// _my_scoreboard START
int CScoreboard::ScoreboardNumPlayers (int Team)
{
//	return m_pClient->m_Snap.m_aTeamSize[Team];
	int NumPlayers = 0;
	for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
			if(pInfo->m_Team == Team)
			{
				if(++NumPlayers == MAX_CLIENTS)
					break;
			}
		}
	}
	return NumPlayers;
}

float CScoreboard::ScoreboardHeight (int Team)
{
	float LineHeight = 36.0f;

	// find players
	int NumPlayers = ScoreboardNumPlayers(Team);
	if (Team == TEAM_SPECTATORS && NumPlayers < 2) { NumPlayers = 2; }
	else if (NumPlayers < 3) { NumPlayers = 3; }

	// title + headline + player_scores + my_add
	return (54 + 29 + LineHeight * NumPlayers + 30);
}
// _my_scoreboard END

float CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, float h)	// _my_scoreboard
{
// _my_scoreboard BEGIN
	//if(Team == TEAM_SPECTATORS)
		//return;

	if (h == 0) h = ScoreboardHeight(Team);
	float res_y = y + h;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.65f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// render title
	float TitleFontsize = 40.0f;
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	}
	TextRender()->TextColor(0.8f,0.8f,0.8f,1.0f);
	TextRender()->Text(0, x+20.0f, y, TitleFontsize, pTitle, -1);
	float tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1);

	char aBuf[128] = {0};

	// render players num
	TextRender()->TextColor(1.0f,1.0f,1.0f,1.0f);
	str_format(aBuf, sizeof(aBuf), "(%d)", ScoreboardNumPlayers(Team));
	TextRender()->Text(0, x+30+tw, y+5, 28, aBuf, -1);

	// render team score
	if(Team != TEAM_SPECTATORS) {
	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_Snap.m_pGameDataObj)
		{
			int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	else
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW &&
			m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID])
		{
			int Score = m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
		else if(m_pClient->m_Snap.m_pLocalInfo)
		{
			int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
	TextRender()->TextColor(1.0f,1.0f,1.0f,1.0f);
	TextRender()->Text(0, x+w-tw-20.0f, y, TitleFontsize, aBuf, -1);
	}

	// calculate measurements
	x += 10.0f;
	float LineHeight = 36.0f;
	float TeeSizeMod = 0.6f;
	float Spacing = 0.0f;

	float ScoreOffset = x+10.0f, ScoreLength = 60.0f;
	float TeeOffset = ScoreOffset+ScoreLength, TeeLength = 60*TeeSizeMod;
	float NameOffset = TeeOffset+TeeLength, NameLength = 300.0f-TeeLength;
	float PingOffset = x+610.0f, PingLength = 65.0f;
	float CountryOffset = PingOffset-(LineHeight-Spacing-TeeSizeMod*5.0f)*2.0f, CountryLength = (LineHeight-Spacing-TeeSizeMod*5.0f)*2.0f;
	float ClanOffset = x+370.0f, ClanLength = 230.0f-CountryLength;

	// render headlines
	TextRender()->TextColor(0.5f,0.5f,0.5f,1.0f);
	y += 50.0f;
	float HeadlineFontsize = 22.0f;

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);
	if(Team != TEAM_SPECTATORS)
	TextRender()->Text(0, ScoreOffset+ScoreLength-tw, y, HeadlineFontsize, Localize("Score"), -1);

	TextRender()->Text(0, NameOffset, y, HeadlineFontsize, Localize("Name"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, ClanOffset+ClanLength/2-tw/2, y, HeadlineFontsize, Localize("Clan"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->Text(0, PingOffset+PingLength-tw, y, HeadlineFontsize, Localize("Ping"), -1);

	// render player entries
	TextRender()->TextColor(1.0f,1.0f,1.0f,1.0f);
	y += HeadlineFontsize*2.0f;
	float FontSize = 24.0f;
	CTextCursor Cursor;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// make sure that we render the correct team
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		// background so it's easy to find the local player or the followed one in spectator mode
		if(pInfo->m_Local || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(x, y, w-20.0f, LineHeight, 10.0f);
			Graphics()->QuadsEnd();
		}

		// score
		if(Team != TEAM_SPECTATORS) {
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -999, 999));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->SetCursor(&Cursor, ScoreOffset+ScoreLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = ScoreLength;
		TextRender()->TextEx(&Cursor, aBuf, -1);
		}

		// flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientID))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(pInfo->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(TeeOffset+0.0f, y-5.0f-Spacing/2.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// avatar
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
		TeeInfo.m_Size *= TeeSizeMod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+LineHeight/2));

		// name
		TextRender()->SetCursor(&Cursor, NameOffset, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = NameLength;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);

		// clan
		tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
		TextRender()->SetCursor(&Cursor, ClanOffset+ClanLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = ClanLength;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

		// country flag
		Graphics()->TextureSet(m_pClient->m_pCountryFlags->Get(m_pClient->m_aClients[pInfo->m_ClientID].m_Country)->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
		IGraphics::CQuadItem QuadItem(CountryOffset, y+(Spacing+TeeSizeMod*5.0f)/2.0f, CountryLength, LineHeight-Spacing-TeeSizeMod*5.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		// ping
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, 0, 1000));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->TextColor(0.5f,0.5f,0.5f,1.0f);
		TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = PingLength;
		TextRender()->TextEx(&Cursor, aBuf, -1);
		TextRender()->TextColor(1.0f,1.0f,1.0f,1.0f);

		y += LineHeight+Spacing;
	}
	return res_y;
// _my_scoreboard END
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.65f);	// _my_scoreboard
	RenderTools()->DrawRoundRectExt(x, 0.0f, 180.0f, 50.0f, 10.0f, CUI::CORNER_B);	// _my_scoreboard
	Graphics()->QuadsEnd();

	//draw the red dot
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(x+20, 15.0f, 20.0f, 20.0f, 10.0f);
	Graphics()->QuadsEnd();

	//draw the text
	char aBuf[64];
	int Seconds = m_pClient->DemoRecorder()->Length();
	str_format(aBuf, sizeof(aBuf), Localize("REC %3d:%02d"), Seconds/60, Seconds%60);
	TextRender()->Text(0, x+50.0f, 10.0f, 20.0f, aBuf, -1);
}

void CScoreboard::OnRender()
{
	if(!Active())
		return;

	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();


	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 700.0f;
	float my_y = 0;	// _my_scoreboard

	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS))
			my_y = RenderScoreboard(Width/2-w/2, 150.0f, w, 0, 0);	// _my_scoreboard
		else
		{
			const char *pRedClanName = GetClanName(TEAM_RED);
			const char *pBlueClanName = GetClanName(TEAM_BLUE);

			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && m_pClient->m_Snap.m_pGameDataObj)
			{
				char aText[256];
				str_copy(aText, Localize("Draw!"), sizeof(aText));

				if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue)
				{
					if(pRedClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pRedClanName);
					else
						str_copy(aText, Localize("Red team wins!"), sizeof(aText));
				}
				else if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed)
				{
					if(pBlueClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pBlueClanName);
					else
						str_copy(aText, Localize("Blue team wins!"), sizeof(aText));
				}

				float w = TextRender()->TextWidth(0, 86.0f, aText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, aText, -1);
			}

// _my_scoreboard START
			int h1 = ScoreboardHeight(TEAM_RED);
			int h2 = ScoreboardHeight(TEAM_BLUE);
			if (h1 < h2) { h1 = h2; }
			float my_y1 = RenderScoreboard(Width/2-w-5.0f, 150.0f, w, TEAM_RED, pRedClanName ? pRedClanName : Localize("Red team"), h1);
			float my_y2 = RenderScoreboard(Width/2+5.0f, 150.0f, w, TEAM_BLUE, pBlueClanName ? pBlueClanName : Localize("Blue team"), h1);
			if (my_y1 >= my_y2) { my_y = my_y1; } else { my_y = my_y2; }
// _my_scoreboard END
		}
	}

	RenderGoals(Width/2-w/2, Height, w);	// _my_scoreboard
	RenderScoreboard(Width/2-w/2, my_y+25, w, TEAM_SPECTATORS, Localize("Spectators"));	// _my_scoreboard
	RenderRecordingNotification((Width/7)*4);
}

bool CScoreboard::Active()
{
	// if we activly wanna look on the scoreboard
	if(m_Active)
		return true;

	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead
		if(!m_pClient->m_Snap.m_pLocalCharacter)
			return true;
	}

	// if the game is over
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
		return true;

	return false;
}

const char *CScoreboard::GetClanName(int Team)
{
	int ClanPlayers = 0;
	const char *pClanName = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = m_pClient->m_aClients[pInfo->m_ClientID].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return 0;
		}
	}

	if(ClanPlayers > 1 && pClanName[0])
		return pClanName;
	else
		return 0;
}
