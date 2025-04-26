/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "map/mapcache.hpp"

#include <memory_resource>
#include <ranges>
#include <algorithm>
#include <source_location>

#include "game/movement/teleport.hpp"
#include "game/zones/zone.hpp"
#include "io/filestream.hpp"
#include "items/containers/depot/depotlocker.hpp"
#include "items/item.hpp"
#include "map/map.hpp"
#include "utils/hash.hpp"

static phmap::flat_hash_map<size_t, std::shared_ptr<BasicItem>> items;
static phmap::flat_hash_map<size_t, std::shared_ptr<BasicTile>> tiles;

std::shared_ptr<BasicItem> static_tryGetItemFromCache(const std::shared_ptr<BasicItem> &ref) {
	return ref ? items.try_emplace(ref->hash(), ref).first->second : nullptr;
}

std::shared_ptr<BasicTile> static_tryGetTileFromCache(const std::shared_ptr<BasicTile> &ref) {
	return ref ? tiles.try_emplace(ref->hash(), ref).first->second : nullptr;
}

void MapCache::flush() const {
	items.clear();
	tiles.clear();
}

void MapCache::parseItemAttr(const std::shared_ptr<BasicItem> &basicItem, const std::shared_ptr<Item> &item) const {
	if (basicItem->charges > 0) {
		item->setSubType(basicItem->charges);
	}

	if (basicItem->actionId > 0) {
		item->setAttribute(ItemAttribute_t::ACTIONID, basicItem->actionId);
	}

	if (basicItem->uniqueId > 0) {
		item->addUniqueId(basicItem->uniqueId);
	}

	if (item->getTeleport() && (basicItem->destX != 0 || basicItem->destY != 0 || basicItem->destZ != 0)) {
		const auto dest = Position(basicItem->destX, basicItem->destY, basicItem->destZ);
		item->getTeleport()->setDestPos(dest);
	}

	if (item->getDoor() && basicItem->doorOrDepotId != 0) {
		item->getDoor()->setDoorId(basicItem->doorOrDepotId);
	}

	if (item->getContainer() && item->getContainer()->getDepotLocker() && basicItem->doorOrDepotId != 0) {
		item->getContainer()->getDepotLocker()->setDepotId(basicItem->doorOrDepotId);
	}

	if (!basicItem->text.empty()) {
		item->setAttribute(ItemAttribute_t::TEXT, basicItem->text);
	}

	/* if (basicItem.description != 0)
	    item->setAttribute(ItemAttribute_t::DESCRIPTION, STRING_CACHE[basicItem.description]);*/
}

std::shared_ptr<Item> MapCache::createItem(const std::shared_ptr<BasicItem> &basicItem, Position position) {
	const auto &item = Item::CreateItem(basicItem->id, position);
	if (!item) {
		return nullptr;
	}

	parseItemAttr(basicItem, item);

	if (item->getContainer() && !basicItem->items.empty()) {
		// Reserva espaço antecipadamente para evitar realocações
		const auto containerSize = basicItem->items.size();
		// item->getContainer()->reserve(containerSize);

		// Usa views para transformar os elementos
		auto itemsView = basicItem->items | std::views::transform([this, &position](const auto &basicItemInside) {
							 return createItem(basicItemInside, position);
						 });

		// Adiciona os itens filtrados (não nulos)
		for (const auto &itemInside : itemsView) {
			if (itemInside) {
				item->getContainer()->addItem(itemInside);
				item->getContainer()->updateItemWeight(itemInside->getWeight());
			}
		}
	}

	if (item->getItemCount() == 0) {
		item->setItemCount(1);
	}

	if (item->canDecay()) {
		item->startDecaying();
	}
	item->loadedFromMap = true;
	item->decayDisabled = Item::items[item->getID()].decayTo != -1;

	return item;
}

// Sobrecarga otimizada para move semantics
std::shared_ptr<Item> MapCache::createItem(std::shared_ptr<BasicItem> &&basicItem, Position position) {
	const auto &item = Item::CreateItem(basicItem->id, position);
	if (!item) {
		return nullptr;
	}

	parseItemAttr(basicItem, item);

	if (item->getContainer() && !basicItem->items.empty()) {
		// Reserva espaço antecipadamente
		const auto containerSize = basicItem->items.size();
		// item->getContainer()->reserve(containerSize);

		// Move os items (transfer ownership)
		for (auto &&basicItemInside : std::move(basicItem->items)) {
			if (auto itemInside = createItem(std::move(basicItemInside), position)) {
				item->getContainer()->addItem(itemInside);
				item->getContainer()->updateItemWeight(itemInside->getWeight());
			}
		}
	}

	if (item->getItemCount() == 0) {
		item->setItemCount(1);
	}

	if (item->canDecay()) {
		item->startDecaying();
	}
	item->loadedFromMap = true;
	item->decayDisabled = Item::items[item->getID()].decayTo != -1;

	return item;
}

