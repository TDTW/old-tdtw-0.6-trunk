/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/components/sounds.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/localization.h>
#include <string.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"

CMenusKeyBinder CMenus::m_Binder;

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
}

bool CMenusKeyBinder::OnInput(IInput::CEvent Event)
{
	if(m_TakeKey)
	{
		if(Event.m_Flags&IInput::FLAG_PRESS)
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;
		}
		return true;
	}

	return false;
}

void CMenus::RenderSettingsGame(CUIRect MainView)
{
	char aBuf[128];
	CUIRect Label, Button, Left, Right, Game, Client;
	MainView.HSplitTop(MainView.h/2-5.0f, &Game, &Client);
	Client.HSplitTop(10.0f, 0, &Client);
	RenderTools()->DrawUIRect(&Game, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f);
	Game.Margin(10.0f, &Game);
	RenderTools()->DrawUIRect(&Client, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Client.Margin(10.0f, &Client);

	// game
	{
		// headline
		Game.HSplitTop(30.0f, &Label, &Game);
		UI()->DoLabelScaled(&Label, Localize("Game"), 20.0f, -1);
		Game.Margin(5.0f, &Game);
		Game.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// dynamic camera
		Left.HSplitTop(20.0f, &Button, &Left);
		static int s_DynamicCameraButton = 0;
		if(DoButton_CheckBox(&s_DynamicCameraButton, Localize("Dynamic Camera"), g_Config.m_ClMouseDeadzone != 0, &Button))
		{
			if(g_Config.m_ClMouseDeadzone)
			{
				g_Config.m_ClMouseFollowfactor = 0;
				g_Config.m_ClMouseMaxDistance = 400;
				g_Config.m_ClMouseDeadzone = 0;
			}
			else
			{
				g_Config.m_ClMouseFollowfactor = 60;
				g_Config.m_ClMouseMaxDistance = 1000;
				g_Config.m_ClMouseDeadzone = 300;
			}
		}

		// weapon pickup
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClAutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
			g_Config.m_ClAutoswitchWeapons ^= 1;

		// show hud
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClShowhud, Localize("Show ingame HUD"), g_Config.m_ClShowhud, &Button))
			g_Config.m_ClShowhud ^= 1;

		// name plates
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClNameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
			g_Config.m_ClNameplates ^= 1;

		if(g_Config.m_ClNameplates)
		{
			Right.HSplitTop(2.5f, 0, &Right);
			Right.VSplitLeft(30.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClNameplatesAlways, Localize("Always show name plates"), g_Config.m_ClNameplatesAlways, &Button))
				g_Config.m_ClNameplatesAlways ^= 1;

			Right.HSplitTop(2.5f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Name plates size"), g_Config.m_ClNameplatesSize);
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Button.HMargin(2.0f, &Button);
			g_Config.m_ClNameplatesSize = (int)(DoScrollbarH(&g_Config.m_ClNameplatesSize, &Button, g_Config.m_ClNameplatesSize/100.0f)*100.0f+0.1f);

			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClNameplatesTeamcolors, Localize("Use team colors for name plates"), g_Config.m_ClNameplatesTeamcolors, &Button))
				g_Config.m_ClNameplatesTeamcolors ^= 1;
				
			Right.HSplitTop(5.0f, 0, &Right);			
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClClanplates, Localize("Use clan plates"), g_Config.m_ClClanplates, &Button))
				g_Config.m_ClClanplates ^= 1;
		}
	}

	// client
	{
		// headline
		Client.HSplitTop(30.0f, &Label, &Client);
		UI()->DoLabelScaled(&Label, Localize("Client"), 20.0f, -1);
		Client.Margin(5.0f, &Client);
		Client.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// auto demo settings
		{
			Left.HSplitTop(20.0f, &Button, &Left);
			if(DoButton_CheckBox(&g_Config.m_ClAutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
				g_Config.m_ClAutoDemoRecord ^= 1;

			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
				g_Config.m_ClAutoScreenshot ^= 1;

			Left.HSplitTop(10.0f, 0, &Left);
			Left.VSplitLeft(20.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			char aBuf[64];
			if(g_Config.m_ClAutoDemoMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max demos"), g_Config.m_ClAutoDemoMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max demos"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Left.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			g_Config.m_ClAutoDemoMax = static_cast<int>(DoScrollbarH(&g_Config.m_ClAutoDemoMax, &Button, g_Config.m_ClAutoDemoMax/1000.0f)*1000.0f+0.1f);

			Right.HSplitTop(10.0f, 0, &Right);
			Right.VSplitLeft(20.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Button.VSplitRight(20.0f, &Button, 0);
			if(g_Config.m_ClAutoScreenshotMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoScreenshotMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Right.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			g_Config.m_ClAutoScreenshotMax = static_cast<int>(DoScrollbarH(&g_Config.m_ClAutoScreenshotMax, &Button, g_Config.m_ClAutoScreenshotMax/1000.0f)*1000.0f+0.1f);
		}
	}
}

void CMenus::RenderSettingsCountry(CUIRect MainView)
{
	// country flag selector
	MainView.HSplitTop(10.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	UiDoListboxStart(&s_ScrollValue, &MainView, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
		if(pEntry->m_CountryCode == g_Config.m_PlayerCountry)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&pEntry->m_CountryCode, OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
			float OldWidth = Item.m_Rect.w;
			Item.m_Rect.w = Item.m_Rect.h*2;
			Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
			Graphics()->TextureSet(pEntry->m_Texture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		g_Config.m_PlayerCountry = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
		m_NeedSendinfo = true;
	}
}
 
void CMenus::RenderSettingsTee(CUIRect MainView)
{
	CUIRect Button, Label, TopRight, TopLeft, Top;		
	MainView.HSplitTop(100.0f, &Top, &MainView);
	// player name
	Top.VSplitMid(&TopRight, &TopLeft);
	TopLeft.VSplitLeft(10.0f, 0, &TopLeft);
	
	RenderTools()->DrawUIRect(&TopRight, vec4(1,1,1,0.25f), CUI::CORNER_TL, 5.0f);
	TopRight.Margin(10.0f, &TopRight);
	RenderTools()->DrawUIRect(&TopLeft, vec4(1,1,1,0.25f), CUI::CORNER_TR, 5.0f);
	TopLeft.Margin(10.0f, &TopLeft);
	
	TopRight.HSplitTop(20.0f, 0, &TopRight);
	TopRight.VSplitLeft(25.0f, 0, &TopRight);
	TopRight.HSplitTop(20.0f, &Button, &TopRight);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetName = 0.0f;
	if(DoEditBox(g_Config.m_PlayerName, &Button, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName))
		m_NeedSendinfo = true;

	// player clan
	TopRight.HSplitTop(5.0f, 0, &TopRight);
	TopRight.HSplitTop(20.0f, &Button, &TopRight);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetClan = 0.0f;
	if(DoEditBox(g_Config.m_PlayerClan, &Button, g_Config.m_PlayerClan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan))
		m_NeedSendinfo = true;
	
	// skin info
	const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(g_Config.m_PlayerSkin));
	CTeeRenderInfo OwnSkinInfo;
	if(g_Config.m_PlayerUseCustomColor)
	{
		OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(g_Config.m_PlayerColorBody);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(g_Config.m_PlayerColorFeet);
	}
	else
	{
		OwnSkinInfo.m_Texture = pOwnSkin->m_OrgTexture;
		OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f*UI()->Scale();

	TopLeft.HSplitTop(20.0f, &Label, &TopLeft);
	Label.VSplitLeft(230.0f, &Label, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Your skin"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, -1);

	TopLeft.HSplitTop(50.0f, &Label, &TopLeft);
	Label.VSplitLeft(230.0f, &Label, 0);
	RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x+30.0f, Label.y+28.0f));
	Label.HSplitTop(15.0f, 0, &Label);;
	Label.VSplitLeft(70.0f, 0, &Label);
	UI()->DoLabelScaled(&Label, g_Config.m_PlayerSkin, 14.0f, -1, 150.0f);
	
	// custom colour selector
	CUIRect Center;
	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(130.0f, &Center, &MainView);
	RenderTools()->DrawUIRect(&Center, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Center.Margin(10.0f, &Center);
	Center.HSplitTop(20.0f, &Button, &Center);
	Button.VSplitLeft(230.0f, &Button, 0);
	if(DoButton_CheckBox(&g_Config.m_PlayerColorBody, Localize("Custom colors"), g_Config.m_PlayerUseCustomColor, &Button))
	{
		g_Config.m_PlayerUseCustomColor = g_Config.m_PlayerUseCustomColor?0:1;
		m_NeedSendinfo = true;
	}

	Center.HSplitTop(5.0f, 0, &Center);
	Center.HSplitTop(82.5f, &Label, &Center);
	if(g_Config.m_PlayerUseCustomColor)
	{
		CUIRect aRects[2];
		Label.VSplitMid(&aRects[0], &aRects[1]);
		aRects[0].VSplitRight(10.0f, &aRects[0], 0);
		aRects[1].VSplitLeft(10.0f, 0, &aRects[1]);

		int *paColors[2];
		paColors[0] = &g_Config.m_PlayerColorBody;
		paColors[1] = &g_Config.m_PlayerColorFeet;

		const char *paParts[] = {
			Localize("Body"),
			Localize("Feet")};
		const char *paLabels[] = {
			Localize("Hue"),
			Localize("Sat."),
			Localize("Lht.")};
		static int s_aColorSlider[2][3] = {{0}};

		for(int i = 0; i < 2; i++)
		{
			aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
			UI()->DoLabelScaled(&Label, paParts[i], 14.0f, -1);
			aRects[i].VSplitLeft(20.0f, 0, &aRects[i]);
			aRects[i].HSplitTop(2.5f, 0, &aRects[i]);

			int PrevColor = *paColors[i];
			int Color = 0;
			for(int s = 0; s < 3; s++)
			{
				aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
				Label.VSplitLeft(100.0f, &Label, &Button);
				Button.HMargin(2.0f, &Button);

				float k = ((PrevColor>>((2-s)*8))&0xff) / 255.0f;
				k = DoScrollbarH(&s_aColorSlider[i][s], &Button, k);
				Color <<= 8;
				Color += clamp((int)(k*255), 0, 255);
				UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
			}

			if(PrevColor != Color)
				m_NeedSendinfo = true;

			*paColors[i] = Color;
		}
	}

	// skin selector
	MainView.HSplitTop(10.0f, 0, &MainView);
	static bool s_InitSkinlist = true;
	static sorted_array<const CSkins::CSkin *> s_paSkinList;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
		{
			const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;				
			if(!strncmp(s->m_aName, "PsychoGod", sizeof("PsychoGod")) || !strncmp(s->m_aName, "soullibra", sizeof("soullibra")) || !strncmp(s->m_aName, "Sergio", sizeof("Sergio")))
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = false;
	}

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &MainView, 50.0f, Localize("Skins"), "", s_paSkinList.size(), 4, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CSkins::CSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;

		if(str_comp(s->m_aName, g_Config.m_PlayerSkin) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			if(g_Config.m_PlayerUseCustomColor)
			{
				Info.m_Texture = s->m_ColorTexture;
				Info.m_ColorBody = m_pClient->m_pSkins->GetColorV4(g_Config.m_PlayerColorBody);
				Info.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(g_Config.m_PlayerColorFeet);
			}
			else
			{
				Info.m_Texture = s->m_OrgTexture;
				Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));

			if(g_Config.m_Debug)
			{
				vec3 BloodColor = g_Config.m_PlayerUseCustomColor ? m_pClient->m_pSkins->GetColorV3(g_Config.m_PlayerColorBody) : s->m_BloodColor;
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(BloodColor.r, BloodColor.g, BloodColor.b, 1.0f);
				IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, 12.0f, 12.0f);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		mem_copy(g_Config.m_PlayerSkin, s_paSkinList[NewSelected]->m_aName, sizeof(g_Config.m_PlayerSkin));
		m_NeedSendinfo = true;
	}
}


typedef void (*pfnAssignFuncCallback)(CConfiguration *pConfig, int Value);

typedef struct
{
	CLocConstString m_Name;
	const char *m_pCommand;
	int m_KeyId;
} CKeyInfo;

static CKeyInfo gs_aKeys[] =
{
	{ "Move left", "+left", 0},		// Localize - these strings are localized within CLocConstString
	{ "Move right", "+right", 0 },
	{ "Jump", "+jump", 0 },
	{ "Fire", "+fire", 0 },
	{ "Hook", "+hook", 0 },
	{ "Hammer", "+weapon1", 0 },
	{ "Pistol", "+weapon2", 0 },
	{ "Shotgun", "+weapon3", 0 },
	{ "Grenade", "+weapon4", 0 },
	{ "Rifle", "+weapon5", 0 },
	{ "Next weapon", "+nextweapon", 0 },
	{ "Prev. weapon", "+prevweapon", 0 },
	{ "Vote yes", "vote yes", 0 },
	{ "Vote no", "vote no", 0 },
	{ "Chat", "chat all", 0 },
	{ "Team chat", "chat team", 0 },
	{ "Show chat", "+show_chat", 0 },
	{ "Emoticon", "+emote", 0 },
	{ "Spectator mode", "+spectate", 0 },
	{ "Spectate next", "spectate_next", 0 },
	{ "Spectate previous", "spectate_previous", 0 },
	{ "Console", "toggle_local_console", 0 },
	{ "Remote console", "toggle_remote_console", 0 },
	{ "Screenshot", "screenshot", 0 },
	{ "Scoreboard", "+scoreboard", 0 },	
	{ "Dyn camera toggle", "dynamic_camera_toggle", 0},
	{ "Start recording", "record", 0},
	{ "Stop recording", "stoprecord", 0},
	{ "Zoom In", "+zoomin", 0},
	{ "Zoom Out", "+zoomout", 0},
	{ "Zoom Reset", "zoomreset", 0},

};

/*	This is for scripts/update_localization.py to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");Localize("Hammer");
	Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Rifle");Localize("Next weapon");Localize("Prev. weapon");
	Localize("Vote yes");Localize("Vote no");Localize("Chat");Localize("Team chat");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");Localize("Remote console");Localize("Screenshot");Localize("Scoreboard");
*/

const int g_KeyCount = sizeof(gs_aKeys) / sizeof(CKeyInfo);

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View)
{
	for (int i = Start; i < Stop; i++)
	{
		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(20.0f, &Button, &View);
		Button.VSplitLeft(135.0f, &Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", (const char *)Key.m_Name);

		UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
		int OldId = Key.m_KeyId;
		int NewId = DoKeyReader((void *)&gs_aKeys[i].m_Name, &Button, OldId);
		if(NewId != OldId)
		{
			if(OldId != 0 || NewId == 0)
				m_pClient->m_pBinds->Bind(OldId, "");
			if(NewId != 0)
				m_pClient->m_pBinds->Bind(NewId, gs_aKeys[i].m_pCommand);
		}
		View.HSplitTop(5.0f, 0, &View);
	}
}

void CMenus::RenderSettingsControls(CUIRect MainView)
{	
	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}

	CUIRect MovementSettings, WeaponSettings, VotingSettings, ChatSettings, MiscSettings, ResetButton;
	MainView.VSplitLeft(MainView.w/2-5.0f, &MovementSettings, &VotingSettings);

	// movement settings
	{
		MovementSettings.HSplitTop(MainView.h/3+45.0f, &MovementSettings, &WeaponSettings);
		RenderTools()->DrawUIRect(&MovementSettings, vec4(1,1,1,0.25f), CUI::CORNER_TL, 5.0f);
		MovementSettings.Margin(10.0f, &MovementSettings);

		TextRender()->Text(0, MovementSettings.x, MovementSettings.y, 14.0f*UI()->Scale(), Localize("Movement"), -1);

		MovementSettings.HSplitTop(14.0f+10.0f, 0, &MovementSettings);

		{
			CUIRect Button, Label;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(135.0f, &Label, &Button);
			UI()->DoLabel(&Label, Localize("Ingame mouse sens."), 14.0f*UI()->Scale(), -1);
			Button.HMargin(2.0f, &Button);
			g_Config.m_InpMousesens = DoCoolScrollbarH(&g_Config.m_InpMousesens, &Button, g_Config.m_InpMousesens ,5,500.0f);
			MovementSettings.HSplitTop(2.0f, 0, &MovementSettings);
		}
        
		{
			CUIRect Button, Label;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(135.0f, &Label, &Button);
			UI()->DoLabel(&Label, Localize("Menu mouse sens."), 14.0f*UI()->Scale(), -1);
			Button.HMargin(2.0f, &Button);
			g_Config.m_UiMousesens = DoCoolScrollbarH(&g_Config.m_UiMousesens, &Button, g_Config.m_UiMousesens ,5,500.0f);
			MovementSettings.HSplitTop(2.0f, 0, &MovementSettings);
		}
		UiDoGetButtons(0, 5, MovementSettings);

	}

	// weapon settings
	{
		WeaponSettings.HSplitTop(10.0f, 0, &WeaponSettings);
		WeaponSettings.HSplitTop(MainView.h/3+55.0f, &WeaponSettings, &ResetButton);
		RenderTools()->DrawUIRect(&WeaponSettings, vec4(1,1,1,0.25f), 0, 0.0f);
		WeaponSettings.Margin(10.0f, &WeaponSettings);

		TextRender()->Text(0, WeaponSettings.x, WeaponSettings.y, 14.0f*UI()->Scale(), Localize("Weapon"), -1);

		WeaponSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &WeaponSettings);
		UiDoGetButtons(5, 12, WeaponSettings);
	}

	// defaults
	{
		ResetButton.HSplitTop(10.0f, 0, &ResetButton);
		RenderTools()->DrawUIRect(&ResetButton, vec4(1,1,1,0.25f), CUI::CORNER_BL, 5.0f);
		ResetButton.HMargin(10.0f, &ResetButton);
		ResetButton.VMargin(30.0f, &ResetButton);
		static int s_DefaultButton = 0;
		if(DoButton_Menu((void*)&s_DefaultButton, Localize("Reset to defaults"), 0, &ResetButton))
			m_pClient->m_pBinds->SetDefaults();
	}

	// voting settings
	{
		VotingSettings.VSplitLeft(10.0f, 0, &VotingSettings);
		VotingSettings.HSplitTop(MainView.h/3-70.0f, &VotingSettings, &ChatSettings);
		RenderTools()->DrawUIRect(&VotingSettings, vec4(1,1,1,0.25f), CUI::CORNER_TR, 5.0f);
		VotingSettings.Margin(10.0f, &VotingSettings);

		TextRender()->Text(0, VotingSettings.x, VotingSettings.y, 14.0f*UI()->Scale(), Localize("Voting"), -1);

		VotingSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &VotingSettings);
		UiDoGetButtons(12, 14, VotingSettings);
	}

	// chat settings
	{
		ChatSettings.HSplitTop(10.0f, 0, &ChatSettings);
		ChatSettings.HSplitTop(MainView.h/3-45.0f, &ChatSettings, &MiscSettings);
		RenderTools()->DrawUIRect(&ChatSettings, vec4(1,1,1,0.25f), 0, 0.0f);
		ChatSettings.Margin(10.0f, &ChatSettings);

		TextRender()->Text(0, ChatSettings.x, ChatSettings.y, 14.0f*UI()->Scale(), Localize("Chat"), -1);

		ChatSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &ChatSettings);
		UiDoGetButtons(14, 17, ChatSettings);
	}

	// misc settings
	{
		MiscSettings.HSplitTop(10.0f, 0, &MiscSettings);
		RenderTools()->DrawUIRect(&MiscSettings, vec4(1,1,1,0.25f), CUI::CORNER_BR, 5.0f);
		MiscSettings.Margin(10.0f, &MiscSettings);

		TextRender()->Text(0, MiscSettings.x, MiscSettings.y, 14.0f*UI()->Scale(), Localize("Miscellaneous"), -1);

		MiscSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &MiscSettings);
		UiDoGetButtons(17, 25, MiscSettings);
	}
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CUIRect Button;
	char aBuf[128];
	bool CheckSettings = false;

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS);
	static int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	static int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	static int s_GfxColorDepth = g_Config.m_GfxColorDepth;
	static int s_GfxFullscreen = g_Config.m_GfxFullscreen;
	static int s_GfxVsync = g_Config.m_GfxVsync;
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static int s_GfxTextureQuality = g_Config.m_GfxTextureQuality;
	static int s_GfxTextureCompression = g_Config.m_GfxTextureCompression;

	CUIRect ModeList;
	MainView.VSplitLeft(300.0f, &MainView, &ModeList);
	RenderTools()->DrawUIRect(&MainView, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 5.0f);
	MainView.Margin(10.0f, &MainView);

	// draw allmodes switch
	ModeList.VSplitLeft(10, 0, &ModeList);
	ModeList.HSplitTop(20, &Button, &ModeList);
	if(DoButton_CheckBox(&g_Config.m_GfxDisplayAllModes, Localize("Show only supported"), g_Config.m_GfxDisplayAllModes^1, &Button))
	{
		g_Config.m_GfxDisplayAllModes ^= 1;
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS);
	}

	// display mode list
	static float s_ScrollValue = 0;
	int OldSelected = -1;
	int G = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
	str_format(aBuf, sizeof(aBuf), "%s: %dx%d %d bit (%d:%d)", Localize("Current"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxColorDepth, s_GfxScreenWidth/G, s_GfxScreenHeight/G);
	UiDoListboxStart(&s_NumNodes , &ModeList, 24.0f, Localize("Display Modes"), aBuf, s_NumNodes, 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_NumNodes; ++i)
	{
		const int Depth = s_aModes[i].m_Red+s_aModes[i].m_Green+s_aModes[i].m_Blue > 16 ? 24 : 16;
		if(g_Config.m_GfxColorDepth == Depth &&
			g_Config.m_GfxScreenWidth == s_aModes[i].m_Width &&
			g_Config.m_GfxScreenHeight == s_aModes[i].m_Height)
		{
			OldSelected = i;
		}

		CListboxItem Item = UiDoListboxNextItem(&s_aModes[i], OldSelected == i);
		if(Item.m_Visible)
		{
			int G = gcd(s_aModes[i].m_Width, s_aModes[i].m_Height);
			str_format(aBuf, sizeof(aBuf), " %dx%d %d bit (%d:%d)", s_aModes[i].m_Width, s_aModes[i].m_Height, Depth, s_aModes[i].m_Width/G, s_aModes[i].m_Height/G);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red+s_aModes[NewSelected].m_Green+s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_Width;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_Height;
		CheckSettings = true;
	}

	// switches
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxFullscreen, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
	{
		g_Config.m_GfxFullscreen ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxVsync, Localize("V-Sync"), g_Config.m_GfxVsync, &Button))
	{
		g_Config.m_GfxVsync ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox_Number(&g_Config.m_GfxFsaaSamples, Localize("FSAA samples"), g_Config.m_GfxFsaaSamples, &Button))
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples+1)%17;
		CheckSettings = true;
	}

	MainView.HSplitTop(40.0f, &Button, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxTextureQuality, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxTextureCompression, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;

	// check if the new settings require a restart
	if(CheckSettings)
	{
		if(s_GfxScreenWidth == g_Config.m_GfxScreenWidth &&
			s_GfxScreenHeight == g_Config.m_GfxScreenHeight &&
			s_GfxColorDepth == g_Config.m_GfxColorDepth &&
			s_GfxFullscreen == g_Config.m_GfxFullscreen &&
			s_GfxVsync == g_Config.m_GfxVsync &&
			s_GfxFsaaSamples == g_Config.m_GfxFsaaSamples &&
			s_GfxTextureQuality == g_Config.m_GfxTextureQuality &&
			s_GfxTextureCompression == g_Config.m_GfxTextureCompression)
			m_NeedRestartGraphics = false;
		else
			m_NeedRestartGraphics = true;
	}

	//

	CUIRect Text;
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	//text.VSplitLeft(15.0f, 0, &text);
	UI()->DoLabelScaled(&Text, Localize("UI Color"), 14.0f, -1);

	const char *paLabels[] = {
		Localize("Hue"),
		Localize("Sat."),
		Localize("Lht."),
		Localize("Alpha")};
	int *pColorSlider[4] = {&g_Config.m_UiColorHue, &g_Config.m_UiColorSat, &g_Config.m_UiColorLht, &g_Config.m_UiColorAlpha};
	for(int s = 0; s < 4; s++)
	{
		CUIRect Text;
		MainView.HSplitTop(19.0f, &Button, &MainView);
		Button.VMargin(15.0f, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);
		//Button.VSplitRight(5.0f, &Button, 0);
		Button.HSplitTop(4.0f, 0, &Button);

		float k = (*pColorSlider[s]) / 255.0f;
		k = DoScrollbarH(pColorSlider[s], &Button, k);
		*pColorSlider[s] = (int)(k*255.0f);
		UI()->DoLabelScaled(&Text, paLabels[s], 15.0f, -1);
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Button;
	RenderTools()->DrawUIRect(&MainView, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 5.0f);
	MainView.Margin(10.0f, &MainView);
	MainView.VSplitMid(&MainView, 0);
	static int s_SndEnable = g_Config.m_SndEnable;
	static int s_SndRate = g_Config.m_SndRate;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &Button))
	{
		g_Config.m_SndEnable ^= 1;
		if(g_Config.m_SndEnable)
		{
			if(g_Config.m_SndMusic)
				m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f, vec2(0, 0));
		}
		else
			m_pClient->m_pSounds->Stop(SOUND_MENU);
		m_NeedRestartSound = g_Config.m_SndEnable && (!s_SndEnable || s_SndRate != g_Config.m_SndRate);
	}

	if(!g_Config.m_SndEnable)
		return;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndMusic, Localize("Play background music"), g_Config.m_SndMusic, &Button))
	{
		g_Config.m_SndMusic ^= 1;
		if(g_Config.m_SndMusic)
			m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f, vec2(0, 0));
		else
			m_pClient->m_pSounds->Stop(SOUND_MENU);
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	// sample rate box
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_SndRate);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoLabelScaled(&Button, Localize("Sample rate"), 14.0f, -1);
		Button.VSplitLeft(190.0f, 0, &Button);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_SndRate, &Button, aBuf, sizeof(aBuf), 14.0f, &Offset);
		g_Config.m_SndRate = max(1, str_toint(aBuf));
		m_NeedRestartSound = !s_SndEnable || s_SndRate != g_Config.m_SndRate;
	}

	// volume slider
	{
		CUIRect Button, Label;
		MainView.HSplitTop(10.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sound volume"), 14.0f, -1);
		//g_Config.m_SndVolume = (int)(DoScrollbarH(&g_Config.m_SndVolume, &Button, g_Config.m_SndVolume/100.0f)*100.0f);
		g_Config.m_SndVolume = DoCoolScrollbarH(&g_Config.m_SndVolume, &Button, g_Config.m_SndVolume ,0.0f ,100.0f);
		MainView.HSplitTop(20.0f, 0, &MainView);
	}
}

