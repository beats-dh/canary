function Monster:onDropLoot(corpse)
	if configManager.getNumber(configKeys.RATE_LOOT) == 0 then
		return
	end

	local player = Player(corpse:getCorpseOwner())
	local mType = self:getType()
	if not player or player:getStamina() > 840 then
		local monsterLoot = mType:getLoot()
		for i = 1, #monsterLoot do
			local item = corpse:createLootItem(monsterLoot[i])
			if not item then
				Spdlog.info("Could not add loot item to corpse of monster '".. mType:getName() .."'")
			end
		end

		if player then
			local text = {}
			local oldClientText = ""
			local version = player:getClient().version
			if self:getName():lower() == (Game.getBoostedCreature()):lower() then
				 text = ("Loot of %s: %s (boosted loot)"):format(mType:getNameDescription(), corpse:getContentDescription())
				 if party or version < 1200 then
					oldClientText = ("Loot of %s: %s (boosted loot)"):format(mType:getNameDescription(), corpse:getContentDescription(true))
				 end
			else
				 text = ("Loot of %s: %s"):format(mType:getNameDescription(), corpse:getContentDescription())
				 if party or version < 1200 then
					oldClientText = ("Loot of %s: %s"):format(mType:getNameDescription(), corpse:getContentDescription(true))
				 end
			end
			local party = player:getParty()
			if party then
				party:broadcastPartyLoot(text, oldClientText)
			else
				player:sendTextMessage(MESSAGE_LOOT, version >= 1200 and text or oldClientText)
				if version < 1200 then
					player:sendTextMessage(MESSAGE_GUILD, oldClientText, 9)
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
end
