#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <charconv>

/* 
Общими требованиями к лабораторной работе являются:
   1) вводить  граф из файла в понятной для пользователя форме
(не в виде матрицы смежности, без дублирования информации и т.
п.);
   2) обеспечить   возможность   многократных   запросов   без
повторного запуска программы там, где это требуется.

28. На диске имеется база  данных  по  физическим  эффектам
(ФЭ).  Каждый  ФЭ  записан в виде тройки компонент:  название,
входная  физическая  величина  (вход),   выходная   физическая
величина  (выход).  Имеется  не более 100 различных физических
величин. Требуется построить в основной памяти граф связи ФЭ и
обеспечить  поиск  цепочек  ФЭ от одного до четырех звеньев по
заданным входу и выходу по возрастанию количества звеньев (12).



разобраться с поиском в глубину

граф по велечинам строить
*/

const std::string QUANTITIES_FILE_NAME = "P_NAME.TXT";
const std::string EFFECTS_FILE_NAME = "PHYS.TXT";
const std::string RESULT_FILE_NAME = "RESULT.TXT";
const std::string ARROW = " \xE2\x86\x92 ";
const std::string LIST_SEPARATOR = ", ";
const std::string WHITESPACE = " \t\r\n";
const char QUANTITY_DELIMITER = ':';
const char EFFECT_DELIMITER = ';';
const char INPUT_DELIMITER = ',';
const int MAX_QUANTITY_ID = 100;
const size_t MIN_PATH_LENGTH = 1;
const size_t MAX_PATH_LENGTH = 4;
const size_t SINGLE_INPUT_COUNT = 1;

struct PhysicalEffect
{
	std::string name;
	std::vector<int> inputIds;
	int outputId;
};

struct Graph
{
	std::map<int, std::string> quantityNames;
	std::vector<PhysicalEffect> effects;
	std::map<int, std::vector<size_t>> effectsByInput;
};

std::string Trim(
	const std::string& text
)
{
	size_t first = text.find_first_not_of(WHITESPACE);
	if (first == std::string::npos)
	{
		return "";
	}
	size_t last = text.find_last_not_of(WHITESPACE);
	return text.substr(first, last - first + 1);
}

std::vector<std::string> Split(
	const std::string& text, 
	char delimiter
)
{
	std::vector<std::string> parts;
	size_t start = 0;
	while (true)
	{
		size_t position = text.find(delimiter, start);
		if (position == std::string::npos)
		{
			parts.push_back(Trim(text.substr(start)));
			break;
		}
		parts.push_back(Trim(text.substr(start, position - start)));
		start = position + 1;
	}
	return parts;
}

bool ParseInteger(
	const std::string& text, 
	int& value
)
{
	if (text.empty())
	{
		return false;
	}

	auto result = std::from_chars(text.data(), text.data() + text.size(), value);

	return result.ec == std::errc() && result.ptr == text.data() + text.size();
}

bool IsValidQuantityId(
	const Graph& graph, 
	int id
)
{
	return graph.quantityNames.count(id) > 0;
}

void ReportBadLine(
	const std::string& fileName, 
	size_t lineNumber, 
	const std::string& reason
)
{
	std::cerr << "Предупреждение: " << fileName << ", строка " << lineNumber << ": " << reason << std::endl;
}

bool LoadQuantities(
	const std::string& fileName, 
	Graph& graph
)
{
	std::ifstream file(fileName);
	if (!file)
	{
		std::cerr << "Ошибка: невозможно открыть файл " << fileName << std::endl;
		return false;
	}

	std::string line;
	size_t lineNumber = 0;
	while (std::getline(file, line))
	{
		++lineNumber;
		line = Trim(line);
		if (line.empty())
		{
			continue;
		}

		size_t position = line.find(QUANTITY_DELIMITER);
		int id = 0;
		if (position == std::string::npos || !ParseInteger(Trim(line.substr(0, position)), id))
		{
			ReportBadLine(fileName, lineNumber, "ожидаемый формат: идентификатор:имя");
			continue;
		}
		std::string name = Trim(line.substr(position + 1));
		if (id < 1 || id > MAX_QUANTITY_ID)
		{
			ReportBadLine(fileName, lineNumber, "идентификатор вне допустимого диапазона");
			continue;
		}
		if (name.empty())
		{
			ReportBadLine(fileName, lineNumber, "пустое имя физической величины");
			continue;
		}
		if (IsValidQuantityId(graph, id))
		{
			ReportBadLine(fileName, lineNumber, "дубликат идентификатора");
			continue;
		}
		graph.quantityNames[id] = name;
	}

	if (graph.quantityNames.empty())
	{
		std::cerr << "Ошибка: не загружены физические величины из " << fileName << std::endl;
		return false;
	}
	return true;
}

