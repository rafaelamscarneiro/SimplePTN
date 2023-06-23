// Copyright Open Logistics Foundation
// 
// Licensed under the Open Logistics Foundation License 1.3.
// For details on the licensing terms, see the LICENSE file.
// SPDX-License-Identifier: OLFL-1.3
// 

#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#include "SimplePTN/petri_net.hpp"

static std::mutex io_mutex;

class Supplier {
public:
  virtual void attachToNet(sptn::PetriNet<> &net) = 0;
};

template <uint32_t amount, uint32_t per_tick> class EnabledSupplier : public Supplier {
private:
  bool enabled_;

public:
  EnabledSupplier() : enabled_(false) {}

  virtual void attachToNet(sptn::PetriNet<> &main_net) override {
    // Generate name
    std::ostringstream oss;
    oss << amount << "@" << per_tick;
    std::string suffix = oss.str();
    std::string place_name = "supplier_stock_" + suffix;
    std::string transition_name = "supply_" + suffix;

    // add supplier stock
    main_net.addPlace(place_name, amount);

    // move per_tick from supplierstock to freight
    sptn::PetriNet<>::TransitionSketch supply{
        transition_name, {{place_name, per_tick}}, {{"freight", per_tick}}};
    main_net.addTransition(supply);

    // AutoFire when this->enabled_ == true
    main_net.findTransition(transition_name)->autoFire([&](const auto &transition) {
      (void)transition;  // We already know what transition is asking to fire
      return this->enabled_;
    });
  }

  void enable() { this->enabled_ = true; }
  void disable() { this->enabled_ = false; }
};

class PortNetManager {
private:
  sptn::PetriNet<> net_;

  // Important elements of the net (we want to manually fire those)
  std::shared_ptr<sptn::PetriNet<>::TransitionT> enter_a_;
  std::shared_ptr<sptn::PetriNet<>::TransitionT> enter_b_;
  std::shared_ptr<sptn::PetriNet<>::TransitionT> leave_a_;
  std::shared_ptr<sptn::PetriNet<>::TransitionT> leave_b_;

public:
  PortNetManager() {
    // no ship in port a
    net_.addPlace("port_a", 0);
    net_.addPlace("port_a_free", 1);

    // no ship in port b
    net_.addPlace("port_b", 0);
    net_.addPlace("port_b_free", 1);

    // Initially no freight to load
    net_.addPlace("freight", 0);

    // Enter/leave transitions
    sptn::PetriNet<>::TransitionSketch enter_a = {"enter_a", {{"port_a_free", 1}}, {{"port_a", 1}}};
    sptn::PetriNet<>::TransitionSketch enter_b = {"enter_b", {{"port_b_free", 1}}, {{"port_b", 1}}};
    sptn::PetriNet<>::TransitionSketch leave_a = {
        "leave_a", {{"port_a", 1}, {"freight", 2}}, {{"port_a_free", 1}}};
    sptn::PetriNet<>::TransitionSketch leave_b = {
        "leave_b", {{"port_b", 1}, {"freight", 3}}, {{"port_b_free", 1}}};

    // add sketches
    net_.addTransition(enter_a);
    net_.addTransition(enter_b);
    net_.addTransition(leave_a);
    net_.addTransition(leave_b);

    // Get a reference to the real transitions (for easier access)
    this->enter_a_ = net_.findTransition("enter_a");
    this->enter_b_ = net_.findTransition("enter_b");
    this->leave_a_ = net_.findTransition("leave_a");
    this->leave_b_ = net_.findTransition("leave_b");

    // Print when freight arrives or was taken
    net_.findPlace("freight")->onChange([&](const auto &place, uint32_t prev_tokens) {
      std::lock_guard lock(io_mutex);
      int64_t delta = (int64_t)place.getTokens() - (int64_t)prev_tokens;
      if (delta < 0) {
        std::cout << "Ship took " << -delta << " freights. "
                  << "Total: " << place.getTokens() << std::endl;
      } else {
        std::cout << "Supplier brought " << delta << " freights. "
                  << "Total: " << place.getTokens() << std::endl;
      }
    });

    // Print when ships enter or leave Port A or B
    auto on_port_change = [&](const auto &place, uint32_t) {
      std::lock_guard lock(io_mutex);
      if (place.getTokens() == 0) {
        std::cout << "Ship left " << place.getID() << std::endl;
      } else {
        std::cout << "Ship entered " << place.getID() << std::endl;
      }
    };
    net_.findPlace("port_a")->onChange(on_port_change);
    net_.findPlace("port_b")->onChange(on_port_change);
  }

  void addSupplier(Supplier &supplier) { supplier.attachToNet(this->net_); }

  bool tryEnterA() { return this->enter_a_->fire(); }
  bool tryEnterB() { return this->enter_b_->fire(); }
  bool tryLeaveA() { return this->leave_a_->fire(); }
  bool tryLeaveB() { return this->leave_b_->fire(); }
  bool canEnterA() { return this->enter_a_->ready(); }
  bool canEnterB() { return this->enter_b_->ready(); }
  bool canLeaveA() { return this->leave_a_->ready(); }
  bool canLeaveB() { return this->leave_b_->ready(); }

  void tick() { this->net_.tick(); }
};

int main() {
  PortNetManager port;
  EnabledSupplier<28, 2> s1;
  EnabledSupplier<10, 1> s2;
  port.addSupplier(s1);
  port.addSupplier(s2);

  // Start some threads
  // the ship_leave threads will concurrently try to leave ships from port a and b
  // the other threads will arrive new ships and freights

  std::thread ship_leaver_a([&] {
    using namespace std::chrono_literals;
    for (;;) {
      port.tryLeaveA();
      std::this_thread::sleep_for(1ms);
    }
  });

  std::thread ship_leaver_b([&] {
    using namespace std::chrono_literals;
    for (;;) {
      port.tryLeaveB();
      std::this_thread::sleep_for(1ms);
    }
  });

  std::thread ship_arrival([&] {
    using namespace std::chrono_literals;
    for (;;) {
      port.tryEnterA();
      port.tryEnterB();
      std::this_thread::sleep_for(1s);
    }
  });

  std::thread enabler_disabler_1([&] {
    using namespace std::chrono_literals;
    for (;;) {
      s1.enable();
      std::this_thread::sleep_for(3500ms);
      s1.disable();
      std::this_thread::sleep_for(6500ms);
    }
  });

  std::thread enabler_disabler_2([&] {
    using namespace std::chrono_literals;
    for (;;) {
      s2.enable();
      std::this_thread::sleep_for(2500ms);
      s2.disable();
      std::this_thread::sleep_for(5500ms);
    }
  });

  // Tick the net at 1Hz (this only effects the suppliers in this example)
  for (;;) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    port.tick();
  }
}