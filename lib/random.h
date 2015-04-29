// src/random.h – Random variables utils (from necomi)
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <random>

namespace plstim {

/**
 * Implements the SeedSequence concept for std::random_device.
 */
struct RandomDevSeedSequence
{
  typedef std::uint32_t result_type;
  RandomDevSeedSequence() {}

  template <typename InputIterator>
  RandomDevSeedSequence(InputIterator ia, InputIterator ib)
  {
    (void) ia; (void) ib;
  }

  RandomDevSeedSequence(std::initializer_list<std::uint32_t> il)
  {
    (void) il;
  }

  template <typename InputIterator>
  void generate(InputIterator ia, InputIterator ib) {
    for (; ia != ib; ++ia)
      *ia = m_dist(m_dev);
  }

  size_t size() { return 0; }

  template <typename OutputIterator>
  void param(OutputIterator ob)
  {
    (void) ob;
  }
protected:
  std::random_device m_dev;
  std::uniform_int_distribution<std::uint32_t> m_dist;
};

} // namespace plstim
 
// Local variables:
// mode: c++
// End:
