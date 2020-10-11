// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
//
// SPDX-License-Identifier: MIT

#include <iostream>
#include <boost/test/unit_test.hpp>

#include <boost/atomic.hpp>

BOOST_AUTO_TEST_SUITE(basic_tests)

BOOST_AUTO_TEST_CASE(is_lock_free)
{
    boost::atomic<int> i;
    std::cout << i.is_lock_free() << '\n';
    BOOST_CHECK(i.is_lock_free());
}

BOOST_AUTO_TEST_SUITE_END()
