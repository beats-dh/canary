/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include <memory>

class LuaObject;
using LuaObjectPtr = std::shared_ptr<LuaObject>;

class LuaObject : public std::enable_shared_from_this<LuaObject> {
	public:
		LuaObjectPtr asLuaObject() {
			return shared_from_this();
		}

		template <typename T>
		std::shared_ptr<T> static_self_cast() {
			return std::static_pointer_cast<T>(shared_from_this());
		}

		template <typename T>
		std::shared_ptr<T> dynamic_self_cast() {
			return std::dynamic_pointer_cast<T>(shared_from_this());
		}

		void operator=(const LuaObject &) const { }
};

using LuaObjectPtr = std::shared_ptr<LuaObject>;