class CLanguage
{
public:
	CLanguage() {}
	CLanguage(const char *n, const char *f, int Code) : m_Name(n), m_FileName(f), m_CountryCode(Code) {}

	string m_Name;
	string m_FileName;
	int m_CountryCode;

	bool operator<(const CLanguage &Other) { return m_Name < Other.m_Name; }
};

void LoadLanguageIndexfile(IStorage *pStorage, IConsole *pConsole, sorted_array<CLanguage> *pLanguages)
{
	IOHANDLE File = pStorage->OpenFile("languages/index.txt", IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "couldn't open index file");
		return;
	}

	char aOrigin[128];
	char aReplacement[128];
	CLineReader LineReader;
	LineReader.Init(File);
	char *pLine;
	while((pLine = LineReader.Get()))
	{
		if(!str_length(pLine) || pLine[0] == '#') // skip empty lines and comments
			continue;

		str_copy(aOrigin, pLine, sizeof(aOrigin));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			(void)LineReader.Get();
			continue;
		}
		str_copy(aReplacement, pLine+3, sizeof(aReplacement));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			continue;
		}

		char aFileName[128];
		str_format(aFileName, sizeof(aFileName), "languages/%s.txt", aOrigin);
		pLanguages->add(CLanguage(aReplacement, aFileName, str_toint(pLine+3)));
	}
	io_close(File);
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	CUIRect Right, Left, Bottom;
	
	MainView.HSplitTop(MainView.h/2-5.0f, &MainView, &Bottom);
	
	MainView.VSplitLeft(MainView.w/2-5.0f, &Left, &Right);
	Right.VSplitLeft(10.0f, 0, &Right);
	
	RenderSettingsCountry(Bottom);
	RenderLanguageSelection(Left);
	RenderFontSelection(Right);
}

