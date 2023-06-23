// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 

#define CATCH_CONFIG_NO_POSIX_SIGNALS

#include "SimplePTN/place.hpp"

#include <catch2/catch.hpp>

enum class ID { k_one, k_two, k_three };

struct Counter {
  int internal_ = 0;

  constexpr void operator+(const Counter &other) { this->internal_ += other.internal_; }
  constexpr void operator-(const Counter &other) { this->internal_ -= other.internal_; }
  constexpr bool operator==(const Counter &other) const {
    return this->internal_ == other.internal_;
  }
};

namespace sptn {
// Proxy class (for firend access)
template <typename IDT = std::string, typename TokenCounterT = uint32_t> class PetriNet {
public:
  using PlaceT = Place<IDT, TokenCounterT>;

  static void fMapTokens(PlaceT &place, std::function<TokenCounterT(TokenCounterT)> f) {
    place.tokens_ = f(place.tokens_);
  }

  static std::shared_ptr<PlaceT> makePlace(const IDT &id, TokenCounterT initial) {
    return std::shared_ptr<PlaceT>(new PlaceT(id, initial));
  }
};

}  // namespace sptn

TEST_CASE("sptn::Place behaviour", "[SPTN][Place]") {
  GIVEN("A Place with default types and a Place with custom types") {
    auto place = sptn::PetriNet<>::makePlace("ID1", 1);
    auto place2 = sptn::PetriNet<ID, Counter>::makePlace(ID::k_one, Counter{3});

    WHEN("Getters are called") {
      THEN("The return values match") {
        REQUIRE(place->getID() == "ID1");
        REQUIRE(place->getTokens() == 1);
        REQUIRE(place2->getID() == ID::k_one);
        REQUIRE(place2->getTokens() == Counter{3});
      }
    }
  }
}
