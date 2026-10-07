#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

   /*1) указывать  имена входного  и  выходного   файлов   через
командную строку или с клавиатуры в процессе работы программы;
   2) обрабатывать ошибки открытия файлов;
   3) ориентироваться на кодировку ANSI (1251);
   3) не использовать  стандартные функции работы со  строками
      и символами
      COPY, POS, Str, Val и т. п. из ПАСКАЛя; 
      stoi, to_string, substr, strstr, find, regex и т. п.  из 
      C++;
   4) иметь возможность работы с текстами на русском языке;
   5) не переписывать файл в оперативную память целиком.

   Задано  некоторое  слово.  Требуется  составить из букв
этого  слова  максимальное  количество  других  слов,  которые 
имеются  в  словаре.  Каждая  буква  заданного   слова   может 
использоваться  только  один  раз.  Например,  если в заданном
слове  имеется 2 буквы 'а', то слова словаря, в которых больше
двух  букв 'а', не подходят. Каждое найденное слово из словаря
оценивается  количеством очков, равным  длине слова. Результат
игры определяется суммой  очков. Первая  строка входного файла
содержит  заданное  слово. Далее  находятся слова словаря. Все 
слова  состоят   из   строчных  (маленьких)  латинских   букв. 
Количество слов словаря  не  превышает 100. В каждом  слове не 
более 20  букв. Длина  заданного  слова  также  не  больше  20 
символов. В первой строке выводится количество набранных очков. 
Начиная со второй строки, следуют найденные слова. Они  должны
выводиться  по  одному  в  строке по убыванию количества букв. 
Слова  с  одинаковым  количеством  букв  должны  следовать  по 
алфавиту (8).
   
   
   Чижов Дмитрий 
   Visual Studio Code
   
   
   */

const int ALPHABET_SIZE = 59;

struct Word
{
    string text;
    int score;
};

string ReadFileName()
{
    string fileName;

    cin >> fileName;

    return fileName;
}

bool OpenInputFile(ifstream& inputFile, const string& fileName)
{
    inputFile.open(fileName, ios::in);

    if (!inputFile.is_open())
    {
        cout << "The input file could not be opened for reading." << endl;
        return false;
    }

    return true;
}

bool OpenOutputFile(ofstream& outputFile, const string& fileName)
{
    outputFile.open(fileName, ios::out);

    if (!outputFile.is_open())
    {
        cout << "The output file could not be opened for writing." << endl;
        return false;
    }

    return true;
}

string ReadSourceWord(ifstream& inputFile)
{
    string sourceWord;

    inputFile >> sourceWord;

    return sourceWord;
}

int GetLetterIndex(char ch)
{
    if (ch >= 'a' && ch <= 'z')
    {
        return ch - 'a';
    }

    if (ch == '�')
    {
        return 32;
    }

    return ch - '�' + 26;
}

void CountLetters(const string& word, int letterCounts[])
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        letterCounts[i] = 0;
    }

    for (char ch: word) 
    {
        letterCounts[GetLetterIndex(ch)]++;
    }
}

bool CanComposeWord(const string& word, const int sourceLetterCounts[])
{
    int letterCounts[ALPHABET_SIZE];
    CountLetters(word, letterCounts);

    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (letterCounts[i] > sourceLetterCounts[i]) 
        {
            return false;
        }
    }

    return true;
}

void AddWord(vector<Word>& words, const string& word)
{
    Word newWord;

    newWord.text = word;
    newWord.score = word.length();

    words.push_back(newWord);
}

void ReadDictionary(
    ifstream& inputFile,
    const int sourceLetterCounts[],
    vector<Word>& words)
{
    string word;

    while (getline(inputFile, word))
    {
        if (word != "") 
        {
            if (CanComposeWord(word, sourceLetterCounts))
            {
                AddWord(words, word);
            }
        }
    }
}

bool CompareWords(const Word& first, const Word& second)
{
    if (first.score != second.score)
    {
        return first.score > second.score;
    }

    return first.text < second.text;
}

int CalculateScore(const vector<Word>& words)
{
    int totalScore = 0;

    for (int i = 0; i < words.size(); i++)
    {
        totalScore += words[i].score;
    }

    return totalScore;
}

void WriteResult(
    ofstream& outputFile,
    const vector<Word>& words,
    int score)
{
    outputFile << score << endl;

    for (int i = 0; i < words.size(); i++)
    {
        outputFile << words[i].text << endl;
    }
}

int main()
{
    string inputFileName;
    string outputFileName;

    ifstream inputFile;
    ofstream outputFile;

    vector<Word> words;

    int sourceLetterCounts[ALPHABET_SIZE];

    cout << "Enter the name of the input file: ";
    inputFileName = ReadFileName();
    cout << "Enter the name of the output file: ";
    outputFileName = ReadFileName();

    if (!OpenInputFile(inputFile, inputFileName))
    {
        return 1;
    }

    if (!OpenOutputFile(outputFile, outputFileName))
    {
        return 1;
    }

    string sourceWord = ReadSourceWord(inputFile);

    CountLetters(sourceWord, sourceLetterCounts);

    ReadDictionary(inputFile, sourceLetterCounts, words);

    int score = CalculateScore(words);

    sort(words.begin(), words.end(), CompareWords);

    WriteResult(outputFile, words, score);

    inputFile.close();
    outputFile.close();

    return 0;
}