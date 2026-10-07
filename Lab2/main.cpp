#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>

/*
Общими требованиями к лабораторной работе являются:
   1) организовать  ввод  данных  из  файла  в   понятной  для 
пользователя форме;
   2) обеспечить   возможность   многократных   запросов   без
повторного запуска программы;
   3) при реализации в С++ не использовать контейнерные классы
для работы с линейными списками типа stack, queue и т. п.


22. В строке текстового файла  задано  выражение  из  целых 
чисел и операций '+', '-', '*', '/', '^', SIN, COS, EXP. Порядок
вычислений  определяется  приоритетом  операций   и   круглыми
скобками. Возможен одноместный минус в  начале  выражения  или
после открывающей скобки. Преобразовать выражение в постфиксную
форму (алгоритм Дейкстры) и вычислить его  значение.  Показать 
этапы  выполнения (10).

Чижов Дмитрий Visual Studio Code

*/

const int MAX_EXPRESSION_SIZE = 1000;
const int MAX_STACK_SIZE = 100;
const int MAX_TOKEN_SIZE = 20;
const std::string ALPHABET_LETTER = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const std::string ALPHABET_DIGIT = "0123456789";
const std::string ALPHABET_OPERATOR = "+-*/^";

enum TokenType
{
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_FUNCTION,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_NEG,
    TOKEN_UNDEFINED
};

enum ErrorCode
{
    ERROR_STACK_OVERFLOW = 1,
    ERROR_INCORRECT_EXPRESSION,
    ERROR_ARGUMENTS,
    ERROR_POSTFIX,
    ERROR_DIVISION_BY_ZERO,
    ERROR_EXPRESSION_TOO_LONG,
    ERROR_INPUT_FILE,
    ERROR_OUTPUT_FILE,
    ERROR_SAME_FILES,
    ERROR_INVALID_ANSWER
};

struct Token
{
    TokenType type;
    std::string value;
};

struct OperatorStack
{
    Token tokens[MAX_STACK_SIZE];
    int top;
};

struct NumberStack
{
    double numbers[MAX_STACK_SIZE];
    int top;
};

bool ReportError(ErrorCode code)
{
    std::string message;

    switch (code)
    {
    case ERROR_STACK_OVERFLOW:
        message = "Стек переполнен.";
        break;
    case ERROR_INCORRECT_EXPRESSION:
        message = "Некорректное выражение.";
        break;
    case ERROR_ARGUMENTS:
        message = "Некорректное количество аргументов.";
        break;
    case ERROR_POSTFIX:
        message = "Обработка постфиксной формы завершилась некорректно.";
        break;
    case ERROR_DIVISION_BY_ZERO:
        message = "Деление на ноль.";
        break;
    case ERROR_EXPRESSION_TOO_LONG:
        message = "Выражение слишком длинное.";
        break;
    case ERROR_INPUT_FILE:
        message = "Не удалось открыть входной файл для чтения.";
        break;
    case ERROR_OUTPUT_FILE:
        message = "Не удалось открыть выходной файл для записи.";
        break;
    case ERROR_SAME_FILES:
        message = "Имена входного и выходного файлов должны различаться.";
        break;
    case ERROR_INVALID_ANSWER:
        message = "Введите Y (да) или N (нет).";
        break;
    default:
        message = "Неизвестная ошибка.";
        break;
    }

    std::cout << "ERROR " << code << ": " << message << std::endl;
    return false;
}

std::string Trim(const std::string& text)
{
    const std::string spaces = " \t\r\n";

    std::size_t first = text.find_first_not_of(spaces);
    if (first == std::string::npos)
    {
        return "";
    }

    std::size_t last = text.find_last_not_of(spaces);
    return text.substr(first, last - first + 1);
}

