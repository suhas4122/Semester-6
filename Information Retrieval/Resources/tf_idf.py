from math import log10, sqrt

d = "car insurance auto insurance"
q = "best car insurance"

doc = d.split()
query = q.split()

vocab = set()
for d in [doc, query]:
    for w in d:
        vocab.add(w)

vocab = sorted(vocab)

print("Vocabulary:", vocab)
print()

df_cnt = [5000, 50000, 10000, 1000]
N = 1000000

df = {vocab[i]: df_cnt[i] for i in range(len(vocab))}
idf = {vocab[i]: log10(N / df_cnt[i]) for i in range(len(vocab))}

print("df", df)
print("idf", idf)
print()

q_tf = {}
q_tf_wt = {}
q_tf_idf_wt = {}
sumsq = 0
for w in vocab:
    q_tf[w] = query.count(w)
    if q_tf[w] > 0:
        q_tf_wt[w] = 1 + log10(q_tf[w])
    else:
        q_tf_wt[w] = 0
    q_tf_idf_wt[w] = q_tf_wt[w] * idf[w]
    sumsq += q_tf_idf_wt[w] ** 2
q_tf_idf_normalized = {
    w: q_tf_idf_wt[w] / sqrt(sumsq) for w in q_tf_idf_wt}

print("q_tf:", q_tf)
print("q_tf_wt:", q_tf_wt)
print("q_tf_idf_wt:", q_tf_idf_wt)
print("q_tf_idf_normalized", q_tf_idf_normalized)
print()

doc_tf = {}
doc_tf_wt = {}
doc_tf_idf_wt = {}
sumsq = 0
for w in vocab:
    doc_tf[w] = doc.count(w)
    if doc_tf[w] > 0:
        doc_tf_wt[w] = 1 + log10(doc_tf[w])
    else:
        doc_tf_wt[w] = 0
    # doc_tf_idf_wt[w] = doc_tf_wt[w] * idf[w]
    doc_tf_idf_wt[w] = doc_tf_wt[w]
    sumsq += doc_tf_idf_wt[w] ** 2
doc_tf_idf_normalized = {
    w: doc_tf_idf_wt[w] / sqrt(sumsq) for w in doc_tf_idf_wt}

print("doc_tf:", doc_tf)
print("doc_tf_wt:", doc_tf_wt)
print("doc_tf_idf_wt:", doc_tf_idf_wt)
print("doc_tf_idf_normalized", doc_tf_idf_normalized)

prod = {vocab[i]: q_tf_idf_normalized[vocab[i]] * doc_tf_idf_normalized[vocab[i]] for i in range(len(vocab))}
cos_sim = sum(prod.values())

print("prod:", prod)
print("cos_sim:", cos_sim)