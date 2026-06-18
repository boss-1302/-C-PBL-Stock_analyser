# Stock Exchange Analyser

C program that loads historical stock data, calculates technical indicators, and generates BUY/SELL/HOLD trading signals.

## Project Structure

```
stock_analyser/
├── include/
│   ├── common.h        # Structs: TradeRecord, Stock, Market
│   ├── parser.h        # Declarations for CSV loading & memory cleanup
│   └── analytics.h     # Declarations for SMA, RSI, signals, ranking, charts
├── src/
│   ├── main.c          # Menu UI, Win32 console colors, cursor control
│   ├── parser.c        # CSV parsing, dynamic memory, tokenization
│   └── analytics.c     # SMA, RSI, enhanced signals, qsort ranking, ASCII charts, report export
├── exports/            # Generated reports land here
├── Makefile
└── stock_analyser.exe
```

## How to Build

```bash
make
# or
gcc -Wall -Wextra -O2 -Iinclude src/main.c src/parser.c src/analytics.c -o stock_analyser.exe
```

## Menu Options — What Each Does & Which Functions It Uses

### Option 1 — Load Stock CSV Asset Dataset

Loads a stock's historical data from a CSV file into memory.

**Functions used:**

- `load_stock_from_csv(filepath, ticker)` in parser.c — opens the CSV, reads line-by-line with `fgets`, tokenizes with `strtok`, dynamically allocates `Stock` and `TradeRecord` arrays with `malloc`/`realloc`, doubles capacity on overflow
- `strncpy` + `trim_and_clean` — sanitizes filepath and ticker input
- `strtod` / `strtoul` — converts price and volume strings to numbers
- `_stricmp` in main.c — checks if ticker already loaded (case-insensitive)
- `free_stock` in parser.c — if replacing an existing stock, frees old data first

**C concepts:** file I/O, string tokenization, dynamic memory, structs, type conversion

---

### Option 2 — 30-Day Market Performance Leaderboard

Ranks all loaded stocks by trailing 30-day price change and displays them sorted.

**Functions used:**

- `calculate_growth_30d(stock)` in analytics.c (static) — calculates `((latest_close - old_close) / old_close) * 100`
- `compare_performance(a, b)` in analytics.c (static) — comparator for `qsort`, returns descending order
- `qsort(market->stocks, count, sizeof(Stock*), compare_performance)` — sorts the stock array in-place
- `generate_enhanced_signal(stock, signal, sizeof(signal))` in analytics.c — gets the combined SMA + RSI signal for each stock
- `SetConsoleTextAttribute(hConsole, color)` — colors growth green (positive) or red (negative), colors signal

**C concepts:** `qsort` with function pointers, static helper functions, Win32 console API

---

### Option 3 — Detailed Stock Analytics & ASCII Chart

Shows full technical breakdown + a visual price trend for a single stock.

**Functions used:**

- `calculate_sma(stock, target_index, 5)` — 5-day Simple Moving Average
- `calculate_sma(stock, target_index, 20)` — 20-day Simple Moving Average
- `calculate_rsi(stock, target_index, 14)` — 14-period Relative Strength Index

  RSI formula: RS = avg_gain / avg_loss → RSI = 100 - (100 / (1 + RS))
  Measures momentum. Above 70 = overbought, below 30 = oversold.

- `generate_enhanced_signal(stock, signal, buf_size)` — combines SMA and RSI:

  | Condition | Signal |
  |---|---|
  | SMA5 > SMA20 AND RSI < 70 | BUY |
  | SMA5 < SMA20 AND RSI > 30 | SELL |
  | everything else | HOLD |

  Two-indicator confirmation prevents false signals.

- `render_ascii_trend(stock, datapoints)` — builds a 15-row × N-column grid, maps close prices to rows via ratio, renders `*` as price points, prints min/max/mid axis labels, date range

**C concepts:** pointer safety (NULL checks), array indexing, mathematical formulas, 2D char arrays, memset

---

### Option 4 — Export Analytical Report

