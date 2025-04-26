/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "map/map.hpp"

#include "config/configmanager.hpp"
#include "creatures/monsters/monster.hpp"
#include "creatures/players/player.hpp"
#include "game/game.hpp"
#include "game/scheduling/dispatcher.hpp"
#include "game/zones/zone.hpp"
#include "io/iomap.hpp"
#include "io/iomapserialize.hpp"
#include "lua/callbacks/event_callback.hpp"
#include "lua/callbacks/events_callbacks.hpp"
#include "map/spectators.hpp"
#include "utils/astarnodes.hpp"

void Map::load(std::string_view identifier, const Position &pos) {
	try {
		path = identifier;
		IOMap::loadMap(this, pos);
	} catch (const std::exception &e) {
		g_logger().warn("[Map::load] - The map in folder {} is missing or corrupted: {}", identifier, e.what());
	}
}

void Map::loadMap(std::string_view identifier, bool mainMap, bool loadHouses, bool loadMonsters, bool loadNpcs, bool loadZones, const Position &pos) {
	// Only download map if is loading the main map and it is not already downloaded
	if (mainMap && g_configManager().getBoolean(TOGGLE_DOWNLOAD_MAP) && !std::filesystem::exists(identifier)) {
		const auto mapDownloadUrl = g_configManager().getString(MAP_DOWNLOAD_URL);
		if (mapDownloadUrl.empty()) {
			g_logger().warn("Map download URL in config.lua is empty, download disabled");
		}

		if (CURL* curl = curl_easy_init(); curl && !mapDownloadUrl.empty()) {
			g_logger().info("Downloading {} to world folder", g_configManager().getString(MAP_NAME) + ".otbm");
			FILE* otbm = fopen(std::string(identifier).c_str(), "wb");
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_URL, mapDownloadUrl.c_str());
			curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, otbm);
			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			fclose(otbm);
		}
	}

	// Load the map
	load(identifier, pos);

	// Only create items from lua functions if is loading main map
	// It needs to be after the load map to ensure the map already exists before creating the items
	if (mainMap) {
		// Create items from lua scripts per position
		// Example: ActionFunctions::luaActionPosition
		g_game().createLuaItemsOnMap();
	}

	// Try to load all resources with proper error handling
	if (loadMonsters && !IOMap::loadMonsters(this)) {
		g_logger().warn("Failed to load monster data for map {}", identifier);
	}

	if (loadHouses) {
		auto result = IOMap::loadHouses(this);
		if (!result) {
			g_logger().warn("Failed to load house data for map {}: {}", identifier, result.error());
		}

		/**
		 * Only load houses items if map custom load is disabled
		 * If map custom is enabled, then it is load in loadMapCustom function
		 * NOTE: This will ensure that the information is not duplicated
		 */
		if (!g_configManager().getBoolean(TOGGLE_MAP_CUSTOM)) {
			IOMapSerialize::loadHouseInfo();
			IOMapSerialize::loadHouseItems(this);
		}
	}

	if (loadNpcs && !IOMap::loadNpcs(this)) {
		g_logger().warn("Failed to load NPC data for map {}", identifier);
	}

	if (loadZones && !IOMap::loadZones(this)) {
		g_logger().warn("Failed to load zone data for map {}", identifier);
	}

	// Files need to be cleaned up if custom map is enabled to open, or will try to load main map files
	if (g_configManager().getBoolean(TOGGLE_MAP_CUSTOM)) {
		monsterfile.clear();
		housefile.clear();
		npcfile.clear();
		zonesfile.clear();
	}

	if (!mainMap) {
		g_callbacks().executeCallback(EventCallback_t::mapOnLoad, &EventCallback::mapOnLoad, path.string());
	}
}

void Map::loadMapCustom(std::string_view mapName, bool loadHouses, bool loadMonsters, bool loadNpcs, bool loadZones, int customMapIndex) {
	// Validate customMapIndex
	if (customMapIndex < 0 || customMapIndex >= static_cast<int>(MAX_CUSTOM_MAPS)) {
		g_logger().error("Invalid custom map index: {}", customMapIndex);
		return;
	}

	// Load the map
	const auto customMapPath = std::format("{}/world/custom/{}.otbm", g_configManager().getString(DATA_DIRECTORY), mapName);
	load(customMapPath);

	// Load resources with proper error handling
	if (loadMonsters) {
		auto result = IOMap::loadMonstersCustom(this, mapName, customMapIndex);
		if (!result) {
			g_logger().warn("Failed to load monster custom data: {}", result.error());
		}
	}

	if (loadHouses) {
		auto result = IOMap::loadHousesCustom(this, mapName, customMapIndex);
		if (!result) {
			g_logger().warn("Failed to load house custom data: {}", result.error());
		}
	}

	if (loadNpcs) {
		auto result = IOMap::loadNpcsCustom(this, mapName, customMapIndex);
		if (!result) {
			g_logger().warn("Failed to load npc custom spawn data: {}", result.error());
		}
	}

	if (loadZones) {
		auto result = IOMap::loadZonesCustom(this, mapName, customMapIndex);
		if (!result) {
			g_logger().warn("Failed to load zones custom data: {}", result.error());
		}
	}

	// Files need to be cleaned up or will try to load previous map files again
	monsterfile.clear();
	housefile.clear();
	npcfile.clear();
	zonesfile.clear();
}

