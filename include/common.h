#ifndef COMMON_H
#define COMMON_H

#define INITIAL_CAPACITY 32
#define SIGNAL_BUF 16     // max signal string length ("BUY"/"SELL"/"HOLD" + null)

typedef struct {
    char date[11];          // "YYYY-MM-DD\0"
    double open;
    double high;
    double low;
    double close;
    unsigned long volume;
} TradeRecord;

typedef struct {
    char ticker[10];
    TradeRecord *records;    // dynamic heap array
    int record_count;
    int capacity;
} Stock;

typedef struct {
    Stock **stocks;          // dynamic array of pointers to individual loaded stocks
    int stock_count;
    int max_capacity;
} Market;

#endif /* COMMON_H */
