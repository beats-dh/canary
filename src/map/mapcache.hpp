/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include "items/items_definitions.hpp"
#include "utils/mapsector.hpp"

class Map;
class Tile;
class Item;
struct Position;
class FileStream;

// Estrutura hash para usar com unordered_map
struct identity_hash {
	constexpr size_t operator()(const size_t &v) const noexcept {
		return v;
	}
};

#pragma pack(1)
struct BasicItem {
	std::string text;
	// size_t description { 0 };

	uint16_t id { 0 };

	uint16_t charges { 0 }; // Runecharges and Count Too
	uint16_t actionId { 0 };
	uint16_t uniqueId { 0 };
	uint16_t destX { 0 }, destY { 0 };
	uint16_t doorOrDepotId { 0 };

	uint8_t destZ { 0 };

	std::vector<std::shared_ptr<BasicItem>> items;

	std::expected<bool, std::string> unserializeItemNode(FileStream &propStream, uint16_t x, uint16_t y, uint8_t z);
	void readAttr(FileStream &propStream);

	[[nodiscard]] size_t hash() const noexcept {
		size_t h = 0;
		hash(h);
		return h;
	}

private:
	void hash(size_t &h) const;

	friend struct BasicTile;
};

struct BasicTile {
	std::shared_ptr<BasicItem> ground { nullptr };
	std::vector<std::shared_ptr<BasicItem>> items;

	uint32_t flags { 0 }, houseId { 0 };
	uint8_t type { TILESTATE_NONE };

	bool isStatic { false };

	[[nodiscard]] constexpr bool isEmpty(bool ignoreFlag = false) const noexcept {
		return (ignoreFlag || flags == 0) && ground == nullptr && items.empty();
	}

	[[nodiscard]] constexpr bool isHouse() const noexcept {
		return houseId != 0;
	}

	[[nodiscard]] size_t hash() const noexcept {
		size_t h = 0;
		hash(h);
		return h;
	}

private:
	void hash(size_t &h) const;
};

#pragma pack()

class MapCache {
public:
	virtual ~MapCache() = default;

	void setBasicTile(uint16_t x, uint16_t y, uint8_t z, const std::shared_ptr<BasicTile> &basicTile);

	std::shared_ptr<BasicItem> tryReplaceItemFromCache(const std::shared_ptr<BasicItem> &ref) const;

	void flush() const;

	/**
	 * Creates a map sector.
	 * \returns A pointer to that map sector.
	 */
	MapSector* createMapSector(uint32_t x, uint32_t y);
	MapSector* getBestMapSector(uint32_t x, uint32_t y);

	/**
	 * Gets a map sector.
	 * \returns A pointer to that map sector.
	 */
	[[nodiscard]] MapSector* getMapSector(const uint32_t x, const uint32_t y) {
		const auto it = mapSectors.find(x / SECTOR_SIZE | y / SECTOR_SIZE << 16);
		return it != mapSectors.end() ? &it->second : nullptr;
	}

	[[nodiscard]] const MapSector* getMapSector(const uint32_t x, const uint32_t y) const {
		const auto it = mapSectors.find(x / SECTOR_SIZE | y / SECTOR_SIZE << 16);
		return it != mapSectors.end() ? &it->second : nullptr;
	}

protected:
	std::shared_ptr<Tile> getOrCreateTileFromCache(const std::shared_ptr<Floor> &floor, uint16_t x, uint16_t y);

	std::unordered_map<uint32_t, MapSector> mapSectors;

private:
	void parseItemAttr(const std::shared_ptr<BasicItem> &basicItem, const std::shared_ptr<Item> &item) const;
	std::shared_ptr<Item> createItem(const std::shared_ptr<BasicItem> &basicItem, Position position);

	// Nova sobrecarga para otimização de movimento
	std::shared_ptr<Item> createItem(std::shared_ptr<BasicItem> &&basicItem, Position position);
};
