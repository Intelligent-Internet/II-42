# ii42 M320 Dense-Assisted Posting-Native Report

## Summary

M320 tests a direct posting-native atom encoder:

```text
text -> encoder -> atom postings -> BM25 + atom unified retrieval
```

Dense/PPLX is kept as an auxiliary teacher, but the main objective is retrieval
through atom postings. The first canary reused the M1050 `all-test` data root
only as a fixed comparison surface. It did not rerun M1040/M1050 training.

## Runs

Remote host:

- `spark-2`

Runs:

- `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1`
- `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050`
- `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-w010-gap005`
- `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005`
- `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005-clipped`
- `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-dfaware`

ClearML:

- `ii42-m320 / m320-nfcorpus-scifact-canary-v1`
- `ii42-m320 / m320-nfcorpus-scifact-seed1050-admission-prior005-clipped`
- `ii42-m320 / m320-nfcorpus-scifact-seed1050-dfaware`

Datasets:

- `nfcorpus`
- `scifact`

The seed-1050 runs are the valid apples-to-apples comparison with M1050,
because they use the same heldout split.

## Training Signal

### M320 w0.35

Configuration:

- `dense_teacher_weight=0.35`
- `dense_teacher_min_gap=0.02`

Final train history:

- `rank_loss=0.4094`
- `dense_teacher_loss=0.3375`
- `coverage_loss=0.1583`
- `fanout_loss=6.9928`

This proves the objective is trainable, but dense retention is strong enough to
hurt top-rank quality on `scifact`.

### M320 w0.10 / gap0.05

Configuration:

- `dense_teacher_weight=0.10`
- `dense_teacher_min_gap=0.05`

Final train history:

- `rank_loss=0.5170`
- `dense_teacher_loss=0.4385`
- `coverage_loss=0.1903`
- `fanout_loss=17.1751`

This is more conservative and keeps more atom coverage. It improves atom-only
Recall@100 and some unified metrics, but still loses ranking quality on
`scifact`.

### M320 admission / BM25-prior

Configuration:

- `dense_teacher_mode=admission`
- `dense_teacher_weight=0.10`
- `dense_teacher_min_gap=0.05`
- `bm25_hard_negative_prior=0.05`

Final train history:

- `rank_loss=0.7296`
- `dense_teacher_loss=0.1827`
- `coverage_loss=0.2738`
- `fanout_loss=40.8249`

This version made the core signal much clearer. Atom-BM25 quality improved
strongly, especially on `scifact`, but the learned atoms became physically
unusable: high-DF atoms touched almost every document.

### M320 train/eval clipped

Configuration:

- same as M320 admission / BM25-prior;
- training now applies the same post-active clipping as export/evaluation.

Final train history:

- `rank_loss=0.5879`
- `dense_teacher_loss=0.1531`
- `coverage_loss=0.2584`
- `fanout_loss=10.4661`

The clipping fix reduced fanout and improved the training curve, but it also
removed useful semantic atoms on `scifact`. This shows that active clipping is
necessary for consistency, but it is not a substitute for learning atom utility.

### M320 df-aware

Configuration:

- same as M320 clipped;
- `fanout_penalty=0.10`
- `query_fanout_penalty=0.02`
- `max_batch_df_ratio=0.25`
- `df_overflow_weight=0.25`
- `background_docs_per_dataset=512`
- `background_per_batch=32`

Final train history:

- `rank_loss=0.5066`
- `dense_teacher_loss=0.1100`
- `coverage_loss=0.1846`
- `fanout_loss=7.1084`
- `df_overflow_loss=3.6121`

This version successfully reduced head-atom pressure and physical cost, but it
lost too much ranking quality. The result is useful diagnostically: the high-DF
atoms are not pure noise. Some are carrying the semantic admission signal.

## Apples-To-Apples Heldout Matrix

