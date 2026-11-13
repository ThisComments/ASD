#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>

/*
28. На диске имеется база  данных  по  физическим  эффектам
(ФЭ).  Каждый  ФЭ  записан в виде тройки компонент:  название,
входная  физическая  величина  (вход),   выходная   физическая
величина  (выход).  Имеется  не более 100 различных физических
величин. Требуется построить в основной памяти граф связи ФЭ и
обеспечить  поиск  цепочек  ФЭ от одного до четырех звеньев по
заданным входу и выходу по возрастанию количества звеньев (12).
*/

const std::string QUANTITIES_FILE_NAME = "P_NAME.TXT";
const std::string EFFECTS_FILE_NAME = "PHYS.TXT";
const std::string RESULT_FILE_NAME = "RESULT.TXT";
const int MAX_QUANTITY_ID = 100;
const size_t MAX_CHAIN_LENGTH = 4;

struct PhysicalEffect
{
	std::string name;
	std::vector<int> inputIds;
	int outputId;
};

struct QuantityEdge
{
	int targetQuantityId;
	std::vector<size_t> effectIndices;
};

struct QuantityNode
{
	int id;
	std::string name;
	std::vector<QuantityEdge> outgoingEdges;
};

struct Graph
{
	std::map<int, QuantityNode> nodes;
	std::vector<PhysicalEffect> effects;
};

using QuantityPath = std::vector<int>;

std::string Trim(const std::string& text)
{
	size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		return "";
	}
	size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

std::vector<std::string> Split(const std::string& text)
{
	std::vector<std::string> parts;
	size_t start = 0;
	while (true)
	{
		size_t position = text.find(',', start);
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

std::string JoinStrings(const std::vector<std::string>& items)
{
	std::string result;
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i > 0)
		{
			result += ", ";
		}
		result += items[i];
	}
	return result;
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
	return graph.nodes.find(id) != graph.nodes.end();
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
	if (!file.is_open())
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

		size_t position = line.find(':');
		int id = 0;
		if (position == std::string::npos || !ParseInteger(Trim(line.substr(0, position)), id))
		{
			ReportBadLine(fileName, lineNumber, "ожидаемый формат: id:название");
			continue;
		}

		std::string name = Trim(line.substr(position + 1));
		if (id < 1 || id > MAX_QUANTITY_ID)
		{
			std::string reportLine = "id вне допустимого диапазона (1.." + MAX_QUANTITY_ID + ')';
			ReportBadLine(fileName, lineNumber, reportLine);
			continue;
		}
		if (name.empty())
		{
			ReportBadLine(fileName, lineNumber, "пустое название физической величины");
			continue;
		}
		if (IsValidQuantityId(graph, id))
		{
			ReportBadLine(fileName, lineNumber, "дубликат id физической величины");
			continue;
		}

		QuantityNode node;
		node.id = id;
		node.name = name;
		graph.nodes[id] = node;
	}

	if (graph.nodes.empty())
	{
		std::cerr << "Ошибка: не загружены физические величины из " << fileName << std::endl;
		return false;
	}
	return true;
}

QuantityEdge* FindOutgoingEdge(
	QuantityNode& node,
	int targetId
)
{
	for (QuantityEdge& edge : node.outgoingEdges)
	{
		if (edge.targetQuantityId == targetId)
		{
			return &edge;
		}
	}
	return nullptr;
}

const QuantityEdge* FindOutgoingEdge(
	const QuantityNode& node,
	int targetId
)
{
	for (const QuantityEdge& edge : node.outgoingEdges)
	{
		if (edge.targetQuantityId == targetId)
		{
			return &edge;
		}
	}
	return nullptr;
}

void LinkEffectToEdge(
	Graph& graph,
	int inputId,
	int outputId,
	size_t effectIndex
)
{
	QuantityNode& sourceNode = graph.nodes[inputId];
	QuantityEdge* existingEdge = FindOutgoingEdge(sourceNode, outputId);

	if (existingEdge != nullptr)
	{
		if (std::find(existingEdge->effectIndices.begin(), existingEdge->effectIndices.end(), effectIndex) == existingEdge->effectIndices.end())
		{
			existingEdge->effectIndices.push_back(effectIndex);
		}
	}
	else
	{
		QuantityEdge newEdge;
		newEdge.targetQuantityId = outputId;
		newEdge.effectIndices.push_back(effectIndex);
		sourceNode.outgoingEdges.push_back(newEdge);
	}
}

