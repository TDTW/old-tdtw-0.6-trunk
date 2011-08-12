/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/render.h>

#include "spectator.h"
#include "players.h"


void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active &&
		(pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK || pSelf->DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER))
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	((CSpectator *)pUserData)->Spectate(pResult->GetInteger(0));
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID + 1; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = 0; i < pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i++)
			{
				if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = i;
				GotNewSpectatorID = true;
				break;
			}
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = MAX_CLIENTS -1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID - 1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = MAX_CLIENTS - 1; i > pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i--)
			{
				if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
				break;
			}
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

CSpectator::CSpectator()
{
	OnReset();
}

void CSpectator::OnConsoleInit()
{
	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "i", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
}

bool CSpectator::OnMouseMove(float x, float y)
{
	if(!m_Active)
		return false;

	UI()->ConvertMouseMove(&x, &y);
	m_SelectorMouse += vec2(x,y);
	return true;
}

void CSpectator::OnRelease()
{
	OnReset();
}

bool CSpectator::InView(const CUIRect *r, float Width, float Height)
{
	if(m_SelectorMouse.x+Width >= r->x && 
	m_SelectorMouse.x+Width <= r->x+r->w &&
	m_SelectorMouse.y+Height >= r->y && 
	m_SelectorMouse.y+Height <= r->y+r->h)
		return true;
	return false;
}

