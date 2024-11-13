#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include "json.hpp"
#include <memory>
#include <string_view>

using json = nlohmann::json;
using namespace std;

// Use string_view for read-only string operations
using StringPair = pair<string_view, string_view>;

// Custom hash function for string_view pairs
struct PairHash {
    size_t operator()(const StringPair& p) const {
        return hash<string_view>{}(p.first) ^ (hash<string_view>{}(p.second) << 1);
    }
};

// Use unordered_map for O(1) lookups instead of map's O(log n)
static const unordered_map<string, string> arpabetToIpa = {
    {"AA", "ɑ"},
    {"AE", "æ"},
    {"AH", "ʌ"},
    {"AO", "ɔ"},
    {"AW", "aʊ"},
    {"AY", "aɪ"},
    {"B", "b"},
    {"CH", "tʃ"},
    {"D", "d"},
    {"DH", "ð"},
    {"EH", "ɛ"},
    {"ER", "ɛ"},  // "ɝ"
    {"EY", "eɪ"},
    {"F", "f"},
    {"G", "ɡ"},
    {"HH", "h"},
    {"IH", "ɪ"},
    {"IY", "i"},
    {"JH", "dʒ"},
    {"K", "k"},
    {"L", "l"},
    {"M", "m"},
    {"N", "n"},
    {"NG", "ŋ"},
    {"OW", "oʊ"},
    {"OY", "ɔɪ"},
    {"P", "p"},
    {"R", "ɹ"},
    {"S", "s"},
    {"SH", "ʃ"},
    {"T", "t"},
    {"TH", "θ"},
    {"UH", "ʊ"},
    {"UW", "u"},
    {"V", "v"},
    {"W", "w"},
    {"Y", "j"},
    {"Z", "z"},
    {"ZH", "ʒ"}
};

class ConlangGenerator {
private:
    unique_ptr<mt19937> rng;
    unordered_map<string, double> wordFrequency;
    unordered_map<string, vector<string>> wordToArpabet;
    json ipaFeatureMapping;
    vector<vector<string>> words;
    const int vocabSize;
    
    // Cache for phoneme distances
    unordered_map<StringPair, int, PairHash> phonemeDistanceCache;

public:
    ConlangGenerator(int size = 100) : vocabSize(size) {
        rng = make_unique<mt19937>(random_device{}());
    }

private:
    int getPhonemeDistance(string_view phoneme1, string_view phoneme2) {
        if (phoneme1.empty() || phoneme2.empty()) return 1;
        
        StringPair pair{phoneme1, phoneme2};
        auto it = phonemeDistanceCache.find(pair);
        if (it != phonemeDistanceCache.end()) {
            return it->second;
        }
        
        int distance = 0;
        const auto& features1 = ipaFeatureMapping[string(phoneme1)];
        const auto& features2 = ipaFeatureMapping[string(phoneme2)];
        
        for (const auto& [key, value] : features1.items()) {
            if (features2.contains(key) && value != features2[key]) {
                distance++;
            }
        }
        
        phonemeDistanceCache[pair] = distance;
        return distance;
    }

    int levenshteinDistance(const vector<string>& a, const vector<string>& b) {
        const int m = a.size(), n = b.size();
        vector<int> v0(n + 1), v1(n + 1);

        for (int i = 0; i <= n; i++) {
            v0[i] = i > 0 ? getPhonemeDistance("", b[i-1]) * i : 0;
        }

        for (int i = 0; i < m; i++) {
            v1[0] = (i + 1) * getPhonemeDistance("", a[i]);

            for (int j = 0; j < n; j++) {
                int deletionCost = v0[j + 1] + getPhonemeDistance("", a[i]);
                int insertionCost = v1[j] + getPhonemeDistance("", b[j]);
                int substitutionCost = v0[j] + 
                    (a[i] != b[j]) * getPhonemeDistance(a[i], b[j]);

                v1[j + 1] = min({deletionCost, insertionCost, substitutionCost});
            }
            swap(v0, v1);
        }
        return v0[n];
    }

