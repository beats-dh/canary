/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "map/spectators.hpp"

#include "creatures/creature.hpp"
#include "game/game.hpp"

// Pré-alocação para o cache de espectadores para evitar rehashing
phmap::flat_hash_map<Position, SpectatorsCache> Spectators::spectatorsCache { 256 };

void Spectators::clearCache() {
	spectatorsCache.clear();
}

Spectators Spectators::insert(const std::shared_ptr<Creature> &creature) {
	if (creature) {
		creatures.emplace_back(creature);
	}
	return *this;
}

Spectators Spectators::insertAll(const CreatureVector &list) {
	if (list.empty()) {
		return *this;
	}

	// Caso especial para lista vazia - apenas copiar
	if (creatures.empty()) {
		creatures = list;
		return *this;
	}

	// Pré-alocar para evitar múltiplas realocações
	creatures.reserve(creatures.size() + list.size());

	// Adicionar novos elementos
	creatures.insert(creatures.end(), list.begin(), list.end());

	// Remover duplicatas usando abordagem baseada no tamanho da lista
	if (creatures.size() > 100) {
		// Para listas grandes, usar flat_hash_set é mais eficiente
		phmap::flat_hash_set<std::shared_ptr<Creature>> uniqueSet;
		uniqueSet.reserve(creatures.size());

		CreatureVector uniqueCreatures;
		uniqueCreatures.reserve(creatures.size());

		for (const auto &creature : creatures) {
			if (uniqueSet.insert(creature).second) {
				uniqueCreatures.emplace_back(creature);
			}
		}

		creatures = std::move(uniqueCreatures);
	} else {
		// Para listas pequenas, sort+unique é mais eficiente
		std::ranges::sort(creatures);
		const auto last = std::ranges::unique(creatures).begin();
		creatures.erase(last, creatures.end());
	}

	return *this;
}

bool Spectators::checkCache(const SpectatorsCache::FloorData &specData, bool onlyPlayers, bool onlyMonsters, bool onlyNpcs, const Position &centerPos, bool checkDistance, bool multifloor, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	const auto &list = multifloor || !specData.floor ? specData.multiFloor : specData.floor;

	if (!list || list->empty()) {
		return false;
	}

	if (!multifloor && !specData.floor) {
		// Force check the distance of creatures as we only need to pick up creatures from the Floor(centerPos.z)
		checkDistance = true;
	}

	if (checkDistance) {
		// Pré-alocar para evitar realocações durante o loop
		const size_t estimatedSize = list->size() / 2; // Estimativa conservadora
		if (creatures.capacity() < estimatedSize) {
			creatures.reserve(estimatedSize);
		}

		// Verificações rápidas para filtrar criaturas
		for (const auto &creature : *list) {
			const auto &specPos = creature->getPosition();

			// Otimização de branch prediction - verificar a condição mais frequente primeiro
			if (!multifloor && specPos.z != centerPos.z) {
				continue;
			}

			// Verificar se está dentro dos limites XY
			const int dx = centerPos.x - specPos.x;
			const int dy = centerPos.y - specPos.y;

			if (dx < minRangeX || dx > maxRangeX || dy < minRangeY || dy > maxRangeY) {
				continue;
			}

			// Filtrar por tipo de criatura
			if (onlyPlayers) {
				if (!creature->getPlayer()) {
					continue;
				}
			} else if (onlyMonsters) {
				if (!creature->getMonster()) {
					continue;
				}
			} else if (onlyNpcs) {
				if (!creature->getNpc()) {
					continue;
				}
			}

			// Passou por todas as verificações, adicionar à lista
			creatures.emplace_back(creature);
		}
	} else {
		// Sem verificação de distância, adicionar tudo
		insertAll(*list);
	}

	return true;
}

