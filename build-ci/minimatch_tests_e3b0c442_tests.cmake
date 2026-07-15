add_test([=[OrderBook.FifoAtSamePrice]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.FifoAtSamePrice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.FifoAtSamePrice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:5]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.BestPriceBeforeTime]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.BestPriceBeforeTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.BestPriceBeforeTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:6]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.IOCDoesNotRest]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.IOCDoesNotRest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.IOCDoesNotRest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:7]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.MarketDoesNotRest]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.MarketDoesNotRest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.MarketDoesNotRest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:8]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.CancelRemovesOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.CancelRemovesOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.CancelRemovesOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:9]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.PartialFillRestsLimitRemainder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.PartialFillRestsLimitRemainder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.PartialFillRestsLimitRemainder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:10]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.DeterministicStateHash]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.DeterministicStateHash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.DeterministicStateHash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:11]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OrderBook.RejectDuplicate]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OrderBook.RejectDuplicate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OrderBook.RejectDuplicate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/order_book_test.cpp:12]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedOrderBook.FillOrKillRejectsWithoutMutation]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedOrderBook.FillOrKillRejectsWithoutMutation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedOrderBook.FillOrKillRejectsWithoutMutation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:7]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedOrderBook.FillOrKillExecutesCompletely]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedOrderBook.FillOrKillExecutesCompletely]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedOrderBook.FillOrKillExecutesCompletely]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:16]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedOrderBook.PostOnlyRejectsCrossingOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedOrderBook.PostOnlyRejectsCrossingOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedOrderBook.PostOnlyRejectsCrossingOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:27]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedOrderBook.QuantityReductionPreservesPriority]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedOrderBook.QuantityReductionPreservesPriority]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedOrderBook.QuantityReductionPreservesPriority]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:35]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedOrderBook.PriceChangeLosesPriority]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedOrderBook.PriceChangeLosesPriority]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedOrderBook.PriceChangeLosesPriority]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:49]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedOrderBook.DepthIncludesOrderCount]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedOrderBook.DepthIncludesOrderCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedOrderBook.DepthIncludesOrderCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:86]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedExchange.MultiSymbolIsolation]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedExchange.MultiSymbolIsolation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedExchange.MultiSymbolIsolation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:61]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AdvancedExchange.SnapshotRoundTrip]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=AdvancedExchange.SnapshotRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AdvancedExchange.SnapshotRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/advanced_test.cpp:72]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RiskEngine.RejectsOversizedOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RiskEngine.RejectsOversizedOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RiskEngine.RejectsOversizedOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/system_test.cpp:7]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RiskEngine.EnforcesPositionLimit]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RiskEngine.EnforcesPositionLimit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RiskEngine.EnforcesPositionLimit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/system_test.cpp:12]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RiskEngine.DailyLossTriggersKillSwitch]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RiskEngine.DailyLossTriggersKillSwitch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RiskEngine.DailyLossTriggersKillSwitch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/system_test.cpp:19]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Analytics.ComputesDrawdownAndWinRate]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=Analytics.ComputesDrawdownAndWinRate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Analytics.ComputesDrawdownAndWinRate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/system_test.cpp:26]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Options.PutCallParityAndImpliedVolatility]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=Options.PutCallParityAndImpliedVolatility]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Options.PutCallParityAndImpliedVolatility]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/quant_test.cpp:10]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Options.MonteCarloConvergesNearBlackScholes]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=Options.MonteCarloConvergesNearBlackScholes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Options.MonteCarloConvergesNearBlackScholes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/quant_test.cpp:22]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Execution.SchedulesConserveQuantity]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=Execution.SchedulesConserveQuantity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Execution.SchedulesConserveQuantity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/quant_test.cpp:29]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[QueueModel.TradesConsumeAheadBeforeOwnOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=QueueModel.TradesConsumeAheadBeforeOwnOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[QueueModel.TradesConsumeAheadBeforeOwnOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/quant_test.cpp:40]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pairs.RecoversSyntheticHedgeRatio]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=Pairs.RecoversSyntheticHedgeRatio]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pairs.RecoversSyntheticHedgeRatio]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/quant_test.cpp:50]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SpscRing.PreservesFifoOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SpscRing.PreservesFifoOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SpscRing.PreservesFifoOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/pipeline_test.cpp:6]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pipeline.ProcessesCommandsDeterministically]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=Pipeline.ProcessesCommandsDeterministically]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pipeline.ProcessesCommandsDeterministically]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/pipeline_test.cpp:10]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.BuyUsesLowestEffectiveAskFirst]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.BuyUsesLowestEffectiveAskFirst]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.BuyUsesLowestEffectiveAskFirst]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:38]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.ConsumesMultipleLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.ConsumesMultipleLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.ConsumesMultipleLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:64]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.SplitsAcrossVenuesAndLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.SplitsAcrossVenuesAndLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.SplitsAcrossVenuesAndLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:94]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.SellUsesHighestBidLevelsFirst]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.SellUsesHighestBidLevelsFirst]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.SellUsesHighestBidLevelsFirst]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:133]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.ReportsPartialDepthRoute]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.ReportsPartialDepthRoute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.ReportsPartialDepthRoute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:165]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.IgnoresInvalidLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.IgnoresInvalidLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.IgnoresInvalidLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:186]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SmartOrderRouter.LatencyPenaltyCanChangeVenue]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=SmartOrderRouter.LatencyPenaltyCanChangeVenue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SmartOrderRouter.LatencyPenaltyCanChangeVenue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:208]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.BuyLimitPriceStopsExpensiveLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.BuyLimitPriceStopsExpensiveLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.BuyLimitPriceStopsExpensiveLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:238]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.SellLimitPriceStopsCheapLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.SellLimitPriceStopsCheapLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.SellLimitPriceStopsCheapLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:265]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.MaximumVenueCountIsEnforced]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.MaximumVenueCountIsEnforced]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.MaximumVenueCountIsEnforced]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:292]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.AllOrNoneClearsPartialRoute]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.AllOrNoneClearsPartialRoute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.AllOrNoneClearsPartialRoute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:340]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.BuySlippageLimitStopsWorseLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.BuySlippageLimitStopsWorseLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.BuySlippageLimitStopsWorseLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:368]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.SellSlippageLimitStopsWorseLevels]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.SellSlippageLimitStopsWorseLevels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.SellSlippageLimitStopsWorseLevels]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:398]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterConstraints.RejectsInvalidControls]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterConstraints.RejectsInvalidControls]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterConstraints.RejectsInvalidControls]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_test.cpp:428]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterMarketData.ConvertsBookToVenueQuotes]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterMarketData.ConvertsBookToVenueQuotes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterMarketData.ConvertsBookToVenueQuotes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_market_data_test.cpp:7]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RouterMarketData.RoutesUsingMarketData]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RouterMarketData.RoutesUsingMarketData]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RouterMarketData.RoutesUsingMarketData]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/router_market_data_test.cpp:44]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.MarketUsesSingleImmediateSlice]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.MarketUsesSingleImmediateSlice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.MarketUsesSingleImmediateSlice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:28]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.TWAPUsesEqualTimeSlices]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.TWAPUsesEqualTimeSlices]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.TWAPUsesEqualTimeSlices]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:41]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.VWAPFollowsVolumeProfile]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.VWAPFollowsVolumeProfile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.VWAPFollowsVolumeProfile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:62]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.POVRespectsParticipationRate]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.POVRespectsParticipationRate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.POVRespectsParticipationRate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:88]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.POVStopsWhenParentQuantityIsFilled]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.POVStopsWhenParentQuantityIsFilled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.POVStopsWhenParentQuantityIsFilled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:113]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.IcebergUsesDisplayedQuantity]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.IcebergUsesDisplayedQuantity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.IcebergUsesDisplayedQuantity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:136]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.BackwardCompatibleTWAPOverload]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.BackwardCompatibleTWAPOverload]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.BackwardCompatibleTWAPOverload]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:156]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.RejectsInvalidCommonSettings]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.RejectsInvalidCommonSettings]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.RejectsInvalidCommonSettings]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:168]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionAlgo.RejectsInvalidAlgorithmSettings]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionAlgo.RejectsInvalidAlgorithmSettings]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionAlgo.RejectsInvalidAlgorithmSettings]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_algo_test.cpp:206]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionReplay.BuildsDeterministicTimeline]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionReplay.BuildsDeterministicTimeline]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionReplay.BuildsDeterministicTimeline]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_replay_test.cpp:58]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionReplay.PreservesChildOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionReplay.PreservesChildOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionReplay.PreservesChildOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_replay_test.cpp:92]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionReplay.AccumulatesLatency]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionReplay.AccumulatesLatency]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionReplay.AccumulatesLatency]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_replay_test.cpp:105]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionReplay.RepresentsRejectedChild]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionReplay.RepresentsRejectedChild]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionReplay.RepresentsRejectedChild]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_replay_test.cpp:127]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionReplay.RepresentsPartialFill]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionReplay.RepresentsPartialFill]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionReplay.RepresentsPartialFill]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_replay_test.cpp:165]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionReplay.EmptyChildrenStillProducesParentEvents]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionReplay.EmptyChildrenStillProducesParentEvents]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionReplay.EmptyChildrenStillProducesParentEvents]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_replay_test.cpp:193]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.SortsEventsByTimestamp]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.SortsEventsByTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.SortsEventsByTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:33]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.IteratesDeterministically]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.IteratesDeterministically]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.IteratesDeterministically]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:68]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.SeeksByIndex]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.SeeksByIndex]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.SeeksByIndex]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:108]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.SeeksByTimestamp]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.SeeksByTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.SeeksByTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:139]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.CalculatesStatistics]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.CalculatesStatistics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.CalculatesStatistics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:168]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.LoadsCsvFile]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.LoadsCsvFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.LoadsCsvFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:205]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.RejectsInvalidCsvHeader]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.RejectsInvalidCsvHeader]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.RejectsInvalidCsvHeader]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:235]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketReplay.RejectsCrossedQuote]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketReplay.RejectsCrossedQuote]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketReplay.RejectsCrossedQuote]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_replay_test.cpp:251]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.AppliesSnapshot]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.AppliesSnapshot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.AppliesSnapshot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:35]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.AppliesIncrementalUpdate]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.AppliesIncrementalUpdate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.AppliesIncrementalUpdate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:61]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.DeletesPriceLevel]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.DeletesPriceLevel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.DeletesPriceLevel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:91]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.DetectsSequenceGap]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.DetectsSequenceGap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.DetectsSequenceGap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:120]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.RequiresSnapshotBeforeIncrementals]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.RequiresSnapshotBeforeIncrementals]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.RequiresSnapshotBeforeIncrementals]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:149]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.ReturnsRequestedDepth]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.ReturnsRequestedDepth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.ReturnsRequestedDepth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:171]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.RejectsCrossedSnapshot]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.RejectsCrossedSnapshot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.RejectsCrossedSnapshot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:188]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.BuildsConsolidatedBbo]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.BuildsConsolidatedBbo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.BuildsConsolidatedBbo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:202]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.ExcludesUnsynchronizedVenue]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.ExcludesUnsynchronizedVenue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.ExcludesUnsynchronizedVenue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:258]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.ListsVenuesForSymbol]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.ListsVenuesForSymbol]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.ListsVenuesForSymbol]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:297]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.AcceptsSnapshotSequenceZero]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.AcceptsSnapshotSequenceZero]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.AcceptsSnapshotSequenceZero]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:317]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.AppliesMultipleUpdatesWithOneSequence]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.AppliesMultipleUpdatesWithOneSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.AppliesMultipleUpdatesWithOneSequence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:331]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketData.RejectsCrossedConsolidatedBbo]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketData.RejectsCrossedConsolidatedBbo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketData.RejectsCrossedConsolidatedBbo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_data_test.cpp:393]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketRecorder.WritesAndReadsSnapshot]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketRecorder.WritesAndReadsSnapshot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketRecorder.WritesAndReadsSnapshot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_recorder_test.cpp:93]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketRecorder.WritesAndReadsUpdateBatch]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketRecorder.WritesAndReadsUpdateBatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketRecorder.WritesAndReadsUpdateBatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_recorder_test.cpp:162]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketRecorder.ReplaysDeterministically]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketRecorder.ReplaysDeterministically]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketRecorder.ReplaysDeterministically]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_recorder_test.cpp:207]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketRecorder.SupportsTimestampRange]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketRecorder.SupportsTimestampRange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketRecorder.SupportsTimestampRange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_recorder_test.cpp:318]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketRecorder.RewindsReader]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketRecorder.RewindsReader]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketRecorder.RewindsReader]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_recorder_test.cpp:360]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MarketRecorder.RejectsCorruptHeader]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=MarketRecorder.RejectsCorruptHeader]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MarketRecorder.RejectsCorruptHeader]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/market_recorder_test.cpp:390]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CoinbaseLevel2.ParsesSnapshot]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=CoinbaseLevel2.ParsesSnapshot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CoinbaseLevel2.ParsesSnapshot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/coinbase_l2_test.cpp:13]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CoinbaseLevel2.ParsesIncrementalUpdates]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=CoinbaseLevel2.ParsesIncrementalUpdates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CoinbaseLevel2.ParsesIncrementalUpdates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/coinbase_l2_test.cpp:77]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CoinbaseLevel2.ParsesHeartbeat]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=CoinbaseLevel2.ParsesHeartbeat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CoinbaseLevel2.ParsesHeartbeat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/coinbase_l2_test.cpp:139]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CoinbaseLevel2.IgnoresUnsupportedChannel]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=CoinbaseLevel2.IgnoresUnsupportedChannel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CoinbaseLevel2.IgnoresUnsupportedChannel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/coinbase_l2_test.cpp:162]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CoinbaseLevel2.BuildsSubscriptions]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=CoinbaseLevel2.BuildsSubscriptions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CoinbaseLevel2.BuildsSubscriptions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/coinbase_l2_test.cpp:176]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CoinbaseLevel2.EnvelopeSequenceGapsDoNotAffectNormalizedBook]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=CoinbaseLevel2.EnvelopeSequenceGapsDoNotAffectNormalizedBook]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CoinbaseLevel2.EnvelopeSequenceGapsDoNotAffectNormalizedBook]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/coinbase_l2_test.cpp:197]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.RunsTwapAgainstHistoricalQuotes]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.RunsTwapAgainstHistoricalQuotes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.RunsTwapAgainstHistoricalQuotes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:63]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.ReportsPartialFill]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.ReportsPartialFill]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.ReportsPartialFill]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:96]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.CalculatesMarketVwap]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.CalculatesMarketVwap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.CalculatesMarketVwap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:120]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.RejectsMissingSymbol]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.RejectsMissingSymbol]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.RejectsMissingSymbol]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:141]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.RejectsInvalidQuantity]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.RejectsInvalidQuantity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.RejectsInvalidQuantity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:162]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.CreatesParentAndChildOrders]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.CreatesParentAndChildOrders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.CreatesParentAndChildOrders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:184]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.CancelsIncompleteParent]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.CancelsIncompleteParent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.CancelsIncompleteParent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:226]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.FillsReferenceOmsRecords]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.FillsReferenceOmsRecords]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.FillsReferenceOmsRecords]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:264]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.RejectsChildWhenKillSwitchIsActive]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.RejectsChildWhenKillSwitchIsActive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.RejectsChildWhenKillSwitchIsActive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:311]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.RejectsChildAboveQuantityLimit]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.RejectsChildAboveQuantityLimit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.RejectsChildAboveQuantityLimit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:357]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BacktestEngine.AcceptsChildWithinRiskLimits]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=BacktestEngine.AcceptsChildWithinRiskLimits]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BacktestEngine.AcceptsChildWithinRiskLimits]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/backtest_engine_test.cpp:392]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.FullyExecutesRoute]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.FullyExecutesRoute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.FullyExecutesRoute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:48]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.SupportsPartialFills]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.SupportsPartialFills]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.SupportsPartialFills]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:68]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.ComputesAverageFillPrice]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.ComputesAverageFillPrice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.ComputesAverageFillPrice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:88]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.ComputesFees]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.ComputesFees]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.ComputesFees]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:101]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.RejectsInvalidFillRatio]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.RejectsInvalidFillRatio]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.RejectsInvalidFillRatio]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:115]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.RejectsAllChildren]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.RejectsAllChildren]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.RejectsAllChildren]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:128]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.SameSeedProducesSameResults]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.SameSeedProducesSameResults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.SameSeedProducesSameResults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:162]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.AddsConfiguredLatency]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.AddsConfiguredLatency]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.AddsConfiguredLatency]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:209]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RoutedExecutionEngine.RejectsInvalidSimulationSettings]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=RoutedExecutionEngine.RejectsInvalidSimulationSettings]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RoutedExecutionEngine.RejectsInvalidSimulationSettings]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_engine_test.cpp:239]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.CreatesParentOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.CreatesParentOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.CreatesParentOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:14]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.CreatesChildAndMovesParentToWorking]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.CreatesChildAndMovesParentToWorking]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.CreatesChildAndMovesParentToWorking]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:43]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.RecordsPartialChildFill]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.RecordsPartialChildFill]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.RecordsPartialChildFill]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:86]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.CompletesParentAcrossMultipleChildren]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.CompletesParentAcrossMultipleChildren]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.CompletesParentAcrossMultipleChildren]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:166]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.RejectsOverfill]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.RejectsOverfill]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.RejectsOverfill]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:245]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.CancelsParentAndWorkingChildren]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.CancelsParentAndWorkingChildren]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.CancelsParentAndWorkingChildren]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:281]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.RejectsChildAndUpdatesParent]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.RejectsChildAndUpdatesParent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.RejectsChildAndUpdatesParent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:338]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.ReturnsChildrenAndFillsForParent]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.ReturnsChildrenAndFillsForParent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.ReturnsChildrenAndFillsForParent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:377]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OMS.RejectsInvalidParentAndChildRequests]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=OMS.RejectsInvalidParentAndChildRequests]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OMS.RejectsInvalidParentAndChildRequests]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/oms_test.cpp:423]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.EncodesAndParsesLogon]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.EncodesAndParsesLogon]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.EncodesAndParsesLogon]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:15]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.EncodesNewOrderSingle]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.EncodesNewOrderSingle]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.EncodesNewOrderSingle]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:57]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.RejectsInvalidChecksum]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.RejectsInvalidChecksum]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.RejectsInvalidChecksum]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:107]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.RejectsInvalidBodyLength]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.RejectsInvalidBodyLength]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.RejectsInvalidBodyLength]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:143]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.SupportsCancelAndExecutionReport]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.SupportsCancelAndExecutionReport]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.SupportsCancelAndExecutionReport]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:207]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.PreparesPossDupResend]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.PreparesPossDupResend]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.PreparesPossDupResend]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:253]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixCodec.CreatesSequenceResetGapFill]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixCodec.CreatesSequenceResetGapFill]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixCodec.CreatesSequenceResetGapFill]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_test.cpp:303]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.AcceptsLogonAndBecomesActive]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.AcceptsLogonAndBecomesActive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.AcceptsLogonAndBecomesActive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:29]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.RejectsIncorrectSequenceNumber]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.RejectsIncorrectSequenceNumber]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.RejectsIncorrectSequenceNumber]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:63]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.RejectsApplicationMessageBeforeLogon]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.RejectsApplicationMessageBeforeLogon]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.RejectsApplicationMessageBeforeLogon]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:86]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.AcceptsApplicationMessageAfterLogon]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.AcceptsApplicationMessageAfterLogon]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.AcceptsApplicationMessageAfterLogon]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:108]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.RespondsToTestRequestWithHeartbeat]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.RespondsToTestRequestWithHeartbeat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.RespondsToTestRequestWithHeartbeat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:138]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.HandlesLogout]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.HandlesLogout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.HandlesLogout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:182]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.DetectsHeartbeatAndTestRequestDeadlines]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.DetectsHeartbeatAndTestRequestDeadlines]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.DetectsHeartbeatAndTestRequestDeadlines]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:217]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.RejectsCompIdMismatch]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.RejectsCompIdMismatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.RejectsCompIdMismatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:271]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.OutgoingMessagesIncrementSequence]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.OutgoingMessagesIncrementSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.OutgoingMessagesIncrementSequence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:295]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.ResetRestoresInitialState]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.ResetRestoresInitialState]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.ResetRestoresInitialState]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:312]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.SequencesOutboundExecutionReports]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.SequencesOutboundExecutionReports]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.SequencesOutboundExecutionReports]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:335]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.RestoresPersistentSequenceState]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.RestoresPersistentSequenceState]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.RestoresPersistentSequenceState]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:389]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.RequestsResendForSequenceGap]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.RequestsResendForSequenceGap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.RequestsResendForSequenceGap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:422]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.AcceptsValidResendRequest]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.AcceptsValidResendRequest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.AcceptsValidResendRequest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:467]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixSession.AppliesSequenceReset]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixSession.AppliesSequenceReset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixSession.AppliesSequenceReset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_session_test.cpp:508]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixStore.SavesAndLoadsSessionSequenceState]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixStore.SavesAndLoadsSessionSequenceState]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixStore.SavesAndLoadsSessionSequenceState]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_store_test.cpp:23]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixStore.ReturnsDefaultsForUnknownSession]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixStore.ReturnsDefaultsForUnknownSession]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixStore.ReturnsDefaultsForUnknownSession]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_store_test.cpp:77]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixStore.PersistsInboundAndOutboundMessages]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixStore.PersistsInboundAndOutboundMessages]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixStore.PersistsInboundAndOutboundMessages]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_store_test.cpp:96]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixStore.LoadsOutboundMessagesFromSequence]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixStore.LoadsOutboundMessagesFromSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixStore.LoadsOutboundMessagesFromSequence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_store_test.cpp:140]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixStore.UpdatesExistingSessionSnapshot]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixStore.UpdatesExistingSessionSnapshot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixStore.UpdatesExistingSessionSnapshot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_store_test.cpp:182]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixGateway.CreatesOmsOrdersFromNewOrderSingle]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixGateway.CreatesOmsOrdersFromNewOrderSingle]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixGateway.CreatesOmsOrdersFromNewOrderSingle]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_gateway_test.cpp:32]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixGateway.RejectsDuplicateClientOrderId]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixGateway.RejectsDuplicateClientOrderId]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixGateway.RejectsDuplicateClientOrderId]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_gateway_test.cpp:71]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixGateway.CancelsExistingOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixGateway.CancelsExistingOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixGateway.CancelsExistingOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_gateway_test.cpp:96]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixGateway.RejectsUnknownCancelOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixGateway.RejectsUnknownCancelOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixGateway.RejectsUnknownCancelOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_gateway_test.cpp:146]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixGateway.RejectsMissingRequiredFields]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixGateway.RejectsMissingRequiredFields]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixGateway.RejectsMissingRequiredFields]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_gateway_test.cpp:171]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FixGateway.SupportsMarketOrders]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=FixGateway.SupportsMarketOrders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FixGateway.SupportsMarketOrders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/fix_gateway_test.cpp:192]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.AcceptsValidOrder]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.AcceptsValidOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.AcceptsValidOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:24]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.KillSwitchRejectsOrders]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.KillSwitchRejectsOrders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.KillSwitchRejectsOrders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:45]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsParentQuantityLimit]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsParentQuantityLimit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsParentQuantityLimit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:66]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsChildQuantityLimit]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsChildQuantityLimit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsChildQuantityLimit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:82]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsNotionalLimit]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsNotionalLimit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsNotionalLimit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:98]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsParticipationLimit]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsParticipationLimit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsParticipationLimit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:114]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsBuyAbovePriceCollar]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsBuyAbovePriceCollar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsBuyAbovePriceCollar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:130]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsSellBelowPriceCollar]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsSellBelowPriceCollar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsSellBelowPriceCollar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:149]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.AllowsFavorablePriceMovement]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.AllowsFavorablePriceMovement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.AllowsFavorablePriceMovement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:169]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsInvalidConfiguration]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsInvalidConfiguration]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsInvalidConfiguration]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:192]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.RejectsInvalidOrderValues]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.RejectsInvalidOrderValues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.RejectsInvalidOrderValues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:212]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionRisk.ZeroParticipationLimitDisablesCheck]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionRisk.ZeroParticipationLimitDisablesCheck]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionRisk.ZeroParticipationLimitDisablesCheck]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_risk_test.cpp:228]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionStore.CreatesSchema]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionStore.CreatesSchema]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionStore.CreatesSchema]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_store_test.cpp:71]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionStore.PersistsRouteAndChildren]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionStore.PersistsRouteAndChildren]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionStore.PersistsRouteAndChildren]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_store_test.cpp:86]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionStore.PersistsMultipleExecutions]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionStore.PersistsMultipleExecutions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionStore.PersistsMultipleExecutions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_store_test.cpp:117]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionStore.LoadsExecutionById]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionStore.LoadsExecutionById]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionStore.LoadsExecutionById]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_store_test.cpp:149]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionStore.ReturnsRecentExecutionsNewestFirst]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionStore.ReturnsRecentExecutionsNewestFirst]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionStore.ReturnsRecentExecutionsNewestFirst]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_store_test.cpp:178]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExecutionStore.MissingExecutionReturnsEmpty]=]  /Users/namtran/minimatch/build-ci/minimatch_tests [==[--gtest_filter=ExecutionStore.MissingExecutionReturnsEmpty]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExecutionStore.MissingExecutionReturnsEmpty]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/Users/namtran/minimatch/tests/execution_store_test.cpp:211]==]
    WORKING_DIRECTORY [==[/Users/namtran/minimatch/build-ci]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
set(minimatch_tests_TESTS [==[OrderBook.FifoAtSamePrice]==] [==[OrderBook.BestPriceBeforeTime]==] [==[OrderBook.IOCDoesNotRest]==] [==[OrderBook.MarketDoesNotRest]==] [==[OrderBook.CancelRemovesOrder]==] [==[OrderBook.PartialFillRestsLimitRemainder]==] [==[OrderBook.DeterministicStateHash]==] [==[OrderBook.RejectDuplicate]==] [==[AdvancedOrderBook.FillOrKillRejectsWithoutMutation]==] [==[AdvancedOrderBook.FillOrKillExecutesCompletely]==] [==[AdvancedOrderBook.PostOnlyRejectsCrossingOrder]==] [==[AdvancedOrderBook.QuantityReductionPreservesPriority]==] [==[AdvancedOrderBook.PriceChangeLosesPriority]==] [==[AdvancedOrderBook.DepthIncludesOrderCount]==] [==[AdvancedExchange.MultiSymbolIsolation]==] [==[AdvancedExchange.SnapshotRoundTrip]==] [==[RiskEngine.RejectsOversizedOrder]==] [==[RiskEngine.EnforcesPositionLimit]==] [==[RiskEngine.DailyLossTriggersKillSwitch]==] [==[Analytics.ComputesDrawdownAndWinRate]==] [==[Options.PutCallParityAndImpliedVolatility]==] [==[Options.MonteCarloConvergesNearBlackScholes]==] [==[Execution.SchedulesConserveQuantity]==] [==[QueueModel.TradesConsumeAheadBeforeOwnOrder]==] [==[Pairs.RecoversSyntheticHedgeRatio]==] [==[SpscRing.PreservesFifoOrder]==] [==[Pipeline.ProcessesCommandsDeterministically]==] [==[SmartOrderRouter.BuyUsesLowestEffectiveAskFirst]==] [==[SmartOrderRouter.ConsumesMultipleLevels]==] [==[SmartOrderRouter.SplitsAcrossVenuesAndLevels]==] [==[SmartOrderRouter.SellUsesHighestBidLevelsFirst]==] [==[SmartOrderRouter.ReportsPartialDepthRoute]==] [==[SmartOrderRouter.IgnoresInvalidLevels]==] [==[SmartOrderRouter.LatencyPenaltyCanChangeVenue]==] [==[RouterConstraints.BuyLimitPriceStopsExpensiveLevels]==] [==[RouterConstraints.SellLimitPriceStopsCheapLevels]==] [==[RouterConstraints.MaximumVenueCountIsEnforced]==] [==[RouterConstraints.AllOrNoneClearsPartialRoute]==] [==[RouterConstraints.BuySlippageLimitStopsWorseLevels]==] [==[RouterConstraints.SellSlippageLimitStopsWorseLevels]==] [==[RouterConstraints.RejectsInvalidControls]==] [==[RouterMarketData.ConvertsBookToVenueQuotes]==] [==[RouterMarketData.RoutesUsingMarketData]==] [==[ExecutionAlgo.MarketUsesSingleImmediateSlice]==] [==[ExecutionAlgo.TWAPUsesEqualTimeSlices]==] [==[ExecutionAlgo.VWAPFollowsVolumeProfile]==] [==[ExecutionAlgo.POVRespectsParticipationRate]==] [==[ExecutionAlgo.POVStopsWhenParentQuantityIsFilled]==] [==[ExecutionAlgo.IcebergUsesDisplayedQuantity]==] [==[ExecutionAlgo.BackwardCompatibleTWAPOverload]==] [==[ExecutionAlgo.RejectsInvalidCommonSettings]==] [==[ExecutionAlgo.RejectsInvalidAlgorithmSettings]==] [==[ExecutionReplay.BuildsDeterministicTimeline]==] [==[ExecutionReplay.PreservesChildOrder]==] [==[ExecutionReplay.AccumulatesLatency]==] [==[ExecutionReplay.RepresentsRejectedChild]==] [==[ExecutionReplay.RepresentsPartialFill]==] [==[ExecutionReplay.EmptyChildrenStillProducesParentEvents]==] [==[MarketReplay.SortsEventsByTimestamp]==] [==[MarketReplay.IteratesDeterministically]==] [==[MarketReplay.SeeksByIndex]==] [==[MarketReplay.SeeksByTimestamp]==] [==[MarketReplay.CalculatesStatistics]==] [==[MarketReplay.LoadsCsvFile]==] [==[MarketReplay.RejectsInvalidCsvHeader]==] [==[MarketReplay.RejectsCrossedQuote]==] [==[MarketData.AppliesSnapshot]==] [==[MarketData.AppliesIncrementalUpdate]==] [==[MarketData.DeletesPriceLevel]==] [==[MarketData.DetectsSequenceGap]==] [==[MarketData.RequiresSnapshotBeforeIncrementals]==] [==[MarketData.ReturnsRequestedDepth]==] [==[MarketData.RejectsCrossedSnapshot]==] [==[MarketData.BuildsConsolidatedBbo]==] [==[MarketData.ExcludesUnsynchronizedVenue]==] [==[MarketData.ListsVenuesForSymbol]==] [==[MarketData.AcceptsSnapshotSequenceZero]==] [==[MarketData.AppliesMultipleUpdatesWithOneSequence]==] [==[MarketData.RejectsCrossedConsolidatedBbo]==] [==[MarketRecorder.WritesAndReadsSnapshot]==] [==[MarketRecorder.WritesAndReadsUpdateBatch]==] [==[MarketRecorder.ReplaysDeterministically]==] [==[MarketRecorder.SupportsTimestampRange]==] [==[MarketRecorder.RewindsReader]==] [==[MarketRecorder.RejectsCorruptHeader]==] [==[CoinbaseLevel2.ParsesSnapshot]==] [==[CoinbaseLevel2.ParsesIncrementalUpdates]==] [==[CoinbaseLevel2.ParsesHeartbeat]==] [==[CoinbaseLevel2.IgnoresUnsupportedChannel]==] [==[CoinbaseLevel2.BuildsSubscriptions]==] [==[CoinbaseLevel2.EnvelopeSequenceGapsDoNotAffectNormalizedBook]==] [==[BacktestEngine.RunsTwapAgainstHistoricalQuotes]==] [==[BacktestEngine.ReportsPartialFill]==] [==[BacktestEngine.CalculatesMarketVwap]==] [==[BacktestEngine.RejectsMissingSymbol]==] [==[BacktestEngine.RejectsInvalidQuantity]==] [==[BacktestEngine.CreatesParentAndChildOrders]==] [==[BacktestEngine.CancelsIncompleteParent]==] [==[BacktestEngine.FillsReferenceOmsRecords]==] [==[BacktestEngine.RejectsChildWhenKillSwitchIsActive]==] [==[BacktestEngine.RejectsChildAboveQuantityLimit]==] [==[BacktestEngine.AcceptsChildWithinRiskLimits]==] [==[RoutedExecutionEngine.FullyExecutesRoute]==] [==[RoutedExecutionEngine.SupportsPartialFills]==] [==[RoutedExecutionEngine.ComputesAverageFillPrice]==] [==[RoutedExecutionEngine.ComputesFees]==] [==[RoutedExecutionEngine.RejectsInvalidFillRatio]==] [==[RoutedExecutionEngine.RejectsAllChildren]==] [==[RoutedExecutionEngine.SameSeedProducesSameResults]==] [==[RoutedExecutionEngine.AddsConfiguredLatency]==] [==[RoutedExecutionEngine.RejectsInvalidSimulationSettings]==] [==[OMS.CreatesParentOrder]==] [==[OMS.CreatesChildAndMovesParentToWorking]==] [==[OMS.RecordsPartialChildFill]==] [==[OMS.CompletesParentAcrossMultipleChildren]==] [==[OMS.RejectsOverfill]==] [==[OMS.CancelsParentAndWorkingChildren]==] [==[OMS.RejectsChildAndUpdatesParent]==] [==[OMS.ReturnsChildrenAndFillsForParent]==] [==[OMS.RejectsInvalidParentAndChildRequests]==] [==[FixCodec.EncodesAndParsesLogon]==] [==[FixCodec.EncodesNewOrderSingle]==] [==[FixCodec.RejectsInvalidChecksum]==] [==[FixCodec.RejectsInvalidBodyLength]==] [==[FixCodec.SupportsCancelAndExecutionReport]==] [==[FixCodec.PreparesPossDupResend]==] [==[FixCodec.CreatesSequenceResetGapFill]==] [==[FixSession.AcceptsLogonAndBecomesActive]==] [==[FixSession.RejectsIncorrectSequenceNumber]==] [==[FixSession.RejectsApplicationMessageBeforeLogon]==] [==[FixSession.AcceptsApplicationMessageAfterLogon]==] [==[FixSession.RespondsToTestRequestWithHeartbeat]==] [==[FixSession.HandlesLogout]==] [==[FixSession.DetectsHeartbeatAndTestRequestDeadlines]==] [==[FixSession.RejectsCompIdMismatch]==] [==[FixSession.OutgoingMessagesIncrementSequence]==] [==[FixSession.ResetRestoresInitialState]==] [==[FixSession.SequencesOutboundExecutionReports]==] [==[FixSession.RestoresPersistentSequenceState]==] [==[FixSession.RequestsResendForSequenceGap]==] [==[FixSession.AcceptsValidResendRequest]==] [==[FixSession.AppliesSequenceReset]==] [==[FixStore.SavesAndLoadsSessionSequenceState]==] [==[FixStore.ReturnsDefaultsForUnknownSession]==] [==[FixStore.PersistsInboundAndOutboundMessages]==] [==[FixStore.LoadsOutboundMessagesFromSequence]==] [==[FixStore.UpdatesExistingSessionSnapshot]==] [==[FixGateway.CreatesOmsOrdersFromNewOrderSingle]==] [==[FixGateway.RejectsDuplicateClientOrderId]==] [==[FixGateway.CancelsExistingOrder]==] [==[FixGateway.RejectsUnknownCancelOrder]==] [==[FixGateway.RejectsMissingRequiredFields]==] [==[FixGateway.SupportsMarketOrders]==] [==[ExecutionRisk.AcceptsValidOrder]==] [==[ExecutionRisk.KillSwitchRejectsOrders]==] [==[ExecutionRisk.RejectsParentQuantityLimit]==] [==[ExecutionRisk.RejectsChildQuantityLimit]==] [==[ExecutionRisk.RejectsNotionalLimit]==] [==[ExecutionRisk.RejectsParticipationLimit]==] [==[ExecutionRisk.RejectsBuyAbovePriceCollar]==] [==[ExecutionRisk.RejectsSellBelowPriceCollar]==] [==[ExecutionRisk.AllowsFavorablePriceMovement]==] [==[ExecutionRisk.RejectsInvalidConfiguration]==] [==[ExecutionRisk.RejectsInvalidOrderValues]==] [==[ExecutionRisk.ZeroParticipationLimitDisablesCheck]==] [==[ExecutionStore.CreatesSchema]==] [==[ExecutionStore.PersistsRouteAndChildren]==] [==[ExecutionStore.PersistsMultipleExecutions]==] [==[ExecutionStore.LoadsExecutionById]==] [==[ExecutionStore.ReturnsRecentExecutionsNewestFirst]==] [==[ExecutionStore.MissingExecutionReturnsEmpty]==])
