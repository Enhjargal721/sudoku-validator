# Multithreaded Sudoku Solution Validator

## Project Description

Энэхүү төсөл нь үйлдлийн системийн хичээлийн хүрээнд C хэл болон POSIX Threads (`pthread`) ашиглан 9×9 хэмжээтэй Sudoku шийдлийг шалгах програм юм.

Програм нь нийт **11 thread** ашиглана:
- 1 thread — бүх мөрийг шалгана.
- 1 thread — бүх баганыг шалгана.
- 9 thread — тус бүр нэг 3×3 дэд хүснэгтийг шалгана.

## Features

- Хэрэглэгчээс 9×9 Sudoku өгөгдөл авна.
- Мөр, багана, 3×3 дэд хүснэгтүүдийг шалгана.
- Шалгалтын үр дүнг `PASS` эсвэл `FAIL` гэж харуулна.
- Эцсийн үр дүнг `VALID` эсвэл `INVALID` гэж гаргана.
- Програмыг хаалгүйгээр дахин Sudoku шалгах боломжтой.

## Requirements

- C compiler (GCC)
- POSIX Threads (`pthread`)
- Windows үйлдлийн систем дээр MSYS2 UCRT64 орчин

## How to Run

MSYS2 UCRT64 терминалыг нээгээд:

```bash
cd /c/OS-BD1/sudoku-validator
gcc main.c -o sudoku_validator.exe -pthread
./sudoku_validator.exe
```

## How to Use

1. Програмыг ажиллуулна.
2. `Row 1`-ээс `Row 9` хүртэл мөр бүрт зайгаар тусгаарласан 9 бүхэл тоо оруулна.
3. Програм шалгалтын үр дүнг харуулна.
4. `Check another Sudoku? (y/n)` гэсэн асуултад:
   - `y` — дахин Sudoku шалгах
   - `n` — програмаас гарах

## Test Cases

**Test 1 — Valid Sudoku**

Зөв Sudoku оруулахад бүх шалгалт `PASS` болж, `Sudoku result: VALID` гарна.

**Test 2 — Invalid Sudoku**

Давхардсан тоотой Sudoku оруулахад холбогдох шалгалтууд `FAIL` болж, `Sudoku result: INVALID` гарна.

## AI Usage

Төслийн код боловсруулах, тайлбарлах болон баримтжуулалтад ChatGPT-ийн тусламж ашигласан.
