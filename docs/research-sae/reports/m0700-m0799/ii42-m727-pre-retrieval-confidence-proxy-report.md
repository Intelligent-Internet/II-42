# M727 Pre-Retrieval Confidence Proxy

## Question

M726 found a near-pass oracle gate: selecting only dense-safe O@100 movements
almost removed the M721b rank harm.  But that oracle uses post-retrieval
dense-overlap deltas, so it is not deployable.

M727 tests whether the same safe subset can be predicted from features
available before native retrieval.

The proxy target is dense-equivalence safety, not qrels.  Qrels-derived
metrics are used only for post-hoc reporting from prior native replay outputs.

## Artifacts

- Script: `scripts/audit_m727_pre_retrieval_confidence_proxy.py`
- Output JSON: `runs/m727_pre_retrieval_confidence_proxy_v1/m727_summary.json`
- Output report: `runs/m727_pre_retrieval_confidence_proxy_v1/m727_report.md`

## Feature Set

The proxy uses pre-retrieval movement and selector features:

- selected atom count
- selected target/negative proxy shares from dense-boundary training records
- pair-HGB score statistics
- pair interaction feature aggregates
- query movement norm
- support cosine / support recall before retrieval
- top-32 atom replacement counts

It does not use BM25, reranker scores, qrels, or post-retrieval dense-overlap
metrics as runtime features.

## Training Result

| Split | Rows | Positive share | AUC |
| --- | ---: | ---: | ---: |
| Train | 839 | 0.202622 | 0.969454 |
| Eval | 496 | 0.143145 | 0.569876 |

The train/eval gap is large.  The model can fit the oracle label on train rows,
but the same feature shape barely transfers to held-out rows.

## Replay Result

| Gate | Accepted | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | dO@100 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_o100_dense_all_nonneg` | 0.180 | +0.000068 | +0.000001 | +0.000009 | +0.000000 | +0.001880 | -0.000004 |
| `proxy_all` | 0.168 | +0.000068 | +0.000004 | -0.000003 | +0.000000 | +0.001150 | -0.000001 |
| `proxy_eval_only` | 0.047 | +0.000000 | -0.000004 | +0.000000 | +0.000000 | +0.000113 | +0.000000 |

`proxy_all` is not acceptable because Recall@100 is negative.  `proxy_eval_only`
is too weak and regresses MAP.  Neither clears the strict first-stage promotion
gate.

## Interpretation

M727 does not find a deployable pre-retrieval proxy for the M726 oracle gate.

This is an important negative result:

- M726 showed that a dense-safe subset exists in post-hoc replay.
- M727 shows the current pair/movement features do not predict that subset
  well enough on held-out rows.
- Therefore the pair-interaction movement route remains diagnostic only.

The likely failure mode is not more training depth.  The proxy overfits train
strongly and fails to generalize, which points to insufficiently causal
features for rank-safe movement.

## Decision

Do not continue scaling M721b/M725/M727 as a first-stage optimization route.

Keep the artifacts as evidence:

- movement can improve dense overlap@100
- global movement is unsafe
- oracle dense-safe movement is near-pass
- current pre-retrieval features cannot reproduce the oracle safely

The next first-stage work should return to a direct dense-topK / rank-margin
preserving compiler objective, where the loss itself binds topK membership and
rank geometry instead of relying on a separate post-hoc movement gate.
