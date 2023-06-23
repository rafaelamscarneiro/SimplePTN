// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 

#include "SimplePTN/petri_net.hpp"

#include <catch2/catch.hpp>
#include <iostream>
#include <map>

TEST_CASE("sptn::PetriNet find{Place,Transition}()"
          "[SPTN][PetriNet]") {
  GIVEN("A petri net with some transitions and places") {
    sptn::PetriNet<> net;
    net.addPlace("A", 2);
    net.addPlace("B", 3);
    net.addPlace("C", 4);
    net.addPlace("D", 0);
    net.addPlace("E", 0);

    sptn::PetriNet<>::TransitionSketch sketchT1{
        "T1", {{"A", 1}, {"B", 2}, {"C", 1}}, {{"D", 1}, {"E", 1}}};
    sptn::PetriNet<>::TransitionSketch sketchT2{
        "T2", {{"D", 1}, {"E", 1}}, {{"A", 1}, {"B", 2}, {"C", 1}}};
    net.addTransition(sketchT1);
    net.addTransition(sketchT2);

    WHEN("findTransition is called") {
      THEN("the transition is returned") {
        auto t1 = net.findTransition("T1");
        auto t2 = net.findTransition("T2");

        REQUIRE(t1 != nullptr);
        REQUIRE(t2 != nullptr);
        REQUIRE(t1->getID() == "T1");
        REQUIRE(t2->getID() == "T2");
      }
      THEN("Invalid id return nullptr") { REQUIRE(net.findTransition("invalid") == nullptr); }
    }

    WHEN("findPlace is called") {
      THEN("the place is returned") {
        auto a = net.findPlace("A");
        auto b = net.findPlace("B");
        auto c = net.findPlace("C");
        auto d = net.findPlace("D");
        auto e = net.findPlace("E");

        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(c != nullptr);
        REQUIRE(d != nullptr);
        REQUIRE(e != nullptr);
        REQUIRE(a->getID() == "A");
        REQUIRE(b->getID() == "B");
        REQUIRE(c->getID() == "C");
        REQUIRE(d->getID() == "D");
        REQUIRE(e->getID() == "E");
        REQUIRE(a->getTokens() == 2);
        REQUIRE(b->getTokens() == 3);
        REQUIRE(c->getTokens() == 4);
        REQUIRE(d->getTokens() == 0);
        REQUIRE(e->getTokens() == 0);
      }

      THEN("Invalid id return nullptr") { REQUIRE(net.findPlace("invalid") == nullptr); }
    }
  }
}

TEST_CASE("sptn::PetriNet combined test"
          "[SPTN][PetriNet]") {
  GIVEN("A petri net with some transitions and places") {
    sptn::PetriNet<> net;
    net.addPlace("A", 2);
    net.addPlace("B", 3);
    net.addPlace("C", 4);
    net.addPlace("D", 0);
    net.addPlace("E", 0);

    sptn::PetriNet<>::TransitionSketch sketchT1{
        "T1", {{"A", 1}, {"B", 2}, {"C", 1}}, {{"D", 1}, {"E", 1}}};
    sptn::PetriNet<>::TransitionSketch sketchT2{"T2", {{"D", 1}}, {{"A", 1}, {"B", 2}, {"C", 1}}};
    net.addTransition(sketchT1);
    net.addTransition(sketchT2);

    std::map<std::string, uint32_t> changes;
    auto register_change = [&](const typename sptn::PetriNet<>::PlaceT &p, uint32_t prev) {
      (void)prev;
      changes[p.getID()] = p.getTokens();
    };

    net.findPlace("A")->onChange(register_change);
    net.findPlace("B")->onChange(register_change);
    net.findPlace("C")->onChange(register_change);
    net.findPlace("D")->onChange(register_change);
    net.findPlace("E")->onChange(register_change);

    bool cond1 = false;
    bool cond2 = false;
    auto check_cond1 = [&](const typename sptn::PetriNet<>::TransitionT &) { return cond1; };
    auto check_cond2 = [&](const typename sptn::PetriNet<>::TransitionT &) { return cond2; };

    net.findTransition("T1")->autoFire(check_cond1);
    net.findTransition("T2")->autoFire(check_cond2);

    WHEN("Condition1 is enabled but Condition2 disabled") {
      cond1 = true;
      cond2 = false;
      net.tick();
      net.tick();  // should do nothing
      net.tick();  // should do nothing
      net.tick();  // should do nothing

      THEN("The net should do one transaction") {
        REQUIRE(changes["A"] == 1);
        REQUIRE(changes["B"] == 1);
        REQUIRE(changes["C"] == 3);
        REQUIRE(changes["D"] == 1);
        REQUIRE(changes["E"] == 1);
      }
    }

    WHEN("Condition1 and Condition2 are enabled") {
      cond1 = true;
      cond2 = true;

      net.tick();
      net.tick();

      THEN("The net must be valid after the second transaction") {
        REQUIRE(changes["A"] == 2);
        REQUIRE(changes["B"] == 3);
        REQUIRE(changes["C"] == 4);
        REQUIRE(changes["D"] == 0);
        REQUIRE(changes["E"] == 2);
      }
    }
  }
}

