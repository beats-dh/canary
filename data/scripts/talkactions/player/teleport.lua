local talk = TalkAction("/ir")

function talk.onSay(player, words, param)
local config = {
pz = false, -- players precisam estar em protection zone para usar? (true or false)
battle = true, -- players deve estar sem battle (true or false)
custo = true, -- se os teleport irão custa (true or false)
need_level = false, -- se os teleport irão precisar de level (true or false)
premium = false -- se precisa ser premium account (true or false)
}
--[[ Config lugares]]--
local lugar = {
		["promoOrc"] = { -- nome do lugar
		pos = {x=1696, y=1280, z=7},level = 1,price = 0, premium = 0},
		["anfallas"] = { -- nome do lugar
		pos = {x=982, y=1298, z=7},level = 1,price = 0, premium = 0},
		["vamp2"] = { -- nome do lugar
		pos = {x=1818, y=537, z=8},level = 1,price = 0, premium = 1},
		["dol"] = { -- nome do lugar
		pos = {x=1206, y=753, z=7},level = 1,price = 0, premium = 0},
		["edoras"] = { -- nome do lugar
		pos = {x=1066, y=1008, z=5},level = 1,price = 1, premium = 0},
		["bree"] = { -- nome do lugar
		pos = {x=742, y=537, z=7},level = 1,price = 0, premium = 0},
		["mordor"] = { -- nome do lugar
		pos = {x=1519, y=1236, z=7},level = 1,price = 0, premium = 0},
		["belfallas"] = { -- nome do lugar
		pos = {x=1187, y=1514, z=7},level = 1,price = 0, premium = 0},
		["ashenport"] = { -- nome do lugar
		pos = {x=334, y=611, z=6},level = 1,price = 0, premium = 0},
		["esg"] = { -- nome do lugar
		pos = {x=1413, y=502, z=7},level = 1,price = 0, premium = 0},
		["argond"] = { -- nome do lugar
		pos = {x=607, y=867, z=7},level = 1,price = 0, premium = 0},
		["moria"] = { -- nome do lugar
		pos = {x=1018, y=636, z=7},level = 1,price = 0, premium = 0},
		["forod"] = { -- nome do lugar
		pos = {x=709, y=105, z=7},level = 1,price = 0, premium = 0},
		["condado"] = { -- nome do lugar
		pos = {x=621, y=533, z=7},level = 1,price = 0, premium = 0},
		["dunedain"] = { -- nome do lugar
		pos = {x=1591, y=371, z=6},level = 1,price = 0, premium = 0},
		["minas"] = { -- nome do lugar
		pos = {x=1345, y=1371, z=6},level = 1,price = 0, premium = 1},
		["troll1"] = { -- nome do lugar
		pos = {x=1071, y=877, z=8},level = 1,price = 0, premium = 0},
		["troll2"] = { -- nome do lugar
		pos = {x=1168, y=922, z=8},level = 1,price = 0, premium = 0},
		["rot"] = { -- nome do lugar
		pos = {x=1142, y=930, z=9},level = 1,price = 0, premium = 0},
		["pirata"] = { -- nome do lugar
		pos = {x=193, y=651, z=7},level = 1,price = 0, premium = 0},
		["mino1"] = { -- nome do lugar
		pos = {x=740, y=415, z=8},level = 1,price = 0, premium = 0},
		["mino2"] = { -- nome do lugar
		pos = {x=556, y=570, z=7},level = 1,price = 0, premium = 0},
		["pantano"] = { -- nome do lugar
		pos = {x=1247, y=1074, z=7},level = 1,price = 0, premium = 0},
		["eriador"] = { -- nome do lugar
		pos = {x=825, y=724, z=7},level = 1,price = 0, premium = 0},
		["dwarf1"] = { -- nome do lugar
		pos = {x=1410, y=438, z=7},level = 1,price = 0, premium = 0},
		["dwarf2"] = { -- nome do lugar
		pos = {x=1000, y=617, z=7},level = 1,price = 0, premium = 0},
		["macacos1"] = { -- nome do lugar
		pos = {x=1270, y=753, z=7},level = 1,price = 0, premium = 0},
		["macacos2"] = { -- nome do lugar
		pos = {x=624, y=610, z=7},level = 1,price = 0, premium = 0},
		["slime1"] = { -- nome do lugar
		pos = {x=1362, y=1426, z=6},level = 1,price = 0, premium = 1},
		["slime2"] = { -- nome do lugar
		pos = {x=1365, y=475, z=7},level = 1,price = 0, premium = 0},
		["slime3"] = { -- nome do lugar
		pos = {x=433, y=651, z=7},level = 1,price = 0, premium = 0},
		["ghoul"] = { -- nome do lugar
		pos = {x=1136, y=1052, z=7},level = 1,price = 0, premium = 0},
		["bandit1"] = { -- nome do lugar
		pos = {x=1120, y=1090, z=7},level = 1,price = 0, premium = 0},
		["bandit2"] = { -- nome do lugar
		pos = {x=710, y=383, z=7},level = 1,price = 0, premium = 0},
		["cyc1"] = { -- nome do lugar
		pos = {x=1038, y=869, z=7},level = 1,price = 0, premium = 0},
		["cyc2"] = { -- nome do lugar
		pos = {x=1104, y=788, z=10},level = 1,price = 0, premium = 1},
		["cyc3"] = { -- nome do lugar
		pos = {x=1276, y=914, z=6},level = 1,price = 0, premium = 0},
		["stonegolem"] = { -- nome do lugar
		pos = {x=1324, y=1047, z=7},level = 1,price = 0, premium = 0},
		["dworc1"] = { -- nome do lugar
		pos = {x=640, y=214, z=7},level = 1,price = 0, premium = 0},
		["eregion"] = { -- nome do lugar
		pos = {x=901, y=725, z=7},level = 1,price = 0, premium = 0},
		["rhun"] = { -- nome do lugar
		pos = {x=1759, y=813, z=7},level = 1,price = 0, premium = 1},
		["dragonvip"] = { -- nome do lugar
		pos = {x=1539, y=723, z=6},level = 1,price = 0, premium = 1},
		["erebor"] = { -- nome do lugar
		pos = {x=1415, y=424, z=4},level = 1,price = 0, premium = 0},
		["mirkwood"] = { -- nome do lugar
		pos = {x=1253, y=637, z=7},level = 1,price = 0, premium = 0},
		["forochel"] = { -- nome do lugar
		pos = {x=507, y=116, z=6},level = 1,price = 0, premium = 0},
		["carn"] = { -- nome do lugar
		pos = {x=676, y=199, z=6},level = 1,price = 0, premium = 0},
		["enedwaith"] = { -- nome do lugar
		pos = {x=867, y=1004, z=7},level = 1,price = 0, premium = 0},
		["elven"] = { -- nome do lugar
		pos = {x=999, y=582, z=2},level = 1,price = 0, premium = 0},
		["wyvern"] = { -- nome do lugar
		pos = {x=823, y=354, z=6},level = 1,price = 0, premium = 0},
		["corsario"] = { -- nome do lugar
		pos = {x=872, y=1472, z=6},level = 1,price = 0, premium = 1},
		["dunland"] = { -- nome do lugar
		pos = {x=808, y=930, z=7},level = 1,price = 0, premium = 0},
		["beleghost"] = { -- nome do lugar
		pos = {x=182, y=589, z=6},level = 1,price = 0, premium = 1},
		["evendim"] = { -- nome do lugar
		pos = {x=580, y=242, z=6},level = 1,price = 0, premium = 0},
		["bonebeast1"] = { -- nome do lugar
		pos = {x=1302, y=658, z=7},level = 1,price = 0, premium = 0},
		["bonebeast2"] = { -- nome do lugar
		pos = {x=1327, y=1300, z=5},level = 1,price = 0, premium = 1},
		["blacknight1"] = { -- nome do lugar
		pos = {x=861, y=416, z=7},level = 1,price = 0, premium = 0},
		["blacknight2"] = { -- nome do lugar
		pos = {x=1257, y=1371, z=5},level = 1,price = 0, premium = 1},
		["hero1"] = { -- nome do lugar
		pos = {x=1137, y=1453, z=7},level = 1,price = 0, premium = 0},
		["hero2"] = { -- nome do lugar
		pos = {x=1200, y=1440, z=7},level = 1,price = 0, premium = 0},
		["hero3"] = { -- nome do lugar
		pos = {x=1257, y=1367, z=5},level = 1,price = 0, premium = 1},
		["hydra1"] = { -- nome do lugar
		pos = {x=667, y=724, z=6},level = 1,price = 0, premium = 0},
		["hydra2"] = { -- nome do lugar
		pos = {x=526, y=641, z=6},level = 1,price = 0, premium = 0},
		["hydra3"] = { -- nome do lugar
		pos = {x=498, y=686, z=6},level = 1,price = 0, premium = 0},
		["lich"] = { -- nome do lugar
		pos = {x=1470, y=1034, z=7},level = 1,price = 0, premium = 0},
		["icewitch1"] = { -- nome do lugar
		pos = {x=723, y=74, z=7},level = 1,price = 0, premium = 0},
		["crystal"] = { -- nome do lugar
		pos = {x=696, y=80, z=7},level = 1,price = 0, premium = 0},
		["barbarian"] = { -- nome do lugar
		pos = {x=951, y=98, z=7},level = 1,price = 0, premium = 0},
		["dragon1"] = { -- nome do lugar
		pos = {x=679, y=327, z=7},level = 1,price = 0, premium = 0},
		["dragon2"] = { -- nome do lugar
		pos = {x=368, y=660, z=7},level = 1,price = 0, premium = 0},
		["vamp"] = { -- nome do lugar
		pos = {x=1275, y=1382, z=7},level = 1,price = 0, premium = 1},
		["turtle"] = { -- nome do lugar
		pos = {x=452, y=509, z=7},level = 1,price = 0, premium = 0},
		["purga"] = { -- nome do lugar
		pos = {x=1085, y=346, z=7},level = 1,price = 0, premium = 0},
		["northern"] = { -- nome do lugar
		pos = {x=1541, y=142, z=7},level = 1,price = 0, premium = 1},
		["northern2"] = { -- nome do lugar
		pos = {x=1541, y=120, z=7},level = 1,price = 0, premium = 1},
		["ered"] = { -- nome do lugar
		pos = {x=1282, y=90, z=7},level = 1,price = 0, premium = 1},
		["orodruin"] = { -- nome do lugar
		pos = {x=1578, y=1208, z=0},level = 1,price = 0, premium = 1},
		["ice"] = { -- nome do lugar
		pos = {x=859, y=127, z=7},level = 1,price = 0, premium = 0},
		["nimrais"] = { -- nome do lugar
		pos = {x=740, y=1210, z=0},level = 1,price = 0, premium = 1},
		["defiler"] = { -- nome do lugar
		pos = {x=642, y=492, z=7},level = 1,price = 0, premium = 0},
		["behedemon"] = { -- nome do lugar
		pos = {x=996, y=611, z=10},level = 1,price = 0, premium = 0},
		["harlond"] = { -- nome do lugar
		pos = {x=379, y=812, z=5},level = 1,price = 0, premium = 1},
		["riv"] = { -- nome do lugar
		pos = {x=1052, y=541, z=4},level = 1,price = 0, premium = 0},
		["orc"] = { -- nome do lugar
		pos = {x=1275, y=804, z=7},level = 1,price = 0, premium = 0},
		["icewitch2"] = { -- nome do lugar
		pos = {x=1306, y=1290, z=3},level = 1,price = 0, premium = 1},
		["roshamuul"] = { -- nome do lugar
		pos = {x=192, y=1377, z=7},level = 1,price = 0, premium = 1},
		["quara"] = { -- nome do lugar
		pos = {x=1375, y=246, z=11},level = 1,price = 0, premium = 1},
		["erech"] = { -- nome do lugar
		pos = {x=681, y=1295, z=5},level = 1,price = 0, premium = 1},
		["cormaya"] = { -- nome do lugar
		pos = {x=962, y=1428, z=7},level = 1,price = 0, premium = 1},
		["minoisland"] = { -- nome do lugar
		pos = {x=556, y=1629, z=7},level = 1,price = 0, premium = 1},
		["feyrist"] = { -- nome do lugar
		pos = {x=197, y=1021, z=6},level = 1,price = 0, premium = 1}
		}