std::string NumberToString(double value)
{
    if (value == 0)
    {
        value = 0.0;
    }

    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string OrEmpty(const std::string& text)
{
    return text.empty() ? "(пусто)" : text;
}

void AppendText(std::string& text, const std::string& part)
{
    if (!text.empty())
    {
        text += ' ';
    }
    text += part;
}

std::string PostfixToString(const std::vector<Token>& postfixTokens)
{
    std::string text;

    for (std::size_t i = 0; i < postfixTokens.size(); i++)
    {
        AppendText(text, postfixTokens[i].value);
    }

    return text;
}

std::string OperatorStackToString(const OperatorStack& stack)
{
    std::string text;

    for (int i = 0; i < stack.top; i++)
    {
        AppendText(text, stack.tokens[i].value);
    }

    return OrEmpty(text);
}

std::string NumberStackToString(const NumberStack& stack)
{
    std::string text;

    for (int i = 0; i < stack.top; i++)
    {
        AppendText(text, NumberToString(stack.numbers[i]));
    }

    return OrEmpty(text);
}

void ShowConversionStep(
    int step,
    const std::string& token,
    const std::string& action,
    const OperatorStack& operatorStack,
    const std::vector<Token>& postfixTokens)
{
    std::cout << "Шаг " << step << ". Токен: " << token << std::endl;
    std::cout << "    Действие: " << action << std::endl;
    std::cout << "    Стек операторов: " << OperatorStackToString(operatorStack) << std::endl;
    std::cout << "    Выходная строка: " << OrEmpty(PostfixToString(postfixTokens)) << std::endl;
}

void ShowEvaluationStep(
    int step,
    const std::string& token,
    const std::string& action,
    const NumberStack& numberStack)
{
    std::cout << "Шаг " << step << ". Токен: " << token << std::endl;
    std::cout << "    Действие: " << action << std::endl;
    std::cout << "    Стек чисел: " << NumberStackToString(numberStack) << std::endl;
}

bool ReadFileName(const std::string& prompt, std::string& fileName)
{
    std::cout << prompt;

    if (!std::getline(std::cin, fileName))
    {
        return false;
    }

    fileName = Trim(fileName);
    return true;
}

bool OpenInputFile(std::ifstream& inputFile, const std::string& fileName)
{
    inputFile.open(fileName, std::ios::in);

    if (!inputFile.is_open())
    {
        return ReportError(ERROR_INPUT_FILE);
    }

    return true;
}

bool OpenOutputFile(std::ofstream& outputFile, const std::string& fileName)
{
    outputFile.open(fileName, std::ios::out);

    if (!outputFile.is_open())
    {
        return ReportError(ERROR_OUTPUT_FILE);
    }

    return true;
}

std::string ReadExpression(std::ifstream& inputFile)
{
    std::string expression;

    std::getline(inputFile, expression);

    if (!expression.empty() && expression.back() == '\r')
    {
        expression.pop_back();
    }

    return expression;
}

bool AskContinue()
{
    while (true)
    {
        std::cout << std::endl << "Обработать ещё одно выражение? (Y/n): ";

        std::string answer;

        if (!std::getline(std::cin, answer))
        {
            return false;
        }

        answer = Trim(answer);

        if (answer.empty() || answer == "Y" || answer == "y")
        {
            std::cout << std::endl;
            return true;
        }

        if (answer == "N" || answer == "n")
        {
            return false;
        }

        ReportError(ERROR_INVALID_ANSWER);
    }
}

bool IsDigit(char ch)
{
    return ALPHABET_DIGIT.find(ch) != std::string::npos;
}

bool IsLetter(char ch)
{
    return ALPHABET_LETTER.find(ch) != std::string::npos;
}

bool IsOperator(char ch)
{
    return ALPHABET_OPERATOR.find(ch) != std::string::npos;
}

bool IsFunction(const std::string& value)
{
    return value == "COS" ||
           value == "SIN" ||
           value == "EXP";
}

bool IsTokenTooLong(const std::string& value)
{
    return static_cast<int>(value.length()) > MAX_TOKEN_SIZE;
}

bool IsUnaryMinus(const std::string& expression, int index)
{
    if (expression[index] != '-')
    {
        return false;
    }
    index--;

    while (index >= 0 && expression[index] == ' ')
    {
        index--;
    }

    return index < 0 ||
           expression[index] == '(' ||
           IsOperator(expression[index]);
}

void SkipSpaces(const std::string& expression, int& index)
{
    int length = static_cast<int>(expression.length());

    while (index < length && expression[index] == ' ')
    {
        index++;
    }
}

Token GetNextToken(const std::string& expression, int& index)
{
    Token token;
    token.type = TOKEN_UNDEFINED;
    token.value = "";

    int length = static_cast<int>(expression.length());

    if (index >= length)
    {
        return token;
    }

    if (IsDigit(expression[index]))
    {
        token.type = TOKEN_NUMBER;

        while (index < length && IsDigit(expression[index]))
        {
            token.value += expression[index];
            index++;
        }

        if (IsTokenTooLong(token.value))
        {
            token.type = TOKEN_UNDEFINED;
            token.value = "";
        }
    }
    else if (IsLetter(expression[index]))
    {
        token.type = TOKEN_FUNCTION;

        while (index < length && IsLetter(expression[index]))
        {
            token.value += expression[index];
            index++;
        }

        if (IsTokenTooLong(token.value) || !IsFunction(token.value))
        {
            token.type = TOKEN_UNDEFINED;
            token.value = "";
        }
    }
    else if (IsOperator(expression[index]))
    {
        if (IsUnaryMinus(expression, index))
        {
            token.type = TOKEN_NEG;
            token.value = "NEG";
        }
        else
        {
            token.type = TOKEN_OPERATOR;
            token.value = expression[index];
        }
        index++;
    }
    else if (expression[index] == '(')
    {
        token.type = TOKEN_LEFT_BRACKET;
        token.value = expression[index];
        index++;
    }
    else if (expression[index] == ')')
    {
        token.type = TOKEN_RIGHT_BRACKET;
        token.value = expression[index];
        index++;
    }

    return token;
}

void InitOperatorStack(OperatorStack& stack)
{
    stack.top = 0;
}

bool IsOperatorStackEmpty(const OperatorStack& stack)
{
    return stack.top == 0;
}

bool IsOperatorStackFull(const OperatorStack& stack)
{
    return stack.top >= MAX_STACK_SIZE;
}

bool PushOperator(OperatorStack& stack, const Token& token)
{
    if (IsOperatorStackFull(stack))
    {
        return false;
    }

    stack.tokens[stack.top] = token;
    stack.top++;
    return true;
}

Token PopOperator(OperatorStack& stack)
{
    stack.top--;
    return stack.tokens[stack.top];
}

Token PeekOperator(const OperatorStack& stack)
{
    return stack.tokens[stack.top - 1];
}

int GetPriority(const std::string& operation)
{
    int priority = 0;

    if (operation == "-" || operation == "+")
    {
        priority = 1;
    }
    else if (operation == "*" || operation == "/")
    {
        priority = 2;
    }
    else if (operation == "NEG")
    {
        priority = 3;
    }
    else if (operation == "^")
    {
        priority = 4;
    }
    else if (operation == "SIN" || operation == "EXP" || operation == "COS")
    {
        priority = 5;
    }

    return priority;
}

bool IsRightAssociative(const std::string& operation)
{
    return operation == "^" || operation == "NEG";
}

bool ShouldPopOperator(
    const Token& stackToken,
    const Token& currentToken)
{
    if (currentToken.type == TOKEN_NEG)
    {
        return false;
    }

    int stackPriority = GetPriority(stackToken.value);
    int currentPriority = GetPriority(currentToken.value);

    if (IsRightAssociative(currentToken.value))
    {
        return stackPriority > currentPriority;
    }
    else
    {
        return stackPriority >= currentPriority;
    }
}

bool ConvertToPostfix(
    const std::string& expression,
    std::vector<Token>& postfixTokens)
{
    OperatorStack operatorStack;
    InitOperatorStack(operatorStack);

    std::cout << "=== Этап 1. Преобразование в постфиксную форму (алгоритм Дейкстры) ===" << std::endl;
    std::cout << "(стек операторов показан от дна к вершине)" << std::endl << std::endl;

    int index = 0;
    int step = 0;
    int length = static_cast<int>(expression.length());

    while (true)
    {
        SkipSpaces(expression, index);

        if (index >= length)
        {
            break;
        }

        Token currentToken = GetNextToken(expression, index);

        if (currentToken.type == TOKEN_UNDEFINED)
        {
            return ReportError(ERROR_INCORRECT_EXPRESSION);
        }

        std::string action;

        if (currentToken.type == TOKEN_NUMBER)
        {
            postfixTokens.push_back(currentToken);
            action = "число - переносим в выходную строку";
        }
        else if (currentToken.type == TOKEN_FUNCTION)
        {
            if (!PushOperator(operatorStack, currentToken))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }
            action = "функция - помещаем в стек";
        }
        else if (currentToken.type == TOKEN_OPERATOR || currentToken.type == TOKEN_NEG)
        {
            std::string popped;

            while (!IsOperatorStackEmpty(operatorStack))
            {
                Token stackToken = PeekOperator(operatorStack);

                if (stackToken.type == TOKEN_LEFT_BRACKET)
                {
                    break;
                }

                if (!ShouldPopOperator(stackToken, currentToken))
                {
                    break;
                }

                Token poppedToken = PopOperator(operatorStack);
                postfixTokens.push_back(poppedToken);
                AppendText(popped, poppedToken.value);
            }

            if (!PushOperator(operatorStack, currentToken))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }

            if (!popped.empty())
            {
                action = "выталкиваем в выходную строку: " + popped + "; ";
            }
            action += "помещаем '" + currentToken.value + "' в стек";
        }
        else if (currentToken.type == TOKEN_LEFT_BRACKET)
        {
            if (!PushOperator(operatorStack, currentToken))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }
            action = "помещаем '(' в стек";
        }
        else if (currentToken.type == TOKEN_RIGHT_BRACKET)
        {
            bool leftBracketFound = false;
            std::string popped;
            std::string functionName;

            while (!IsOperatorStackEmpty(operatorStack))
            {
                Token stackToken = PopOperator(operatorStack);

                if (stackToken.type != TOKEN_LEFT_BRACKET)
                {
                    postfixTokens.push_back(stackToken);
                    AppendText(popped, stackToken.value);
                }
                else
                {
                    leftBracketFound = true;

                    if (!IsOperatorStackEmpty(operatorStack))
                    {
                        if (PeekOperator(operatorStack).type == TOKEN_FUNCTION)
                        {
                            Token functionToken = PopOperator(operatorStack);
                            postfixTokens.push_back(functionToken);
                            functionName = functionToken.value;
                        }
                    }

                    break;
                }
            }

            if (!leftBracketFound)
            {
                return ReportError(ERROR_INCORRECT_EXPRESSION);
            }

            action = "выталкиваем в выходную строку до '(': " +
                     (popped.empty() ? std::string("ничего") : popped) +
                     "; '(' удаляем из стека";

            if (!functionName.empty())
            {
                action += "; функцию " + functionName + " - в выходную строку";
            }
        }

        step++;
        ShowConversionStep(step, currentToken.value, action, operatorStack, postfixTokens);
    }

    std::string flushed;

    while (!IsOperatorStackEmpty(operatorStack))
    {
        Token stackToken = PopOperator(operatorStack);

        if (stackToken.type == TOKEN_LEFT_BRACKET)
        {
            return ReportError(ERROR_INCORRECT_EXPRESSION);
        }

        postfixTokens.push_back(stackToken);
        AppendText(flushed, stackToken.value);
    }

    step++;
    ShowConversionStep(
        step,
        "(конец выражения)",
        flushed.empty() ? std::string("стек пуст - выталкивать нечего")
                        : "выталкиваем остаток стека в выходную строку: " + flushed,
        operatorStack,
        postfixTokens);

    return true;
}

