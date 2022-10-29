#include <catch2/catch.hpp>
#include "server/game/field/field.h"
#include "share/constants/codes.h"
#include "share/defines.h"
#include "share/objects/units.h"
#include "server/game/player/player.h"
#include "share/tools/random/random.h"
#include "testing_utils.h"
#include "share/tools/utils/utils.h"

std::pair<std::shared_ptr<Player>, std::shared_ptr<Field>> SetUpPlayer(bool resources) {
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  std::shared_ptr<Field> field = std::make_shared<Field>(ran_gen->RandomInt(50, 150), 
      ran_gen->RandomInt(50, 150), ran_gen);
  field->BuildGraph();
  auto nucleus_positions = field->AddNucleus(2);
  std::shared_ptr<Player> player_one_ = std::make_shared<Player>("p1", field, ran_gen, 0);
  player_one_->SetupNucleusAndResources(nucleus_positions[0]);
  std::shared_ptr<Player> player_two_ = std::make_shared<Player>("p2", field, ran_gen, 1);
  player_two_->SetupNucleusAndResources(nucleus_positions[1]);
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
  return {player_one_, field};
}

void CreateRandomNeurons(std::shared_ptr<Player> player, std::shared_ptr<Field> field, int num, 
    std::shared_ptr<RandomGenerator> ran_gen) {
  for (int i=0; i<num; i++) {
    auto pos = t_utils::GetRandomPositionInField(field, ran_gen);
    player->AddNeuron(pos, ran_gen->RandomInt(0, 1));
  }
}

TEST_CASE("test_ipsp_takes_epsp_potential", "[test_player]") {
  std::shared_ptr<RandomGenerator> ran_gen = std::make_shared<RandomGenerator>();
  auto player_and_field = SetUpPlayer(true); // set up player with lots of resources.
  auto player = player_and_field.first;
  auto field = player_and_field.second;

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
    player->AddVoltageToNeuron(player->GetOneNucleus(), 1);
    REQUIRE(player->GetNucleusLive() == "1 / 9");
  }

  SECTION("test HasLost") {
    REQUIRE(player->HasLost() == false);  // player has not already lost at the beginnig of the game.
    player->AddVoltageToNeuron(player->GetOneNucleus(), 8);
    REQUIRE(player->HasLost() == false);  // player still alive with 8/9 voltage.
    player->AddVoltageToNeuron(player->GetOneNucleus(), 1);
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
    player->AddNeuron(pos, SYNAPSE, t_utils::GetRandomPositionInField(field, ran_gen));
    
    REQUIRE(player->AddWayPosForSynapse(pos, {0, 0}, true) == -1);  // technology missing returns -1
    player->AddTechnology(WAY);
    REQUIRE(player->AddWayPosForSynapse({-1, -1}, {0, 0}) == -1);  // unkown synapse returns -1
    REQUIRE(player->AddWayPosForSynapse(pos, {-1, -1}) == -1);  // target outside field returns -1
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 1);  // Adding first way-point returns 1
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 2);  // Adding second way-point returns 2
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}) == 3);  // Adding third way-point returns 3
    REQUIRE(player->AddWayPosForSynapse(pos, {pos.first+1, pos.second+1}, true) == 1);  // Reseting returns 1
  }
}
