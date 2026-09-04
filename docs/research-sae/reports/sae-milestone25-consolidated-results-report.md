# SAE Milestone 2.5 Consolidated Results Report

Date: 2026-05-12

## Purpose

Milestone 2 showed that the strongest immediate deep-fusion route is not
source-level BM25+SAE fusion, but a shared evidence-atom scorer:

```text
token atoms + SAE latent atoms -> one impact namespace -> one sparse scorer
```

The follow-up pass then found that impact-head candidate generation can keep
the same quality frontier while touching far fewer postings.

This pass consolidates those directions and increases the retrieval-aware
selector training scale:

- Route 1 source-blind atom scoring remains the baseline.
- Atom-local DF calibration is tested for candidate generation and scoring.
- Impact-head candidate generation is tested at head sizes `8`, `16`, and `32`.
- Query-conditioned SAE selector training uses qrels plus up to `2000` pseudo
  queries per dataset, `12` epochs, and `16` negatives.
- A safe selector variant preserves the original high-impact SAE atoms before
  applying trained selector weights, so training can refine but not fully
  replace the stable baseline atoms.

Artifacts:

```text
scripts/research_sae_milestone25_consolidated.py
results/sae/milestone25/consolidated/milestone25_consolidated.json
results/sae/milestone25/consolidated/summary.md
```

## Configuration

Datasets:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

SAE artifact:

```text
sae_8192_64
```

Training scale:

```text
pseudo_train_limit = 2000
training_epochs = 12
training_negatives = 16
selector_preserve_sae = 32
```

The local pseudo-query artifacts are already close to this cap: `scifact`,
`scidocs`, `arguana`, and `fiqa` each have `2000` pseudo queries; `nfcorpus`
has `2063`. This run is therefore effectively using the current available
pseudo-query scale.

## Mean Matrix

| Run | Recall@100 | MRR@20 | Postings touched | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| `route1_scaled_atom` | 0.7951 | 0.6790 | 17655.8 | 1980.9 |
| `head8_route1` | 0.7856 | 0.6791 | 705.4 | 516.8 |
| `head16_route1` | 0.7964 | 0.6790 | 1381.7 | 852.9 |
| `head32_route1` | 0.7948 | 0.6790 | 2652.3 | 1268.1 |
| `head16_df_candidate` | 0.7964 | 0.6790 | 1381.7 | 852.9 |
| `head16_df_scored` | 0.7952 | 0.6724 | 1381.7 | 852.9 |
| `trained_selector_64_head16` | 0.7808 | 0.6569 | 1068.5 | 731.3 |
| `trained_selector_96_head16` | 0.7859 | 0.6555 | 1212.8 | 800.1 |
| `safe_selector_64_head16` | 0.7852 | 0.6642 | 868.7 | 613.0 |
| `safe_selector_96_head16` | 0.7933 | 0.6695 | 1121.2 | 752.7 |

## Interpretation

### Impact-Head Remains The Best Systems Route

`head16_route1` is still the best quality/cost point:

```text
Recall@100: 0.7964 vs 0.7951 route1 full
MRR@20:     0.6790 vs 0.6790 route1 full
Postings:  1381.7 vs 17655.8 route1 full
Candidates: 852.9 vs 1980.9 route1 full
```

This is the most important result. The source-blind atom scorer does not need
to scan all postings to preserve the current quality frontier. A native
implementation should therefore model impact-head or impact-ordered traversal
as a first-class execution path.

### DF Calibration Is Not The Next Mainline

DF-calibrated candidate generation produced the same mean result as route1
head16 when scoring stayed on the original rows. DF-calibrated scoring slightly
hurt MRR:

```text
head16_df_candidate: Recall@100 0.7964, MRR@20 0.6790
head16_df_scored:    Recall@100 0.7952, MRR@20 0.6724
```

Decision: keep DF/reliability features as diagnostics and possible index
metadata, but do not replace the current atom impacts with this unsupervised
calibration.

### Larger Selector Training Does Not Generalize Yet

The query-conditioned selector helps individual datasets, especially
`scifact`, but does not improve the five-dataset mean. The best trained
selector head run is below `head16_route1`:

```text
trained_selector_96_head16: Recall@100 0.7859, MRR@20 0.6555
head16_route1:              Recall@100 0.7964, MRR@20 0.6790
```

The safe selector reduces the damage by preserving stable high-impact SAE
atoms:

```text
safe_selector_96_head16: Recall@100 0.7933, MRR@20 0.6695
```

This is closer, but still not better than `head16_route1`.

Decision: do not spend the next cycle on more selector weight tuning. If we
continue the learned route, it should move from reweighting existing SAE atoms
to retrieval-aware atom training with an explicit candidate-budget objective.

### Aggressive Head8 Is A Valid Low-Cost Profile

`head8_route1` touches about half the postings of `head16_route1`:

```text
head8_route1:  Recall@100 0.7856, MRR@20 0.6791, postings 705.4
head16_route1: Recall@100 0.7964, MRR@20 0.6790, postings 1381.7
```

The recall loss is about one point, while MRR remains effectively unchanged.
This is not the safest default, but it is a useful low-latency profile for
future native benchmarking.

## Dataset Notes

| Dataset | Main observation |
| --- | --- |
| `scifact` | trained selector can win locally: `trained_selector_64_head16` reaches Recall@100 `0.9900`, MRR@20 `0.8249`. |
| `scidocs` | safe selector improves Recall@100 slightly (`0.7065`) but loses MRR versus head16 route1. |
| `nfcorpus` | selector variants remain weaker; preserving baseline atoms helps but does not exceed head16 route1. |
| `arguana` | head route1 keeps perfect Recall@100; selector drops recall and should not be trusted here. |
| `fiqa` | head16 route1 is still the best balanced profile; safe selector narrows but does not close the gap. |

## Decision

The consolidated milestone-2.5 decision is:

```text
Keep:   source-blind evidence atoms + impact-head candidate generation.
Pause:  unsupervised DF scoring as the main scorer.
Pause:  selector weight tuning over fixed SAE atoms.
Explore next:
        retrieval-aware atom training or mixed atom generation with an
        explicit candidate-budget/fanout objective.
```

For SQL/native integration, the next implementation target should not be a
Python selector. It should be a read-only evidence-atom payload that supports:

- one atom namespace for lexical and SAE impacts;
- impact-ordered or head-limited candidate generation;
- exact reranking over the generated candidate pool;
- diagnostics for touched postings, candidate docs, and source contribution.

This keeps the current breakthrough focused on the physical index design: make
semantic SAE-style recall behave like a sparse inverted-index workload.
