from math import log2

def dcg(ranks):
    gain = list()
    for i, rank in enumerate(ranks):
        if i == 0:
            gain.append(rank)
        else:
            gain.append(rank / log2(i + 1))
    print(f'Gain: {gain}')
    dcg = list()
    for i, g in enumerate(gain):
        if i == 0:
            dcg.append(g)
        else:
            dcg.append(g + dcg[i - 1])
    print(f'DCG: {dcg}')
    return dcg


def ndcg(ranks):
    sorted_ranks = sorted(ranks, reverse=True)
    print(f'Ideal ranks: {sorted_ranks}')
    actual_dcg = dcg(ranks)
    ideal_dcg = dcg(sorted_ranks)
    ndcg = [a / i for a, i in zip(actual_dcg, ideal_dcg)]
    print(f'NDCG: {ndcg}')
    return ndcg


def vb_code(n):
    bytes = list()
    while True:
        bytes.insert(0, bin(n % 128)[2:].zfill(8))
        if n < 128:
            break
        n = n // 128
    bytes[len(bytes) - 1] = '1' + bytes[len(bytes) - 1][1:]
    return bytes


def gamma_code(n):
    offset = bin(n)[3:]
    length = len(offset)
    gamma = '1' * length + '0' + offset
    return gamma


def avg_precision(preds):
    p = list()
    cnt = 0
    rel = 0
    rel_p = list()
    for pred in preds:
        cnt += 1
        if pred == 1:
            rel += 1
            rel_p.append(rel / cnt)
        p.append(rel / cnt)
    print(f'Precision: {p}')
    avg_p = sum(rel_p) / len(rel_p)
    print(f'Average precision: {avg_p}')


ranks = [3, 2, 3, 0, 0, 1, 2, 2, 3, 0]
# dcg(ranks)
# ndcg(ranks)

# print(vb_code(5))
# print(gamma_code(1))

preds = [1, 0, 1, 1, 1, 1, 0, 0, 0, 1]
# avg_precision(preds)