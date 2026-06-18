#include "analytics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define ROWS 15
#define MAX_COLS 80

double calculate_sma(const Stock *stock, int target_index, int window) {
    if (!stock || target_index < 0 || target_index >= stock->record_count || window <= 0) {
        return 0.0;
    }
    int start = target_index - window + 1;
    if (start < 0) {
        return 0.0; // Not enough elements
    }
    double sum = 0.0;
    for (int i = start; i <= target_index; i++) {
        sum += stock->records[i].close;
    }
    return sum / window;
}

void generate_trading_signal(const Stock *stock, char *output_signal, size_t buf_size) {
    if (!stock || stock->record_count <= 0 || buf_size < SIGNAL_BUF) {
        if (buf_size > 0) {
            strncpy(output_signal, "HOLD", buf_size - 1);
            output_signal[buf_size - 1] = '\0';
        }
        return;
    }

    int latest_idx = stock->record_count - 1;
    double sma5 = calculate_sma(stock, latest_idx, 5);
    double sma20 = calculate_sma(stock, latest_idx, 20);

    if (sma5 == 0.0 || sma20 == 0.0) {
        strncpy(output_signal, "HOLD", buf_size - 1);
    } else if (sma5 > sma20) {
        strncpy(output_signal, "BUY", buf_size - 1);
    } else if (sma5 < sma20) {
        strncpy(output_signal, "SELL", buf_size - 1);
    } else {
        strncpy(output_signal, "HOLD", buf_size - 1);
    }
    output_signal[buf_size - 1] = '\0';
}

