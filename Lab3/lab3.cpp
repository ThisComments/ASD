#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

/*
Общими требованиями к лабораторной работе являются:
   1) вводить   исходное   дерево  из  файла  в  понятной  для
пользователя форме, а не с клавиатуры;
   2) по требованию прользователя показывать дерево на экране;
   3) обрабатывать  дерево  в  оперативной памяти,  а не путем
многократного обращения к файлу;
   4) обеспечить   возможность   многократных   запросов   без
повторного запуска программы.

27. Имеется  И-ИЛИ   дерево,   соответствующее   некоторому
множеству  конструкций.  Требуется выдать на экран в наглядном
виде все элементы дерева (13).
*/


enum class NodeType
{
	LEAF,
	AND,
	OR
};

struct Node
{
	std::string name;
	NodeType type;
	std::vector<std::unique_ptr<Node>> children;
};

struct ParsedLine
{
	size_t depth;
	NodeType type;
	std::string name;
};

struct Entry
{
	std::string name;
	size_t depth;
};

using Combination = std::vector<Entry>;

const char DEPTH_CHAR = '.';
const char TYPE_AND = 'a';
const char TYPE_OR = 'o';
const char TYPE_LEAF = 'l';
const char CARRIAGE_RETURN = '\r';
const size_t ROOT_DEPTH = 0;
const size_t TYPE_TEXT_LENGTH = 1;
const std::string WHITESPACE = " \t";
const std::string ROOT_DEPTH_ERROR = "the first node must be the root (depth 0)";
const std::string BRANCH_MIDDLE = "├── ";
const std::string BRANCH_LAST = "└── ";
const std::string PREFIX_MIDDLE = "│   ";
const std::string PREFIX_LAST = "    ";
const std::string LABEL_AND = " [AND]";
const std::string LABEL_OR = " [OR]";
const std::string COMBINATION_TITLE = "Number ";
const int MENU_EXIT = 0;
const int MENU_SHOW = 1;
const int MENU_COMBINATIONS = 2;
const int MENU_REPLACE = 3;
const int MENU_INVALID = -1;

std::string ReadFileName()
{
	std::string fileName;
	std::cout << "Enter input file name: ";
	std::getline(std::cin, fileName);
	return fileName;
}

bool OpenInputFile(const std::string &fileName, std::ifstream &input)
{
	input.open(fileName);
	return input.is_open();
}

std::string Trim(const std::string &text)
{
	size_t first = text.find_first_not_of(WHITESPACE);
	if (first == std::string::npos)
	{
		return "";
	}
	size_t last = text.find_last_not_of(WHITESPACE);
	return text.substr(first, last - first + 1);
}

bool ParseNodeType(char symbol, NodeType &type)
{
	if (symbol == TYPE_AND)
	{
		type = NodeType::AND;
		return true;
	}
	if (symbol == TYPE_OR)
	{
		type = NodeType::OR;
		return true;
	}
	if (symbol == TYPE_LEAF)
	{
		type = NodeType::LEAF;
		return true;
	}
	return false;
}

bool ParseNodeLine(const std::string &line, ParsedLine &parsed, std::string &errorMessage)
{
	size_t position = 0;
	while (position < line.size() && line[position] == DEPTH_CHAR)
	{
		position++;
	}
	parsed.depth = position;

	std::istringstream stream(line.substr(position));
	std::string name;
	std::string typeText;
	std::string extraText;

	if (!(stream >> name))
	{
		errorMessage = "empty node name";
		return false;
	}
	if (!(stream >> typeText))
	{
		errorMessage = "missing node type for node \"" + name + "\"";
		return false;
	}
	if (stream >> extraText)
	{
		errorMessage = "unexpected text after node type: \"" + extraText + "\"";
		return false;
	}
	if (typeText.size() != TYPE_TEXT_LENGTH || !ParseNodeType(typeText.front(), parsed.type))
	{
		errorMessage = "unknown node type \"" + typeText + "\"";
		return false;
	}
	parsed.name = name;
	return true;
}

