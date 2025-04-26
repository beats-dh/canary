/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "io/iomap.hpp"

#include "game/zones/zone.hpp"
#include "io/filestream.hpp"

#include <stacktrace>

/*
    OTBM_ROOTV1
    |
    |--- OTBM_MAP_DATA
    |	|
    |	|--- OTBM_TILE_AREA
    |	|	|--- OTBM_TILE
    |	|	|--- OTBM_TILE_SQUARE (not implemented)
    |	|	|--- OTBM_TILE_REF (not implemented)
    |	|	|--- OTBM_HOUSETILE
    |	|
    |	|--- OTBM_SPAWNS (not implemented)
    |	|	|--- OTBM_SPAWN_AREA (not implemented)
    |	|	|--- OTBM_MONSTER (not implemented)
    |	|
    |	|--- OTBM_TOWNS
    |	|	|--- OTBM_TOWN
    |	|
    |	|--- OTBM_WAYPOINTS
    |		|--- OTBM_WAYPOINT
    |
    |--- OTBM_ITEM_DEF (not implemented)
*/

void IOMap::loadMap(Map* map, const Position &pos) {
	Benchmark bm_mapLoad;

	try {
		const auto &fileByte = mio::mmap_source(map->path.string());
		const auto begin = fileByte.begin() + sizeof(OTB::Identifier { { 'O', 'T', 'B', 'M' } });

		// Usar std::span para melhor manipulação de memória
		std::span fileSpan { begin, fileByte.end() };
		FileStream stream { fileSpan.data(), fileSpan.data() + fileSpan.size() };

		if (!stream.startNode()) {
			throw IOMapException("Could not read map node.");
		}

		stream.skip(1); // Type Node

		const uint32_t version = stream.getU32();
		map->width = stream.getU16();
		map->height = stream.getU16();
		const uint32_t majorVersionItems = stream.getU32();
		stream.getU32(); // minorVersionItems

		if (version > 2) {
			throw IOMapException("Unknown OTBM version detected.");
		}

		if (majorVersionItems < 3) {
			throw IOMapException("This map need to be upgraded by using the latest map editor version to be able to load correctly.");
		}

		if (stream.startNode(OTBM_MAP_DATA)) {
			parseMapDataAttributes(stream, map);
			parseTileArea(stream, *map, pos);
			stream.endNode();
		}

		parseTowns(stream, *map);
		parseWaypoints(stream, *map);

		map->flush();

		g_logger().debug("Map Loaded {} ({}x{}) in {} milliseconds", map->path.filename().string(), map->width, map->height, bm_mapLoad.duration());
	} catch (const std::exception &e) {
		g_logger().error("Failed to load map: {}\nStacktrace: {}", e.what(), std::to_string(std::stacktrace::current()));
		throw;
	}
}

void IOMap::parseMapDataAttributes(FileStream &stream, Map* map) {
	// Usar enum class para atributos
	enum class MapAttribute : uint8_t {
		Description = OTBM_ATTR_DESCRIPTION,
		SpawnMonsterFile = OTBM_ATTR_EXT_SPAWN_MONSTER_FILE,
		SpawnNpcFile = OTBM_ATTR_EXT_SPAWN_NPC_FILE,
		HouseFile = OTBM_ATTR_EXT_HOUSE_FILE,
		ZoneFile = OTBM_ATTR_EXT_ZONE_FILE
	};

	// Obter o diretório base do mapa uma única vez
	const std::string baseDir = map->path.string().substr(0, map->path.string().rfind('/') + 1);

	while (true) {
		const uint8_t attrValue = stream.getU8();
		if (attrValue == 0 || attrValue > static_cast<uint8_t>(MapAttribute::ZoneFile)) {
			stream.back();
			break;
		}

		const auto attr = static_cast<MapAttribute>(attrValue);
		// Usar std::string_view para evitar cópias
		const std::string_view fileName = stream.getString();

		switch (attr) {
			case MapAttribute::Description:
				// Apenas ignorar a descrição
				break;

			case MapAttribute::SpawnMonsterFile:
				map->monsterfile = std::format("{}{}", baseDir, fileName);
				break;

			case MapAttribute::SpawnNpcFile:
				map->npcfile = std::format("{}{}", baseDir, fileName);
				break;

			case MapAttribute::HouseFile:
				map->housefile = std::format("{}{}", baseDir, fileName);
				break;

			case MapAttribute::ZoneFile:
				map->zonesfile = std::format("{}{}", baseDir, fileName);
				break;
		}
	}
}

void IOMap::parseTileArea(FileStream &stream, Map &map, const Position &pos) {
	while (stream.startNode(OTBM_TILE_AREA)) {
		const uint16_t base_x = stream.getU16();
		const uint16_t base_y = stream.getU16();
		const uint8_t base_z = stream.getU8();

		while (stream.startNode()) {
			const uint8_t tileType = stream.getU8();
			if (tileType != OTBM_HOUSETILE && tileType != OTBM_TILE) {
				throw IOMapException("Could not read tile type node.");
			}

			auto tile = std::make_shared<BasicTile>();

			const uint8_t tileCoordsX = stream.getU8();
			const uint8_t tileCoordsY = stream.getU8();

			const uint16_t x = base_x + tileCoordsX + pos.x;
			const uint16_t y = base_y + tileCoordsY + pos.y;
			const auto z = static_cast<uint8_t>(base_z + pos.z);

			if (tileType == OTBM_HOUSETILE) {
				tile->houseId = stream.getU32();
				if (!map.houses.addHouse(tile->houseId)) {
					throw IOMapException(std::format("[x:{}, y:{}, z:{}] Could not create house id: {}", x, y, z, tile->houseId));
				}
			}

			if (stream.isProp(OTBM_ATTR_TILE_FLAGS)) {
				const uint32_t flags = stream.getU32();
				// Usando bit manipulation mais eficiente
				if (flags & OTBM_TILEFLAG_PROTECTIONZONE) {
					tile->flags |= TILESTATE_PROTECTIONZONE;
				} else if (flags & OTBM_TILEFLAG_NOPVPZONE) {
					tile->flags |= TILESTATE_NOPVPZONE;
				} else if (flags & OTBM_TILEFLAG_PVPZONE) {
					tile->flags |= TILESTATE_PVPZONE;
				}

				if (flags & OTBM_TILEFLAG_NOLOGOUT) {
					tile->flags |= TILESTATE_NOLOGOUT;
				}
			}

			if (stream.isProp(OTBM_ATTR_ITEM)) {
				const uint16_t id = stream.getU16();
				const auto &iType = Item::items[id];

				if (!tile->isHouse() || !iType.isBed()) {
					auto item = std::make_shared<BasicItem>();
					item->id = id;

					if (tile->isHouse() && iType.movable) {
						g_logger().warn(std::format("[IOMap::loadMap] - Movable item with ID: {}, in house: {}, at position: x {}, y {}, z {}", id, tile->houseId, x, y, z));
					} else if (iType.isGroundTile()) {
						tile->ground = map.tryReplaceItemFromCache(std::move(item));
					} else {
						tile->items.emplace_back(map.tryReplaceItemFromCache(std::move(item)));
					}
				}
			}

			while (stream.startNode()) {
				auto type = stream.getU8();
				switch (type) {
					case OTBM_ITEM: {
						const uint16_t id = stream.getU16();
						const auto &iType = Item::items[id];
						auto item = std::make_shared<BasicItem>();
						item->id = id;

						if (!item->unserializeItemNode(stream, x, y, z)) {
							throw IOMapException(std::format("[x:{}, y:{}, z:{}] Failed to load item {}, Node Type.", x, y, z, id));
						}

						if (tile->isHouse() && (iType.isBed() || iType.isTrashHolder())) {
							// nothing
						} else if (tile->isHouse() && iType.movable) {
							g_logger().warn(std::format("[IOMap::loadMap] - Movable item with ID: {}, in house: {}, at position: x {}, y {}, z {}", id, tile->houseId, x, y, z));
						} else if (iType.isGroundTile()) {
							tile->ground = map.tryReplaceItemFromCache(std::move(item));
						} else {
							tile->items.emplace_back(map.tryReplaceItemFromCache(std::move(item)));
						}
					} break;
					case OTBM_TILE_ZONE: {
						const auto zoneCount = stream.getU16();
						for (uint16_t i = 0; i < zoneCount; ++i) {
							const auto zoneId = stream.getU16();
							if (!zoneId) {
								throw IOMapException(std::format("[x:{}, y:{}, z:{}] Invalid zone id.", x, y, z));
							}
							auto zone = Zone::getZone(zoneId);
							zone->addPosition(Position(x, y, z));
						}
					} break;
					default:
						throw IOMapException(std::format("[x:{}, y:{}, z:{}] Could not read item/zone node.", x, y, z));
				}

				if (!stream.endNode()) {
					throw IOMapException(std::format("[x:{}, y:{}, z:{}] Could not end node.", x, y, z));
				}
			}

			if (!stream.endNode()) {
				throw IOMapException(std::format("[x:{}, y:{}, z:{}] Could not end node.", x, y, z));
			}

			if (tile->isEmpty(true)) {
				continue;
			}

			map.setBasicTile(x, y, z, std::move(tile));
		}

		if (!stream.endNode()) {
			throw IOMapException("Could not end node.");
		}
	}
}

