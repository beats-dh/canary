config = {
        quests = {
            [7172] = { -- ActionID que será colocado no baú
                name = "dos Crystal Coins", -- Nome da quest
                rewards = {
                    {id = 2160, count = 100}, -- Prêmio: ID - Count
                },
                level = {
                    active = true, -- Level minimo para pegar?
                    min = 1, -- Se true, qual o minimo
                },
                premium = {
                    active = true, -- Precisa ser premium para fazer a quest
                },
                storage = {
                    active = true, -- Player poderá pegar somente uma vez?
                    key = 91143, -- Apenas uma key por quest
                },
                effectWin = 30, -- Efeito que vai aparecer quando fizer a quest
            },
            [1025] = { -- ActionID que será colocado no baú
                name = "dos Elven Armor", -- Nome da quest
                rewards = {
                    {id = 2505, count = 1}, -- Prêmio: ID - Count
                },
                level = {
                    active = true, -- Level minimo para pegar?
                    min = 50, -- Se true, qual o minimo
                },
                premium = {
                    active = false, -- Precisa ser premium para fazer a quest
                },
                storage = {
                    active = true, -- Player poderá pegar somente uma vez?
                    key = 90002, -- Apenas uma key por quest
                },
                effectWin = 30, -- Efeito que vai aparecer quando fizer a quest
            },
        },
    messages = {
        notExist = "Essa quest nao existe.",
        win = "Voce fez a quest %s.",
        notWin = "Voce ja fez a quest %s.",
        level = "Voce precisa ser level %d ou maior para fazer a quest %s.",
        premium = "Voce precisa ser premiun account pra fazer essa quest"
    },
}

local gerenciadorquest = Action()
function gerenciadorquest.onUse(player, item, fromPosition, target, toPosition, isHotkey)
    local player = Player(player)
    local choose = config.quests[item.actionid]

    if not choose then
        player:sendCancelMessage(config.messages.notExist)
        player:getPosition():sendMagicEffect(CONST_ME_POFF)
        return true
    end

    if choose.level.active and player:getLevel() < choose.level.min then
        player:sendCancelMessage(config.messages.level:format(choose.level.min, choose.name))
        player:getPosition():sendMagicEffect(CONST_ME_POFF)
        return true
    end

    if choose.premium.active and player:getPremiumDays() <= 0 then
	player:sendCancelMessage(config.messages.premium:format(choose.premium.active, choose.name))
	player:getPosition():sendMagicEffect(CONST_ME_POFF)
	return true
    end

    if choose.storage.active and player:getStorageValue(choose.storage.key) >= 1 then
        player:sendCancelMessage(config.messages.notWin:format(choose.name))
        player:getPosition():sendMagicEffect(CONST_ME_POFF)
        return true
    end

    for i = 1, #choose.rewards do
        player:addItem(choose.rewards[i].id, choose.rewards[i].count)
    end

    player:setStorageValue(choose.storage.key, 1)
    player:sendCancelMessage(config.messages.win:format(choose.name))
    player:getPosition():sendMagicEffect(choose.effectWin)

    return true
end

gerenciadorquest:aid(1025,7171,7172,7173,7174)
gerenciadorquest:register()
