#include "catch.hpp"

#include "../lib/experiment.h"
using namespace plstim;


TEST_CASE( "experiment", "[library]" ) {

  QQmlEngine engine;
  
  SECTION( "load" ) {

    // Empty experiment is invalid
    auto c1 = load_experiment(&engine, "tests/experiments/empty.qml", ".");
    REQUIRE( c1->isError() );


    auto c2 = load_experiment(&engine, "tests/experiments/nop.qml", ".");
    if (c2->isError())
      for (auto& err : c2->errors())
	qWarning() << err;
    REQUIRE( ! c2->isError() );
  }
}