void Map::loadHouseInfo() {
	IOMapSerialize::loadHouseInfo();
	IOMapSerialize::loadHouseItems(this);
}

bool Map::save() {
	const uint8_t maxTries = 6;

	for (uint8_t tries = 0; tries < maxTries; tries++) {
		if (IOMapSerialize::saveHouseInfo() && IOMapSerialize::saveHouseItems()) {
			return true;
		}
	}

	return false;
}

std::shared_ptr<Tile> Map::getOrCreateTile(uint16_t x, uint16_t y, uint8_t z, bool isDynamic) {
	auto tile = getTile(x, y, z);
	if (!tile) {
		if (isDynamic) {
			tile = std::make_shared<DynamicTile>(x, y, z);
		} else {
			tile = std::make_shared<StaticTile>(x, y, z);
		}

		setTile(x, y, z, tile);
	}

	return tile;
}

std::shared_ptr<Tile> Map::getLoadedTile(uint16_t x, uint16_t y, uint8_t z) {
	if (z >= MAP_MAX_LAYERS) {
		return nullptr;
	}

	const auto &leaf = getMapSector(x, y);
	if (!leaf) {
		return nullptr;
	}

	const auto &floor = leaf->getFloor(z);
	if (!floor) {
		return nullptr;
	}

	return floor->getTile(x, y);
}

std::shared_ptr<Tile> Map::getTile(uint16_t x, uint16_t y, uint8_t z) {
	if (z >= MAP_MAX_LAYERS) {
		return nullptr;
	}

	const auto &sector = getMapSector(x, y);
	if (!sector) {
		return nullptr;
	}

	const auto &floor = sector->getFloor(z);
	if (!floor) {
		return nullptr;
	}

	return getOrCreateTileFromCache(floor, x, y);
}

void Map::refreshZones(uint16_t x, uint16_t y, uint8_t z) {
	const auto &tile = getLoadedTile(x, y, z);
	if (!tile) {
		return;
	}

	tile->clearZones();
	const auto &zones = Zone::getZones(tile->getPosition());
	for (const auto &zone : zones) {
		tile->addZone(zone);
	}
}

void Map::setTile(uint16_t x, uint16_t y, uint8_t z, const std::shared_ptr<Tile> &newTile) {
	if (z >= MAP_MAX_LAYERS) {
		g_logger().error("Attempt to set tile on invalid coordinate: {}", Position(x, y, z).toString());
		return;
	}

	if (const auto &sector = getMapSector(x, y)) {
		sector->createFloor(z)->setTile(x, y, newTile);
	} else {
		getBestMapSector(x, y)->createFloor(z)->setTile(x, y, newTile);
	}
}

bool Map::placeCreature(const Position &centerPos, const std::shared_ptr<Creature> &creature, bool extendedPos, bool forceLogin) {
	// Safety check for nullptr
	if (!creature) {
		return false;
	}

	const auto &monster = creature->getMonster();
	if (monster) {
		monster->ignoreFieldDamage = true;
	}

	bool foundTile = false;
	bool placeInPZ = false;

	auto tile = getTile(centerPos.x, centerPos.y, centerPos.z);
	if (tile) {
		placeInPZ = tile->hasFlag(TILESTATE_PROTECTIONZONE);
		ReturnValue ret = tile->queryAdd(0, creature, 1, FLAG_IGNOREBLOCKITEM | FLAG_IGNOREFIELDDAMAGE);
		foundTile = forceLogin || ret == RETURNVALUE_NOERROR || ret == RETURNVALUE_PLAYERISNOTINVITED;
	}

	if (monster) {
		monster->ignoreFieldDamage = false;
	}

	if (!foundTile) {
		// Use static array rather than static vector for position offset pairs (no dynamic allocation)
		static constexpr std::array<std::pair<int32_t, int32_t>, 12> extendedRelList { { { 0, -2 }, { -1, -1 }, { 0, -1 }, { 1, -1 }, { -2, 0 }, { -1, 0 }, { 1, 0 }, { 2, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }, { 0, 2 } } };

		static constexpr std::array<std::pair<int32_t, int32_t>, 8> normalRelList { { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } } };

		// Create a shuffled view of the array without modifying the original
		auto relView = extendedPos ? std::span<const std::pair<int32_t, int32_t>>(extendedRelList) : std::span<const std::pair<int32_t, int32_t>>(normalRelList);

		// Create a vector of indices instead of shuffling the pairs
		std::vector<size_t> indices(relView.size());
		std::iota(indices.begin(), indices.end(), 0);

		if (extendedPos) {
			// Shuffle 0-3 and 4-11 separately
			std::shuffle(indices.begin(), indices.begin() + 4, getRandomGenerator());
			std::shuffle(indices.begin() + 4, indices.end(), getRandomGenerator());
		} else {
			std::shuffle(indices.begin(), indices.end(), getRandomGenerator());
		}

		// Try positions using shuffled indices
		for (size_t i : indices) {
			const auto &[xOffset, yOffset] = relView[i];
			Position tryPos(centerPos.x + xOffset, centerPos.y + yOffset, centerPos.z);

			tile = getTile(tryPos.x, tryPos.y, tryPos.z);
			if (!tile || (placeInPZ && !tile->hasFlag(TILESTATE_PROTECTIONZONE))) {
				continue;
			}

			// Will never add the creature inside a teleport, avoiding infinite loop bug
			if (tile->hasFlag(TILESTATE_TELEPORT)) {
				continue;
			}

			if (monster) {
				monster->ignoreFieldDamage = true;
			}

			if (tile->queryAdd(0, creature, 1, FLAG_IGNOREBLOCKITEM | FLAG_IGNOREFIELDDAMAGE) == RETURNVALUE_NOERROR) {
				if (!extendedPos || isSightClear(centerPos, tryPos, false)) {
					foundTile = true;
					break;
				}
			}
		}

		if (!foundTile) {
			return false;
		} else if (monster) {
			monster->ignoreFieldDamage = false;
		}
	}

	int32_t index = 0;
	uint32_t flags = 0;
	std::shared_ptr<Item> toItem = nullptr;

	if (tile) {
		const auto toCylinder = tile->queryDestination(index, creature, toItem, flags);
		toCylinder->internalAddThing(creature);

		const Position &dest = toCylinder->getPosition();
		getMapSector(dest.x, dest.y)->addCreature(creature);
	}
	return true;
}

