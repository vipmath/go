#include "zero/mcts.h"

#include <memory>

#include "zero/features.h"
#include "zero/game.h"
#include "zero/grid.h"
#include "zero/player.h"

class Random;

namespace go_zero {

GameTree::GameTree(double decay, const Game& state, const Player& player,
                   const GameTree* parent)
  : decay_(decay),
    state_(state),
    children_(state.width(), state.height()),
    visits_(state.width(), state.height()),
    action_(state.width(), state.height()),
    parent_(parent) {
  // Evaluate position
  Features features;
  state.GetFeatures(&features);
  evaluation_ = std::move(player.Evaluate(features));

  // Initialize Action weights.
  // TODO: Add Move loop iteration (as explained in https://stackoverflow.com/questions/8164567/how-to-make-my-custom-type-to-work-with-range-based-for-loops)
  for (int x = 0; x < action_.width(); ++x) {
    for (int y = 0; y < action_.height(); ++y) {
      Move move(Move::Type::kPlayStone, Pos(x, y));
      action_.Get(move) = evaluation_->policy().Get(move);
    }
  }
  Move move(Move::Type::kPass);
  // TODO: Make sure this is doing a data copy (not ref copy).
  action_.Get(move) = evaluation_->policy().Get(move);
}

Move GameTree::RandomMove(Random* random) const {
  // TODO
  return Move(Move::Type::kPass);
}

void GameTree::AddVisit(Move move) {
  visits_.Get(move) += 1;
  action_.Get(move) *= decay_;
}

MCTS::MCTS(int searches_per_move, double decay, const Game& initial_state,
           const Player* player)
  : searches_per_move_(searches_per_move),
    decay_(decay),
    player_(player),
    root_(decay_, initial_state, *player_, nullptr),
    current_node_(&root_) {}
  

void MCTS::SearchOnce() {
  GameTree* node = current_node_;
  while (true) {
    // Choose a random move to explore.
    Move move = node->RandomMove(&random_);
    node->AddVisit(move);
    if (node->child(move) == nullptr) {
      // Unexplored node. Evaluate it and finish.
      Game new_state(node->state());
      if (new_state.Play(move)) {
        node->set_child(move, new GameTree(decay_, new_state, *player_, node));
        // TODO: Update Actions for all parents.
      } else {
        // TODO: What to do about illegal moves? Right now we do less
        // exploration because of them.
      }
      return;
    }
    node = node->child(move);
  }
}

void MCTS::MoveOnce() {
  // Explore tree.
  for (int i = 0; i < searches_per_move_; ++i) {
    SearchOnce();
  }
  // Choose a move. We just choose the move we've explored the most (which
  // is a proxy for the best move because we bias our search towards the move
  // we think is best).
  int max_visits = -1;
  std::vector<Move> best_moves;
  for (int x = 0; x < current_node_->width(); ++x) {
    for (int y = 0; y < current_node_->height(); ++y) {
    }
  }
  // TODO: Pass

  // Pick a random best move.
  Move move = random_.Choose(best_moves);

  moves_.push_back(move);
  current_node_ = current_node_->child(move);
}

// TODO: Remove Legacy methods

// Runs single rollout and returns whether or not current player won.
bool MCRollout(const Game& starting_position, const Player& player,
               int max_moves, Random* random) {
  Game game(starting_position);
  while (!game.IsGameOver() && game.moves_played() < max_moves) {
    // Get input board features (color of stone at every point, etc.)
    Features features;
    game.GetFeatures(&features);

    // Evaluate this in the Neural Network.
    std::unique_ptr<PlayerEvaluation> policy = player.Evaluate(features);

    // Keep trying to play moves until you play a legal one.
    // TODO: Will this cause problems? Maybe Pass if illegal move is chosen?
    bool successfully_moved = false;
    while (!successfully_moved) {
      // Randomly choose a move based on the policy.
      Move move = policy->RandomMove(random);
      successfully_moved = game.Play(move);
    }
  }

  return (game.winner() == starting_position.curr_player());
}

double MCPercent(const Game& starting_position, const Player& player,
                 int max_moves, Random* random, int num_runs) {
  int successes = 0;
  for (int run_num = 0; run_num < num_runs; ++run_num) {
    if (MCRollout(starting_position, player, max_moves, random)) {
      successes += 1;
    }
  }
  return static_cast<double>(successes) / num_runs;
}

}  // namespace go_zero