void CMenus::RenderLanguageSelection(CUIRect MainView)
{
	static int s_LanguageList = 0;
	static int s_SelectedLanguage = 0;
	static sorted_array<CLanguage> s_Languages;
	static float s_ScrollValue = 0;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
	}

	int OldSelected = s_SelectedLanguage;

	UiDoListboxStart(&s_LanguageList , &MainView, 24.0f, Localize("Language"), "", s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem(&r.front());

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			Graphics()->TextureSet(m_pClient->m_pCountryFlags->GetByCountryCode(r.front().m_CountryCode)->m_Texture);
			Graphics()->QuadsBegin();
			IGraphics::CQuadItem QuadItem(Rect.x, Rect.y, Rect.w, Rect.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			Item.m_Rect.HSplitTop(2.0f, 0, &Item.m_Rect);
 			UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, 16.0f, -1);
		}
	}

	s_SelectedLanguage = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());
	}
}

class CFontFile
{
public:
	CFontFile() {}
	CFontFile(const char *n, const char *f) : m_Name(n), m_FileName(f) {}

	string m_Name;
	string m_FileName;

	bool operator<(const CFontFile &Other) { return m_Name < Other.m_Name; }
};

int GatherFonts(const char *pFileName, int IsDir, int Type, void *pUser)
{
 	const int PathLength = str_length(pFileName);
	if(IsDir || PathLength <= 4 || pFileName[PathLength-4] != '.' || str_comp_nocase(pFileName+PathLength-3, "ttf") || pFileName[0] == '.')
		return 0;

	sorted_array<CFontFile> &Fonts = *((sorted_array<CFontFile> *)pUser);
	
	char aNiceName[128];
	str_copy(aNiceName, pFileName, PathLength-3);
	aNiceName[0] = str_uppercase(aNiceName[0]);

	// check if the font was already added
	for(int i = 0; i < Fonts.size(); i++)
		if(!str_comp(Fonts[i].m_Name, aNiceName))
			return 0;
	
	Fonts.add(CFontFile(aNiceName, pFileName)); 
	return 0;
}

