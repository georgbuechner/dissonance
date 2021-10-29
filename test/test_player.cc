#include <catch2/catch.hpp>
#include "game/field.h"
#include "objects/units.h"
#include "player/audio_ki.h"
#include "player/player.h"
#include "random/random.h"
#include "testing_utils.h"
#include "utils/utils.h"

std::pair<Player*, Field*> SetUpPlayer(bool resources) {
  RandomGenerator* ran_gen = new RandomGenerator();
  Field* field = new Field(ran_gen->RandomInt(50, 150), ran_gen->RandomInt(50, 150), ran_gen);
  position_t nucleus_pos_1 = field->AddNucleus(8);
  position_t nucleus_pos_2 = field->AddNucleus(1);
  field->BuildGraph(nucleus_pos_1, nucleus_pos_2);
  auto resource_positions_1 = field->AddResources(nucleus_pos_1);
  auto resource_positions_2 = field->AddResources(nucleus_pos_2);
  Player* player_one_ = new Player(nucleus_pos_1, field, ran_gen, resource_positions_1);
  Player* player_two_ = new Player(nucleus_pos_2, field, ran_gen, resource_positions_2);
  player_one_->set_enemy(player_two_);
  player_two_->set_enemy(player_one_);

  if (resources) {
    // Increase resources 20 times for iron gain.
    for (int i=0; i<100; i++)
      player_one_->IncreaseResources(true);
    // Activated each resource
    for (int i=Resources::IRON; i<Resources::SEROTONIN; i++)
      for (int counter=0; counter<3; counter++)
        player_one_->DistributeIron(i);
    // Increase resources 100 times for resource gain.
    for (int i=0; i<100; i++)
      player_one_->IncreaseResources(true);
  }
  return {player_one_, field};
}

void CreateRandomNeurons(Player* player, Field* field, int num, RandomGenerator* ran_gen) {
  for (int i=0; i<num; i++)
    player->AddNeuron(t_utils::GetRandomPositionInField(field, ran_gen), ran_gen->RandomInt(0, 1));
}

TEST_CASE("test_ipsp_takes_epsp_potential", "[test_player]") {
  RandomGenerator* ran_gen = new RandomGenerator();
  auto player_and_field = SetUpPlayer(true); // set up player with lots of resources.
  Player* player = player_and_field.first;
  Field* field = player_and_field.second;

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

    // All synapses and all activated neurons +6 (nucleus & resource-neurons) should equal the number of all neurons.
    REQUIRE(all_activated_neurons.size() + all_synapse_position.size() + 6 == player->GetAllPositionsOfNeurons().size());
  }
  
  SECTION ("test ResetWayForSynapse") {
    auto pos = t_utils::GetRandomPositionInField(field, ran_gen);
    player->AddNeuron(pos, SYNAPSE);
    
    // Check invalid
    REQUIRE(player->AddWayPosForSynapse({-1, -1}, {0, 0}) == -1);  // invalids returns -1
    REQUIRE(player->ResetWayForSynapse({-1, -1}, {0, 0}) == -1);  // invalids returns -1

    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 1);  // Adding first way-point returns 1
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 2);  // Adding second way-point returns 1
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 3);  // Adding third way-point returns 3
    REQUIRE(player->ResetWayForSynapse(pos, {pos.first+1, pos.second+1}) == 1);  // Reseting returns 1
  }
}

