import { useState } from "react";

interface Reflection {
  number: string;
  area: string;
  title: string;
  summary: string;
  assumption?: string;
  challenge?: string;
  change?: string;
  lesson?: string;
}

const reflections: Reflection[] = [
  {
    number: "01",
    area: "MATCHING ENGINE",
    title: "Determinism Before Performance",
    summary:
      "How price-time priority, replayable state, and correctness became the foundation for every optimization that followed.",

    assumption:
      "I initially thought the hardest part of a matching engine would be making it fast. That pushed me toward data structures, cache behavior, and latency almost immediately.",

    challenge:
      "The more difficult problem was proving that the engine behaved identically under the same event sequence. FIFO ordering, partial fills, cancellation, amendments, IOC, FOK, and Post-Only semantics all created subtle state transitions where a small inconsistency could invalidate later results.",

    change:
      "I made deterministic behavior a first-class requirement. Every accepted event could be replayed in sequence, and the resulting engine state could be compared through a deterministic state hash. Performance work came after those invariants were covered by tests.",

    lesson:
      "For stateful systems, speed is only useful after behavior is reproducible. Determinism became both a correctness property and a debugging tool, and it ultimately made later performance optimization safer.",
  },
  {
    number: "02",
    area: "MARKET DATA",
    title: "From a Book to a Market",
    summary:
      "Why multi-venue feeds forced synchronization, staleness, sequence recovery, and health to become first-class state.",

    assumption:
      "I started by thinking of market data as a stream of prices feeding the rest of the system. As long as the latest bid and ask were available, downstream components could treat the market as current.",

    challenge:
      "That model broke once I connected multiple venues. Coinbase, Kraken, and Binance could disconnect independently, fall behind, lose sequence continuity, or temporarily publish stale quotes. A numerically valid quote was not necessarily a trustworthy quote.",

    change:
      "I moved operational health into the market-data model itself. Each venue carried synchronization state, quote age, sequence-gap counts, reconnect information, and message rate alongside its prices. The consolidated market view could then distinguish fresh, synchronized data from degraded feeds.",

    lesson:
      "In a real-time system, data quality is more than the value itself. Provenance, freshness, continuity, and recovery state are part of correctness. Once I treated health metadata as trading data rather than logging metadata, the rest of the architecture became easier to reason about.",
  },
  {
    number: "03",
    area: "EXECUTION + OMS",
    title: "Orders Are Lifecycles, Not Requests",
    summary:
      "What parent-child execution taught me about scheduling, state transitions, fills, and algorithmic execution.",

    assumption:
      "At first, I treated execution as a simple extension of order entry: build an order, send it to the engine, and wait for the result. That abstraction worked for isolated orders but became too shallow once I introduced execution algorithms.",

    challenge:
      "TWAP, VWAP, POV, and Iceberg orders created state that lived much longer than a single request. A parent order could generate many child orders over time, each with its own venue, status, fills, cancellations, fees, and remaining quantity. The hard part became maintaining a coherent lifecycle across all of those states.",

    change:
      "I introduced explicit parent and child order models and made the OMS responsible for their relationships. Execution algorithms scheduled child orders while fills flowed back into parent state, execution-quality metrics, and reconciliation. Cancelling or partially filling one child no longer looked like an isolated event; it became part of a larger execution lifecycle.",

    lesson:
      "In trading systems, an order is not just a command. It is a state machine with dependencies, history, and downstream consequences. Modeling that lifecycle explicitly made the system easier to extend and made later features like execution analytics, reconciliation, and recovery possible.",
  },
  {
    number: "04",
    area: "RISK",
    title: "Controls Belong in the Architecture",
    summary:
      "Why position limits, STP, circuit breakers, and portfolio controls could not remain checks scattered through order handling.",

    assumption:
      "My first risk controls were naturally implemented close to order submission. Before accepting an order, the system could check its size, current position, or loss limits and reject it when necessary. For a small engine, that seemed sufficient.",

    challenge:
      "As MiniMatch grew, risk stopped being a single pre-trade decision. Self-trade prevention needed information about resting orders. Position and portfolio limits depended on fills. Circuit breakers responded to broader system conditions, while a global halt had to affect execution consistently across multiple entry points. Scattering these checks through trading code made their interactions increasingly difficult to reason about.",

    change:
      "I separated risk into an explicit control layer with portfolio limits, position state, STP policies, circuit breakers, and system-wide trading controls. Instead of individual features deciding independently whether trading should continue, they contributed to a common set of enforceable system constraints.",

    lesson:
      "Risk controls are architectural boundaries, not defensive conditionals. The more ways a system can create or modify exposure, the more important it becomes to centralize the invariants that every path must obey. Designing risk as its own subsystem made those guarantees visible and testable.",
  },
  {
    number: "05",
    area: "REPLAY",
    title: "Debugging Through Determinism",
    summary:
      "How deterministic replay turned difficult state bugs into reproducible sequences that could be inspected and verified.",

    assumption:
      "I originally built deterministic behavior mainly as a correctness property. If the same sequence of orders produced the same trades, active orders, and state hash, I could verify that changes to the matching engine had not silently changed its behavior.",

    challenge:
      "Once more subsystems were connected, failures became temporal. A bad state might only appear after a particular sequence of submissions, fills, cancellations, or market-data updates. Looking at the final state or reading logs after the fact was not enough to understand exactly where the system diverged.",

    change:
      "I turned determinism into a debugging workflow. MiniMatch records ordered events and can reconstruct historical execution from them. I then added replay controls for restarting, pausing, seeking through progress, and changing playback speed so intermediate state could be inspected rather than only the final outcome.",

    lesson:
      "Reproducibility changes the economics of debugging. Instead of trying to reproduce a transient failure manually, I can preserve the sequence that caused it and repeatedly inspect the same transition. The event log became more than historical data; it became an executable description of system behavior.",
  },
  {
    number: "06",
    area: "POST-TRADE",
    title: "Execution Is Not the End",
    summary:
      "Why reconciliation and drop-copy infrastructure became necessary once execution state existed in multiple independent views.",

    assumption:
      "Initially, I treated a successful execution as the end of the order lifecycle. If the matching engine produced a fill and the OMS updated the parent and child orders correctly, I assumed the system had enough information to describe what happened.",

    challenge:
      "That assumption became weaker as execution state spread across the matching engine, OMS, positions, and persisted records. A component reporting a successful fill did not prove that every other view had reached the same conclusion. Recovery made this harder because reconstructed state also needed to agree with what had previously been recorded.",

    change:
      "I added an independent drop-copy path that persists execution events and maintains an order index outside the primary OMS view. On top of that, I introduced parent-order and position reconciliation so independently maintained records could be compared for consistency. Recovery could then be validated against durable execution history rather than trusting one in-memory representation.",

    lesson:
      "A system should not prove its own correctness using only the state it is trying to verify. Independent records and reconciliation create stronger evidence about what actually happened. I began to think of post-trade infrastructure as a consistency system rather than simply a place to store completed trades.",
  },
  {
    number: "07",
    area: "OBSERVABILITY",
    title: "Metrics Are Part of the Product",
    summary:
      "How latency percentiles, venue health, Prometheus, and Grafana changed the way I reasoned about whether the system was healthy.",

    assumption:
      "At the beginning, I relied heavily on logs. They were useful for tracing individual failures, so I assumed better logging would be enough to understand whether the system was operating correctly.",

    challenge:
      "As MiniMatch became more concurrent and stateful, logs became good at explaining isolated events but poor at answering operational questions. I needed to know whether latency was degrading, whether a venue was stale, whether sequence gaps were increasing, whether reconciliation remained healthy, and whether recovery behavior was changing over time.",

    change:
      "I added structured operational metrics alongside the application state: P50, P95, and P99 latency, venue message rates, quote age, synchronization state, reconnects, and integrity counters. I then exposed Prometheus metrics and provisioned Grafana dashboards for reconciliation, recovery, drop-copy lookup performance, and position consistency.",

    lesson:
      "Observability is not something added after a system is finished. It changes how the system itself is designed. Once I had metrics that described health over time, I stopped asking only whether a component worked and started asking whether I could prove that it was working under changing conditions.",
  },
  {
    number: "08",
    area: "FRONTEND",
    title: "Making Infrastructure Legible",
    summary:
      "What changed when the goal became not only building systems, but making their behavior understandable to another engineer.",

    assumption:
      "I initially treated the frontend as a visualization layer on top of the interesting engineering work. The matching engine, routing, OMS, risk, replay, and observability systems felt like the real product, while the UI mainly needed to expose their outputs.",

    challenge:
      "As more systems became available, the interface started reflecting the architecture too literally. Pages accumulated one-off layouts, raw counters, and controls without a consistent hierarchy. Even when the backend was behaving correctly, it was difficult for another engineer to quickly understand what was healthy, what was changing, and where to investigate next.",

    change:
      "I redesigned the frontend around shared interaction patterns: compact status strips, consistent KPI cards, live charts, health summaries, dense tables, and clearer separation between monitoring and control. I also centralized live market state so multiple pages could consume the same WebSocket-driven snapshot instead of independently recreating market context.",

    lesson:
      "A system is easier to operate when its interface teaches the architecture. Good frontend work for infrastructure is not decoration; it compresses complexity into a structure that helps engineers form the right mental model of the system quickly.",
  },
  {
    number: "09",
    area: "PERFORMANCE",
    title: "Measure Before Optimizing",
    summary:
      "The difference between optimizations that sounded fast and changes that actually improved measured tail latency.",

    assumption:
      "Early performance work was driven mostly by intuition. I focused on changes that looked obviously faster: fewer allocations, tighter data structures, reduced lookup work, and more direct access paths through the engine.",

    challenge:
      "Some changes improved average behavior without materially changing the slowest requests, while others moved work into a different part of the system and only appeared faster locally. Once routing, replay, persistence, analytics, and recovery were involved, it became difficult to reason about performance from code structure alone.",

    change:
      "I started treating measurement as part of the implementation. Benchmarks tracked throughput and P50, P95, and P99 latency rather than relying on a single average. I compared baseline and optimized paths, profiled dependency lookups and drop-copy access, and kept performance changes only when they produced a measurable improvement under repeatable workloads.",

    lesson:
      "Performance engineering is an empirical discipline. A change is not an optimization because it looks efficient; it is an optimization when the measurement proves that it improves the metric that matters. Tail latency became especially important because the slowest requests are often the ones users and downstream systems actually notice.",
  },
  {
    number: "10",
    area: "RETROSPECTIVE",
    title: "What I Would Build Differently",
    summary:
      "Architecture decisions I would revisit after seeing the matching, execution, risk, market-data, and observability systems interact.",

    assumption:
      "While building MiniMatch, I optimized for forward progress. New capabilities were usually added where they fit most naturally at the time, and only later did the interactions between matching, market data, OMS, risk, persistence, replay, and observability become fully visible.",

    challenge:
      "As the project grew, some boundaries became less clean than I would want in a production system. The API process owns a large amount of shared state, several features depend directly on in-process objects, and some historical or observability paths were added after the core architecture had already formed.",

    change:
      "If I rebuilt the system, I would define clearer event contracts between subsystems from the beginning. Matching, execution, risk, market data, and post-trade services would publish typed events into a common event stream, with persistence and replay designed around that log. I would also separate command APIs from read models so dashboards could consume purpose-built views instead of depending on operational objects directly.",

    lesson:
      "The biggest architectural lesson was that boundaries become more important as features accumulate. A good abstraction is not only one that makes today's implementation clean; it should also make tomorrow's failure modes easier to isolate. Building MiniMatch end-to-end gave me a much stronger sense of which interfaces deserve to be stable early and which details should remain replaceable.",
  },
];

