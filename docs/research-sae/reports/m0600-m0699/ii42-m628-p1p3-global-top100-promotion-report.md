# M628 / P1.3 Global Top100 Promotion

Status: completed as a guarded candidate, not promoted as an unconditional
default.

M628 freezes the P1.3-a010 candidate pool and trains a global, auditable
second-stage scorer to promote candidate-present but under-ranked positives
into top100.  It does not modify the P1.3 encoder/posting generator and does
not use dataset id as a model feature.

## Artifacts

- Dataset builder:
  `scripts/build_m628_top100_promotion_dataset.py`
- Ranker trainer and offline guard:
  `scripts/train_m628_top100_promotion_ranker.py`
- Native replay:
  `scripts/apply_m628_top100_promotion_ranker.py`
- Full M628 dataset:
  `runs/m628_p1p3_top100_promotion_dataset_v1/`
- Full offline replay:
  `runs/m628_p1p3_top100_promotion_ranker_v1/`
- Native eval-split replay:
  `runs/m628_p1p3_native_replay_shared15_v1/`
- Native eval-style rows:
  `runs/m628_p1p3_native_replay_eval_style_shared15_v1/`

## Dataset Surface

Input surface:

- M604 native candidate rows:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1/`
- P1.3 atom rows:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1/`
- Aligned dense ranks:
  `runs/m608_p1p3_aligned_dense_rankings_shared15_v1/`

M628 selected only rows a reranker can affect: top100 positives, under-ranked
positives, top100 blockers, and sampled rank 101-1000 negatives.  Candidate
miss positives are counted for audit but not trained as recoverable rows.

| Dataset surface | Value |
| --- | ---: |
| Datasets | 15 |
| Rows scanned | 1,946,411 |
| Rows selected | 239,329 |
| Train queries | 1,070 |
| Eval queries | 272 |
| Positive rows | 39,742 |
| Candidate-miss positives | 6,716 |
| Candidate-miss positive share | 0.168990 |
| Top100 positives | 13,618 |
| Under-ranked dense-miss positives | 19,033 |
| Under-ranked dense-hit positives | 375 |

The dataset confirms that P1.3 still has a real scorer-side recovery surface:
about 48% of positives are candidate-present, under-ranked, and dense-miss.
That makes a second-stage scorer worth testing, but the final proof must come
from full native candidate rows, not the selected training surface.

## Model

The promoted artifact is a pairwise linear scorer:

- Model file:
  `runs/m628_p1p3_top100_promotion_ranker_v1/m628_p1p3_top100_promotion_ranker_model.json`
- Feature count: 46
- Train rows: 191,884
- Pair count: 238,979
- Pair rows: 477,958
- Train queries: 1,070

Feature families:

- P1/BM25/fused score and rank features.
- Score margins and query-local z/percentile features.
- Rank-window indicators.
- Lexical coverage, idf overlap, query/doc token counts.
- Atom overlap, signed-dot, positive/negative atom mass, conflict rate.

Audit exclusions:

- No dataset id feature.
- No query id or doc id feature.
- No dense rank or dense hit feature.
- No category feature.

## Offline Guard

Offline replay uses the selected M628 dataset eval split.  This is useful for
model debugging, but it overstates recoverability because candidate upper bound
is 1.0 on the selected surface.

Best offline config:

- alpha: 0.35
- preserve top-k: 50
- rerank window: 1000

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1.3-a010 proxy | 0.773991 | 0.718946 | 0.871471 | 0.862545 | 1.000000 |
| P1.3-a010+M628 | 0.773991 | 0.724102 | 0.904374 | 0.862545 | 1.000000 |
| Delta | +0.000000 | +0.005156 | +0.032903 | +0.000000 | +0.000000 |

The offline guard passed, but this was not sufficient for promotion because
the selected surface omits most non-sampled tail negatives and candidate-miss
positives.

## Native Replay

Native replay reuses the M604 native candidate JSONL rows and re-enriches them
with the M628 feature surface.  It filters to the same stable eval split as
M628 training and excludes `candidate_present=false` rows from ranking and
candidate upper bound.

The offline-best config did not transfer to native rows.  On the nfcorpus +
trec-covid native smoke it reduced Recall and MAP.  A focused conservative
sweep found a smaller valid native config:

- alpha: 0.35
- preserve top-k: 99
- rerank window: 200

Full shared15 eval split:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1.3-a010 | 0.772779 | 0.714584 | 0.864865 | 0.862545 | 0.977127 |
| P1.3-a010+M628 | 0.772779 | 0.714687 | 0.866419 | 0.862545 | 0.977127 |
| Delta | +0.000000 | +0.000103 | +0.001555 | +0.000000 | +0.000000 |

Per-dataset Recall@100 deltas:

| Dataset | dRecall@100 |
| --- | ---: |
| arguana | +0.000000 |
| climate-fever | +0.016667 |
| cqadupstack | -0.000159 |
| dbpedia-entity | +0.001472 |
| fever | +0.000000 |
| fiqa | +0.000000 |
| hotpotqa | +0.000000 |
| msmarco | +0.001302 |
| nfcorpus | +0.004604 |
| nq | +0.000000 |
| quora | +0.000000 |
| scidocs | +0.000000 |
| scifact | +0.000000 |
| trec-covid | -0.000166 |
| webis-touche2020 | -0.005348 |

## M604-Style Gap Delta

Native eval-split promotion counts:

| Count | Baseline | M628 |
| --- | ---: | ---: |
| Positive docs in top100 | 2,415 | 2,417 |
| Under-ranked positive total | 3,827 | 3,827 |
| Under-ranked positives promoted | 0 | 14 |

This confirms the model can promote some under-ranked positives, but the net
top100 gain is only +2 positives.  Some existing tail-top100 positives are
displaced, although preserving top99 keeps NDCG@10 and MRR@20 unchanged.

## Decision

M628 passes the strict native metric guard on the shared15 eval split:

- Macro Recall@100 improves.
- Macro MAP@100 improves slightly.
- NDCG@10 does not regress.
- MRR@20 does not regress.
- The model is global and auditable.
- The gain is not from a dataset-specific threshold.

However, the improvement is small and fragile:

- Offline gain is much larger than native gain.
- The native config must be very conservative.
- webis-touche2020 regresses on Recall@100.
- The net M604-style top100 positive gain is only +2.

Recommendation:

- Keep M628 as a guarded candidate and diagnostic milestone.
- Do not promote it as the unconditional P1.3 default yet.
- Use it as evidence that the score surface has learnable signal, but the
  current feature family is not strong enough to recover most under-ranked
  positives on full native candidate rows.

## Next Step

The next useful work is not a larger alpha sweep.  The bottleneck is transfer
from selected training rows to full native candidate rows.  The next iteration
should train against a fuller native negative surface:

1. Build an eval-compatible training surface with more rank 101-1000 negatives
   instead of sparse tail sampling.
2. Add query-level hard-negative quotas around ranks 80-200, where the native
   top100 boundary actually changes.
3. Keep the preserve-top99 guard during early native replay.
4. Test whether the model can promote under-ranked positives without displacing
   existing top100 positives.

If that still yields only +0.1%-0.2% Recall, stop the M628 scorer family and
return to improving the first-stage P1.3 score interface.