local a = lugar[param]

if config.need_level == true and player:getLevel() < a.level then
	player:sendTextMessage(MESSAGE_EVENT_DEFAULT, "Você precisa do level ".. a.level .." para ir até essa hunt.")
	player:getPosition():sendMagicEffect(CONST_ME_POFF)
	return false
end

if not a then
	player:popupFYI(
					"[+] As cidades que você pode ir sâo: promo, edoras, belfallas, ashenport, bree, riv \z
					dol, esg, zargond, moria, mordor, forod, condado, dunedain, anfallas, minas. \n\n\z
					
					As hunts que você pode ir sâo: \n\n\z
					
					[+] Level 0 - 30 [+] \n\z
					troll1, troll2, rot, pirata, mino1, mino2, pantano, eriador, dwarf1, dwarf2, macacos1, \z
					macacos2, slime1, slime2, slime3, ghoul, bandit1, bandit2, cyc1, cyc2, cyc3, dworc1, \z
					stonegolem, eregion, promoOrc. \n\n\z
					
					[+] Level 31-100 [+] \n\z
					rhun, dragonvip, erebor, mirkwood, forochel, carn, enedwaith, elven, wyvern, corsario, dunland, \z
					beleghost, evendim, bonebeast1, bonebeast2, orc, blacknight1, blacknight2, hero1, hero2, hero3 \z
					hydra1, hydra2, hydra3, lich, icewitch1, icewitch2, crystal, barbarian, dragon1, dragon2, vamp, turtle, quara. \n\n\z
					
					[+] Level 100 - 200 [+] \n\z
					frostdragon, subsolodeargond, northern, ered, orodruin, ice, nimrais, defiler, behedemon, harlond, northern2, erech. \n\n\z
					
					[+] Level 200+ [+] \n\z
					belzebuth, elementos, formorgar, bloody, submundo, item400, icevip, ghastlydragon, elementais,\z
					prision, hellhound, iceserpente, lizard, cerberus, roshamuul, ancalagon, scarab, grimvip\n\n\z
					
					[+] Apenas Vip [+] \n\z
					vamp2, minas, slime1, cyc2, rhun, dragonvip, corsario, beleghost, bonebeast2, blacknight2, hero3, vamp, northern, northern2, \z
					ered, orodruin, nimrais, harlond, icewitch2, roshamuul, quara, erech, cormaya, minoisland, feyrist."
					)
	player:getPosition():sendMagicEffect(CONST_ME_POFF)
	return false
end

if config.custo == true and player:getMoney() < a.price then
	player:sendTextMessage(MESSAGE_EVENT_DEFAULT, "Você precisa de ".. a.price .." money para ir até essa hunt.")
	player:getPosition():sendMagicEffect(CONST_ME_POFF)
	return false
end

if player:getPremiumDays() <= 0 and config.premium == true then
	player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Apenas contas vip tem esse recurso.")
	player:getPosition():sendMagicEffect(CONST_ME_POFF)
	return false
end

if  player:hasCondition(CONDITION_INFIGHT) and config.battle == true then
	player:sendTextMessage(MESSAGE_EVENT_DEFAULT, "Você não pode se teleportar em uma batalha.")
	player:getPosition():sendMagicEffect(CONST_ME_POFF)
	return false
end

	player:getPosition():sendMagicEffect(27)
	player:teleportTo(a.pos)
	player:getPosition():sendMagicEffect(40)
	player:removeMoney(a.price)

end

talk:separator(" ")
talk:register()