export default function Journal() {
  const [expanded, setExpanded] =
    useState<string | null>("01");

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          ENGINEERING REFLECTIONS
        </span>

        <h1>Engineering Journal</h1>
      </div>

      <div className="journal-index">
        {reflections.map(
          (reflection) => (
            <article
              className={`journal-entry ${
                expanded === reflection.number
                  ? "journal-entry--expanded"
                  : ""
              }`}
              key={reflection.number}
            >
              <button
                type="button"
                className="journal-entry__trigger"
                onClick={() =>
                  setExpanded(
                    expanded === reflection.number
                      ? null
                      : reflection.number
                  )
                }
              >
                <div className="journal-entry__number">
                  {reflection.number}
                </div>

                <div className="journal-entry__content">
                  <span className="eyebrow">
                    {reflection.area}
                  </span>

                  <h2>
                    {reflection.title}
                  </h2>

                  <p>
                    {reflection.summary}
                  </p>
                </div>

                <span className="journal-entry__arrow">
                  {expanded === reflection.number
                    ? "−"
                    : "+"}
                </span>
              </button>

              {expanded === reflection.number &&
                reflection.assumption && (
                  <div className="journal-reflection">
                    <div>
                      <span className="eyebrow">
                        INITIAL ASSUMPTION
                      </span>

                      <p>
                        {reflection.assumption}
                      </p>
                    </div>

                    <div>
                      <span className="eyebrow">
                        WHAT WAS HARD
                      </span>

                      <p>
                        {reflection.challenge}
                      </p>
                    </div>

                    <div>
                      <span className="eyebrow">
                        DESIGN CHANGE
                      </span>

                      <p>
                        {reflection.change}
                      </p>
                    </div>

                    <div>
                      <span className="eyebrow">
                        LESSON
                      </span>

                      <p>
                        {reflection.lesson}
                      </p>
                    </div>
                  </div>
                )}
            </article>
          )
        )}
      </div>
    </section>
  );
}
