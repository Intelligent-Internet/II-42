# M606 / P1 Dense-Surface Diagnostic

Status: M606-A completed on the bounded native decision surface.

M606 freezes the current P1-a0125 / M549U / M603 / M604 / M605 conclusion and
does not promote M605 as a component. This diagnostic asks a narrower
first-stage question: does the frozen P1 score/posting surface preserve dense
top100 membership and dense ranking geometry well enough before any BM25 or
qrels-aware second-stage work?

## Inputs

The diagnostic uses existing native-path artifacts only:

- `runs/m604_p1_scorer_gap_audit_v1/nfcorpus_full/nfcorpus_m604_p1_scorer_gap.jsonl`
- `runs/m604_p1_scorer_gap_audit_v1/webis_touche2020_full/webis-touche2020_m604_p1_scorer_gap.jsonl`
- `runs/m604_p1_scorer_gap_audit_v1/cqadupstack_q40_postings/cqadupstack_m604_p1_scorer_gap.jsonl`
- `runs/m603_p1_native_dense_rankings_v1/{nfcorpus,webis-touche2020,cqadupstack}/dense_rankings.jsonl`

These dense ranking JSONL files contain ordered doc ids but not raw dense
scores. Therefore this pass measures membership, rank order, and P1 score
separation. Raw dense-score calibration remains a required follow-up if the
next training round needs score-distribution loss.

## Artifacts

- Summary JSON: `runs/m606_p1_dense_surface_diagnostic_v1/m606_p1_dense_surface_diagnostic.json`
- Per-query JSONL: `runs/m606_p1_dense_surface_diagnostic_v1/m606_p1_dense_surface_queries.jsonl`
- Run-local Markdown: `runs/m606_p1_dense_surface_diagnostic_v1/m606_p1_dense_surface_diagnostic.md`
- Script: `scripts/audit_m606_p1_dense_surface.py`
- Tests: `tests/test_audit_m606_p1_dense_surface.py`

## Matrix

| Dataset | Queries | Dense in candidate | Dense in P1 cand | Dense in P1 top100 | P1 top100 precision | Rank corr | Pairwise | Mean P1 rank |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 40 | 0.935250 | 0.882000 | 0.460000 | 0.460000 | 0.367055 | 0.676568 | 185.747370 |
| `nfcorpus` | 323 | 0.920805 | 0.877090 | 0.380743 | 0.380743 | 0.315497 | 0.641780 | 238.154326 |
| `webis-touche2020` | 49 | 0.980408 | 0.965102 | 0.656531 | 0.656531 | 0.513206 | 0.738370 | 115.759651 |
| Query macro | 412 | 0.929296 | 0.888034 | 0.421238 | 0.421238 | 0.344016 | 0.656645 | 218.509624 |
| Dataset macro | 412 | 0.945488 | 0.908064 | 0.499091 | 0.499091 | 0.398586 | 0.685573 | 179.887115 |

Additional query-macro signals:

- Dense top100 missing from P1 candidates: `11.196602` docs/query.
- Dense top100 missing from candidate union: `7.070388` docs/query.
- P1 z-score mean for dense top100: `1.325547`.
- P1 z-score mean for P1 top100 but not dense top100: `1.908369`.
- Dense-vs-false-top100 z-score margin: `-0.582822`.
- Dense rank vs P1 rank Pearson: `0.344016`.

## Interpretation

The current P1-a0125 first-stage surface is not dense-equivalent enough.

The main failure is not a total candidate-generation collapse. P1 top1000
contains about `88.8%` of dense top100 on the query-weighted macro surface, and
the full audited candidate union contains about `92.9%`. That means many
dense-like documents are already present.

The larger failure is score/rank geometry:

- Only `42.1%` of dense top100 survives into P1 top100.
- The average P1 rank for dense top100 documents is about `218.5`.
- Rank correlation is weak at `0.344`.
- P1 top100 false-dense documents receive higher P1 z-scores than dense top100
  documents on average.

So M607 should not start as another downstream scorer. The first-stage P1.2
training target should directly repair the output/posting surface:

- Preserve dense top-k membership, especially the missing dense tail.
- Improve pairwise/listwise dense order among already-present dense-like docs.
- Calibrate P1 scores so P1 top100 is not dominated by non-dense top100 docs.
- Export raw dense teacher scores for a second M606-B score-calibration audit.

## M607 Recommendation

Start M607 with output-head / posting-compiler / score-surface training, not a
full encoder change and not BM25/qrels-aware scoring.

Recommended first loss family:

- Dense top100 membership BCE or sampled soft target over dense top-k docs.
- Pairwise dense-order loss on dense top100 intersections.
- Listwise softmax distillation over candidate sets once raw dense scores are
  exported.
- Active/support preservation regularizer from the existing compiler metrics.
- Conservative gate: active recall/support must not materially regress.

Do not restart M605 unless a P1.2 dense-only gate improves dense overlap/order
but native M604 still shows measurable under-ranked positives.

## Current Gaps

- Raw dense scores are unavailable in current dense ranking artifacts.
- Support cosine and active recall are not present in M604 native JSONL.
- This bounded surface has 412 common queries; it is enough for diagnosis, not
  the final official matrix.

Next concrete step: implement M607-A with a frozen dense teacher export that
includes raw scores, then rerun this diagnostic as M606-B/P1.2 before any
native benchmark regression.