void Map::moveCreature(const std::shared_ptr<Creature> &creature, const std::shared_ptr<Tile> &newTile, bool forceTeleport) {
	if (!creature || creature->isRemoved() || !newTile) {
		return;
	}

	const auto &oldTile = creature->getTile();

	if (!oldTile) {
		return;
	}

	const auto &oldPos = oldTile->getPosition();
	const auto &newPos = newTile->getPosition();

	if (oldPos == newPos) {
		return; // Nada para fazer se a posição não mudou
	}

	const auto &fromZones = oldTile->getZones();
	const auto &toZones = newTile->getZones();

	// Verificar se a mudança de zona é permitida
	if (const auto &ret = g_game().beforeCreatureZoneChange(creature, fromZones, toZones); ret != RETURNVALUE_NOERROR) {
		return;
	}

	const bool teleport = forceTeleport || !newTile->getGround() || !Position::areInRange<1, 1, 0>(oldPos, newPos);

	// Otimização: coletar espectadores uma vez para todos os propósitos
	Spectators spectators;

	// Otimizar a busca de espectadores com base no tipo de movimento
	if (!teleport && oldPos.z == newPos.z) {
		// Movimento normal (não teleporte) no mesmo nível z
		// Calcular o alcance de visão baseado na direção do movimento
		int32_t minRangeX = MAP_MAX_VIEW_PORT_X;
		int32_t maxRangeX = MAP_MAX_VIEW_PORT_X;
		int32_t minRangeY = MAP_MAX_VIEW_PORT_Y;
		int32_t maxRangeY = MAP_MAX_VIEW_PORT_Y;

		// Ajustar os limites de visão com base na direção do movimento
		if (oldPos.y > newPos.y) {
			++minRangeY; // Movimento para norte
		} else if (oldPos.y < newPos.y) {
			++maxRangeY; // Movimento para sul
		}

		if (oldPos.x < newPos.x) {
			++maxRangeX; // Movimento para leste
		} else if (oldPos.x > newPos.x) {
			++minRangeX; // Movimento para oeste
		}

		spectators.find<Creature>(oldPos, true, minRangeX, maxRangeX, minRangeY, maxRangeY, false);
	} else {
		// Para teleportes ou movimento entre níveis z, buscar espectadores nas duas posições
		spectators.find<Creature>(oldPos, true, 0, 0, 0, 0, false);
		spectators.find<Creature>(newPos, true, 0, 0, 0, 0, false);
	}

	// Filtrar apenas jogadores entre os espectadores (otimização)
	const auto playersSpectators = spectators.filter<Player>();

	// Pré-calcular os índices de stack para todos os jogadores
	std::vector<int32_t> oldStackPosVector;
	oldStackPosVector.reserve(playersSpectators.size());

	for (const auto &spec : playersSpectators) {
		if (spec->canSeeCreature(creature)) {
			oldStackPosVector.push_back(oldTile->getClientIndexOfCreature(spec->getPlayer(), creature));
		} else {
			oldStackPosVector.push_back(-1);
		}
	}

	// Remover a criatura do antigo tile
	oldTile->removeThing(creature, 0);

	// Otimização para troca de setor do mapa
	MapSector* old_sector = getMapSector(oldPos.x, oldPos.y);
	MapSector* new_sector = getMapSector(newPos.x, newPos.y);

	// Apenas trocar de setor se necessário
	if (old_sector != new_sector) {
		old_sector->removeCreature(creature);
		new_sector->addCreature(creature);
	}

	// Adicionar a criatura ao novo tile
	newTile->addThing(creature);

	// Ajustar a direção apenas para movimentos não-teleporte
	if (!teleport) {
		// Otimização: usar regra precisa para determinar a direção
		if (oldPos.y > newPos.y) {
			creature->setDirection(DIRECTION_NORTH);
		} else if (oldPos.y < newPos.y) {
			creature->setDirection(DIRECTION_SOUTH);
		}

		if (oldPos.x < newPos.x) {
			creature->setDirection(DIRECTION_EAST);
		} else if (oldPos.x > newPos.x) {
			creature->setDirection(DIRECTION_WEST);
		}
	}

	// Enviar atualizações para os clientes
	size_t i = 0;
	for (const auto &spectator : playersSpectators) {
		// Usar o stackpos correto para cada jogador
		const int32_t stackpos = oldStackPosVector[i++];
		if (stackpos != -1) {
			const auto &player = spectator->getPlayer();
			player->sendCreatureMove(creature, newPos, newTile->getStackposOfCreature(player, creature), oldPos, stackpos, teleport);
		}
	}

	// Notificar os espectadores sobre o movimento da criatura
	for (const auto &spectator : spectators) {
		spectator->onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);
	}

	// Criar uma função lambda para as ações pós-movimento
	auto postMoveActions = [=] {
		oldTile->postRemoveNotification(creature, newTile, 0);
		newTile->postAddNotification(creature, oldTile, 0);
		g_game().afterCreatureZoneChange(creature, fromZones, toZones);
	};

	// Executar as ações pós-movimento de forma apropriada com base no contexto
	if (g_dispatcher().context().getGroup() == TaskGroup::Walk) {
		// Para movimento de monstros, adiar as ações para evitar problemas de assincronicidade
		g_dispatcher().addEvent(std::move(postMoveActions), "Map::moveCreature");
	} else {
		// Para outros contextos, executar imediatamente
		postMoveActions();
	}

	// Efeitos visuais para teleportes forçados
	if (forceTeleport) {
		if (const auto &player = creature->getPlayer()) {
			player->sendMagicEffect(oldPos, CONST_ME_TELEPORT);
			player->sendMagicEffect(newPos, CONST_ME_TELEPORT);
		}
	}
}

