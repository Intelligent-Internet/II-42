# ii42 M160 B12 Named Scoring Profile Report

Date: 2026-06-05

## Status

This report formalizes the first post-B12 scoring-profile step. It does not
retrain the encoder or rebuild embeddings. It reuses the completed B12
full-corpus rankings and compares a small number of named runtime-safe
profiles.

Input rankings:

```text
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-full-corpus-eval/m110_full_corpus_rankings.jsonl
```

Diagnostic artifact:

```text
/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-posthoc-fusion-diagnostic
```

Important scope note: this is the current continuity/full-corpus gate over the
M130 all-data eval corpus, not the final full BEIR15 official matrix. The
query surface contains merged BEIR-derived families such as `nfcorpus`,
`msmarco`, `fiqa`, `scifact`, `trec-covid`, and `arguana`.

## Named Profiles

The useful profiles are:

| Profile | Meaning |
| --- | --- |
| `bm25_sae_score_fusion` | B12 checkpoint fusion. Best MRR-oriented profile. |
| `residual_w0.5_bm25top100` | Runtime-safe residual fusion. Best NDCG/MAP profile. |
| `additive_w0.15_bm25top20` | Recall-oriented admission profile. Diagnostic only. |

The residual profile is:

```text
score = normalized_sae + 0.5 * normalized_bm25 * (1 - normalized_sae)
bm25 admission = top100
```

This is intentionally simple. The point of this step is to verify whether the
remaining B12 gap is a scoring-profile problem before doing more training.

## Macro Matrix

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 0.1771 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| `Dense` | 0.2408 | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| `BM25+Dense score` | 0.2405 | 0.3185 | 0.2970 | 0.2256 | 0.1499 |
| `SAE-only` | 0.2425 | 0.3274 | 0.2975 | 0.2221 | 0.1539 |
| `B12 checkpoint BM25+SAE` | 0.2448 | 0.3344 | 0.3017 | 0.2225 | 0.1526 |
| `B12 ndcg_map residual` | 0.2457 | 0.3322 | 0.2945 | 0.2278 | 0.1586 |
| `B12 recall additive` | 0.2453 | 0.3368 | 0.2901 | 0.2207 | 0.1528 |

Decision signal:

- The checkpoint profile is best for `MRR@20`.
- The residual profile is best for `NDCG@10` and `MAP@100`.
- The additive recall profile is best for `Recall@100`, but the MRR/NDCG loss
  is too large for a default profile.

## Per-Dataset Read

| Dataset | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `Dense` | 1.0000 | 0.5220 | 0.6264 | 0.5232 |
| `arguana` | `B12 checkpoint` | 1.0000 | 0.5488 | 0.6342 | 0.5498 |
| `arguana` | `B12 residual` | 1.0000 | 0.5377 | 0.6241 | 0.5386 |
| `fiqa` | `Dense` | 0.8854 | 0.6367 | 0.5915 | 0.5206 |
| `fiqa` | `B12 checkpoint` | 0.8541 | 0.7183 | 0.6329 | 0.5682 |
| `fiqa` | `B12 residual` | 0.8439 | 0.7242 | 0.6325 | 0.5652 |
| `msmarco` | `Dense` | 0.4505 | 0.3346 | 0.3607 | 0.3065 |
| `msmarco` | `B12 checkpoint` | 0.4507 | 0.3364 | 0.3568 | 0.3069 |
| `msmarco` | `B12 residual` | 0.4514 | 0.3963 | 0.4042 | 0.3674 |
| `nfcorpus` | `Dense` | 0.1538 | 0.2038 | 0.1017 | 0.0390 |
| `nfcorpus` | `B12 checkpoint` | 0.1792 | 0.2041 | 0.0996 | 0.0388 |
| `nfcorpus` | `B12 residual` | 0.1732 | 0.1896 | 0.1000 | 0.0384 |
| `scifact` | `Dense` | 0.7154 | 0.3974 | 0.4523 | 0.3958 |
| `scifact` | `B12 checkpoint` | 0.8308 | 0.3841 | 0.4328 | 0.3854 |
| `scifact` | `B12 residual` | 0.8615 | 0.3620 | 0.4261 | 0.3648 |
| `trec-covid` | `Dense` | 0.2765 | 0.7019 | 0.4090 | 0.1096 |
| `trec-covid` | `B12 checkpoint` | 0.2482 | 0.6361 | 0.3613 | 0.0897 |
| `trec-covid` | `B12 residual` | 0.2480 | 0.5489 | 0.3505 | 0.0915 |

