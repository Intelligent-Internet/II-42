# II-42 M604 P1.2 Native Scorer Gap Audit

Date: 2026-07-06

## Objective

This audit checks whether the P1.2 active512 native candidate pool still loses
mainly because relevant documents are missing, or because they are present but
under-ranked.  It uses the native PostgreSQL/plugin candidate path, not the
offline dense scan evaluator.

Frozen baseline:

- First-stage surface: P1.2 active512 root-identity signed-coordinate atoms
- Fusion row: P1.2-a0125
- BM25 weight: 0.125
- Semantic weight: 0.875
- Dataset surface: local shared15
- Schema: `ii42_shared15`
- P1.2 atom schema: `ii42_p1p2`

## Artifacts

- Full audit root:
  `runs/m608_p1p2_m604_scorer_gap_shared15_v1/`
- Native benchmark matrix:
  `runs/m608_p1p2_native_shared15_v1/m603_p1_native_p1p2_active512_shared15_matrix.json`
- Atom root:
  `runs/m608_p1p2_root_identity_atoms_shared15_v1/`

Each dataset has:

- `<dataset>_m604_p1_scorer_gap.json`
- `<dataset>_m604_p1_scorer_gap.md`
- `<dataset>_m604_p1_scorer_gap.jsonl`

The JSONL exports are large because they keep per-candidate native features for
later scorer training.

## Native Baseline

P1.2-a0125 is materially better than the previous P1-a0125 native shared15
baseline.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.2-a0125 | 0.94206 | 0.79425 | 0.77869 | 0.69805 | 0.85385 | 0.87068 |
| P1-a0125 | 0.93731 | 0.41668 | 0.74494 | 0.66274 | 0.84349 | 0.83975 |
| M549U | 0.94084 | 0.40435 | 0.73078 | 0.64769 | 0.83595 | 0.82700 |
| BM25 | 0.90617 |  | 0.67006 | 0.59398 | 0.78372 | 0.78811 |
| dense | 0.86900 |  | 0.72854 | 0.64042 | 0.77817 | 0.83541 |

Delta P1.2-a0125 minus P1-a0125:

| Metric | Delta |
| --- | ---: |
| CUB | +0.00475 |
| O@100 | +0.37758 |
| NDCG@10 | +0.03375 |
| MAP@100 | +0.03531 |
| Recall@100 | +0.01037 |
| MRR@20 | +0.03094 |

## Gap Categories

Weighted by qrels positives:

| Category | Rate |
| --- | ---: |
| top100 hit | 0.34188 |
| candidate present but under-ranked | 0.48724 |
| candidate miss | 0.17088 |

Macro over datasets:

| Category | Rate |
| --- | ---: |
| top100 hit | 0.80592 |
| candidate present but under-ranked | 0.16369 |
| candidate miss | 0.03039 |

## Dataset Breakdown

| Dataset | Queries | Positives | Top100 hit | Under-ranked | Candidate miss | BM25-only pos | Semantic-only pos |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 100 | 100 | 1.0000 | 0.0000 | 0.0000 | 0 | 0 |
| climate-fever | 100 | 292 | 0.9589 | 0.0411 | 0.0000 | 1 | 4 |
| cqadupstack | 100 | 787 | 0.6950 | 0.3050 | 0.0000 | 0 | 172 |
| dbpedia-entity | 100 | 3458 | 0.7869 | 0.2099 | 0.0032 | 5 | 591 |
| fever | 100 | 109 | 1.0000 | 0.0000 | 0.0000 | 0 | 0 |
| fiqa | 100 | 267 | 0.9326 | 0.0674 | 0.0000 | 1 | 16 |
| hotpotqa | 100 | 200 | 1.0000 | 0.0000 | 0.0000 | 0 | 0 |
| msmarco | 43 | 4102 | 0.6424 | 0.3498 | 0.0078 | 10 | 358 |
| nfcorpus | 100 | 3818 | 0.2423 | 0.5998 | 0.1579 | 545 | 955 |
| nq | 100 | 121 | 1.0000 | 0.0000 | 0.0000 | 0 | 1 |
| quora | 100 | 275 | 1.0000 | 0.0000 | 0.0000 | 0 | 3 |
| scidocs | 100 | 492 | 0.6870 | 0.2744 | 0.0386 | 12 | 68 |
| scifact | 100 | 116 | 0.9914 | 0.0086 | 0.0000 | 0 | 0 |
| trec-covid | 50 | 24673 | 0.1642 | 0.5875 | 0.2483 | 2120 | 5638 |
| webis-touche2020 | 49 | 932 | 0.9882 | 0.0118 | 0.0000 | 0 | 17 |

## Interpretation

The P1.2 first-stage surface should be kept as the current frozen baseline.
It improved both dense overlap and native ranking metrics over P1-a0125.

The remaining broad shared15 gap is not primarily a candidate-generation
collapse.  Weighted by positives, under-ranked positives are 0.48724 versus
candidate misses at 0.17088.  The heaviest contributors are `trec-covid`,
`nfcorpus`, `msmarco`, and `dbpedia-entity`, where many relevant documents are
available in the native candidate pool but not promoted into top100.

The macro view is less severe because many datasets are already near saturated.
That means a global scorer must be checked carefully: it should recover the
heavy under-ranked rows without damaging saturated rows such as `arguana`,
`fever`, `hotpotqa`, `nq`, and `quora`.

## Decision

M604 gives enough evidence to restart a bounded second-stage scorer experiment,
but only against the P1.2-a0125 native candidate pool.

Recommended next step:

1. Freeze P1.2 active512 and P1.2-a0125 as the first-stage baseline.
2. Start M605.2 as a global scorer recovery probe, not as dataset tuning.
3. Train from the exported M604 JSONL features.
4. Require native DB/plugin re-evaluation before any promotion.
5. Reject M605.2 if gains concentrate in one dataset or regress NDCG@10/MRR@20.

Do not return to residual first-stage training until M605.2 either succeeds or
fails under this frozen P1.2 candidate pool.
