/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include "mapcache.hpp"
#include "map/town.hpp"
#include "map/house/house.hpp"
#include "creatures/monsters/spawns/spawn_monster.hpp"
#include "creatures/npcs/spawns/spawn_npc.hpp"

class Creature;
class Player;
class Game;
class Tile;
class Map;

struct FindPathParams;
class FrozenPathingConditionCall;

/**
 * Result type for map operations that can fail
 */
using MapResult = std::expected<bool, std::string>;

/**
 * Map class.
 * Holds all the actual map-data
 */
class Map final : public MapCache {
public:
	/**
	 * Constants for maximum custom maps
	 */
	static constexpr size_t MAX_CUSTOM_MAPS = 50;

	/**
	 * Clean the map and return number of items removed
	 */
	[[nodiscard]] uint32_t clean() const;

	/**
	 * Get the map path
	 */
	[[nodiscard]] std::filesystem::path getPath() const {
		return path;
	}

	/**
	 * Load a map.
	 * @param identifier Map file path
	 * @param pos Starting position
	 */
	void load(std::string_view identifier, const Position &pos = Position());

	/**
	 * Load the main map
	 * @param identifier Map file name (.otbm)
	 * @param mainMap Whether this is the main map
	 * @param loadHouses Whether to load houses
	 * @param loadMonsters Whether to load monsters
	 * @param loadNpcs Whether to load NPCs
	 * @param loadZones Whether to load zones
	 * @param pos Starting position
	 */
	void loadMap(std::string_view identifier, bool mainMap = false, bool loadHouses = false, bool loadMonsters = false, bool loadNpcs = false, bool loadZones = false, const Position &pos = Position());

	/**
	 * Load a custom map
	 * @param mapName Name of the custom map
	 * @param loadHouses Whether to load houses
	 * @param loadMonsters Whether to load monsters
	 * @param loadNpcs Whether to load NPCs
	 * @param loadZones Whether to load zones
	 * @param customMapIndex Index of the custom map
	 */
	void loadMapCustom(std::string_view mapName, bool loadHouses, bool loadMonsters, bool loadNpcs, bool loadZones, int customMapIndex);

	/**
	 * Load house information
	 */
	void loadHouseInfo();

	/**
	 * Save map data
	 * @return Whether the save was successful
	 */
	[[nodiscard]] static bool save();

	/**
	 * Get a single tile
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @param z Z coordinate
	 * @return Pointer to the tile or nullptr if not found
	 */
	[[nodiscard]] std::shared_ptr<Tile> getTile(uint16_t x, uint16_t y, uint8_t z);

	/**
	 * Get a single tile by position
	 * @param pos Position
	 * @return Pointer to the tile or nullptr if not found
	 */
	[[nodiscard]] std::shared_ptr<Tile> getTile(const Position &pos) {
		return getTile(pos.x, pos.y, pos.z);
	}

	/**
	 * Refresh zones at a specific position
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @param z Z coordinate
	 */
	void refreshZones(uint16_t x, uint16_t y, uint8_t z);

	/**
	 * Refresh zones at a specific position
	 * @param pos Position
	 */
	void refreshZones(const Position &pos) {
		refreshZones(pos.x, pos.y, pos.z);
	}

	/**
	 * Get or create a tile at a specific position
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @param z Z coordinate
	 * @param isDynamic Whether the tile is dynamic
	 * @return Pointer to the tile
	 */
	[[nodiscard]] std::shared_ptr<Tile> getOrCreateTile(uint16_t x, uint16_t y, uint8_t z, bool isDynamic = false);

	/**
	 * Get or create a tile at a specific position
	 * @param pos Position
	 * @param isDynamic Whether the tile is dynamic
	 * @return Pointer to the tile
	 */
	[[nodiscard]] std::shared_ptr<Tile> getOrCreateTile(const Position &pos, bool isDynamic = false) {
		return getOrCreateTile(pos.x, pos.y, pos.z, isDynamic);
	}

	/**
	 * Place a creature on the map
	 * @param centerPos Center position
	 * @param creature Creature to place
	 * @param extendedPos Whether to use extended position finding
	 * @param forceLogin Whether to force login even if obstacles exist
	 * @return Whether the creature was placed successfully
	 */
	[[nodiscard]] bool placeCreature(const Position &centerPos, const std::shared_ptr<Creature> &creature, bool extendedPos = false, bool forceLogin = false);

	/**
	 * Move a creature on the map
	 * @param creature Creature to move
	 * @param newTile Destination tile
	 * @param forceTeleport Whether to force teleport
	 */
	void moveCreature(const std::shared_ptr<Creature> &creature, const std::shared_ptr<Tile> &newTile, bool forceTeleport = false);

