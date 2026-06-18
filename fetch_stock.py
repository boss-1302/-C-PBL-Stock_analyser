import yfinance as yf

ticker = input("Enter stock ticker (e.g. AAPL): ").strip().upper()
period = input("Enter period (e.g. 1y, 6mo, 3mo, 1mo): ").strip() or "1y"

stock = yf.Ticker(ticker)
hist = stock.history(period=period)

if hist.empty:
    print(f"No data found for {ticker}")
else:
    hist = hist.reset_index()
    hist.columns = [c.replace(" ", "_") for c in hist.columns]
    filename = f"{ticker}.csv"
    hist.to_csv(filename, index=False)
    print(f"Saved {filename} — {len(hist)} rows")