### NFCorpus

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1050 atom-BM25 | 0.0705 | 0.0989 | 0.0633 | 0.0277 |
| M320 w0.35 atom-BM25 | 0.1313 | 0.0970 | 0.0537 | 0.0250 |
| M320 w0.10 atom-BM25 | 0.1371 | 0.1187 | 0.0677 | 0.0337 |
| M320 admission atom-BM25 | 0.1598 | 0.1809 | 0.0960 | 0.0452 |
| M320 clipped atom-BM25 | 0.1733 | 0.1312 | 0.0757 | 0.0358 |
| M320 df-aware atom-BM25 | 0.1395 | 0.1174 | 0.0583 | 0.0296 |
| M1050 unified scale 0.5 | 0.2611 | 0.4573 | 0.2872 | 0.1301 |
| M320 w0.35 unified scale 0.5 | 0.2587 | 0.4385 | 0.2850 | 0.1341 |
| M320 w0.10 unified scale 0.5 | 0.2751 | 0.4615 | 0.2911 | 0.1376 |
| M320 admission unified scale 0.5 | 0.2698 | 0.4705 | 0.3010 | 0.1403 |
| M320 clipped unified scale 0.5 | 0.2805 | 0.4628 | 0.2960 | 0.1397 |
| M320 df-aware unified scale 0.5 | 0.2757 | 0.4647 | 0.2972 | 0.1367 |

NFCorpus verdict:

- M320 admission is strongest for atom-only MRR/NDCG/MAP, but it touches the
  whole corpus.
- M320 clipped has the best atom-only Recall@100 and the best unified
  Recall@100, but top-rank metrics are below M320 admission.
- M320 df-aware lowers cost but gives back too much quality.

### SciFact

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1050 atom-BM25 | 0.4689 | 0.2723 | 0.2837 | 0.2748 |
| M320 w0.35 atom-BM25 | 0.4815 | 0.1675 | 0.1853 | 0.1699 |
| M320 w0.10 atom-BM25 | 0.5122 | 0.1914 | 0.2083 | 0.1956 |
| M320 admission atom-BM25 | 0.6496 | 0.2922 | 0.3177 | 0.2899 |
| M320 clipped atom-BM25 | 0.5167 | 0.1872 | 0.2091 | 0.1908 |
| M320 df-aware atom-BM25 | 0.5833 | 0.1595 | 0.1817 | 0.1666 |
| M1050 unified scale 0.5 | 0.8552 | 0.6011 | 0.6240 | 0.5938 |
| M320 w0.35 unified scale 0.5 | 0.8352 | 0.5688 | 0.5985 | 0.5564 |
| M320 w0.10 unified scale 0.5 | 0.8574 | 0.5523 | 0.5937 | 0.5425 |
| M320 admission unified scale 0.5 | 0.8463 | 0.5621 | 0.5993 | 0.5516 |
| M320 clipped unified scale 0.5 | 0.8574 | 0.5573 | 0.5902 | 0.5493 |
| M320 df-aware unified scale 0.5 | 0.8463 | 0.5474 | 0.5831 | 0.5398 |

SciFact verdict:

- M320 admission substantially improves atom-only Recall@100 and also beats
  M1050 atom-only ranking metrics.
- That quality comes from very high-DF atoms: the atom index effectively touches
  the whole corpus.
- Clipping and df-aware variants reduce cost, but neither preserves the M320
  admission ranking quality.

## Physical Cost

Heldout atom-BM25 touched-docs / postings:

| Dataset | Model | Touched Docs | Postings |
| --- | --- | ---: | ---: |
| `nfcorpus` | M1050 | 160.9 | 164.5 |
| `nfcorpus` | M320 admission | 3633.0 | 39532.5 |
| `nfcorpus` | M320 clipped | 3171.4 | 11323.6 |
| `nfcorpus` | M320 df-aware | 2299.2 | 5017.4 |
| `scifact` | M1050 | 246.8 | 270.3 |
| `scifact` | M320 admission | 5183.0 | 87242.1 |
| `scifact` | M320 clipped | 5012.9 | 32977.6 |
| `scifact` | M320 df-aware | 4740.7 | 18940.1 |

The physical-cost result is the main blocker. M320 admission proves the semantic
posting target is promising, but its current atoms are not index-ready.

## Interpretation

M320 validates the idea but not the current loss as final.

What worked:

- Direct posting-native training is trainable.
- Dense teacher can be kept without making the route collapse.
- Lower dense weight improves atom recall while retaining BM25 complement.
- The best small canary, `w0.10/gap0.05`, improves NFCorpus unified metrics and
  raises atom-only recall on both datasets.
- M320 admission shows that direct dense-assisted posting-native atoms can beat
  M1050 atom-only quality by a large margin.
- Train/eval clipping and df-overflow losses both reduce fanout, so the cost
  problem is controllable in principle.

What failed:

- The high-quality M320 admission atoms rely on high-DF heads.
- Hard clipping and generic DF penalties reduce cost but remove useful semantic
  admission information.
- Static additive fusion still cannot turn the improved atom-only recall into
  robust top-rank unified quality.

## Decision

Do not promote M320 as a product route yet.

Keep M320 as a promising research direction, but change the next objective:

1. keep dense teacher as an admission/coverage teacher;
2. learn per-atom utility or export-time pruning instead of blunt DF penalties;
3. move top-rank supervision to final BM25+atom listwise ranking;
4. use dense-hit/BM25-miss positives and high-BM25 false positives explicitly;
5. avoid static additive fusion as the only scoring path.

The next useful experiment is M321:

- freeze the data/split from seed 1050;
- use the M320 admission checkpoint as the semantic high-recall source;
- add a post-hoc per-atom utility / DF-pruning diagnostic first;
- then train a small final candidate-pool listwise scorer if pruning cannot
  preserve M320 admission quality at usable cost.

If M321 cannot preserve the M320 admission gains while reducing touched docs by
an order of magnitude, this route should stop before full15 scale-up.

## M321 Post-Hoc Atom DF Pruning Diagnostic

Run:

- `/home/huoju/leask/runs/ii42-m321-atom-pruning-diagnostic-v1`
- checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`

M321 does not retrain. It loads the strongest M320 admission checkpoint and
evaluates export-time atom pruning by document-frequency ratio.

### NFCorpus M321 Heldout

| Variant | Atom R@100 | Atom MRR@20 | Atom NDCG@10 | Atom MAP@100 | Atom Docs | Atom Postings | Unified0.5 R@100 | Unified0.5 MRR@20 | Unified0.5 NDCG@10 | Unified0.5 MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `df<=1.0` | 0.1598 | 0.1809 | 0.0960 | 0.0452 | 3633.0 | 39532.5 | 0.2698 | 0.4705 | 0.3010 | 0.1403 |
| `df<=0.75` | 0.1483 | 0.1770 | 0.0944 | 0.0444 | 2460.0 | 6896.5 | 0.2652 | 0.4741 | 0.2966 | 0.1374 |
| `df<=0.50` | 0.1275 | 0.1305 | 0.0790 | 0.0410 | 1603.2 | 3132.3 | 0.2556 | 0.4708 | 0.2938 | 0.1361 |
| `df<=0.25` | 0.1009 | 0.1128 | 0.0536 | 0.0180 | 936.7 | 1377.3 | 0.2521 | 0.4753 | 0.2926 | 0.1352 |
| `df<=0.10` | 0.0844 | 0.0763 | 0.0476 | 0.0183 | 295.9 | 328.7 | 0.2511 | 0.4751 | 0.2914 | 0.1335 |

### SciFact M321 Heldout

| Variant | Atom R@100 | Atom MRR@20 | Atom NDCG@10 | Atom MAP@100 | Atom Docs | Atom Postings | Unified0.5 R@100 | Unified0.5 MRR@20 | Unified0.5 NDCG@10 | Unified0.5 MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `df<=1.0` | 0.6496 | 0.2922 | 0.3177 | 0.2899 | 5183.0 | 87242.1 | 0.8463 | 0.5621 | 0.5993 | 0.5516 |
| `df<=0.75` | 0.6663 | 0.3206 | 0.3452 | 0.3192 | 5160.6 | 47318.4 | 0.8463 | 0.5583 | 0.5944 | 0.5456 |
| `df<=0.50` | 0.6441 | 0.3201 | 0.3383 | 0.3182 | 5131.4 | 38692.6 | 0.8463 | 0.5578 | 0.5946 | 0.5458 |
| `df<=0.25` | 0.6244 | 0.2675 | 0.2972 | 0.2691 | 4484.2 | 13022.3 | 0.8552 | 0.5537 | 0.5901 | 0.5463 |
| `df<=0.10` | 0.5667 | 0.1863 | 0.2111 | 0.1859 | 2168.4 | 3012.4 | 0.8330 | 0.5394 | 0.5791 | 0.5288 |

### M321 Interpretation

The pruning diagnostic changes the conclusion:

- High-DF atoms are not uniformly useful or useless.
- On `scifact`, removing the most frequent atoms improves atom-BM25 ranking
  while cutting postings roughly in half.
- On `nfcorpus`, pruning hurts atom-only recall, but unified top-rank metrics
  stay close because lexical BM25 carries much of the final ranking.
- A single global DF threshold is too blunt. The next step should learn
  per-atom or per-query utility, then prune/downweight atoms by utility rather
  than raw DF alone.

M321 therefore keeps the route open. The next version should be M322:

1. compute per-atom utility from qrel-positive contribution, dense-hit/BM25-miss
   contribution, high-BM25 false-positive overlap, and DF cost;
2. evaluate utility-pruned exports against the same seed-1050 surface;
3. only if utility pruning preserves quality at much lower cost, scale to more
   datasets.

## M321 Utility-Pruning Follow-Up

Run:

- `/home/huoju/leask/runs/ii42-m321-atom-utility-pruning-v1`

This adds a first utility heuristic:

```text
utility(atom) =
    qrel_positive_overlap
    - 0.25 * BM25_false_positive_overlap
    -------------------------------------
             (df + 1) ^ 0.5