    vector<vector<string>> generateConlangIpaWords(int numSyllables) {
        static const vector<string> consonants = {"l", "n", "s", "t", "k", "v", "b"};
        static const vector<string> vowels = {"ɑ", "ɛ", "oʊ", "ʌ", "i", "a"};
        static const vector<vector<string>> languageFunctionSyllables = {{"l", "ɑ"}, {"l", "ʌ"}, {"s", "ɛ"}, {"v", "oʊ"}, {"k", "ɑ"}, {"n", "a"}, {"n", "i"}};
        vector<vector<string>> result;
        
        // Pre-calculate capacity needed
        int capacity = pow(consonants.size() * vowels.size(), numSyllables);
        result.reserve(capacity);
        
        if (numSyllables == 1) {
            for (const auto& c : consonants) {
                for (const auto& v : vowels) {
                    vector<string> newWord = {c, v};

                    if(count(languageFunctionSyllables.begin(), languageFunctionSyllables.end(), newWord) > 0){
                        continue;
                    }

                    result.push_back({c, v});
                }
            }
        } else {
            auto subWords = generateConlangIpaWords(numSyllables - 1);
            for (const auto& c : consonants) {
                for (const auto& v : vowels) {
                    for (const auto& subWord : subWords) {
                        vector<string> newWord = {c, v};

                        if(count(languageFunctionSyllables.begin(), languageFunctionSyllables.end(), newWord) > 0){
                            continue;
                        }

                        newWord.insert(newWord.end(), subWord.begin(), subWord.end());
                        result.push_back(move(newWord));
                    }
                }
            }
        }
        return result;
    }

    double wordLoss(const vector<string>& word, const vector<string>& conlangWord) {
        return levenshteinDistance(word, conlangWord) + 10 * conlangWord.size();
    }

public:
    void loadData() {
        // Optimize file reading with larger buffer
        constexpr size_t BUFFER_SIZE = 1 << 16;  // 64KB buffer
        char buffer[BUFFER_SIZE];
        
        {
            unordered_map<std::string, double> allWordFrequency;
            ifstream file("wordFrequencies.txt", ios::binary);
            file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
            string word;
            double frequency;
            while (file >> word >> frequency) {
                transform(word.begin(), word.end(), word.begin(), ::toupper);
                allWordFrequency[move(word)] = frequency;
            }

            ifstream file2("basic-english.txt", ios::binary);
            file2.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
            while (file2 >> word) {
                transform(word.begin(), word.end(), word.begin(), ::toupper);
                if(allWordFrequency.count(word)){
                    wordFrequency[move(word)] = allWordFrequency[word];
                }
                else{
                    wordFrequency[move(word)] = 0;
                }
            }
        }
        
        {
            ifstream file("cmudict-0.7b", ios::binary);
            file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
            string line;
            while (getline(file, line)) {
                istringstream iss(line);
                string word;
                iss >> word;
                
                vector<string> arpabet;
                string phoneme;
                while (iss >> phoneme) {
                    phoneme.erase(
                        remove_if(phoneme.begin(), phoneme.end(), ::isdigit),
                        phoneme.end()
                    );
                    arpabet.push_back(move(phoneme));
                }
                wordToArpabet[move(word)] = move(arpabet);
            }
        }

        {
            ifstream f("ipaFeatureMapping.json");
            ipaFeatureMapping = json::parse(f, nullptr, false, true);
        }
    }

    void generate() {
        // Sort words by frequency
        vector<pair<string, double>> sortedWords(wordFrequency.begin(), wordFrequency.end());
        sort(sortedWords.begin(), sortedWords.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });

        // Generate words of different syllable lengths
        auto words1 = generateConlangIpaWords(1);
        auto words2 = generateConlangIpaWords(2);
        auto words3 = generateConlangIpaWords(3);

        words.reserve(words1.size() + words2.size() + words3.size());
        words.insert(words.end(), 
                    make_move_iterator(words1.begin()), 
                    make_move_iterator(words1.end()));
        words.insert(words.end(), 
                    make_move_iterator(words2.begin()), 
                    make_move_iterator(words2.end()));
        words.insert(words.end(), 
                    make_move_iterator(words3.begin()), 
                    make_move_iterator(words3.end()));

        // Find best matches for words
        unordered_map<string, vector<string>> conlangVocab;
        double minLoss = 1e12;
        
        for (const auto& [englishWord, freq] : sortedWords) {
            if (conlangVocab.size() >= vocabSize) break;
            if (!wordToArpabet.count(englishWord)) continue;

            const auto& wordIpaPhonemes = getArpabetToIpa(wordToArpabet[englishWord]);
            
            double bestLoss = 1e9;
            size_t bestIdx = 0;
            
            for (size_t i = 0; i < words.size(); i++) {
                double currLoss = wordLoss(wordIpaPhonemes, words[i]);
                if (currLoss < bestLoss) {
                    bestLoss = currLoss;
                    bestIdx = i;
                }
            }
            
            conlangVocab[englishWord] = words[bestIdx];
            words.erase(words.begin() + bestIdx);
        }

        // Save vocabulary to JSON file
        json output(conlangVocab);
        ofstream outFile("conlangVocab.json");
        outFile << output.dump(4);
    }

    vector<string> getArpabetToIpa(const vector<string>& word) {
        vector<string> ipa;
        ipa.reserve(word.size());
        for (const auto& phoneme : word) {
            ipa.push_back(arpabetToIpa.at(phoneme));
        }
        return ipa;
    }
};

int main() {
    try {
        ConlangGenerator generator(600);
        generator.loadData();
        generator.generate();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}