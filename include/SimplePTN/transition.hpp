// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 
// This file contains the Transition class

#ifndef THIRD_PARTY_SIMPLEPTN_INCLUDE_SIMPLEPTN_TRANSITION_HPP_
#define THIRD_PARTY_SIMPLEPTN_INCLUDE_SIMPLEPTN_TRANSITION_HPP_

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "place.hpp"

namespace sptn {

///
///\brief Class representing a PTN Transition
///
/// Objects of this class shall not be instanciated directly, \see sptn::PetriNet
///
///\tparam IDT the ID type (must overload operator==)
///\tparam TokenCounterT the counting type (must overload operator+, operator- and operator<)
///
template <typename ID = std::string, typename TokenCounter = uint32_t> class Transition {
public:
  using IDT = ID;
  using TokenCounterT = TokenCounter;
  using PlaceT = Place<IDT, TokenCounterT>;
  using WeightPairT = std::pair<std::shared_ptr<PlaceT>, TokenCounterT>;
  template <typename A, typename B> friend class PetriNet;
  template <typename A, typename B> friend class Place;

private:
  IDT id_;
  std::vector<WeightPairT> ingoing_;
  std::vector<WeightPairT> outgoing_;
  std::function<bool(const Transition<IDT, TokenCounterT> &)> evaluate_condition_;
  std::shared_ptr<std::shared_mutex> net_mutex_;

  ///
  ///\brief Construct a new Transition with ingoing and outgoing weight pairs
  ///
  ///\param id the Transition ID
  ///\param ingoing the ingoing Places with weights
  ///\param outgoing  the outgoing Places with weights
  ///\param net_mutex the Mutex of the PetriNet
  ///
  Transition(const IDT &id, std::vector<WeightPairT> &&ingoing, std::vector<WeightPairT> &&outgoing,
             std::shared_ptr<std::shared_mutex> net_mutex)
      : id_(id), ingoing_(ingoing), outgoing_(outgoing), net_mutex_(net_mutex) {}

  Transition(Transition<IDT, TokenCounterT> &&from) { *this = std::move(from); }

  ///
  ///\brief Move all members to this Transition (invalidates old members)
  ///
  ///\param from Transition to move from
  ///
  void operator=(Transition<IDT, TokenCounterT> &&from) {
    this->id_ = std::move(from.id_);
    this->ingoing_ = std::move(from.ingoing_);
    this->outgoing_ = std::move(from.outgoing_);
    this->evaluate_condition_ = std::move(from.evaluate_condition_);
  }

  void operator=(const Transition<IDT, TokenCounterT> &from) = delete;

  ///
  ///\brief Perform ready() check (does not lock mutex)
  ///
  ///\return true
  ///\return false
  ///
  bool readyAcquired() const noexcept(true) {
    for (auto &[place, weight] : ingoing_) {
      if (place->tokens_ < weight) {
        return false;
      }
    }

    return true;
  }

  ///
  ///\brief Tick this transition, then deepTick all transitions that might
  /// have become ready.
  ///
  ///\param std::set<IDT> &seen all places encountered so far (cycle detection)
  ///\throws std::runtime_error if cycles were detected
  ///
  void deepTick(std::set<IDT> &seen) {
    if (this->tick()) {
      for (auto &[place, _] : this->outgoing_) {
        place->deepTick(seen);
      }
    }
  }

public:
  ///
  ///\brief Check if this Transition is ready to fire()
  ///
  ///\return true ready
  ///\return false not ready
  ///
  [[nodiscard]] bool ready() const noexcept(true) {
    std::shared_lock lock(*this->net_mutex_);
    return this->readyAcquired();
  }

  ///
  ///\brief Try to fire() this  Transition
  ///
  ///\return true successfull
  ///\return false not ready
  ///
  bool fire() const noexcept(true) {
    bool rdy = false;

    // Memorize places, before the datastructures get unlocked again
    std::vector<std::pair<std::shared_ptr<PlaceT>, TokenCounterT>> places_to_notify;

    this->net_mutex_->lock();

    // Fire, if ready
    rdy = this->readyAcquired();
    if (rdy) {
      places_to_notify.reserve(this->ingoing_.size() + this->outgoing_.size());

      for (auto &[place, weight] : this->ingoing_) {
        places_to_notify.push_back(std::make_pair(place, place->tokens_));
        place->tokens_ = place->tokens_ - weight;
      }
      for (auto &[place, weight] : this->outgoing_) {
        places_to_notify.push_back(std::make_pair(place, place->tokens_));
        place->tokens_ = place->tokens_ + weight;
      }
    }

    this->net_mutex_->unlock();

    // Notify about place changes (in an unlocked context)
    for (const auto &[place, prev] : places_to_notify) {
      place->changed(prev);
    }

    return rdy;
  }

  ///
  ///\brief Fire this transition, then deepTick all transitions that might
  /// have become ready.
  ///
  /// \throws std::runtime_error if cycles were detected
  ///
  bool deepFire() noexcept(false) {
    if (this->fire()) {
      std::set<IDT> seen;
      for (auto &[place, _] : this->outgoing_) {
        place->deepTick(seen);
      }
      return true;
    }
    return false;
  }

  ///
  ///\brief Add a evaluate function as a condition for this transition to fire
  ///
  /// The parameter can also be an empty function to disable this feature.
  /// fire() will be called when tick() was called and evaluate_condition() returns
  /// true for this call
  ///
  ///\param evaluate_condition the condition function
  ///
  void autoFire(std::function<bool(const Transition<IDT, TokenCounterT> &)>
                    evaluate_condition) noexcept(true) {
    this->evaluate_condition_ = evaluate_condition;
  }

  ///
  ///\brief Auto-fire this transition on each tick
  ///
  void autoFire() {
    this->evaluate_condition_ = [](auto &) { return true; };
  }

  ///
  ///\brief tick() once if autoFire() is unset, nothing will happen
  ///
  ///\return bool fired?
  ///
  bool tick() noexcept(true) {
    if (this->evaluate_condition_ != nullptr && this->evaluate_condition_(*this)) {
      return this->fire();
    }
    return false;
  }

  ///
  ///\brief Tick this transition, then deepTick all transitions that might
  /// have become ready.
  ///
  /// \throws std::runtime_error if cycles were detected
  ///
  void deepTick() noexcept(false) {
    std::set<IDT> seen;
    this->deepTick(seen);
  }

  ///
  ///\brief get the Transition's ID
  ///
  ///\return const IDT&
  ///
  [[nodiscard]] const IDT &getID() const noexcept(true) { return this->id_; }
};

}  // namespace sptn

#endif  // THIRD_PARTY_SIMPLEPTN_INCLUDE_SIMPLEPTN_TRANSITION_HPP_
