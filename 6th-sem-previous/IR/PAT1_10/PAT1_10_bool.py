import sys
import os
import re
import pickle
from tqdm import tqdm
# Loading saved queries and inverted index
queries_file = open(sys.argv[2], "r")
queries = [i for i in queries_file]
with open(sys.argv[1], 'rb') as f:
    inverted_index = pickle.load(f)

# Megre function for merging 2 posting list
def merge(l1,l2):
    i =0; j =0; res = []
    while i < len(l1) and j < len(l2):
        if l1[i][0]==l2[j][0]:
            res.append(l1[i])
            i= i+1
            j= j+1
        elif l1[i]<l2[j]:
            i= i+1
        else:
            j = j+1
    return res


f = open("PAT1_10_results.txt", "w")
for query in tqdm(queries):
#     print(query)
    query_id = query[:3]; query_text = query[5:len(query)-1]
    query_tokens = query_text.split(" ")
    posting_lists = []; result = []
    flag = 0
    # Accumulating posting list for each query term
    for token in query_tokens:
        if token not in inverted_index.keys():
#             print(str(token) + "Not found")
            f.write(query_id + ": "+"\n")
            flag = 1
            break
        postings = inverted_index[token].copy()
        posting_lists.append(postings)
    if flag == 1:
        continue
    # If n lists then n-1 merges
    length = len(posting_lists)-1
    for i in range(length):
        posting_lists.sort(key=len)                                 # Take the 2 smallest postings list
        intersect = merge(posting_lists[0], posting_lists[1])       # Call merge routine
        if len(posting_lists) > 2:                                  # Replace 2 lists with the merged result
            posting_lists = posting_lists[2:]
            posting_lists.append(intersect)
        else:
            result = intersect

    # Saving the result as space separated doc ids
    result_docs = [i[0] for i in result]
    finalresult = ' '.join(result_docs)
    f.write(query_id + ": " + finalresult+"\n")

    # Uncomment to see result
#     print(query_id + " : " + finalresult)

f.close()