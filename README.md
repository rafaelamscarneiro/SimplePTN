# SimplePTN

This is a lightweight header-only [Petri Net](https://en.wikipedia.org/wiki/Petri_net) library, mainly
providing data structures.

## Features

- Templated data structures, allowing custom identifier and counting types
- Runtime extension
- Mutual exclusion for all operations on the net
- Passive and active driving
- Net observations


## Where to start?

When implementing a PTN, you first want to include the PetriNet header:

```c++
#include <SimplePTN/petri_net.hpp>
```

The next thing to do is to determine the id and counting types you want to use:

```c++
using MyPTN = sptn::PetriNet<MYID, MYCNT>;

// default: sptn::PetriNet<> = sptn::PetriNet<std::string, uint32_t>
```

The only thing left to do is to instantiate a net
and fill it with the given member functions.

```c++
MyPTN net;

net.addPlace(...);
net.addTransition();
net.findPlace(...)->(...);
net.findTransition(...)->(...);
net.tick();
net.merge(other net)
```

**Documentation of the functions is currently only
inside the doxygen annotations of the headers**

Running the net:

```c++
// Active firing
net->findTransition(transition_id)->fire();

// Passive firing
net->findTransition(transition_id)->autoFire(lambda_evaluate_condition);
net->tick();
```

## Examples

See [Examples](examples/README.md)

## Building

Dependencies:
- Catch2 (testing)

If building with CMake, use add_subdirectory() to
add the root folder if SimplePTN and add the dependency
SimplePTN.

Otherwise you can simply add the SimplePTN/include  include-directory.

## Testing

```sh
mkdir build
cd build
cmake ..
make simpleptn_test
ctest
```