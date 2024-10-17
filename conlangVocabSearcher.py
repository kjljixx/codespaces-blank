# search conlang vocab for user input
import json

def load_conlang_vocab(file_path):
    with open(file_path, 'r') as file:
        return json.load(file)

def search_conlang_vocab(vocab, query):
    results = []
    for word, details in vocab.items():
        if query.lower() in word.lower():
            results.append((word, details))
    return results

ipa_to_conlang_dict = {
    "l":"l", "n":"n", "s":"s", "t":"t",
    "ɑ":"ah", "ɛ":"eh", "oʊ":"oh", "ʌ":"uh"
}

def ipa_to_conlang(ipa):
    # convert ipa to conlang
    conlang = ""
    for char in ipa:
        if char in ipa_to_conlang_dict:
            conlang += ipa_to_conlang_dict[char]
        else:
            conlang += char
    return conlang

if __name__ == "__main__":
    vocab_file_path = 'conlangVocab.json'
    conlang_vocab = load_conlang_vocab(vocab_file_path)
    while True:
        user_query = input("Enter a word to search in the conlang vocabulary: ")
        search_results = search_conlang_vocab(conlang_vocab, user_query)
        
        if search_results:
            print("Search results:")
            for word, conlang in search_results:
                print(f"Word: {word}, Translated Pronunciation: {ipa_to_conlang(conlang)}")
        else:
            print("No matching words found in the conlang vocabulary.")