void CMenus::RenderFontSelection(CUIRect MainView)
{
 	static int s_FontList  = 0;
	static int s_SelectedFont = 0;
	static sorted_array<CFontFile> s_Fonts;
	static float s_ScrollValue = 0;

	if(s_Fonts.size() == 0)
	{
		Storage()->ListDirectory(IStorage::TYPE_ALL, "fonts", GatherFonts, &s_Fonts);
		for(int i = 0; i < s_Fonts.size(); i++)
			if(str_comp(s_Fonts[i].m_FileName, g_Config.m_ClFontfile) == 0)
			{
				s_SelectedFont = i;
				break;
			}
	}

	int OldSelectedFont = s_SelectedFont;

	UiDoListboxStart(&s_FontList , &MainView, 24.0f, Localize("Fonts"), "", s_Fonts.size(), 1, s_SelectedFont, s_ScrollValue);

	for(sorted_array<CFontFile>::range r = s_Fonts.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem(&r.front());

		if(Item.m_Visible)
			UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, 16.0f, -1);
	}

	s_SelectedFont = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelectedFont != s_SelectedFont)
	{
		str_copy(g_Config.m_ClFontfile, s_Fonts[s_SelectedFont].m_FileName, sizeof(g_Config.m_ClFontfile));
		char aRelFontPath[512];
		str_format(aRelFontPath, sizeof(aRelFontPath), "fonts/%s", g_Config.m_ClFontfile);
		char aFontPath[512];
		IOHANDLE File = Storage()->OpenFile(aRelFontPath, IOFLAG_READ, IStorage::TYPE_ALL, aFontPath, sizeof(aFontPath));
		if(File)
			io_close(File);
		TextRender()->SetDefaultFont(TextRender()->LoadFont(aFontPath));
	} 
}

