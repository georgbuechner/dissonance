#include <catch2/catch.hpp>
#include <iterator>
#include "server/game/field/field.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/game/player/audio_ki.h"
#include "server/game/player/player.h"
#include "share/tools/random/random.h"
#include "spdlog/spdlog.h"
#include "testing_utils.h"
#include "share/tools/utils/utils.h"

std::pair<Player*, Field*> SetUpPlayer(bool resources) {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(ran_gen->RandomInt(50, 150), ran_gen->RandomInt(50, 150), ran_gen);
  spdlog::get(LOGGER)->info("SetUpPlayer with field cols: {} and lines: {}", field->cols(), field->lines());
  field->BuildGraph();
  auto nucleus_positions = field->AddNucleus(2);
  Player* player_one_ = new Player(nucleus_positions[0], field, ran_gen, 0);
  Player* player_two_ = new Player(nucleus_positions[1], field, ran_gen, 1);
  player_one_->set_enemies({player_two_});
  player_two_->set_enemies({player_one_});

  if (resources) {
    // Increase resources 20 times for iron gain.
    for (int i=0; i<100; i++)
      player_one_->IncreaseResources(true);
    // Activated each resource
    for (int i=Resources::IRON+1; i<=Resources::SEROTONIN; i++)
      for (int counter=0; counter<2; counter++) {
        player_one_->DistributeIron(i);
        player_one_->IncreaseResources(true);
      }
    // Increase resources 100 times for resource gain.
    for (int i=0; i<100; i++)
      player_one_->IncreaseResources(true);
  }
  for (const auto& it : player_one_->resources()) {
    spdlog::get(LOGGER)->debug("{}. Active: {}", resources_name_mapping.at(it.first), it.second.Active());
  }
  return {player_one_, field};
}

void CreateRandomNeurons(Player* player, Field* field, int num, RandomGenerator* ran_gen) {
  spdlog::get(LOGGER)->info("CreateRandomNeurons {}", num);
  for (int i=0; i<num; i++) {
    auto pos = t_utils::GetRandomPositionInField(field, ran_gen);
    spdlog::get(LOGGER)->debug("Creating neuron with pos: {}", utils::PositionToString(pos));
    player->AddNeuron(pos, ran_gen->RandomInt(0, 1));
  }
}

