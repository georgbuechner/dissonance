#include <catch2/catch.hpp>
#include "game/field.h"
#include "player/player.h"

TEST_CASE("test_ipsp_takes_epsp_potential", "[test_player]") {
  int lines = 100;
  int cols = 100;
  Field* field_ = new Field(lines, cols, new RandomGenerator(&RandomGenerator::ran));
  field_->AddHills();
  Player* player_one_ = new Player(field_->AddDen(lines/1.5, lines-5, cols/1.5, cols-5), 3);
  Player* player_two_ = new Player(field_->AddDen(5, lines/3, 5, cols/3), 4);
  field_->BuildGraph(player_one_->nucleus_pos(), player_two_->nucleus_pos());
}

