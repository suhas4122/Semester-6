import sys
import os
import re
import pickle
from collections import Counter
from tqdm import tqdm
from math import log,sqrt
from statistics import mean
import csv

queries_file = open(sys.argv[3], "r")
queries = [i for i in queries_file]
with open(sys.argv[2], 'rb') as f:                                          # opening save inverted index
    inverted_index = pickle.load(f)

V = inverted_index.keys()                                                   # vocabulary
document_term_count_dict = {}                                               # helper dict to get tf(t,d) for documents
all_docs = set()                                                            # to save all doc ids for ranking
for word in V:
    posting_list = inverted_index[word]
    for doc_id, cnt in posting_list:
        all_docs.add(doc_id)
        if doc_id in document_term_count_dict.keys():
            if word in document_term_count_dict[doc_id].keys():
                document_term_count_dict[doc_id][word]+=cnt
            else:
                document_term_count_dict[doc_id][word]=cnt
        else:
            document_term_count_dict[doc_id]={}
            document_term_count_dict[doc_id][word]=cnt

all_docs = list(all_docs)
N = len(all_docs)
print("Number of unique doc ids", N)
def get_whole_idf(config):                                                  # function to get idf(t) with a given scheme
    df = {word:len(inverted_index[word]) for word in V}
    if(config=='n'):
        idf = {word:1 for word in df.keys()}
    if(config=='t'):
        idf = {word:log(N/df[word],10) for word in df.keys()}
    if(config=='p'):
        idf = {word:max(0,log((N-df[word])/df[word],10)) for word in df.keys()}
    return idf

def get_whole_tf(config, raw_tf):                                               # function to get tf with a given scheme
    tf_list = [v for k,v in raw_tf.items()]
    if(config=='l'):
        tf = {word:1+log(i,10) if i>0 else 0 for word,i in raw_tf.items()}
    if(config=='L'):
        m = sum(tf_list); ave=0
        if m>0:
            ave = 1 + log(m/len(V),10)
        tf = {word:(1+log(i,10))/(ave) if (i>0 and ave!=0) else 0 for word,i in raw_tf.items()}
    if(config=='a'):
        mx = max(tf_list)
        tf = {word:0.5 + ((0.5*i)/(mx)) if mx!=0 else 0.5 for word,i in raw_tf.items()}
    return tf

def form_vector(raw_tf, idf, config):                                           # form vectors with the given scheme
    tf = get_whole_tf(config[0], raw_tf)
    vector = {word: tf[word]*idf[word] for word in raw_tf.keys()}
    normal = sqrt(sum([v*v for k,v in vector.items()]))                         # cosine normalisation
    normalized_vector = {k:v/(normal) if normal!=0 else 0 for k,v in vector.items()}
    return normalized_vector

files = []
for directory in os.listdir(sys.argv[1]):
    pathofdir = os.path.join(sys.argv[1],directory)
    listoffiles = os.listdir(pathofdir)
    pathoffiles = [os.path.join(pathofdir,i) for i in listoffiles]
    files+=pathoffiles

def rank(config_d, config_q):                                                   # function to rank
    results = []
    idf_q = get_whole_idf(config_q[1])                                          # precompute idf as they will be fixed
    print("IDF formed for queries")
    idf_d = get_whole_idf(config_d[1])
    print("IDF formed for documents")
    for query in tqdm(queries):
        query_id = query[:3]; query_text = query[5:]
        query_text = query_text[:-1]
        raw_tf_q = Counter(query_text.split(" "))
        # take TFs of terms common to vocab and given query as all others will be 0 and won't contribute
        raw_tf_q = {k:v for k,v in raw_tf_q.items() if k in V}
        query_V = raw_tf_q.keys()     
        query_vector = form_vector(raw_tf_q, idf_q, config_q)                   # query vector
        scores = []
        for doc_id in list(all_docs):
            # check for only terms present in doc as all others will be 0 and won't contribute
            doc_V = document_term_count_dict[doc_id].keys()                                         # terms in doc
            raw_tf_d = {word: document_term_count_dict[doc_id][word] for word in doc_V} 
            document_vector = form_vector(raw_tf_d, idf_d, config_d)                        # document vector
            intersect = list(set(query_V) & set(doc_V))                                     # terms in both query and doc
            # only taking terms both in query and doc into account
            score = sum([query_vector[i]*document_vector[i] for i in intersect])              # find score
            scores.append((score,doc_id))
            # print(query_id, doc_id, score)
        # print(query_id, "Done!")
        scores.sort(reverse=True)                                                       # get top 50
        result_q = [[query_id,scores[i][1]] for i in range(50)]
        results+=result_q
    return results


def tocsv(results, fname):                                                              # save results in csv file
    with open(fname,'w',newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Query_ID","Document_ID"])
        writer.writerows(results)


r1 = rank("lnc","ltc")                                                                  # all 3 mentioned config in PS
tocsv(r1, "PAT2_10_ranked_list_A.csv")
print("A done")
r2 = rank("Lnc","Lpc")
tocsv(r2, "PAT2_10_ranked_list_B.csv")
print("B done")
r3 = rank("anc","apc")
tocsv(r3, "PAT2_10_ranked_list_C.csv")
print("C done")