	/**
	 * Check if an object can be thrown to a position
	 * @param fromPos Source position
	 * @param toPos Destination position
	 * @param lineOfSight Line of sight check type
	 * @param rangex Maximum X range
	 * @param rangey Maximum Y range
	 * @return Whether the throw is possible
	 */
	[[nodiscard]] bool canThrowObjectTo(const Position &fromPos, const Position &toPos, SightLines_t lineOfSight = SightLine_CheckSightLine, int32_t rangex = MAP_MAX_CLIENT_VIEW_PORT_X, int32_t rangey = MAP_MAX_CLIENT_VIEW_PORT_Y);

	/**
	 * Check if there is clear line of sight between positions
	 * @param fromPos Source position
	 * @param toPos Destination position
	 * @param floorCheck Whether to check floor difference
	 * @return Whether there is clear line of sight
	 */
	[[nodiscard]] bool isSightClear(const Position &fromPos, const Position &toPos, bool floorCheck);

	/**
	 * Check if there is a sight line between positions
	 * @param start Start position
	 * @param destination Destination position
	 * @return Whether there is a sight line
	 */
	[[nodiscard]] bool checkSightLine(Position start, Position destination);

	/**
	 * Check if a creature can walk to a position
	 * @param creature Creature
	 * @param pos Destination position
	 * @return Pointer to the tile if walkable, nullptr otherwise
	 */
	[[nodiscard]] std::shared_ptr<Tile> canWalkTo(const std::shared_ptr<Creature> &creature, const Position &pos);

	/**
	 * Get path matching for a creature
	 * @param creature Creature
	 * @param dirList Direction list to be filled
	 * @param pathCondition Path condition
	 * @param fpp Path finding parameters
	 * @return Whether a path was found
	 */
	[[nodiscard]] bool getPathMatching(const std::shared_ptr<Creature> &creature, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp);

	/**
	 * Get path matching for a creature to a target position
	 * @param creature Creature
	 * @param targetPos Target position
	 * @param dirList Direction list to be filled
	 * @param pathCondition Path condition
	 * @param fpp Path finding parameters
	 * @return Whether a path was found
	 */
	[[nodiscard]] bool getPathMatching(const std::shared_ptr<Creature> &creature, const Position &targetPos, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp);

	/**
	 * Get path matching for a creature to a target position with conditions
	 * @param creature Creature
	 * @param targetPos Target position
	 * @param dirList Direction list to be filled
	 * @param pathCondition Path condition
	 * @param fpp Path finding parameters
	 * @return Whether a path was found
	 */
	[[nodiscard]] bool getPathMatchingCond(const std::shared_ptr<Creature> &creature, const Position &targetPos, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp);

	/**
	 * Get path matching from a start position
	 * @param startPos Start position
	 * @param dirList Direction list to be filled
	 * @param pathCondition Path condition
	 * @param fpp Path finding parameters
	 * @return Whether a path was found
	 */
	[[nodiscard]] bool getPathMatching(const Position &startPos, std::vector<Direction> &dirList, const FrozenPathingConditionCall &pathCondition, const FindPathParams &fpp) {
		return getPathMatching(nullptr, startPos, dirList, pathCondition, fpp);
	}

	// Waypoints map
	std::map<std::string, Position> waypoints;

	// Storage for the main map
	SpawnsMonster spawnsMonster;
	SpawnsNpc spawnsNpc;
	Towns towns;
	Houses houses;

	// Storage for custom maps
	std::array<SpawnsMonster, MAX_CUSTOM_MAPS> spawnsMonsterCustomMaps;
	std::array<SpawnsNpc, MAX_CUSTOM_MAPS> spawnsNpcCustomMaps;
	std::array<Houses, MAX_CUSTOM_MAPS> housesCustomMaps;

private:
	/**
	 * Set a tile at a specific position
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @param z Z coordinate
	 * @param newTile New tile
	 */
	void setTile(uint16_t x, uint16_t y, uint8_t z, const std::shared_ptr<Tile> &newTile);

	/**
	 * Set a tile at a specific position
	 * @param pos Position
	 * @param newTile New tile
	 */
	void setTile(const Position &pos, const std::shared_ptr<Tile> &newTile) {
		setTile(pos.x, pos.y, pos.z, newTile);
	}

	/**
	 * Get a loaded tile without creating if not exists
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @param z Z coordinate
	 * @return Pointer to the tile or nullptr if not found
	 */
	[[nodiscard]] std::shared_ptr<Tile> getLoadedTile(uint16_t x, uint16_t y, uint8_t z);

	// Map file path
	std::filesystem::path path;

	// Resource file paths
	std::string monsterfile;
	std::string housefile;
	std::string npcfile;
	std::string zonesfile;

	// Map dimensions
	uint32_t width = 0;
	uint32_t height = 0;

	// Friend classes
	friend class Game;
	friend class IOMap;
	friend class MapCache;
};
