/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

class PropStream;

namespace OTB {
	using Identifier = std::array<char, 4>;

	inline constexpr Identifier wildcard = { { '\0', '\0', '\0', '\0' } };

	struct Node {
		enum NodeChar : uint8_t {
			ESCAPE = 0xFD,
			START = 0xFE,
			END = 0xFF,
		};

		Node() = default;
		Node(Node &&) noexcept = default;
		Node &operator=(Node &&) noexcept = default;

		Node(const Node &) = delete;
		Node &operator=(const Node &) = delete;

#if __cpp_lib_memory_resource >= 201603L
		std::pmr::list<Node> children;
#else
		std::list<Node> children;
#endif

		mio::mmap_source::const_iterator propsBegin {};
		mio::mmap_source::const_iterator propsEnd {};
		uint8_t type {};
	};

	enum class LoadErrorCode {
		Success,
		InvalidOTBFormat,
		FileNotFound,
		MemoryMapError
	};

	struct LoadError : std::exception {
		[[nodiscard]] const char* what() const noexcept override = 0;
	};

	struct InvalidOTBFormat final : LoadError {
		[[nodiscard]] const char* what() const noexcept override {
			return "Invalid OTBM file format";
		}
	};

	class Loader {
	public:
		explicit Loader(std::string_view fileName, const Identifier &acceptedIdentifier);

		[[nodiscard]] bool getProps(const Node &node, PropStream &props) const;
		[[nodiscard]] const Node &parseTree();

		[[nodiscard]] bool isLoaded() const noexcept {
			return !fileContents.empty();
		}

	private:
		mio::mmap_source fileContents;
		Node root;
		mutable std::vector<char> propBuffer;

		struct PreallocatedNodeStack {
			std::vector<Node*> container;

			PreallocatedNodeStack() {
				container.reserve(32);
			}

			void push(Node* node) noexcept {
				container.push_back(node);
			}

			void pop() noexcept {
				if (!container.empty()) {
					container.pop_back();
				}
			}

			[[nodiscard]] Node* top() const noexcept {
				return container.empty() ? nullptr : container.back();
			}

			[[nodiscard]] bool empty() const noexcept {
				return container.empty();
			}
		};

		[[nodiscard]] static Node &getCurrentNode(const PreallocatedNodeStack &nodeStack);
	};
} // namespace OTB

class PropStream {
public:
	void init(const std::span<const char> &data) noexcept {
		p = data.data();
		end = p + data.size();
	}

	void init(const char* a, size_t size) noexcept {
		p = a;
		end = a + size;
	}

	[[nodiscard]] constexpr size_t size() const noexcept {
		return end - p;
	}

	[[nodiscard]] constexpr bool empty() const noexcept {
		return p >= end;
	}

	[[nodiscard]] constexpr const char* position() const noexcept {
		return p;
	}

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	[[nodiscard]] bool read(T &ret) noexcept {
		if (size() < sizeof(T)) {
			return false;
		}

		if constexpr (sizeof(T) <= 8 && (sizeof(T) & (sizeof(T) - 1)) == 0) {
			ret = *reinterpret_cast<const T*>(p);
		} else if constexpr (sizeof(T) <= 16) {
			ret = std::bit_cast<T>(*reinterpret_cast<const std::array<std::byte, sizeof(T)>*>(p));
		} else {
			std::memcpy(&ret, p, sizeof(T));
		}

		p += sizeof(T);
		return true;
	}

	[[nodiscard]] bool readString(std::string &ret) {
		uint16_t strLen;
		if (!read<uint16_t>(strLen)) {
			return false;
		}

		if (size() < strLen) {
			return false;
		}

		if (strLen > 0) {
			ret.resize(strLen);
			std::memcpy(ret.data(), p, strLen);
		} else {
			ret.clear();
		}

		p += strLen;
		return true;
	}

	[[nodiscard]] bool skip(const size_t n) noexcept {
		if (size() < n) {
			return false;
		}

		p += n;
		return true;
	}

private:
	const char* p = nullptr;
	const char* end = nullptr;
};

// Stream de escrita otimizada
class PropWriteStream {
public:
	PropWriteStream() = default;

	PropWriteStream(const PropWriteStream &) = delete;
	PropWriteStream &operator=(const PropWriteStream &) = delete;

	PropWriteStream(PropWriteStream &&) noexcept = default;
	PropWriteStream &operator=(PropWriteStream &&) noexcept = default;

	[[nodiscard]] std::span<const char> getStream() const noexcept {
		return { buffer.data(), buffer.size() };
	}

	[[nodiscard]] const char* getStream(size_t &size) const noexcept {
		size = buffer.size();
		return buffer.data();
	}

	void clear() noexcept {
		buffer.clear();
	}

	void reserve(const size_t capacity) {
		buffer.reserve(capacity);
	}

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	void write(T add) {
		const size_t oldSize = buffer.size();
		buffer.resize(oldSize + sizeof(T));
		std::memcpy(buffer.data() + oldSize, &add, sizeof(T));
	}

	void writeString(const std::string_view str) {
		const size_t strLength = str.size();
		if (strLength > std::numeric_limits<uint16_t>::max()) {
			write<uint16_t>(0);
			return;
		}

		write(static_cast<uint16_t>(strLength));

		if (strLength > 0) {
			const size_t oldSize = buffer.size();
			buffer.resize(oldSize + strLength);
			std::memcpy(buffer.data() + oldSize, str.data(), strLength);
		}
	}

	void writeRaw(const std::span<const char> data) {
		if (data.empty()) {
			return;
		}

		const size_t oldSize = buffer.size();
		buffer.resize(oldSize + data.size());
		std::memcpy(buffer.data() + oldSize, data.data(), data.size());
	}

private:
	std::vector<char> buffer;
};