std::shared_ptr<Tile> MapCache::getOrCreateTileFromCache(const std::shared_ptr<Floor> &floor, uint16_t x, uint16_t y) {
	const auto &cachedTile = floor->getTileCache(x, y);
	const auto oldTile = floor->getTile(x, y);
	if (!cachedTile) {
		return oldTile;
	}

	std::unique_lock l(floor->getMutex());

	const uint8_t z = floor->getZ();
	const auto map = static_cast<Map*>(this);

	std::vector<std::shared_ptr<Creature>> oldCreatureList;
	if (oldTile) {
		if (CreatureVector* creatures = oldTile->getCreatures()) {
			// Reserva espaço para evitar realocações
			oldCreatureList.reserve(creatures->size());
			// Copia directamente todos os elementos
			std::ranges::copy(*creatures, std::back_inserter(oldCreatureList));
		}
	}

	std::shared_ptr<Tile> tile = nullptr;

	auto pos = Position(x, y, z);

	if (cachedTile->isHouse()) {
		if (const auto &house = map->houses.getHouse(cachedTile->houseId)) {
			tile = std::make_shared<HouseTile>(pos, house);
			tile->safeCall([tile] {
				tile->getHouse()->addTile(tile->static_self_cast<HouseTile>());
			});
		} else {
			g_logger().error("[{}] house not found for houseId {}", std::source_location::current().function_name(), cachedTile->houseId);
		}
	} else if (cachedTile->isStatic) {
		tile = std::make_shared<StaticTile>(pos);
	} else {
		tile = std::make_shared<DynamicTile>(pos);
	}

	if (cachedTile->ground != nullptr) {
		tile->internalAddThing(createItem(cachedTile->ground, pos));
	}

	// Pré-alocação e transformação com views
	if (!cachedTile->items.empty()) {
		auto itemsView = cachedTile->items | std::views::transform([this, &pos](const auto &basicItem) {
							 return createItem(basicItem, pos);
						 });

		for (const auto &item : itemsView) {
			tile->internalAddThing(item);
		}
	}

	tile->setFlag(static_cast<TileFlags_t>(cachedTile->flags));

	tile->safeCall([tile, pos, movedOldCreatureList = std::move(oldCreatureList)]() {
		for (const auto &creature : movedOldCreatureList) {
			tile->internalAddThing(creature);
		}

		for (const auto &zone : Zone::getZones(pos)) {
			tile->addZone(zone);
		}
	});

	floor->setTile(x, y, tile);

	// Remove Tile from cache
	floor->setTileCache(x, y, nullptr);

	return tile;
}

void MapCache::setBasicTile(uint16_t x, uint16_t y, uint8_t z, const std::shared_ptr<BasicTile> &newTile) {
	if (z >= MAP_MAX_LAYERS) {
		g_logger().error("[{}] Attempt to set tile on invalid coordinate: {}", std::source_location::current().function_name(), Position(x, y, z).toString());
		return;
	}

	const auto &tile = static_tryGetTileFromCache(newTile);
	if (const auto sector = getMapSector(x, y)) {
		sector->createFloor(z)->setTileCache(x, y, tile);
	} else {
		getBestMapSector(x, y)->createFloor(z)->setTileCache(x, y, tile);
	}
}

std::shared_ptr<BasicItem> MapCache::tryReplaceItemFromCache(const std::shared_ptr<BasicItem> &ref) const {
	return static_tryGetItemFromCache(ref);
}

MapSector* MapCache::createMapSector(const uint32_t x, const uint32_t y) {
	const uint32_t index = x / SECTOR_SIZE | y / SECTOR_SIZE << 16;
	const auto it = mapSectors.find(index);
	if (it != mapSectors.end()) {
		return &it->second;
	}

	MapSector::newSector = true;
	return &mapSectors[index];
}

