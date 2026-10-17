
#include <stdio.h>
#include <pthread.h>

#define SIZE 9
#define TOTAL_THREADS 11

int sudoku[SIZE][SIZE] = {
    {6, 2, 4, 5, 3, 9, 1, 8, 7},
    {5, 1, 9, 7, 2, 8, 6, 3, 4},
    {8, 3, 7, 6, 1, 4, 2, 9, 5},
    {1, 4, 3, 8, 6, 5, 7, 2, 9},
    {9, 5, 8, 2, 4, 7, 3, 6, 1},
    {7, 6, 2, 3, 9, 1, 4, 5, 8},
    {3, 7, 1, 9, 5, 6, 8, 4, 2},
    {4, 9, 6, 1, 8, 2, 5, 7, 3},
    {2, 8, 5, 4, 7, 3, 9, 1, 6}
};

int results[TOTAL_THREADS];

void *check_rows(void *arg)
{
    for (int row = 0; row < SIZE; row++) {
        int seen[10] = {0};

        for (int col = 0; col < SIZE; col++) {
            int value = sudoku[row][col];

            if (value < 1 || value > 9 || seen[value]) {
                results[0] = 0;
                return NULL;
            }

            seen[value] = 1;
        }
    }

    results[0] = 1;
    return NULL;
}

void *check_columns(void *arg)
{
    for (int col = 0; col < SIZE; col++) {
        int seen[10] = {0};

        for (int row = 0; row < SIZE; row++) {
            int value = sudoku[row][col];

            if (value < 1 || value > 9 || seen[value]) {
                results[1] = 0;
                return NULL;
            }

            seen[value] = 1;
        }
    }

    results[1] = 1;
    return NULL;
}

void *check_subgrid(void *arg)
{
    int index = *(int *)arg;
    int start_row = (index / 3) * 3;
    int start_col = (index % 3) * 3;
    int seen[10] = {0};

    for (int row = start_row; row < start_row + 3; row++) {
        for (int col = start_col; col < start_col + 3; col++) {
            int value = sudoku[row][col];

            if (value < 1 || value > 9 || seen[value]) {
                results[index + 2] = 0;
                return NULL;
            }

            seen[value] = 1;
        }
    }

    results[index + 2] = 1;
    return NULL;
}

int main(void)
{
    pthread_t threads[TOTAL_THREADS];
    int subgrid_ids[9];

    for (int i = 0; i < TOTAL_THREADS; i++) {
        results[i] = 0;
    }

    pthread_create(&threads[0], NULL, check_rows, NULL);
    pthread_create(&threads[1], NULL, check_columns, NULL);

    for (int i = 0; i < 9; i++) {
        subgrid_ids[i] = i;

        pthread_create(
            &threads[i + 2],
            NULL,
            check_subgrid,
            &subgrid_ids[i]
        );
    }

    for (int i = 0; i < TOTAL_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Row check: %s\n",
           results[0] ? "PASS" : "FAIL");

    printf("Column check: %s\n",
           results[1] ? "PASS" : "FAIL");

    for (int i = 0; i < 9; i++) {
        printf("3x3 subgrid %d: %s\n",
               i + 1,
               results[i + 2] ? "PASS" : "FAIL");
    }

    int valid = 1;

    for (int i = 0; i < TOTAL_THREADS; i++) {
        if (results[i] == 0) {
            valid = 0;
        }
    }

    printf("\nSudoku result: %s\n",
           valid ? "VALID" : "INVALID");

    return 0;
}