CreatureVector Spectators::getSpectators(const Position &centerPos, bool multifloor, bool onlyPlayers, bool onlyMonsters, bool onlyNpcs, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	// Cálculo otimizado para limites Z
	uint8_t minRangeZ = centerPos.z;
	uint8_t maxRangeZ = centerPos.z;

	if (multifloor) {
		if (centerPos.z > MAP_INIT_SURFACE_LAYER) {
			minRangeZ = static_cast<uint8_t>(std::max<int8_t>(centerPos.z - MAP_LAYER_VIEW_LIMIT, 0u));
			maxRangeZ = static_cast<uint8_t>(std::min<int8_t>(centerPos.z + MAP_LAYER_VIEW_LIMIT, MAP_MAX_LAYERS - 1));
		} else if (centerPos.z == MAP_INIT_SURFACE_LAYER - 1) {
			minRangeZ = 0;
			maxRangeZ = (MAP_INIT_SURFACE_LAYER - 1) + MAP_LAYER_VIEW_LIMIT;
		} else if (centerPos.z == MAP_INIT_SURFACE_LAYER) {
			minRangeZ = 0;
			maxRangeZ = MAP_INIT_SURFACE_LAYER + MAP_LAYER_VIEW_LIMIT;
		} else {
			minRangeZ = 0;
			maxRangeZ = MAP_INIT_SURFACE_LAYER;
		}
	}

	// Pré-calcular limites uma única vez
	const int32_t min_y = centerPos.y + minRangeY;
	const int32_t min_x = centerPos.x + minRangeX;
	const int32_t max_y = centerPos.y + maxRangeY;
	const int32_t max_x = centerPos.x + maxRangeX;

	const auto width = static_cast<uint32_t>(max_x - min_x);
	const auto height = static_cast<uint32_t>(max_y - min_y);
	const auto depth = static_cast<uint32_t>(maxRangeZ - minRangeZ);

	const int32_t minoffset = centerPos.getZ() - maxRangeZ;
	const int32_t x1 = std::min<int32_t>(0xFFFF, std::max<int32_t>(0, min_x + minoffset));
	const int32_t y1 = std::min<int32_t>(0xFFFF, std::max<int32_t>(0, min_y + minoffset));

	const int32_t maxoffset = centerPos.getZ() - minRangeZ;
	const int32_t x2 = std::min<int32_t>(0xFFFF, std::max<int32_t>(0, max_x + maxoffset));
	const int32_t y2 = std::min<int32_t>(0xFFFF, std::max<int32_t>(0, max_y + maxoffset));

	const int32_t startx1 = x1 - (x1 & SECTOR_MASK);
	const int32_t starty1 = y1 - (y1 & SECTOR_MASK);
	const int32_t endx2 = x2 - (x2 & SECTOR_MASK);
	const int32_t endy2 = y2 - (y2 & SECTOR_MASK);

	// Reserva de memória otimizada - usar tamanho mais realista
	CreatureVector spectators;
	spectators.reserve(64); // Valor típico para um setor com players

	// Cache local para setores já processados para evitar processamento duplicado
	phmap::flat_hash_set<const MapSector*> processedSectors;
	processedSectors.reserve(16); // Típico número de setores para um campo de visão normal

	// Busca em setores otimizada
	const MapSector* startSector = g_game().map.getMapSector(startx1, starty1);
	const MapSector* sectorS = startSector;

	for (int32_t ny = starty1; ny <= endy2; ny += SECTOR_SIZE) {
		const MapSector* sectorE = sectorS;
		for (int32_t nx = startx1; nx <= endx2; nx += SECTOR_SIZE) {
			if (sectorE) {
				// Evitar processamento duplicado (pode ocorrer em certos padrões de mapa)
				if (processedSectors.insert(sectorE).second) {
					// Selecionar a lista apropriada apenas uma vez
					const auto &nodeList = onlyPlayers ? sectorE->player_list : onlyMonsters ? sectorE->monster_list
						: onlyNpcs                                                           ? sectorE->npc_list
																							 : sectorE->creature_list;

					// Verificar se a lista não está vazia antes de processá-la
					if (!nodeList.empty()) {
						// Pré-alocar mais espaço se necessário
						if (spectators.size() + nodeList.size() > spectators.capacity()) {
							spectators.reserve(spectators.capacity() + nodeList.size());
						}

						for (const auto &creature : nodeList) {
							const auto &cpos = creature->getPosition();

							// Verificação rápida de profundidade primeiro (branch prediction otimizada)
							const uint32_t zDiff = static_cast<uint32_t>(static_cast<int32_t>(cpos.z) - minRangeZ);
							if (zDiff > depth) {
								continue;
							}

							const int_fast16_t offsetZ = Position::getOffsetZ(centerPos, cpos);

							// Calcular offsets uma única vez e verificar limites
							const uint32_t offsetX = cpos.x - offsetZ - min_x;
							const uint32_t offsetY = cpos.y - offsetZ - min_y;

							if (offsetX <= width && offsetY <= height) {
								spectators.emplace_back(creature);
							}
						}
					}
				}

				sectorE = sectorE->sectorE;
			} else {
				sectorE = g_game().map.getMapSector(nx + SECTOR_SIZE, ny);
			}
		}

		if (sectorS) {
			sectorS = sectorS->sectorS;
		} else {
			sectorS = g_game().map.getMapSector(startx1, ny + SECTOR_SIZE);
		}
	}

	return spectators;
}

