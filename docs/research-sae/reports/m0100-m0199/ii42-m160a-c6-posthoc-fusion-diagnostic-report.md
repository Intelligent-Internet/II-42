# ii42 M160A C6 Post-Hoc Fusion Diagnostic Report

Date: 2026-06-04

## Summary

This diagnostic uses the completed `ii42-m160a-stageb-direct-c6-official-dense-miss-v1`
checkpoint and re-scores its existing final full-corpus rankings. It does not
retrain the model or rebuild embeddings.

The result is directional but clear:

- SAE-only is already stronger than dense on Recall@100 and MAP@100.
- The learned BM25+SAE score fusion improves MRR over SAE-only, but hurts MAP and
  NDCG because the BM25 contribution is still too blunt.
- Post-hoc BM25 admission/fusion can improve Recall@20, Recall@100, NDCG@10, and
  MAP@100 over SAE-only and learned BM25+SAE.
- No fixed profile beats dense on MRR@20. Dense remains the strongest top-rank
  MRR baseline on this mixed evaluation surface.
- The simple C7 runtime-safe gate does not beat the best fixed post-hoc profile.
  Do not train a query gate yet; first settle the fixed fusion/admission policy.

## Inputs

```text
rankings:
/home/huoju/leask/runs/ii42-m160a-stageb-direct-c6-official-dense-miss-v1-full-corpus-eval/m110_full_corpus_rankings.jsonl

post-hoc sweep output:
/home/huoju/leask/runs/ii42-m160a-stageb-direct-c6-official-dense-miss-v1-posthoc-fusion-diagnostic

C7 gate output:
/home/huoju/leask/runs/ii42-m160a-stageb-direct-c6-official-dense-miss-v1-c7-fusion-policy-diagnostic
```

The final eval surface contains `886` queries from a mixed M-series / BEIR query
surface. It is not the complete official BEIR15 per-dataset matrix.

## Baseline

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.1771 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.2408 | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.2393 | 0.3152 | 0.2815 | 0.2086 | 0.1375 |
| SAE-only | 0.2406 | 0.3279 | 0.2922 | 0.2217 | 0.1541 |
| Learned BM25+SAE score fusion | 0.2428 | 0.3263 | 0.2966 | 0.2162 | 0.1439 |
| BM25+SAE RRF | 0.2302 | 0.3272 | 0.2763 | 0.1983 | 0.1315 |

## Best Post-Hoc Sweep Points

The sweep covered BM25 weights
`0, 0.01, 0.025, 0.05, 0.075, 0.1, 0.15, 0.2, 0.3, 0.5, 0.75, 1.0`, BM25
candidate caps `0, 3, 5, 10, 20, 50, 100`, and fusion modes
`additive`, `residual`, and `excess`.

| Metric | Best source | Value |
| --- | --- | ---: |
| Recall@20 | `residual_w0.75_bm25top50` | 0.2489 |
| Recall@100 | `excess_w0.3_bm25top100` | 0.3370 |
| MRR@20 | `dense` | 0.2992 |
| NDCG@10 | `residual_w0.5_bm25top100` | 0.2267 |
| MAP@100 | `residual_w0.5_bm25top100` | 0.1581 |

The best balanced fixed profile is currently:

```text
residual_w0.5_bm25top100
```

It improves over SAE-only on Recall@20, Recall@100, NDCG@10, and MAP@100:

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| SAE-only | 0.2406 | 0.3279 | 0.2922 | 0.2217 | 0.1541 |
| `residual_w0.5_bm25top100` | 0.2479 | 0.3339 | 0.2916 | 0.2267 | 0.1581 |
| `excess_w0.3_bm25top100` | 0.2481 | 0.3370 | 0.2897 | 0.2242 | 0.1576 |
| `residual_w0.75_bm25top50` | 0.2489 | 0.3367 | 0.2917 | 0.2248 | 0.1571 |

## Dataset Signals

| Dataset | Main signal |
| --- | --- |
| `arguana` | SAE is already saturated. BM25 is mostly a ranking-only risk. |
| `fiqa` | SAE is much better than learned BM25+SAE. Small `excess` is safest. |
| `msmarco` | Residual BM25 admission helps strongly across ranking metrics. |
| `nfcorpus` | BM25 admission improves recall but can weaken top-rank ordering. |
| `scifact` | Learned BM25+SAE remains useful; BM25 helps recall heavily. |
| `trec-covid` | Dense remains much stronger. BM25 admission helps recall slightly but does not fix ranking. |

The dataset pattern matches the earlier M150 C6 lesson: BM25 is useful as
selective admission/boost, but a global learned BM25 scale is too blunt.

## Admission Versus Ordering

The key remaining question is whether the model is missing relevant documents
before ranking, or whether the candidates are present but ordered poorly. A
simple candidate-pool diagnostic answers that directly:

