#include "minimatch/order_book.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace minimatch {
namespace {

struct BookCapture {
  std::vector<Trade> trades;
  std::vector<ExecutionReport> reports;

  void attach(
      OrderBook& book
  ) {
    book.on_trade(
        [this](
            const Trade& trade
        ) {
          trades.push_back(
              trade
          );
        }
    );

    book.on_report(
        [this](
            const ExecutionReport&
                report
        ) {
          reports.push_back(
              report
          );
        }
    );
  }

  bool has_report(
      OrderId order_id,
      ExecutionReport::Status status
  ) const {
    for (
        const auto& report :
        reports
    ) {
      if (
          report.order_id ==
              order_id &&
          report.status ==
              status
      ) {
        return true;
      }
    }

    return false;
  }
};

TEST(
    SelfTradePrevention,
    CancelNewestCancelsIncoming
) {
  OrderBook book;

  book.set_stp_policy(
      SelfTradePreventionPolicy::
          CancelNewest
  );

  BookCapture capture;
  capture.attach(book);

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 1,
              .client_id = 42,
              .side = Side::Buy,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 2,
              .client_id = 42,
              .side = Side::Sell,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  EXPECT_TRUE(
      capture.trades.empty()
  );

  EXPECT_TRUE(
      capture.has_report(
          2,
          ExecutionReport::Status::
              Cancelled
      )
  );

  EXPECT_EQ(
      book.quantity_at(
          Side::Buy,
          10000
      ),
      100U
  );

  EXPECT_EQ(
      book.quantity_at(
          Side::Sell,
          10000
      ),
      0U
  );
}

TEST(
    SelfTradePrevention,
    CancelOldestCancelsResting
) {
  OrderBook book;

  book.set_stp_policy(
      SelfTradePreventionPolicy::
          CancelOldest
  );

  BookCapture capture;
  capture.attach(book);

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 1,
              .client_id = 42,
              .side = Side::Buy,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 2,
              .client_id = 42,
              .side = Side::Sell,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  EXPECT_TRUE(
      capture.trades.empty()
  );

  EXPECT_TRUE(
      capture.has_report(
          1,
          ExecutionReport::Status::
              Cancelled
      )
  );

  EXPECT_EQ(
      book.quantity_at(
          Side::Buy,
          10000
      ),
      0U
  );

  EXPECT_EQ(
      book.quantity_at(
          Side::Sell,
          10000
      ),
      100U
  );
}

TEST(
    SelfTradePrevention,
    CancelBothCancelsBothOrders
) {
  OrderBook book;

  book.set_stp_policy(
      SelfTradePreventionPolicy::
          CancelBoth
  );

  BookCapture capture;
  capture.attach(book);

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 1,
              .client_id = 42,
              .side = Side::Buy,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 2,
              .client_id = 42,
              .side = Side::Sell,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  EXPECT_TRUE(
      capture.trades.empty()
  );

  EXPECT_TRUE(
      capture.has_report(
          1,
          ExecutionReport::Status::
              Cancelled
      )
  );

  EXPECT_TRUE(
      capture.has_report(
          2,
          ExecutionReport::Status::
              Cancelled
      )
  );

  EXPECT_EQ(
      book.active_orders(),
      0U
  );
}

TEST(
    SelfTradePrevention,
    NoneAllowsSelfTrade
) {
  OrderBook book;

  book.set_stp_policy(
      SelfTradePreventionPolicy::None
  );

  BookCapture capture;
  capture.attach(book);

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 1,
              .client_id = 42,
              .side = Side::Buy,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  ASSERT_TRUE(
      book.submit(
          OrderRequest{
              .order_id = 2,
              .client_id = 42,
              .side = Side::Sell,
              .type = OrderType::Limit,
              .price = 10000,
              .quantity = 100
          }
      )
  );

  ASSERT_EQ(
      capture.trades.size(),
      1U
  );

  EXPECT_EQ(
      capture.trades[0].quantity,
      100U
  );

  EXPECT_EQ(
      book.active_orders(),
      0U
  );
}

}  // namespace
}  // namespace minimatch