Spectators Spectators::find(const Position &centerPos, bool multifloor, bool onlyPlayers, bool onlyMonsters, bool onlyNpcs, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY, bool useCache) {
	// Normalizar os parâmetros de range
	minRangeX = (minRangeX == 0 ? -MAP_MAX_VIEW_PORT_X : -minRangeX);
	maxRangeX = (maxRangeX == 0 ? MAP_MAX_VIEW_PORT_X : maxRangeX);
	minRangeY = (minRangeY == 0 ? -MAP_MAX_VIEW_PORT_Y : -minRangeY);
	maxRangeY = (maxRangeY == 0 ? MAP_MAX_VIEW_PORT_Y : maxRangeY);

	// Fast path - ignorar cache se solicitado
	if (!useCache) {
		insertAll(getSpectators(centerPos, multifloor, onlyPlayers, onlyMonsters, onlyNpcs, minRangeX, maxRangeX, minRangeY, maxRangeY));
		return *this;
	}

	// Verificar cache existente
	auto it = spectatorsCache.find(centerPos);
	const bool cacheFound = it != spectatorsCache.end();

	if (cacheFound) {
		auto &cache = it->second;

		// Verificar se o cache atual abrange os ranges solicitados
		if (minRangeX >= cache.minRangeX && maxRangeX <= cache.maxRangeX && minRangeY >= cache.minRangeY && maxRangeY <= cache.maxRangeY) {

			const bool checkDistance = minRangeX != cache.minRangeX || maxRangeX != cache.maxRangeX || minRangeY != cache.minRangeY || maxRangeY != cache.maxRangeY;

			// Tentativa 1: Verificar cache específico (players/monsters/npcs)
			if (onlyPlayers || onlyMonsters || onlyNpcs) {
				const auto &creaturesCache = onlyPlayers ? cache.players : onlyMonsters ? cache.monsters
					: onlyNpcs                                                          ? cache.npcs
																						: cache.creatures;

				if (checkCache(creaturesCache, onlyPlayers, onlyMonsters, onlyNpcs, centerPos, checkDistance, multifloor, minRangeX, maxRangeX, minRangeY, maxRangeY)) {
					return *this;
				}

				// Tentativa 2: Usar o cache de todas as criaturas com filtro
				if (checkCache(cache.creatures, onlyPlayers, onlyMonsters, onlyNpcs, centerPos, true, multifloor, minRangeX, maxRangeX, minRangeY, maxRangeY)) {
					return *this;
				}
			} else if (checkCache(cache.creatures, false, false, false, centerPos, checkDistance, multifloor, minRangeX, maxRangeX, minRangeY, maxRangeY)) {
				// Tentativa 3: Para todas as criaturas, usar o cache geral
				return *this;
			}
		} else {
			// O cache existe mas não cobre o range solicitado - expandir o cache
			cache.minRangeX = minRangeX = std::min<int32_t>(minRangeX, cache.minRangeX);
			cache.minRangeY = minRangeY = std::min<int32_t>(minRangeY, cache.minRangeY);
			cache.maxRangeX = maxRangeX = std::max<int32_t>(maxRangeX, cache.maxRangeX);
			cache.maxRangeY = maxRangeY = std::max<int32_t>(maxRangeY, cache.maxRangeY);
		}
	}

	// Cache miss ou cache inválido - computar novos espectadores
	const auto &spectators = getSpectators(centerPos, multifloor, onlyPlayers, onlyMonsters, onlyNpcs, minRangeX, maxRangeX, minRangeY, maxRangeY);

	// Criar ou atualizar cache
	auto &cache = cacheFound ? it->second : spectatorsCache.try_emplace(centerPos, SpectatorsCache { .minRangeX = minRangeX, .maxRangeX = maxRangeX, .minRangeY = minRangeY, .maxRangeY = maxRangeY }).first->second;

	// Atualizar cache específico (players/monsters/npcs ou geral)
	auto &creaturesCache = onlyPlayers ? cache.players : onlyMonsters ? cache.monsters
		: onlyNpcs                                                    ? cache.npcs
																	  : cache.creatures;

	// Atualizar lista floor ou multifloor
	auto &creatureList = (multifloor ? creaturesCache.multiFloor : creaturesCache.floor);

	if (!creatureList) {
		creatureList.emplace();
		creatureList->reserve(spectators.size() + 16); // Adicionar margem para crescimento
	} else {
		creatureList->clear();

		// Redimensionar se necessário para evitar realocações futuras
		if (creatureList->capacity() < spectators.size()) {
			creatureList->reserve(spectators.size() + 16);
		}
	}

	// Copiar espectadores para o cache
	if (!spectators.empty()) {
		creatureList->insert(creatureList->end(), spectators.begin(), spectators.end());

		// Atualizar nossa lista de espectadores
		insertAll(spectators);
	}

	return *this;
}

