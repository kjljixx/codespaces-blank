import json
import random
import math

arpabetToIpa = {
    "AA": "ɑ",
    "AE": "æ",
    "AH": "ʌ",
    "AO": "ɔ",
    "AW": "aʊ",
    "AY": "aɪ",
    "B": "b",
    "CH": "tʃ",
    "D": "d",
    "DH": "ð",
    "EH": "ɛ",
    "ER": "ɛ",#"ɝ",
    "EY": "eɪ",
    "F": "f",
    "G": "ɡ",
    "HH": "h",
    "IH": "ɪ",
    "IY": "i",
    "JH": "dʒ",
    "K": "k",
    "L": "l",
    "M": "m",
    "N": "n",
    "NG": "ŋ",
    "OW": "oʊ",
    "OY": "ɔɪ",
    "P": "p",
    "R": "ɹ",
    "S": "s",
    "SH": "ʃ",
    "T": "t",
    "TH": "θ",
    "UH": "ʊ",
    "UW": "u",
    "V": "v",
    "W": "w",
    "Y": "j",
    "Z": "z",
    "ZH": "ʒ"
}

def getWordsWithFrequency(fileName: str):
    wordFrequency = {}
    with open(fileName, 'r') as file:
        for line in file:
            s = line.split()
            word = s[0].upper()
            frequency = float(s[1])
            wordFrequency[word] = frequency
    return wordFrequency

def getWordsToArpabet(fileName: str):
    wordToArpabet = {}
    with open(fileName, 'r') as file:
        for line in file:
            s = line.split()
            word = s[0]
            arpabet = s[1:]
            for i in range(len(arpabet)):
                arpabet[i] = arpabet[i].strip("0123456789")
            wordToArpabet[word] = arpabet
    return wordToArpabet

def getArpabetToIpa(word: list):
    ipa = []
    for phoneme in word:
        ipa.append(arpabetToIpa[phoneme])
    return ipa

def getPhonemeDistance(phoneme1: str, phoneme2: str, ipaFeatureMapping: dict):
    phoneme1Features = ipaFeatureMapping[phoneme1]
    phoneme2Features = ipaFeatureMapping[phoneme2]
    distance = 0
    for i in phoneme1Features.keys():
        if phoneme1Features[i] != phoneme2Features[i]:
            distance += 1
    return distance

def getAllPhonemeDistances(ipaFeatureMapping: dict):
    phonemeDistances = {}
    for phoneme1 in ipaFeatureMapping.keys():
        for phoneme2 in ipaFeatureMapping.keys():
            phonemeDistances[(phoneme1, phoneme2)] = getPhonemeDistance(phoneme1, phoneme2, ipaFeatureMapping)
    return phonemeDistances

def levenshteinDistance(a: str, b: str, ipaFeatureMapping: dict, phonemeDistances: dict) -> int:
    m, n = len(a), len(b)
    v0 = [0] * (n + 1)
    v1 = [0] * (n + 1)

    for i in range(n + 1):
        v0[i] = i * phonemeDistances[("", b[i-1])] if i > 0 else 0

    for i in range(m):
        v1[0] = (i + 1) * phonemeDistances[("", a[i])]

        for j in range(n):
            deletionCost = v0[j + 1] + phonemeDistances[("", a[i])]
            insertionCost = v1[j] + phonemeDistances[("", b[j])]
            substitutionCost = v0[j] + (a[i] != b[j]) * phonemeDistances[(a[i], b[j])]

            v1[j + 1] = min(deletionCost, insertionCost, substitutionCost)

        v0, v1 = v1, v0

    return v0[n]
# def levenshteinDistance(a: str, b: str, ipaFeatureMapping: dict, phonemeDistances: dict, i: int = arbitraryNegInt, j: int = arbitraryNegInt) -> int:
#     if(i == arbitraryNegInt):
#         i = len(a)-1
#         j = len(b)-1

