# SAE M55 Stage-B Ranking Anchor Results Report

## Summary

M55 tested whether a small supervised Stage-B pass can improve ranking when
starting from the balanced M54 checkpoint.

Result:

```text
ranking signal = positive on the two-dataset smoke
teacher-shape preservation = too weak
decision = do not scale this exact loss balance
```

The run confirms that supervised ranking pressure can move NDCG/MAP in the
right direction, but it also damages query-side teacher atom fidelity too much
for a Stage-B promotion.

## Run

```text
checkpoint = M54 e2-clean
datasets = scifact, nfcorpus
epochs = 1
learning_rate = 1e-4
device = mps
output = results/sae/m55/stage-b-anchor-smoke
```

The loss combined:

- teacher-shape anchor: support/value/support-margin/fanout/exposure;
- supervised ranking: qrels retrieval pairs;
- teacher neighborhood: listwise teacher and multi-positive coverage;
- runtime-shape terms: soft top-k, candidate budget, asymmetric active loss.

## Ranking Result

Aggregate best student rows:

| Run | Best Recall Source | Recall@100 | Best MRR Source | MRR@20 | Best NDCG Source | NDCG@10 | Best MAP Source | MAP@100 |
| --- | --- | ---: | --- | ---: | --- | ---: | --- | ---: |
| M55 baseline e2 | `bm25_student_atoms` | 0.640169 | `bm25_student_atoms_w0p5` | 0.703694 | `bm25_student_atoms_w0p25` | 0.592530 | `bm25_student_atoms_w0p25` | 0.480114 |
| M55 stage-b | `bm25_student_atoms_w0p5` | 0.636541 | `bm25_student_atoms_w0p25` | 0.711693 | `bm25_student_atoms_w0p25` | 0.596859 | `bm25_student_atoms_w0p25` | 0.485356 |

At `w0p25`, both smoke datasets improve NDCG/MAP:

| Dataset | Baseline NDCG | Stage-B NDCG | Delta | Baseline MAP | Stage-B MAP | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| scifact | 0.804776 | 0.810347 | +0.005571 | 0.769583 | 0.778534 | +0.008951 |
| nfcorpus | 0.380283 | 0.383372 | +0.003089 | 0.190644 | 0.192178 | +0.001534 |

This is useful evidence: the Stage-B supervision is not directionally wrong.

## Teacher-Shape Regression

Same-corpus `scifact` fidelity:

| Run | Docs Recall | Queries Recall | Docs Jaccard | Queries Jaccard | Neighborhood Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| M54 e2 baseline | 0.336422 | 0.188750 | 0.127304 | 0.082421 | 0.308500 |
| M55 stage-b | 0.281555 | 0.153281 | 0.104400 | 0.065930 | 0.274200 |

The query-side drop is too large. The model gained local ranking quality by
moving away from the teacher sparse shape, which is exactly the failure mode
M55 was supposed to avoid.

## Decision

Do not scale this exact M55 recipe.

The useful signal is:

```text
small supervised ranking pressure can improve NDCG/MAP
```

The blocker is:

```text
the current Stage-B objective updates the shared encoder too aggressively and
damages query/doc atom fidelity
```

## Next Step

M56 should keep the positive ranking signal but make the update safer:

- lower ranking pressure by at least 2-4x;
- strengthen query-support/value preservation specifically;
- consider query-side-only calibration if the current shared encoder cannot
  preserve document and query atom shape under ranking pressure;
- keep the same smoke gate, but require teacher-shape regression to stay small
  before scaling beyond `scifact/nfcorpus`.
