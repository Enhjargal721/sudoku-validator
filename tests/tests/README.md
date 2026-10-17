# Sudoku Validator Tests

This directory documents the test cases used to verify the multithreaded Sudoku validator.

## Test 1: Valid Sudoku

Input: A completed 9×9 Sudoku grid with digits 1–9 appearing exactly once in each row, column, and 3×3 subgrid.

Expected result: All checks pass and the program displays `Sudoku result: VALID`.

## Test 2: Invalid Sudoku

Input: A Sudoku grid containing a duplicate digit in a row.

Expected result: The affected checks fail and the program displays `Sudoku result: INVALID`.

## Test 3: Out-of-range value

Input: A Sudoku grid containing a value outside the range 1–9.

Expected result: The program marks the relevant checks as failed and displays `Sudoku result: INVALID`.
