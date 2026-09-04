# M329 Sampled Boundary Scorer Report

## Status

M329 continued the M328 boundary-aware scorer direction. The goal was to keep
the useful boundary objective, reduce useless training after the early peak, and
avoid another scalar fusion sweep.

The result is positive enough to continue:

- M329 is the best NDCG/MAP row in the M327-M329 scorer family.
- It preserves the Recall/MRR gains from M328.
- Semantic false-positive diagnostics continue to move in the right direction.
- Sampling boundary negatives did not materially reduce per-epoch runtime,
  which means the next engineering bottleneck is the per-query Python loop and
  evaluator, not only boundary-matrix size.

## Method

New controls:

- `--boundary-negatives-per-cutoff`
  keeps only the nearest current-score boundary negatives per cutoff.
- `--early-stop-patience`
  stops training after repeated non-improving eval points.
- `--early-stop-min-delta`
  sets the minimum NDCG+MAP improvement needed to reset patience.

The M329 canary used:

- `boundary_weight=0.35`
- `boundary_cutoff=10/20/100`
- `boundary_negatives_per_cutoff=16`
- `pairwise_weight=0.75`
- `eval_every=5`
- `early_stop_patience=2`

## Run

Output:

`/home/huoju/leask/runs/ii42-m329-sampled-boundary-v1/m329_boundary035_sample16_eval5_seed1050.json`

Common surface:

- datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`
- checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- candidate cache:
  `/home/huoju/leask/runs/ii42-m326-expansion5-candidate-cache-v1`
- candidate config:
  `candidate_k=160`, `doc_post_active_k=96`, `query_post_active_k=80`,
  `max_length=256`, `max_atom_df_ratio=0.25`

## Aggregate Metrics

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M327 product dense teacher scorer | 0.7236 | 0.3654 | 0.3339 | 0.2424 |
| M328 boundary 0.35, cutoffs 10/20/100 | 0.7285 | 0.3662 | 0.3335 | 0.2418 |
| M329 sampled boundary 16 | 0.7276 | 0.3660 | 0.3347 | 0.2425 |

M329 is not a large jump, but it is the cleanest row so far for the
admission/ranking scorer: it keeps M328's Recall/MRR and improves NDCG/MAP.

## Per-Dataset Metrics

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 0.9878 | 0.2214 | 0.3381 | 0.2240 |
| `fiqa` | 0.7298 | 0.4942 | 0.4031 | 0.3366 |
| `nfcorpus` | 0.3077 | 0.5803 | 0.3502 | 0.1633 |
| `scidocs` | 0.4284 | 0.3428 | 0.1845 | 0.1249 |
| `scifact` | 0.9569 | 0.6113 | 0.6544 | 0.6031 |

The gain mostly comes from `fiqa`, `nfcorpus`, and `scifact`. `arguana` and
`scidocs` remain the weak spots for this scorer family.

## Diagnostics

| Run | BM25-supported qrel suppressed | Qrel admitted but low-ranked | Semantic false positive over-ranked | Teacher qrel top10 lost |
| --- | ---: | ---: | ---: | ---: |
| M327 product dense teacher scorer | 340 | 161 | 273 | 158 |
| M328 boundary 0.35, cutoffs 10/20/100 | 341 | 162 | 268 | 156 |
| M329 sampled boundary 16 | 343 | 166 | 266 | 154 |

M329 improves semantic false positives and teacher-qrel loss, but BM25-supported
qrel suppression worsens slightly. This is the next issue to address.

## Training Behavior

Events:

- epoch 5:
  `R@100=0.7454`, `MRR@20=0.3519`, `NDCG@10=0.3307`, `MAP@100=0.2395`
- epoch 10:
  `R@100=0.7478`, `MRR@20=0.3569`, `NDCG@10=0.3372`, `MAP@100=0.2435`
- epoch 15:
  `R@100=0.7472`, `MRR@20=0.3558`, `NDCG@10=0.3366`, `MAP@100=0.2434`
- epoch 20:
  `R@100=0.7485`, `MRR@20=0.3489`, `NDCG@10=0.3327`, `MAP@100=0.2391`
- early stop:
  `epoch=20`, `best_score=0.5807`

The useful training point is again early. More epochs add admission pressure but
hurt top-rank quality.

## Decision

Continue with boundary-aware scorer training. M329 validates the direction more
strongly than M328.

Do not broaden this exact implementation yet. Sampling reduced the loss matrix
but not the dominant runtime cost. The next step should optimize the trainer
itself.

## Next Step

M330 should:

1. Batch query examples with similar candidate counts or pre-materialize compact
   tensors for the scorer-only path.
2. Preserve M329's exact candidate cache and metric surface.
3. Add an explicit BM25-supported positive preservation feature or loss, but
   only after the trainer is fast enough to run a few controlled variants.
4. Expand from the 5-dataset surface after the batch trainer reproduces M329.

The target is not just speed. The target is enough iteration bandwidth to learn
a better boundary policy without falling back to manual fusion weights.