void AddEffect(
	Graph& graph, 
	const PhysicalEffect& effect
)
{
	for (const PhysicalEffect& existing : graph.effects)
	{
		if (existing.name == effect.name && existing.outputId == effect.outputId
			&& existing.inputIds == effect.inputIds)
		{
			return;
		}
	}
	size_t effectIndex = graph.effects.size();
	graph.effects.push_back(effect);
	for (int inputId : effect.inputIds)
	{
		graph.effectsByInput[inputId].push_back(effectIndex);
	}
}

bool SplitEffectLine(
	const std::string& line, 
	std::string& name, 
	std::string& inputs, 
	std::string& output
)
{
	size_t firstPosition = line.find(EFFECT_DELIMITER);
	if (firstPosition == std::string::npos)
	{
		return false;
	}
	size_t secondPosition = line.find(EFFECT_DELIMITER, firstPosition + 1);
	if (secondPosition == std::string::npos)
	{
		return false;
	}
	size_t thirdPosition = line.find(EFFECT_DELIMITER, secondPosition + 1);

	name = Trim(line.substr(0, firstPosition));
	inputs = Trim(line.substr(firstPosition + 1, secondPosition - firstPosition - 1));
	if (thirdPosition == std::string::npos)
	{
		output = Trim(line.substr(secondPosition + 1));
		return true;
	}
	output = Trim(line.substr(secondPosition + 1, thirdPosition - secondPosition - 1));
	return true;
}

bool ParseInputIds(
	const Graph& graph, 
	const std::string& inputsText, 
	std::vector<int>& inputIds, 
	std::string& error
)
{
	for (const std::string& inputText : Split(inputsText, INPUT_DELIMITER))
	{
		int inputId = 0;
		if (!ParseInteger(inputText, inputId) || !IsValidQuantityId(graph, inputId))
		{
			error = "неизвестная входная величина '" + inputText + "'";
			return false;
		}
		if (std::find(inputIds.begin(), inputIds.end(), inputId) == inputIds.end())
		{
			inputIds.push_back(inputId);
		}
	}
	return true;
}

bool LoadEffects(
	const std::string& fileName, 
	Graph& graph
)
{
	std::ifstream file(fileName);
	if (!file)
	{
		std::cerr << "Ошибка: невозможно открыть файл " << fileName << std::endl;
		return false;
	}

	std::string line;
	size_t lineNumber = 0;
	while (std::getline(file, line))
	{
		++lineNumber;
		line = Trim(line);
		if (line.empty())
		{
			continue;
		}

		PhysicalEffect effect;
		std::string inputs;
		std::string output;
		if (!SplitEffectLine(line, effect.name, inputs, output) || effect.name.empty()
			|| !ParseInteger(output, effect.outputId))
		{
			ReportBadLine(fileName, lineNumber, "ожидаемый формат имя;входы;выходы;");
			continue;
		}
		if (!IsValidQuantityId(graph, effect.outputId))
		{
			ReportBadLine(fileName, lineNumber, "неизвестная выходная величина " + output);
			continue;
		}

		std::string error;
		if (!ParseInputIds(graph, inputs, effect.inputIds, error))
		{
			ReportBadLine(fileName, lineNumber, error);
			continue;
		}
		AddEffect(graph, effect);
	}

	if (graph.effects.empty())
	{
		std::cerr << "Ошибка: не загружены физические эффекты из " << fileName << std::endl;
		return false;
	}
	return true;
}

