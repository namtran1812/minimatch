import type { ReactNode } from "react";

type PageHeaderItem = {
  label: string;
  content: ReactNode;
};

type PageHeaderProps = {
  eyebrow: ReactNode;
  title: ReactNode;
  actions?: ReactNode;
  items?: PageHeaderItem[];
  children?: ReactNode;
};

export default function PageHeader({
  eyebrow,
  title,
  actions,
  items,
  children,
}: PageHeaderProps) {
  const hasItems =
    Array.isArray(items) &&
    items.length > 0;

  return (
    <header className="mm-page-header">
      <div className="mm-page-header__top">
        <div className="mm-page-header__identity">
          <span className="eyebrow">
            {eyebrow}
          </span>

          <h1>{title}</h1>
        </div>

        {actions && (
          <div className="mm-page-header__actions">
            {actions}
          </div>
        )}
      </div>

      {hasItems && (
        <div className="mm-page-header__meta">
          {items.map((item) => (
            <div key={item.label}>
              <span className="eyebrow">
                {item.label}
              </span>

              <p>{item.content}</p>
            </div>
          ))}
        </div>
      )}

      {!hasItems && children && (
        <div className="mm-page-header__meta">
          {children}
        </div>
      )}
    </header>
  );
}
