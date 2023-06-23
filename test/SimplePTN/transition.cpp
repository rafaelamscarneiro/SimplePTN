// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 

#include "SimplePTN/transition.hpp"

#include <catch2/catch.hpp>
#include <functional>
#include <list>
#include <map>

#include "SimplePTN/place.hpp"

namespace sptn {

// Proxy class (for firend access)
template <typename IDT = std::string, typename TokenCounterT = uint32_t> class PetriNet {
public:
  using PlaceT = Place<IDT, TokenCounterT>;
  using TransitionT = Transition<IDT, TokenCounterT>;
  using WeightPairT = typename TransitionT::WeightPairT;

  static void fMapTokens(PlaceT &place, std::function<TokenCounterT(TokenCounterT)> f) {
    place.tokens_ = f(place.tokens_);
  }

  static std::shared_ptr<TransitionT> makeTransition(const IDT &id,
                                                     std::vector<WeightPairT> &&ingoing,
                                                     std::vector<WeightPairT> &&outgoing) {
    return std::shared_ptr<TransitionT>(new TransitionT(id, std::move(ingoing), std::move(outgoing),
                                                        std::make_shared<std::shared_mutex>()));
  }

  static std::shared_ptr<PlaceT> makePlace(const IDT &id, TokenCounterT initial) {
    return std::shared_ptr<PlaceT>(new PlaceT(id, initial));
  }
};

}  // namespace sptn

TEST_CASE("sptn::Transition ready()", "[SPTN][Transition]") {
  GIVEN("A Transition with multiple ingoing Places") {
    auto placeA = sptn::PetriNet<>::makePlace("A", 2);
    auto placeB = sptn::PetriNet<>::makePlace("B", 3);
    auto placeC = sptn::PetriNet<>::makePlace("C", 4);
    auto placeD = sptn::PetriNet<>::makePlace("D", 0);
    auto placeE = sptn::PetriNet<>::makePlace("E", 0);

    auto transition = sptn::PetriNet<>::makeTransition("T", {{placeA, 2}, {placeB, 3}, {placeC, 4}},
                                                       {{placeD, 1}, {placeE, 1}});

    WHEN("when having enough tokens") {
      THEN("ready() returns true") { REQUIRE(transition->ready()); }
    }
    WHEN("missing some tokens") {
      using namespace std::placeholders;
      auto minus1 = std::bind(std::minus<uint32_t>{}, _1, 1);
      sptn::PetriNet<>::fMapTokens(*placeC, minus1);
      THEN("ready() returns false") { REQUIRE_FALSE(transition->ready()); }
    }
  }
}

TEST_CASE("sptn::Transition fire()", "[SPTN][Transition]") {
  GIVEN("A Transition with multiple ingoing Places") {
    auto placeA = sptn::PetriNet<>::makePlace("A", 3);
    auto placeB = sptn::PetriNet<>::makePlace("B", 4);
    auto placeC = sptn::PetriNet<>::makePlace("C", 5);
    auto placeD = sptn::PetriNet<>::makePlace("D", 0);
    auto placeE = sptn::PetriNet<>::makePlace("E", 0);

    auto transition = sptn::PetriNet<>::makeTransition("T", {{placeA, 2}, {placeB, 3}, {placeC, 4}},
                                                       {{placeD, 1}, {placeE, 1}});

    WHEN("firing the transition") {
      transition->fire();
      THEN("the tokens get distributed properly") {
        REQUIRE(placeA->getTokens() == 1);
        REQUIRE(placeB->getTokens() == 1);
        REQUIRE(placeC->getTokens() == 1);
        REQUIRE(placeD->getTokens() == 1);
        REQUIRE(placeE->getTokens() == 1);
      }
      WHEN("firing an additional time") {
        THEN("it returns false") { REQUIRE_FALSE(transition->fire()); }
      }
    }
  }
}