Generates a text file with all analytics and an ASCII trend chart.

**Functions used:**

- `export_analytical_report(stock)` in analytics.c
  - `CreateDirectoryA("exports", NULL)` — creates exports folder (Win32)
  - `snprintf` — builds filepath `exports/<TICKER>_report.txt`
  - `fopen` / `fprintf` / `fclose` — writes structured report file
  - Calls `calculate_sma`, `calculate_rsi`, `generate_enhanced_signal`, `calculate_growth_30d`
  - Embeds its own ASCII trend chart (same algorithm as render_ascii_trend)
  - `time()` / `localtime()` / `strftime()` — timestamps the report

**C concepts:** file output, formatted strings, timestamp formatting, code reuse across functions

---

### Option 5 — Clear Memory & Exit

Frees all allocated memory and terminates.

**Functions used:**

- `free_market(market)` in parser.c — iterates all stocks, calls `free_stock` on each, frees the Stock** array, frees the Market struct
- `free_stock(stock)` — frees the TradeRecord array, then frees the Stock struct
- Both are NULL-safe — safe to call on partially initialized data

**C concepts:** memory management, nested free chains, pointer safety

---

### Option 6 — Portfolio Comparison Table

Side-by-side comparison of all loaded stocks in a formatted table.

**Functions used:**

- `display_portfolio_table(market)` in main.c
- Calls `calculate_sma`, `calculate_rsi`, `generate_enhanced_signal` for each stock
- Uses growth formula inline with bounds check (handles stocks with < 30 days of data)
- `SetConsoleTextAttribute` for green/red coloring on growth and signal columns
- `printf` with formatted width specifiers (`%-8s`, `%-8d`, `$%-10.2f`) for aligned columns

**C concepts:** formatted console output, loop over array of struct pointers, color-coded display

## Technical Indicators Explained (for the viva)

### Simple Moving Average (SMA)

Average of closing prices over N days.

```
SMA = (price_1 + price_2 + ... + price_N) / N
```

We use 5-day (short-term trend) and 20-day (medium-term trend).
When 5-day crosses above 20-day = upward momentum.
When 5-day crosses below 20-day = downward momentum.

### Relative Strength Index (RSI)

Measures speed and magnitude of recent price changes.

```
RS = average_gain / average_loss over 14 days
RSI = 100 - (100 / (1 + RS))
```

- RSI > 70 → overbought (price may drop)
- RSI < 30 → oversold (price may rise)
- 30–70 → neutral zone

### Enhanced Signal Logic

Combines SMA crossover direction with RSI to avoid trading at extremes:

| SMA Signal | RSI Check | Final Decision |
|---|---|---|
| BUY (SMA5 > SMA20) | RSI < 70 → confirms uptrend has room | BUY |
| BUY (SMA5 > SMA20) | RSI >= 70 → stock is overbought, might reverse | HOLD |
| SELL (SMA5 < SMA20) | RSI > 30 → confirms downtrend has room | SELL |
| SELL (SMA5 < SMA20) | RSI <= 30 → stock is oversold, might bounce | HOLD |

This is a simplified version of what quantitative trading systems do.

## PBL Rubric Coverage

| Rubric Item | How Covered |
|---|---|
| Problem statement clarity | Stock market analysis with trading signals |
| Input design | CSV files + interactive menu |
| Logical decision-making | SMA crossover + RSI confirmation |
| Output = decisions | BUY/SELL/HOLD with colored console + text report |
| Arrays / structures | TradeRecord, Stock, Market — 3-level nested structs |
| Functions | 10+ functions across 6 files, strict .h/.c separation |
| Pointers | Double pointers, dynamic heap allocation, qsort function pointers |
| Dynamic memory | Initial capacity 32 with doubling realloc strategy |
| File handling | CSV read + formatted report export |
| Strings | strtok tokenization, strncpy safe copy, snprintf formatting |
| Control structures | Menu loop, array iteration, conditional signal logic |
| Modular design | 3 header files + 3 source files = clean separation of concerns |
