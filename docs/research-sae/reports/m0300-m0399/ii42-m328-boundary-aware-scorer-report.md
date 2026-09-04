# M328 Boundary-Aware Scorer Report

## Status

M328 tested the next step after M327: instead of adding another scalar fusion
or rescue weight, train the scorer against the final top-k boundary.

The result is cautiously positive:

- Boundary-aware training is better than the M327 BM25-rescue loss.
- It produces the best MRR among this local scorer family.
- It reduces semantic false-positive diagnostics slightly.
- The gain is still small, so this is a direction to improve, not a promoted
  model yet.

## Method

M328 adds optional boundary loss to the existing M322 scorer. For each query,
the scorer takes its current detached top-k non-qrels as dynamic hard
negatives and trains qrel positives to beat those exact boundary candidates.

This is different from the earlier pairwise loss:

- Old pairwise loss selected hard negatives from the whole candidate pool.
- M327 rescue protected BM25-supported positives from atom-only negatives.
- M328 boundary loss directly targets the final top-k admission/ranking
  surface.

The implementation is disabled by default and only activates with
`--boundary-weight > 0`.

## Runs

Common surface:

- datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`
- checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- candidate cache:
  `/home/huoju/leask/runs/ii42-m326-expansion5-candidate-cache-v1`
- candidate config:
  `candidate_k=160`, `doc_post_active_k=96`, `query_post_active_k=80`,
  `max_length=256`, `max_atom_df_ratio=0.25`

Boundary variants:

- `boundary035_c10_20_100`:
  `/home/huoju/leask/runs/ii42-m328-boundary-scorer-v1/m328_boundary035_pair075_seed1050.json`
- `boundary025_c20_100`:
  `/home/huoju/leask/runs/ii42-m328-boundary-scorer-v1/m328_boundary025_pair075_c20_100_seed1050.json`

## Aggregate Metrics

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M327 exact dense teacher scorer | 0.7258 | 0.3616 | 0.3312 | 0.2408 |
| M327 product dense teacher scorer | 0.7236 | 0.3654 | 0.3339 | 0.2424 |
| M327 BM25 rescue 0.25 | 0.7271 | 0.3595 | 0.3300 | 0.2399 |
| M328 boundary 0.35, cutoffs 10/20/100 | 0.7285 | 0.3662 | 0.3335 | 0.2418 |
| M328 boundary 0.25, cutoffs 20/100 | 0.7285 | 0.3655 | 0.3333 | 0.2416 |

The first M328 variant is the best MRR row in this group and keeps Recall
higher than the exact/product teacher scorer rows. Product-dense teacher still
has slightly better MAP and NDCG, but M328 narrows the gap while improving
admission.

## Diagnostics

| Run | BM25-supported qrel suppressed | Qrel admitted but low-ranked | Semantic false positive over-ranked | Teacher qrel top10 lost |
| --- | ---: | ---: | ---: | ---: |
| M327 exact dense teacher scorer | 339 | 160 | 277 | 165 |
| M327 product dense teacher scorer | 340 | 161 | 273 | 158 |
| M327 BM25 rescue 0.25 | 341 | 165 | 272 | 158 |
| M328 boundary 0.35, cutoffs 10/20/100 | 341 | 162 | 268 | 156 |
| M328 boundary 0.25, cutoffs 20/100 | 341 | 163 | 269 | 157 |

Boundary loss moves the right diagnostics slightly: semantic false positives
and teacher-qrel losses are lower than M327 product/rescue. It does not yet fix
BM25-supported qrel suppression.

## Training Behavior

Both boundary variants peaked early:

- `boundary035_c10_20_100` epoch 10:
  `R@100=0.7486`, `MRR@20=0.3571`, `NDCG@10=0.3359`, `MAP@100=0.2428`
- `boundary035_c10_20_100` epoch 20:
  `R@100=0.7460`, `MRR@20=0.3506`, `NDCG@10=0.3343`, `MAP@100=0.2406`
- `boundary025_c20_100` epoch 10:
  `R@100=0.7487`, `MRR@20=0.3565`, `NDCG@10=0.3358`, `MAP@100=0.2427`
- `boundary025_c20_100` epoch 20:
  `R@100=0.7456`, `MRR@20=0.3497`, `NDCG@10=0.3324`, `MAP@100=0.2396`

This means M328 should use early stopping aggressively. More epochs are not
automatically better.

## Decision

Keep boundary-aware scoring as the next active direction. It is more promising
than product-teacher replacement and BM25-rescue pairwise loss.

Do not broaden it yet as-is. The current implementation is slow because it uses
per-query Python loops and dynamic top-k selection. Before large sweeps, it
should be optimized or converted into a sampled/batched boundary trainer.

## Next Step

M329 should focus on two changes:

1. Make boundary training faster:
   - sample a fixed number of boundary negatives per cutoff;
   - avoid full per-query top-k work where possible;
   - preserve identical metrics and diagnostics.
2. Train a slightly stronger boundary scorer:
   - keep early stopping;
   - keep exact dense as oracle label source;
   - optionally use product dense only as a reported control;
   - add explicit features for boundary position and source type if needed.

The immediate target is to improve NDCG/MAP while keeping M328's Recall/MRR
gain.
