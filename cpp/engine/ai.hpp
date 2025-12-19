#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "game.hpp"

namespace pyrisk {

using AttackStrategy = std::function<bool(int, int)>;
using MoveStrategy = std::function<int(int)>;

struct AttackPlan {
    Territory* src;
    Territory* dst;
    AttackStrategy attack_strategy;
    MoveStrategy move_strategy;
};

struct MoveOrder {
    Territory* src;
    Territory* dst;
    int count;
};

class AI {
public:
    AI(Player& player, Game& game);
    virtual ~AI() = default;

    virtual void start() {}
    virtual void end() {}
    virtual void on_event(const Event& /*event*/) {}

    virtual Territory* initial_placement(const std::vector<Territory*>& empty, int remaining) = 0;
    virtual std::unordered_map<Territory*, int> reinforce(int available) = 0;
    virtual std::vector<AttackPlan> attack() = 0;
    virtual std::optional<MoveOrder> freemove() { return std::nullopt; }

protected:
    std::vector<Territory*> owned_territories() const;
    std::vector<Territory*> owned_territories(const Player& player) const;

    Player& player_;
    Game& game_;
    World& world_;
    PythonicRNG& rng_;
};

class StupidAI : public AI {
public:
    using AI::AI;

    Territory* initial_placement(const std::vector<Territory*>& empty, int remaining) override;
    std::unordered_map<Territory*, int> reinforce(int available) override;
    std::vector<AttackPlan> attack() override;
};

class GameDriver {
public:
    using AiFactory = std::function<std::unique_ptr<AI>(Player&, Game&)>;

    GameDriver(World world, std::vector<std::string> player_names,
               std::vector<AiFactory> ai_factories, bool deal = false,
               EventLogger logger = {}, std::optional<std::uint32_t> seed = std::nullopt);

    Game& game();
    const Game& game() const;

    std::string play();

private:
    Player& current_player();
    AI& current_ai();
    void setup_turn_order();
    void initial_placement();
    void initial_deal(std::vector<Territory*>& empty,
                      std::unordered_map<std::string, int>& remaining);
    void finish_initial_reinforcements(std::unordered_map<std::string, int>& remaining);
    void handle_reinforcements(Player& player, AI& ai);
    void handle_attacks(Player& player, AI& ai);
    void handle_freemove(Player& player, AI& ai);
    bool player_alive(const Player& player) const;
    int alive_players() const;
    std::vector<Territory*> owned_territories(const Player& player) const;

    Game game_;
    std::vector<std::unique_ptr<AI>> ais_;
    std::vector<std::size_t> turn_order_;
    std::size_t turn_{0};
    bool deal_{false};
};

}  // namespace pyrisk
