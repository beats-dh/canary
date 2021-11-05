function Container.isContainer(self)
	return true
end

--[[
	return values for autoloot
	0 = Did not drop the item. No error
	-1 = For some reason, the item can not be created.
	> 0 = UID
]]

function Container.createLootItem(self, item, boolCharm)
	if self:getEmptySlots() == 0 then
		return 0
	end

	local itemCount = 0
	local randvalue = getLootRandom()
	local lootBlockType = ItemType(item.itemId)
	local chanceTo = item.chance

	if not lootBlockType then
		return
	end

	if boolCharm and lootBlockType:getType() == ITEM_TYPE_CREATUREPRODUCT then
		chanceTo = (chanceTo * (GLOBAL_CHARM_GUT + 100))/100
	end

	if randvalue < chanceTo then
		if lootBlockType:isStackable() then
			local maxc, minc = item.maxCount or 1, item.minCount or 1
			itemCount = math.max(0, randvalue % (maxc - minc + 1)) + minc			
		else
			itemCount = 1
		end
	end
	
	local tmpItem = false
	if itemCount > 0 then
		tmpItem = self:addItem(item.itemId, math.min(itemCount, 100))
		if not tmpItem then
			return -1
		end

		if tmpItem:isContainer() then
			for i = 1, #item.childLoot do
				if not tmpItem:createLootItem(item.childLoot[i]) then
					tmpItem:remove()
					return -1
				end
			end
		end

		if item.subType ~= -1 then
			tmpItem:transform(item.itemId, item.subType)
		elseif lootBlockType:isFluidContainer() then
			tmpItem:transform(item.itemId, 0)
		end

		if item.actionId ~= -1 then
			tmpItem:setActionId(item.actionId)
		end

		if item.text and item.text ~= "" then
			tmpItem:setText(item.text)
		end
	end
	return tmpItem and tmpItem.uid or 0
end
