#include "catch.hpp"

#include "../lib/experiment.h"
using namespace plstim;

#include <tuple>
using std::tie;


TEST_CASE( "experiment", "[library]" ) {

  QQmlEngine engine;
  
  SECTION( "load" ) {

    QQmlComponent* comp;
    QObject* obj;
    
    // Empty experiment is invalid
    tie(comp, obj) = load_experiment(&engine, "tests/experiments/empty.qml", ".");
    REQUIRE( comp->isError() );
    REQUIRE( obj == nullptr );
    delete obj;


    tie(comp, obj) = load_experiment(&engine, "tests/experiments/nop.qml", ".");
    if (comp->isError())
      for (auto& err : comp->errors())
	qWarning() << err;
    REQUIRE( ! comp->isError() );
    REQUIRE( obj != NULL );
    delete obj;
  }
}
