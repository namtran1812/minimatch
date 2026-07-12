#include "minimatch/quant.hpp"
#include <iomanip>
#include <iostream>

int main() {
  using namespace minimatch;
  OptionInputs in{100.0, 105.0, 0.04, 0.01, 0.25, 0.5};
  const auto bs = black_scholes(OptionKind::Call, in);
  const auto mc = monte_carlo_option_price(OptionKind::Call, in, 200000);
  OptionInputs guess = in;
  guess.volatility = 0.15;
  const double iv = implied_volatility(OptionKind::Call, bs.price, guess);
  std::cout << std::fixed << std::setprecision(6)
            << "black_scholes_price=" << bs.price << '\n'
            << "monte_carlo_price=" << mc << '\n'
            << "implied_volatility=" << iv << '\n'
            << "delta=" << bs.delta << '\n'
            << "gamma=" << bs.gamma << '\n'
            << "vega=" << bs.vega << '\n'
            << "theta=" << bs.theta << '\n'
            << "rho=" << bs.rho << '\n';
}