| Surface | Recall@20 | Recall@100 | Candidate-pool Recall | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.1771 | 0.2439 | 0.2439 | 0.2211 | 0.1542 | 0.0970 |
| Dense | 0.2408 | 0.3132 | 0.3132 | 0.2992 | 0.2251 | 0.1508 |
| SAE-only | 0.2406 | 0.3279 | 0.3279 | 0.2922 | 0.2217 | 0.1545 |
| Learned BM25+SAE score fusion | 0.2428 | 0.3263 | 0.3263 | 0.2966 | 0.2162 | 0.1444 |
| `SAE top100 UNION BM25 top100` | 0.2406 | 0.3279 | 0.3588 | 0.2922 | 0.2217 | 0.1545 |
| `SAE top100 UNION dense top100` | 0.2406 | 0.3279 | 0.3513 | 0.2922 | 0.2217 | 0.1545 |

`Candidate-pool Recall` measures whether a relevant document appears anywhere
in the combined pool before final ordering. It is not a deployable ranked
metric; it is an upper-bound diagnostic for admission.

This shows that the `SAE top100 UNION BM25 top100` pool contains substantially
more relevant documents than SAE-only: `0.3588` versus `0.3279` Recall@100.
The current best post-hoc ranked policy reaches only `0.3370` Recall@100, so
there is still roughly three points of admission/reranking headroom that the
current fusion functions do not capture.

Per-dataset, the remaining headroom is uneven:

| Dataset | SAE Recall@100 | `SAE UNION BM25` pool recall | Main interpretation |
| --- | ---: | ---: | --- |
| `arguana` | 1.0000 | 1.0000 | No admission headroom; do not let BM25 disturb SAE ranking. |
| `fiqa` | 0.8590 | 0.8718 | Small admission headroom; BM25 can help, but only with a weak boost. |
| `msmarco` | 0.4399 | 0.4545 | Moderate admission headroom and ranking gains from residual BM25. |
| `nfcorpus` | 0.1787 | 0.2026 | BM25 adds recall, but top-rank ordering remains fragile. |
| `scifact` | 0.7692 | 0.8615 | Large BM25 admission headroom; selective admission is valuable. |
| `trec-covid` | 0.2368 | 0.3507 | Large pool headroom, but ranking remains the hard part. |

The practical conclusion is that the next step should not be another generic
Stage-C fine-tune. The next step should target admission/ranking directly:
keep SAE as the base ranker, admit a bounded BM25 tail, and train or calibrate
only the decision that decides which BM25 candidates deserve to enter and how
far they may move upward.

## C7 Runtime Gate

The existing C7 runtime-safe gate was rerun after fixing dataset parsing for
mixed query ids. It selected between SAE and fixed boost profiles using simple
runtime features such as top overlap, BM25 score gap, and query length.

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| SAE-only | 0.2406 | 0.3279 | 0.2922 | 0.2217 | 0.1541 |
| `rank_boost` | 0.2420 | 0.3364 | 0.2885 | 0.2221 | 0.1560 |
| `recall_boost` | 0.2443 | 0.3368 | 0.2889 | 0.2228 | 0.1550 |
| `recall20_boost` | 0.2457 | 0.3339 | 0.2926 | 0.2244 | 0.1560 |
| `cv_runtime_gate` | 0.2435 | 0.3349 | 0.2865 | 0.2213 | 0.1535 |

The runtime gate underperforms the best fixed post-hoc profiles. This suggests
the current gate features are not enough to select per-query policy robustly.

Oracle profile selection still has upside:

| Oracle | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Oracle Recall@100 | 0.2409 | 0.3408 | 0.2926 | 0.2221 | 0.1550 |
| Oracle MAP@100 | 0.2494 | 0.3405 | 0.3186 | 0.2369 | 0.1686 |

The gap between the simple gate and oracle means adaptive policy may still be
valuable, but it needs better query features or supervision. It should not be
the immediate next step.

## Decision

Do not launch another Stage-C training run yet.

The next actionable step is to make the evaluator and runtime path support a
small set of explicit fixed BM25 admission/fusion policies:

1. SAE-only baseline.
2. `residual_w0.5_bm25top100` as the current balanced default candidate.
3. `excess_w0.3_bm25top100` as the recall-first candidate.
4. `residual_w0.75_bm25top50` as the Recall@20 candidate.

Then run the same policies on a broader official BEIR15 per-dataset matrix. If
the fixed policy remains stable, implement it as the Stage-C deployment scoring
candidate. If it collapses on additional datasets, design a second-generation
adaptive gate using stronger query-level features rather than the current C7
heuristics.

If the fixed policy still leaves the same admission gap, the next training run
should be a narrow fusion/admission run built from full-corpus candidate pools:
positive rows should focus on BM25-only relevant hits and dense/SAE misses,
while negative rows should focus on high-BM25 false positives. The objective
should optimize final top-100 admission and top-20 ordering, not raw SAE atom
imitation.