TEST_CASE("sptn::PetriNet - merge()", "[SPTN][PetriNet]") {
  GIVEN("Two compatible petri nets") {
    sptn::PetriNet<> net1;
    sptn::PetriNet<> net2;

    net1.addPlace("A", 1);
    net1.addPlace("B", 1);
    net1.addPlace("C", 1);
    net1.addTransition({"T1", {{"A", 1}}, {{"C", 1}}});

    net2.addPlace("D", 1);
    net2.addPlace("E", 1);
    net2.addPlace("F", 1);
    net2.addTransition({"T2", {{"E", 2}}, {{"F", 1}}});

    std::vector<sptn::PetriNet<>::TransitionSketch> interconnections = {
        {"T3", {{"A", 1}, {"B", 1}}, {{"E", 2}}}, {"T4", {{"F", 1}, {"D", 1}}, {{"C", 2}}}};

    WHEN("Net2 is merged into Net1") {
      net1.merge(std::move(net2), interconnections);

      THEN("Net1 contains all places and transition") {
        REQUIRE(net1.findPlace("A") != nullptr);
        REQUIRE(net1.findPlace("B") != nullptr);
        REQUIRE(net1.findPlace("C") != nullptr);
        REQUIRE(net1.findPlace("D") != nullptr);
        REQUIRE(net1.findPlace("E") != nullptr);
        REQUIRE(net1.findPlace("F") != nullptr);

        REQUIRE(net1.findTransition("T1") != nullptr);
        REQUIRE(net1.findTransition("T2") != nullptr);
        REQUIRE(net1.findTransition("T3") != nullptr);
        REQUIRE(net1.findTransition("T4") != nullptr);
      }
    }
  }

  GIVEN("Two incompatible petri nets (duplicate PlaceIDs") {
    sptn::PetriNet<> net1;
    sptn::PetriNet<> net2;

    net1.addPlace("A", 1);
    net2.addPlace("A", 1);

    WHEN("Net2 is merged into Net1") {
      THEN("An exception is thrown") {
        REQUIRE_THROWS_AS(net1.merge(std::move(net2), {}), std::invalid_argument);
      }
    }
  }

  GIVEN("Two incompatible petri nets (duplicate TransitionIDs") {
    sptn::PetriNet<> net1;
    sptn::PetriNet<> net2;

    net1.addPlace("A", 1);
    net1.addTransition({"T1", {}, {}});
    net2.addPlace("B", 1);
    net2.addTransition({"T1", {}, {}});

    WHEN("Net2 is merged into Net1") {
      THEN("An exception is thrown") {
        REQUIRE_THROWS_AS(net1.merge(std::move(net2), {}), std::invalid_argument);
      }
    }
  }

  GIVEN("Two incompatible petri nets (duplicate interconnections)") {
    sptn::PetriNet<> net1;
    sptn::PetriNet<> net2;

    net1.addPlace("A", 1);
    net1.addTransition({"T1", {}, {}});
    net2.addPlace("B", 1);
    net2.addTransition({"T2", {}, {}});

    WHEN("Net2 is merged into Net1") {
      THEN("An exception is thrown") {
        REQUIRE_THROWS_AS(net1.merge(std::move(net2), {{"T3", {{"A", 1}}, {{"C", 2}}}}),
                          std::invalid_argument);
      }
    }
  }

  GIVEN("Two incompatible petri nets (invalid interconnections)") {
    sptn::PetriNet<> net1;
    sptn::PetriNet<> net2;

    net1.addPlace("A", 1);
    net1.addTransition({"T1", {}, {}});
    net2.addPlace("B", 1);
    net2.addTransition({"T2", {}, {}});

    WHEN("Net2 is merged into Net1") {
      THEN("An exception is thrown") {
        REQUIRE_THROWS_AS(net1.merge(std::move(net2), {{"T3", {{"D", 1}}, {{"C", 2}}}}),
                          std::invalid_argument);
      }
    }
  }

  GIVEN("Two petri nets with callbacks with are merged") {
    sptn::PetriNet<> net1;
    sptn::PetriNet<> net2;

    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;

    net1.addPlace("A", 1);
    net1.addTransition({"T1", {}, {}});
    net1.findTransition("T1")->autoFire([&](const auto &) {
      flag1 = true;
      return true;
    });

    net2.addPlace("B", 1);
    net2.addTransition({"T2", {}, {}});
    net2.findTransition("T2")->autoFire([&](const auto &) {
      flag2 = true;
      return true;
    });
    net2.findPlace("B")->onChange([&](const auto &, auto) { flag3 = true; });

    net1.merge(std::move(net2), {{"T3", {{"A", 1}}, {{"B", 2}}}});

    WHEN("All callbacks are provoked") {
      net1.tick();
      net1.findTransition("T3")->fire();

      THEN("They were called") {
        REQUIRE(flag1);
        REQUIRE(flag2);
        REQUIRE(flag3);
      }
    }
  }
}