```

The first utility heuristic is not sufficient:

| Dataset | Variant | Atom R@100 | Atom MRR@20 | Atom NDCG@10 | Atom MAP@100 | Atom Docs | Atom Postings | Unified0.5 MRR@20 | Unified0.5 MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | `utility_top_0.75` | 0.1481 | 0.1632 | 0.0929 | 0.0466 | 3633.0 | 39345.0 | 0.4625 | 0.1403 |
| `nfcorpus` | `df<=0.75` | 0.1483 | 0.1770 | 0.0944 | 0.0444 | 2460.0 | 6896.5 | 0.4741 | 0.1374 |
| `scifact` | `utility_top_0.75` | 0.4648 | 0.1525 | 0.1672 | 0.1539 | 3047.0 | 5281.7 | 0.5679 | 0.5590 |
| `scifact` | `df<=0.75` | 0.6663 | 0.3206 | 0.3452 | 0.3192 | 5160.6 | 47318.4 | 0.5583 | 0.5456 |

Interpretation:

- The naive utility score selected too few high-DF semantic hubs.
- It did not reduce `nfcorpus` touched docs because selected atoms still appear
  nearly everywhere.
- It did improve `scifact` unified MAP/MRR slightly, so feature-based utility
  has signal, but the current formula is not suitable for atom-only retrieval.

Do not scale this utility heuristic. The next useful canary should make the
utility formula more cost-aware:

- increase DF cost exponent;
- increase BM25 false-positive penalty;
- keep dense/qrel positive contribution, but do not allow a tiny high-DF atom
  set to dominate the export.

## M321 Utility-Pruning v2

Run:

- `/home/huoju/leask/runs/ii42-m321-atom-utility-pruning-v2`

This run increased the DF cost and BM25 false-positive penalty:

- `utility_cost_alpha=1.0`
- `utility_false_positive_weight=1.0`
- `utility_false_positive_top_k=50`

It also fixed the `utility_top_x` semantics so `x` means a fraction of
positive-utility atoms, rather than a hidden fixed count.

The result is still not a promoted export rule:

| Dataset | Variant | Atom R@100 | Atom MRR@20 | Atom NDCG@10 | Atom MAP@100 | Atom Docs | Atom Postings | Unified0.5 R@100 | Unified0.5 MRR@20 | Unified0.5 MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | `utility_top_1` | 0.1227 | 0.1090 | 0.0627 | 0.0336 | 3633.0 | 22624.1 | 0.2560 | 0.4600 | 0.1390 |
| `nfcorpus` | `df<=0.25` | 0.1009 | 0.1128 | 0.0536 | 0.0180 | 936.7 | 1377.3 | 0.2521 | 0.4753 | 0.1352 |
| `scifact` | `utility_top_1` | 0.0556 | 0.0016 | 0.0000 | 0.0023 | 12.2 | 12.2 | 0.8241 | 0.5671 | 0.5604 |
| `scifact` | `df<=0.25` | 0.6244 | 0.2675 | 0.2972 | 0.2691 | 4484.2 | 13022.3 | 0.8552 | 0.5537 | 0.5463 |

Interpretation:

- On `nfcorpus`, the stronger heuristic still keeps hub atoms and touches the
  full corpus.
- On `scifact`, it swings too far toward rare atoms and collapses atom-only
  retrieval, although BM25+atom top-rank still has a small scoring signal.
- The failure mode is feature interaction, not a single scalar utility formula.

Therefore the next step should not be another manual utility formula. It should
test whether a learned candidate-pool scorer can exploit BM25/atom features
without changing the encoder.

## M322 Candidate-Pool Scorer

Files:

- `scripts/research_sae_m322_candidate_pool_scorer.py`
- `scripts/run_m322_candidate_pool_scorer_spark.sh`

Run:

- `/home/huoju/leask/runs/ii42-m322-candidate-pool-scorer-v2`

Invalid run:

- `/home/huoju/leask/runs/ii42-m322-candidate-pool-scorer-v1`

`v1` is invalid because heldout qrel positives were forced into the candidate
pool. That measured an oracle candidate surface, not real BM25/atom admission.
`v2` fixes this: qrel positives are only forced into training examples; heldout
examples use candidates admitted by BM25 and atom retrieval only.

M322 freezes the M320 admission checkpoint and trains a small residual scorer
over the union of BM25 and atom candidates. The scorer uses query/runtime-safe
features such as normalized BM25 score, atom score, rank features, BM25/atom
presence flags, query/doc atom counts, overlap mass, overlap IDF mass, and
overlap DF bands.

### Aggregate Heldout

The aggregate below is the same seed-1050 `nfcorpus` + `scifact` heldout
surface. The `best` row is the direct heldout scorer-selection metric from the
training script; the aggregate rows use the existing weighted aggregation helper
and are included for continuity.

| Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `df<=1.0` | `unified_scale_0.5` | 0.5474 | 0.5146 | 0.4446 | 0.3384 |
| `df<=1.0` | `m322_scorer` | 0.6075 | 0.5938 | 0.5009 | 0.3812 |
| `df<=1.0` | `m322_best` | 0.6170 | 0.5950 | 0.5061 | 0.3883 |
| `df<=0.75` | `unified_scale_0.5` | 0.5450 | 0.5146 | 0.4400 | 0.3339 |
| `df<=0.75` | `m322_scorer` | 0.6047 | 0.5893 | 0.4969 | 0.3807 |
| `df<=0.75` | `m322_best` | 0.6106 | 0.5902 | 0.5002 | 0.3851 |
| `df<=0.50` | `unified_scale_0.5` | 0.5400 | 0.5127 | 0.4387 | 0.3334 |
| `df<=0.50` | `m322_scorer` | 0.6078 | 0.5976 | 0.5029 | 0.3816 |
| `df<=0.50` | `m322_best` | 0.6177 | 0.5988 | 0.5080 | 0.3887 |
| `df<=0.25` | `unified_scale_0.5` | 0.5425 | 0.5130 | 0.4358 | 0.3332 |
| `df<=0.25` | `m322_scorer` | 0.6090 | 0.6125 | 0.5083 | 0.3934 |
| `df<=0.25` | `m322_best` | 0.6247 | 0.6145 | 0.5167 | 0.4054 |

### Per-Dataset Heldout

| Dataset | Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 | Atom Docs | Atom Postings | Candidates |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | `df<=0.25` | `unified_scale_0.5` | 0.2521 | 0.4753 | 0.2926 | 0.1352 | 936.7 | 1377.3 | - |
| `nfcorpus` | `df<=0.25` | `m322_scorer` | 0.3029 | 0.5732 | 0.3433 | 0.1590 | 936.7 | 1377.3 | 215.4 |
| `scifact` | `df<=0.25` | `unified_scale_0.5` | 0.8552 | 0.5537 | 0.5901 | 0.5463 | 4484.2 | 13022.3 | - |
| `scifact` | `df<=0.25` | `m322_scorer` | 0.9386 | 0.6547 | 0.6859 | 0.6458 | 4484.2 | 13022.3 | 297.7 |

Interpretation:

- M322 validates that the main M320/M321 blocker was not only atom allocation;
  final admission/ranking is learnable from BM25+atom posting features.
- `df<=0.25` is the best current cost/quality point. It beats static
  `unified_scale_0.5` on both datasets while keeping much lower atom posting
  cost than `df<=1.0`.
- The scorer overfits after early epochs on some variants. Future runs should
  use shorter training, stronger base preservation, or early stopping by
  heldout proxy.
- This is still a canary, not a full15 product result. The next useful step is
  to turn M322 into a stricter M323 validation: no qrel leakage, fixed early
  stopping, more datasets, and a source-independent scorer feature set.

## M323-M325 Candidate-Pool Follow-Up

M323 expanded the valid no-leakage scorer canary to `nfcorpus`, `scifact`, and
`fiqa` on the same seed-1050 M320 checkpoint.

Run:

- `/home/huoju/leask/runs/ii42-m323-candidate-pool-scorer-fiqa-v1`

Dense exact reference on the same split:

- `/Volumes/Betty/Tmp/ii42-m323-dense-same-split-db-exact.json`

Important validation caveat: the local VectorChord index path was not used for
the dense reference because the current local `vchordrq` index on these
halfvec embeddings produced divergent top-k results. The dense reference here
is exact PostgreSQL scan output with index scans disabled.

### M323 Heldout Aggregate

| Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `df<=0.25` | `unified_scale_0.5` | 0.5374 | 0.4262 | 0.3540 | 0.2815 |
| `df<=0.25` | `candidate_upper_bound` | 0.7225 | 1.0000 | 0.8426 | 0.7225 |
| `df<=0.25` | `m323_scorer` | 0.6588 | 0.5440 | 0.4463 | 0.3587 |
| `df<=0.25` | `best_epoch_20` | 0.6642 | 0.5485 | 0.4536 | 0.3663 |
| `df<=0.5` | `unified_scale_0.5` | 0.5456 | 0.4287 | 0.3566 | 0.2803 |
| `df<=0.5` | `m323_scorer` | 0.6584 | 0.5497 | 0.4513 | 0.3579 |
| `df<=0.5` | `best_epoch_40` | 0.6605 | 0.5517 | 0.4557 | 0.3614 |
| `dense_exact` | `same_split` | 0.7433 | 0.6152 | 0.5330 | 0.4464 |

Interpretation:

- M323 clearly beats static additive fusion, so learned final admission/ranking
  is a useful direction.
- M323 is still behind exact PPLX dense on all aggregate metrics.
- The gap is partly candidate admission and partly scoring: candidate upper
  bound is high but not dense-equivalent, and scorer quality is lower than the
  upper bound.

### M324 k300 Negative Result

M324 tested the simple hypothesis that M323 was mostly candidate-starved.

Run:

- `/home/huoju/leask/runs/ii42-m324-candidate-pool-scorer-k300-v1`

Changes from M323:

- `candidate_k=300`
- `hidden_dim=96`
- `residual_alpha=0.45`
- `epochs=120`

Heldout aggregate:

| Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `df<=0.25` | `unified_scale_0.5` | 0.5374 | 0.4262 | 0.3540 | 0.2815 |
| `df<=0.25` | `candidate_upper_bound` | 0.7542 | 1.0000 | 0.8698 | 0.7542 |
| `df<=0.25` | `m324_scorer` | 0.6259 | 0.5206 | 0.4227 | 0.3454 |
| `df<=0.25` | `best_epoch_40` | 0.6337 | 0.5236 | 0.4294 | 0.3531 |
| `df<=0.5` | `unified_scale_0.5` | 0.5456 | 0.4287 | 0.3566 | 0.2803 |
| `df<=0.5` | `candidate_upper_bound` | 0.7634 | 1.0000 | 0.8761 | 0.7634 |
| `df<=0.5` | `m324_scorer` | 0.6533 | 0.5144 | 0.4233 | 0.3370 |
| `df<=0.5` | `best_epoch_20` | 0.6558 | 0.5169 | 0.4273 | 0.3411 |

M324 raises the candidate upper bound, but the learned scorer regresses versus
M323. Larger candidate pools add useful positives and many more hard/noisy
negatives; the current pairwise/listwise objective does not distinguish
BM25-only, atom-only, and BM25+atom candidates well enough.

Decision:

- Do not continue the plain `candidate_k=300` + larger scorer branch.
- Keep the M323 k160 scorer as the current best learned scorer baseline.
- Continue with source-balanced admission/ranking, not more topK expansion.

### M325 Source-Balanced Scorer

M325 keeps the M320 checkpoint frozen and tests whether the k300 candidate
capacity can be used safely by balancing hard negatives by candidate source:

- forced training-only positive;
- BM25-only;
- atom-only;
- BM25+atom.

Code changes:

- `QueryExample` now stores per-candidate source labels.
- `query_loss` supports `--hard-negatives-per-source`.
- `candidate_pool_cost` reports candidate source ratios.
- The runner exposes `HARD_NEGATIVES` and `HARD_NEGATIVES_PER_SOURCE`.
- `--surface-cache-dir` caches frozen doc/query atom surfaces so scorer sweeps
  do not repeatedly pool transformer hidden states.

Active run:

- `/home/huoju/leask/runs/ii42-m325-source-balanced-scorer-k300-v1`

Initial parameters:

- `candidate_k=300`
- `hidden_dim=64`
- `residual_alpha=0.30`
- `hard_negatives=64`
- `hard_negatives_per_source=16`
- `epochs=80`
- `eval_every=20`

Promotion signal:

- If M325 does not beat M323 best on MRR/NDCG/MAP, source balancing alone is not
  enough. The next step should be a learned admission objective or feature
  family, not another scalar fusion or topK sweep.

### M325 Result

M325 completed but is not promoted.

Heldout aggregate:

| Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `df<=0.25` | `unified_scale_0.5` | 0.5374 | 0.4262 | 0.3543 | 0.2814 |
| `df<=0.25` | `candidate_upper_bound` | 0.7542 | 1.0000 | 0.8698 | 0.7542 |
| `df<=0.25` | `m325_scorer` | 0.6414 | 0.5100 | 0.4262 | 0.3360 |
| `df<=0.25` | `best_epoch_20` | 0.6488 | 0.5131 | 0.4325 | 0.3436 |
| `df<=0.5` | `unified_scale_0.5` | 0.5456 | 0.4287 | 0.3566 | 0.2803 |
| `df<=0.5` | `candidate_upper_bound` | 0.7634 | 1.0000 | 0.8761 | 0.7634 |
| `df<=0.5` | `m325_scorer` | 0.6549 | 0.5099 | 0.4277 | 0.3342 |
| `df<=0.5` | `best_epoch_20` | 0.6570 | 0.5126 | 0.4321 | 0.3385 |

M325 slightly improves some M324 NDCG points, but it remains below M323 and far
below the exact dense reference. Source-balanced hard negatives are therefore
not the missing ingredient.

Updated interpretation:

- Larger candidate pools expose useful positives; the upper bound is strong.
- The current scorer cannot convert that capacity into ranking quality with
  qrel-only positive labels and sparse runtime features.
- The next meaningful direction is dense-teacher or admission-impact
  supervision for the scorer: train the same runtime feature family against
  dense top-k / BM25+dense top-k soft targets, while still evaluating with qrels.
- Further scalar fusion, topK expansion, or hard-negative balancing is unlikely
  to close the dense gap by itself.
