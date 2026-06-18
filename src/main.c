#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <locale.h>

#include "common.h"
#include "parser.h"
#include "analytics.h"

void set_cursor_position(int x, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hConsole, coord);
}

void clear_console() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire buffer with spaces
    if (!FillConsoleOutputCharacterA(hConsole, ' ', cellCount, homeCoords, &count)) return;

    // Fill the entire buffer with the current attributes
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) return;

    // Move the cursor home
    SetConsoleCursorPosition(hConsole, homeCoords);
}

void display_portfolio_table(Market *market) {
    if (!market || market->stock_count <= 0) {
        printf("No assets loaded in market database.\n");
        return;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hConsole, 11); // Cyan borders
    printf("=================================================================================\n");
    printf("                          PORTFOLIO COMPARISON TABLE                             \n");
    printf("=================================================================================\n");
    SetConsoleTextAttribute(hConsole, 7);

    printf("TICKER  |  RECORDS  |  LATEST      |  30d GROWTH  |  SMA5    |  SMA20   |  RSI  |  SIGNAL\n");
    printf("--------+-----------+-------------+--------------+---------+----------+-------+---------\n");

    for (int i = 0; i < market->stock_count; i++) {
        Stock *s = market->stocks[i];
        int latest_idx = s->record_count - 1;
        double latest_close = s->records[latest_idx].close;
        double sma5 = calculate_sma(s, latest_idx, 5);
        double sma20 = calculate_sma(s, latest_idx, 20);
        double rsi = calculate_rsi(s, latest_idx, 14);
        char signal[SIGNAL_BUF];
        generate_enhanced_signal(s, signal, sizeof(signal));

        double growth = 0.0;
        if (s->record_count > 1) {
            int prev_idx = latest_idx - 30;
            if (prev_idx < 0) prev_idx = 0;
            double old_close = s->records[prev_idx].close;
            if (old_close != 0.0) {
                growth = ((latest_close - old_close) / old_close) * 100.0;
            }
        }

        printf("%-8s|   %-8d|  $%-10.2f|  ", s->ticker, s->record_count, latest_close);
        
        if (growth > 0) {
            SetConsoleTextAttribute(hConsole, 2); // Green
            printf("+%-10.2f%%", growth);
        } else if (growth < 0) {
            SetConsoleTextAttribute(hConsole, 4); // Red
            printf("%-11.2f%%", growth);
        } else {
            printf("%-11.2f%%", growth);
        }
        SetConsoleTextAttribute(hConsole, 7);

        printf(" |  %-7.2f|  %-8.2f|  %-5.0f|  ", sma5, sma20, rsi);

        if (strcmp(signal, "BUY") == 0) {
            SetConsoleTextAttribute(hConsole, 2); // Green
            printf("BUY");
        } else if (strcmp(signal, "SELL") == 0) {
            SetConsoleTextAttribute(hConsole, 4); // Red
            printf("SELL");
        } else {
            printf("HOLD");
        }
        SetConsoleTextAttribute(hConsole, 7);
        printf("\n");
    }
    SetConsoleTextAttribute(hConsole, 11);
    printf("=================================================================================\n");
    SetConsoleTextAttribute(hConsole, 7);
}

void print_main_menu(Market *market) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Cyan borders
    SetConsoleTextAttribute(hConsole, 11);
    printf("=======================================================================\n");
    printf("                  STOCK EXCHANGE ANALYSER - DASHBOARD                 \n");
    printf("=======================================================================\n");
    SetConsoleTextAttribute(hConsole, 7);

    printf(" Loaded Assets Count: ");
    SetConsoleTextAttribute(hConsole, 11);
    printf("%d\n", market->stock_count);
    SetConsoleTextAttribute(hConsole, 7);
    printf("-----------------------------------------------------------------------\n");
    printf(" 1. Load Stock CSV Asset Dataset\n");
    printf(" 2. View Loaded Assets Matrix & 30-Day Leaderboard\n");
    printf(" 3. View Comprehensive Detailed Analytics & Visual ASCII Graph\n");
    printf(" 4. Export Analytical Text Report to File\n");
    printf(" 5. Clear Memory & Exit\n");
    printf(" 6. View Full Portfolio Comparison Table\n");
    printf("-----------------------------------------------------------------------\n");
    printf(" Enter your choice (1-6): ");
}