void CMenus::RenderColFeat(CUIRect MainView)
{
	CUIRect Button, Antiping, Effects, LaserCol, Other, Highlights, Left;
	MainView.HSplitBottom(120.0f, &MainView, &Other);
	MainView.VSplitLeft(MainView.w/2-5.0f, &Left, &Effects);
	
	Left.HSplitTop(100.0f, &Highlights, &LaserCol);
		
	RenderTools()->DrawUIRect(&Highlights, vec4(1,1,1,0.25f), CUI::CORNER_TL, 5.0f);
	Highlights.Margin(10.0f, &Highlights);
	
	TextRender()->Text(0, Highlights.x, Highlights.y-5, 18, Localize("Highlights in browser"), -1);
	Highlights.HSplitTop(20.0f, 0, &Highlights);
	Highlights.HSplitTop(20.0f, &Button, &Highlights);
	if(DoButton_CheckBox(&g_Config.m_ClHighlightGametypes, Localize("Highlight for gametypes"), g_Config.m_ClHighlightGametypes, &Button))
		g_Config.m_ClHighlightGametypes ^= 1;
	Highlights.HSplitTop(20.0f, &Button, &Highlights);
	if(DoButton_CheckBox(&g_Config.m_ClHighlightPlayer, Localize("Highlight for number of players"), g_Config.m_ClHighlightPlayer, &Button))
		g_Config.m_ClHighlightPlayer ^= 1;
	Highlights.HSplitTop(20.0f, &Button, &Highlights);
	if(DoButton_CheckBox(&g_Config.m_ClHighlightPing, Localize("Highlight for ping"), g_Config.m_ClHighlightPing, &Button))
		g_Config.m_ClHighlightPing ^= 1;
		
	LaserCol.HSplitTop(10.0f, 0, &LaserCol);	
	RenderTools()->DrawUIRect(&LaserCol, vec4(1,1,1,0.25f), 0, 5.0f);
	LaserCol.Margin(10.0f, &LaserCol);	
	TextRender()->Text(0, LaserCol.x, LaserCol.y-5, 18, Localize("Laser color"), -1);
	LaserCol.HSplitTop(20.0f, 0, &LaserCol);
	
	// laser color
	{
		CUIRect Laser, Label, OutLine;
		LaserCol.VSplitLeft(10.0f, 0, &LaserCol);
		LaserCol.HSplitTop(80.0f, &LaserCol, &OutLine);

		int *pColor;
		pColor = &g_Config.m_ClLaserColorInner;
		int *pColor2;
		pColor2 = &g_Config.m_ClLaserColorOuter;

		const char *pParts[] = {
			Localize("Inner color"),
			Localize("Outline color")};
		const char *paLabels[] = {
			Localize("Hue"),
			Localize("Sat."),
			Localize("Lht.")};
		static int s_aColorSlider[3] = {0};
		static int s_aColorSlider2[3] = {0};

		LaserCol.HSplitTop(15.0f, &Label, &LaserCol);
		UI()->DoLabelScaled(&Label, pParts[0], 14.0f, -1);
		LaserCol.VSplitLeft(10.0f, 0, &LaserCol);
		LaserCol.HSplitTop(2.5f, 0, &LaserCol);

		int PrevColor = *pColor;
		int Color = 0;
		for(int s = 0; s < 3; s++)
		{
			LaserCol.HSplitTop(20.0f, &Label, &LaserCol);
			Label.VSplitLeft(100.0f, &Label, &Button);
			Button.HMargin(2.0f, &Button);

			float k = ((PrevColor>>((2-s)*8))&0xff)  / 255.0f;
			k = DoScrollbarH(&s_aColorSlider[s], &Button, k);
			Color <<= 8;
			Color += clamp((int)(k*255), 0, 255);
			UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
		}
		
		*pColor = Color;

		OutLine.HSplitTop(15.0f, &Label, &OutLine);
		UI()->DoLabelScaled(&Label, pParts[1], 14.0f, -1);
		OutLine.VSplitLeft(10.0f, 0, &OutLine);
		OutLine.HSplitTop(2.5f, 0, &OutLine);

		int PrevColor2 = *pColor2;
		int Color2 = 0;
		for(int s = 0; s < 3; s++)
		{
			OutLine.HSplitTop(20.0f, &Label, &OutLine);
			Label.VSplitLeft(100.0f, &Label, &Button);
			Button.HMargin(2.0f, &Button);

			float k = ((PrevColor2>>((2-s)*8))&0xff)  / 255.0f;
			k = DoScrollbarH(&s_aColorSlider2[s], &Button, k);
			Color2 <<= 8;
			Color2 += clamp((int)(k*255), 0, 255);
			UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
		}

		*pColor2 = Color2;

		OutLine.HSplitTop(15.0f, 0, &Laser);
		Laser.HSplitTop(18.0f, 0, &Button);
		Button.VSplitRight(20.0f, &Button, 0);
		Laser.VSplitLeft(5.0f, 0, &Laser);
		
		// darw laser
		vec2 From = vec2(Laser.x, Laser.y);
		vec2 Pos = vec2(Laser.x+Laser.w-10.0f, Laser.y);

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();

		// do outline
		vec3 RgbOut = m_pClient->m_pSkins->GetColorV3(g_Config.m_ClLaserColorOuter);
		Graphics()->SetColor(RgbOut.r, RgbOut.g, RgbOut.b, 1.0f); // outline
		vec2 Out = vec2(0.0f, -1.0f) * (3.15f);

		IGraphics::CFreeformItem Freeform(
				From.x-Out.x, From.y-Out.y,
				From.x+Out.x, From.y+Out.y,
				Pos.x-Out.x, Pos.y-Out.y,
				Pos.x+Out.x, Pos.y+Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);

		// do inner	
		Out = vec2(0.0f, -1.0f) * (2.25f);
		vec3 RgbInner = m_pClient->m_pSkins->GetColorV3(g_Config.m_ClLaserColorInner);
		Graphics()->SetColor(RgbInner.r, RgbInner.g, RgbInner.b, 1.0f); // center

		Freeform = IGraphics::CFreeformItem(
				From.x-Out.x, From.y-Out.y,
				From.x+Out.x, From.y+Out.y,
				Pos.x-Out.x, Pos.y-Out.y,
				Pos.x+Out.x, Pos.y+Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	
		Graphics()->QuadsEnd();
		
		// render head
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(SPRITE_PART_SPLAT01);
		Graphics()->SetColor(RgbOut.r, RgbOut.g, RgbOut.b, 1.0f);
		IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 24, 24);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->SetColor(RgbInner.r, RgbInner.g, RgbInner.b, 1.0f);
		QuadItem = IGraphics::CQuadItem(Pos.x, Pos.y, 20, 20);
		Graphics()->QuadsDraw(&QuadItem, 1);

		Graphics()->QuadsEnd();

		// draw laser weapon
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(SPRITE_WEAPON_RIFLE_BODY);
		RenderTools()->DrawSprite(Laser.x, Laser.y, 60.0f);

		Graphics()->QuadsEnd();
				
		//Button.Margin(10, &Button);
		static int s_ResetButton = 0;
		if(DoButton_Menu(&s_ResetButton, Localize("Reset color"), 0, &Button))
		{
			g_Config.m_ClLaserColorInner = 11665217;
			g_Config.m_ClLaserColorOuter = 11665217;
		}
	}
	
	Effects.VSplitLeft(10, &Left, &Effects);
	Effects.HSplitTop(80.0f, &Effects, &Antiping);
	RenderTools()->DrawUIRect(&Effects, vec4(1,1,1,0.25f), CUI::CORNER_TR, 5.0f);
	Effects.Margin(10.0f, &Effects);
	
	Other.HSplitTop(10.0f, 0, &Other);
	RenderTools()->DrawUIRect(&Other, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Other.Margin(10.0f, &Other);
			
	TextRender()->Text(0, Effects.x, Effects.y-5, 18, Localize("Effects"), -1);
	Effects.HSplitTop(20.0f, 0, &Effects);
	Effects.HSplitTop(20.0f, &Button, &Effects);
	if(DoButton_CheckBox(&g_Config.m_ClEffectsFlagtrail, Localize("FlagTrail"), g_Config.m_ClEffectsFlagtrail, &Button))
		g_Config.m_ClEffectsFlagtrail ^= 1;	
		
	Effects.HSplitTop(20.0f, &Button, &Effects);	
	if(DoButton_CheckBox(&g_Config.m_ClEffectsWeapontrail, Localize("WeaponTrail"), g_Config.m_ClEffectsWeapontrail, &Button))
		g_Config.m_ClEffectsWeapontrail ^= 1;	
	
	Antiping.HSplitTop(10.0f, 0, &Antiping);
	RenderTools()->DrawUIRect(&Antiping, vec4(1,1,1,0.25f), 0, 5.0f);
	Antiping.Margin(10.0f, &Antiping);
		
	TextRender()->Text(0, Antiping.x, Antiping.y-5, 18, Localize("Antiping"), -1);
	Antiping.HSplitTop(20.0f, 0, &Antiping);
	Antiping.HSplitTop(20.0f, &Button, &Antiping);
	
	if(DoButton_CheckBox(&g_Config.m_AntiPing, Localize("Antiping"), g_Config.m_AntiPing, &Button))
		g_Config.m_AntiPing = g_Config.m_AntiPing?0:1;
	
	if(g_Config.m_AntiPing == 1 || g_Config.m_AntiPing == 2)
	{
		Antiping.HSplitTop(20.0f, &Button, &Antiping);
		Button.VSplitLeft(15.0f, 0, &Button);
		if(DoButton_CheckBox(&g_Config.m_AntiPingGrenade, Localize("Antiping only grenade"), g_Config.m_AntiPingGrenade, &Button))
			g_Config.m_AntiPingGrenade ^=1;
		if(g_Config.m_AntiPingGrenade == 1)
			g_Config.m_AntiPing = 2;
		else
			g_Config.m_AntiPing = 1;
	}	
		
	if(g_Config.m_AntiPing == 1)
	{
		CUIRect aRects, Label;
		Antiping.VSplitLeft(15.0f, 0, &Antiping);
		Antiping.HSplitTop(20.0f, &Label, &Antiping);	
		
		if(DoButton_CheckBox(&g_Config.m_AntiPingTeeColor, Localize("Use tee color"), g_Config.m_AntiPingTeeColor, &Label))
			g_Config.m_AntiPingTeeColor ^=1;
			
		Antiping.HSplitTop(20.0f, &Label, &aRects);	
		
		int *paColors;
		paColors = &g_Config.m_PlayerColorGhost;

		const char *paLabels[] = {
			Localize("Hue"),
			Localize("Sat."),
			Localize("Lht."),
			Localize("Alpha")};
		static int s_aColorSlider[3] = {0};

		UI()->DoLabelScaled(&Label, Localize("Ghost color"), 14.0f, -1);
		aRects.VSplitLeft(20.0f, 0, &aRects);
		aRects.HSplitTop(2.5f, 0, &aRects);

		int PrevColor = *paColors;
		int Color = 0;
		if(g_Config.m_AntiPingTeeColor == 0)
		{
			for(int s = 0; s < 3; s++)
			{
				aRects.HSplitTop(20.0f, &Label, &aRects);
				Label.VSplitLeft(100.0f, &Label, &Button);
				Button.HMargin(2.0f, &Button);

				float k = ((PrevColor>>((2-s)*8))&0xff) / 255.0f;
				k = DoScrollbarH(&s_aColorSlider[s], &Button, k);
				Color <<= 8;
				Color += clamp((int)(k*255), 0, 255);
				UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
			}
		}
		else
		{
			for(int s = 0; s < 3; s++)
			{		
				float k = ((PrevColor>>((2-s)*8))&0xff);
				if(s == 1)
					k = 255;
				else if(s == 2)
					k = 0;
				else
					k++;
				if(k >= 255 && s == 0)
					k = 0;
				Color <<= 8;
				Color += clamp((int)(k), 0, 255);
			}
		}

		*paColors = Color;

		aRects.HSplitTop(20.0f, &Label, &aRects);
		Label.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);

		float k = (g_Config.m_PlayerColorGhostAlpha) / 255.0f;
		k = DoScrollbarH(&g_Config.m_PlayerColorGhostAlpha, &Button, k);
		g_Config.m_PlayerColorGhostAlpha = (int)(k*255.0f);
		UI()->DoLabelScaled(&Label, paLabels[3], 14.0f, -1);	
		
		// skin info
		const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(g_Config.m_PlayerSkin));
		CTeeRenderInfo OwnSkinInfo;
		
		OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(g_Config.m_PlayerColorGhost);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(g_Config.m_PlayerColorGhost);
		OwnSkinInfo.m_ColorBody.a = g_Config.m_PlayerColorGhostAlpha/255.0f;
		OwnSkinInfo.m_ColorFeet.a = g_Config.m_PlayerColorGhostAlpha/255.0f;
		
		OwnSkinInfo.m_Size = 50.0f*UI()->Scale();
		
		aRects.HSplitTop(20.0f, 0, &Label);
		Label.VSplitLeft(230.0f, &Label, 0);
		Label.HSplitTop(50.0f, &Label, 0);
		RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x+30.0f, Label.y+28.0f));
		Label.HSplitTop(15.0f, 0, &Label);
		Label.VSplitLeft(70.0f, 0, &Label);
		UI()->DoLabelScaled(&Label, Localize("Antiping ghost"), 14.0f, -1, 150.0f);
	}
	
	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}
	
	TextRender()->Text(0, Other.x, Other.y-5, 18, Localize("Other"), -1);
	Other.HSplitTop(20.0f, 0, &Other);
	UiDoGetButtons(25, 28, Other);
}