bool Map::canThrowObjectTo(const Position &fromPos, const Position &toPos, SightLines_t lineOfSight, const int32_t rangex, const int32_t rangey) {
	// Verificações otimizadas para níveis z
	// níveis subterrâneos: 8->15
	// níveis de superfície e acima: 7->0
	if ((fromPos.z >= 8 && toPos.z <= MAP_INIT_SURFACE_LAYER) || (toPos.z >= MAP_INIT_SURFACE_LAYER + 1 && fromPos.z <= MAP_INIT_SURFACE_LAYER)) {
		return false; // Não é possível lançar entre níveis separados por uma grande distância vertical
	}

	// Verificar distância vertical
	const int32_t deltaz = Position::getDistanceZ(fromPos, toPos);
	if (deltaz > MAP_LAYER_VIEW_LIMIT) {
		return false; // Excede o limite de visão vertical
	}

	// Verificar distância horizontal ajustada pela distância vertical
	if ((Position::getDistanceX(fromPos, toPos) - deltaz) > rangex) {
		return false; // Excede o alcance horizontal
	}

	if ((Position::getDistanceY(fromPos, toPos) - deltaz) > rangey) {
		return false; // Excede o alcance vertical
	}

	// Verificar linha de visão apenas se necessário
	if (!(lineOfSight & SightLine_CheckSightLine)) {
		return true; // Não é necessário verificar linha de visão
	}

	return isSightClear(fromPos, toPos, lineOfSight & SightLine_FloorCheck);
}

bool Map::checkSightLine(Position start, Position destination) {
	if (start.x == destination.x && start.y == destination.y) {
		return true; // Mesma posição, visão limpa
	}

	int32_t distanceX = Position::getDistanceX(start, destination);
	int32_t distanceY = Position::getDistanceY(start, destination);

	// Otimização para linhas retas horizontais ou verticais
	if (start.y == destination.y) {
		// Linha horizontal
		const uint16_t delta = start.x < destination.x ? 1 : 0xFFFF;
		while (--distanceX > 0) {
			start.x += delta;

			const auto &tile = getTile(start.x, start.y, start.z);
			if (tile && tile->hasProperty(CONST_PROP_BLOCKPROJECTILE)) {
				return false; // Bloqueado
			}
		}
		return true; // Visão limpa
	}

	if (start.x == destination.x) {
		// Linha vertical
		const uint16_t delta = start.y < destination.y ? 1 : 0xFFFF;
		while (--distanceY > 0) {
			start.y += delta;

			const auto &tile = getTile(start.x, start.y, start.z);
			if (tile && tile->hasProperty(CONST_PROP_BLOCKPROJECTILE)) {
				return false; // Bloqueado
			}
		}
		return true; // Visão limpa
	}

	// Algoritmo de linha de Xiaolin Wu para linhas diagonais
	// baseado na implementação de Michael Abrash
	uint16_t eAdj;
	uint16_t eAcc = 0;
	uint16_t deltaX = 1;
	uint16_t deltaY = 1;

	if (distanceY > distanceX) {
		eAdj = (static_cast<uint32_t>(distanceX) << 16) / static_cast<uint32_t>(distanceY);

		if (start.y > destination.y) {
			std::swap(start.x, destination.x);
			std::swap(start.y, destination.y);
		}

		if (start.x > destination.x) {
			deltaX = 0xFFFF;
			eAcc -= eAdj;
		}

		while (--distanceY > 0) {
			uint16_t xIncrease = 0;
			const uint16_t eAccTemp = eAcc;
			eAcc += eAdj;
			if (eAcc <= eAccTemp) {
				xIncrease = deltaX;
			}

			const auto &tile = getTile(start.x + xIncrease, start.y + deltaY, start.z);
			if (tile && tile->hasProperty(CONST_PROP_BLOCKPROJECTILE)) {
				if (Position::areInRange<1, 1>(start, destination)) {
					return true; // Adjacente, ainda possível
				}
				return false; // Bloqueado
			}

			start.x += xIncrease;
			start.y += deltaY;
		}
	} else {
		eAdj = (static_cast<uint32_t>(distanceY) << 16) / static_cast<uint32_t>(distanceX);

		if (start.x > destination.x) {
			std::swap(start.x, destination.x);
			std::swap(start.y, destination.y);
		}

		if (start.y > destination.y) {
			deltaY = 0xFFFF;
			eAcc -= eAdj;
		}

		while (--distanceX > 0) {
			uint16_t yIncrease = 0;
			const uint16_t eAccTemp = eAcc;
			eAcc += eAdj;
			if (eAcc <= eAccTemp) {
				yIncrease = deltaY;
			}

			const auto &tile = getTile(start.x + deltaX, start.y + yIncrease, start.z);
			if (tile && tile->hasProperty(CONST_PROP_BLOCKPROJECTILE)) {
				if (Position::areInRange<1, 1>(start, destination)) {
					return true; // Adjacente, ainda possível
				}
				return false; // Bloqueado
			}

			start.x += deltaX;
			start.y += yIncrease;
		}
	}

	return true; // Nenhum bloqueio encontrado
}