TEST_CASE("sptn::PetriNet deepTick()", "[sptn][PetriNet]") {
  GIVEN("A petri net") {
    sptn::PetriNet<> net;
    net.addPlace("E", 0);
    net.addPlace("D", 0);
    net.addPlace("C", 0);
    net.addPlace("B", 1);
    net.addPlace("A", 1);

    net.addTransition({"TAC", {{"A", 1}}, {{"C", 1}}})->autoFire();
    net.addTransition({"TBD", {{"B", 1}}, {{"D", 1}}})->autoFire();
    net.addTransition({"TCDE", {{"C", 1}, {"D", 1}}, {{"E", 1}}})->autoFire();

    WHEN("Deep ticking from A") {
      net.deepTick("A");

      THEN("A=0,B=1,C=1,D=0,E=0") {
        REQUIRE(net.findPlace("A")->getTokens() == 0);
        REQUIRE(net.findPlace("B")->getTokens() == 1);
        REQUIRE(net.findPlace("C")->getTokens() == 1);
        REQUIRE(net.findPlace("D")->getTokens() == 0);
        REQUIRE(net.findPlace("E")->getTokens() == 0);
      }
      WHEN("Deep ticking from B") {
        net.deepTick("B");

        THEN("A=0,B=0,C=0,D=0,E=1") {
          REQUIRE(net.findPlace("A")->getTokens() == 0);
          REQUIRE(net.findPlace("B")->getTokens() == 0);
          REQUIRE(net.findPlace("C")->getTokens() == 0);
          REQUIRE(net.findPlace("D")->getTokens() == 0);
          REQUIRE(net.findPlace("E")->getTokens() == 1);
        }
      }
    }
    WHEN("Deep ticking from B") {
      net.deepTick("B");

      THEN("A=1,B=0,C=0,D=1,E=0") {
        REQUIRE(net.findPlace("A")->getTokens() == 1);
        REQUIRE(net.findPlace("B")->getTokens() == 0);
        REQUIRE(net.findPlace("C")->getTokens() == 0);
        REQUIRE(net.findPlace("D")->getTokens() == 1);
        REQUIRE(net.findPlace("E")->getTokens() == 0);
      }
      WHEN("Deep ticking from A") {
        net.deepTick("A");

        THEN("A=0,B=0,C=0,D=0,E=1") {
          REQUIRE(net.findPlace("A")->getTokens() == 0);
          REQUIRE(net.findPlace("B")->getTokens() == 0);
          REQUIRE(net.findPlace("C")->getTokens() == 0);
          REQUIRE(net.findPlace("D")->getTokens() == 0);
          REQUIRE(net.findPlace("E")->getTokens() == 1);
        }
      }
    }
  }
}

TEST_CASE("sptn::PetriNet deepTick() cycle detection", "[sptn][PetriNet]") {
  GIVEN("A petri net with a cycle") {
    sptn::PetriNet<> net;
    net.addPlace("A", 1);
    net.addPlace("B", 0);
    net.addPlace("C", 0);
    net.addPlace("D", 0);
    net.addPlace("E", 0);

    net.addTransition({"AB", {{"A", 1}}, {{"B", 1}}})->autoFire();
    net.addTransition({"BC", {{"B", 1}}, {{"C", 1}}})->autoFire();
    net.addTransition({"CD", {{"C", 1}}, {{"D", 1}}})->autoFire();
    net.addTransition({"DEA", {{"D", 1}}, {{"E", 1}, {"A", 1}}})->autoFire();

    WHEN("deepTick() is called") {
      THEN("it throws a runtime error") {
        REQUIRE_THROWS_AS(net.deepTick("A"), std::runtime_error);
      }
    }
  }

  GIVEN("A diamond shaped petri net without a cycle") {
    sptn::PetriNet<> net;
    net.addPlace("A", 1);
    net.addPlace("B", 0);
    net.addPlace("C", 0);
    net.addPlace("D", 0);
    net.addPlace("E", 0);

    net.addTransition({"ABC", {{"A", 1}}, {{"B", 1}, {"C", 1}}})->autoFire();
    net.addTransition({"BD", {{"B", 1}}, {{"D", 1}}})->autoFire();
    net.addTransition({"CD", {{"C", 1}}, {{"D", 1}}})->autoFire();
    net.addTransition({"AE", {{"A", 1}}, {{"E", 1}}})->autoFire();

    WHEN("deepTick() is called") {
      THEN("it dows not trow a runtime error") { REQUIRE_NOTHROW(net.deepTick("A")); }
    }
  }
}