void CMenus::RenderSettingsHudMod(CUIRect MainView)
{
	CUIRect Button;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(70.0f, &Button, 0);
	if(DoButton_CheckBox(&g_Config.m_HudModHideAll, Localize("Hide all"), g_Config.m_HudModHideAll, &Button))
		g_Config.m_HudModHideAll ^= 1;

	if(g_Config.m_HudModHideAll)
		return;
		
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(110.0f, &Button, 0);
	Button.VMargin(15.0f, &Button);
	if(DoButton_CheckBox(&g_Config.m_ClShowfps, Localize("Show fps"), g_Config.m_ClShowfps, &Button))
		g_Config.m_ClShowfps ^= 1;
			
	const char *paLabels[] = {
		Localize("Hide chat"),
		Localize("Hide kill messages"),
		Localize("Hide health and ammo"),
		Localize("Hide game timer"),
		Localize("Hide broadcast"),
		Localize("Hide sudden death"),
		Localize("Hide score hud"),
		Localize("Hide warmup timer"),
		Localize("Hide teambalance warning"),
		Localize("Hide voting"),
		Localize("Hide cursor"),
		Localize("Hide spectator hud")};
	int *paButton[] = {
		&g_Config.m_HudModHideChat,
		&g_Config.m_HudModHideKillMessages,
		&g_Config.m_HudModHideHealthAndAmmo,
		&g_Config.m_HudModHideGameTimer,
		&g_Config.m_HudModHideBroadcast,
		&g_Config.m_HudModHideSuddenDeath,
		&g_Config.m_HudModHideScoreHud,
		&g_Config.m_HudModHideWarmupTimer,
		&g_Config.m_HudModHideTeambalanceWarning,
		&g_Config.m_HudModHideVoting,
		&g_Config.m_HudModHideCursor,
		&g_Config.m_HudModHideSpectator};
			
	for(int s = 0; s < 12; s++)
	{
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VMargin(15.0f, &Button);
		if(DoButton_CheckBox(paButton[s], paLabels[s], *paButton[s], &Button))
			*paButton[s] ^= 1;
	}
}