TEST_CASE("test_ipsp_takes_epsp_potential", "[test_player]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  auto player_and_field = SetUpPlayer(true); // set up player with lots of resources.
  Player* player = player_and_field.first;
  Field* field = player_and_field.second;

  /*
    TODO (fux): this test should be added. However, it is currently hard to create enemy neurons 
    and potentials at specific positions.
  SECTION("test ipsp-swallow") {
    Player* enemy = player->enemies().front();
    auto nucleus_pos = player->GetOneNucleus();
    auto synape_pos = field->FindFree(nucleus_pos, 2, 3); // random pos in range of nucleus
    auto enemy_pos = field->FindFree(synape_pos, 2, 3);  // close free position
    auto way = field->GetWay(synape_pos, {enemy_pos}); // get way from synapse to enemy_pos
    std::cout << "Synapse pos: " << utils::PositionToString(synape_pos) << std::endl;
    std::cout << "Enemy pos: " << utils::PositionToString(enemy_pos) << std::endl;
    std::cout << "WAY: " << std::endl;
    for (const auto& it : way)
      std::cout << "- " << utils::PositionToString(it) << std::endl;
    REQUIRE(way.size() >= 3);

    // Get position of epsp on the way between synape_pos and enemy_pos.
    auto it = way.begin();
    std::advance(it, 1);
    auto epsp_pos = *it; 
    std::cout << "epsp_pos: " << utils::PositionToString(epsp_pos) << std::endl;

    // Create epsps at this position (set epsps of field is enough since they are check in HandleIpsp)
    field->set_epsps({{{epsp_pos}, {{"epsp1", enemy}, {"epsp2", enemy}}}});

    // Create synape at synape_pos with ipsp-target=enemy_pos;
    player->AddNeuron(synape_pos, SYNAPSE, enemy_pos, enemy_pos);
    player->AddPotential(synape_pos, IPSP);
    player->AddPotential(synape_pos, IPSP);

    REQUIRE(player->statistics()->epsp_swallowed() == 0);
    // Move 8 times (as ipsp-speed = -8)
    for (unsigned int i=0; i<8; i++) 
      player->MovePotential();
    for (unsigned int i=0; i<8; i++) 
      player->MovePotential();

    // Ipsp should have swallowed the added potential
    REQUIRE(player->statistics()->epsp_swallowed() == 1);
  }
  */

  SECTION("test GetPositionOfClosestNeuron") {
    CreateRandomNeurons(player, field, 10, ran_gen);
    position_t start_pos = t_utils::GetRandomPositionInField(field, ran_gen);
    auto pos = player->GetPositionOfClosestNeuron(start_pos, ACTIVATEDNEURON);
    
    // Check that all other neurons are further apart that returned neuron position.
    int dist = utils::Dist(start_pos, pos);
    for (const auto& it : player->GetAllPositionsOfNeurons(ACTIVATEDNEURON))
      if (it != pos)
        REQUIRE(utils::Dist(it, start_pos) >= dist);
  }

  SECTION("test GetNucleusLive") {
    REQUIRE(player->GetNucleusLive() == "0 / 9");
    player->AddPotentialToNeuron(player->GetOneNucleus(), 1);
    REQUIRE(player->GetNucleusLive() == "1 / 9");
  }

  SECTION("test HasLost") {
    REQUIRE(player->HasLost() == false);  // player has not already lost at the beginnig of the game.
    player->AddPotentialToNeuron(player->GetOneNucleus(), 8);
    REQUIRE(player->HasLost() == false);  // player still alive with 8/9 voltage.
    player->AddPotentialToNeuron(player->GetOneNucleus(), 1);
    REQUIRE(player->HasLost() == true);  // player has lost.
  }

  SECTION("test GetNeuronTypeAtPosition") {
    // Creating 100 ranom neurons should definitly create at least one activated neuron and one synape.
    CreateRandomNeurons(player, field, 100, ran_gen); 
    
    // Check for activated neurons
    auto all_activated_neurons = player->GetAllPositionsOfNeurons(ACTIVATEDNEURON);
    for (const auto& it : all_activated_neurons)
      REQUIRE(player->GetNeuronTypeAtPosition(it) == ACTIVATEDNEURON);
    
    // Check for synapses
    auto all_synapse_position = player->GetAllPositionsOfNeurons(SYNAPSE);
    for (const auto& it : all_synapse_position)
      REQUIRE(player->GetNeuronTypeAtPosition(it) == SYNAPSE);

    auto all_nucleus = player->GetAllPositionsOfNeurons(NUCLEUS).size();
    auto all_resource_neuerons = player->GetAllPositionsOfNeurons(RESOURCENEURON).size();

    // All synapses and all activated neurons +6 (nucleus & resource-neurons) should equal the number of all neurons.
    REQUIRE(all_activated_neurons.size() + all_synapse_position.size() + all_nucleus + all_resource_neuerons
        == player->GetAllPositionsOfNeurons().size());
    REQUIRE(all_nucleus == 1);
    REQUIRE(all_resource_neuerons == 6);
  }
  
  SECTION ("test ResetWayForSynapse") {
    auto pos = t_utils::GetRandomPositionInField(field, ran_gen);
    player->AddNeuron(pos, SYNAPSE);
    
    // Check invalid
    REQUIRE(player->AddWayPosForSynapse({-1, -1}, {0, 0}) == -1);  // invalids returns -1
    REQUIRE(player->AddWayPosForSynapse({-1, -1}, {0, 0}, true) == -1);  // invalids returns -1

    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 1);  // Adding first way-point returns 1
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 2);  // Adding second way-point returns 1
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 3);  // Adding third way-point returns 3
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}, true) == 1);  // Reseting returns 1
  }
}
