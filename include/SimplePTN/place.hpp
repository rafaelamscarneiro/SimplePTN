// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 
// This file contains the Place class

#ifndef SIMPLEPTN_INCLUDE_SIMPLEPTN_PLACE_HPP_
#define SIMPLEPTN_INCLUDE_SIMPLEPTN_PLACE_HPP_

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace sptn {

template <typename ID, typename B> class Transition;

///
///\brief Class representing a PTN Place
///
/// Objects of this class shall not be instanciated directly, \see sptn::PetriNet
///
///\tparam IDT the ID type (must overload operator==)
///\tparam TokenCounterT the counting type (must overload operator+, operator- and operator<)
///
template <typename ID = std::string, typename TokenCounter = uint32_t> class Place final {
  template <typename A, typename B> friend class Transition;
  template <typename A, typename B> friend class PetriNet;

public:
  using IDT = ID;
  using TokenCounterT = TokenCounter;
  using TransitionT = Transition<IDT, TokenCounterT>;

private:
  IDT id_;
  TokenCounterT tokens_;
  std::function<void(const Place<IDT, TokenCounterT> &, TokenCounterT)> on_change_;
  std::vector<std::shared_ptr<TransitionT>> outgoing_to_;
  std::vector<std::shared_ptr<TransitionT>> ingoing_to_;

  ///
  ///\brief Call on_change_ if set
  ///
  ///\param prev the previous token count
  ///
  void changed(TokenCounterT prev) const noexcept(true) {
    if (this->on_change_ != nullptr) {
      this->on_change_(*this, prev);
    }
  }

  ///
  ///\brief Forward a deepTick to all transitions this place is positive incident to
  ///
  ///\param seen all seen places (cycle detection)
  ///\throws std::runtime_error if cycles were detected
  ///
  void deepTick(std::set<IDT> &seen) noexcept(false) {
    if (seen.find(this->id_) != cend(seen)) {
      throw std::runtime_error("Cycle detected");
    }

    seen.insert(this->id_);

    for (auto &transition : this->ingoing_to_) {
      transition->deepTick(seen);
    }

    seen.erase(this->id_);
  }

  ///
  ///\brief Construct a new Place object
  ///
  ///\param id  the id
  ///\param initial_tokens the initial number of tokens
  ///
  Place(const IDT &id, TokenCounterT initial_tokens) : id_(id), tokens_(initial_tokens) {}

public:
  ///
  ///\brief Get the current amount of Tokens on this Place
  ///
  ///\return TokenCounterT the amount
  ///
  [[nodiscard]] TokenCounterT getTokens() const noexcept(true) { return this->tokens_; }

  ///
  ///\brief Set the active onChange listener (args: (Place, prev_tokens))
  ///
  /// The listener will be called as soon, as the amount of tokens changes
  ///
  ///\param fn the listener to call
  ///
  void onChange(std::function<void(const Place<IDT, TokenCounterT> &, TokenCounterT)> fn) noexcept(
      true) {
    this->on_change_ = fn;
  }

  ///
  ///\brief Get the Place's ID
  ///
  ///\return const IDT&
  ///
  [[nodiscard]] const IDT &getID() const noexcept(true) { return this->id_; }

  ///
  ///\brief Forward a deepTick to all transitions this place is positive incident to
  ///
  ///\throws std::runtime_error if cycles were detected
  ///
  void deepTick() noexcept(false) {
    std::set<IDT> seen;
    this->deepTick(seen);
  }
};

}  // namespace sptn

#endif  // SIMPLEPTN_INCLUDE_SIMPLEPTN_PLACE_HPP_
