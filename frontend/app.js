const tabs = Array.from(
  document.querySelectorAll(".tab-button")
);

const panels = Array.from(
  document.querySelectorAll(".tab-panel")
);

const activeCommand =
  document.querySelector("#active-command");

const footerPosition =
  document.querySelector("#footer-position");

const commandByTab = {
  introduction: "cat README.md",
  engine: "inspect matching_engine",
  "market-data": "tail -f market_data.log",
  routing: "run smart_order_router",
  "record-replay": "replay --interactive",
  execution: "inspect execution_pipeline",
  protocols: "fix-session --status",
  observability: "metrics --watch",
  benchmarks: "benchmark --report",
  building: "git log --reverse --stat"
};

let activeIndex = 0;

function activateTab(index, updateHash = true) {
  const normalized =
    (index + tabs.length) % tabs.length;

  activeIndex = normalized;

  const selected = tabs[normalized];
  const targetId = selected.dataset.tab;

  tabs.forEach((tab, tabIndex) => {
    const active = tabIndex === normalized;

    tab.classList.toggle("active", active);
    tab.setAttribute(
      "aria-selected",
      active ? "true" : "false"
    );
  });

  panels.forEach((panel) => {
    panel.classList.toggle(
      "active",
      panel.id === targetId
    );
  });

  activeCommand.textContent =
    commandByTab[targetId] || "cat README.md";

  footerPosition.textContent =
    `tab ${String(normalized).padStart(2, "0")} / ` +
    `${String(tabs.length - 1).padStart(2, "0")}`;

  if (updateHash) {
    history.replaceState(
      null,
      "",
      `#${targetId}`
    );
  }

  const contentScroll =
    document.querySelector(".content-scroll");

  contentScroll.scrollTo({
    top: 0,
    behavior: "instant"
  });
}

tabs.forEach((tab, index) => {
  tab.addEventListener("click", () => {
    activateTab(index);
  });
});

document.addEventListener("keydown", (event) => {
  const target = event.target;

  const isTyping =
    target instanceof HTMLInputElement ||
    target instanceof HTMLTextAreaElement ||
    target instanceof HTMLSelectElement;

  if (isTyping) {
    return;
  }

  if (
    event.key === "j" ||
    event.key === "ArrowDown"
  ) {
    event.preventDefault();
    activateTab(activeIndex + 1);
  }

  if (
    event.key === "k" ||
    event.key === "ArrowUp"
  ) {
    event.preventDefault();
    activateTab(activeIndex - 1);
  }

  if (event.key === "Enter") {
    tabs[activeIndex].click();
  }

  const numeric = Number(event.key);

  if (
    Number.isInteger(numeric) &&
    numeric >= 0 &&
    numeric < tabs.length
  ) {
    activateTab(numeric);
  }
});

function initializeFromHash() {
  const targetId =
    window.location.hash.replace("#", "");

  const index = tabs.findIndex(
    (tab) => tab.dataset.tab === targetId
  );

  activateTab(
    index >= 0 ? index : 0,
    false
  );
}

initializeFromHash();

const replayButtons =
  document.querySelectorAll(
    "[data-replay-command], [data-speed]"
  );

replayButtons.forEach((button) => {
  button.addEventListener("click", () => {
    replayButtons.forEach((candidate) => {
      candidate.classList.remove("active");
    });

    button.classList.add("active");

    const command =
      button.dataset.replayCommand;

    const speed =
      button.dataset.speed;

    const label =
      command || `${speed}x`;

    const metadata =
      document.querySelector(
        ".replay-meta span:last-child"
      );

    if (metadata) {
      metadata.textContent =
        `preview control: ${label}`;
    }
  });
});
