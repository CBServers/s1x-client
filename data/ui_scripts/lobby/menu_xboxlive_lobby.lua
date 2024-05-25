local MPLobbyPublic = LUI.mp_menus.MPLobbyPublic

function StartButtonAction(f2_arg0, f2_arg1)
	Engine.SetDvarInt("party_minplayers", 1)
end

function StartButtonText(f5_arg0, f5_arg1)
	f5_arg0:processEvent({
		name = "refresh_disabled"
	})
	f5_arg0:setText(Engine.Localize("@LUA_MENU_START_GAME"))
end

function OnLeaveLobby(f6_arg0)
	LUI.MarketingPanel.ClearViewedMessages({
		LUI.MarketingLocation.Lobby
	})
	LUI.FlowManager.RequestLeaveMenu(f6_arg0)
end

function OnGameSetup(f10_arg0, f10_arg1)
	LUI.FlowManager.RequestAddMenu(f10_arg0, "gamesetup_menu_main", true, f10_arg1.controller, false)
end

function menu_xboxlive_lobby(f7_arg0, f7_arg1)
	local f7_local0 = false
	if not Engine.IsZombiesMode() and Engine.GetDvarBool("ui_opensummary") then
		f7_local0 = true
	end
	if Engine.IsZombiesMode() then
		ZombiesUpdateMapBkg()
	end
	local f7_local1 = LUI.MPLobbyBase.new(f7_arg0, {
		menu_title = "@PLATFORM_UI_HEADER_PLAY_MP_CAPS",
		has_match_summary = true,
		has_new_item_usage_widget = true
	}, true)
	f7_local1:setClass(LUI.MPLobbyPublic)
	if Engine.IsMultiplayer() then
		f7_local1:AddReadyUpButton("pt_AliensReadyUpPublicInUse", StartButtonAction, false, StartButtonText)
		-- f7_local1:AddButton( "@LUA_MENU_GAME_SETUP", OnGameSetup ) -- WIP
	end
	if Engine.IsCoreMode() then
		f7_local1:AddNewItemsWidget()
		f7_local1:AddCACButton()
		f7_local1:AddCAOButton()
		f7_local1:AddArmoryButton()
	end
	f7_local1:AddOptionsButton()
	if not f7_local1:CheckAddMapAndMarketingPanels(f7_local0) then
		f7_local1:registerEventHandler("CheckAddMapAndMarketingPanels", function(element, event)
			LUI.MPLobbyPublic.CheckAddMapAndMarketingPanels(element, f7_local0)
		end)
		local self = LUI.UITimer.new(100, "CheckAddMapAndMarketingPanels")
		self.id = "MPLobbyPublic_add_map_timer"
		f7_local1.mapTimer = self
		f7_local1:addElement(self)
	end
	f7_local1:registerEventHandler("exit_public_lobby", OnLeaveLobby)
	f7_local1:registerEventHandler("player_joined", Cac.PlayerJoinedEvent)
	f7_local1:registerEventHandler("loadout_request", Cac.PlayerJoinedEvent)
	Lobby.EnteredLobby()
	if f7_local0 then
		LUI.InventoryUtils.ProcessLootExpiration()
	end
	return f7_local1
end

LUI.MenuBuilder.m_types_build["menu_xboxlive_lobby"] = menu_xboxlive_lobby

LUI.FlowManager.RegisterMenuStack("menu_xboxlive_lobby", function()
	if Engine.IsCoreMode() and Playlist.GetPreselectedCategoryClass() ~= 8 then
		return {
			"mp_main_menu",
			"menu_xboxlive",
			"FindGameMenu",
			"FindGameSubMenu"
		}
	else
		return {
			"mp_main_menu",
			"menu_xboxlive",
			"FindGameMenu"
		}
	end
end)
VLobby.InitMenuMode("menu_xboxlive_lobby", VirtualLobbyModes.LUI_MODE_LOBBY, function()
	if LUI.MPLobbyBase.UseReadyUp then
		Lobby.ClearLocalReadyUpFlag()
	end
	local f10_local0 = PersistentBackground.StackFunc(nil)
	f10_local0()
end, LUI.MPLobbyBase.CollectGarbage)

LUI.FlowManager.RegisterStackResumeBehaviour("menu_xboxlive_lobby", PersistentBackground.StackFunc(nil))