double calculate_rsi(const Stock *stock, int target_index, int period) {
    if (!stock || target_index < period || target_index >= stock->record_count || period <= 0)
        return 50.0;  // neutral default

    double avg_gain = 0.0, avg_loss = 0.0;

    // First average gain/loss over the initial period
    for (int i = target_index - period + 1; i <= target_index; i++) {
        double change = stock->records[i].close - stock->records[i - 1].close;
        if (change > 0) avg_gain += change;
        else            avg_loss -= change;  // abs value
    }
    avg_gain /= period;
    avg_loss /= period;

    if (avg_loss == 0.0) return 100.0;  // no losses at all
    double rs = avg_gain / avg_loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

void generate_enhanced_signal(const Stock *stock, char *output_signal, size_t buf_size) {
    if (!stock || stock->record_count <= 0 || buf_size < SIGNAL_BUF) {
        if (buf_size > 0) {
            strncpy(output_signal, "HOLD", buf_size - 1);
            output_signal[buf_size - 1] = '\0';
        }
        return;
    }

    int latest_idx = stock->record_count - 1;
    double sma5 = calculate_sma(stock, latest_idx, 5);
    double sma20 = calculate_sma(stock, latest_idx, 20);
    double rsi = calculate_rsi(stock, latest_idx, 14);

    if (sma5 == 0.0 || sma20 == 0.0) {
        strncpy(output_signal, "HOLD", buf_size - 1);
    } else if (sma5 > sma20 && rsi < 70.0) {
        strncpy(output_signal, "BUY", buf_size - 1);
    } else if (sma5 < sma20 && rsi > 30.0) {
        strncpy(output_signal, "SELL", buf_size - 1);
    } else {
        strncpy(output_signal, "HOLD", buf_size - 1);
    }
    output_signal[buf_size - 1] = '\0';
}

static double calculate_growth_30d(const Stock *stock) {
    if (!stock || stock->record_count <= 1) {
        return 0.0;
    }
    int latest_idx = stock->record_count - 1;
    int prev_idx = latest_idx - 30;
    if (prev_idx < 0) {
        prev_idx = 0; // Fallback to oldest record
    }
    double old_close = stock->records[prev_idx].close;
    double new_close = stock->records[latest_idx].close;
    if (old_close == 0.0) {
        return 0.0;
    }
    return ((new_close - old_close) / old_close) * 100.0;
}

static int compare_performance(const void *a, const void *b) {
    Stock *stockA = *(Stock**)a;
    Stock *stockB = *(Stock**)b;
    double growthA = calculate_growth_30d(stockA);
    double growthB = calculate_growth_30d(stockB);
    if (growthA < growthB) return 1;
    if (growthA > growthB) return -1;
    return 0;
}

void rank_market_performers(Market *market) {
    if (!market || market->stock_count <= 0) {
        printf("No assets loaded in market database.\n");
        return;
    }

    qsort(market->stocks, market->stock_count, sizeof(Stock*), compare_performance);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Title border in Cyan
    SetConsoleTextAttribute(hConsole, 11);
    printf("=======================================================================\n");
    printf("                   30-DAY MARKET PERFORMANCE LEADERBOARD               \n");
    printf("=======================================================================\n");
    SetConsoleTextAttribute(hConsole, 7);

    printf("%-5s | %-10s | %-12s | %-15s | %-10s\n", "Rank", "Ticker", "Latest Close", "30d Growth %", "Signal");
    printf("-----------------------------------------------------------------------\n");

    for (int i = 0; i < market->stock_count; i++) {
        Stock *s = market->stocks[i];
        double latest_close = s->records[s->record_count - 1].close;
        double growth = calculate_growth_30d(s);
        char signal[SIGNAL_BUF];
        generate_enhanced_signal(s, signal, sizeof(signal));

        printf("%-5d | %-10s | $%-11.2f | ", i + 1, s->ticker, latest_close);

        // Highlight Growth column
        if (growth > 0) {
            SetConsoleTextAttribute(hConsole, 2); // Green
            printf("+%-14.2f%%", growth);
        } else if (growth < 0) {
            SetConsoleTextAttribute(hConsole, 4); // Red
            printf("%-15.2f%%", growth);
        } else {
            SetConsoleTextAttribute(hConsole, 7); // Default
            printf("%-15.2f%%", growth);
        }
        SetConsoleTextAttribute(hConsole, 7);

        printf(" | ");

        // Highlight Signal
        if (strcmp(signal, "BUY") == 0) {
            SetConsoleTextAttribute(hConsole, 2); // Green
            printf("[BUY]");
        } else if (strcmp(signal, "SELL") == 0) {
            SetConsoleTextAttribute(hConsole, 4); // Red
            printf("[SELL]");
        } else {
            SetConsoleTextAttribute(hConsole, 7); // Default
            printf("[HOLD]");
        }
        SetConsoleTextAttribute(hConsole, 7);
        printf("\n");
    }
    printf("-----------------------------------------------------------------------\n");
}

void render_ascii_trend(const Stock *stock, int datapoints) {
    if (!stock || stock->record_count <= 0 || datapoints <= 0) {
        printf("No data to plot.\n");
        return;
    }

    int start_idx = stock->record_count - datapoints;
    if (start_idx < 0) start_idx = 0;
    int num_points = stock->record_count - start_idx;

    if (num_points <= 1) {
        printf("Not enough data to plot a trend.\n");
        return;
    }

    double min_val = stock->records[start_idx].close;
    double max_val = stock->records[start_idx].close;
    for (int i = 0; i < num_points; i++) {
        double val = stock->records[start_idx + i].close;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    if (max_val == min_val) {
        min_val -= 1.0;
        max_val += 1.0;
    }

    char grid[ROWS][MAX_COLS];
    memset(grid, ' ', sizeof(grid));

    for (int i = 0; i < num_points; i++) {
        double val = stock->records[start_idx + i].close;
        double ratio = (val - min_val) / (max_val - min_val);
        int r = (ROWS - 1) - (int)(ratio * (ROWS - 1) + 0.5);
        if (r < 0) r = 0;
        if (r >= ROWS) r = ROWS - 1;
        grid[r][i] = '*';
    }

    printf("\n%s Closing Trend Graph (%d days):\n", stock->ticker, num_points);
    for (int r = 0; r < ROWS; r++) {
        if (r == 0) {
            printf("%9.2f | ", max_val);
        } else if (r == ROWS - 1) {
            printf("%9.2f | ", min_val);
        } else if (r == ROWS / 2) {
            printf("%9.2f | ", (max_val + min_val) / 2.0);
        } else {
            printf("          | ");
        }

        for (int c = 0; c < num_points; c++) {
            printf("%c", grid[r][c]);
        }
        printf("\n");
    }
    printf("          +");
    for (int i = 0; i < num_points; i++) {
        printf("-");
    }
    printf("\n");
    printf("           Date: %s to %s\n", stock->records[start_idx].date, stock->records[stock->record_count - 1].date);
}

void export_analytical_report(const Stock *stock) {
    if (!stock || stock->record_count <= 0) {
        printf("No stock data to export.\n");
        return;
    }

    // Create exports directory
    CreateDirectoryA("exports", NULL);

    char filepath[64];
    snprintf(filepath, sizeof(filepath), "exports/%s_report.txt", stock->ticker);

    FILE *file = fopen(filepath, "w");
    if (!file) {
        printf("Failed to create report file: %s\n", filepath);
        return;
    }

    double latest_close = stock->records[stock->record_count - 1].close;
    double latest_open = stock->records[stock->record_count - 1].open;
    double latest_high = stock->records[stock->record_count - 1].high;
    double latest_low = stock->records[stock->record_count - 1].low;
    unsigned long latest_vol = stock->records[stock->record_count - 1].volume;
    double growth = calculate_growth_30d(stock);
    char signal[SIGNAL_BUF];
    generate_enhanced_signal(stock, signal, sizeof(signal));

    double sma5 = calculate_sma(stock, stock->record_count - 1, 5);
    double sma20 = calculate_sma(stock, stock->record_count - 1, 20);
    double rsi = calculate_rsi(stock, stock->record_count - 1, 14);

    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(file, "========================================================\n");
    fprintf(file, "               STOCK ANALYTICS REPORT: %s\n", stock->ticker);
    fprintf(file, "========================================================\n");
    fprintf(file, "Export Date/Time: %s\n", time_buffer);
    fprintf(file, "Ticker:           %s\n", stock->ticker);
    fprintf(file, "Total Records:    %d days\n", stock->record_count);
    fprintf(file, "Date Range:       %s to %s\n", stock->records[0].date, stock->records[stock->record_count - 1].date);
    fprintf(file, "--------------------------------------------------------\n");
    fprintf(file, "LATEST SESSION VALUES (%s):\n", stock->records[stock->record_count - 1].date);
    fprintf(file, "  Open:           $%.2f\n", latest_open);
    fprintf(file, "  High:           $%.2f\n", latest_high);
    fprintf(file, "  Low:            $%.2f\n", latest_low);
    fprintf(file, "  Close:          $%.2f\n", latest_close);
    fprintf(file, "  Volume:         %lu shares\n", latest_vol);
    fprintf(file, "--------------------------------------------------------\n");
    fprintf(file, "TECHNICAL INDICATORS:\n");
    fprintf(file, "  5-day SMA:      $%.2f\n", sma5);
    fprintf(file, "  20-day SMA:     $%.2f\n", sma20);
    fprintf(file, "  RSI-14:         %.2f\n", rsi);
    fprintf(file, "  30-day Growth:  %.2f%%\n", growth);
    fprintf(file, "  Current Signal: %s\n", signal);
    fprintf(file, "========================================================\n");

    // Add ASCII chart to report
    int datapoints = 40; // use a fixed width of 40 in file report
    int start_idx = stock->record_count - datapoints;
    if (start_idx < 0) start_idx = 0;
    int num_points = stock->record_count - start_idx;

    if (num_points > 1) {
        double min_val = stock->records[start_idx].close;
        double max_val = stock->records[start_idx].close;
        for (int i = 0; i < num_points; i++) {
            double val = stock->records[start_idx + i].close;
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }
        if (max_val == min_val) {
            min_val -= 1.0;
            max_val += 1.0;
        }

        char grid[ROWS][MAX_COLS];
        memset(grid, ' ', sizeof(grid));

        for (int i = 0; i < num_points; i++) {
            double val = stock->records[start_idx + i].close;
            double ratio = (val - min_val) / (max_val - min_val);
            int r = (ROWS - 1) - (int)(ratio * (ROWS - 1) + 0.5);
            if (r < 0) r = 0;
            if (r >= ROWS) r = ROWS - 1;
            grid[r][i] = '*';
        }

        fprintf(file, "\n%s Closing Trend Graph (%d days):\n", stock->ticker, num_points);
        for (int r = 0; r < ROWS; r++) {
            if (r == 0) {
                fprintf(file, "%9.2f | ", max_val);
            } else if (r == ROWS - 1) {
                fprintf(file, "%9.2f | ", min_val);
            } else if (r == ROWS / 2) {
                fprintf(file, "%9.2f | ", (max_val + min_val) / 2.0);
            } else {
                fprintf(file, "          | ");
            }

            for (int c = 0; c < num_points; c++) {
                fprintf(file, "%c", grid[r][c]);
            }
            fprintf(file, "\n");
        }
        fprintf(file, "          +");
        for (int i = 0; i < num_points; i++) {
            fprintf(file, "-");
        }
        fprintf(file, "\n");
        fprintf(file, "           Date: %s to %s\n", stock->records[start_idx].date, stock->records[stock->record_count - 1].date);
        fprintf(file, "========================================================\n");
    }

    fclose(file);
    printf("Report successfully exported to: %s\n", filepath);
}
