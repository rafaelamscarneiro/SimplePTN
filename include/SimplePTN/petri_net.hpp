// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 
// This file contains the PetriNet class

#ifndef THIRD_PARTY_SIMPLEPTN_INCLUDE_SIMPLEPTN_PETRI_NET_HPP_
#define THIRD_PARTY_SIMPLEPTN_INCLUDE_SIMPLEPTN_PETRI_NET_HPP_

#include <algorithm>
#include <memory>
#include <set>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "place.hpp"
#include "transition.hpp"

namespace sptn {

///
///\brief Class representing a PTN
///
/// This is the only class which has to be instanciated by the user.
/// Places and Transitions are meant to be initialized via this class.
///
///\tparam IDT the ID type (must overload operator== and operator<)
///\tparam TokenCounterT the counting type (must overload operator+, operator- and operator<)
///
template <typename ID = std::string, typename TokenCounter = uint32_t> class PetriNet {
public:
  using IDT = ID;
  using TokenCounterT = TokenCounter;
  using PlaceT = Place<IDT, TokenCounterT>;
  using TransitionT = Transition<IDT, TokenCounterT>;

  ///
  ///\brief A descriptive Structure for Transitions used as a blueprint
  ///
  /// NOTE: Weight will be subtracted from ingoing places and added to outgoing places
  ///
  struct TransitionSketch {
    using SketchWeightPairT = std::pair<IDT, TokenCounterT>;
    ///\brief the desired id
    IDT id;

    ///\brief all ingoing places <placeId, weight> pair
    std::vector<SketchWeightPairT> ingoing;

    ///\brief all outgoing places <placeId, weight> pair
    std::vector<SketchWeightPairT> outgoing;
  };

private:
  using WeightPairT = typename TransitionT::WeightPairT;
  std::vector<std::shared_ptr<PlaceT>> places_;
  std::vector<std::shared_ptr<TransitionT>> transitions_;
  mutable std::shared_ptr<std::shared_mutex> mutex_;

  ///
  ///\brief Find a Place by ID (does not lock the mutex)
  ///
  ///\param id the id to search for
  ///\return std::shared_ptr<PlaceT> may ne nullptr if not found
  ///
  std::shared_ptr<PlaceT> findPlaceAcquired(const IDT &id) noexcept(true) {
    auto match_id = [&](std::shared_ptr<PlaceT> place) {
      return place != nullptr && place->getID() == id;
    };

    auto it = std::find_if(cbegin(this->places_), cend(this->places_), match_id);
    if (it != cend(this->places_)) {
      return *it;
    }

    return nullptr;
  }

  ///
  ///\brief Find a Transition by ID (does not lock the mutex)
  ///
  ///\param id the id to search for
  ///\return std::shared_ptr<TransitionT> may ne nullptr if not found
  ///
  std::shared_ptr<TransitionT> findTransitionAcquired(const IDT &id) noexcept(true) {
    auto match_id = [&](std::shared_ptr<TransitionT> transition) {
      return transition != nullptr && transition->getID() == id;
    };

    auto it = std::find_if(cbegin(this->transitions_), cend(this->transitions_), match_id);
    if (it != cend(this->transitions_)) {
      return *it;
    }

    return nullptr;
  }

  ///
  ///\brief Add a new Place (do not lock the mutex)
  ///
  ///\param id the Place's ID
  ///\param initial_tokens the number of initial tokens on this Place
  ///
  ///\throws std::invalid_argument if the ID already exists
  ///
  std::shared_ptr<PlaceT> addPlaceAcquired(const IDT &id,
                                           TokenCounterT initial_tokens) noexcept(false) {
    // Do not allow duplicate IDs
    if (findPlaceAcquired(id) != nullptr) {
      throw std::invalid_argument("PlaceID already exists");
    }

    auto ptr = std::shared_ptr<PlaceT>(new PlaceT(id, initial_tokens));
    places_.push_back(ptr);
    return ptr;
  }

  ///
  ///\brief Add a new Transition (do not lock the mutex)
  ///
  ///\param sketch the transition blueprint
  ///
  ///\throws std::invalid_argument if the ID already exists or any ID of a Place referenced in it
  /// is non-existent.
  ///
  std::shared_ptr<TransitionT> addTransitionAcquired(const TransitionSketch &sketch) noexcept(
      false) {
    std::vector<WeightPairT> ingoing;
    ingoing.reserve(sketch.ingoing.size());
    std::vector<WeightPairT> outgoing;
    outgoing.reserve(sketch.outgoing.size());

    // Do not allow duplicate IDs
    if (findTransitionAcquired(sketch.id) != nullptr) {
      throw std::invalid_argument("TransitionID already exists");
    }

    // Find all ingoing places
    for (auto &[pid, weight] : sketch.ingoing) {
      auto place = findPlaceAcquired(pid);
      if (place == nullptr) {
        throw std::invalid_argument("Sketch contains invalid IDs");
      }
      ingoing.push_back(std::make_pair(place, weight));
    }

    // Find all outgoing places
    for (auto &[pid, weight] : sketch.outgoing) {
      auto place = findPlaceAcquired(pid);
      if (place == nullptr) {
        throw std::invalid_argument("Sketch contains invalid IDs");
      }
      outgoing.push_back(std::make_pair(place, weight));
    }

    // add new transition
    auto ptr = std::shared_ptr<TransitionT>(
        new TransitionT(sketch.id, std::move(ingoing), std::move(outgoing), mutex_));
    transitions_.push_back(ptr);

    // add to neighbour mappings
    for (auto &[place, _] : ptr->ingoing_) {
      place->ingoing_to_.push_back(ptr);
    }
    for (auto &[place, _] : ptr->outgoing_) {
      place->outgoing_to_.push_back(ptr);
    }

    return ptr;
  }

public:
  ///
  ///\brief Construct a new PetriNet
  ///
  PetriNet() : mutex_(std::make_shared<std::shared_mutex>()) {}

  ///
  ///\brief find a Place with the given ID
  ///
  ///\param id  the ID
  ///\return std::shared_ptr<PlaceT> may be nullptr if the ID was not found
  ///
  [[nodiscard]] std::shared_ptr<PlaceT> findPlace(const IDT &id) noexcept(true) {
    std::shared_lock lock(*this->mutex_);

    return this->findPlaceAcquired(id);
  }

  ///
  ///\brief find a Transition with the given ID
  ///
  ///\param id the ID
  ///\return std::shared_ptr<TransitionT> may be nullptr if the ID was not found
  ///
  [[nodiscard]] std::shared_ptr<TransitionT> findTransition(const IDT &id) noexcept(true) {
    std::shared_lock lock(*this->mutex_);

    return this->findTransitionAcquired(id);
  }

  PetriNet<IDT, TokenCounterT> &&clone() const;

  ///
  ///\brief Add a new Place
  ///
  ///\param id the Place's ID
  ///\param initial_tokens the number of initial tokens on this Place
  ///
  ///\throws std::invalid_argument if the ID already exists
  ///
  std::shared_ptr<PlaceT> addPlace(const IDT &id, TokenCounterT initial_tokens) noexcept(false) {
    std::lock_guard lock(*this->mutex_);

    return this->addPlaceAcquired(id, initial_tokens);
  }

  ///
  ///\brief Add a new Transition
  ///
  ///\param sketch the transition blueprint
  ///
  ///\throws std::invalid_argument if the ID already exists or any ID of a Place referenced in it
  /// is non-existent.
  ///
  std::shared_ptr<TransitionT> addTransition(const TransitionSketch &sketch) noexcept(false) {
    std::lock_guard lock(*this->mutex_);

    return this->addTransitionAcquired(sketch);
  }

  ///
  ///\brief Merge another PetriNet into this one.
  ///
  /// The interconnections are used to connect the two nets, each interconnection
  /// shall contain a Place from this and a place from other.
  /// NOTE: The other PetriNet will be empty afterwards to ensure no two Nodes/Transitions have
  /// the same callback functions registered
  ///
  ///\param other the PetriNet to merge into this
  ///\param interconnections all interconnections between our places and other places
  ///
  void merge(PetriNet &&other,
             const std::vector<TransitionSketch> &interconnections) noexcept(false) {
    // Keep both nets locked
    std::lock_guard l1(*other.mutex_);
    std::lock_guard l2(*this->mutex_);

    // Check for duplicate PlaceIDs
    for (const auto &place : other.places_) {
      auto it = std::find_if(cbegin(this->places_), cend(this->places_),
                             [&](const auto &p) { return p->getID() == place->getID(); });
      if (it != cend(this->places_)) {
        throw std::invalid_argument("Duplicate PlaceIDs");
      }
    }

    // Check for duplicate TransitionIDs in other
    for (const auto &transition : other.transitions_) {
      auto it = std::find_if(cbegin(this->transitions_), cend(this->transitions_),
                             [&](const auto &t) { return t->getID() == transition->getID(); });
      if (it != cend(this->transitions_)) {
        throw std::invalid_argument("Duplicate TransactionIDs");
      }
    }

    // Check for duplicate TransitionIDs in interconnections
    for (const auto &[tid, _, __] : interconnections) {
      const auto &tid_r = tid;  // lambda captures seem to dislike structural decomposition vars :(
      auto it = std::find_if(cbegin(this->transitions_), cend(this->transitions_),
                             [&](const auto &t) { return t->getID() == tid_r; });
      if (it != cend(this->transitions_)) {
        throw std::invalid_argument("Duplicate TransitionIDs");
      }
    }

    using SketchEvalCondPair =
        std::pair<TransitionSketch, std::function<bool(const TransitionT &)>>;
    std::vector<SketchEvalCondPair> new_transitions;
    new_transitions.reserve(other.transitions_.size());

    // Create sketches for each other transition
    for (const auto &t : other.transitions_) {
      TransitionSketch sketch;
      sketch.id = t->getID();

      auto to_sketch_weight_pair = [&](const auto &wp) ->
          typename TransitionSketch::SketchWeightPairT {
            return {wp.first->getID(), wp.second};
          };
      std::transform(cbegin(t->ingoing_), cend(t->ingoing_), std::back_inserter(sketch.ingoing),
                     to_sketch_weight_pair);
      std::transform(cbegin(t->outgoing_), cend(t->outgoing_), std::back_inserter(sketch.outgoing),
                     to_sketch_weight_pair);

      new_transitions.push_back({std::move(sketch), t->evaluate_condition_});
    }

    other.transitions_.clear();

    // Move other places
    for (auto &place : other.places_) {
      place->ingoing_to_.clear();
      place->outgoing_to_.clear();
    }
    std::move(begin(other.places_), end(other.places_), std::back_inserter(this->places_));

    // Create other transitions (will have valid data)
    for (const auto &[sketch, eval_cond] : new_transitions) {
      this->addTransitionAcquired(sketch);
      this->findTransitionAcquired(sketch.id)->evaluate_condition_ = eval_cond;
    }

    // Create interconnections
    for (const auto &sketch : interconnections) {
      this->addTransitionAcquired(sketch);
    }
  }

  ///
  ///\brief execute tick() of every transition once
  ///
  void tick() noexcept(true) {
    for (auto &transition : transitions_) {
      transition->tick();
    }
  }

  ///
  ///\brief Tick through the whole PTN from a starting place
  ///
  /// When a Transition A on the Path fires, all Transitions that follow
  /// on any of the outgoing Places from A will fire, too.
  /// NOTE: The PTN has to be acyclic, otherwise an exception will be thrown
  ///
  ///\param start_place_id the starting place id
  ///\throws std::runtime_error when cycles are detected
  ///\throws std::invalid_argument when the start_place_id was not found
  ///
  void deepTick(IDT start_place_id) noexcept(false) {
    auto ptr = this->findPlace(start_place_id);
    if (ptr == nullptr) {
      throw std::invalid_argument("Start place ID not found");
    }
    ptr->deepTick();
  }

  ///
  ///\brief Execute deepTick() for every place
  ///
  void deepTickCover() noexcept(true) {
    for (const auto &place_ptr : this->places_) {
      this->deepTick(place_ptr->getID());
    }
  }
};

}  // namespace sptn

#endif  // THIRD_PARTY_SIMPLEPTN_INCLUDE_SIMPLEPTN_PETRI_NET_HPP_
