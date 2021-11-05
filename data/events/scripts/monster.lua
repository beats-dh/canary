function Monster:onDropLoot(corpse)
	local player = Player(corpse:getCorpseOwner())
	local autolooted = ""
	if not player or player:getStamina() > 840 then
		local monsterLoot = mType:getLoot()
		for i = 1, #monsterLoot do
			local item = corpse:createLootItem(monsterLoot[i])
			if item < 0 then
				print('[Warning] DropLoot:', 'Could not add loot item to corpse. itemId: '..monsterLoot[i].itemId)
			else
				-- autoloot
				if item > 0 then
					local tmpItem = Item(item)
					if player and player:getAutoLootItem(tmpItem:getId()) and configManager.getNumber(configKeys.AUTOLOOT_MODE) == 1 then
						if tmpItem:moveTo(player) then
							autolooted = string.format("%s, %s", autolooted, tmpItem:getNameDescription())
						end
					end
				end

		if player then
			local text = ("Loot of %s: "):format(mType:getNameDescription())
			-- autoloot
			local lootMsg = corpse:getContentDescription()
			if autolooted ~= "" and corpse:getContentDescription() == "nothing" then
				lootMsg = autolooted:gsub(",", "", 1) .. " that was autolooted"
			elseif autolooted ~= "" then
				lootMsg = corpse:getContentDescription() .. " and " .. autolooted:gsub(",", "", 1) .. " was auto looted"
			end
			text = string.format("%s%s", text, lootMsg)
			local party = player:getParty()
			if party then
				if autolooted ~= "" then
					text = string.format("%s by %s", text, player:getName())
				end
				party:broadcastPartyLoot(text)
			else
				player:sendTextMessage(MESSAGE_LOOT, text)
			end
		end
	end
end
	else
		local text = ("Loot of %s: nothing (due to low stamina)"):format(mType:getNameDescription())
		local party = player:getParty()
		if party then
			party:broadcastPartyLoot(text)
		else
			player:sendTextMessage(MESSAGE_LOOT, text)
		end
	end
	if configManager.getNumber(configKeys.AUTOLOOT_MODE) == 2 then
		corpse:setActionId(500)
	end
end