void IOMap::parseTowns(FileStream &stream, Map &map) {
	if (!stream.startNode(OTBM_TOWNS)) {
		throw IOMapException("Could not read towns node.");
	}

	// Pré-alocar o vetor para uma quantidade típica de cidades
	std::vector<std::tuple<uint32_t, std::string, Position>> towns;
	towns.reserve(16);

	while (stream.startNode(OTBM_TOWN)) {
		const uint32_t townId = stream.getU32();
		const auto &townName = stream.getString();
		const uint16_t x = stream.getU16();
		const uint16_t y = stream.getU16();
		const uint8_t z = stream.getU8();

		towns.emplace_back(townId, townName, Position(x, y, z));

		if (!stream.endNode()) {
			throw IOMapException("Could not end node.");
		}
	}

	// Criar as cidades em ordem
	for (const auto &[id, name, pos] : towns) {
		auto town = map.towns.getOrCreateTown(id);
		town->setName(name);
		town->setTemplePos(pos);
	}

	if (!stream.endNode()) {
		throw IOMapException("Could not end node.");
	}
}

void IOMap::parseWaypoints(FileStream &stream, Map &map) {
	if (!stream.startNode(OTBM_WAYPOINTS)) {
		throw IOMapException("Could not read waypoints node.");
	}

	while (stream.startNode(OTBM_WAYPOINT)) {
		const auto &name = stream.getString();
		const uint16_t x = stream.getU16();
		const uint16_t y = stream.getU16();
		const uint8_t z = stream.getU8();

		map.waypoints.emplace(name, Position(x, y, z));

		if (!stream.endNode()) {
			throw IOMapException("Could not end node.");
		}
	}

	if (!stream.endNode()) {
		throw IOMapException("Could not end node.");
	}
}

std::string IOMap::getFullPath(const std::string &currentPath, std::string_view mapName, std::string_view suffix) {
	if (!currentPath.empty()) {
		return currentPath;
	}
	return std::format("{}{}", mapName, suffix);
}

// Implementações dos métodos para carregar arquivos adicionais
std::expected<bool, std::string> IOMap::loadMonsters(Map* map) {
	if (map->monsterfile.empty()) {
		map->monsterfile = std::format("{}-monster.xml", g_configManager().getString(MAP_NAME));
	}

	if (!map->spawnsMonster.loadFromXML(map->monsterfile)) {
		return std::unexpected(std::format("Failed to load monster file: {}", map->monsterfile));
	}

	return true;
}

std::expected<bool, std::string> IOMap::loadZones(Map* map) {
	if (map->zonesfile.empty()) {
		map->zonesfile = std::format("{}-zones.xml", g_configManager().getString(MAP_NAME));
	}

	if (!Zone::loadFromXML(map->zonesfile)) {
		return std::unexpected(std::format("Failed to load zones file: {}", map->zonesfile));
	}

	return true;
}

std::expected<bool, std::string> IOMap::loadNpcs(Map* map) {
	if (map->npcfile.empty()) {
		map->npcfile = std::format("{}-npc.xml", g_configManager().getString(MAP_NAME));
	}

	if (!map->spawnsNpc.loadFromXml(map->npcfile)) {
		return std::unexpected(std::format("Failed to load NPC file: {}", map->npcfile));
	}

	return true;
}

