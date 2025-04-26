/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

class Map;
struct Position;
class FileStream;

class IOMap {
public:
	/**
	 * Carrega o mapa a partir do arquivo OTBM
	 * @param map Ponteiro para o objeto Map
	 * @param pos Posição de deslocamento (opcional)
	 * @throws IOMapException em caso de erro
	 */
	static void loadMap(Map* map, const Position &pos = Position());

	/**
	 * Carrega monstros do mapa principal
	 * @param map Ponteiro para o objeto Map
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadMonsters(Map* map);

	/**
	 * Carrega zonas do mapa principal
	 * @param map Ponteiro para o objeto Map
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadZones(Map* map);

	/**
	 * Carrega NPCs do mapa principal
	 * @param map Ponteiro para o objeto Map
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadNpcs(Map* map);

	/**
	 * Carrega casas do mapa principal
	 * @param map Ponteiro para o objeto Map
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadHouses(Map* map);

	/**
	 * Carrega monstros de mapas customizados
	 * @param map Ponteiro para o objeto Map
	 * @param mapName Nome do mapa customizado
	 * @param customMapIndex Índice do mapa customizado
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadMonstersCustom(Map* map, std::string_view mapName, int customMapIndex);

	/**
	 * Carrega zonas de mapas customizados
	 * @param map Ponteiro para o objeto Map
	 * @param mapName Nome do mapa customizado
	 * @param customMapIndex Índice do mapa customizado
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadZonesCustom(const Map* map, std::string_view mapName, int customMapIndex);

	/**
	 * Carrega NPCs de mapas customizados
	 * @param map Ponteiro para o objeto Map
	 * @param mapName Nome do mapa customizado
	 * @param customMapIndex Índice do mapa customizado
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadNpcsCustom(Map* map, std::string_view mapName, int customMapIndex);

	/**
	 * Carrega casas de mapas customizados
	 * @param map Ponteiro para o objeto Map
	 * @param mapName Nome do mapa customizado
	 * @param customMapIndex Índice do mapa customizado
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadHousesCustom(Map* map, std::string_view mapName, int customMapIndex);

	/**
	 * Carrega todos os recursos (monstros, zonas, NPCs, casas) sequencialmente
	 * @param map Ponteiro para o objeto Map
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadAllResources(Map* map);

	/**
	 * Carrega todos os recursos customizados sequencialmente
	 * @param map Ponteiro para o objeto Map
	 * @param mapName Nome do mapa customizado
	 * @param customMapIndex Índice do mapa customizado
	 * @return std::expected com true se sucesso ou mensagem de erro
	 */
	static std::expected<bool, std::string> loadAllResourcesCustom(Map* map, std::string_view mapName, int customMapIndex);

private:
	/**
	 * Analisa atributos de dados do mapa
	 * @param stream Stream de dados do arquivo
	 * @param map Ponteiro para o objeto Map
	 */
	static void parseMapDataAttributes(FileStream &stream, Map* map);

	/**
	 * Analisa waypoints do mapa
	 * @param stream Stream de dados do arquivo
	 * @param map Referência para o objeto Map
	 */
	static void parseWaypoints(FileStream &stream, Map &map);

	/**
	 * Analisa cidades do mapa
	 * @param stream Stream de dados do arquivo
	 * @param map Referência para o objeto Map
	 */
	static void parseTowns(FileStream &stream, Map &map);

	/**
	 * Analisa área de tiles do mapa
	 * @param stream Stream de dados do arquivo
	 * @param map Referência para o objeto Map
	 * @param pos Posição de deslocamento
	 */
	static void parseTileArea(FileStream &stream, Map &map, const Position &pos);

	/**
	 * Obtém o caminho completo para um arquivo baseado no nome do mapa
	 * @param currentPath Caminho atual, se estiver vazio será gerado
	 * @param mapName Nome do mapa
	 * @param suffix Sufixo a ser adicionado
	 * @return Caminho completo para o arquivo
	 */
	static std::string getFullPath(const std::string &currentPath, std::string_view mapName, std::string_view suffix);
};

/**
 * Exceção específica para erros relacionados ao carregamento de mapas
 */
class IOMapException final : public std::exception {
public:
	explicit IOMapException(std::string msg) :
		message(std::move(msg)) { }

	[[nodiscard]] const char* what() const noexcept override {
		return message.c_str();
	}

private:
	std::string message;
};
