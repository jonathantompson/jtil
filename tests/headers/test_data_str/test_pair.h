//
//  test_pair.h
//
//  Created by Jonathan Tompson on 5/10/12.
//

#include <string>
#include "jtil/data_str/pair.h"
#include "test_unit/test_unit.h"

using jtil::data_str::Pair;

TEST(Pair, CreationCopyEquality) {
  Pair<std::string, int> pair1;
  pair1.first = std::string("HelloPartner");
  pair1.second = 42;
  Pair<std::string, int> pair2;
  pair2.first = std::string("HowdyPartner");
  pair2.second = -42;

  pair1 = pair2;
  EXPECT_EQ(pair1.first, pair2.first);
  EXPECT_EQ(pair1.second, pair2.second);
  EXPECT_TRUE(pair1 == pair2);
  pair1.first = std::string("HelloPartner");
  pair1.second = 42;
  EXPECT_NEQ(pair1.first, pair2.first);
  EXPECT_NEQ(pair1.second, pair2.second);
  EXPECT_FALSE(pair1 == pair2);
}