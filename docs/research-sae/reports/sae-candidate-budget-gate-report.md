# SAE Incremental Candidate-Budget Gate Report

Date: 2026-05-11

## Objective

The previous query-selection experiment used a static normalized-DF cost:

```text
query selection score = activation - lambda * normalized_log_df
```

That reduces broad latents, but it is too blunt. It cannot tell whether a
latent opens many new candidate documents for the current query, or whether
those documents were already opened by earlier selected latents.

This phase tests an incremental query-time candidate-budget gate:

```text
query selection score =
    activation
    - static_selection_cost
    - lambda * normalized_incremental_candidate_openings
```

The incremental term is recomputed greedily while selecting query latents. For
each candidate latent it estimates:

```text
documents opened by this latent that are not already covered
```

This is still an offline Python prototype. It does not change the native index
or train a new model.

## Prototype

New script:

```text
scripts/research_sae_candidate_budget_gate_eval.py
```

The script:

- loads an existing SAE run;
- keeps document latents fixed;
- builds a document-postings matrix by latent dimension;
- re-encodes only query latents with the incremental gate;
- evaluates direct SAE ranking and unified BM25+SAE ranking;
- sweeps candidate-budget weights.

The default `base-cost=auto` preserves the existing selection cost, so
`weight=0` reproduces the current exported query latents. This detail matters:
the existing top-k path allows zero-activation latents to consume slots when
the selection cost makes positive high-fanout latents too expensive.

## Baseline Document SAE: Small Weight Sweep

This is the strongest signal from this phase. It uses the fixed document SAE
baseline from the generalization matrix and applies only query-time incremental
candidate budgeting.

Five-dataset aggregate:

| Weight | Direct R@100 | Direct MRR@20 | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `0.000` | `0.9260` | `0.7737` | `0.9291` | `0.7642` | `1313.1` | `3146.6` |
| `0.005` | `0.9260` | `0.7713` | `0.9296` | `0.7663` | `1279.6` | `3037.0` |
| `0.010` | `0.9242` | `0.7709` | `0.9308` | `0.7669` | `1249.2` | `2930.6` |
| `0.020` | `0.9233` | `0.7717` | `0.9303` | `0.7681` | `1177.4` | `2697.8` |
| `0.050` | `0.9238` | `0.7590` | `0.9270` | `0.7703` | `990.7` | `2139.4` |

Interpretation:

```text
0.005 is the safest no-regression cost reduction.
0.010 is the best balanced point.
0.020 is the stronger cost point with small direct-recall loss.
0.050 is too aggressive for a default despite lower postings.
```

`0.010` improves unified Recall@100 from `0.9291` to `0.9308`, improves
unified MRR@20 from `0.7642` to `0.7669`, and reduces SAE postings from
`3146.6` to `2930.6`.

`0.020` reduces postings further to `2697.8` and has the highest aggregate
unified MRR@20, but direct R@100 drops more than at `0.010`.

## Dataset Details

Baseline document SAE by dataset:

| Dataset | Weight | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `0.000` | `1.0000` | `0.5480` | `815.1` | `1532.5` |
| `arguana` | `0.010` | `1.0000` | `0.5496` | `745.4` | `1385.6` |
| `arguana` | `0.020` | `1.0000` | `0.5514` | `683.8` | `1257.0` |
| `fiqa` | `0.000` | `0.9750` | `0.7628` | `1677.2` | `4601.4` |
| `fiqa` | `0.010` | `0.9800` | `0.7643` | `1619.0` | `4278.5` |
| `fiqa` | `0.020` | `0.9800` | `0.7604` | `1563.6` | `3991.6` |
| `nfcorpus` | `0.000` | `0.7526` | `0.8569` | `1974.5` | `5668.2` |
| `nfcorpus` | `0.010` | `0.7520` | `0.8570` | `1960.3` | `5490.9` |
| `nfcorpus` | `0.020` | `0.7556` | `0.8618` | `1937.6` | `5255.5` |
| `scidocs` | `0.000` | `0.9180` | `0.7797` | `1023.1` | `1958.6` |
| `scidocs` | `0.010` | `0.9220` | `0.7860` | `938.6` | `1746.2` |
| `scidocs` | `0.020` | `0.9160` | `0.7941` | `837.8` | `1482.9` |
| `scifact` | `0.000` | `1.0000` | `0.8737` | `1075.5` | `1972.2` |
| `scifact` | `0.010` | `1.0000` | `0.8776` | `982.5` | `1751.8` |
| `scifact` | `0.020` | `1.0000` | `0.8728` | `864.4` | `1501.7` |

The important result is NFCorpus: unlike the older high-weight static qsel
path, small incremental gating does not collapse recall on this dataset.

## Asymmetric Query Encoder Check

I also tested the same small-weight gate on the `q32_pseudo_frac10` and
`q64_pseudo_frac10` asymmetric query-encoder variants.

Five-dataset aggregate:

| Variant | Weight | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: |
| `q32_pseudo_frac10` | `0.000` | `0.8971` | `0.7148` | `1110.9` | `2317.0` |
| `q32_pseudo_frac10` | `0.010` | `0.8947` | `0.7159` | `1057.3` | `2177.7` |
| `q32_pseudo_frac10` | `0.020` | `0.8947` | `0.7116` | `1008.9` | `2049.1` |
| `q64_pseudo_frac10` | `0.000` | `0.8991` | `0.7208` | `1176.0` | `2603.9` |
| `q64_pseudo_frac10` | `0.010` | `0.8978` | `0.7128` | `1112.0` | `2418.7` |
| `q64_pseudo_frac10` | `0.020` | `0.9015` | `0.7174` | `1059.9` | `2271.1` |

The asymmetric query encoders still trail the baseline document SAE by a large
quality margin. They reduce cost, but not enough to justify the quality loss.

## Conclusion

The best next direction is:

```text
baseline document SAE
+ existing document selection cost
+ small query-time incremental candidate-budget gate
```

Recommended next default for experiments:

```text
candidate_budget_weight = 0.010
```

`0.020` is a useful cost-favoring profile. It should not be the default until
we test larger query sets and a native WAND/MaxScore-style simulation.

The next implementation step should not be more static DF tuning. It should be
training-aware integration of this gate:

```text
use incremental candidate-budget selection for query-teacher training
then export queries with the same gate
```

The risk is runtime cost. The current greedy exact prototype is fine for
offline evaluation, but a database index would need an impact-ordered or
block-bound approximation rather than scanning a dense postings mask per query.

## Follow-Up: Training and Physical Checks

The follow-up phase is documented in
`sae-candidate-budget-training-and-physical-report.md`.

It tested the planned training-aware integration and an impact-ordered physical
simulator across the same five benchmark corpora. The result changes the
implementation priority:

```text
post-training candidate-budget gate remains the current default.
trained query encoder can lower postings, but loses ranking quality.
impact-ordered traversal still touches too many postings.
```

Aggregate comparison:

| Path | Unified R@100 | Unified MRR@20 | SAE postings |
| --- | ---: | ---: | ---: |
| post-training gate `0.010` | `0.9308` | `0.7669` | `2930.6` |
| trained query encoder | `0.9255` | `0.7559` | `2481.6` |

So the next round should not promote the trained query encoder yet. It should
add retention-aware distillation and a stronger block/impact-max physical
pruning model.
