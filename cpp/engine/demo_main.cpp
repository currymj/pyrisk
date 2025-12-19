#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ai.hpp"
#include "world_data.hpp"

int main() {
    using namespace pyrisk;

    World world;
    world.load(kAreas, kConnectionData);

    std::vector<std::string> names = {"StupidAI_1", "StupidAI_2"};
    std::vector<GameDriver::AiFactory> factories;
    factories.push_back([](Player& p, Game& g) { return std::make_unique<StupidAI>(p, g); });
    factories.push_back([](Player& p, Game& g) { return std::make_unique<StupidAI>(p, g); });

    GameDriver driver(std::move(world), names, factories, /*deal=*/false, {},
                      /*seed=*/std::optional<std::uint32_t>(42));
    std::string winner = driver.play();

    std::cout << "Winner: " << winner << std::endl;
    return 0;
}
