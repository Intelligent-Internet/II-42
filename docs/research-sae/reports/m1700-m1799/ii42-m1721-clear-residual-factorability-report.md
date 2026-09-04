# M1721 CLEAR-Style Residual Factorability Report

Date: 2026-07-12

Decision: **stop the fixed rank-residual teacher before model training**.

ClearML task: `715157a774de49f0ace1d32fdaa094fc`.

## Question

M1720A showed that lexical complement is not an orthogonal low-rank subspace.
M1721 therefore moved residualization to the score surface. It constructed a
qrels-free teacher from frozen BM25 and BGE rankings:

```text
teacher(q, d) = relu(dense_discount(q, d) - bm25_discount(q, d))
target(q, d) = bm25_discount(q, d) + teacher(q, d)
             = max(bm25_discount(q, d), dense_discount(q, d))
```

The audit separated three questions:

1. can a free rank-32 field express the teacher;
2. can frozen document BGE vectors express it;
3. can one global bilinear transform trained on two corpora transfer to a
   held-out third corpus.

No qrel, dataset threshold, rank grid, or retrieval metric selected a factor.

## Capacity And Observability

| Dataset | Rank-32 energy | Rank at 90% | Rank at 95% | Free O@100 | Free recovery | Doc O@100 | Doc recovery |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | 0.780153 | 60 | 77 | 0.671100 | 0.599929 | 0.771700 | 0.640713 |
| SciFact | 0.706548 | 66 | 78 | 0.702400 | 0.227478 | 0.718900 | 0.143502 |
| FiQA | 0.711048 | 69 | 83 | 0.699400 | 0.235923 | 0.732500 | 0.170598 |

The teacher is not compact at rank 32. More importantly, a direct document
ridge projection has score cosine `0.82–0.91` but does not preserve the
candidate movement. Lower score error is again not equivalent to boundary
membership.

## Leave-One-Dataset-Out Result

| Dataset | Variant | Score cosine | Target O@100 | Dense-BM25 recovery | NDCG@10 | MAP@100 | Recall@100 | CUB | Max DF |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | full bilinear | 0.369692 | 0.418500 | 0.372677 | 0.387577 | 0.190895 | 0.295400 | 0.648056 | 1.000000 |
| NFCorpus | rank 32 | 0.120644 | 0.419600 | 0.301968 | 0.374879 | 0.180126 | 0.272813 | 0.630916 | 1.000000 |
| NFCorpus | TopK 4 | 0.073418 | 0.433500 | 0.290484 | 0.376249 | 0.181166 | 0.273837 | 0.569190 | 0.322346 |
| SciFact | full bilinear | 0.450647 | 0.544600 | 0.140253 | 0.811627 | 0.777451 | 0.946667 | 0.990000 | 1.000000 |
| SciFact | rank 32 | -0.050989 | 0.624000 | 0.022200 | 0.806745 | 0.773918 | 0.939667 | 0.990000 | 1.000000 |
| SciFact | TopK 4 | -0.011033 | 0.634700 | 0.021197 | 0.809283 | 0.774003 | 0.961667 | 1.000000 | 0.276500 |
| FiQA | full bilinear | 0.183719 | 0.483700 | 0.135094 | 0.477929 | 0.418014 | 0.781794 | 0.958139 | 1.000000 |
| FiQA | rank 32 | -0.057553 | 0.537200 | 0.047173 | 0.467400 | 0.407165 | 0.713913 | 0.932448 | 1.000000 |
| FiQA | TopK 4 | -0.117137 | 0.607800 | 0.029683 | 0.466447 | 0.407966 | 0.713758 | 0.924948 | 0.596500 |

All three gates pass `0/3`. The unrestricted linear transform is already too
weak on held-out corpora, so rank 64 or longer optimization cannot repair the
causal failure. Sparse TopK factors also produce 27.7% to 59.7% maximum DF,
far outside the product gate.

## Interpretation

The result does not invalidate CLEAR. CLEAR trains a dense branch to improve
lexical errors while retaining a separate dense index. M1721 asks the harder
question of whether the same correction is a compact, globally transferable
posting dot product. The answer on this locked surface is no.

This also differs from M1501/M1502. M1501's qrel-derived field had a compact
free factor but no text transcodability. M1721's corpus-computable teacher
fails earlier: its rank-32 capacity is inadequate, and even its full linear
map does not transfer.

## Stop Decision

- do not sweep rank, ridge, rank discount, clipping, or candidate depth;
- do not train an MLP/LoRA against this fixed residual matrix;
- do not convert the factors to a posting namespace;
- do not interpret high SciFact CUB as a pass; its dense-minus-BM25 recovery
  is only `0.0222` for the rank-32 LODO model.

The qrels-free complementarity signal remains valuable as a **sampling and
loss weight**, but M1721 only rejects the tested hard rank-discount matrix as a
compact fixed linear target. It does not test whether that signal can reshape
a high-dimensional balanced source during retrieval-native training.

Accordingly, M1721 does not authorize an MLP/LoRA over this matrix, a rank
sweep, or conversion of these factors. It does permit the independently
controlled M1722 experiment, where residual supervision is compared with the
original dense teacher on the same frozen M1600 source.

## Reproducibility

- Formal result:
  `/home/huoju/leask/runs/ii42-m1721-clear-residual-v1/runs/m1721a-clear-residual-factorability-shared3-v2/summary.json`
- Summary SHA-256:
  `468fcd04fdb7f5e9a562ea912ef9dec50dabf985dbf50e34fd299f1e80af5788`
- ClearML task: `715157a774de49f0ace1d32fdaa094fc`.
- The `v1` launch stopped before evaluation because of a keyword-interface
  mismatch. The fix added direct call-path coverage; only `v2` is interpreted.
- Focused regression suite: 27 tests passed with M1520, M1541, M1542, and
  M1710 compatibility coverage.
