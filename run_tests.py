import subprocess
import sys

T = "./translator"
TIMEOUT = 5  # секунд на каждый тест

def run(label, test_file, input_data="", expected="", expect_fail=True):
    try:
        result = subprocess.run(
            [T, test_file],
            input=input_data,
            capture_output=True,
            timeout=TIMEOUT,
            encoding="utf-8", errors="replace"
        )
        output = (result.stdout + result.stderr).strip()
        ok = True
        reason = ""

        if expect_fail:
            # Для ошибочных тестов: должен быть ненулевой код возврата и текст ошибки
            ok = result.returncode != 0 and len(output) > 0
            if ok and expected:
                ok = expected.lower() in output.lower()
                if not ok:
                    reason = f" (ожидалось подстроки '{expected}' в выводе)"
        else:
            # Для успешных тестов: код возврата 0 и проверка expected
            ok = result.returncode == 0
            if ok and expected:
                ok = expected in output
                if not ok:
                    reason = f" (ожидание '{expected}', got: '{output[:80]}')"

        status = "OK  " if ok else "FAIL"
        print(f"[{status}] {label}{reason}")
        if not ok:
            print(f"         output: {output[:200]}")
            print(f"         returncode: {result.returncode}")
    except subprocess.TimeoutExpired:
        print(f"[HANG] {label}  <-- завис, убит через {TIMEOUT}с")
    except FileNotFoundError:
        print(f"[ERR ] {label}  <-- бинарник не найден!")
    print()

print("=" * 60)
print("TRANSLATOR TESTS")
print("=" * 60)
print()

# --- Успешные тесты ---
run("1.1 Сумма       (3,4 -> 7)",      "tests/test1.1.txt", "3\n4",            "7",      expect_fail=False)
run("1.2 Макс из 3  (5,3,8 -> 8)",     "tests/test1.2.txt", "5\n3\n8",         "8",      expect_fail=False)
run("1.3 Разность     (4,3 -> 1)",       "tests/test1.3.txt", "4.0\n3.0",        "1",      expect_fail=False)
run("1.4 if/else    (5,3 -> 3.2)",     "tests/test1.4.txt", "5.0\n3.0",        "3.2",    expect_fail=False)
run("2   Сортировка [5,3,1,4,1,5]",    "tests/test2.txt",   "5\n3\n1\n4\n1\n5",  "1",      expect_fail=False)

# --- Ошибочные тесты ---
run("3.1 Неизвестный символ @",        "tests/test3.1.txt", "",                 "Неизвестн", expect_fail=True)
run("3.2 Незавершён вещественный 3.",  "tests/test3.2.txt", "",                 "Неизвестн", expect_fail=True)
run("3.3 = вместо :=",                 "tests/test3.3.txt", "",                 ":=",      expect_fail=True)
run("3.4 Нет then",                    "tests/test3.4.txt", "",                 "write",   expect_fail=True)
run("3.5 Нет do",                      "tests/test3.5.txt", "",                 "i",       expect_fail=True)
run("3.6 Нет end",                     "tests/test3.6.txt", "",                 "end",     expect_fail=True)
run("3.7 Неверные скобки",             "tests/test3.7.txt", "",                 ")",       expect_fail=True)
run("3.8 Нет точки с запятой",         "tests/test3.8.txt", "",                 "b",       expect_fail=True)
run("3.9 Необъявленная переменная",    "tests/test3.9.txt", "",                 "y",       expect_fail=True)
run("3.10 Деление на ноль",            "tests/test3.10.txt","7",                "деление", expect_fail=True)

print("=" * 60)
print("Done.")