void AddEffectToGraph(
	Graph& graph,
	const PhysicalEffect& effect
)
{
	for (const PhysicalEffect& existing : graph.effects)
	{
		if (existing.name == effect.name
			&& existing.outputId == effect.outputId
			&& existing.inputIds == effect.inputIds)
		{
			return;
		}
	}

	size_t effectIndex = graph.effects.size();
	graph.effects.push_back(effect);

	for (int inputId : effect.inputIds)
	{
		LinkEffectToEdge(graph, inputId, effect.outputId, effectIndex);
	}
}

bool SplitEffectLine(
	const std::string& line,
	std::string& name,
	std::string& inputs,
	std::string& output
)
{
	size_t firstPosition = line.find(';');
	if (firstPosition == std::string::npos)
	{
		return false;
	}
	size_t secondPosition = line.find(';', firstPosition + 1);
	if (secondPosition == std::string::npos)
	{
		return false;
	}
	size_t thirdPosition = line.find(';', secondPosition + 1);

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
	for (const std::string& inputText : Split(inputsText))
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
	return !inputIds.empty();
}

bool LoadEffects(
	const std::string& fileName,
	Graph& graph
)
{
	std::ifstream file(fileName);
	if (!file.is_open())
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
		std::string inputsText;
		std::string outputText;

		if (!SplitEffectLine(line, effect.name, inputsText, outputText)
			|| effect.name.empty()
			|| !ParseInteger(outputText, effect.outputId))
		{
			ReportBadLine(fileName, lineNumber, "ожидаемый формат: название ФЭ;id_входов;id_выхода");
			continue;
		}

		if (!IsValidQuantityId(graph, effect.outputId))
		{
			ReportBadLine(fileName, lineNumber, "неизвестная выходная величина " + outputText);
			continue;
		}

		std::string error;
		if (!ParseInputIds(graph, inputsText, effect.inputIds, error))
		{
			ReportBadLine(fileName, lineNumber, error);
			continue;
		}

		AddEffectToGraph(graph, effect);
	}

	if (graph.effects.empty())
	{
		std::cerr << "Ошибка: не загружены физические эффекты из " << fileName << std::endl;
		return false;
	}
	return true;
}

bool IsQuantityInPath(const QuantityPath& path, int quantityId)
{
	return std::find(path.begin(), path.end(), quantityId) != path.end();
}

std::vector<QuantityPath> AdvanceBfsOneLevel(
	const Graph& graph,
	int targetId,
	std::queue<QuantityPath>& pathQueue
)
{
	std::vector<QuantityPath> foundOnThisLevel;
	size_t pathsInCurrentLevel = pathQueue.size();

	for (size_t i = 0; i < pathsInCurrentLevel; ++i)
	{
		QuantityPath currentPath = pathQueue.front();
		pathQueue.pop();

		int lastId = currentPath.back();
		const QuantityNode& currentNode = graph.nodes.at(lastId);

		for (const QuantityEdge& edge : currentNode.outgoingEdges)
		{
			int nextId = edge.targetQuantityId;
			if (IsQuantityInPath(currentPath, nextId))
			{
				continue;
			}

			QuantityPath nextPath = currentPath;
			nextPath.push_back(nextId);

			if (nextId == targetId)
			{
				foundOnThisLevel.push_back(nextPath);
			}
			else
			{
				pathQueue.push(nextPath);
			}
		}
	}

	return foundOnThisLevel;
}

std::vector<int> FindMissingInputs(
	const PhysicalEffect& effect,
	const QuantityPath& path,
	size_t stepIndex
)
{
	std::vector<int> missingIds;
	auto availableEnd = path.begin() + stepIndex + 1;
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
	const QuantityPath& path,
	size_t stepIndex
)
{
	std::vector<int> missingIds = FindMissingInputs(effect, path, stepIndex);
	if (missingIds.empty())
	{
		if (effect.inputIds.size() > 1)
		{
			return effect.name + " (все входные величины уже получены в цепочке)";
		}
		return effect.name;
	}

	std::vector<std::string> missingDescriptions;
	for (int missingId : missingIds)
	{
		const QuantityNode& node = graph.nodes.at(missingId);
		missingDescriptions.push_back(std::to_string(node.id) + ": " + node.name);
	}
	return effect.name + " (требуется доп. вход: " + JoinStrings(missingDescriptions) + ")";
}

void PrintQuantities(const Graph& graph)
{
	std::cout << "\nДоступные физические величины:" << std::endl;
	for (const auto& pair : graph.nodes)
	{
		std::cout << pair.first << ": " << pair.second.name << std::endl;
	}
	std::cout << std::endl;
}