#     # print(i, j, char)
#     if(i == -1 and j == -1):
#         return 0

#     deletion = levenshteinDistance(a, b, ipaFeatureMapping, phonemeDistances, i-1, j)+phonemeDistances[("", a[i])] if i >= 0 else 100000000
#     addition = levenshteinDistance(a, b, ipaFeatureMapping, phonemeDistances, i, j-1)+phonemeDistances[("", b[j])] if j >= 0 else 100000000
#     substitution = levenshteinDistance(a, b, ipaFeatureMapping, phonemeDistances, i-1, j-1)+(a[i] != b[j])*phonemeDistances[(a[i], b[j])] if (i >= 0 and j >= 0) else 100000000
#     minDist = min(deletion, addition, substitution)

#     return minDist

def generateConlangIpaWords(numSyllables: int):
    consonants = ["l", "n", "s", "t", "k", "v", "b"] #l, n, s, t
    vowels = ["ɑ", "ɛ", "oʊ", "ʌ"] #ah, eh, oh, uh
    syllables = []
    for consonant in consonants:
        for vowel in vowels:
            syllables.append([consonant, vowel])

    words = []
    newWords = []
    for i in range(len(syllables)):
        words.append(syllables[i])
    for i in range(numSyllables-1):
        for word in words:
            for syllable in syllables:
                newWords.append(word+syllable)
        words = newWords
        newWords = []
    return words

def wordLoss(word: str, conlangWord: str, ipaFeatureMapping: dict, phonemeDist: dict):
    return levenshteinDistance(word, conlangWord, ipaFeatureMapping, phonemeDist)+5*len(conlangWord)

def conlangVocabLoss(conlangVocab: dict, wordFrequency: dict, ipaFeatureMapping: dict, phonemeDist: dict):
    loss = 0
    for word in conlangVocab.keys():
        loss += wordFrequency[word]*(wordLoss(conlangVocab[word][0], conlangVocab[word][1], ipaFeatureMapping, phonemeDist))
    return loss

if __name__ == "__main__":
    random.seed(0)
    wordFrequency = getWordsWithFrequency("test.txt")
    wordToArpabet = getWordsToArpabet("cmudict-0.7b")
    ipaFeatureMapping = json.load(open("ipaFeatureMapping.json"))
    phonemeDistances = getAllPhonemeDistances(ipaFeatureMapping)

    minLoss = 10000000000000
    minLossConlangVocab = {}
    vocabSize = 10
    words = generateConlangIpaWords(1) + generateConlangIpaWords(2) + generateConlangIpaWords(3) + generateConlangIpaWords(4)
    print("Words generated")
    for iter in range(1):
        wordsSubset = words.copy()
        random.shuffle(wordsSubset)
        wordsSubset = wordsSubset[:vocabSize*100]
        print("Iter", iter, "Shuffled Words")
        conlangVocab = {}
        loss = 0
        for word in wordFrequency.keys():
            wordIpaPhonemes = getArpabetToIpa(wordToArpabet[word])

            currWordMinLoss = 1000000
            currWordMinLossWord = -1
            for i in range(len(wordsSubset)):
                currWordLoss = wordLoss(wordIpaPhonemes, wordsSubset[i], ipaFeatureMapping, phonemeDistances)
                if(currWordLoss < currWordMinLoss):
                    currWordMinLoss = currWordLoss
                    currWordMinLossWord = i
            loss += wordFrequency[word]*currWordMinLoss
            conlangVocab[word] = wordsSubset[currWordMinLossWord]
            wordsSubset.pop(currWordMinLossWord)
            if(len(conlangVocab) == vocabSize):
                break
        print("Iter", iter, "Generated Dictionary")

        if(loss < minLoss):
            minLoss = loss
            minLossConlangVocab = conlangVocab
            print(conlangVocab)
            print(loss/vocabSize)
    #Output Vocab to JSON file
    with open("conlangVocab.json", "w") as file:
        json.dump(minLossConlangVocab, file)