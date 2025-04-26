/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

enum Direction : uint8_t {
	DIRECTION_NORTH = 0,
	DIRECTION_EAST = 1,
	DIRECTION_SOUTH = 2,
	DIRECTION_WEST = 3,

	DIRECTION_DIAGONAL_MASK = 4,
	DIRECTION_SOUTHWEST = DIRECTION_DIAGONAL_MASK | 0,
	DIRECTION_SOUTHEAST = DIRECTION_DIAGONAL_MASK | 1,
	DIRECTION_NORTHWEST = DIRECTION_DIAGONAL_MASK | 2,
	DIRECTION_NORTHEAST = DIRECTION_DIAGONAL_MASK | 3,

	DIRECTION_LAST = DIRECTION_NORTHEAST,
	DIRECTION_NONE = 8,
};

struct Position {
	constexpr Position() = default;
	constexpr Position(uint16_t initX, uint16_t initY, uint8_t initZ) :
		x(initX), y(initY), z(initZ) { }

	template <int_fast32_t deltax, int_fast32_t deltay>
	[[nodiscard]] static constexpr bool areInRange(const Position &p1, const Position &p2) {
		return Position::getDistanceX(p1, p2) <= deltax && Position::getDistanceY(p1, p2) <= deltay;
	}

	template <int_fast32_t deltax, int_fast32_t deltay, int_fast16_t deltaz>
	[[nodiscard]] static constexpr bool areInRange(const Position &p1, const Position &p2) {
		return Position::getDistanceX(p1, p2) <= deltax && Position::getDistanceY(p1, p2) <= deltay && Position::getDistanceZ(p1, p2) <= deltaz;
	}

	[[nodiscard]] static constexpr int_fast32_t getOffsetX(const Position &p1, const Position &p2) {
		return p1.getX() - p2.getX();
	}

	[[nodiscard]] static constexpr int_fast32_t getOffsetY(const Position &p1, const Position &p2) {
		return p1.getY() - p2.getY();
	}

	[[nodiscard]] static int_fast16_t getOffsetZ(const Position &p1, const Position &p2) {
		return p1.getZ() - p2.getZ();
	}

	[[nodiscard]] static int32_t getDistanceX(const Position &p1, const Position &p2) {
		return std::abs(Position::getOffsetX(p1, p2));
	}

	[[nodiscard]] static int32_t getDistanceY(const Position &p1, const Position &p2) {
		return std::abs(Position::getOffsetY(p1, p2));
	}

	[[nodiscard]] static int16_t getDistanceZ(const Position &p1, const Position &p2) {
		return std::abs(Position::getOffsetZ(p1, p2));
	}

	[[nodiscard]] static constexpr int32_t getDiagonalDistance(const Position &p1, const Position &p2) {
		return std::max(Position::getDistanceX(p1, p2), Position::getDistanceY(p1, p2));
	}

	[[nodiscard]] static double getEuclideanDistance(const Position &p1, const Position &p2);

	[[nodiscard]] static Direction getRandomDirection();

	uint16_t x = 0;
	uint16_t y = 0;
	uint8_t z = 0;

	[[nodiscard]] auto operator<=>(const Position &p) const noexcept {
		if (const auto cmp = z <=> p.z; cmp != 0) {
			return cmp;
		}
		if (const auto cmp = y <=> p.y; cmp != 0) {
			return cmp;
		}
		return x <=> p.x;
	}

	[[nodiscard]] bool operator==(const Position &p) const = default;

	[[nodiscard]] Position operator+(const Position &p1) const {
		return Position(x + p1.x, y + p1.y, z + p1.z);
	}

	[[nodiscard]] Position operator-(const Position &p1) const {
		return Position(x - p1.x, y - p1.y, z - p1.z);
	}

	[[nodiscard]] std::string toString() const {
		return std::format("( {}, {}, {} )", getX(), getY(), getZ());
	}

	[[nodiscard]] constexpr int_fast32_t getX() const {
		return x;
	}

	[[nodiscard]] constexpr int_fast32_t getY() const {
		return y;
	}

	[[nodiscard]] constexpr int_fast16_t getZ() const {
		return z;
	}
};

namespace std {
	template <>
	struct hash<Position> {
		[[nodiscard]] std::size_t operator()(const Position &p) const noexcept {
			return static_cast<std::size_t>(p.x) | (static_cast<std::size_t>(p.y) << 16) | (static_cast<std::size_t>(p.z) << 32);
		}
	};
}

std::ostream &operator<<(std::ostream &, const Position &);
std::ostream &operator<<(std::ostream &, const Direction &);
