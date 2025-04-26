/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "io/filestream.hpp"
#include "io/fileloader.hpp"

uint8_t FileStream::getU8() noexcept {
	// Verificação rápida de limites
	if (m_pos >= m_data.size()) {
		g_logger().error("Failed to getU8: end of stream");
		return 0;
	}

	if (m_nodes > 0 && m_data[m_pos] == static_cast<std::byte>(OTB::Node::ESCAPE)) {
		++m_pos;
		if (m_pos >= m_data.size()) {
			g_logger().error("Failed to getU8: escape at end of stream");
			return 0;
		}
	}
	return static_cast<uint8_t>(m_data[m_pos++]);
}

uint16_t FileStream::getU16() noexcept {
	uint16_t v = 0;
	if (m_nodes > 0) {
		read(v, true);
	} else {
		if (m_pos + sizeof(uint16_t) <= m_data.size()) {
			std::memcpy(&v, &m_data[m_pos], sizeof(uint16_t));
			m_pos += sizeof(uint16_t);
		} else {
			g_logger().error("Failed to getU16: buffer overflow");
		}
	}
	return v;
}

uint32_t FileStream::getU32() noexcept {
	uint32_t v = 0;
	if (m_nodes > 0) {
		read(v, true);
	} else {
		if (m_pos + sizeof(uint32_t) <= m_data.size()) {
			std::memcpy(&v, &m_data[m_pos], sizeof(uint32_t));
			m_pos += sizeof(uint32_t);
		} else {
			g_logger().error("Failed to getU32: buffer overflow");
		}
	}
	return v;
}

uint64_t FileStream::getU64() noexcept {
	uint64_t v = 0;
	if (m_nodes > 0) {
		read(v, true);
	} else {
		if (m_pos + sizeof(uint64_t) <= m_data.size()) {
			std::memcpy(&v, &m_data[m_pos], sizeof(uint64_t));
			m_pos += sizeof(uint64_t);
		} else {
			g_logger().error("Failed to getU64: buffer overflow");
		}
	}
	return v;
}

std::string FileStream::getString() {
	constexpr uint16_t maxStringLen = 8192;
	const uint16_t len = getU16();

	if (len == 0) {
		return {};
	}

	if (len >= maxStringLen) {
		g_logger().error("Read failed: string too large ({})", len);
		return {};
	}

	if (m_pos + len > m_data.size()) {
		g_logger().error("Read failed: string would exceed buffer size");
		return {};
	}

	std::string str;
	str.reserve(len);
	str.assign(reinterpret_cast<const char*>(&m_data[m_pos]), len);
	m_pos += len;

	return str;
}

bool FileStream::isProp(const uint8_t prop, const bool toNext) noexcept {
	const uint32_t originalPos = m_pos;

	if (eof()) {
		return false;
	}

	const uint8_t byte = getU8();
	if (byte == prop) {
		if (!toNext) {
			m_pos = originalPos;
		}
		return true;
	}

	m_pos = originalPos;
	return false;
}

bool FileStream::startNode(const uint8_t type) noexcept {
	const uint32_t originalPos = m_pos;

	if (eof() || getU8() != OTB::Node::START) {
		m_pos = originalPos;
		return false;
	}

	if (type == 0) {
		++m_nodes;
		return true;
	}

	if (eof()) {
		m_pos = originalPos;
		return false;
	}

	const uint8_t nodeType = getU8();
	if (nodeType == type) {
		++m_nodes;
		return true;
	}

	m_pos = originalPos;
	return false;
}

bool FileStream::endNode() noexcept {
	const uint32_t originalPos = m_pos;

	if (eof() || getU8() != OTB::Node::END) {
		m_pos = originalPos;
		return false;
	}

	if (m_nodes > 0) {
		--m_nodes;
	} else {
		g_logger().warn("End node called with no open nodes");
	}
	return true;
}
