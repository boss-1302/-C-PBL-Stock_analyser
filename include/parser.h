#ifndef PARSER_H
#define PARSER_H

#include "common.h"

Stock* load_stock_from_csv(const char *filepath, const char *ticker);
void free_stock(Stock *stock);
void free_market(Market *market);

#endif /* PARSER_H */