void CSpectator::OnRender()
{
	if(!m_Active)
	{
		if(m_WasActive)
		{
			if(m_SelectedSpectatorID != NO_SELECTION)
				Spectate(m_SelectedSpectatorID);
			m_WasActive = false;
		}
		return;
	}
	
	if(m_Selected != m_pClient->m_Snap.m_SpecInfo.m_SpectatorID)
	{
		m_SelectedSpectatorID = m_Selected;
		Spectate(m_SelectedSpectatorID);
		Loading = true;
	}
	else 
		Loading = false;

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpectatorID = NO_SELECTION;
	
 	// draw background
	
	CUIRect Screen = *UI()->Screen(), TopMenu, BottomMenu;
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
	Screen.Margin(10.0f, &Screen);
	vec4 ms_ColorTabbarActive = vec4(0.0f, 0.0f, 0.0f, 0.40f);
	vec4 ms_ColorTabbarActive2 = vec4(1.0f, 1.0f, 1.0f, 0.40f);
	
	Screen.HSplitBottom(10*3.0f*Graphics()->ScreenAspect(), &Screen, 0);
	Screen.HMargin(21*3.0f*Graphics()->ScreenAspect(), &Screen);
	Screen.VMargin(10*3.0f*Graphics()->ScreenAspect(), &Screen);
	
	float Width = Screen.w/2-10, Height = Screen.h/2-10;
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -Width, Width);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -Height, Height);
	Width += 10*3.0f*Graphics()->ScreenAspect()+10;
	Height += 20*3.0f*Graphics()->ScreenAspect()+10;
	
	Screen.HSplitTop(Screen.h/2-20, &TopMenu, &BottomMenu);
	TopMenu.HSplitBottom(5, &TopMenu, 0);
	//TopMenu.VMargin(15*3.0f*Graphics()->ScreenAspect(), &TopMenu);
	
	// RenderTools()->DrawUIRect(&Screen, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	// RenderTools()->DrawUIRect(&TopMenu, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	//RenderTools()->DrawUIRect(&BottomMenu, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	{
		CUIRect Left, Middle, Right;
		CUIRect Button, Button2;
				
		int Temp = TopMenu.w/3;
		TopMenu.VSplitLeft(Temp, &Left, &TopMenu);
		TopMenu.VSplitRight(Temp, &Middle, &Right);
		Middle.HMargin(5, &Middle);
		Left.VMargin(5, &Left);
		Right.VMargin(5, &Right);
		
		//Right.HSplitTop(25, 0, &Right);
		//Right.VSplitRight(Right.w/3, &Right, 0);
		Left.HSplitTop(10, 0, &Left);
		Right.HSplitTop(10, 0, &Right);
		
		if(Loading)
		{
			if(Button2vec < 0.6f)
				Button2vec += 0.01f;
		}
		else if(Button2vec > 0.0f) 
			Button2vec -= 0.01f;
			
		RenderTools()->DrawUIRect(&Left, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
		RenderTools()->DrawUIRect(&Middle, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
		RenderTools()->DrawUIRect(&Right, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
		
		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID >= 0)
		{
			CTeeRenderInfo TeeInfo = m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_RenderInfo;
			TeeInfo.m_Size *= 2;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_HAPPY, vec2(1.0f, 0.0f), vec2(Middle.x+Middle.w/2, Middle.y+Middle.h/2+10));
			UI()->DoLabel(&Middle, m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_aName,20.0f*UI()->Scale(),  0);
		}
		
		CUIRect TempCui;
		Middle.HSplitBottom(24, &Middle, &TempCui);
		RenderTools()->DrawUIRect(&TempCui, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);
		UI()->DoLabel(&TempCui, Localize("Follower"),20.0f*UI()->Scale(),  0);
						
		int NewSpectatorID_Next;
		bool GotNewSpectatorID_Next = false;

		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID_Next = i;
				GotNewSpectatorID_Next = true;
				break;
			}
		}
		else
		{
			for(int i = m_pClient->m_Snap.m_SpecInfo.m_SpectatorID + 1; i < MAX_CLIENTS; i++)
			{
				if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID_Next = i;
				GotNewSpectatorID_Next = true;
				break;
			}

			if(!GotNewSpectatorID_Next)
			{
				for(int i = 0; i < m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i++)
				{
					if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
						continue;

					NewSpectatorID_Next = i;
					GotNewSpectatorID_Next = true;
					break;
				}
			}
		}
		if(GotNewSpectatorID_Next)
		{
			CTeeRenderInfo TeeInfo = m_pClient->m_aClients[NewSpectatorID_Next].m_RenderInfo;
			TeeInfo.m_Size *= 1.5f;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Right.x+Right.w/2, Right.y+Right.h/2+10));
			UI()->DoLabel(&Right, m_pClient->m_aClients[NewSpectatorID_Next].m_aName,15.0f*UI()->Scale(),  0);
		}		
		Right.HSplitBottom(22, &Right, &TempCui);
		RenderTools()->DrawUIRect(&TempCui, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);
		UI()->DoLabel(&TempCui, Localize("Next"),18.0f*UI()->Scale(),  0);
		
		if(InView(&Right, Width, Height))		
			RenderTools()->DrawUIRect(&Right, ms_ColorTabbarActive2, CUI::CORNER_T, 10.0f);
		if(Input()->KeyPressed(KEY_MOUSE_1) && InView(&Right, Width, Height))
			RenderTools()->DrawUIRect(&Right, ms_ColorTabbarActive, CUI::CORNER_T, 10.0f);
		if(Input()->KeyDown(KEY_MOUSE_1) && InView(&Right, Width, Height))
		{
			if(GotNewSpectatorID_Next)			
				Spectate(NewSpectatorID_Next);
			m_Selected = NewSpectatorID_Next;
		}
			
		int NewSpectatorID_Prev;
		bool GotNewSpectatorID_Prev = false;

		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
		{
			for(int i = MAX_CLIENTS -1; i > -1; i--)
			{
				if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID_Prev = i;
				GotNewSpectatorID_Prev = true;
				break;
			}
		}
		else
		{
			for(int i = m_pClient->m_Snap.m_SpecInfo.m_SpectatorID - 1; i > -1; i--)
			{
				if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID_Prev = i;
				GotNewSpectatorID_Prev = true;
				break;
			}

			if(!GotNewSpectatorID_Prev)
			{
				for(int i = MAX_CLIENTS - 1; i > m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i--)
				{
					if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
						continue;

					NewSpectatorID_Prev = i;
					GotNewSpectatorID_Prev = true;
					break;
				}
			}
		}
		if(GotNewSpectatorID_Prev)
		{
			CTeeRenderInfo TeeInfo = m_pClient->m_aClients[NewSpectatorID_Prev].m_RenderInfo;
			TeeInfo.m_Size *= 1.5f;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Left.x+Left.w/2, Left.y+Left.h/2+10));
			UI()->DoLabel(&Left, m_pClient->m_aClients[NewSpectatorID_Prev].m_aName,15.0f*UI()->Scale(),  0);
		}			
		Left.HSplitBottom(22, &Left, &TempCui);
		RenderTools()->DrawUIRect(&TempCui, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);
		UI()->DoLabel(&TempCui, Localize("Previous"),18.0f*UI()->Scale(),  0);
		
		if(InView(&Left, Width, Height))		
			RenderTools()->DrawUIRect(&Left, ms_ColorTabbarActive2, CUI::CORNER_T, 10.0f);
		if(Input()->KeyPressed(KEY_MOUSE_1) && InView(&Left, Width, Height))
			RenderTools()->DrawUIRect(&Left, ms_ColorTabbarActive, CUI::CORNER_T, 10.0f);
		if(Input()->KeyDown(KEY_MOUSE_1) && InView(&Left, Width, Height))
		{
			if(GotNewSpectatorID_Prev)
				Spectate(NewSpectatorID_Prev);		
			m_Selected = NewSpectatorID_Prev;
		}
	}	
	
	float TempWidth = BottomMenu.h/2.5f, TempHeight = BottomMenu.h/2.5f;	
	int Count = 0;	
	for(int i = 0; i < 16; i++)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
			continue;
		Count++;				
	}
	
	CUIRect ButtonPlayer, BottomMenu1, BottomMenu2, Button, Button2;
		
	BottomMenu.VSplitLeft(2.0f, 0, &BottomMenu);
	BottomMenu.HSplitTop(TempHeight+5.0f, &BottomMenu1, &BottomMenu2);
	BottomMenu2.HSplitTop(5.0f, 0, &BottomMenu2);	
	BottomMenu2.HSplitTop(TempHeight, &BottomMenu2, &Button);
	
	int MaxCount = floor((BottomMenu1.w+5.0f) / TempWidth);
	
	if(Count > 8)
	{
		BottomMenu1.VMargin((BottomMenu1.w-((TempHeight+5.0f)*8))/2, &BottomMenu1);
		BottomMenu2.VMargin((BottomMenu2.w-((TempHeight+5.0f)*(Count%8)))/2, &BottomMenu2);
	}
	else
	{
		BottomMenu1.VMargin((BottomMenu1.w-((TempHeight+5.0f)*Count))/2, &BottomMenu1);
	}	
	
	Button.VSplitLeft(10.0f, 0, &Button);
	Button.VSplitLeft(200.0f, &Button, &Button2);
	Button2.VSplitRight(10.0f, &Button2, 0);
	Button2.VSplitRight(200.0f, 0, &Button2);
	
	vec4 ms_ColorTabbarActive3 = vec4(0.0f, 0.0f, 0.0f, Button2vec*1.6f);
	RenderTools()->DrawUIRect(&Button, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	RenderTools()->DrawUIRect(&Button2, ms_ColorTabbarActive3, CUI::CORNER_ALL, 10.0f);
	if(InView(&Button, Width, Height))		
		RenderTools()->DrawUIRect(&Button, ms_ColorTabbarActive2, CUI::CORNER_ALL, 10.0f);
	if(Input()->KeyPressed(KEY_MOUSE_1) && InView(&Button, Width, Height))
		RenderTools()->DrawUIRect(&Button, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	if(Input()->KeyDown(KEY_MOUSE_1) && InView(&Button, Width, Height))
	{
		m_SelectedSpectatorID = SPEC_FREEVIEW;
		Spectate(m_SelectedSpectatorID);
		m_Selected = -1;
	}
	if(m_Selected == -1)
		TextRender()->TextOutlineColor(0.0f, 0.0f, 6.0f, 0.5f);
	TextRender()->Text(0, Button.x+7, Button.y+2, 18.0f*UI()->Scale(), Localize("Free-View"), Button.w-4);		
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, Button2vec*1.6f);
	TextRender()->Text(0, Button2.x+10, Button2.y, 22.0f*UI()->Scale(), Localize("Loading"), Button2.w-4);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		
	Count = 0;
	for(int j = 0; j < 16; j++)
	{				
		if(!m_pClient->m_Snap.m_paPlayerInfos[j] || m_pClient->m_Snap.m_paPlayerInfos[j]->m_Team == TEAM_SPECTATORS)
			continue;
		Count ++;
		if(Count == 1)
			BottomMenu = BottomMenu1;
		else if(Count == 9)
			BottomMenu = BottomMenu2;
		
		BottomMenu.VSplitLeft(TempWidth, &ButtonPlayer, &BottomMenu);
		BottomMenu.VSplitLeft(5.0f, 0, &BottomMenu);
		RenderTools()->DrawUIRect(&ButtonPlayer, ms_ColorTabbarActive2, CUI::CORNER_ALL, 10.0f);
				
		if(InView(&ButtonPlayer, Width, Height))		
			RenderTools()->DrawUIRect(&ButtonPlayer, ms_ColorTabbarActive2, CUI::CORNER_ALL, 10.0f);
		if(Input()->KeyPressed(KEY_MOUSE_1) && InView(&ButtonPlayer, Width, Height))
			RenderTools()->DrawUIRect(&ButtonPlayer, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
		if(Input()->KeyDown(KEY_MOUSE_1) && InView(&ButtonPlayer, Width, Height))
		{
			m_SelectedSpectatorID = j;
			Spectate(m_SelectedSpectatorID);
			m_Selected = j;
		}
			
		if(m_Selected == j)
			TextRender()->TextOutlineColor(0.0f, 0.0f, 2.0f, 0.5f);
		TextRender()->Text(0, ButtonPlayer.x+4, ButtonPlayer.y+2, 12.0f*UI()->Scale(), m_pClient->m_aClients[j].m_aName, TempWidth-6.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
		
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[j].m_RenderInfo;
		TeeInfo.m_Size = TempWidth/2;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(ButtonPlayer.x+TempWidth/2, ButtonPlayer.y+TempWidth/2+20));
	}

/* 	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f);
	RenderTools()->DrawRoundRect(Width/2.0f-300.0f, Height/2.0f-300.0f, 600.0f, 600.0f, 20.0f);
	Graphics()->QuadsEnd();

	// clamp mouse position to selector area
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -280.0f, 280.0f);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -280.0f, 280.0f);

	// draw selections
	float FontSize = 20.0f;
	float StartY = -190.0f;
	float LineHeight = 60.0f;
	bool Selected = false;

	if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
		RenderTools()->DrawRoundRect(Width/2.0f-280.0f, Height/2.0f-280.0f, 270.0f, 60.0f, 20.0f);
		Graphics()->QuadsEnd();
	}

	if(m_SelectorMouse.x >= -280.0f && m_SelectorMouse.x <= -10.0f &&
		m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
	{
		m_SelectedSpectatorID = SPEC_FREEVIEW;
		Selected = true;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
	TextRender()->Text(0, Width/2.0f-240.0f, Height/2.0f-265.0f, FontSize, Localize("Free-View"), -1);

	float x = -270.0f, y = StartY;
	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
			continue;

		if(++Count%9 == 0)
		{
			x += 290.0f;
			y = StartY;
		}

		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == i)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(Width/2.0f+x-10.0f, Height/2.0f+y-10.0f, 270.0f, 60.0f, 20.0f);
			Graphics()->QuadsEnd();
		}

		Selected = false;
		if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+260.0f &&
			m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
		{
			m_SelectedSpectatorID = i;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f+x+50.0f, Height/2.0f+y+5.0f, FontSize, m_pClient->m_aClients[i].m_aName, 220.0f);

		// flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == m_pClient->m_Snap.m_paPlayerInfos[i]->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == m_pClient->m_Snap.m_paPlayerInfos[i]->m_ClientID))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(Width/2.0f+x-LineHeight/5.0f, Height/2.0f+y-LineHeight/3.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[i].m_RenderInfo;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Width/2.0f+x+20.0f, Height/2.0f+y+20.0f));

		y += LineHeight;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f); */

 	// draw cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(m_SelectorMouse.x+Width, m_SelectorMouse.y+Height, 24.0f, 24.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd(); 
}

void CSpectator::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedSpectatorID = NO_SELECTION;
	m_ButtonPress = 0;
	m_Selected = -1;
	Button2vec = 0.0f;
	Loading = false;
}

void CSpectator::Spectate(int SpectatorID)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->m_DemoSpecID = clamp(SpectatorID, (int)SPEC_FREEVIEW, MAX_CLIENTS-1);
		return;
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID)
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
