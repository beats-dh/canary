/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "io/fileloader.hpp"

namespace OTB {
	Loader::Loader(const std::string_view fileName, const Identifier &acceptedIdentifier) {
		try {
			fileContents = mio::mmap_source(std::string(fileName), 0, /* offset */
			                                mio::map_entire_file);

			constexpr auto minimalSize = sizeof(Identifier) + sizeof(Node::START) + sizeof(Node::type) + sizeof(Node::END);
			if (fileContents.size() <= minimalSize) {
				throw InvalidOTBFormat {};
			}

			Identifier fileIdentifier;
			std::memcpy(fileIdentifier.data(), fileContents.data(), fileIdentifier.size());

			if (std::memcmp(fileIdentifier.data(), acceptedIdentifier.data(), fileIdentifier.size()) != 0 && std::memcmp(fileIdentifier.data(), wildcard.data(), fileIdentifier.size()) != 0) {
				throw InvalidOTBFormat {};
			}
		} catch (const std::exception &e) {
			fileContents = mio::mmap_source {};
			throw;
		}
	}

	Node &Loader::getCurrentNode(const PreallocatedNodeStack &nodeStack) {
		if (nodeStack.empty() || nodeStack.top() == nullptr) {
			throw InvalidOTBFormat {};
		}
		return *nodeStack.top();
	}

	const Node &Loader::parseTree() {
		if (!isLoaded()) {
			throw InvalidOTBFormat {};
		}

		auto it = fileContents.data() + sizeof(Identifier);
		const auto end = fileContents.data() + fileContents.size();

		if (static_cast<uint8_t>(*it) != Node::START) {
			throw InvalidOTBFormat {};
		}

		root.type = *(++it);
		root.propsBegin = ++it;

		PreallocatedNodeStack parseStack;
		parseStack.push(&root);

		while (it < end) {
			const auto byte = static_cast<uint8_t>(*it);

			if (byte == Node::START) {
				auto &currentNode = getCurrentNode(parseStack);
				if (currentNode.children.empty()) {
					currentNode.propsEnd = it;
				}

				currentNode.children.emplace_back();
				auto &child = currentNode.children.back();

				if (++it >= end) {
					throw InvalidOTBFormat {};
				}

				child.type = *it;
				child.propsBegin = it + sizeof(Node::type);
				parseStack.push(&child);
			} else if (byte == Node::END) {
				auto &currentNode = getCurrentNode(parseStack);
				if (currentNode.children.empty()) {
					currentNode.propsEnd = it;
				}
				parseStack.pop();
			} else if (byte == Node::ESCAPE) {
				if (++it >= end) {
					throw InvalidOTBFormat {};
				}
			}

			++it;
		}

		if (!parseStack.empty()) {
			throw InvalidOTBFormat {};
		}

		return root;
	}

	bool Loader::getProps(const Node &node, PropStream &props) const {
		const auto begin = node.propsBegin;
		const auto end = node.propsEnd;

		const auto size = std::distance(begin, end);
		if (size <= 0) {
			return false;
		}

		propBuffer.resize(size);

		size_t outputIdx = 0;
		bool lastEscaped = false;

		for (auto it = begin; it != end; ++it) {
			const char c = *it;
			if (c == static_cast<char>(Node::ESCAPE) && !lastEscaped) {
				lastEscaped = true;
			} else {
				propBuffer[outputIdx++] = c;
				lastEscaped = false;
			}
		}

		props.init(propBuffer.data(), outputIdx);
		return true;
	}
} // namespace OTB
