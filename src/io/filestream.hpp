/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

namespace OTB {
	struct Node;
}

/**
 * @class FileStream
 * @brief Classe ultra-otimizada para leitura de streams de arquivo com suporte a formatação OTB
 */
class FileStream {
public:
	/**
	 * @brief Construtor a partir de span de bytes
	 * @param data Span de bytes contendo os dados a serem lidos
	 */
	explicit FileStream(const std::span<const std::byte> data) noexcept :
		m_data(data) { }

	/**
	 * @brief Construtor compatível com a versão anterior usando ponteiros char
	 */
	FileStream(const char* begin, const char* end) :
		FileStream(std::span<const std::byte>(reinterpret_cast<const std::byte*>(begin), static_cast<size_t>(end - begin))) { }

	/**
	 * @brief Construtor a partir de mmap_source
	 * @param source O arquivo mapeado em memória
	 */
	explicit FileStream(const mio::mmap_source &source) :
		FileStream(std::span<const std::byte>(reinterpret_cast<const std::byte*>(source.data()), source.size())) { }

	[[nodiscard]] uint32_t tell() const noexcept {
		return m_pos;
	}

	void seek(const uint32_t pos) noexcept {
		if (pos > m_data.size()) {
			g_logger().error("Seek failed: position out of bounds");
			return;
		}
		m_pos = pos;
	}

	void skip(const uint32_t len) noexcept {
		const uint32_t newPos = m_pos + len;
		if (newPos < m_pos) {
			g_logger().error("Skip failed: uint32_t overflow");
			return;
		}
		seek(newPos);
	}

	[[nodiscard]] uint32_t size() const noexcept {
		const size_t dataSize = m_data.size();
		if (dataSize > std::numeric_limits<uint32_t>::max()) {
			g_logger().error("File size exceeds uint32_t range");
			return 0;
		}
		return static_cast<uint32_t>(dataSize);
	}

	void back(const uint32_t pos = 1) noexcept {
		if (pos > m_pos) {
			g_logger().error("Back failed: would result in negative position");
			m_pos = 0;
			return;
		}
		m_pos -= pos;
	}

	[[nodiscard]] bool startNode(uint8_t type = 0) noexcept;
	bool endNode() noexcept;
	[[nodiscard]] bool isProp(uint8_t prop, bool toNext = true) noexcept;

	uint8_t getU8() noexcept;
	uint16_t getU16() noexcept;
	uint32_t getU32() noexcept;
	uint64_t getU64() noexcept;
	std::string getString();

	[[nodiscard]] bool isValid() const noexcept {
		return m_pos <= m_data.size();
	}
	[[nodiscard]] bool eof() const noexcept {
		return m_pos >= m_data.size();
	}
	[[nodiscard]] uint32_t remainingBytes() const noexcept {
		return m_data.size() > m_pos ? static_cast<uint32_t>(m_data.size() - m_pos) : 0;
	}

	[[nodiscard]] std::span<const std::byte> rawData(const size_t offset = 0, const size_t length = std::numeric_limits<size_t>::max()) const noexcept {
		if (offset > m_data.size()) {
			return {};
		}
		const size_t maxLength = m_data.size() - offset;
		const size_t actualLength = std::min(length, maxLength);
		return m_data.subspan(offset, actualLength);
	}

	[[nodiscard]] std::span<const std::byte> currentData(const size_t length = std::numeric_limits<size_t>::max()) const noexcept {
		return rawData(m_pos, length);
	}

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	bool read(T &ret, bool escape = false) noexcept;

private:
	uint32_t m_nodes { 0 };

	uint32_t m_pos { 0 };

	std::span<const std::byte> m_data;
};

template <typename T>
	requires std::is_trivially_copyable_v<T>
bool FileStream::read(T &ret, const bool escape) noexcept {
	static constexpr auto size = sizeof(T);

	if (m_pos + size > m_data.size()) {
		g_logger().error("Read failed: buffer overflow");
		return false;
	}

	if constexpr (size == 1 && std::is_same_v<T, uint8_t>) {
		if (escape && m_data[m_pos] == static_cast<std::byte>(OTB::Node::ESCAPE)) {
			++m_pos;
			if (m_pos >= m_data.size()) {
				g_logger().error("Read failed: escape character at end of buffer");
				return false;
			}
		}
		ret = static_cast<uint8_t>(m_data[m_pos]);
		++m_pos;
		return true;
	} else if (escape) {
		std::array<std::byte, size> array;
		for (size_t i = 0; i < size; ++i) {
			if (m_pos >= m_data.size()) {
				g_logger().error("Read failed: unexpected end of data");
				return false;
			}

			if (m_data[m_pos] == static_cast<std::byte>(OTB::Node::ESCAPE)) {
				++m_pos;
				if (m_pos >= m_data.size()) {
					g_logger().error("Read failed: escape character at end of buffer");
					return false;
				}
			}

			array[i] = m_data[m_pos];
			++m_pos;
		}
		ret = std::bit_cast<T>(array);
	} else {
		if constexpr (size == 2) {
			uint16_t value;
			std::memcpy(&value, &m_data[m_pos], sizeof(uint16_t));
			ret = value;
		} else if constexpr (size == 4) {
			uint32_t value;
			std::memcpy(&value, &m_data[m_pos], sizeof(uint32_t));
			ret = value;
		} else if constexpr (size == 8) {
			uint64_t value;
			std::memcpy(&value, &m_data[m_pos], sizeof(uint64_t));
			ret = value;
		} else if constexpr (size <= 16) {
			std::memcpy(&ret, &m_data[m_pos], size);
		} else {
			std::array<std::byte, size> array;
			std::memcpy(array.data(), &m_data[m_pos], size);
			ret = std::bit_cast<T>(array);
		}
		m_pos += size;
	}
	return true;
}