std::expected<bool, std::string> IOMap::loadHouses(Map* map) {
	if (map->housefile.empty()) {
		map->housefile = std::format("{}-house.xml", g_configManager().getString(MAP_NAME));
	}

	if (!map->houses.loadHousesXML(map->housefile)) {
		return std::unexpected(std::format("Failed to load houses file: {}", map->housefile));
	}

	return true;
}

// Implementações específicas para mapas customizados
std::expected<bool, std::string> IOMap::loadMonstersCustom(Map* map, std::string_view mapName, int customMapIndex) {
	std::string fileName = getFullPath(map->monsterfile, mapName, "-monster.xml");

	if (!map->spawnsMonsterCustomMaps[customMapIndex].loadFromXML(fileName)) {
		return std::unexpected(std::format("Failed to load monster file: {}", fileName));
	}

	return true;
}

std::expected<bool, std::string> IOMap::loadZonesCustom(const Map* map, std::string_view mapName, int customMapIndex) {
	// Variável para armazenar o caminho completo do arquivo
	std::string fullPath;

	// Se já temos um arquivo de zona definido, usamos ele como base
	if (!map->zonesfile.empty()) {
		fullPath = map->zonesfile;
	} else {
		// Construir o caminho completo baseado no nome do mapa
		// Importante: Incluir o diretório completo!
		fullPath = std::format("{}/world/custom/{}-zones.xml", g_configManager().getString(DATA_DIRECTORY), mapName);
	}

	// Exibir o caminho para debug
	g_logger().debug("Loading zones from: {}", fullPath);

	// Carregar as zonas a partir do caminho completo
	if (!Zone::loadFromXML(fullPath, customMapIndex)) {
		return std::unexpected(std::format("Failed to load zones file: {}", fullPath));
	}

	return true;
}

std::expected<bool, std::string> IOMap::loadNpcsCustom(Map* map, std::string_view mapName, int customMapIndex) {
	std::string fileName = getFullPath(map->npcfile, mapName, "-npc.xml");

	if (!map->spawnsNpcCustomMaps[customMapIndex].loadFromXml(fileName)) {
		return std::unexpected(std::format("Failed to load NPC file: {}", fileName));
	}

	return true;
}

std::expected<bool, std::string> IOMap::loadHousesCustom(Map* map, std::string_view mapName, int customMapIndex) {
	std::string fileName = getFullPath(map->housefile, mapName, "-house.xml");

	if (!map->housesCustomMaps[customMapIndex].loadHousesXML(fileName)) {
		return std::unexpected(std::format("Failed to load houses file: {}", fileName));
	}

	return true;
}

// Carregar todos os recursos sequencialmente
std::expected<bool, std::string> IOMap::loadAllResources(Map* map) {
	Benchmark bm_loadResources;

	// Carregar monstros
	auto monstersResult = loadMonsters(map);
	if (!monstersResult) {
		return monstersResult;
	}

	// Carregar zonas
	auto zonesResult = loadZones(map);
	if (!zonesResult) {
		return zonesResult;
	}

	// Carregar NPCs
	auto npcsResult = loadNpcs(map);
	if (!npcsResult) {
		return npcsResult;
	}

	// Carregar casas
	auto housesResult = loadHouses(map);
	if (!housesResult) {
		return housesResult;
	}

	g_logger().debug("All resources loaded in {} milliseconds", bm_loadResources.duration());
	return true;
}

// Carregar todos os recursos customizados sequencialmente
std::expected<bool, std::string> IOMap::loadAllResourcesCustom(Map* map, std::string_view mapName, int customMapIndex) {
	Benchmark bm_loadCustomResources;

	// Carregar monstros
	auto monstersResult = loadMonstersCustom(map, mapName, customMapIndex);
	if (!monstersResult) {
		return monstersResult;
	}

	// Carregar zonas
	auto zonesResult = loadZonesCustom(map, mapName, customMapIndex);
	if (!zonesResult) {
		return zonesResult;
	}

	// Carregar NPCs
	auto npcsResult = loadNpcsCustom(map, mapName, customMapIndex);
	if (!npcsResult) {
		return npcsResult;
	}

	// Carregar casas
	auto housesResult = loadHousesCustom(map, mapName, customMapIndex);
	if (!housesResult) {
		return housesResult;
	}

	g_logger().debug("All custom resources for {} (index {}) loaded in {} milliseconds", mapName, customMapIndex, bm_loadCustomResources.duration());
	return true;
}
