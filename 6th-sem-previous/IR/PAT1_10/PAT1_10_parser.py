import sys
import os
import re

import nltk
nltk.download('stopwords')
nltk.download('punkt')
nltk.download('wordnet')
from nltk.corpus import stopwords
from nltk.tokenize import word_tokenize
from nltk.stem import WordNetLemmatizer

# Read the raw query file
query_dict = {}
query_files = open(sys.argv[1], "r")                                                    # Path passed as input
test_str = query_files.read()

# Parse the file to extract query_id and query_text
reg_str = "<title>(.*?)</title>"
query_text = re.findall(reg_str, test_str) 
reg_str = "<num>(.*?)</num>"
query_id = re.findall(reg_str, test_str)

swords = stopwords.words('english')
wnl = WordNetLemmatizer()

for i in range(len(query_text)):
    query = query_text[i]
    query_without_punct = str(re.sub(r'[^\w\s]', ' ', query))                            # remove punctuations
    text_tokens = word_tokenize(query_without_punct)                                    # tokenize
    text_tokens = [word.lower() for word in text_tokens]                                # lowercase
    tokens_without_sw = [word for word in text_tokens if not word.lower() in swords]    # remove stopwords
    lemmatized_tokens = [wnl.lemmatize(token) for token in tokens_without_sw]           # lemmatize
    final_query_text = " ".join(lemmatized_tokens)
    query_dict[query_id[i]] = final_query_text                                          # save in dict

# Save the list of queries
f = open("./queries_10.txt", "w")
for key in query_dict:
    f.write(str(key)+", "+str(query_dict[key])+"\n")
f.close()