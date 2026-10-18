
#ifndef SUDOKU_CORE_H
#define SUDOKU_CORE_H

#define SIZE 9
#define TOTAL_THREADS 11

extern int sudoku[SIZE][SIZE];
extern int results[TOTAL_THREADS];

void *check_rows(void *arg);
void *check_columns(void *arg);
void *check_subgrid(void *arg);

int validate_sudoku(void);

#endif