## Interpretation

The residual profile is not a universal replacement. It works because it
substantially repairs `msmarco` top-rank scoring, where BM25 residual evidence
is strongly additive to SAE. It is weaker on `trec-covid` and `scifact`, where
extra BM25 admission hurts top-rank order.

This means the remaining problem is real, but narrow:

1. B12 representation is strong enough for the current gate.
2. Fixed residual scoring can recover the aggregate NDCG/MAP gap.
3. A single global profile cannot dominate all datasets.
4. Training another encoder is not justified by this evidence.

## Decision

Promote B12 as the milestone checkpoint and keep two official ii42 scoring
profiles for the next gate:

- `ii42_mrr`: checkpoint `bm25_sae_score_fusion`.
- `ii42_ndcg_map`: `residual_w0.5_bm25top100`.

Do not train a generic adaptive gate yet. The next useful work is a constrained
profile selector or product-level scoring profile choice, backed by per-dataset
evidence. If a single runtime default is required, the choice depends on the
product metric:

- choose `ii42_mrr` if first relevant hit / answerability is primary;
- choose `ii42_ndcg_map` if top-10 graded quality and MAP are primary.

Only start another supervised admission/ranking run if these fixed named
profiles cannot cover the desired product tradeoff.

## Selector Diagnostic

A constrained runtime-safe selector was tested after the fixed profile matrix.
It uses only query text length, BM25/SAE score distribution features, and
BM25/SAE overlap features. It does not use dense scores at runtime.

Script:

```text
scripts/research_sae_m160_profile_selector.py
```

Local artifacts:

```text
results/ii42-m160a-c6-b12-profile-selector-v1
results/ii42-m160a-c6-b12-profile-selector-ndcg-v1
results/ii42-m160a-c6-b12-profile-selector-map-v1
results/ii42-m160a-c6-b12-profile-selector-mrr-v1
```

Composite objective result:

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `checkpoint` | 0.3344 | 0.3017 | 0.2225 | 0.1526 |
| `residual` | 0.3322 | 0.2945 | 0.2278 | 0.1586 |
| `oracle_objective` | 0.3371 | 0.3167 | 0.2362 | 0.1643 |
| `selector_lodo` | 0.3350 | 0.2876 | 0.2215 | 0.1504 |

Additional objective checks:

| Objective | Selector R@100 | Selector MRR@20 | Selector NDCG@10 | Selector MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `ndcg` | 0.3347 | 0.2926 | 0.2214 | 0.1509 |
| `map` | 0.3341 | 0.2895 | 0.2228 | 0.1522 |
| `mrr` | 0.3341 | 0.2976 | 0.2228 | 0.1528 |

The oracle confirms that per-query profile choice has real headroom, but the
current runtime-safe selector cannot capture it under leave-one-dataset-out
validation. It often chooses the residual profile on datasets where residual
hurts top-rank order.

Decision: do not promote or continue the selector path now. Keep the two fixed
profiles as explicit ii42 scoring choices. Reopen selector training only if a
stronger runtime-safe feature set is available, or if product requirements
explicitly need one automatic default rather than exposing/selecting a profile.

## Balanced Profile Sweep

A focused residual sweep was run after the selector diagnostic. The goal was to
find one deterministic profile between `ii42_mrr` and `ii42_ndcg_map` that
improves NDCG/MAP without causing top-rank collapse.

Script:

```text
scripts/research_sae_m160_balanced_profile_sweep.py
```