void InitNumberStack(NumberStack& stack)
{
    stack.top = 0;
}

bool IsNumberStackEmpty(const NumberStack& stack)
{
    return stack.top == 0;
}

bool IsNumberStackFull(const NumberStack& stack)
{
    return stack.top >= MAX_STACK_SIZE;
}

bool PushNumber(NumberStack& stack, double number)
{
    if (IsNumberStackFull(stack))
    {
        return false;
    }

    stack.numbers[stack.top] = number;
    stack.top++;
    return true;
}

double PopNumber(NumberStack& stack)
{
    stack.top--;
    return stack.numbers[stack.top];
}

bool ApplyBinaryOperator(
    const std::string& operation,
    double leftOperand,
    double rightOperand,
    double& result)
{
    result = 0;

    if (operation == "+")
    {
        result = leftOperand + rightOperand;
    }
    else if (operation == "-")
    {
        result = leftOperand - rightOperand;
    }
    else if (operation == "*")
    {
        result = leftOperand * rightOperand;
    }
    else if (operation == "/")
    {
        if (rightOperand == 0)
        {
            return false;
        }

        result = leftOperand / rightOperand;
    }
    else if (operation == "^")
    {
        result = std::pow(leftOperand, rightOperand);
    }

    return true;
}

double ApplyFunction(
    const std::string& functionName,
    double value)
{
    double result = 0;

    if (functionName == "SIN")
    {
        result = std::sin(value);
    }
    else if (functionName == "COS")
    {
        result = std::cos(value);
    }
    else if (functionName == "EXP")
    {
        result = std::exp(value);
    }

    return result;
}