bool AddNode(std::unique_ptr<Node> &root, std::vector<Node *> &lastAtDepth,
	const ParsedLine &parsed, std::string &errorMessage)
{
	std::unique_ptr<Node> node = std::make_unique<Node>();
	node->name = parsed.name;
	node->type = parsed.type;
	Node *rawNode = node.get();

	if (parsed.depth == ROOT_DEPTH)
	{
		if (root != nullptr)
		{
			errorMessage = "more than one root node";
			return false;
		}
		root = std::move(node);
		lastAtDepth.assign(1, rawNode);
		return true;
	}

	if (root == nullptr)
	{
		errorMessage = ROOT_DEPTH_ERROR;
		return false;
	}
	if (parsed.depth > lastAtDepth.size())
	{
		errorMessage = "skipped depth level";
		return false;
	}

	Node *parent = lastAtDepth[parsed.depth - 1];
	if (parent->type == NodeType::LEAF)
	{
		errorMessage = "leaf \"" + parent->name + "\" cannot have children";
		return false;
	}

	parent->children.push_back(std::move(node));
	lastAtDepth.resize(parsed.depth);
	lastAtDepth.push_back(rawNode);
	return true;
}

bool ValidateNode(const Node &node, std::string &errorMessage)
{
	if (node.type != NodeType::LEAF && node.children.empty())
	{
		errorMessage = "AND/OR node \"" + node.name + "\" has no children";
		return false;
	}
	for (const std::unique_ptr<Node> &child : node.children)
	{
		if (!ValidateNode(*child, errorMessage))
		{
			return false;
		}
	}
	return true;
}

bool ReadTree(std::istream &input, std::unique_ptr<Node> &root, std::string &errorMessage)
{
	std::vector<Node *> lastAtDepth;
	std::string line;
	size_t lineNumber = 0;

	while (std::getline(input, line))
	{
		lineNumber++;
		if (!line.empty() && line.back() == CARRIAGE_RETURN)
		{
			line.pop_back();
		}
		if (Trim(line).empty())
		{
			continue;
		}

		ParsedLine parsed;
		std::string lineError;
		if (!ParseNodeLine(line, parsed, lineError)
			|| !AddNode(root, lastAtDepth, parsed, lineError))
		{
			errorMessage = "line " + std::to_string(lineNumber) + ": " + lineError;
			return false;
		}
	}

	if (root == nullptr)
	{
		errorMessage = "the file is empty";
		return false;
	}
	return ValidateNode(*root, errorMessage);
}

bool LoadTree(const std::string &fileName, std::unique_ptr<Node> &root, std::string &errorMessage)
{
	std::ifstream input;
	if (!OpenInputFile(fileName, input))
	{
		errorMessage = "cannot open the file \"" + fileName + "\"";
		return false;
	}

	std::unique_ptr<Node> newRoot;
	bool isRead = ReadTree(input, newRoot, errorMessage);
	input.close();
	if (!isRead)
	{
		return false;
	}

	root = std::move(newRoot);
	return true;
}

std::string GetNodeLabel(const Node &node)
{
	if (node.type == NodeType::AND)
	{
		return node.name + LABEL_AND;
	}
	if (node.type == NodeType::OR)
	{
		return node.name + LABEL_OR;
	}
	return node.name;
}

void PrintNode(const Node &node, const std::string &prefix, bool isLast)
{
	std::cout << prefix << (isLast ? BRANCH_LAST : BRANCH_MIDDLE)
		<< GetNodeLabel(node) << std::endl;

	std::string childPrefix = prefix + (isLast ? PREFIX_LAST : PREFIX_MIDDLE);
	for (size_t i = 0; i < node.children.size(); i++)
	{
		PrintNode(*node.children[i], childPrefix, i + 1 == node.children.size());
	}
}

void PrintTree(const Node &root)
{
	std::cout << GetNodeLabel(root) << std::endl;
	for (size_t i = 0; i < root.children.size(); i++)
	{
		PrintNode(*root.children[i], "", i + 1 == root.children.size());
	}
}

std::vector<Combination> GenerateCombinations(const Node &node, size_t depth);

std::vector<Combination> GenerateOrCombinations(const Node &node, size_t depth)
{
	std::vector<Combination> result;
	for (const std::unique_ptr<Node> &child : node.children)
	{
		for (const Combination &childCombination : GenerateCombinations(*child, depth + 1))
		{
			Combination combination;
			combination.push_back(Entry{node.name, depth});
			combination.insert(combination.end(), childCombination.begin(), childCombination.end());
			result.push_back(combination);
		}
	}
	return result;
}

