local teste = MoveEvent()
teste:type("stepin")

function teste.onStepIn(creature, item, fromPosition)

	local player = creature:getPlayer()
	if not player then
		return true
	end

    local function teleport()
        pos = {x=1696, y=1280, z=7}
        player:teleportTo(pos)
        return false
    end
	addEvent(teleport, 10000)

end

teste:aid(3031)
teste:register()