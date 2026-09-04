# M1543 Fixed-Budget Semantic/Lexical Complement Audit

## Decision

**Stop fixed-budget route/BM25 substitution and do not train a scorer.**

The 10% dual semantic route plus BM25 fill-to-15% union improves equal-budget
candidate capacity only on SciFact. It loses capacity relative to dense top15%
on NFCorpus and FiQA, so the predeclared candidate gate is `1/3` and unsafe.

Commit under test: `6b35434f`.

ClearML task: `bb3e1d80fda949e79a4a3a643b7b6616`.

## Equal-Budget Capacity

| Dataset | Dense top15% CUB | BM25 top15% CUB | Route10 + lexical fill CUB | Delta vs best |
| --- | ---: | ---: | ---: | ---: |
| nfcorpus | 0.500551 | 0.334128 | 0.485207 | -0.015344 |
| scifact | 0.980000 | 0.976667 | 0.990000 | +0.010000 |
| fiqa | 0.978056 | 0.847841 | 0.951667 | -0.026389 |
| macro | 0.819536 | 0.719545 | 0.808958 | -0.010578 |

The union is not a reliable expansion. A fixed lexical reservation removes
more semantic candidates than it rescues on two of three rows. This is the
same action-source allocation problem identified in the M1244/M1245 line,
now reproduced on the new corpus-route source under an honest equal budget.

## Ranking Result

| Macro source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB | Union |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense top15% | 0.667473 | 0.562316 | 0.767131 | 0.757520 | 0.819536 | 0.1501 |
| BM25 top15% | 0.549246 | 0.454358 | 0.652973 | 0.643514 | 0.719545 | 0.1254 |
| route10 tail256 | 0.623656 | 0.525337 | 0.692782 | 0.703574 | 0.721927 | 0.1001 |
| union tail256 | 0.667060 | 0.562037 | 0.757089 | 0.756304 | 0.808958 | 0.1428 |
| union P1 RRF a0125 | 0.663520 | 0.558250 | 0.758920 | 0.754647 | 0.808958 | 0.1428 |
| union exact-dense upper | 0.669365 | 0.563184 | 0.756434 | 0.759224 | 0.808958 | 0.1428 |

The frozen union is efficient and close to dense, but it is not a recall
breakthrough. Even the exact-dense rerank upper cannot repair missing
candidates. A learned scorer would optimize inside the wrong candidate set.

## What Is Retained

M1543 still leaves two useful engineering facts:

1. M1542 dual routes plus tail256 can support a roughly 14% union with nearly
   dense macro quality;
2. the tail scorer is again not the bottleneck: union tail and exact-dense
   upper are close.

These facts support route atoms as a compression/efficiency option, but not as
the route to higher recall.

## Next Structural Probe

Do not test more budget splits, alphas, gates, selectors, or epochs. The
remaining untested mechanism is query-aware block pruning inside the existing
signed coordinate postings:

- group each signed-coordinate posting list into corpus-geometric blocks;
- store one frozen block summary/centroid;
- score blocks by full query-to-summary similarity;
- read only the highest-scoring blocks under an exact posting-read cap;
- accumulate exact sparse evidence for admitted documents, then reuse tail256;
- measure summary scans, posting reads, unique union, and dense overlap
  separately.

This is a Seismic-style access-structure test, not another source/gate model.
It directly addresses M1541's failure: true neighbors require accumulated
moderate evidence, while individual-edge truncation cannot see block-level
semantic coherence. If block pruning also fails at the predeclared budget,
the honest conclusion is to keep dense geometry in a proper ANN index rather
than train another unified-posting approximation.

## Artifacts

- `runs/m1543a_fixed_budget_complement_v1/summary.json`
- `runs/m1543a_fixed_budget_complement_v1/summary.md`