bool Map::isSightClear(const Position &fromPos, const Position &toPos, bool floorCheck) {
	// Verificação rápida para rejeitar casos impossíveis
	if (floorCheck && fromPos.z != toPos.z) {
		return false; // Visão bloqueada em diferentes níveis z com verificação de andar
	}

	// Verificação rápida para aceitar casos óbvios
	if (fromPos.z == toPos.z && (Position::areInRange<1, 1>(fromPos, toPos) || (!floorCheck && fromPos.z == 0))) {
		return true; // Adjacente ou no nível 0 sem verificação de andar
	}

	// Não podemos lançar mais do que um andar para cima
	if (fromPos.z > toPos.z && Position::getDistanceZ(fromPos, toPos) > 1) {
		return false;
	}

	// Verificar linha de visão no andar atual
	const bool sightClear = checkSightLine(fromPos, toPos);
	if (floorCheck || (fromPos.z == toPos.z && sightClear)) {
		return sightClear;
	}

	uint8_t startZ;
	if (sightClear && (fromPos.z < toPos.z || fromPos.z == toPos.z)) {
		startZ = fromPos.z;
	} else {
		// Verificar se podemos lançar acima do obstáculo
		const auto &tile = getTile(fromPos.x, fromPos.y, fromPos.z - 1);
		if ((tile && (tile->getGround() || tile->hasProperty(CONST_PROP_BLOCKPROJECTILE))) || !checkSightLine(Position(fromPos.x, fromPos.y, fromPos.z - 1), Position(toPos.x, toPos.y, toPos.z - 1))) {
			return false; // Não podemos lançar acima do obstáculo
		}

		// Podemos lançar acima do obstáculo
		if (fromPos.z > toPos.z) {
			return true;
		}

		startZ = fromPos.z - 1;
	}

	// Verificar se há algum bloqueio entre andares
	for (; startZ != toPos.z; ++startZ) {
		const auto &tile = getTile(toPos.x, toPos.y, startZ);
		if (tile && (tile->getGround() || tile->hasProperty(CONST_PROP_BLOCKPROJECTILE))) {
			return false; // Bloqueado por um tile
		}
	}

	return true; // Caminho livre
}

std::shared_ptr<Tile> Map::canWalkTo(const std::shared_ptr<Creature> &creature, const Position &pos) {
	if (!creature || creature->isRemoved()) {
		return nullptr;
	}

	const auto &tile = getTile(pos.x, pos.y, pos.z);
	if (creature->getTile() != tile) {
		if (!tile || tile->queryAdd(0, creature, 1, FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) != RETURNVALUE_NOERROR) {
			return nullptr; // Não pode andar para este tile
		}
	}

	return tile;
}

uint32_t Map::clean() const {
	const auto start = std::chrono::steady_clock::now();
	size_t qntTiles = 0;

	// Alterar estado do jogo para manutenção durante a limpeza
	if (g_game().getGameState() == GAME_STATE_NORMAL) {
		g_game().setGameState(GAME_STATE_MAINTAIN);
	}

	// Pré-alocar vetor para itens a serem removidos para evitar realocações
	ItemVector toRemove;
	toRemove.reserve(128);

	// Coletar todos os itens a serem removidos
	for (const auto &tile : g_game().getTilesToClean()) {
		if (!tile) {
			continue;
		}

		if (const auto &items = tile->getItemList()) {
			++qntTiles;
			for (const auto &item : *items) {
				if (item->isCleanable()) {
					toRemove.emplace_back(item);
				}
			}
		}
	}

	// Registrar o número de itens antes de removê-los
	const size_t count = toRemove.size();

	// Remover todos os itens coletados
	for (const auto &item : toRemove) {
		g_game().internalRemoveItem(item, -1);
	}

	// Limpar a lista de tiles para limpeza
	g_game().clearTilesToClean();

	// Restaurar o estado do jogo se necessário
	if (g_game().getGameState() == GAME_STATE_MAINTAIN) {
		g_game().setGameState(GAME_STATE_NORMAL);
	}

	// Calcular tempo decorrido
	const auto end = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	// Registrar estatísticas
	g_logger().info("CLEAN: Removed {} item{} from {} tile{} in {:.3f} seconds", count, (count != 1 ? "s" : ""), qntTiles, (qntTiles != 1 ? "s" : ""), elapsed / 1000.0f);

	return count;
}

/**
 * Implementação otimizada do algoritmo de pathfinding
 */
