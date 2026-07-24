import {
  Component,
  type ErrorInfo,
  type ReactNode,
} from "react";

interface Props {
  children: ReactNode;
}

interface State {
  error: Error | null;
}

export default class AppErrorBoundary extends Component<
  Props,
  State
> {
  state: State = {
    error: null,
  };

  static getDerivedStateFromError(
    error: Error
  ): State {
    return {
      error,
    };
  }

  componentDidCatch(
    error: Error,
    info: ErrorInfo
  ) {
    console.error(
      "MiniMatch render failure",
      error,
      info
    );
  }

  private handleRetry = () => {
    this.setState({
      error: null,
    });
  };

  private handleReload = () => {
    window.location.reload();
  };

  render() {
    if (!this.state.error) {
      return this.props.children;
    }

    return (
      <div className="app-failure">
        <div className="app-failure__panel">
          <span className="eyebrow">
            INTERFACE RECOVERY
          </span>

          <h1>
            MiniMatch encountered a display error.
          </h1>

          <p>
            The trading system may still be running.
            This interface component failed while rendering.
          </p>

          <div className="app-failure__error">
            {this.state.error.message}
          </div>

          <div className="app-failure__actions">
            <button
              type="button"
              onClick={this.handleRetry}
            >
              RETRY INTERFACE
            </button>

            <button
              type="button"
              onClick={this.handleReload}
            >
              RELOAD APPLICATION
            </button>
          </div>
        </div>
      </div>
    );
  }
}