TEST_CASE("sptn::Transition autoFire()", "[SPTN][Transition]") {
  GIVEN("A Transition with multiple ingoing Places") {
    auto placeA = sptn::PetriNet<>::makePlace("A", 3);
    auto placeB = sptn::PetriNet<>::makePlace("B", 4);
    auto placeC = sptn::PetriNet<>::makePlace("C", 5);
    auto placeD = sptn::PetriNet<>::makePlace("D", 0);
    auto placeE = sptn::PetriNet<>::makePlace("E", 0);

    auto transition = sptn::PetriNet<>::makeTransition("T", {{placeA, 2}, {placeB, 3}, {placeC, 4}},
                                                       {{placeD, 1}, {placeE, 1}});

    WHEN("autoFire is disabled") {
      THEN("tick() will do nothing") {
        transition->tick();
        REQUIRE(placeA->getTokens() == 3);
        REQUIRE(placeB->getTokens() == 4);
        REQUIRE(placeC->getTokens() == 5);
        REQUIRE(placeD->getTokens() == 0);
        REQUIRE(placeE->getTokens() == 0);
      }
    }

    WHEN("autoFire is enabled") {
      bool condition = false;
      auto condition_checker = [&](const sptn::Transition<> &) { return condition; };
      transition->autoFire(condition_checker);
      THEN("before tick() nothing happened") {
        REQUIRE(placeA->getTokens() == 3);
        REQUIRE(placeB->getTokens() == 4);
        REQUIRE(placeC->getTokens() == 5);
        REQUIRE(placeD->getTokens() == 0);
        REQUIRE(placeE->getTokens() == 0);
      }
      THEN("after tick() (condition = false) nothing happened") {
        condition = false;
        transition->tick();
        REQUIRE(placeA->getTokens() == 3);
        REQUIRE(placeB->getTokens() == 4);
        REQUIRE(placeC->getTokens() == 5);
        REQUIRE(placeD->getTokens() == 0);
        REQUIRE(placeE->getTokens() == 0);
      }
      THEN("after tick() (condition = true) the tokens get distributed properly") {
        condition = true;
        transition->tick();
        REQUIRE(placeA->getTokens() == 1);
        REQUIRE(placeB->getTokens() == 1);
        REQUIRE(placeC->getTokens() == 1);
        REQUIRE(placeD->getTokens() == 1);
        REQUIRE(placeE->getTokens() == 1);
      }
    }
  }
}

TEST_CASE("sptn::Transition triggers onChange of places", "[SPTN][Transition]") {
  GIVEN("A Transition with multiple ingoing Places") {
    auto placeA = sptn::PetriNet<>::makePlace("A", 3);
    auto placeB = sptn::PetriNet<>::makePlace("B", 4);
    auto placeC = sptn::PetriNet<>::makePlace("C", 5);
    auto placeD = sptn::PetriNet<>::makePlace("D", 0);
    auto placeE = sptn::PetriNet<>::makePlace("E", 0);

    auto transition = sptn::PetriNet<>::makeTransition("T", {{placeA, 2}, {placeB, 2}, {placeC, 2}},
                                                       {{placeD, 1}, {placeE, 2}});

    std::map<std::string, uint32_t> on_change_calls;
    auto registerChange = [&](const sptn::Place<> &p, uint32_t discard) {
      (void)discard;
      on_change_calls[p.getID()] = p.getTokens();
    };
    placeA->onChange(registerChange);
    placeB->onChange(registerChange);
    placeC->onChange(registerChange);
    placeD->onChange(registerChange);
    placeE->onChange(registerChange);

    WHEN("firing the transition") {
      transition->fire();
      THEN("all places get notified with the correct values") {
        REQUIRE(on_change_calls["A"] == 1);
        REQUIRE(on_change_calls["B"] == 2);
        REQUIRE(on_change_calls["C"] == 3);
        REQUIRE(on_change_calls["D"] == 1);
        REQUIRE(on_change_calls["E"] == 2);
        REQUIRE(on_change_calls.size() == 5);
      }
    }
  }
}