bool Map::getPathMatching(const std::shared_ptr<Creature> &creature, const Position &targetPos, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp) {
	// Cache estático para evitar alocações repetidas em chamadas sucessivas
	static constexpr std::array<std::array<int_fast32_t, 2>, 8> allNeighbors = { { { { -1, 0 } }, { { 0, 1 } }, { { 1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } }, { { 1, 1 } }, { { -1, 1 } } } };

	static constexpr std::array<std::array<std::array<int_fast32_t, 2>, 5>, 8> dirNeighbors = { { { { { { -1, 0 } }, { { 0, 1 } }, { { 1, 0 } }, { { 1, 1 } }, { { -1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 0, 1 } }, { { 0, -1 } }, { { -1, -1 } }, { { -1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } } } },
		                                                                                          { { { { 0, 1 } }, { { 1, 0 } }, { { 0, -1 } }, { { 1, -1 } }, { { 1, 1 } } } },
		                                                                                          { { { { 1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } }, { { 1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } }, { { -1, 1 } } } },
		                                                                                          { { { { 0, 1 } }, { { 1, 0 } }, { { 1, -1 } }, { { 1, 1 } }, { { -1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 0, 1 } }, { { -1, -1 } }, { { 1, 1 } }, { { -1, 1 } } } } } };

	const bool withoutCreature = creature == nullptr;

	Position pos = withoutCreature ? targetPos : creature->getPosition();
	Position endPos;

	// Inicializar nós A* com a posição inicial
	const auto &startTile = getTile(pos.x, pos.y, pos.z);
	if (!startTile) {
		return false;
	}

	AStarNodes nodes(pos.x, pos.y, AStarNodes::getTileWalkCost(creature, startTile));

	int32_t bestMatch = 0;

	const auto &startPos = pos;
	const auto &actualTargetPos = withoutCreature ? pathCondition.getTargetPos() : targetPos;

	// Calcular distância inicial para heurística
	const int_fast32_t sX = std::abs(actualTargetPos.getX() - pos.getX());
	const int_fast32_t sY = std::abs(actualTargetPos.getY() - pos.getY());

	// Contador de direções exploradas
	uint_fast16_t cntDirs = 0;

	// Busca A*
	const AStarNode* found = nullptr;
	do {
		AStarNode* n = nodes.getBestNode();
		if (!n) {
			// Sem nós para explorar
			if (found) {
				break; // Encontramos um caminho parcial
			}
			return false; // Nenhum caminho encontrado
		}

		const int_fast32_t x = n->x;
		const int_fast32_t y = n->y;
		pos.x = x;
		pos.y = y;

		// Verificar se a condição de chegada é satisfeita
		if (pathCondition(startPos, pos, fpp, bestMatch)) {
			found = n;
			endPos = pos;
			if (bestMatch == 0) {
				break; // Caminho ideal encontrado
			}
		}

		++cntDirs;

		// Selecionar vizinhos com base na direção do pai
		uint_fast32_t dirCount;
		const std::array<int_fast32_t, 2>* neighbors;

		if (n->parent) {
			// Determinar a direção com base no nó pai
			const int_fast32_t offset_x = n->parent->x - x;
			const int_fast32_t offset_y = n->parent->y - y;

			int dirIndex;
			if (offset_y == 0) {
				dirIndex = offset_x == -1 ? DIRECTION_WEST : DIRECTION_EAST;
			} else if (offset_x == 0) {
				dirIndex = offset_y == -1 ? DIRECTION_NORTH : DIRECTION_SOUTH;
			} else if (offset_y == -1) {
				dirIndex = offset_x == -1 ? DIRECTION_NORTHWEST : DIRECTION_NORTHEAST;
			} else {
				dirIndex = offset_x == -1 ? DIRECTION_SOUTHWEST : DIRECTION_SOUTHEAST;
			}

			neighbors = dirNeighbors[dirIndex].data();
			dirCount = 5;
		} else {
			// Sem nó pai, explorar todas as direções
			neighbors = allNeighbors.data();
			dirCount = 8;
		}

		// Custo acumulado até este nó
		const int_fast32_t f = n->f;

		// Explorar vizinhos
		for (uint_fast32_t i = 0; i < dirCount; ++i) {
			pos.x = x + neighbors[i][0];
			pos.y = y + neighbors[i][1];

			// Verificar se estamos dentro do limite de busca
			if (fpp.maxSearchDist != 0 && (Position::getDistanceX(startPos, pos) > fpp.maxSearchDist || Position::getDistanceY(startPos, pos) > fpp.maxSearchDist)) {
				continue;
			}

			// Verificar se estamos mantendo a distância correta
			if (fpp.keepDistance && !pathCondition.isInRange(startPos, pos, fpp)) {
				continue;
			}

			// Verificar custo do tile
			int_fast32_t extraCost;
			AStarNode* neighborNode = nodes.getNodeByPosition(pos.x, pos.y);

			if (neighborNode) {
				extraCost = neighborNode->c;
			} else {
				// Verificar se podemos andar para este tile
				const auto &tile = withoutCreature ? getTile(pos.x, pos.y, pos.z) : canWalkTo(creature, pos);
				if (!tile) {
					continue;
				}
				extraCost = AStarNodes::getTileWalkCost(creature, tile);
			}

			// Calcular o custo total para este vizinho
			const int_fast32_t cost = AStarNodes::getMapWalkCost(n, pos);
			const int_fast32_t newf = f + cost + extraCost;

			if (neighborNode) {
				// O nó já existe, verificar se este caminho é melhor
				if (neighborNode->f <= newf) {
					continue; // O caminho existente é mais barato
				}

				// Atualizar o nó com o caminho mais barato
				neighborNode->f = newf;
				neighborNode->parent = n;
				nodes.openNode(neighborNode);
			} else {
				// Criar um novo nó
				const int_fast32_t dX = std::abs(actualTargetPos.getX() - pos.getX());
				const int_fast32_t dY = std::abs(actualTargetPos.getY() - pos.getY());

				// Heurística de distância para A*
				const int32_t heuristic = ((dX - sX) << 3) + ((dY - sY) << 3) + (std::max(dX, dY) << 3);

				if (!nodes.createOpenNode(n, pos.x, pos.y, newf, heuristic, extraCost)) {
					if (found) {
						break; // Limite de nós atingido, usar melhor caminho encontrado
					}
					return false;
				}
			}
		}

		// Marcar este nó como explorado
		nodes.closeNode(n);
	} while (fpp.maxSearchDist != 0 || nodes.getClosedNodes() < 100);

	if (!found) {
		return false;
	}

	// Reconstruir o caminho a partir do nó encontrado
	int_fast32_t prevx = endPos.x;
	int_fast32_t prevy = endPos.y;

	// Pré-alocar vetor de direções para evitar realocações
	dirList.reserve(cntDirs);

	// Traçar o caminho de volta para o início
	found = found->parent;
	while (found) {
		pos.x = found->x;
		pos.y = found->y;

		const int_fast32_t dx = pos.getX() - prevx;
		const int_fast32_t dy = pos.getY() - prevy;

		prevx = pos.x;
		prevy = pos.y;

		// Determinar a direção com base na diferença de coordenadas
		if (dx == 1) {
			if (dy == 1) {
				dirList.emplace_back(DIRECTION_NORTHWEST);
			} else if (dy == -1) {
				dirList.emplace_back(DIRECTION_SOUTHWEST);
			} else {
				dirList.emplace_back(DIRECTION_WEST);
			}
		} else if (dx == -1) {
			if (dy == 1) {
				dirList.emplace_back(DIRECTION_NORTHEAST);
			} else if (dy == -1) {
				dirList.emplace_back(DIRECTION_SOUTHEAST);
			} else {
				dirList.emplace_back(DIRECTION_EAST);
			}
		} else if (dy == 1) {
			dirList.emplace_back(DIRECTION_NORTH);
		} else if (dy == -1) {
			dirList.emplace_back(DIRECTION_SOUTH);
		}

		found = found->parent;
	}

	return true;
}