std::set<int> GetNextQuantities(
	const Graph& graph, 
	int current
)
{
	std::set<int> nextQuantities;
	auto effectsIterator = graph.effectsByInput.find(current);
	if (effectsIterator != graph.effectsByInput.end())
	{
		for (size_t effectIndex : effectsIterator->second)
		{
			nextQuantities.insert(graph.effects[effectIndex].outputId);
		}
	}
	return nextQuantities;
}

std::vector<size_t> FindEdgeEffects(
	const Graph& graph, 
	int fromId, 
	int toId
)
{
	std::vector<size_t> edgeEffects;
	auto effectsIterator = graph.effectsByInput.find(fromId);
	if (effectsIterator == graph.effectsByInput.end())
	{
		return edgeEffects;
	}
	for (size_t effectIndex : effectsIterator->second)
	{
		if (graph.effects[effectIndex].outputId == toId)
		{
			edgeEffects.push_back(effectIndex);
		}
	}
	return edgeEffects;
}

void FindPaths(
	const Graph& graph, 
	int current, 
	int target, 
	std::vector<int>& currentPath,
	std::set<int>& visited, 
	std::vector<std::vector<int>>& foundPaths
)
{
	if (current == target)
	{
		foundPaths.push_back(currentPath);
		return;
	}
	if (currentPath.size() - 1 >= MAX_PATH_LENGTH)
	{
		return;
	}

	for (int next : GetNextQuantities(graph, current))
	{
		if (visited.count(next) > 0)
		{
			continue;
		}
		visited.insert(next);
		currentPath.push_back(next);
		FindPaths(graph, next, target, currentPath, visited, foundPaths);
		currentPath.pop_back();
		visited.erase(next);
	}
}

std::vector<std::vector<int>> SearchPaths(
	const Graph& graph, 
	int startId, 
	int targetId
)
{
	std::vector<std::vector<int>> foundPaths;
	std::vector<int> currentPath = { startId };
	std::set<int> visited = { startId };
	FindPaths(graph, startId, targetId, currentPath, visited, foundPaths);
	std::stable_sort(foundPaths.begin(), foundPaths.end(),
		[](const std::vector<int>& left, const std::vector<int>& right)
		{
			return left.size() < right.size();
		});
	return foundPaths;
}

std::string JoinStrings(
	const std::vector<std::string>& items, 
	const std::string& separator
)
{
	std::string result;
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i > 0)
		{
			result += separator;
		}
		result += items[i];
	}
	return result;
}

std::vector<int> FindMissingInputs(
	const PhysicalEffect& effect, 
	const std::vector<int>& path, 
	size_t edgeIndex
)
{
	std::vector<int> missingIds;
	auto availableEnd = path.begin() + edgeIndex + 1;
	for (int inputId : effect.inputIds)
	{
		if (std::find(path.begin(), availableEnd, inputId) == availableEnd)
		{
			missingIds.push_back(inputId);
		}
	}
	return missingIds;
}

std::string DescribeEffect(
	const Graph& graph, 
	const PhysicalEffect& effect, 
	const std::vector<int>& missingIds
)
{
	if (missingIds.empty())
	{
		return effect.name + (effect.inputIds.size() > SINGLE_INPUT_COUNT ? " (все необходимые входные данные доступны)" : "");
	}
	std::vector<std::string> missingNames;
	for (int missingId : missingIds)
	{
		missingNames.push_back(graph.quantityNames.at(missingId));
	}
	return effect.name + " (дополнительный обязательный ввод: " + JoinStrings(missingNames, LIST_SEPARATOR) + ")";
}

