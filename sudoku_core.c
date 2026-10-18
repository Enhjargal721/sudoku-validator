
#include <stdio.h>
#include <pthread.h>
#include "sudoku_core.h"

int sudoku[SIZE][SIZE];
int results[TOTAL_THREADS];

/* Мөр бүрийг шалгах */
void *check_rows(void *arg)
{
    (void)arg;

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

/* Багана бүрийг шалгах */
void *check_columns(void *arg)
{
    (void)arg;

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

/* 3x3 дэд хүснэгт бүрийг шалгах */
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

/* 11 pthread ашиглан Sudoku-г шалгах */
int validate_sudoku(void)
{
    pthread_t threads[TOTAL_THREADS];
    int subgrid_ids[9];
    int created = 0;

    for (int i = 0; i < TOTAL_THREADS; i++) {
        results[i] = 0;
    }

    /* Мөр шалгах thread */
    if (pthread_create(&threads[0], NULL, check_rows, NULL) != 0) {
        return -1;
    }
    created++;

    /* Багана шалгах thread */
    if (pthread_create(&threads[1], NULL, check_columns, NULL) != 0) {
        for (int i = 0; i < created; i++) {
            pthread_join(threads[i], NULL);
        }
        return -1;
    }
    created++;

    /* Есөн 3x3 дэд хүснэгт шалгах thread */
    for (int i = 0; i < 9; i++) {
        subgrid_ids[i] = i;

        if (pthread_create(
                &threads[i + 2],
                NULL,
                check_subgrid,
                &subgrid_ids[i]) != 0) {

            for (int j = 0; j < created; j++) {
                pthread_join(threads[j], NULL);
            }

            return -1;
        }

        created++;
    }

    /* Бүх thread дуусахыг хүлээх */
    for (int i = 0; i < TOTAL_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            return -1;
        }
    }

    /* Бүх шалгалт амжилттай бол 1, үгүй бол 0 */
    for (int i = 0; i < TOTAL_THREADS; i++) {
        if (results[i] == 0) {
            return 0;
        }
    }

    return 1;
}
