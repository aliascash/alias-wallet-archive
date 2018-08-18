#include <QTest>
#include <QObject>

#define BOOST_TEST_MODULE Spectre_tests
#include <boost/test/unit_test.hpp>

#include "uritests.h"

#ifdef SPECTRE_QT_TEST
// This is all you need to run all the tests
int RunQtTests()
{
    bool fInvalid = false;

    URITests test1;
    if (QTest::qExec(&test1) != 0)
        fInvalid = true;

    return fInvalid;
}

BOOST_AUTO_TEST_CASE(universeInOrder) {
    BOOST_CHECK(RunQtTests() == false);
}
#endif