void WritePath(
	std::ostream& output, 
	const Graph& graph, 
	const std::vector<int>& path, 
	size_t pathNumber
)
{
	std::vector<std::string> names;
	for (int id : path)
	{
		names.push_back(graph.quantityNames.at(id));
	}

	output << pathNumber << ". Звенья: " << path.size() - 1 << std::endl << std::endl;
	output << JoinStrings(names, ARROW) << std::endl;
	for (size_t i = 0; i + 1 < path.size(); ++i)
	{
		std::vector<std::string> descriptions;
		for (size_t effectIndex : FindEdgeEffects(graph, path[i], path[i + 1]))
		{
			const PhysicalEffect& effect = graph.effects[effectIndex];
			descriptions.push_back(DescribeEffect(graph, effect, FindMissingInputs(effect, path, i)));
		}
		output << "Шаг " << i + 1 << ": " << JoinStrings(descriptions, LIST_SEPARATOR) << std::endl;
	}
	output << std::endl;
}

void WriteResults(
	std::ostream& output, 
	const Graph& graph, 
	int startId, 
	int targetId,
	const std::vector<std::vector<int>>& paths
)
{
	output << std::endl << "Поиск: " << graph.quantityNames.at(startId) << ARROW
		<< graph.quantityNames.at(targetId) << std::endl;
	if (paths.empty())
	{
		output << "Цепей длиной от " << MIN_PATH_LENGTH << " до " << MAX_PATH_LENGTH
			<< " не найдено." << std::endl << std::endl;
		return;
	}
	output << "Обнаружено цепей: " << paths.size() << std::endl << std::endl;
	for (size_t i = 0; i < paths.size(); ++i)
	{
		WritePath(output, graph, paths[i], i + 1);
	}
}

bool SaveResults(
	const Graph& graph, 
	int startId, 
	int targetId, 
	const std::vector<std::vector<int>>& paths
)
{
	std::ofstream file(RESULT_FILE_NAME, std::ios::out);
	if (!file)
	{
		std::cerr << "Ошибка: невозможно записать в файл " << RESULT_FILE_NAME << std::endl;
		return false;
	}
	WriteResults(file, graph, startId, targetId, paths);
	file << "----------------------------------------" << std::endl;
	return true;
}

void PrintQuantities(
	const Graph& graph
)
{
	std::cout << "Доступные физические величины:" << std::endl;
	for (const auto& quantity : graph.quantityNames)
	{
		std::cout << quantity.first << ": " << quantity.second << std::endl;
	}
	std::cout << std::endl;
}

bool ReadQuantityId(
	const Graph& graph, 
	const std::string& prompt, 
	int& id
)
{
	std::string line;
	while (true)
	{
		std::cout << prompt;
		if (!std::getline(std::cin, line))
		{
			return false;
		}
		if (ParseInteger(Trim(line), id) && IsValidQuantityId(graph, id))
		{
			return true;
		}
		std::cout << "Недопустимый идентификатор. Введите номер из списка." << std::endl;
	}
}

bool AskContinue()
{
	std::string line;
	std::cout << "Выполнить еще один поиск? (Д/н): ";
	if (!std::getline(std::cin, line))
	{
		return false;
	}
	line = Trim(line);
	return line.empty() || line == "д" || line == "Д";
}

void RunSession(
	const Graph& graph
)
{
	do
	{
		PrintQuantities(graph);

		int startId = 0;
		int targetId = 0;
		if (!ReadQuantityId(graph, "Введите id входной величины: ", startId))
		{
			return;
		}
		if (!ReadQuantityId(graph, "Введите id выходной величины: ", targetId))
		{
			return;
		}
		if (startId == targetId)
		{
			std::cout << "Входные и выходные величины должны различаться." << std::endl;
			continue;
		}

		std::vector<std::vector<int>> paths = SearchPaths(graph, startId, targetId);
		WriteResults(std::cout, graph, startId, targetId, paths);
		if (SaveResults(graph, startId, targetId, paths))
		{
			std::cout << "Результат сохранён в " << RESULT_FILE_NAME << std::endl << std::endl;
		}
	}
	while (AskContinue());
}

int main()
{
	Graph graph;
	if (!LoadQuantities(QUANTITIES_FILE_NAME, graph) || !LoadEffects(EFFECTS_FILE_NAME, graph))
	{
		return 1;
	}
	RunSession(graph);
	return 0;
}