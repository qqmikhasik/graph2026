/**
 * @file methods/hungarian_algorithm_method.cpp
 * @author Shvaikov
 *
 * Серверная часть венгерского алгоритма решения задачи о назначениях.
 */

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <hungarian_algorithm.hpp>

namespace graph {

template<typename T>
static int HungarianAlgorithmMethodHelper(const nlohmann::json& input,
    nlohmann::json* output);

/**
 * @brief Метод венгерского алгоритма решения задачи о назначениях.
 *
 * @param input Входные данные в формате JSON.
 * @param output Выходные данные в формате JSON.
 * @return Функция возвращает 0 в случае успеха и отрицательное число,
 * если входные данные заданы некорректно.
 *
 * Функция выбирает тип элементов матрицы стоимостей по полю "weight_type"
 * и запускает соответствующую специализацию вспомогательной функции.
 */
int HungarianAlgorithmMethod(const nlohmann::json& input,
    nlohmann::json* output) {
  /* Тип элементов матрицы стоимостей. Венгерский алгоритм требует знаковый
   * тип, поэтому поддерживаем int и double. */
  std::string weightType = input.at("weight_type");

  if (weightType == "int") {
    return HungarianAlgorithmMethodHelper<int>(input, output);
  } else if (weightType == "double") {
    return HungarianAlgorithmMethodHelper<double>(input, output);
  }

  return -1;
}

/**
 * @brief Вспомогательная функция метода венгерского алгоритма.
 *
 * @tparam T Тип элементов матрицы стоимостей.
 * @param input Входные данные в формате JSON.
 * @param output Выходные данные в формате JSON.
 * @return Функция возвращает 0 в случае успеха и отрицательное число,
 * если входные данные заданы некорректно.
 *
 * Функция читает матрицу стоимостей из поля "matrix", проверяет её
 * корректность (непустая, прямоугольная, число строк не больше числа
 * столбцов), запускает венгерский алгоритм и записывает в выходной JSON
 * минимальную стоимость ("cost") и назначение ("assignment").
 */
template<typename T>
static int HungarianAlgorithmMethodHelper(const nlohmann::json& input,
    nlohmann::json* output) {
  std::vector<std::vector<T>> matrix;

  /* Считываем матрицу стоимостей построчно. */
  for (const auto& row : input.at("matrix")) {
    std::vector<T> currentRow;

    for (const auto& element : row)
      currentRow.push_back(element.get<T>());

    matrix.push_back(currentRow);
  }

  size_t numRows = matrix.size();

  /* Пустая матрица некорректна для данной задачи. */
  if (numRows == 0)
    return -1;

  size_t numColumns = matrix[0].size();

  /* Проверяем, что матрица прямоугольная (все строки одной длины). */
  for (const auto& row : matrix) {
    if (row.size() != numColumns)
      return -1;
  }

  /* Должно быть хотя бы по одному столбцу и число строк не больше
   * числа столбцов, иначе назначение всех строк невозможно. */
  if (numColumns == 0 || numRows > numColumns)
    return -1;

  std::vector<size_t> assignment;
  T cost = HungarianAlgorithm(matrix, &assignment);

  (*output)["cost"] = cost;
  (*output)["assignment"] = assignment;

  return 0;
}

}  // namespace graph
