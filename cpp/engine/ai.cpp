#include "ai.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <unordered_set>

namespace pyrisk {
namespace {

std::vector<Player> make_players(const std::vector<std::string>& names) {
    std::vector<Player> players;
    players.reserve(names.size());
    for (const auto& name : names) {
        players.emplace_back(name);
    }
    return players;
}

int remaining_total(const std::unordered_map<std::string, int>& remaining) {
    return std::accumulate(
        remaining.begin(), remaining.end(), 0,
        [](int acc, const auto& entry) { return acc + entry.second; });
}

}  // namespace

AI::AI(Player& player, Game& game)
    : player_(player), game_(game), world_(game.world), rng_(game.rng()) {}

std::vector<Territory*> AI::owned_territories() const { return owned_territories(player_); }

std::vector<Territory*> AI::owned_territories(const Player& player) const {
    std::vector<Territory*> owned;
    for (const auto& [_, territory] : world_.territories) {
        if (territory->owner == &player) {
            owned.push_back(territory.get());
        }
    }
    return owned;
}

Territory* StupidAI::initial_placement(const std::vector<Territory*>& empty, int /*remaining*/) {
    if (!empty.empty()) {
        int idx = rng_.randbelow(static_cast<int>(empty.size()));
        return empty[static_cast<std::size_t>(idx)];
    }

    std::vector<Territory*> owned = owned_territories(player_);
    if (owned.empty()) {
        return nullptr;
    }
    int idx = rng_.randbelow(static_cast<int>(owned.size()));
    return owned[static_cast<std::size_t>(idx)];
}

std::unordered_map<Territory*, int> StupidAI::reinforce(int available) {
    std::unordered_map<Territory*, int> allocations;
    std::vector<Territory*> borders;
    for (auto* territory : owned_territories(player_)) {
        if (territory->border()) {
            borders.push_back(territory);
        }
    }
    if (borders.empty()) {
        borders = owned_territories(player_);
    }
    if (borders.empty()) {
        return allocations;
    }

    for (int i = 0; i < available; ++i) {
        int idx = rng_.randbelow(static_cast<int>(borders.size()));
        allocations[borders[static_cast<std::size_t>(idx)]] += 1;
    }
    return allocations;
}

std::vector<AttackPlan> StupidAI::attack() {
    std::vector<AttackPlan> plans;
    for (auto* territory : owned_territories(player_)) {
        for (auto* adjacent : territory->connect) {
            if (adjacent->owner != &player_ && territory->forces > adjacent->forces) {
                plans.push_back({territory, adjacent, {}, {}});
            }
        }
    }
    return plans;
}

GameDriver::GameDriver(World world, std::vector<std::string> player_names,
                       std::vector<AiFactory> ai_factories, bool deal, EventLogger logger,
                       std::optional<std::uint32_t> seed)
    : game_(std::move(world), make_players(player_names), {}, seed), deal_(deal) {
    if (player_names.size() != ai_factories.size()) {
        throw std::invalid_argument("player count must match AI factory count");
    }

    ais_.reserve(ai_factories.size());
    for (std::size_t i = 0; i < ai_factories.size(); ++i) {
        ais_.push_back(ai_factories[i](game_.players[i], game_));
    }

    game_.set_logger([this, logger](const Event& event) {
        if (logger) {
            logger(event);
        }
        for (auto& ai : ais_) {
            ai->on_event(event);
        }
    });
}

Game& GameDriver::game() { return game_; }
const Game& GameDriver::game() const { return game_; }

Player& GameDriver::current_player() {
    return game_.players[turn_order_[turn_ % turn_order_.size()]];
}

AI& GameDriver::current_ai() { return *ais_[turn_order_[turn_ % turn_order_.size()]]; }

void GameDriver::setup_turn_order() {
    turn_order_.resize(game_.players.size());
    std::iota(turn_order_.begin(), turn_order_.end(), 0);
    game_.rng().shuffle(turn_order_.begin(), turn_order_.end());

    static const std::vector<char> ords = {'\\', '/', '-', '|', '+'};
    for (std::size_t i = 0; i < turn_order_.size(); ++i) {
        auto& player = game_.players[turn_order_[i]];
        player.color = static_cast<int>(i) + 1;
        player.ord = i < ords.size() ? ords[i] : '*';
    }
}

std::string GameDriver::play() {
    setup_turn_order();
    for (auto& ai : ais_) {
        ai->start();
    }

    initial_placement();

    while (alive_players() > 1) {
        if (player_alive(current_player())) {
            auto& player = current_player();
            auto& ai = current_ai();
            handle_reinforcements(player, ai);
            handle_attacks(player, ai);
            handle_freemove(player, ai);
        }
        ++turn_;
    }

    Player* winner = nullptr;
    for (auto& player : game_.players) {
        if (player_alive(player)) {
            winner = &player;
            break;
        }
    }

    if (winner) {
        game_.victory(winner->name);
    }
    for (auto& ai : ais_) {
        ai->end();
    }
    return winner ? winner->name : std::string();
}

void GameDriver::initial_deal(std::vector<Territory*>& empty,
                              std::unordered_map<std::string, int>& remaining) {
    game_.rng().shuffle(empty.begin(), empty.end());
    while (!empty.empty()) {
        auto* territory = empty.back();
        empty.pop_back();
        auto& player = current_player();
        game_.claim(player.name, territory->name, 1);
        remaining[player.name] -= 1;
        ++turn_;
    }
}

void GameDriver::finish_initial_reinforcements(std::unordered_map<std::string, int>& remaining) {
    while (remaining_total(remaining) > 0) {
        auto& player = current_player();
        if (remaining[player.name] > 0) {
            auto* choice = current_ai().initial_placement({}, remaining[player.name]);
            if (choice != nullptr && choice->owner == &player) {
                game_.reinforce(player.name, choice->name, 1);
                remaining[player.name] -= 1;
            }
        }
        ++turn_;
    }
}

void GameDriver::initial_placement() {
    std::vector<Territory*> empty;
    empty.reserve(game_.world.territories.size());
    for (auto& [_, territory] : game_.world.territories) {
        empty.push_back(territory.get());
    }

    int available = 35 - 2 * static_cast<int>(game_.players.size());
    std::unordered_map<std::string, int> remaining;
    for (auto& player : game_.players) {
        remaining[player.name] = available;
    }

    if (deal_) {
        initial_deal(empty, remaining);
    } else {
        while (!empty.empty()) {
            auto& player = current_player();
            auto& ai = current_ai();
            auto* choice = ai.initial_placement(empty, remaining[player.name]);
            if (choice != nullptr &&
                std::find(empty.begin(), empty.end(), choice) != empty.end()) {
                game_.claim(player.name, choice->name, 1);
                remaining[player.name] -= 1;
                empty.erase(std::remove(empty.begin(), empty.end(), choice), empty.end());
            }
            ++turn_;
        }
    }

    finish_initial_reinforcements(remaining);
}

void GameDriver::handle_reinforcements(Player& player, AI& ai) {
    int reinforcements = game_.reinforcement_count(player);
    auto allocations = ai.reinforce(reinforcements);
    int assigned = 0;
    for (const auto& [territory, count] : allocations) {
        if (territory != nullptr && territory->owner == &player && count > 0) {
            game_.reinforce(player.name, territory->name, count);
            assigned += count;
        }
    }

    if (assigned < reinforcements) {
        auto owned = owned_territories(player);
        if (!owned.empty()) {
            game_.reinforce(player.name, owned.front()->name, reinforcements - assigned);
        }
    }
}

void GameDriver::handle_attacks(Player& player, AI& ai) {
    for (const auto& plan : ai.attack()) {
        if (!plan.src || !plan.dst) {
            continue;
        }
        if (plan.src->owner != &player || plan.dst->owner == &player) {
            continue;
        }
        if (plan.src->connect.count(plan.dst) == 0) {
            continue;
        }
        game_.resolve_combat(plan.src->name, plan.dst->name, plan.attack_strategy,
                             plan.move_strategy);
    }
}

void GameDriver::handle_freemove(Player& player, AI& ai) {
    auto move_order = ai.freemove();
    if (!move_order.has_value()) {
        return;
    }
    const auto& [src, dst, count] = move_order.value();
    if (src != nullptr && dst != nullptr && src->owner == &player && dst->owner == &player &&
        count >= 0) {
        game_.move(player.name, src->name, dst->name, count);
    }
}

bool GameDriver::player_alive(const Player& player) const {
    return game_.territory_count(player) > 0;
}

int GameDriver::alive_players() const {
    return std::count_if(game_.players.begin(), game_.players.end(),
                         [&](const Player& player) { return player_alive(player); });
}

std::vector<Territory*> GameDriver::owned_territories(const Player& player) const {
    std::vector<Territory*> owned;
    for (const auto& [_, territory] : game_.world.territories) {
        if (territory->owner == &player) {
            owned.push_back(territory.get());
        }
    }
    return owned;
}

}  // namespace pyrisk
