# M1229 Native-Context Guarded Replay

## Question

M1228 showed that native rank-context features make safe/harm movement
observable.  M1229 tests one bounded conversion: a LODO native-only query guard
with a fixed `0.5` threshold.

This is intentionally narrow:

- no threshold sweep
- no dataset-specific tuning
- no new candidate source
- no BM25 alpha tuning

## Result

Full `shared15`, using M1228 query records:

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1225_cub_source_abs_top8_s1` | 7.970 | +0.001685 | +0.002312 | +0.002033 | +0.002252 | +0.000012 | 0.023946 |
| `m1229_native_all_safe_p05` | 5.133 | +0.000423 | +0.001232 | +0.001045 | +0.001584 | +0.000063 | 0.011132 |
| `m1229_native_rank_safe_p05` | 5.139 | +0.000453 | +0.001241 | +0.000841 | +0.001108 | +0.000055 | 0.009937 |

The guard is macro-positive, but it keeps only about half of M1225's score.

## Row Shape

| Variant | Negative rows | Main failures |
| --- | ---: | --- |
| `m1225_cub_source_abs_top8_s1` | 6 | `cqadupstack`, `fiqa`, `webis-touche2020`, `trec-covid`, `dbpedia-entity`, `nfcorpus` |
| `m1229_native_all_safe_p05` | 4 | `cqadupstack`, `nfcorpus`, `trec-covid`, `webis-touche2020` |
| `m1229_native_rank_safe_p05` | 5 | `climate-fever`, `cqadupstack`, `nfcorpus`, `trec-covid`, `webis-touche2020` |

`all_safe` is the better fixed guard.  It removes `fiqa` and
`dbpedia-entity` failures, but worsens `cqadupstack`:

- `cqadupstack` M1225 score: `-0.129310`
- `cqadupstack` M1229 all-safe score: `-0.165910`

So the guard is not a final policy.

## Interpretation

M1229 confirms the M1228 signal is usable but incomplete.

Native rank-context can remove some row harm, which means this is not a dead
end.  However, query-level gating is too coarse: it either keeps or removes all
selected atoms for a query.  That cannot handle rows where some atoms are useful
and some atoms damage rank geometry.

## Decision

Do not continue query-level gate tuning.

The next meaningful branch is atom-level native-context construction:

1. Add atom-doc overlap/fanout features:
   top100/top256 hit rate, boundary-tail hit rate, protected-head hit rate,
   BM25-only/P1-only document hit rates, and atom global fanout.
2. Run a separability-only audit against the M1224 CUB-specific target/harm
   atom labels.
3. Only if atom-level AUC is strong, train an atom selector/objective.

This keeps the new evidence chain intact:

`M1224 CUB-specific teacher -> M1228 native context visibility -> M1229 query guard partially works -> next: atom-level native context`.

## Artifacts

- Script: `scripts/replay_m1229_native_context_guard.py`
- JSON:
  `runs/m1229_native_context_guard_v1/m1229_native_context_guard.json`
- Markdown:
  `runs/m1229_native_context_guard_v1/m1229_native_context_guard.md`
