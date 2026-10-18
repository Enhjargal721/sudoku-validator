
#include <stdio.h>
#include <pthread.h>

#define SIZE 9
#define TOTAL_THREADS 11

int sudoku[SIZE][SIZE];
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

int read_sudoku(void)
{
    printf("\nEnter 81 integers (1-9) for the Sudoku grid.\n");
    printf("Enter 9 numbers per row, separated by spaces.\n\n");

    for (int row = 0; row < SIZE; row++) {
        printf("Row %d: ", row + 1);

        for (int col = 0; col < SIZE; col++) {
            if (scanf("%d", &sudoku[row][col]) != 1) {
                printf("\nError: Please enter integers only.\n");
                return 0;
            }
        }
    }

    return 1;
}

int validate_sudoku(void)
{
    pthread_t threads[TOTAL_THREADS];
    int subgrid_ids[9];

    for (int i = 0; i < TOTAL_THREADS; i++) {
        results[i] = 0;
    }

    if (pthread_create(&threads[0], NULL, check_rows, NULL) != 0) {
        printf("Error: Could not create row-check thread.\n");
        return 0;
    }

    if (pthread_create(&threads[1], NULL, check_columns, NULL) != 0) {
        pthread_join(threads[0], NULL);
        printf("Error: Could not create column-check thread.\n");
        return 0;
    }

    int created = 2;

    for (int i = 0; i < 9; i++) {
        subgrid_ids[i] = i;

        if (pthread_create(
                &threads[i + 2], NULL,
                check_subgrid, &subgrid_ids[i]) != 0) {

            for (int j = 0; j < created; j++) {
                pthread_join(threads[j], NULL);
            }

            printf("Error: Could not create subgrid thread.\n");
            return 0;
        }

        created++;
    }

    for (int i = 0; i < TOTAL_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n--- Validation Results ---\n");
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
            break;
        }
    }

    printf("\nSudoku result: %s\n",
           valid ? "VALID" : "INVALID");

    return 1;
}

int main(void)
{
    char again;

    do {
        if (!read_sudoku()) {
            return 1;
        }

        if (!validate_sudoku()) {
            return 1;
        }

        printf("\nCheck another Sudoku? (y/n): ");

        if (scanf(" %c", &again) != 1) {
            return 1;
        }

    } while (again == 'y' || again == 'Y');

    printf("\nProgram finished.\n");
    return 0;
}
