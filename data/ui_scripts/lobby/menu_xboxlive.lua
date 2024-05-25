local Lobby = Lobby
local MPLobbyOnline = LUI.mp_menus.MPLobbyOnline
local MenuData = LUI.mp_menus.MenuData
local MPLobbyUtils = LUI.mp_menus.MPLobbyUtils

game:addlocalizedstring("LUA_MENU_SERVERLIST", "Server List")
game:addlocalizedstring("LUA_MENU_SERVERLIST_DESC", "Browse available servers.");

LeaveXboxLive = function(f5_arg0)
	local f73_local0 = Engine.GetFirstActiveController()
	if f73_local0 ~= nil then
		Engine.ExecNow("xstopprivateparty", f73_local0)
		Cac.NotifyVirtualLobby("leave_lobby", Engine.GetXUIDByController(f73_local0))
		Engine.SetSplitScreen(false)
		Engine.ExecNow("forcesplitscreencontrol menu_xboxlive_CLOSE", f73_local0)
	else
		Engine.Exec("xstopprivateparty")
	end
end

function LeaveLobby(a1)
	LUI.MarketingPanel.ClearViewedMessages({ LUI.MarketingLocation.CaC, LUI.MarketingLocation.PlayOnline,
		LUI.MarketingLocation.CaO })
	LeaveXboxLive()
	LUI.FlowManager.RequestLeaveMenuByName("menu_xboxlive_lobby", nil, true)
end

function menu_xboxlive(a1, a2)
	Engine.SetDvarBool("ui_opensummary", false)
	local menu = LUI.MPLobbyBase.new(a1, {
		menu_title = "@PLATFORM_UI_HEADER_PLAY_MP_CAPS",
		memberListState = Lobby.MemberListStates.Prelobby,
		has_new_item_usage_widget = true
	})

	menu:setClass(LUI.MPLobbyOnline)

	menu.handleGamepadButton = MPLobbyOnline.menu_xboxlive_handleGamepadButton
	if Engine.IsCoreMode() then
		menu:AddNewItemsWidget()
	end

	-- server list button
	local serverListButton = menu:AddButton("@LUA_MENU_SERVERLIST", function(a1, a2)
		LUI.FlowManager.RequestAddMenu(a1, "menu_systemlink_join", true, nil)
	end)
	serverListButton:setDisabledRefreshRate(500)

	-- private match button
	privateMatchButton = menu:AddButton("@MENU_PRIVATE_MATCH", MPLobbyOnline.OnPrivateMatch,
		MPLobbyOnline.disablePrivateMatchButton)
	privateMatchButton:rename("menu_xboxlive_private_match")
	privateMatchButton:setDisabledRefreshRate(500)

	-- combat training button
	if Engine.IsCoreMode() then
		game:addlocalizedstring("LUA_MENU_COMBAT", "Combat Training");
		game:addlocalizedstring("LUA_MENU_COMBAT_DESC", "Rank up offline with bots.");

		local FindGameButton = menu:AddButton("@LUA_MENU_COMBAT",
			MPLobbyOnline.OnPublicMatch, MPLobbyOnline.disablePublicMatchButton)
		FindGameButton:rename("menu_xboxlive_find_game")
		FindGameButton:setDisabledRefreshRate(500)
	end

	if Engine.IsCoreMode() then
		menu:AddCACButton()
		menu:AddCAOButton()
		menu:AddArmoryButton()
	end

	if not Engine.IsCoreMode() then
		local leaderboardButton = menu:AddButton("@LUA_MENU_LEADERBOARD", "OpLeaderboardMain")
		leaderboardButton:rename("OperatorMenu_leaderboard")
	end

	if Engine.IsZombiesMode() then
		menu:AddButton("@ZOMBIES_MENU_MOVIES", "ZombiesMoviesMenu")
	end

	menu:AddOptionsButton()
	local natType = Lobby.GetNATType()
	if natType then
		menu:AddHelp({
			name = "add_button_helper_text",
			button_ref = "nat",
			helper_text = Engine.Localize("NETWORK_YOURNATTYPE", natType),
			side = "left",
			clickable = false
		})
	end
	if Engine.IsZombiesMode() then
		menu:AddZombiesStats(true)
	end

	menu.isSignInMenu = true
	menu:registerEventHandler("gain_focus", LUI.MPLobbyOnline.OnGainFocus)
	menu:registerEventHandler("player_joined", Cac.PlayerJoinedEvent)
	menu:registerEventHandler("exit_live_lobby", LeaveLobby)

	if PersistentBackground.IsCurrent(PersistentBackground.Variants.AARBackground) then
		PersistentBackground.Hide(PersistentBackground.Duration)
	end

	if Engine.IsCoreMode() then
		Engine.ExecNow("eliteclan_refresh", Engine.GetFirstActiveController())
	end

	return menu
end

LUI.MenuBuilder.m_types_build["menu_xboxlive"] = menu_xboxlive
