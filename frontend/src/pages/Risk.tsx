import { useState } from "react";
import {
  useQuery,
  useQueryClient,
} from "@tanstack/react-query";

import {
  getClientRisk,
} from "../api/trading";

import {
  getSystemStatus,
  haltSystem,
  resumeSystem,
} from "../api/systemControl";

import {
  getPortfolio,
  getPortfolioRisk,
  getPositions,
  getStpStatus,
  setStpPolicy,
  getCircuitBreaker,
  configureCircuitBreaker,
  getSymbolStatus,
  haltSymbol,
  resumeSymbol,
  setPriceBand,
  clearPriceBand,
  updatePortfolioRiskLimits,
  type StpPolicy,
} from "../api/risk";

import RiskExposureChart from "../components/charts/RiskExposureChart";
import StatusBadge from "../components/StatusBadge";

export default function Risk({
  focusGlobalHalt,
}: {
  focusGlobalHalt: boolean;
}) {
  const [clientId, setClientId] =
    useState(20);

  const [symbol, setSymbol] =
    useState(1);

  const [controlStatus, setControlStatus] =
    useState("READY");

  const [
    stpPolicy,
    setStpPolicyValue,
  ] = useState<StpPolicy | null>(
    null
  );

  const [
    breakerEnabled,
    setBreakerEnabled,
  ] = useState<boolean | null>(
    null
  );

  const [
    breakerThreshold,
    setBreakerThreshold,
  ] = useState<number | null>(
    null
  );

  const [
    breakerWindow,
    setBreakerWindow,
  ] = useState<number | null>(
    null
  );

  const [
    referencePrice,
    setReferencePrice,
  ] = useState<number | null>(
    null
  );

  const [
    bandPercent,
    setBandPercent,
  ] = useState<number | null>(
    null
  );

  const [
    riskControlStatus,
    setRiskControlStatus,
  ] = useState("READY");

  const [
    maxGrossExposure,
    setMaxGrossExposure,
  ] = useState<number | null>(
    null
  );

  const [
    maxNetExposure,
    setMaxNetExposure,
  ] = useState<number | null>(
    null
  );

  const [
    maxConcentrationPercent,
    setMaxConcentrationPercent,
  ] = useState<number | null>(
    null
  );

  const [
    maxDailyLoss,
    setMaxDailyLoss,
  ] = useState<number | null>(
    null
  );

  const queryClient =
    useQueryClient();

  const { data: risk } = useQuery({
    queryKey: [
      "client-risk",
      clientId,
      symbol,
    ],
    queryFn: () =>
      getClientRisk(
        clientId,
        symbol
      ),
    refetchInterval: 1000,
  });

  const {
    data: systemStatus,
  } = useQuery({
    queryKey: [
      "system-trading-status",
    ],
    queryFn: getSystemStatus,
    refetchInterval: 1000,
  });


  const {
    data: portfolio,
  } = useQuery({
    queryKey: ["portfolio"],
    queryFn: getPortfolio,
    refetchInterval: 1000,
  });

  const {
    data: portfolioRisk,
  } = useQuery({
    queryKey: ["portfolio-risk"],
    queryFn: getPortfolioRisk,
    refetchInterval: 1000,
  });

  const {
    data: positions = [],
  } = useQuery({
    queryKey: ["positions"],
    queryFn: getPositions,
    refetchInterval: 1000,
  });


  const {
    data: stpStatus,
  } = useQuery({
    queryKey: ["stp"],
    queryFn: getStpStatus,
    refetchInterval: 2000,
  });

  const {
    data: circuitBreaker,
  } = useQuery({
    queryKey: ["circuit-breaker"],
    queryFn: getCircuitBreaker,
    refetchInterval: 1000,
  });

  const {
    data: symbolStatus,
  } = useQuery({
    queryKey: [
      "symbol-status",
      symbol,
    ],
    queryFn: () =>
      getSymbolStatus(symbol),
    refetchInterval: 1000,
  });

  const effectiveStpPolicy =
    stpPolicy ??
    stpStatus?.policy ??
    "NONE";

  const effectiveBreakerEnabled =
    breakerEnabled ??
    circuitBreaker?.enabled ??
    false;

  const effectiveBreakerThreshold =
    breakerThreshold ??
    circuitBreaker?.thresholdPercent ??
    5;

  const effectiveBreakerWindow =
    breakerWindow ??
    circuitBreaker?.windowSeconds ??
    60;

  const effectiveReferencePrice =
    referencePrice ??
    (
      circuitBreaker?.currentMidpoint
        ? Math.round(
            circuitBreaker.currentMidpoint
          )
        : 0
    );

  const effectiveBandPercent =
    bandPercent ?? 5;

  const effectiveMaxGrossExposure =
    maxGrossExposure ??
    portfolioRisk?.limits
      .maxGrossExposure ??
    0;

  const effectiveMaxNetExposure =
    maxNetExposure ??
    portfolioRisk?.limits
      .maxNetExposure ??
    0;

  const effectiveMaxConcentration =
    maxConcentrationPercent ??
    portfolioRisk?.limits
      .maxConcentrationPercent ??
    0;

  const effectiveMaxDailyLoss =
    maxDailyLoss ??
    portfolioRisk?.limits
      .maxDailyLoss ??
    0;

  async function handleApplyStp() {
    try {
      setRiskControlStatus(
        "APPLYING STP..."
      );

      await setStpPolicy(
        effectiveStpPolicy
      );

      await queryClient.invalidateQueries({
        queryKey: ["stp"],
      });

      setRiskControlStatus(
        `STP ${effectiveStpPolicy} ACTIVE`
      );
    } catch {
      setRiskControlStatus(
        "STP UPDATE FAILED"
      );
    }
  }

  async function handleApplyBreaker() {
    try {
      setRiskControlStatus(
        "UPDATING CIRCUIT BREAKER..."
      );

      await configureCircuitBreaker({
        enabled:
          effectiveBreakerEnabled,
        thresholdPercent:
          effectiveBreakerThreshold,
        windowSeconds:
          effectiveBreakerWindow,
      });

      await queryClient.invalidateQueries({
        queryKey: [
          "circuit-breaker",
        ],
      });

      setRiskControlStatus(
        "CIRCUIT BREAKER UPDATED"
      );
    } catch {
      setRiskControlStatus(
        "CIRCUIT BREAKER UPDATE FAILED"
      );
    }
  }

  async function handleSymbolToggle() {
    try {
      const halted =
        symbolStatus?.halted ??
        false;

      setRiskControlStatus(
        halted
          ? `RESUMING SYMBOL ${symbol}...`
          : `HALTING SYMBOL ${symbol}...`
      );

      if (halted) {
        await resumeSymbol(symbol);
      } else {
        const confirmed =
          window.confirm(
            `HALT SYMBOL ${symbol}?`
          );

        if (!confirmed) {
          setRiskControlStatus(
            "SYMBOL HALT CANCELLED"
          );
          return;
        }

        await haltSymbol(symbol);
      }

      await queryClient.invalidateQueries({
        queryKey: [
          "symbol-status",
          symbol,
        ],
      });

      setRiskControlStatus(
        halted
          ? `SYMBOL ${symbol} RESUMED`
          : `SYMBOL ${symbol} HALTED`
      );
    } catch {
      setRiskControlStatus(
        "SYMBOL CONTROL FAILED"
      );
    }
  }

  async function handleApplyPriceBand() {
    try {
      setRiskControlStatus(
        "APPLYING PRICE BAND..."
      );

      await setPriceBand(
        symbol,
        {
          referencePrice:
            Math.round(
              effectiveReferencePrice
            ),
          bandPercent:
            effectiveBandPercent,
        }
      );

      setRiskControlStatus(
        "PRICE BAND ACTIVE"
      );
    } catch {
      setRiskControlStatus(
        "PRICE BAND UPDATE FAILED"
      );
    }
  }

  async function handleClearPriceBand() {
    try {
      setRiskControlStatus(
        "CLEARING PRICE BAND..."
      );

      await clearPriceBand(
        symbol
      );

      setRiskControlStatus(
        "PRICE BAND CLEARED"
      );
    } catch {
      setRiskControlStatus(
        "PRICE BAND CLEAR FAILED"
      );
    }
  }

  async function handleApplyPortfolioLimits() {
    const confirmed =
      window.confirm(
        "APPLY PORTFOLIO RISK LIMITS?\n\n" +
        "If the portfolio already breaches the new limits, " +
        "MiniMatch may immediately halt global trading."
      );

    if (!confirmed) {
      return;
    }

    try {
      setRiskControlStatus(
        "UPDATING PORTFOLIO LIMITS..."
      );

      await updatePortfolioRiskLimits({
        maxGrossExposure:
          effectiveMaxGrossExposure,
        maxNetExposure:
          effectiveMaxNetExposure,
        maxConcentrationPercent:
          effectiveMaxConcentration,
        maxDailyLoss:
          effectiveMaxDailyLoss,
      });

      await Promise.all([
        queryClient.invalidateQueries({
          queryKey: [
            "portfolio-risk",
          ],
        }),
        queryClient.invalidateQueries({
          queryKey: [
            "system-trading-status",
          ],
        }),
      ]);

      setRiskControlStatus(
        "PORTFOLIO LIMITS UPDATED"
      );
    } catch {
      setRiskControlStatus(
        "PORTFOLIO LIMIT UPDATE FAILED"
      );
    }
  }

  async function handleHalt() {
    const confirmed =
      window.confirm(
        "HALT ALL TRADING?\n\n" +
        "This will globally stop the MiniMatch exchange."
      );

    if (!confirmed) {
      return;
    }

    try {
      setControlStatus(
        "HALTING..."
      );

      await haltSystem();

      await queryClient.invalidateQueries({
        queryKey: [
          "system-trading-status",
        ],
      });

      setControlStatus(
        "GLOBAL EXCHANGE HALTED"
      );
    } catch {
      setControlStatus(
        "HALT FAILED"
      );
    }
  }

  async function handleResume() {
    const confirmed =
      window.confirm(
        "RESUME GLOBAL TRADING?"
      );

    if (!confirmed) {
      return;
    }

    try {
      setControlStatus(
        "RESUMING..."
      );

      await resumeSystem();

      await queryClient.invalidateQueries({
        queryKey: [
          "system-trading-status",
        ],
      });

      setControlStatus(
        "TRADING RESUMED"
      );
    } catch {
      setControlStatus(
        "RESUME FAILED"
      );
    }
  }

  const globallyHalted =
    systemStatus?.globallyHalted ??
    false;

  const formatMoney = (
    value: number | undefined
  ) =>
    value === undefined
      ? "—"
      : `$${value.toLocaleString(
          undefined,
          {
            minimumFractionDigits: 2,
            maximumFractionDigits: 2,
          }
        )}`;

  const formatNumber = (
    value: number | undefined,
    digits = 2
  ) =>
    value === undefined
      ? "—"
      : value.toLocaleString(
          undefined,
          {
            maximumFractionDigits: digits,
          }
        );

  const grossUtilization =
    portfolioRisk &&
    portfolioRisk.limits
      .maxGrossExposure > 0
      ? Math.abs(
          portfolioRisk.grossExposure
        ) /
        portfolioRisk.limits
          .maxGrossExposure *
        100
      : 0;

  const netUtilization =
    portfolioRisk &&
    portfolioRisk.limits
      .maxNetExposure > 0
      ? Math.abs(
          portfolioRisk.netExposure
        ) /
        portfolioRisk.limits
          .maxNetExposure *
        100
      : 0;

  const concentrationUtilization =
    portfolioRisk &&
    portfolioRisk.limits
      .maxConcentrationPercent > 0
      ? portfolioRisk
          .largestConcentrationPercent /
        portfolioRisk.limits
          .maxConcentrationPercent *
        100
      : 0;

  const dailyLossUtilization =
    portfolioRisk &&
    portfolioRisk.limits
      .maxDailyLoss > 0
      ? Math.max(
          0,
          -portfolioRisk.totalPnl
        ) /
        portfolioRisk.limits
          .maxDailyLoss *
        100
      : 0;

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          REAL-TIME RISK
        </span>

        <h1>Risk</h1>
      </div>

      <div className="risk-info">
        <div className="risk-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Risk monitors client and firm-wide exposure, positions,
            profit and loss, concentration, trading limits, and
            automated protection mechanisms across MiniMatch.
          </p>
        </div>

        <div className="risk-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Start with portfolio health and limit utilization.
            Inspect live positions and client exposure, then use
            operator controls only when adjusting protection rules
            or responding to abnormal trading conditions.
          </p>
        </div>

        <div className="risk-info__section">
          <span className="eyebrow">
            RISK CONTROLS
          </span>

          <p>
            STP prevents self-trading. Circuit breakers respond to
            abnormal price moves. Symbol halts isolate instruments,
            price bands constrain acceptable prices, and portfolio
            limits can automatically halt trading when breached.
          </p>
        </div>
      </div>

      <div className="risk-status-strip">
        <div>
          <span>SYSTEM</span>
          <StatusBadge
            value={
              globallyHalted
                ? "HALTED"
                : "ACTIVE"
            }
          />
        </div>

        <div>
          <span>CLIENT RISK</span>
          <StatusBadge
            value={
              risk?.killed
                ? "KILLED"
                : "SAFE"
            }
          />
        </div>

        <div>
          <span>PORTFOLIO</span>
          <StatusBadge
            value={
              portfolioRisk?.breached
                ? "BREACHED"
                : "SAFE"
            }
          />
        </div>

        <div>
          <span>SYMBOL</span>
          <StatusBadge
            value={
              symbolStatus?.halted
                ? "HALTED"
                : "ACTIVE"
            }
          />
        </div>
      </div>

      <div className="panel risk-client-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              CLIENT RISK
            </span>

            <h2>Client / Instrument</h2>
          </div>

          <span>
            LIVE
          </span>
        </div>

        <div className="risk-client-fields">
          <label>
            CLIENT ID
          <input
            type="number"
            value={clientId}
            onChange={(event) =>
              setClientId(
                Number(
                  event.target.value
                )
              )
            }
          />
        </label>

        <label>
          SYMBOL
          <input
            type="number"
            value={symbol}
            onChange={(event) =>
              setSymbol(
                Number(
                  event.target.value
                )
              )
            }
          />
          </label>
        </div>
      </div>

      <div className="metrics risk-client-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            POSITION
          </span>

          <strong>
            {risk?.position ?? "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            OPEN NOTIONAL
          </span>

          <strong>
            {risk?.openNotional ??
              "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            REALIZED PNL
          </span>

          <strong>
            {risk?.realizedPnl ??
              "—"}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            CLIENT KILL
          </span>

          <strong>
            {risk?.killed
              ? "ACTIVE"
              : "SAFE"}
          </strong>
        </div>
      </div>

      <div className="risk-section-heading">
        <div>
          <span className="eyebrow">
            PORTFOLIO
          </span>

          <h2>Firm-Wide Risk</h2>
        </div>

        <strong
          className={
            portfolioRisk?.breached
              ? "risk-state risk-state--danger"
              : "risk-state risk-state--safe"
          }
        >
          {portfolioRisk?.breached
            ? "LIMIT BREACH"
            : "WITHIN LIMITS"}
        </strong>
      </div>

      <div className="metrics risk-portfolio-metrics">
        <div className="metric-card">
          <span className="eyebrow">
            GROSS EXPOSURE
          </span>

          <strong>
            {formatMoney(
              portfolio?.grossExposure
            )}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            NET EXPOSURE
          </span>

          <strong>
            {formatMoney(
              portfolio?.netExposure
            )}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            TOTAL PNL
          </span>

          <strong>
            {formatMoney(
              portfolio?.totalPnl
            )}
          </strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">
            CONCENTRATION
          </span>

          <strong>
            {portfolio
              ? `${portfolio.largestConcentrationPercent.toFixed(2)}%`
              : "—"}
          </strong>
        </div>
      </div>

      {portfolioRisk && (
        <div className="risk-utilization-grid">
          {[
            [
              "GROSS",
              grossUtilization,
            ],
            [
              "NET",
              netUtilization,
            ],
            [
              "CONCENTRATION",
              concentrationUtilization,
            ],
            [
              "DAILY LOSS",
              dailyLossUtilization,
            ],
          ].map(([name, rawValue]) => {
            const value =
              Number(rawValue);

            return (
              <div
                className="risk-utilization-card"
                key={String(name)}
              >
                <div className="risk-utilization-card__header">
                  <span>
                    {String(name)}
                  </span>

                  <strong
                    className={
                      value >= 100
                        ? "risk-utilization--danger"
                        : value >= 80
                          ? "risk-utilization--warn"
                          : "risk-utilization--safe"
                    }
                  >
                    {value.toFixed(1)}%
                  </strong>
                </div>

                <div className="risk-utilization-track">
                  <div
                    className={
                      value >= 100
                        ? "risk-utilization-fill risk-utilization-fill--danger"
                        : value >= 80
                          ? "risk-utilization-fill risk-utilization-fill--warn"
                          : "risk-utilization-fill"
                    }
                    style={{
                      width: `${Math.min(
                        value,
                        100
                      )}%`,
                    }}
                  />
                </div>
              </div>
            );
          })}
        </div>
      )}

      {portfolioRisk && (
        <div className="risk-visualization-grid">
          <RiskExposureChart
            grossExposure={
              portfolioRisk.grossExposure
            }
            netExposure={
              portfolioRisk.netExposure
            }
            maxGrossExposure={
              portfolioRisk.limits
                .maxGrossExposure
            }
            maxNetExposure={
              portfolioRisk.limits
                .maxNetExposure
            }
          />

          <div className="panel risk-breach-panel">
            <div className="panel-title">
              <div>
                <span className="eyebrow">
                  LIMIT MONITOR
                </span>

                <h2>Risk Checks</h2>
              </div>
            </div>

            {[
              [
                "GROSS EXPOSURE",
                portfolioRisk
                  .grossExposureBreached,
              ],
              [
                "NET EXPOSURE",
                portfolioRisk
                  .netExposureBreached,
              ],
              [
                "CONCENTRATION",
                portfolioRisk
                  .concentrationBreached,
              ],
              [
                "DAILY LOSS",
                portfolioRisk
                  .dailyLossBreached,
              ],
            ].map(([name, breached]) => (
              <div
                className="risk-check-row"
                key={String(name)}
              >
                <span>{String(name)}</span>

                <strong
                  className={
                    breached
                      ? "risk-check--breached"
                      : "risk-check--safe"
                  }
                >
                  {breached
                    ? "BREACHED"
                    : "PASS"}
                </strong>
              </div>
            ))}
          </div>
        </div>
      )}

      <div className="panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              LIVE MARK-TO-MARKET
            </span>

            <h2>Positions</h2>
          </div>

          <span>
            {positions.length} OPEN
          </span>
        </div>

        {positions.length === 0 ? (
          <div className="feedback">
            No portfolio positions.
          </div>
        ) : (
          <div className="risk-position-table">
            <div className="risk-position-row risk-position-row--header">
              <span>SYMBOL</span>
              <span>QTY</span>
              <span>AVG COST</span>
              <span>MARK</span>
              <span>REALIZED</span>
              <span>UNREALIZED</span>
              <span>TOTAL PNL</span>
            </div>

            {positions.map((position) => (
              <div
                className="risk-position-row"
                key={position.symbol}
              >
                <strong>
                  {position.symbol.toUpperCase()}
                </strong>

                <span>
                  {position.netQuantity}
                </span>

                <span>
                  {formatNumber(
                    position.averageCost,
                    2
                  )}
                </span>

                <span>
                  {formatNumber(
                    position.markPrice,
                    2
                  )}
                </span>

                <span>
                  {formatMoney(
                    position.realizedPnl
                  )}
                </span>

                <span>
                  {formatMoney(
                    position.unrealizedPnl
                  )}
                </span>

                <strong>
                  {formatMoney(
                    position.totalPnl
                  )}
                </strong>
              </div>
            ))}
          </div>
        )}
      </div>

      <div className="risk-section-heading">
        <div>
          <span className="eyebrow">
            OPERATOR CONTROLS
          </span>

          <h2>Risk Controls</h2>
        </div>

        <StatusBadge
          value={
            portfolioRisk?.breached
              ? "BREACHED"
              : "SAFE"
          }
        />
      </div>

      <div className="risk-controls-grid">
        <div className="panel risk-control-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                SELF-TRADE PREVENTION
              </span>
              <h2>STP Policy</h2>
            </div>

            <StatusBadge
              value={
                stpStatus?.policy ??
                "NONE"
              }
            />
          </div>

          <label>
            POLICY
            <select
              value={
                effectiveStpPolicy
              }
              onChange={(event) =>
                setStpPolicyValue(
                  event.target
                    .value as StpPolicy
                )
              }
            >
              <option value="NONE">
                NONE
              </option>

              <option value="CANCEL_NEWEST">
                CANCEL NEWEST
              </option>

              <option value="CANCEL_OLDEST">
                CANCEL OLDEST
              </option>

              <option value="CANCEL_BOTH">
                CANCEL BOTH
              </option>
            </select>
          </label>

          <button
            type="button"
            onClick={handleApplyStp}
          >
            APPLY STP POLICY
          </button>
        </div>

        <div className="panel risk-control-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                VOLATILITY PROTECTION
              </span>
              <h2>Circuit Breaker</h2>
            </div>

            <StatusBadge
              value={
                circuitBreaker
                  ?.symbolHalted
                  ? "HALTED"
                  : effectiveBreakerEnabled
                    ? "ACTIVE"
                    : "DISABLED"
              }
            />
          </div>

          <div className="risk-control-fields">
            <label>
              ENABLED
              <select
                value={
                  effectiveBreakerEnabled
                    ? "YES"
                    : "NO"
                }
                onChange={(event) =>
                  setBreakerEnabled(
                    event.target.value ===
                      "YES"
                  )
                }
              >
                <option>YES</option>
                <option>NO</option>
              </select>
            </label>

            <label>
              THRESHOLD %
              <input
                type="number"
                step="0.01"
                value={
                  effectiveBreakerThreshold
                }
                onChange={(event) =>
                  setBreakerThreshold(
                    Number(
                      event.target.value
                    )
                  )
                }
              />
            </label>

            <label>
              WINDOW SEC
              <input
                type="number"
                value={
                  effectiveBreakerWindow
                }
                onChange={(event) =>
                  setBreakerWindow(
                    Number(
                      event.target.value
                    )
                  )
                }
              />
            </label>

            <div className="risk-control-readout">
              <span>CURRENT MOVE</span>
              <strong>
                {circuitBreaker
                  ? `${circuitBreaker.movePercent.toFixed(3)}%`
                  : "—"}
              </strong>
            </div>
          </div>

          <button
            type="button"
            onClick={
              handleApplyBreaker
            }
          >
            APPLY BREAKER CONFIG
          </button>
        </div>

        <div className="panel risk-control-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                INSTRUMENT CONTROL
              </span>
              <h2>Symbol Trading</h2>
            </div>

            <StatusBadge
              value={
                symbolStatus?.halted
                  ? "HALTED"
                  : "ACTIVE"
              }
            />
          </div>

          <div className="risk-control-readout">
            <span>SYMBOL ID</span>
            <strong>{symbol}</strong>
          </div>

          <div className="risk-control-readout">
            <span>GLOBAL STATE</span>
            <strong>
              {symbolStatus
                ?.globallyHalted
                ? "HALTED"
                : "ACTIVE"}
            </strong>
          </div>

          <button
            type="button"
            className={
              symbolStatus?.halted
                ? "system-resume-button"
                : "system-halt-button"
            }
            onClick={
              handleSymbolToggle
            }
          >
            {symbolStatus?.halted
              ? "RESUME SYMBOL"
              : "HALT SYMBOL"}
          </button>
        </div>

        <div className="panel risk-control-panel">
          <div className="panel-title">
            <div>
              <span className="eyebrow">
                PRICE PROTECTION
              </span>
              <h2>Price Band</h2>
            </div>
          </div>

          <div className="risk-control-fields">
            <label>
              REFERENCE PRICE
              <input
                type="number"
                value={
                  effectiveReferencePrice
                }
                onChange={(event) =>
                  setReferencePrice(
                    Number(
                      event.target.value
                    )
                  )
                }
              />
            </label>

            <label>
              BAND %
              <input
                type="number"
                step="0.01"
                value={
                  effectiveBandPercent
                }
                onChange={(event) =>
                  setBandPercent(
                    Number(
                      event.target.value
                    )
                  )
                }
              />
            </label>
          </div>

          <div className="risk-control-actions">
            <button
              type="button"
              onClick={
                handleApplyPriceBand
              }
            >
              APPLY BAND
            </button>

            <button
              type="button"
              onClick={
                handleClearPriceBand
              }
            >
              CLEAR BAND
            </button>
          </div>
        </div>
      </div>

      <div className="panel risk-limits-panel">
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              FIRM-WIDE LIMITS
            </span>

            <h2>Portfolio Risk Limits</h2>
          </div>

          <StatusBadge
            value={
              portfolioRisk?.breached
                ? "BREACHED"
                : "SAFE"
            }
          />
        </div>

        <div className="risk-limit-grid">
          <label>
            MAX GROSS EXPOSURE
            <input
              type="number"
              value={
                effectiveMaxGrossExposure
              }
              onChange={(event) =>
                setMaxGrossExposure(
                  Number(
                    event.target.value
                  )
                )
              }
            />
          </label>

          <label>
            MAX NET EXPOSURE
            <input
              type="number"
              value={
                effectiveMaxNetExposure
              }
              onChange={(event) =>
                setMaxNetExposure(
                  Number(
                    event.target.value
                  )
                )
              }
            />
          </label>

          <label>
            MAX CONCENTRATION %
            <input
              type="number"
              step="0.01"
              value={
                effectiveMaxConcentration
              }
              onChange={(event) =>
                setMaxConcentrationPercent(
                  Number(
                    event.target.value
                  )
                )
              }
            />
          </label>

          <label>
            MAX DAILY LOSS
            <input
              type="number"
              value={
                effectiveMaxDailyLoss
              }
              onChange={(event) =>
                setMaxDailyLoss(
                  Number(
                    event.target.value
                  )
                )
              }
            />
          </label>
        </div>

        <button
          type="button"
          className="risk-limit-apply"
          onClick={
            handleApplyPortfolioLimits
          }
        >
          APPLY PORTFOLIO LIMITS
        </button>

        <div className="feedback">
          {riskControlStatus}
        </div>
      </div>

      <div
        className={
          globallyHalted
            ? "panel system-kill-panel system-kill-panel--halted"
            : "panel system-kill-panel"
        }
      >
        <div className="panel-title">
          <div>
            <span className="eyebrow">
              SYSTEM TRADING CONTROL
            </span>

            <h2>
              Global Exchange
            </h2>
          </div>

          <strong
            className={
              globallyHalted
                ? "system-trading-state system-trading-state--halted"
                : "system-trading-state system-trading-state--running"
            }
          >
            <span className="venue-status__pixel" />

            {globallyHalted
              ? "HALTED"
              : "RUNNING"}
          </strong>
        </div>

        <p className="system-kill-warning">
          Controls the global MiniMatch
          matching engine. Confirmation
          is required before changing
          trading state.
        </p>

        {!globallyHalted ? (
          <button
            id="global-halt-control"
            className="system-halt-button"
            autoFocus={focusGlobalHalt}
            onClick={handleHalt}
          >
            ARM GLOBAL HALT
          </button>
        ) : (
          <button
            id="global-halt-control"
            className="system-resume-button"
            autoFocus={focusGlobalHalt}
            onClick={handleResume}
          >
            RESUME TRADING
          </button>
        )}

        <div className="feedback">
          {controlStatus}
        </div>
      </div>
    </section>
  );
}
