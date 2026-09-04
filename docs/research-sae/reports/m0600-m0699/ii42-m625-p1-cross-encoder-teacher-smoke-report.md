# II-42 M625 P1 Cross-Encoder Teacher Smoke Report

Date: 2026-07-06

## Objective

M625 tests whether a stronger external teacher can produce a real signal after
M623/M624 showed that weak P1/BM25/lexical consensus teachers are not enough.

This is still a smoke:

- no P1 model is trained,
- no postings are changed,
- no native DB/plugin benchmark is run,
- qrels are used only for evaluation and, in the first two runs, diagnostic
  query sampling.

## Environment

Available local packages:

- `torch`
- `transformers`
- `sentence_transformers`

`jinaai/jina-reranker-m0` was not usable in this local Python environment
because its processor path required `torchvision`.  The smoke therefore used
the text-only cross-encoder:

`cross-encoder/ms-marco-MiniLM-L6-v2`

This loaded and scored a single pair successfully on CPU.

## Implementation

- Script:
  `scripts/probe_m625_cross_encoder_teacher_admission.py`
- Test:
  `tests/test_probe_m625_cross_encoder_teacher_admission.py`

The probe preserves the current P1.3-a010 head and admits bottom-slot
candidates by cross-encoder score.

## Runs

### M625-A: informative-query sample, 12k pairs

Selection:

- max queries per dataset: `8`
- max scored pairs: `12,000`
- query selection: informative qrels-bearing diagnostic sample

Best config:

`top98_max200`

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Baseline | 0.703186 | 0.604426 | 0.768300 | 0.818742 | 1.000000 |
| M625-A | 0.703186 | 0.604568 | 0.773129 | 0.818742 | 1.000000 |

Delta:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.000000 |
| MAP@100 | +0.000142 |
| Recall@100 | +0.004829 |
| MRR@20 | +0.000000 |

### M625-B: larger informative-query sample, 30k pairs

Selection:

- max queries per dataset: `20`
- max scored pairs: `30,000`
- query selection: informative qrels-bearing diagnostic sample

Best config:

`top95_max200`

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Baseline | 0.722525 | 0.625749 | 0.786800 | 0.838156 | 1.000000 |
| M625-B | 0.722525 | 0.625761 | 0.789080 | 0.838156 | 1.000000 |

Delta:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.000000 |
| MAP@100 | +0.000012 |
| Recall@100 | +0.002280 |
| MRR@20 | +0.000000 |

The effect remained above the `+0.001` Recall gate but was concentrated:

- `climate-fever`: Recall improved.
- `cqadupstack`: Recall and MAP regressed.

### M625-C: qrels-blind query sample, 30k pairs

Selection:

- max queries per dataset: `20`
- max scored pairs: `30,000`
- query selection: first query ids, no qrels-aware informative filter

No config passed the gate.

Best observed rows were negative:

| Config | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `top99_max500` | +0.000000 | -0.000044 | -0.000846 | +0.000000 |
| `top99_max200` | +0.000000 | -0.000044 | -0.000846 | +0.000000 |
| `top98_max200` | +0.000000 | -0.000044 | -0.000846 | +0.000000 |

## Interpretation

The cross-encoder has a real signal on qrels-informative difficult queries.
That is the first teacher family since M619/M621/M624 to exceed the near-zero
Recall band.

However, the qrels-blind sample invalidates immediate promotion:

- informative-sample gains depend on selecting queries that already have
  under-ranked positives;
- qrels-blind sampling turns the same admission shape negative;
- the larger informative run shows dataset concentration and `cqadupstack`
  regression.

This means the issue is not teacher strength alone.  A cross-encoder can
identify some tail positives, but a naive bottom-slot admission proxy is not a
stable global training signal.

## Decision

Do not train directly from M625-A/B.

Do not run native DB/plugin benchmark for M625.

Keep the cross-encoder teacher route alive only as a stronger-teacher source,
not as a direct admission policy.

The next valid M626 step should be one of:

1. Build a qrels-blind teacher export over a broader, balanced query sample and
   audit score distributions before any training.
2. Use cross-encoder scores only to label hard positives/negatives for a
   first-stage compiler objective, not to directly admit top100 documents.
3. Add a teacher-confidence gate and reject rows where cross-encoder disagrees
   with dense/P1 support, then rerun a qrels-blind proxy.

Stop the route if qrels-blind M626 remains negative.

## Verification

Local verification passed for the M625 tooling:

- `python3 -m py_compile scripts/probe_m625_cross_encoder_teacher_admission.py`
- `pytest -q tests/test_probe_m625_cross_encoder_teacher_admission.py`