Artifacts:

```text
results/ii42-m160a-c6-b12-balanced-profile-sweep-v1
results/ii42-m160a-c6-b12-balanced-profile-sweep-v2
```

The second run is the decision run because it adds a per-dataset MRR collapse
gate. That gate is necessary: the first run found macro-good profiles, but they
hid a large `trec-covid` MRR drop.

Sweep parameters:

```text
mode = residual
bm25_weight = 0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60
bm25_candidate_limit = 20, 50, 100
max_macro_mrr_drop = 0.010
max_macro_recall_drop = 0.006
max_dataset_mrr_drop = 0.050
max_dataset_ndcg_drop = 0.030
max_dataset_map_drop = 0.030
```

Best macro-scoring candidate:

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25_sae_score_fusion` | 0.3344 | 0.3017 | 0.2225 | 0.1526 |
| `residual_w0.55_bm25top20` | 0.3363 | 0.2950 | 0.2272 | 0.1585 |

The macro gain is real:

```text
dRecall@100 = +0.0019
dMRR@20 = -0.0067
dNDCG@10 = +0.0047
dMAP@100 = +0.0059
```

But the profile fails the collapse gate:

| Dataset | dR@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.0000 | -0.0056 | -0.0322 | -0.0057 |
| `fiqa` | +0.0071 | +0.0046 | -0.0010 | -0.0011 |
| `msmarco` | -0.0015 | +0.0601 | +0.0477 | +0.0586 |
| `nfcorpus` | +0.0028 | -0.0136 | +0.0009 | -0.0000 |
| `scifact` | +0.0000 | -0.0239 | -0.0123 | -0.0228 |
| `trec-covid` | -0.0028 | -0.0872 | -0.0111 | -0.0016 |

Decision: no single balanced residual profile should be promoted now. The
candidate profiles mainly trade `msmarco` gains against `trec-covid` top-rank
loss. This reinforces the two-profile decision:

- keep `ii42_mrr` as the safe default if one profile must be chosen;
- expose/use `ii42_ndcg_map` when NDCG/MAP is the explicit objective;
- do not restart model training unless a future product gate requires a single
  automatic profile and provides stronger runtime-safe selector features.

## Fixed Default LODO Search

A stricter fixed-default diagnostic was run after the balanced sweep. Instead
of selecting the best profile on the same evaluation surface, each fold leaves
one dataset out, selects a fixed profile on the remaining datasets, and then
evaluates that profile on the held-out dataset.

Script:

```text
scripts/research_sae_m160_fixed_default_lodo.py
```

Artifact:

```text
results/ii42-m160a-c6-b12-fixed-default-lodo-v1
```

LODO selected profile distribution:

| Profile | Folds |
| --- | ---: |
| `residual_w0.4_bm25top100` | 5 |
| `residual_w0.4_bm25top50` | 1 |

LODO macro:

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25_sae_score_fusion` | 0.5938 | 0.4713 | 0.4196 | 0.3231 |
| `lodo_selected_fixed_default` | 0.5928 | 0.4604 | 0.4250 | 0.3284 |

The selected fixed default improves LODO macro `NDCG@10` and `MAP@100`, but it
again fails held-out collapse checks. The worst fold is `trec-covid`:

```text
dRecall@100 = +0.0005
dMRR@20 = -0.0872
dNDCG@10 = -0.0085
dMAP@100 = +0.0010
```

Decision: the fixed-default route is not closed because it lacks signal; it is
closed because the signal is not robust enough. It repeatedly trades `msmarco`
top-rank gains for `trec-covid` first-hit loss. A product default should not be
chosen from this sweep.

## Doc-Level Adaptive Scorer Diagnostic

After the fixed-default LODO failure, a low-cost doc-level adaptive scorer was
tested. This is the stronger legal adaptive route: it ranks candidate documents
using runtime-visible BM25/SAE/checkpoint/residual ranks and scores, but does
not use dense scores or dataset ids at runtime.

Script:

```text
scripts/research_sae_m160_doclevel_adaptive_scorer.py
```

Artifact:

```text
results/ii42-m160a-c6-b12-doclevel-adaptive-scorer-v1
```

LODO macro:

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25_sae_score_fusion` | 0.5938 | 0.4713 | 0.4196 | 0.3231 |
| `residual_profile` | 0.5979 | 0.4606 | 0.4252 | 0.3286 |
| `adaptive_direct` | 0.5970 | 0.4406 | 0.4063 | 0.3021 |
| `adaptive_checkpoint_residual` | 0.5932 | 0.4570 | 0.4165 | 0.3197 |
| `adaptive_two_band` | 0.5932 | 0.4577 | 0.4200 | 0.3200 |

Best adaptive fold deltas are mixed and still include top-rank loss:

| Dataset | Best adaptive | dR@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `adaptive_checkpoint_residual` | +0.0000 | +0.0016 | +0.0015 | +0.0016 |
| `fiqa` | `adaptive_checkpoint_residual` | -0.0023 | +0.0172 | +0.0082 | +0.0037 |
| `msmarco` | `adaptive_two_band` | +0.0001 | -0.0150 | -0.0065 | -0.0128 |
| `nfcorpus` | `adaptive_direct` | -0.0046 | -0.0097 | +0.0022 | +0.0006 |
| `scifact` | `adaptive_two_band` | +0.0000 | -0.0064 | +0.0049 | -0.0055 |
| `trec-covid` | `adaptive_two_band` | +0.0015 | -0.0519 | -0.0017 | +0.0016 |

Decision: stop the adaptive scorer path for now. The doc-level scorer is better
than the earlier query-level selector idea, but it still does not beat the two
fixed profiles under LODO. Continuing to tune this scorer would likely become
benchmark-specific optimization without solving the general profile problem.

Current default recommendation:

- Use `ii42_mrr = bm25_sae_score_fusion` as the safest default because it is the
  checkpoint-emitted profile and preserves MRR/first-hit behavior.
- Keep `ii42_ndcg_map = residual_w0.5_bm25top100` as an explicit scoring mode
  when the caller wants NDCG/MAP-style ranking.
- Do not claim a single universal balanced/adaptive default from the current
  evidence.

## SAE-Protected Residual Sweep

One more fixed-formula route was tested after the doc-level scorer failed. The
idea was to keep the useful BM25 residual correction, but protect
high-confidence SAE documents from large BM25 perturbations. This should have
helped if the balance problem was only caused by BM25 overpowering good SAE
hits.

Script:

```text
scripts/research_sae_m160_protected_residual_sweep.py
```

Artifact:

```text
results/ii42-m160a-c6-b12-protected-residual-sweep-v1
```

Best macro-scoring candidate:

```text
protected_w0.55_top20_t0.85_floor0.2_p1_both0
```

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25_sae_score_fusion` | 0.3344 | 0.3017 | 0.2225 | 0.1526 |
| `protected_w0.55_top20_t0.85_floor0.2_p1_both0` | 0.3363 | 0.2947 | 0.2258 | 0.1582 |

The macro tradeoff is again attractive but not robust:

```text
dRecall@100 = +0.0019
dMRR@20 = -0.0071
dNDCG@10 = +0.0034
dMAP@100 = +0.0056
```

Worst per-dataset drops:

| Dataset | dR@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `trec-covid` | -0.0028 | -0.0872 | -0.0148 | -0.0018 |
| `scifact` | +0.0000 | -0.0274 | -0.0202 | -0.0276 |
| `arguana` | +0.0000 | -0.0016 | -0.0149 | -0.0016 |

Decision: close the fixed-formula geometry route for now. SAE protection,
BM25 power compression, and intersection boosts do not remove the same
`trec-covid`/`scifact` top-rank failure mode. The remaining balance problem is
not a low-cost scoring-formula issue; it needs either an explicit two-profile
product choice or a training-time objective that learns BM25 residual
admission without damaging SAE first-hit stability.