bool EvaluatePostfix(
    const std::vector<Token>& postfixTokens,
    double& result)
{
    NumberStack numberStack;
    InitNumberStack(numberStack);

    std::cout << "=== Этап 2. Вычисление постфиксной записи ===" << std::endl;
    std::cout << "(стек чисел показан от дна к вершине)" << std::endl << std::endl;

    for (std::size_t i = 0; i < postfixTokens.size(); i++)
    {
        const Token& token = postfixTokens[i];
        std::string action;

        if (token.type == TOKEN_NUMBER)
        {
            double number = std::stod(token.value);

            if (!PushNumber(numberStack, number))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }

            action = "число - помещаем в стек";
        }
        else if (token.type == TOKEN_OPERATOR)
        {
            if (numberStack.top < 2)
            {
                return ReportError(ERROR_ARGUMENTS);
            }

            double rightOperand = PopNumber(numberStack);
            double leftOperand = PopNumber(numberStack);
            double value = 0;

            if (!ApplyBinaryOperator(token.value, leftOperand, rightOperand, value))
            {
                return ReportError(ERROR_DIVISION_BY_ZERO);
            }

            if (!PushNumber(numberStack, value))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }

            action = "извлекаем два числа, вычисляем " +
                     NumberToString(leftOperand) + " " + token.value + " " +
                     NumberToString(rightOperand) + " = " + NumberToString(value) +
                     ", результат - в стек";
        }
        else if (token.type == TOKEN_NEG)
        {
            if (numberStack.top < 1)
            {
                return ReportError(ERROR_ARGUMENTS);
            }

            double operand = PopNumber(numberStack);

            if (!PushNumber(numberStack, -operand))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }

            action = "извлекаем число, меняем знак: -(" + NumberToString(operand) + ") = " +
                     NumberToString(-operand) + ", результат - в стек";
        }
        else if (token.type == TOKEN_FUNCTION)
        {
            if (numberStack.top < 1)
            {
                return ReportError(ERROR_ARGUMENTS);
            }

            double operand = PopNumber(numberStack);
            double value = ApplyFunction(token.value, operand);

            if (!PushNumber(numberStack, value))
            {
                return ReportError(ERROR_STACK_OVERFLOW);
            }

            action = "извлекаем число, вычисляем " + token.value + "(" +
                     NumberToString(operand) + ") = " + NumberToString(value) +
                     ", результат - в стек";
        }

        ShowEvaluationStep(static_cast<int>(i) + 1, token.value, action, numberStack);
    }

    if (numberStack.top != 1)
    {
        return ReportError(ERROR_POSTFIX);
    }

    result = PopNumber(numberStack);
    return true;
}