int main() {
    setlocale(LC_ALL, "");
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    setvbuf(stdout, NULL, _IONBF, 0);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("Stock Exchange Analyser");

    Market *market = (Market*)malloc(sizeof(Market));
    if (!market) {
        fprintf(stderr, "Fatal: failed to allocate market matrix.\n");
        return 1;
    }
    market->stock_count = 0;
    market->max_capacity = INITIAL_CAPACITY;
    market->stocks = (Stock**)malloc(market->max_capacity * sizeof(Stock*));
    if (!market->stocks) {
        free(market);
        fprintf(stderr, "Fatal: failed to allocate market stocks array.\n");
        return 1;
    }

    int run = 1;
    clear_console();

    while (run) {
        set_cursor_position(0, 0);
        print_main_menu(market);

        char ch = _getch();
        if (ch == '1') {
            clear_console();
            
            SetConsoleTextAttribute(hConsole, 11);
            printf(">>> LOAD STOCK CSV ASSET DATASET <<<\n");
            printf("------------------------------------\n");
            SetConsoleTextAttribute(hConsole, 7);

            char filepath[260];
            char ticker[16];

            printf("Enter CSV File Path (e.g. AAPL.csv): ");
            if (fgets(filepath, sizeof(filepath), stdin)) {
                filepath[strcspn(filepath, "\r\n")] = '\0';
            }

            printf("Enter Asset Ticker Symbol (e.g. AAPL): ");
            if (fgets(ticker, sizeof(ticker), stdin)) {
                ticker[strcspn(ticker, "\r\n")] = '\0';
            }

            int exists = -1;
            for (int i = 0; i < market->stock_count; i++) {
                if (_stricmp(market->stocks[i]->ticker, ticker) == 0) {
                    exists = i;
                    break;
                }
            }

            printf("\nLoading asset '%s' from '%s'...\n", ticker, filepath);
            Stock *loaded = load_stock_from_csv(filepath, ticker);
            if (loaded) {
                if (exists != -1) {
                    free_stock(market->stocks[exists]);
                    market->stocks[exists] = loaded;
                    SetConsoleTextAttribute(hConsole, 2);
                    printf("Success: Replaced existing stock '%s' with newly loaded data (%d records).\n", ticker, loaded->record_count);
                } else {
                    if (market->stock_count >= market->max_capacity) {
                        int new_capacity = market->max_capacity * 2;
                        Stock **new_stocks = (Stock**)realloc(market->stocks, new_capacity * sizeof(Stock*));
                        if (!new_stocks) {
                            SetConsoleTextAttribute(hConsole, 4);
                            printf("Error: Memory allocation failure while expanding market capacity.\n");
                            free_stock(loaded);
                        } else {
                            market->stocks = new_stocks;
                            market->max_capacity = new_capacity;
                        }
                    }
                    
                    if (market->stock_count < market->max_capacity) {
                        market->stocks[market->stock_count++] = loaded;
                        SetConsoleTextAttribute(hConsole, 2);
                        printf("Success: Loaded stock '%s' with %d records.\n", ticker, loaded->record_count);
                    }
                }
            } else {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Failed to load CSV file. Check file path, headers, or structure.\n");
            }
            SetConsoleTextAttribute(hConsole, 7);
            printf("\nPress any key to return to main menu...");
            _getch();
            clear_console();

        } else if (ch == '2') {
            clear_console();
            rank_market_performers(market);
            printf("\nPress any key to return to main menu...");
            _getch();
            clear_console();

        } else if (ch == '3') {
            clear_console();
            SetConsoleTextAttribute(hConsole, 11);
            printf(">>> DETAILED STOCK ANALYTICS & GRAPH <<<\n");
            printf("----------------------------------------\n");
            SetConsoleTextAttribute(hConsole, 7);

            if (market->stock_count == 0) {
                printf("No assets currently loaded. Please load a CSV first.\n");
            } else {
                char ticker[16];
                printf("Enter Loaded Stock Ticker (e.g. AAPL): ");
                if (fgets(ticker, sizeof(ticker), stdin)) {
                    ticker[strcspn(ticker, "\r\n")] = '\0';
                }

                Stock *s = NULL;
                for (int i = 0; i < market->stock_count; i++) {
                    if (_stricmp(market->stocks[i]->ticker, ticker) == 0) {
                        s = market->stocks[i];
                        break;
                    }
                }

                if (s) {
                    double latest_close = s->records[s->record_count - 1].close;
                    double sma5 = calculate_sma(s, s->record_count - 1, 5);
                    double sma20 = calculate_sma(s, s->record_count - 1, 20);
                    double rsi = calculate_rsi(s, s->record_count - 1, 14);
                    char signal[SIGNAL_BUF];
                    generate_enhanced_signal(s, signal, sizeof(signal));

                    printf("\n");
                    printf("Ticker:           %s\n", s->ticker);
                    printf("Latest Session:   %s | Close: $%.2f\n", s->records[s->record_count - 1].date, latest_close);
                    printf("5-day SMA:        $%.2f\n", sma5);
                    printf("20-day SMA:       $%.2f\n", sma20);
                    printf("RSI-14:           %.2f\n", rsi);
                    printf("Current Signal:   ");
                    if (strcmp(signal, "BUY") == 0) {
                        SetConsoleTextAttribute(hConsole, 2);
                        printf("%s\n", signal);
                    } else if (strcmp(signal, "SELL") == 0) {
                        SetConsoleTextAttribute(hConsole, 4);
                        printf("%s\n", signal);
                    } else {
                        SetConsoleTextAttribute(hConsole, 7);
                        printf("%s\n", signal);
                    }
                    SetConsoleTextAttribute(hConsole, 7);

                    int datapoints = 40;
                    char dp_str[16];
                    printf("Enter number of data points to plot (e.g. 50): ");
                    if (fgets(dp_str, sizeof(dp_str), stdin)) {
                        int val = atoi(dp_str);
                        if (val > 0) datapoints = val;
                    }
                    render_ascii_trend(s, datapoints);
                } else {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: Stock ticker '%s' not found in loaded memory.\n", ticker);
                    SetConsoleTextAttribute(hConsole, 7);
                }
            }

            printf("\nPress any key to return to main menu...");
            _getch();
            clear_console();

        } else if (ch == '4') {
            clear_console();
            SetConsoleTextAttribute(hConsole, 11);
            printf(">>> EXPORT ANALYTICAL TEXT REPORT <<<\n");
            printf("-------------------------------------\n");
            SetConsoleTextAttribute(hConsole, 7);

            if (market->stock_count == 0) {
                printf("No assets currently loaded. Please load a CSV first.\n");
            } else {
                char ticker[16];
                printf("Enter Loaded Stock Ticker to Export (e.g. AAPL): ");
                if (fgets(ticker, sizeof(ticker), stdin)) {
                    ticker[strcspn(ticker, "\r\n")] = '\0';
                }

                Stock *s = NULL;
                for (int i = 0; i < market->stock_count; i++) {
                    if (_stricmp(market->stocks[i]->ticker, ticker) == 0) {
                        s = market->stocks[i];
                        break;
                    }
                }

                if (s) {
                    export_analytical_report(s);
                } else {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: Stock ticker '%s' not found in loaded memory.\n", ticker);
                    SetConsoleTextAttribute(hConsole, 7);
                }
            }

            printf("\nPress any key to return to main menu...");
            _getch();
            clear_console();

        } else if (ch == '5') {
            clear_console();
            printf("Clearing memory matrices...\n");
            free_market(market);
            SetConsoleTextAttribute(hConsole, 10);
            printf("Goodbye!\n");
            SetConsoleTextAttribute(hConsole, 7);
            run = 0;
        } else if (ch == '6') {
            clear_console();
            display_portfolio_table(market);
            printf("\nPress any key to return to main menu...");
            _getch();
            clear_console();
        }
    }

    return 0;
}