MapSector* MapCache::getBestMapSector(uint32_t x, uint32_t y) {
	MapSector::newSector = false;
	const auto sector = createMapSector(x, y);

	if (MapSector::newSector) {
		// update north sector
		if (const auto northSector = getMapSector(x, y - SECTOR_SIZE)) {
			northSector->sectorS = sector;
		}

		// update west sector
		if (const auto westSector = getMapSector(x - SECTOR_SIZE, y)) {
			westSector->sectorE = sector;
		}

		// update south sector
		if (const auto southSector = getMapSector(x, y + SECTOR_SIZE)) {
			sector->sectorS = southSector;
		}

		// update east sector
		if (const auto eastSector = getMapSector(x + SECTOR_SIZE, y)) {
			sector->sectorE = eastSector;
		}
	}

	return sector;
}

void BasicTile::hash(size_t &h) const {
	// Processar valores diretamente
	if (flags > 0) {
		stdext::hash_combine(h, flags);
	}
	if (houseId > 0) {
		stdext::hash_combine(h, houseId);
	}
	if (type > 0) {
		stdext::hash_combine(h, type);
	}
	if (isStatic) {
		stdext::hash_combine(h, 1); // isStatic é bool
	}

	if (ground != nullptr) {
		ground->hash(h);
	}

	if (!items.empty()) {
		stdext::hash_combine(h, items.size());
		for (const auto &item : items) {
			item->hash(h);
		}
	}
}

void BasicItem::hash(size_t &h) const {
	// Processar valores diretamente
	if (id > 0) {
		stdext::hash_combine(h, id);
	}
	if (charges > 0) {
		stdext::hash_combine(h, charges);
	}
	if (actionId > 0) {
		stdext::hash_combine(h, actionId);
	}
	if (uniqueId > 0) {
		stdext::hash_combine(h, uniqueId);
	}
	if (destX > 0) {
		stdext::hash_combine(h, destX);
	}
	if (destY > 0) {
		stdext::hash_combine(h, destY);
	}
	if (destZ > 0) {
		stdext::hash_combine(h, destZ);
	}
	if (doorOrDepotId > 0) {
		stdext::hash_combine(h, doorOrDepotId);
	}

	if (!text.empty()) {
		stdext::hash_combine(h, text);
	}

	if (!items.empty()) {
		stdext::hash_combine(h, items.size());
		for (const auto &item : items) {
			item->hash(h);
		}
	}
}

std::expected<bool, std::string> BasicItem::unserializeItemNode(FileStream &stream, uint16_t x, uint16_t y, uint8_t z) {
	if (stream.isProp(OTB::Node::END)) {
		stream.back();
		return true;
	}

	readAttr(stream);

	while (stream.startNode()) {
		if (stream.getU8() != OTBM_ITEM) {
			return std::unexpected(fmt::format("[x:{}, y:{}, z:{}] Could not read item node.", x, y, z));
		}

		const uint16_t streamId = stream.getU16();

		const auto item = std::make_shared<BasicItem>();
		item->id = streamId;

		auto result = item->unserializeItemNode(stream, x, y, z);
		if (!result) {
			return std::unexpected(fmt::format("[x:{}, y:{}, z:{}] Failed to load item: {}", x, y, z, result.error()));
		}

		items.emplace_back(static_tryGetItemFromCache(item));

		if (!stream.endNode()) {
			return std::unexpected(fmt::format("[x:{}, y:{}, z:{}] Could not end node.", x, y, z));
		}
	}

	return true;
}

void BasicItem::readAttr(FileStream &stream) {
	bool end = false;
	while (!end) {
		const uint8_t attr = stream.getU8();
		switch (attr) {
			case ATTR_DEPOT_ID: {
				doorOrDepotId = stream.getU16();
			} break;

			case ATTR_HOUSEDOORID: {
				doorOrDepotId = stream.getU8();
			} break;

			case ATTR_TELE_DEST: {
				destX = stream.getU16();
				destY = stream.getU16();
				destZ = stream.getU8();
			} break;

			case ATTR_COUNT: {
				charges = stream.getU8();
			} break;

			case ATTR_CHARGES: {
				charges = stream.getU16();
			} break;

			case ATTR_ACTION_ID: {
				actionId = stream.getU16();
			} break;

			case ATTR_UNIQUE_ID: {
				uniqueId = stream.getU16();
			} break;

			case ATTR_TEXT: {
				const auto str = stream.getString();
				if (!str.empty()) {
					text = str;
				}
			} break;

			case ATTR_DESC: {
				// Atualmente não utilizado, apenas lê e descarta
				stream.getString();
			} break;

			default:
				stream.back();
				end = true;
				break;
		}
	}
}