void CMenus::RenderColHud(CUIRect MainView)
{
	CUIRect Button, Top, Center, Left, Right, Bottom, Label;	
	
	MainView.HSplitTop(40.0f, &Top, &MainView);
	RenderTools()->DrawUIRect(&Top, vec4(1,1,1,0.4f), CUI::CORNER_T, 5.0f);
	Top.Margin(10.0f, &Top);
	Top.HSplitTop(20.0f, &Button, &Top);	
	if(DoButton_CheckBox(&g_Config.m_ClStandartHud, Localize("Standart hud and chat"), g_Config.m_ClStandartHud, &Button))
		g_Config.m_ClStandartHud ^= 1;
	
	MainView.HSplitTop(10.0f, 0, &MainView);
	if(g_Config.m_ClStandartHud == 0)
	{
		//MainView.HSplitTop(200.0f, &Center, &Bottom);		
		MainView.HSplitBottom(115.0f, &MainView, &Bottom);
		Bottom.HSplitTop(10.0f, 0, &Bottom);
		
		MainView.VSplitLeft(305.0f, &Left, &Right);
		
		RenderTools()->DrawUIRect(&Left, vec4(1,1,1,0.25f), 0, 0.0f);
		Left.Margin(10.0f, &Left);
	//#start left bar
 		TextRender()->Text(0, Left.x, Left.y-5, 18, Localize("Hud"), -1);
		Left.HSplitTop(20.0f, 0, &Left); 
		
		RenderSettingsHudMod(Left);
	//#end left bar
		Right.VSplitLeft(10.0f, &Left, &Right);
		
		
		Right.HSplitTop(120.0f, &Right, &Center);
		Center.HSplitTop(10.0f, 0, &Center);
		
		RenderTools()->DrawUIRect(&Right, vec4(1,1,1,0.25f), 0, 10.0f);
		Right.Margin(10.0f, &Right);
		
		
	//#start top right bar
		TextRender()->Text(0, Right.x, Right.y-5, 18, Localize("New weapon bar"), -1);
		Right.HSplitTop(20.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);	
		if(DoButton_CheckBox(&g_Config.m_ClHudShowWeapon, Localize("Show weapon bar"), g_Config.m_ClHudShowWeapon, &Button))
			g_Config.m_ClHudShowWeapon ^= 1;	
			
		if(g_Config.m_ClHudShowWeapon == 1)
		{
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(15.0f, 0, &Button);	
			if(DoButton_CheckBox(&g_Config.m_ClHighlightWeaponBar, Localize("Highlight weapon bar"), g_Config.m_ClHighlightWeaponBar, &Button))
				g_Config.m_ClHighlightWeaponBar ^= 1;	
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(15.0f, 0, &Button);	
			if(DoButton_CheckBox(&g_Config.m_ClBulletWeaponBar, Localize("Show num of bullets"), g_Config.m_ClBulletWeaponBar, &Button))
				g_Config.m_ClBulletWeaponBar ^= 1;	
		}
			
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClHudShowAmmo, Localize("Show ammo bar"), g_Config.m_ClHudShowAmmo, &Button))
			g_Config.m_ClHudShowAmmo ^= 1;	 
	//#end top right bar
		
		RenderTools()->DrawUIRect(&Center, vec4(1,1,1,0.25f), 0, 10.0f);
		Center.Margin(10.0f, &Center);
				
	RenderTools()->DrawUIRect(&Bottom, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Bottom.Margin(10.0f, &Bottom);	
	
	//#start Center right bar
	
	TextRender()->Text(0, Bottom.x, Bottom.y-5, 18, Localize("Zoom"), -1);
	Bottom.HSplitTop(20.0f, 0, &Bottom);		
	
	CUIRect bLeft, bRight;
/* 		Bottom.HSplitTop(5.0f, &Button, &Bottom);
		Bottom.HSplitTop(20.0f, &Button, &Bottom);
		Button.VSplitLeft(110.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabel(&Label, Localize("Zoom"), 14.0f, -1);
		g_Config.m_ZoomMax = (int)(DoScrollbarH(&g_Config.m_ZoomMax, &Button, (g_Config.m_ZoomMax-110)/390.0f)*390.0f)+110; */
		
		Bottom.VSplitLeft(Bottom.w/2-0.5f, &bLeft, &bRight);
		bRight.VSplitLeft(10.0f, 0, &bRight);
		
		bRight.HSplitTop(5.0f, &Button, &bRight);
		bRight.HSplitTop(20.0f, &Button, &bRight);
		Button.VSplitLeft(150.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabel(&Label, Localize("Maximum Zoom"), 14.0f, -1);
		//g_Config.m_ZoomMax = (int)(DoScrollbarH(&g_Config.m_ZoomMax, &Button, (g_Config.m_ZoomMax-110)/390.0f)*390.0f)+110;
		g_Config.m_ZoomMax = DoCoolScrollbarH(&g_Config.m_ZoomMax, &Button, g_Config.m_ZoomMax, 110.0f, 500.0f);//(g_Config.m_ZoomMax-110)/390.0f)*390.0f)+110;
		g_Config.m_ZoomMax = (int)round(g_Config.m_ZoomMax/5.0f)*5;
		
		bRight.HSplitTop(5.0f, &Button, &bRight);
		bRight.HSplitTop(20.0f, &Button, &bRight);
		Button.VSplitLeft(150.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabel(&Label, Localize("Minimum Zoom"), 14.0f, -1);
		//g_Config.m_ZoomMin = (int)(DoScrollbarH(&g_Config.m_ZoomMin, &Button, (g_Config.m_ZoomMin-20)/70.0f)*70.0f)+20;
		g_Config.m_ZoomMin = DoCoolScrollbarH(&g_Config.m_ZoomMin, &Button, g_Config.m_ZoomMin, 20.0f, 90.0f);
		g_Config.m_ZoomMin = (int)round(g_Config.m_ZoomMin/5.0f)*5;
		
 	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	} 

	UiDoGetButtons(28, 31, bLeft);
	//#end Bottom right bar
			
		
 	//#start Center right bar
		TextRender()->Text(0, Center.x, Center.y-5, 18, Localize("Chat"), -1);
		Center.HSplitTop(20.0f, 0, &Center);
		
/*  		Center.HSplitTop(20.0f, &Button, &Center);
		// Notification weapons
		if(DoButton_CheckBox(&g_Config.m_ClNotificationWeapon, Localize("Notification weapons on pickup"), g_Config.m_ClNotificationWeapon, &Button))
			g_Config.m_ClNotificationWeapon ^= 1;  */

		Center.HSplitTop(20.0f, &Button, &Center);
		Button.VSplitLeft(150.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabel(&Label, Localize("Show time msg"), 14.0f, -1);
		//g_Config.m_ClChatShowtime = (int)(DoScrollbarH(&g_Config.m_ClChatShowtime, &Button, (g_Config.m_ClChatShowtime-5)/55.0f)*55.0f)+5;
		g_Config.m_ClChatShowtime = DoCoolScrollbarH(&g_Config.m_ClChatShowtime, &Button, g_Config.m_ClChatShowtime, 5.0f, 60.0f);
		
		Center.HSplitTop(20.0f, &Button, &Center);
		Button.VSplitLeft(150.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabel(&Label, Localize("Chat height"), 14.0f, -1);
		//g_Config.m_ClChatHeightlimit = (int)(DoScrollbarH(&g_Config.m_ClChatHeightlimit, &Button, (g_Config.m_ClChatHeightlimit-50)/250.0f)*250.0f)+50;
		g_Config.m_ClChatHeightlimit = DoCoolScrollbarH(&g_Config.m_ClChatHeightlimit, &Button, g_Config.m_ClChatHeightlimit, 50.0f, 184.0f);
				
		Center.HSplitTop(20.0f, &Button, &Center);
 		Center.HSplitTop(20.0f, &Button, &Center);
		// Notification weapons
		if(DoButton_CheckBox(&g_Config.m_ClChatEffects, Localize("Chat effects"), g_Config.m_ClChatEffects, &Button))
			g_Config.m_ClChatEffects ^= 1; 
			
		if(g_Config.m_ClChatEffects)
		{
			Center.VSplitLeft(10.0f, 0, &Center);
			Center.HSplitTop(20.0f, &Button, &Center);
			Button.VSplitLeft(150.0f, &Label, &Button);
			Button.HMargin(2.0f, &Button);
			UI()->DoLabel(&Label, Localize("Background blend"), 14.0f, -1);
			//g_Config.m_ClChatHeightlimit = (int)(DoScrollbarH(&g_Config.m_ClChatHeightlimit, &Button, (g_Config.m_ClChatHeightlimit-50)/250.0f)*250.0f)+50;
			g_Config.m_ClChatBackground = DoCoolScrollbarH(&g_Config.m_ClChatBackground, &Button, g_Config.m_ClChatBackground, 0.0f, 100.0f);
						
			Center.HSplitTop(20.0f, &Button, &Center);
			Button.VSplitLeft(150.0f, &Label, &Button);
			Button.HMargin(2.0f, &Button);
			UI()->DoLabel(&Label, Localize("Chat speed"), 14.0f, -1);
			//g_Config.m_ClChatHeightlimit = (int)(DoScrollbarH(&g_Config.m_ClChatHeightlimit, &Button, (g_Config.m_ClChatHeightlimit-50)/250.0f)*250.0f)+50;
			g_Config.m_ClChatSpeed = DoCoolScrollbarH(&g_Config.m_ClChatSpeed, &Button, g_Config.m_ClChatSpeed, 4.0f, 10.0f);
		}
			
	//#end Center right bar 
	
	}
	else
	{
		g_Config.m_ClHudShowWeapon 		= 0;
		g_Config.m_ClHudShowAmmo 		= 1;
		g_Config.m_ClChatShowtime 		= 15;
		g_Config.m_ClChatHeightlimit 		= 80;
		//g_Config.m_ClNotificationWeapon		= 0;
		g_Config.m_ClBulletWeaponBar		= 1;
		
 		g_Config.m_HudModHideAll 		= 0;
		g_Config.m_ClShowfps 			= 0;
		g_Config.m_HudModHideChat		= 0;
		g_Config.m_HudModHideKillMessages	= 0;
		g_Config.m_HudModHideHealthAndAmmo	= 0;
		g_Config.m_HudModHideGameTimer		= 0;
		g_Config.m_HudModHideBroadcast		= 0;
		g_Config.m_HudModHideSuddenDeath	= 0;
		g_Config.m_HudModHideScoreHud		= 0;
		g_Config.m_HudModHideWarmupTimer	= 0;
		g_Config.m_HudModHideTeambalanceWarning	= 0;
		g_Config.m_HudModHideVoting		= 0;
		g_Config.m_HudModHideCursor		= 0;	
		g_Config.m_HudModHideSpectator	= 0; 
		
		MainView.HSplitTop(0.0f, &Center, &Bottom);
	}		
}

void CMenus::RenderSettings(CUIRect MainView)
{
	static int s_SettingsPage = 0;
	static int h_SettingsPage = 0;

	// render background
	CUIRect Temp, TabBar, RestartWarning, DownBar;
	MainView.HSplitBottom(15.0f, &MainView, &RestartWarning);
	MainView.VSplitRight(120.0f, &MainView, &TabBar);	
	MainView.HSplitBottom(24.0f, &MainView, &DownBar);

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 5.0f);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 5.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);

	CUIRect Button;

	const char *aTabs[] = {
		Localize("General"),
		Localize("Game"),
		("Tee"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		Localize("TDTW")};

	int NumTabs = (int)(sizeof(aTabs)/sizeof(*aTabs));

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(DoButton_MenuTab(aTabs[i], aTabs[i], s_SettingsPage == i, &Button, CUI::CORNER_R))
			s_SettingsPage = i;
	}

	MainView.Margin(10.0f, &MainView);

	if(s_SettingsPage == 0)
		RenderSettingsGeneral(MainView);
	if(s_SettingsPage == 1)
		RenderSettingsGame(MainView);
	else if(s_SettingsPage == 2)
		RenderSettingsTee(MainView);
	else if(s_SettingsPage == 3)
		RenderSettingsControls(MainView);
	else if(s_SettingsPage == 4)
		RenderSettingsGraphics(MainView);
	else if(s_SettingsPage == 5)
		RenderSettingsSound(MainView);
	else if(s_SettingsPage == 6)
	{
		const char *aTabs2[] = {
			Localize("Features"),
			Localize("Hud")};

		int NumTabs = (int)(sizeof(aTabs2)/sizeof(*aTabs2));

		for(int i = 0; i < NumTabs; i++)
		{
			DownBar.VSplitLeft(20, 0, &DownBar);
			if(i == 1)
				DownBar.VSplitLeft(120, &Button, &DownBar);
			else		
				DownBar.VSplitLeft(100, &Button, &DownBar);
			if(DoButton_ColSettingsTab(aTabs2[i], aTabs2[i], h_SettingsPage == i, &Button))
				h_SettingsPage = i;
		}

 		if(h_SettingsPage == 0)
			RenderColFeat(MainView);
		else if(h_SettingsPage == 1)
			RenderColHud(MainView); 
	}

	if(m_NeedRestartGraphics || m_NeedRestartSound)
		UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 15.0f, -1);
}
