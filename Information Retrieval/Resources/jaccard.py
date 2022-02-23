d = "car insurance auto insurance"
q = "best car insurance"

doc = set(d.split())
query = set(q.split())

intersection = doc.intersection(query)
union = doc.union(query)

jacc = len(intersection) / len(union)
print("jaccard:", jacc)