void WritePostfix(
    std::ofstream& outputFile,
    const std::vector<Token>& postfixTokens)
{
    outputFile << PostfixToString(postfixTokens) << std::endl;
}

void WriteResult(
    std::ofstream& outputFile,
    double result)
{
    outputFile << NumberToString(result) << std::endl;
}

bool ProcessExpression(
    std::ifstream& inputFile,
    std::ofstream& outputFile)
{
    std::string expression = ReadExpression(inputFile);

    if (static_cast<int>(expression.length()) > MAX_EXPRESSION_SIZE)
    {
        return ReportError(ERROR_EXPRESSION_TOO_LONG);
    }

    std::cout << std::endl << "Выражение: " << expression << std::endl << std::endl;

    std::vector<Token> postfixTokens;

    if (!ConvertToPostfix(expression, postfixTokens))
    {
        return false;
    }

    std::cout << std::endl;

    double result = 0;

    if (!EvaluatePostfix(postfixTokens, result))
    {
        return false;
    }

    std::cout << std::endl;
    std::cout << "Постфиксная форма: " << PostfixToString(postfixTokens) << std::endl;
    std::cout << "Результат: " << NumberToString(result) << std::endl;

    WritePostfix(outputFile, postfixTokens);
    WriteResult(outputFile, result);

    return true;
}

bool HandleRequest()
{
    std::string inputFileName;
    std::string outputFileName;

    if (!ReadFileName("Введите имя входного файла: ", inputFileName))
    {
        return false;
    }

    if (!ReadFileName("Введите имя выходного файла: ", outputFileName))
    {
        return false;
    }

    if (inputFileName == outputFileName)
    {
        ReportError(ERROR_SAME_FILES);
        return true;
    }

    std::ifstream inputFile;
    std::ofstream outputFile;

    if (!OpenInputFile(inputFile, inputFileName))
    {
        return true;
    }

    if (!OpenOutputFile(outputFile, outputFileName))
    {
        inputFile.close();
        return true;
    }

    if (ProcessExpression(inputFile, outputFile))
    {
        std::cout << "Постфиксная форма и результат записаны в файл: " << outputFileName << std::endl;
    }

    inputFile.close();
    outputFile.close();

    return true;
}

int main()
{
    while (HandleRequest() && AskContinue())
    {
    }

    return 0;
}