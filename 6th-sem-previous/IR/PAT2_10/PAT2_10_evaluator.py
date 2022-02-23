import sys
import os
import re
import pickle
from collections import Counter
from math import log,sqrt
from statistics import mean
import csv
import xlrd


goldsheet = xlrd.open_workbook(sys.argv[1]).sheet_by_index(0)  #Opening the gold standard scores
file = open(sys.argv[2]) #Opening the ranked documents for queries
model_rank = csv.reader(file)
next(model_rank)


relevance_scores_perquery = {} #To store the gold-standard relevance scores of the documents for each query
gold_dcg_10 = {} #dcg@10 for ideal retreival using gold-standard
gold_dcg_20 = {} #dcg@20 for ideal retreival using gold-standard
queryIDs = set()
for i in range(1,goldsheet.nrows): #Finding gold-standard relevance scores of the documents for each query
    queryIDs.add(goldsheet.cell_value(i,0))
    if goldsheet.cell_value(i,0) in relevance_scores_perquery.keys() :
        relevance_scores_perquery[goldsheet.cell_value(i,0)][goldsheet.cell_value(i,1)] = goldsheet.cell_value(i,2)    
    else:
        relevance_scores_perquery[goldsheet.cell_value(i,0)] = {}
        relevance_scores_perquery[goldsheet.cell_value(i,0)][goldsheet.cell_value(i,1)] = goldsheet.cell_value(i,2)    


for q in relevance_scores_perquery.keys(): #Finding dcg@10 and dcg@20 for ideal retreival using gold-standard
    sorted_rank_list = list(relevance_scores_perquery[q].values())
    sorted_rank_list.sort(reverse = True)
    dcg10 = 0.0
    dcg20 = 0.0
    for i in range(0,len(sorted_rank_list)):
        if(i<10):
            if(i!=0):
                dcg10 = dcg10 + sorted_rank_list[i]/log(i+1,2)
            else:
                dcg10 = sorted_rank_list[i]
        if(i<20):
            if(i!=0):
                dcg20 = dcg20 + sorted_rank_list[i]/log(i+1,2)
            else:
                dcg20 = sorted_rank_list[i]
    gold_dcg_10[q] = dcg10
    gold_dcg_20[q] = dcg20

avgP_10 = {}
avgP_20 = {}
ndcg_10 = {}
ndcg_20 = {}

queries = set()
ranked_docs_per_queries = {}
for row in model_rank: #Storing the ranked documents for our model for each query 
    queries.add(int(row[0]))
    q = int(row[0])
    if q in ranked_docs_per_queries.keys():
        ranked_docs_per_queries[q].append(row[1])
    else:
        ranked_docs_per_queries[q] = []
        ranked_docs_per_queries[q].append(row[1])

map10 = 0.0
map20 = 0.0
andcg10 = 0.0
andcg20 = 0.0

for q in ranked_docs_per_queries.keys(): #Finding the Avgp@10, AvgP@20, ndcg@10 and ndcg@20 for each query 
    avgP_10[q] = 0.0
    avgP_20[q] = 0.0
    ndcg_10[q] = 0.0
    ndcg_20[q] = 0.0
    if q in relevance_scores_perquery.keys():
        rank = 1
        rel_count = 0
        for doc in ranked_docs_per_queries[q]:
            if doc in relevance_scores_perquery[q].keys():
                rel_count = rel_count + 1
                if (rank<=10): 
                    avgP_10[q] = avgP_10[q] + rel_count/rank #Finding Avg@10
                    if(rank!=1): 
                        ndcg_10[q] = ndcg_10[q] + relevance_scores_perquery[q][doc]/log(rank,2) #Finding ndcg@10
                    else:
                        ndcg_10[q] = ndcg_10[q] + relevance_scores_perquery[q][doc]
                if (rank<=20):  
                    avgP_20[q] = avgP_20[q] + rel_count/rank #Finding Avg@20
                    if(rank!=1):
                        ndcg_20[q] = ndcg_20[q] + relevance_scores_perquery[q][doc]/log(rank,2) #Finding ndcg@20
                    else:
                        ndcg_20[q] = ndcg_20[q] + relevance_scores_perquery[q][doc]
            if (rank==10 and rel_count!=0):
                avgP_10[q] = avgP_10[q]/rel_count
            if (rank==20 and rel_count!=0):
                avgP_20[q] = avgP_20[q]/rel_count
            rank = rank + 1
        ndcg_10[q] = ndcg_10[q]/gold_dcg_10[q]
        ndcg_20[q] = ndcg_20[q]/gold_dcg_20[q]
    map10 = map10 + avgP_10[q] #Average metrices for all queries
    map20 = map20 + avgP_20[q]
    andcg10 = andcg10 + ndcg_10[q]
    andcg20 = andcg20 + ndcg_20[q]

numQ = len(queries)
map10 = map10/numQ
map20 = map20/numQ
andcg10 = andcg10/numQ
andcg20 = andcg20/numQ

f = open("./PAT2_10_metrics_"+sys.argv[2][len(sys.argv[2])-5]+".txt", "w") #Printing the output in repective files
f.write("Query_ID, "+"AvgPrec@10, "+"AvgPrec@20, "+"NDCG@10, "+"NDCG@20, "+"\n")
for q in ranked_docs_per_queries:
    f.write(str(q)+", "+str(round(avgP_10[q],4))+", "+str(round(avgP_20[q],4))+", "+str(round(ndcg_10[q],4))+", "+str(round(ndcg_20[q],4))+"\n")
f.write("mAP@10 = "+str(round(map10,4))+"\n")
f.write("mAP@20 = "+str(round(map20,4))+"\n")
f.write("averNDCG@10 = "+str(round(andcg10,4))+"\n")
f.write("averNDCG@20 = "+str(round(andcg20,4))+"\n")
f.close()
file.close()