std::vector<Combination> GenerateAndCombinations(const Node &node, size_t depth)
{
	std::vector<Combination> partial;
	partial.push_back(Combination{Entry{node.name, depth}});

	for (const std::unique_ptr<Node> &child : node.children)
	{
		std::vector<Combination> childCombinations = GenerateCombinations(*child, depth + 1);
		std::vector<Combination> next;
		for (const Combination &prefix : partial)
		{
			for (const Combination &childCombination : childCombinations)
			{
				Combination merged = prefix;
				merged.insert(merged.end(), childCombination.begin(), childCombination.end());
				next.push_back(merged);
			}
		}
		partial = std::move(next);
	}
	return partial;
}

std::vector<Combination> GenerateCombinations(const Node &node, size_t depth)
{
	if (node.type == NodeType::LEAF)
	{
		return std::vector<Combination>{Combination{Entry{node.name, depth}}};
	}
	if (node.type == NodeType::OR)
	{
		return GenerateOrCombinations(node, depth);
	}
	return GenerateAndCombinations(node, depth);
}

std::string FormatCombination(const Combination &combination)
{
	std::string text;
	for (const Entry &entry : combination)
	{
		text += std::string(entry.depth, DEPTH_CHAR) + entry.name + "\n";
	}
	return text;
}

void PrintCombinations(const Node &root)
{
	std::vector<Combination> combinations = GenerateCombinations(root, ROOT_DEPTH);
	std::unordered_set<std::string> printed;
	size_t number = 0;

	for (const Combination &combination : combinations)
	{
		std::string text = FormatCombination(combination);
		if (!printed.insert(text).second)
		{
			continue;
		}
		number++;
		std::cout << COMBINATION_TITLE << number << std::endl << text << std::endl;
	}
}

void ReplaceTree(std::unique_ptr<Node> &root)
{
	std::string errorMessage;
	if (LoadTree(ReadFileName(), root, errorMessage))
	{
		std::cout << "The tree was replaced successfully" << std::endl;
		return;
	}
	std::cout << "Error: " << errorMessage << std::endl;
	std::cout << "The current tree was kept" << std::endl;
}

void ShowMenu()
{
	std::cout << std::endl;
	std::cout << MENU_SHOW << " - display the tree" << std::endl;
	std::cout << MENU_COMBINATIONS << " - display all combinations" << std::endl;
	std::cout << MENU_REPLACE << " - load another tree" << std::endl;
	std::cout << MENU_EXIT << " - exit" << std::endl;
	std::cout << "Your choice: ";
}

int ReadChoice()
{
	std::string line;
	if (!std::getline(std::cin, line))
	{
		return MENU_EXIT;
	}
	try
	{
		size_t position = 0;
		int choice = std::stoi(line, &position);
		if (!Trim(line.substr(position)).empty())
		{
			return MENU_INVALID;
		}
		return choice;
	}
	catch (const std::exception &)
	{
		return MENU_INVALID;
	}
}

bool ProcessUserChoice(int choice, std::unique_ptr<Node> &root)
{
	if (choice == MENU_SHOW)
	{
		std::cout << std::endl;
		PrintTree(*root);
		return true;
	}
	if (choice == MENU_COMBINATIONS)
	{
		std::cout << std::endl;
		PrintCombinations(*root);
		return true;
	}
	if (choice == MENU_REPLACE)
	{
		ReplaceTree(root);
		return true;
	}
	if (choice == MENU_EXIT)
	{
		return false;
	}
	std::cout << "Unknown menu item" << std::endl;
	return true;
}

void RunMenuLoop(std::unique_ptr<Node> &root)
{
	bool isRunning = true;
	while (isRunning)
	{
		ShowMenu();
		isRunning = ProcessUserChoice(ReadChoice(), root);
	}
}

int main()
{
	std::unique_ptr<Node> root;
	std::string errorMessage;

	if (!LoadTree(ReadFileName(), root, errorMessage))
	{
		std::cerr << "Error: " << errorMessage << std::endl;
		return 1;
	}

	RunMenuLoop(root);
	return 0;
}
