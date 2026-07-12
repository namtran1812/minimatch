#include "minimatch/order_book.hpp"
#include <gtest/gtest.h>
#include <vector>
using namespace minimatch;
TEST(OrderBook,FifoAtSamePrice){OrderBook b(16);std::vector<Trade>t;b.on_trade([&](const Trade&x){t.push_back(x);});b.submit({1,1,Side::Sell,OrderType::Limit,100,5});b.submit({2,1,Side::Sell,OrderType::Limit,100,5});b.submit({3,2,Side::Buy,OrderType::Limit,100,7});ASSERT_EQ(t.size(),2);EXPECT_EQ(t[0].maker_order_id,1);EXPECT_EQ(t[0].quantity,5);EXPECT_EQ(t[1].maker_order_id,2);EXPECT_EQ(t[1].quantity,2);EXPECT_EQ(b.quantity_at(Side::Sell,100),3);}
TEST(OrderBook,BestPriceBeforeTime){OrderBook b(16);std::vector<Trade>t;b.on_trade([&](const Trade&x){t.push_back(x);});b.submit({1,1,Side::Sell,OrderType::Limit,101,5});b.submit({2,1,Side::Sell,OrderType::Limit,100,5});b.submit({3,2,Side::Buy,OrderType::Market,0,6});ASSERT_EQ(t.size(),2);EXPECT_EQ(t[0].maker_order_id,2);EXPECT_EQ(t[1].maker_order_id,1);}
TEST(OrderBook,IOCDoesNotRest){OrderBook b(8);b.submit({1,1,Side::Buy,OrderType::IOC,99,10});EXPECT_EQ(b.active_orders(),0);EXPECT_EQ(b.best_bid(),0);}
TEST(OrderBook,MarketDoesNotRest){OrderBook b(8);b.submit({1,1,Side::Buy,OrderType::Market,0,10});EXPECT_EQ(b.active_orders(),0);}
TEST(OrderBook,CancelRemovesOrder){OrderBook b(8);b.submit({1,1,Side::Buy,OrderType::Limit,99,10});EXPECT_TRUE(b.cancel({1,1}));EXPECT_EQ(b.active_orders(),0);EXPECT_EQ(b.best_bid(),0);}
TEST(OrderBook,PartialFillRestsLimitRemainder){OrderBook b(8);b.submit({1,1,Side::Sell,OrderType::Limit,100,4});b.submit({2,2,Side::Buy,OrderType::Limit,100,10});EXPECT_EQ(b.quantity_at(Side::Buy,100),6);}
TEST(OrderBook,DeterministicStateHash){OrderBook a(16),b(16);for(auto*x:{&a,&b}){x->submit({1,1,Side::Buy,OrderType::Limit,99,10});x->submit({2,1,Side::Sell,OrderType::Limit,101,7});x->submit({3,1,Side::Buy,OrderType::IOC,101,3});}EXPECT_EQ(a.state_hash(),b.state_hash());EXPECT_EQ(a.sequence(),b.sequence());}
TEST(OrderBook,RejectDuplicate){OrderBook b(8);b.submit({1,1,Side::Buy,OrderType::Limit,99,10});EXPECT_FALSE(b.submit({1,1,Side::Buy,OrderType::Limit,98,2}));}
