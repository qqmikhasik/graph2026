/**
 * @file hungarian_algorithm_test.cpp
 * @author Shvaikov
 *
 * Тесты для алгоритма graph::HungarianAlgorithm.
 */

#include <httplib.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <climits>
#include <nlohmann/json.hpp>
#include "test_core.hpp"

static void SimpleTest(httplib::Client* cli);
static void RectangularTest(httplib::Client* cli);
static void SingleElementTest(httplib::Client* cli);
static void RandomTest(httplib::Client* cli);

void TestHungarianAlgorithm(httplib::Client* cli) {
  TestSuite suite("TestHungarianAlgorithm");

  RUN_TEST_REMOTE(suite, cli, SimpleTest);
  RUN_TEST_REMOTE(suite, cli, RectangularTest);
  RUN_TEST_REMOTE(suite, cli, SingleElementTest);
  RUN_TEST_REMOTE(suite, cli, RandomTest);
}

/**
 * @brief Перебор всех допустимых назначений для проверки.
 *
 * @param matrix Матрица стоимостей.
 * @param row Текущая строка.
 * @param used Какие столбцы уже заняты.
 * @return Минимальная стоимость назначения оставшихся строк.
 *
 * Рекурсивно перебирает все инъективные отображения строк в столбцы и
 * возвращает минимальную суммарную стоимость. Используется как эталон
 * для небольших матриц (число столбцов не больше 7).
 */
static int BruteForceHelper(const std::vector<std::vector<int>>& matrix,
    size_t row, std::vector<bool>* used) {
  size_t n = matrix.size();
  size_t m = matrix[0].size();

  /* Все строки назначены --- добавлять больше нечего. */
  if (row == n)
    return 0;

  int best = INT_MAX;

  for (size_t col = 0; col < m; col++) {
    if (!(*used)[col]) {
      (*used)[col] = true;
      int sub = BruteForceHelper(matrix, row + 1, used);
      if (sub != INT_MAX)
        best = std::min(best, matrix[row][col] + sub);
      (*used)[col] = false;
    }
  }

  return best;
}

/**
 * @brief Эталонное вычисление минимальной стоимости полным перебором.
 *
 * @param matrix Матрица стоимостей.
 * @return Минимальная суммарная стоимость назначения.
 */
static int BruteForceMin(const std::vector<std::vector<int>>& matrix) {
  std::vector<bool> used(matrix[0].size(), false);
  return BruteForceHelper(matrix, 0, &used);
}

/**
 * @brief Проверка ответа сервера для заданной матрицы.
 *
 * @param cli Указатель на HTTP клиент.
 * @param matrix Матрица стоимостей.
 *
 * Функция отправляет матрицу на сервер, получает результат и проверяет:
 * назначение является корректной инъекцией, сумма выбранных элементов
 * совпадает с полем cost, а cost равен эталонному значению полного перебора.
 */
static void CheckMatrix(httplib::Client* cli,
    const std::vector<std::vector<int>>& matrix) {
  size_t n = matrix.size();
  size_t m = matrix[0].size();

  nlohmann::json input;
  input["weight_type"] = "int";
  input["matrix"] = matrix;

  auto res = cli->Post("/HungarianAlgorithm", input.dump(),
      "application/json");

  if (!res) {
    REQUIRE(false);
    return;
  }

  nlohmann::json output = nlohmann::json::parse(res->body);

  int cost = output.at("cost");
  std::vector<size_t> assignment = output.at("assignment");

  /* Размер назначения равен числу строк. */
  REQUIRE_EQUAL(assignment.size(), n);

  /* Назначение является инъекцией: все столбцы различны и в пределах [0, m). */
  std::vector<bool> seen(m, false);
  int realCost = 0;
  for (size_t i = 0; i < n; i++) {
    size_t col = assignment[i];
    REQUIRE(col < m);
    REQUIRE(!seen[col]);
    seen[col] = true;
    realCost += matrix[i][col];
  }

  /* Поле cost согласовано с самим назначением. */
  REQUIRE_EQUAL(realCost, cost);

  /* Стоимость совпадает с эталоном полного перебора. */
  REQUIRE_EQUAL(cost, BruteForceMin(matrix));
}

/**
 * @brief Простейшие статические тесты на квадратных матрицах.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void SimpleTest(httplib::Client* cli) {
  /* Оптимально назначение по главной диагонали (стоимость 3). */
  CheckMatrix(cli, {
    {1, 100, 100},
    {100, 1, 100},
    {100, 100, 1}
  });

  /* Оптимальное назначение --- по побочной структуре (стоимость 3). */
  CheckMatrix(cli, {
    {3, 1, 2},
    {2, 3, 1},
    {1, 2, 3}
  });

  /* Матрица с нулями и несколькими оптимумами. */
  CheckMatrix(cli, {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
  });

  /* Классическая матрица 4x4. */
  CheckMatrix(cli, {
    {10, 19, 8, 15},
    {10, 18, 7, 17},
    {13, 16, 9, 14},
    {12, 19, 8, 18}
  });
}

/**
 * @brief Статические тесты на прямоугольных матрицах (n < m).
 *
 * @param cli Указатель на HTTP клиент.
 */
static void RectangularTest(httplib::Client* cli) {
  CheckMatrix(cli, {
    {4, 1, 3},
    {2, 0, 5}
  });

  CheckMatrix(cli, {
    {7, 5, 11, 8},
    {5, 4, 1, 6},
    {9, 3, 2, 7}
  });

  /* Одна строка, несколько столбцов: выбирается минимум строки. */
  CheckMatrix(cli, {
    {5, 2, 9, 1, 7}
  });
}

/**
 * @brief Тест на матрице из одного элемента.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void SingleElementTest(httplib::Client* cli) {
  CheckMatrix(cli, { {42} });
  CheckMatrix(cli, { {0} });
}

/**
 * @brief Случайные тесты со сверкой с полным перебором.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void RandomTest(httplib::Client* cli) {
  // Число попыток.
  const int numTries = 200;
  // Используется для инициализации генератора случайных чисел.
  std::random_device rd;
  // Генератор случайных чисел.
  std::mt19937 gen(rd());
  // Распределение для числа строк (держим небольшим ради полного перебора).
  std::uniform_int_distribution<size_t> rowsSize(1, 6);
  // Распределение для числа дополнительных столбцов.
  std::uniform_int_distribution<size_t> extraColumns(0, 2);
  // Распределение для значений стоимости.
  std::uniform_int_distribution<int> weight(0, 30);

  for (int it = 0; it < numTries; it++) {
    size_t n = rowsSize(gen);
    size_t m = n + extraColumns(gen);

    std::vector<std::vector<int>> matrix(n, std::vector<int>(m));

    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
        matrix[i][j] = weight(gen);

    CheckMatrix(cli, matrix);
  }
}
