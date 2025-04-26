/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "game/movement/position.hpp"
#include "utils/tools.hpp"

double Position::getEuclideanDistance(const Position &p1, const Position &p2) {
	const int32_t dx = Position::getDistanceX(p1, p2);
	const int32_t dy = Position::getDistanceY(p1, p2);
	return std::sqrt(static_cast<double>(dx * dx + dy * dy));
}

Direction Position::getRandomDirection() {
	// Usar um array constexpr é mais eficiente que um vetor
	static constexpr std::array<Direction, 4> directions = {
		DIRECTION_NORTH,
		DIRECTION_WEST,
		DIRECTION_EAST,
		DIRECTION_SOUTH
	};

	// Criar uma cópia local para embaralhar
	std::array<Direction, 4> dirList = directions;
	std::ranges::shuffle(dirList, getRandomGenerator());

	return dirList.front();
}

std::ostream &operator<<(std::ostream &os, const Position &pos) {
	return os << pos.toString();
}

std::ostream &operator<<(std::ostream &os, const Direction &dir) {
	// Usar array constexpr com inicialização por índice é mais eficiente que um map para um enum
	static constexpr std::array<std::string_view, 9> directionStrings = {
		"North", // DIRECTION_NORTH (0)
		"East", // DIRECTION_EAST (1)
		"South", // DIRECTION_SOUTH (2)
		"West", // DIRECTION_WEST (3)
		"South-West", // DIRECTION_SOUTHWEST (4)
		"South-East", // DIRECTION_SOUTHEAST (5)
		"North-West", // DIRECTION_NORTHWEST (6)
		"North-East", // DIRECTION_NORTHEAST (7)
		"None" // DIRECTION_NONE (8)
	};

	// Verificar se o índice é válido
	if (dir <= DIRECTION_NONE) {
		return os << directionStrings[dir];
	}

	return os << "Unknown Direction";
}
