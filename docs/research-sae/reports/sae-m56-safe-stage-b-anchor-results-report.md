# SAE M56 Safe Stage-B Anchor Results Report

## Summary

M56 tests the safer version of M55:

```text
lower LR
stronger teacher-shape anchor
ranking/listwise/coverage/budget losses reduced by about 4x
```

This is the first Stage-B smoke that keeps the M54 semantic shape mostly
intact while improving the two-dataset ranking sanity matrix.

## Run

```text
checkpoint = M54 e2-clean
datasets = scifact, nfcorpus
epochs = 1
learning_rate = 5e-5
output = results/sae/m56/stage-b-safe-anchor-smoke
```

Key loss changes versus M55:

| Loss | M55 | M56 |
| --- | ---: | ---: |
| support BCE | 0.55 | 0.75 |
| value MSE | 3.00 | 4.00 |
| retrieval pairwise | 0.04 | 0.01 |
| listwise teacher | 0.08 | 0.02 |
| coverage | 0.10 | 0.025 |
| soft top-k | 0.10 | 0.025 |
| candidate budget | 0.08 | 0.02 |
| asymmetric active | 0.05 | 0.01 |

## Ranking Result

Two-dataset aggregate best student rows:

| Run | Best Recall Source | Recall@100 | Best MRR Source | MRR@20 | Best NDCG Source | NDCG@10 | Best MAP Source | MAP@100 |
| --- | --- | ---: | --- | ---: | --- | ---: | --- | ---: |
| M54 e2 baseline | `bm25_student_atoms` | 0.640169 | `bm25_student_atoms_w0p5` | 0.703694 | `bm25_student_atoms_w0p25` | 0.592530 | `bm25_student_atoms_w0p25` | 0.480114 |
| M55 rank-heavy | `bm25_student_atoms_w0p5` | 0.636541 | `bm25_student_atoms_w0p25` | 0.711693 | `bm25_student_atoms_w0p25` | 0.596859 | `bm25_student_atoms_w0p25` | 0.485356 |
| M56 safe-anchor | `bm25_student_atoms` | 0.640675 | `bm25_student_atoms_w0p25` | 0.708789 | `bm25_student_atoms_w0p25` | 0.597724 | `bm25_student_atoms_w0p25` | 0.487054 |

M56 improves over M54 baseline on all four aggregate metrics and beats M55 on
Recall@100, NDCG@10, and MAP@100.

## Teacher-Shape Preservation

Same-corpus `scifact` fidelity:

| Run | Docs Recall | Queries Recall | Docs Jaccard | Queries Jaccard | Neighborhood Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| M54 e2 baseline | 0.336422 | 0.188750 | 0.127304 | 0.082421 | 0.308500 |
| M55 rank-heavy | 0.281555 | 0.153281 | 0.104400 | 0.065930 | 0.274200 |
| M56 safe-anchor | 0.326164 | 0.184375 | 0.122976 | 0.080357 | 0.308000 |

M56 keeps the neighborhood recall effectively flat while improving ranking.
This is the desired Stage-B behavior.

## Decision

Promote the M56 loss balance as the next Stage-B candidate.

Do not scale M55. M55 proved the supervised ranking signal exists, but M56 is
the first balance that keeps the semantic atom shape under control.

## Next Step

Run M56 on a wider supervised set:

```text
scifact, nfcorpus, fiqa, scidocs
```

Then evaluate:

- full15 ranking matrix;
- teacher-shape fidelity on representative datasets;
- hard-family deltas for `trec-covid`, `msmarco`, and `dbpedia-entity`;
- physical cost once the checkpoint has a plausible quality signal.