Spectators Spectators::excludeMaster() const {
	auto specs = Spectators();
	if (creatures.empty()) {
		return specs;
	}

	specs.creatures.reserve(creatures.size());

	for (const auto &c : creatures) {
		if (c->getMonster() != nullptr && !c->getMaster()) {
			specs.insert(c);
		}
	}

	return specs;
}

Spectators Spectators::excludePlayerMaster() const {
	auto specs = Spectators();
	if (creatures.empty()) {
		return specs;
	}

	specs.creatures.reserve(creatures.size());

	for (const auto &c : creatures) {
		if ((c->getMonster() != nullptr && !c->getMaster()) || (!c->getMaster() || !c->getMaster()->getPlayer())) {
			specs.insert(c);
		}
	}

	return specs;
}

Spectators Spectators::filter(bool onlyPlayers, bool onlyMonsters, bool onlyNpcs) const {
	auto specs = Spectators();
	if (creatures.empty()) {
		return specs;
	}

	// Estimativa de tamanho para pré-alocação
	specs.creatures.reserve(onlyPlayers || onlyMonsters || onlyNpcs ? creatures.size() / 2 : creatures.size());

	// Loop otimizado com branch prediction favorável
	for (const auto &c : creatures) {
		if (onlyPlayers) {
			if (c->getPlayer() != nullptr) {
				specs.insert(c);
			}
		} else if (onlyMonsters) {
			if (c->getMonster() != nullptr) {
				specs.insert(c);
			}
		} else if (onlyNpcs) {
			if (c->getNpc() != nullptr) {
				specs.insert(c);
			}
		} else {
			// Nenhum filtro - adicionar todas
			specs.insert(c);
		}
	}

	return specs;
}