// Implementação das outras funções de pathfinding que chamam a principal
bool Map::getPathMatching(const std::shared_ptr<Creature> &creature, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp) {
	return getPathMatching(creature, creature->getPosition(), dirList, pathCondition, fpp);
}

bool Map::getPathMatchingCond(const std::shared_ptr<Creature> &creature, const Position &targetPos, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp) {
	// Esta implementação é quase idêntica à getPathMatching, mas com pequenas diferenças na lógica
	// Implementação otimizada com recursos C++23

	if (!creature) {
		return false;
	}

	Position pos = creature->getPosition();
	Position endPos;

	const auto &tile = creature->getTile();
	if (!tile) {
		return false;
	}

	AStarNodes nodes(pos.x, pos.y, AStarNodes::getTileWalkCost(creature, tile));

	int32_t bestMatch = 0;

	const Position startPos = pos;

	// Calcular distância inicial para heurística
	const int_fast32_t sX = std::abs(targetPos.getX() - pos.getX());
	const int_fast32_t sY = std::abs(targetPos.getY() - pos.getY());

	// Array para vizinhos por direção (substituindo vetores estáticos)
	// Usando arrays constexpr para melhor otimização em tempo de compilação
	static constexpr std::array<std::array<int_fast32_t, 2>, 8> allNeighbors = { { { { -1, 0 } }, { { 0, 1 } }, { { 1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } }, { { 1, 1 } }, { { -1, 1 } } } };

	static constexpr std::array<std::array<std::array<int_fast32_t, 2>, 5>, 8> dirNeighbors = { { { { { { -1, 0 } }, { { 0, 1 } }, { { 1, 0 } }, { { 1, 1 } }, { { -1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 0, 1 } }, { { 0, -1 } }, { { -1, -1 } }, { { -1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } } } },
		                                                                                          { { { { 0, 1 } }, { { 1, 0 } }, { { 0, -1 } }, { { 1, -1 } }, { { 1, 1 } } } },
		                                                                                          { { { { 1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } }, { { 1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 0, -1 } }, { { -1, -1 } }, { { 1, -1 } }, { { -1, 1 } } } },
		                                                                                          { { { { 0, 1 } }, { { 1, 0 } }, { { 1, -1 } }, { { 1, 1 } }, { { -1, 1 } } } },
		                                                                                          { { { { -1, 0 } }, { { 0, 1 } }, { { -1, -1 } }, { { 1, 1 } }, { { -1, 1 } } } } } };

	uint_fast16_t cntDirs = 0;

	const AStarNode* found = nullptr;
	do {
		AStarNode* n = nodes.getBestNode();
		if (!n) {
			if (found) {
				break;
			}
			return false;
		}

		const int_fast32_t x = n->x;
		const int_fast32_t y = n->y;
		pos.x = x;
		pos.y = y;

		if (pathCondition(startPos, pos, fpp, bestMatch)) {
			found = n;
			endPos = pos;
			if (bestMatch == 0) {
				break;
			}
		}

		++cntDirs;

		// Determinar vizinhos a explorar com base na direção do movimento
		uint_fast32_t dirCount;
		const std::array<int_fast32_t, 2>* neighbors;

		if (n->parent) {
			// Determinar direção com base no nó pai
			const int_fast32_t offset_x = n->parent->x - x;
			const int_fast32_t offset_y = n->parent->y - y;

			int dirIndex;
			if (offset_y == 0) {
				dirIndex = offset_x == -1 ? DIRECTION_WEST : DIRECTION_EAST;
			} else if (offset_x == 0) {
				dirIndex = offset_y == -1 ? DIRECTION_NORTH : DIRECTION_SOUTH;
			} else if (offset_y == -1) {
				dirIndex = offset_x == -1 ? DIRECTION_NORTHWEST : DIRECTION_NORTHEAST;
			} else {
				dirIndex = offset_x == -1 ? DIRECTION_SOUTHWEST : DIRECTION_SOUTHEAST;
			}

			neighbors = dirNeighbors[dirIndex].data();
			dirCount = 5;
		} else {
			// Sem direção preferida, usar todos os vizinhos
			neighbors = allNeighbors.data();
			dirCount = 8;
		}

		const int_fast32_t f = n->f;

		// Explorar todos os vizinhos selecionados
		for (uint_fast32_t i = 0; i < dirCount; ++i) {
			pos.x = x + neighbors[i][0];
			pos.y = y + neighbors[i][1];

			// Verificação de distância máxima
			if (fpp.maxSearchDist != 0 && (Position::getDistanceX(startPos, pos) > fpp.maxSearchDist || Position::getDistanceY(startPos, pos) > fpp.maxSearchDist)) {
				continue;
			}

			// Verificar condição de manter distância
			if (fpp.keepDistance && !pathCondition.isInRange(startPos, pos, fpp)) {
				continue;
			}

			// Obter o custo para este tile
			int_fast32_t extraCost;
			AStarNode* neighborNode = nodes.getNodeByPosition(pos.x, pos.y);

			if (neighborNode) {
				extraCost = neighborNode->c;
			} else {
				const auto &nextTile = Map::canWalkTo(creature, pos);
				if (!nextTile) {
					continue;
				}
				extraCost = AStarNodes::getTileWalkCost(creature, nextTile);
			}

			// Calcular o custo total para este caminho
			const int_fast32_t cost = AStarNodes::getMapWalkCost(n, pos);
			const int_fast32_t newf = f + cost + extraCost;

			if (neighborNode) {
				// Verificar se este caminho é melhor que o existente
				if (neighborNode->f <= newf) {
					continue;
				}

				// Atualizar o nó com o melhor caminho
				neighborNode->f = newf;
				neighborNode->parent = n;
				nodes.openNode(neighborNode);
			} else {
				// Criar um novo nó
				const int_fast32_t dX = std::abs(targetPos.getX() - pos.getX());
				const int_fast32_t dY = std::abs(targetPos.getY() - pos.getY());

				// Heurística mais otimizada para o custo estimado até o destino
				const int_fast32_t heuristic = ((dX - sX) << 3) + ((dY - sY) << 3) + (std::max(dX, dY) << 3);

				if (!nodes.createOpenNode(n, pos.x, pos.y, newf, heuristic, extraCost)) {
					if (found) {
						break;
					}
					return false;
				}
			}
		}

		// Finalizar exploração deste nó
		nodes.closeNode(n);
	} while (fpp.maxSearchDist != 0 || nodes.getClosedNodes() < 100);

	// Sem caminho encontrado
	if (!found) {
		return false;
	}

	// Reconstruir o caminho a partir do nó final
	int_fast32_t prevx = endPos.x;
	int_fast32_t prevy = endPos.y;

	// Reservar espaço para evitar realocações
	dirList.reserve(cntDirs);

	// Traçar caminho de volta para a origem
	found = found->parent;
	while (found) {
		pos.x = found->x;
		pos.y = found->y;

		const int_fast32_t dx = pos.getX() - prevx;
		const int_fast32_t dy = pos.getY() - prevy;

		prevx = pos.x;
		prevy = pos.y;

		// Adicionar direção correta com base nas diferenças de coordenadas
		// Usando estrutura de decisão eficiente
		if (dx == 1) {
			if (dy == 1) {
				dirList.emplace_back(DIRECTION_NORTHWEST);
			} else if (dy == -1) {
				dirList.emplace_back(DIRECTION_SOUTHWEST);
			} else {
				dirList.emplace_back(DIRECTION_WEST);
			}
		} else if (dx == -1) {
			if (dy == 1) {
				dirList.emplace_back(DIRECTION_NORTHEAST);
			} else if (dy == -1) {
				dirList.emplace_back(DIRECTION_SOUTHEAST);
			} else {
				dirList.emplace_back(DIRECTION_EAST);
			}
		} else if (dy == 1) {
			dirList.emplace_back(DIRECTION_NORTH);
		} else if (dy == -1) {
			dirList.emplace_back(DIRECTION_SOUTH);
		}

		found = found->parent;
	}

	return true;
}