void WritePath(
	std::ostream& output,
	const Graph& graph,
	const QuantityPath& path,
	size_t pathIndex
)
{
	size_t edgeCount = path.size() - 1;
	output << "  Цепочка #" << pathIndex << " (звеньев: " << edgeCount << "):" << std::endl;

	for (size_t i = 0; i < edgeCount; ++i)
	{
		int fromId = path[i];
		int toId = path[i + 1];

		const QuantityNode& fromNode = graph.nodes.at(fromId);
		const QuantityNode& toNode = graph.nodes.at(toId);
		const QuantityEdge* edge = FindOutgoingEdge(fromNode, toId);

		output << "    Шаг " << (i + 1) << ": [" << fromNode.id << ": " << fromNode.name
			<< "] -> [" << toNode.id << ": " << toNode.name << "]" << std::endl;

		if (edge != nullptr)
		{
			std::vector<std::string> effectDescriptions;
			for (size_t effectIndex : edge->effectIndices)
			{
				const PhysicalEffect& effect = graph.effects[effectIndex];
				effectDescriptions.push_back(DescribeEffect(graph, effect, path, i));
			}

			output << "      Физические эффекты (" << effectDescriptions.size() << "): "
				<< JoinStrings(effectDescriptions) << std::endl;
		}
	}
	output << std::endl;
}

void WriteLengthGroupResults(
	std::ostream& output,
	const Graph& graph,
	size_t chainLength,
	const std::vector<QuantityPath>& paths,
	size_t& globalPathCounter
)
{
	output << "--- Цепочки из " << chainLength << " зв. ---" << std::endl;
	if (paths.empty())
	{
		output << "  Цепочек длиной " << chainLength << " зв. не найдено.\n" << std::endl;
		return;
	}

	output << "  Найдено цепочек: " << paths.size() << "\n" << std::endl;
	for (const QuantityPath& path : paths)
	{
		++globalPathCounter;
		WritePath(output, graph, path, globalPathCounter);
	}
}

bool AskYesNo(const std::string& prompt)
{
	std::string input;
	while (true)
	{
		std::cout << prompt << " (Д/н): ";
		if (!std::getline(std::cin, input))
		{
			return false;
		}
		input = Trim(input);
		if (input == "д" || input == "Д" || input == "y" || input == "Y" || input == "")
		{
			return true;
		}
		if (input == "н" || input == "Н" || input == "n" || input == "N")
		{
			return false;
		}
		std::cout << "Пожалуйста, введите 'да' или 'нет'." << std::endl;
	}
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

void ProcessSingleQuery(
	const Graph& graph,
	int startId,
	int targetId,
	std::ofstream& resultFile
)
{
	const QuantityNode& startNode = graph.nodes.at(startId);
	const QuantityNode& targetNode = graph.nodes.at(targetId);

	std::cout << "\nПоиск: [" << startNode.id << ": " << startNode.name << "] -> ["
		<< targetNode.id << ": " << targetNode.name << "]\n" << std::endl;

	if (resultFile.is_open())
	{
		resultFile << "========================================" << std::endl;
		resultFile << "Поиск: [" << startNode.id << ": " << startNode.name << "] -> ["
			<< targetNode.id << ": " << targetNode.name << "]\n" << std::endl;
	}

	std::queue<QuantityPath> pathQueue;
	pathQueue.push({ startId });

	size_t globalPathCounter = 0;

	for (size_t length = 1; length <= MAX_CHAIN_LENGTH; ++length)
	{
		if (pathQueue.empty())
		{
			std::cout << "Дальнейших путей в графе нет (все ветки закончились).\n" << std::endl;
			break;
		}

		std::vector<QuantityPath> paths = AdvanceBfsOneLevel(graph, targetId, pathQueue);

		size_t consoleCounter = globalPathCounter;
		WriteLengthGroupResults(std::cout, graph, length, paths, consoleCounter);

		if (resultFile.is_open())
		{
			size_t fileCounter = globalPathCounter;
			WriteLengthGroupResults(resultFile, graph, length, paths, fileCounter);
		}

		globalPathCounter = consoleCounter;

		if (length < MAX_CHAIN_LENGTH)
		{
			if (pathQueue.empty())
			{
				std::cout << "Больше доступных переходов для увеличения длины цепочки нет.\n" << std::endl;
				break;
			}
			if (!AskYesNo("Вывести ещё?"))
			{
				break;
			}
			std::cout << std::endl;
		}
	}
}

void RunSession(const Graph& graph)
{
	std::ofstream resultFile(RESULT_FILE_NAME, std::ios::out);
	if (!resultFile.is_open())
	{
		std::cerr << "Ошибка: невозможно записать в файл " << RESULT_FILE_NAME << std::endl;
	}

	bool keepSearching = true;
	while (keepSearching)
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
		}
		else
		{
			ProcessSingleQuery(graph, startId, targetId, resultFile);
		}

		keepSearching = AskYesNo("Искать ещё?");
	}
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