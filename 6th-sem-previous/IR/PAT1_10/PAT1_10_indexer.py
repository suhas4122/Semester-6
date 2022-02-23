import sys
import os
import re
import pickle
from tqdm import tqdm                                                                     # for progress bar
import codecs
from collections import Counter

import nltk
nltk.download('stopwords')
nltk.download('punkt')
nltk.download('wordnet')
from nltk.corpus import stopwords
from nltk.tokenize import word_tokenize
from nltk.stem import WordNetLemmatizer

# Getting all files
files = []
for directory in os.listdir(sys.argv[1]):
    pathofdir = os.path.join(sys.argv[1],directory)
    listoffiles = os.listdir(pathofdir)
    pathoffiles = [os.path.join(pathofdir,i) for i in listoffiles]
    files+=pathoffiles
files.sort()                                                                              # to get postings sorted
print("Total number of files: ", len(files))
                
inverted_index = {}
swords = stopwords.words('english')
wnl = WordNetLemmatizer()

for k in tqdm(range(len(files))):
#for k in tqdm(range(100)):
    doc_id = os.path.basename(files[k])                                                   # getting doc id
    file = codecs.open(files[k], "r", "utf-8")                                            # reading files        
    test_str = file.read()

    reg_str = "<TEXT>(.*?)</TEXT>"
    res = str(re.findall(reg_str, test_str, flags=re.DOTALL))                                              # get text between <TEXT> </TEXT>
    #     res = str(re.sub(r'[\w][^\w\s][\w]', ' ', res))                                                # removing punctuations
    res = str(re.sub(r'[^\w\s]', ' ', res))                                                # removing punctuations
    file_text = res[1:len(res)]

    text_tokens = word_tokenize(file_text)                                                # tokenization
    text_tokens = [word.lower() for word in text_tokens]                                  # lowercase
    tokens_without_sw = [word for word in text_tokens if not word in swords]              # remove stop words
    lemmatized_tokens = [wnl.lemmatize(token) for token in tokens_without_sw]             # lemmatization

    lemmatized_tokens_with_tf = Counter(lemmatized_tokens)                                # storing TF for part 2
    for k,v in lemmatized_tokens_with_tf.items():
        if k not in inverted_index.keys():
            inverted_index[k] = [(doc_id,v)]
        else:
            inverted_index[k].append( (doc_id,v) )
del inverted_index["n"]             #removing tokens that we got bcz of new line
inverted_index = dict(sorted(inverted_index.items()))                                     # sorting vocabulary 
sort_ii = inverted_index.copy()
for key in sort_ii.keys():
    s = sort_ii[key].sort()
# Uncomment to see inverted index
# for k,v in inverted_index.items():
#     print(k , " Postings ", v, "\n\n")

# Pickle inverted index
with open('model_queries_10.pth', 'wb') as save_index:
    pickle.dump(sort_ii, save_index)

# Uncomment to verify if saved properly
# with open('model_queries_10.pth', 'rb') as f:
#     load_index = pickle.load(f)
# print(load_index == inverted_index)

