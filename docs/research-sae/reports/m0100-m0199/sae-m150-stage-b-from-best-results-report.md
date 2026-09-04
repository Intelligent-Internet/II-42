# SAE M150 Stage B From Best Results Report

Date: 2026-05-28

## Summary

M150 Stage B was resumed from the best broad Stage A checkpoint and completed
on the M130 held-out full-corpus evaluation surface. This is not the full
official BEIR15 matrix. It is the same `993,336` document / `886` query
PPLX all-data surface used by M130, so it is useful for direct continuity
against the current M130 candidate.

Run:

```text
bm25sae-m150-pplx16384k96-stageb-from-best-v1
```

ClearML task:

```text
http://100.116.110.26:8080/projects/636bc925317e42c08c922e016144c012/experiments/3e1217bad0ec48eeb94842ab3067abc2/output/log
```

Remote artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-from-best-v1
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-from-best-v1-full-corpus-eval
```

The run is a meaningful Stage B signal: it beats dense-only and BM25+dense on
Recall@100, MRR@20, and NDCG@10, and it beats BM25+dense on MAP@100. It does
not yet replace the promoted M130 Stage C `doc64/query80` candidate because
quality is lower than M130 Stage C and physical cost is higher.

## Candidate-Surface Training Result

The best checkpoint was selected from the Stage B candidate-surface objective:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-from-best-v1/bm25sae_stageb_best.pt
```

Best summary:

| Item | Value |
| --- | ---: |
| Best metric | 1.4819 |
| Training elapsed | 1,947.7 s |
| Candidate rows | 5,620 train / 886 validation |

Latest validation snapshot at step `5000`:

| Row | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| BM25 | 0.6061 | 0.3951 |
| Dense | 0.6817 | 0.4677 |
| SAE model | 0.8725 | 0.6526 |
| BM25+SAE model | 0.8702 | 0.6431 |

The candidate surface is therefore very strong, but this surface is easier than
the full-corpus retrieval gate because rows are already candidate-constrained.

## Full-Corpus Gate

Evaluation artifact:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-from-best-v1-full-corpus-eval/m110_full_corpus_index_eval.json
```

Full-corpus surface:

| Item | Count |
| --- | ---: |
| Documents with embeddings | 993,336 |
| Queries with embeddings and relevance | 886 |
| Top K | 100 |

Quality matrix:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3155 | 0.2851 | 0.2115 | 0.1393 |
| SAE | 0.3174 | 0.3407 | 0.2368 | 0.1555 |
| BM25+SAE RRF | 0.3246 | 0.3402 | 0.2306 | 0.1387 |
| BM25+SAE score fusion | 0.3248 | 0.3428 | 0.2348 | 0.1498 |

Delta for BM25+SAE score fusion:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus dense | +0.0116 | +0.0436 | +0.0097 | -0.0006 |
| Versus BM25+dense score fusion | +0.0092 | +0.0577 | +0.0233 | +0.0105 |
| Versus M130 Stage C `doc64/query80` | -0.0101 | -0.0260 | -0.0173 | -0.0128 |

Interpretation:

- M150 Stage B confirms that the broad Stage A checkpoint is viable.
- It already beats BM25+dense on the same held-out surface.
- It still trails M130 Stage C, so the active promoted candidate remains M130.
- SAE-only has the best MAP in this run, which means BM25 fusion is not yet
  calibrated optimally for this checkpoint.

## Physical Cost

| Row | Postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| BM25 | 1,829,152 | 475,879 | n/a |
| M150 Stage B SAE | 2,192,742 | 693,371 | 1.386 s |
| M150 Stage B SAE `doc64/query80` | 1,646,049 | 579,705 | 0.818 s |
| M130 Stage C `doc64/query80` | 1,536,353 | 603,979 | 0.735 s |

Relative to M130 Stage C `doc64/query80`, M150 Stage B currently costs about:

- `+42.7%` SAE postings/query.
- `+14.8%` accumulator entries/query.
- `+88.5%` sparse elapsed/query.

This cost gap is expected because this M150 check used the uncompressed
Stage B export path rather than the M130 Stage C clipping/fanout optimization.

## `doc64/query80` Clipped Gate

After the uncompressed result completed, the same checkpoint was re-evaluated
with M130-style `doc_active_k=64` and `query_active_k=80` clipping.

Artifact:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-from-best-v1-full-corpus-eval-doc64-q80/m110_full_corpus_index_eval.json
```

Quality matrix:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3155 | 0.2851 | 0.2115 | 0.1393 |
| SAE | 0.2993 | 0.3171 | 0.2208 | 0.1440 |
| BM25+SAE score fusion | 0.3173 | 0.3386 | 0.2318 | 0.1476 |

Delta for clipped BM25+SAE score fusion:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus unclipped M150 | -0.0075 | -0.0042 | -0.0030 | -0.0021 |
| Versus dense | +0.0041 | +0.0394 | +0.0066 | -0.0028 |
| Versus BM25+dense score fusion | +0.0017 | +0.0535 | +0.0203 | +0.0083 |
| Versus M130 Stage C `doc64/query80` | -0.0176 | -0.0302 | -0.0203 | -0.0150 |

Cost effect:

| Comparison | Postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| Versus unclipped M150 | -24.9% | -16.4% | -41.0% |
| Versus M130 Stage C `doc64/query80` | +7.1% | -4.0% | +11.3% |

Interpretation:

- Clipping works mechanically: it recovers most of the quality while bringing
  physical cost close to the M130 promoted profile.
- The quality gap versus M130 Stage C widens because this is still only a
  post-hoc Stage B export, not a Stage C refresh trained against the clipped
  full-corpus misses.
- This result narrows the next task: M150 needs Stage C-style hard-negative
  refresh or clipping-aware ranking, not another broad Stage A rerun.

## Stage B Fusion Calibration Probe

The initial Stage B comparison was misleading because the trainer learned
BM25/SAE scales on raw candidate SAE scores, while the full-corpus evaluator
uses per-query min-max score fusion. A post-hoc fusion sweep over the existing
full-corpus rankings shows that M150 has stronger SAE ranking signal than the
learned Stage B fusion exposed.

Best post-hoc fusion rows with `SAE weight = 1.0`:

| Row | BM25 weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M130 Stage B | 0.35 | 0.3325 | 0.3330 | 0.2416 | 0.1582 |
| M150 Stage B | 0.50 | 0.3253 | 0.3454 | 0.2449 | 0.1605 |
| M150 Stage B `doc64/query80` | 0.50 | 0.3208 | 0.3328 | 0.2350 | 0.1553 |

Interpretation:

- M150 Stage B is better than the raw learned-fusion comparison suggested.
- M150 beats M130 Stage B on MRR@20, NDCG@10, and MAP@100 after matching the
  fusion geometry, but it still trails on Recall@100.
- The learned Stage B scale overweights BM25 after evaluator-side min-max
  normalization. The next Stage B control should train with normalized SAE
  fusion enabled, then repeat the same full-corpus gate.
- Stage C candidate rows should be built from tuned fusion rankings, not from
  the miscalibrated learned-fusion list.

## Normalized-Fusion Stage B Control

Run:

```text
bm25sae-m150-pplx16384k96-stageb-fusionnorm-v1
```

Remote artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-fusionnorm-v1
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-fusionnorm-v1-full-corpus-eval
```

This control trains Stage B with SAE candidate scores min-max normalized before
BM25+SAE fusion, matching the full-corpus evaluator's score geometry.

Best checkpoint:

| Item | Value |
| --- | ---: |
| Best step | 5000 |
| Best metric | 1.3475 |
| Learned SAE fusion weight | 1.1696 |
| Learned BM25 fusion weight | 0.8611 |

Full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3157 | 0.2860 | 0.2121 | 0.1399 |
| SAE | 0.3009 | 0.3536 | 0.2423 | 0.1545 |
| BM25+SAE score fusion | 0.3211 | 0.3655 | 0.2497 | 0.1596 |

Delta for normalized BM25+SAE score fusion:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus dense | +0.0079 | +0.0663 | +0.0246 | +0.0092 |
| Versus BM25+dense score fusion | +0.0053 | +0.0795 | +0.0377 | +0.0197 |
| Versus original M150 Stage B | -0.0037 | +0.0227 | +0.0149 | +0.0098 |
| Versus M130 Stage C `doc64/query80` | -0.0138 | -0.0033 | -0.0024 | -0.0030 |

Physical cost:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| M150 Stage B normalized | 1,466,511 | 609,635 | 0.918 s |
| M130 Stage C `doc64/query80` | 1,536,353 | 603,979 | 0.735 s |

Interpretation:

- The Stage B issue was real and specific: the old candidate-surface fusion
  geometry did not transfer cleanly into the full-corpus min-max evaluator.
- Normalized Stage B fixes the ranking side: it almost matches M130 Stage C on
  MRR@20, NDCG@10, and MAP@100 before any Stage C refresh.
- It still trails M130 Stage C on Recall@100. That is exactly the gap Stage C
  is meant to close by refreshing hard negatives and low-ranked relevant
  candidates from actual full-corpus behavior.
- Normalized Stage B should become the M150 Stage B shape. Stage C should
  start from this checkpoint.

## Miss Taxonomy

Taxonomy artifact:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stageb-from-best-v1-full-corpus-eval/bm25sae_miss_taxonomy.json
```

Summary:

| Category | Count |
| --- | ---: |
| Relevant docs | 23,475 |
| Dense relevant hits | 3,449 |
| BM25+dense relevant hits | 3,352 |
| BM25+SAE relevant hits | 3,517 |
| Covered by BM25+SAE | 3,517 |
| Dense-hit but SAE candidate missed | 414 |
| Dense-only candidate missed | 333 |
| Candidate hit but score low | 1,177 |
| Not retrieved by controls | 18,034 |

BM25+SAE recovers `+165` more relevant hits than BM25+dense and `+68` more
than dense-only. The remaining blockers are ranking calibration and fanout,
not basic viability.

## Decision

Status: useful Stage B evidence, not promoted over M130 Stage C.

M150 should continue, but the next step should not be another broad Stage B
rerun. The immediate work should be:

1. Finish a Stage B control that trains with SAE scores normalized before
   fusion. This directly tests whether the Stage B weakness is a scale-transfer
   bug rather than a Stage A representation problem.
2. If the normalized Stage B full-corpus gate improves, make normalized fusion
   the Stage B training shape for this branch.
3. Run Stage C as a controlled comparison, not as a blind continuation:
   - `C0` keeps uncompressed full-corpus rankings for candidate discovery.
   - `C1` builds candidate rows from `doc64/query80` full-corpus rankings and
     keeps `doc64/query80` as the first deployment-shaped physical target.
4. In both Stage C variants, use tuned or normalized full-corpus fusion
   rankings as the primary `BM25+SAE` source list. Do not build Stage C rows
   from the old miscalibrated learned-fusion list.
5. Re-run the same M130-surface full-corpus gate and compare against M130 Stage
   C `doc64/query80`.
6. If M150 closes the remaining quality gap to M130 Stage C, run a
   representative official BEIR gate before any larger full BEIR15 matrix.
7. Only after those pass should M150 replace M130 as the active engineering
   candidate.

Implementation hook:

```text
scripts/run_m150_bm25sae_stage_c_spark.sh
```

The script is parameterized so the same code path can run original-shape and
clipped-aware Stage C. `STAGE_B_RANKING_DOC_ACTIVE_K=0` and
`STAGE_B_RANKING_QUERY_ACTIVE_K=0` produce the original C0 rankings;
`STAGE_B_RANKING_DOC_ACTIVE_K=64` and `STAGE_B_RANKING_QUERY_ACTIVE_K=80`
produce the C1 clipped-aware rankings.

M150-specific adaptation:

- Stage C candidate rows still include qrel positives, dense, BM25+dense,
  BM25+SAE, SAE, RRF, and BM25 candidates.
- The teacher target is no longer M130's dense-heavy target. It is now a
  blended M150 target with normalized `BM25+SAE` ranking as the dominant
  signal, standalone SAE as a semantic auxiliary, and dense/BM25+dense/BM25 as
  coverage controls.
- Default M150 Stage C teacher weights are:
  `teacher_bm25_sae_rank_weight=0.8`,
  `teacher_sae_rank_weight=0.2`,
  `teacher_dense_weight=0.4`,
  `teacher_fusion_rank_weight=0.2`, and
  `teacher_bm25_rank_weight=0.1`.

## M150-Aware Stage C C0

Run:

```text
bm25sae-m150-pplx16384k96-stagec-c0-m150aware-v1
```

Remote artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagec-c0-m150aware-v1
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagec-c0-m150aware-v1-full-corpus-eval
```

C0 starts from normalized-fusion Stage B, builds Stage C rows from
uncompressed full-corpus rankings, and evaluates the final checkpoint with the
deployment-shaped `doc64/query80` profile.

Candidate-surface validation at step `5000`:

| Row | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| Dense | 0.7144 | 0.4985 |
| SAE model | 0.8589 | 0.6382 |
| BM25+SAE model | 0.8149 | 0.5550 |

Learned final fusion weights:

| Weight | Value |
| --- | ---: |
| SAE | 1.1606 |
| BM25 | 0.8651 |

Full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3157 | 0.2859 | 0.2120 | 0.1399 |
| SAE | 0.3231 | 0.3538 | 0.2422 | 0.1533 |
| BM25+SAE score fusion | 0.3328 | 0.3771 | 0.2542 | 0.1579 |
| BM25+SAE RRF | 0.3326 | 0.3571 | 0.2409 | 0.1446 |

Delta for C0 BM25+SAE score fusion:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus normalized Stage B | +0.0118 | +0.0116 | +0.0044 | -0.0017 |
| Versus dense | +0.0197 | +0.0779 | +0.0291 | +0.0075 |
| Versus BM25+dense score fusion | +0.0171 | +0.0912 | +0.0422 | +0.0180 |
| Versus M130 Stage C `doc64/query80` | -0.0020 | +0.0084 | +0.0021 | -0.0047 |

Physical cost:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| M150 normalized Stage B | 1,466,511 | 609,635 | 0.918 s |
| M150 Stage C C0 `doc64/query80` | 942,155 | 464,836 | 0.555 s |
| M130 Stage C `doc64/query80` | 1,536,353 | 603,979 | 0.735 s |

Miss taxonomy summary:

| Category | Count |
| --- | ---: |
| Relevant docs | 23,475 |
| Dense relevant hits | 3,449 |
| BM25+dense relevant hits | 3,361 |
| BM25+SAE relevant hits | 3,628 |
| Dense-hit but SAE candidate missed | 431 |
| Dense-only candidate missed | 327 |
| Candidate hit but score low | 1,126 |
| Not retrieved by controls | 17,963 |

Interpretation:

- C0 is a real improvement over normalized Stage B on Recall@100, MRR@20, and
  NDCG@10, while sharply lowering sparse postings and elapsed time.
- C0 beats the promoted M130 Stage C profile on MRR@20 and NDCG@10 and is much
  cheaper, but still trails M130 on Recall@100 and MAP@100.
- The remaining issue is not basic coverage. BM25+SAE retrieves more relevant
  hits than dense and BM25+dense on this surface. The blocker is top-100
  ranking/calibration, visible in `candidate_hit_score_low`.
- Therefore C0 is not yet promoted. C1 should test whether building Stage C
  rows from the same `doc64/query80` deployment shape closes the Recall/MAP
  gap without losing C0's cost improvement.

## M150-Aware Stage C C1

Run:

```text
bm25sae-m150-pplx16384k96-stagec-c1-m150aware-doc64q80-v1
```

Remote artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagec-c1-m150aware-doc64q80-v1
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagec-c1-m150aware-doc64q80-v1-full-corpus-eval
```

C1 uses the same M150-aware teacher as C0, but builds Stage C rows from
`doc64/query80` deployment-shaped rankings. This tests whether training on the
same candidate shape as final serving closes the Recall/MAP gap.

Candidate-surface validation at step `5000`:

| Row | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| Dense | 0.7156 | 0.4895 |
| SAE model | 0.8555 | 0.6432 |
| BM25+SAE model | 0.8115 | 0.5546 |

Learned final fusion weights:

| Weight | Value |
| --- | ---: |
| SAE | 1.1608 |
| BM25 | 0.8649 |

Full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3157 | 0.2859 | 0.2120 | 0.1399 |
| SAE | 0.3186 | 0.3627 | 0.2455 | 0.1569 |
| BM25+SAE score fusion | 0.3279 | 0.3817 | 0.2559 | 0.1608 |
| BM25+SAE RRF | 0.3255 | 0.3561 | 0.2397 | 0.1454 |

Delta for C1 BM25+SAE score fusion:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus C0 learned fusion | -0.0050 | +0.0046 | +0.0017 | +0.0029 |
| Versus dense | +0.0147 | +0.0825 | +0.0308 | +0.0104 |
| Versus BM25+dense score fusion | +0.0122 | +0.0958 | +0.0439 | +0.0208 |
| Versus M130 Stage C `doc64/query80` | -0.0070 | +0.0130 | +0.0038 | -0.0018 |

Physical cost:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| M150 Stage C C0 `doc64/query80` | 942,155 | 464,836 | 0.555 s |
| M150 Stage C C1 `doc64/query80` | 950,920 | 469,514 | 0.612 s |
| M130 Stage C `doc64/query80` | 1,536,353 | 603,979 | 0.735 s |

Interpretation:

- C1 does not fix the Recall@100 gap. It moves the profile toward ranking
  quality: MRR@20 and NDCG@10 are the best learned-fusion rows so far, and
  MAP@100 improves over C0, but Recall@100 falls.
- The clipped-aware training shape is therefore not a universal improvement.
  It is useful as a ranking/MAP profile, while C0 remains the better recall
  profile.
- Both C0 and C1 are physically cheaper than M130 Stage C by a large margin.

## Stage C Fusion Calibration

After C0/C1 completed, a low-cost calibration sweep over the final
`BM25+SAE` fusion ratio showed that the learned candidate-surface fusion scale
is not the full-corpus optimum. Official evaluator reruns confirmed the
following calibrated profiles:

| Profile | BM25 weight | SAE weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | SAE postings/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M130 Stage C learned | 0.9140 | 0.9168 | 0.3349 | 0.3688 | 0.2521 | 0.1626 | 1,536,353 | 0.735 s |
| M150 C0 learned | 0.8651 | 1.1606 | 0.3328 | 0.3771 | 0.2542 | 0.1579 | 942,155 | 0.555 s |
| M150 C0 calibrated recall | 0.1000 | 1.0000 | 0.3374 | 0.3596 | 0.2468 | 0.1573 | 942,155 | 0.603 s |
| M150 C0 calibrated balance | 0.5000 | 1.0000 | 0.3340 | 0.3758 | 0.2562 | 0.1602 | 942,155 | 0.555 s |
| M150 C1 learned | 0.8649 | 1.1608 | 0.3279 | 0.3817 | 0.2559 | 0.1608 | 950,920 | 0.612 s |
| M150 C1 calibrated ranking | 0.5000 | 1.0000 | 0.3278 | 0.3748 | 0.2568 | 0.1634 | 950,920 | 0.558 s |

Interpretation:

- C0 calibrated recall beats M130 learned Recall@100, but sacrifices ranking
  metrics.
- C1 calibrated ranking beats M130 learned MAP@100, MRR@20, and NDCG@10, but
  sacrifices Recall@100.
- C0 calibrated balance is the best single fixed-weight compromise from this
  round: it nearly matches M130 Recall@100, beats M130 MRR@20/NDCG@10, and is
  materially cheaper.
- No single fixed-weight M150 profile dominates M130 on all four quality
  metrics yet. The remaining blocker is fusion calibration, not sparse model
  viability or physical cost.

Decision:

- Do not promote C1 as the only Stage C successor.
- Keep C0 calibrated balance as the current M150 engineering candidate for
  follow-up evaluation.
- Keep C0 calibrated recall and C1 calibrated ranking as Pareto endpoints.
- The next training step should not be another blind Stage C rerun. It should
  explicitly model the fusion tradeoff, either through fixed-ratio training,
  scale regularization, or a runtime-safe per-query calibration gate.

## A1 Stage C Redesign After C0/C1/A2

Date: 2026-05-30

Status: design target for the next A1 run.

The comparison with A2 changes the next step. A2's BM25-complement-aware
Stage A idea is useful, but the A2 checkpoint has worse full-corpus quality and
much higher fanout. Therefore the next promoted path should stay on A1 and use
the C0/C1 evidence directly.

### What C0/C1 Proved

1. C0 is the best recall/cost anchor.

   - It starts from normalized Stage B.
   - It uses uncompressed full-corpus rankings to discover candidates.
   - It reaches `Recall@100=0.3328`, `MRR@20=0.3771`,
     `NDCG@10=0.2542`, `MAP@100=0.1579`.
   - It reduces sparse cost to `942k` SAE postings/query.

2. C1 is the best ranking/MAP anchor.

   - It trains on the final `doc64/query80` deployment-shaped surface.
   - It reaches `MRR@20=0.3817`, `NDCG@10=0.2559`,
     `MAP@100=0.1608`.
   - It loses recall relative to C0: `Recall@100=0.3279`.

3. Fixed full-corpus calibration is stronger than learned candidate-surface
   scale.

   - C0 calibrated balance (`BM25=0.5`, `SAE=1.0`) is the best single
     engineering candidate: `Recall@100=0.3340`, `MRR@20=0.3758`,
     `NDCG@10=0.2562`, `MAP@100=0.1602`.
   - C0 calibrated recall (`BM25=0.1`, `SAE=1.0`) proves recall can beat M130,
     but it sacrifices ranking.
   - C1 calibrated ranking proves MAP/MRR/NDCG can beat M130, but it sacrifices
     recall.

4. A2 hard-row Stage B failed as a mainline successor.

   - A2 BM25+SAE final quality is lower:
     `Recall@100=0.3028`, `MRR@20=0.2796`,
     `NDCG@10=0.2066`, `MAP@100=0.1426`.
   - A2 cost is much higher: `2.51M` SAE postings/query.
   - The useful A2 part is not the checkpoint. It is the hard-row taxonomy:
     dense/BM25+dense hits missed by BM25+SAE, candidates scored too low, and
     high-fanout semantic false positives.

### C2 Training Shape

C2 should start from the C0 checkpoint, not C1.

Rationale:

- C0 already has the better recall/cost frontier.
- C1's ranking gains should be imported as supervision, not used as the sole
  initialization.
- Starting from C1 risks keeping the recall loss that C1 introduced.

Canonical C2 init:

```text
init checkpoint:
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagec-c0-m150aware-v1/bm25sae_stageb_best.pt

deployment profile:
doc_active_k=64
query_active_k=80

initial fusion scale:
BM25=0.5
SAE=1.0
```

Candidate rows should be mixed, not replaced:

| Source | Purpose | Weighting |
| --- | --- | --- |
| C0 rows | Preserve recall and broad full-corpus candidate discovery | high |
| C1 rows | Preserve deployment-shaped ranking/MAP behavior | high |
| A2 hard-row categories | Add diagnostic miss pressure, not checkpoint inheritance | low/targeted |

The first C2 implementation can use a simple merged C0+C1 row set. If that
does not move the full-corpus gate, add A2-style row categories only for:

- dense/BM25+dense relevant hits that A1 BM25+SAE misses;
- A1 BM25+SAE candidates that contain positives but score them below top-100;
- high-DF/high-fanout SAE false-positive groups.

Do not include generic A2 rows wholesale, because A2's representation has a
different score/fanout shape and already failed the promotion gate.

### C2 Loss And Selection

C2 should keep the normalized fusion geometry from Stage B/C:

```text
final_score = sae_scale * minmax(SAE) + bm25_scale * minmax(BM25)
```

Required changes relative to C0/C1:

1. Add fusion-scale regularization.

   Keep the model near the full-corpus calibrated balance:

   ```text
   target_bm25_scale = 0.5
   target_sae_scale = 1.0
   ```

   This prevents candidate-surface training from drifting back toward the
   learned `BM25≈0.86`, `SAE≈1.16` scale that looked good locally but was not
   the full-corpus optimum.

2. Use two candidate objectives.

   - Recall objective from C0 rows: preserve broad relevant-hit coverage.
   - Ranking objective from C1 rows: preserve MRR/NDCG/MAP gains under
     `doc64/query80`.

3. Select by a full-corpus-shaped proxy, not only Hit/MRR.

   Candidate-surface selection should approximate the deployment gate:

   ```text
   selection =
       1.20 * hit@20
     + 1.00 * mrr@20
     + 0.40 * multi_positive_coverage
     - 0.10 * predicted_fanout_penalty
     - 0.05 * scale_drift_penalty
   ```

   If implementation time is constrained, use `hit@20 + mrr@20` only as a
   smoke signal, then immediately rerun full-corpus eval before promotion.

4. Keep learning rate small.

   C0 is already close. C2 is calibration/refinement, not representation
   retraining.

   Recommended defaults:

   ```text
   lr = 1e-5 to 2e-5
   steps = 2500 to 4000
   batch_size = 8 to 12
   feature_k = 96
   st_after = 60%-70% of total steps
   ```

### C2 Acceptance Gate

C2 should be promoted only if it dominates C0 calibrated balance or creates a
strictly better Pareto point.

Primary gate against C0 calibrated balance:

| Metric | C0 balance | C2 target |
| --- | ---: | ---: |
| Recall@100 | 0.3340 | `>= 0.3340` |
| MRR@20 | 0.3758 | `>= 0.3758` |
| NDCG@10 | 0.2562 | `>= 0.2562` |
| MAP@100 | 0.1602 | `>= 0.1602` |
| SAE postings/query | 942,155 | `<= 1.05M` |

Secondary gate against M130 Stage C:

| Metric | M130 Stage C | C2 target |
| --- | ---: | ---: |
| Recall@100 | 0.3349 | close or higher |
| MRR@20 | 0.3688 | higher |
| NDCG@10 | 0.2521 | higher |
| MAP@100 | 0.1626 | close or higher |
| SAE postings/query | 1,536,353 | much lower |

If C2 improves ranking but loses recall like C1, do not promote it as the only
successor. Keep it as a ranking endpoint.

If C2 improves recall but loses ranking like C0 calibrated recall, keep it as a
recall endpoint.

Only a balanced C2 that preserves C0 recall and C1 ranking should become the
new A1-C candidate.

### C3 Only If C2 Gives Signal

C3 should not start unless C2 improves at least one full-corpus metric without
breaking the others.

Possible C3 directions:

1. Runtime-safe per-query gate.

   Use only query-time safe features:

   - BM25 score concentration;
   - SAE score concentration;
   - top-k overlap between BM25 and SAE;
   - SAE/BM25 score span;
   - predicted SAE fanout.

   Output a small BM25/SAE scale adjustment around the C0 balance point.

2. A1 hard-row refresh.

   Rebuild hard rows from A1 C2 full-corpus misses, not from A2. This keeps the
   representation shape consistent.

3. Official BEIR representative gate.

   Once C2 passes the continuity surface, rerun representative official BEIR
   full-corpus datasets before claiming promotion.

### Execution Recommendation

Run C2 on Spark first for tracked ClearML and full-corpus follow-up. The local
Mac can run candidate-surface smoke checks, but local ClearML is currently not
configured with valid credentials, and full-corpus eval still needs the larger
BM25/cache surface.

## A1 C2 Mixed-Surface Launch And Result

Date: 2026-05-30

Status: completed, not promoted as the primary M150 profile.

Run:

```text
scripts/run_m150_a1_c2_spark.sh
session: bm25sae_m150_a1_c2_mixed_v1
run: bm25sae-m150-a1-c2-mixed-v1
ClearML: http://100.116.110.26:8080/projects/636bc925317e42c08c922e016144c012/experiments/571a0034a6b34244a2a76d14b846a3fa/output/log
```

Artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-a1-c2-mixed-v1
/home/huoju/leask/runs/bm25sae-m150-a1-c2-mixed-v1-candidate-rows
/home/huoju/leask/runs/bm25sae-m150-a1-c2-mixed-v1-eval-candidate-rows
/home/huoju/leask/runs/bm25sae-m150-a1-c2-mixed-v1-full-corpus-eval-doc64-q80
```

Implementation changes:

- `research_sae_m130_bm25sae_stage_b_train.py` now supports
  `--loss-scale-regularization`, `--target-sae-scale`,
  `--target-bm25-scale`, and `--selection-scale-drift-weight`.
- `research_sae_merge_candidate_rows.py` merges C0 and C1 row roots while
  preserving corpus symlinks.
- `run_m150_a1_c2_spark.sh` launches the C2 mixed-surface run and final
  `doc64/query80` full-corpus eval.

Merged row surface:

| Split | Rows | Queries | Labels |
| --- | ---: | ---: | ---: |
| Train | 11,240 | 5,620 | 197,102 |
| Validation | 1,772 | 886 | 46,350 |

Initial observed candidate-surface signal:

| Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE scale | BM25 scale |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0.8505 | 0.6010 | 1.0000 | 0.5000 |
| 250 | 0.8521 | 0.6035 | 1.0031 | 0.4994 |
| 750 | 0.8516 | 0.6065 | 1.0093 | 0.4980 |
| 1,750 | 0.8533 | 0.6097 | 1.0219 | 0.4945 |
| 2,500 | 0.8567 | 0.6117 | 1.0312 | 0.4921 |
| 3,500 | 0.8567 | 0.6113 | 1.0438 | 0.4889 |

Interpretation:

- The scale regularizer worked well enough to prevent the old uncontrolled
  BM25/SAE drift. The final checkpoint still moves modestly toward SAE
  (`SAE=1.0438`, `BM25=0.4889`), but the selected best checkpoint is closer
  to the calibrated target (`SAE=1.0281`, `BM25=0.4929`).
- Candidate-surface ranking improved over the run, which confirms that merging
  C0 recall rows and C1 deployment-ranking rows is a real training signal.
- Candidate-surface improvement did not translate into a full promotion. The
  full-corpus gate below is the deciding evidence.

Full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3170 | 0.2953 | 0.2209 | 0.1460 |
| C0 calibrated balance | 0.3340 | 0.3758 | 0.2562 | 0.1602 |
| C1 calibrated ranking | 0.3278 | 0.3748 | 0.2568 | 0.1634 |
| A1 C2 mixed-surface | 0.3294 | 0.3691 | 0.2545 | 0.1614 |

Physical cost:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| C0 calibrated balance | 942,155 | 464,836 | 0.555 s |
| C1 calibrated ranking | 950,920 | 469,514 | 0.558 s |
| A1 C2 mixed-surface | 892,505 | 452,923 | 0.529 s |

Delta for C2:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus BM25+dense score fusion | +0.0123 | +0.0737 | +0.0335 | +0.0154 |
| Versus C0 calibrated balance | -0.0046 | -0.0067 | -0.0018 | +0.0012 |
| Versus C1 calibrated ranking | +0.0016 | -0.0057 | -0.0023 | -0.0020 |

Decision:

- C2 is a valid low-cost profile, but it does not dominate the existing
  C0/C1 Pareto frontier.
- C2 improves cost and slightly improves MAP over C0 balance, but it fails to
  preserve C0's Recall@100, MRR@20, and NDCG@10.
- C2 improves Recall@100 over C1 calibrated ranking, but loses C1's ranking and
  MAP advantage.
- Therefore C0 calibrated balance remains the current single-profile M150
  engineering candidate. C0 calibrated recall and C1 calibrated ranking remain
  Pareto endpoints. C2 should be retained as evidence that mixed-row training
  and scale regularization help cost, not as the next default.

Next implication:

- Do not run another blind row-merge Stage C variant.
- If continuing Stage C, C3 must be targeted: either restore C0 recall while
  preserving C2's lower fanout, or add a runtime-safe calibration gate that
  chooses between C0-balance/C1-ranking/C2-low-cost behavior per query.
- Any C3 row construction should come from C2 full-corpus misses and cost
  diagnostics, not from generic A2 rows or another broad candidate merge.

## A1 C3 From-B Full-Corpus-Aware Launch

Date: 2026-05-30

Status: completed on Spark.

Run:

```text
scripts/run_m150_a1_c3_from_b_spark.sh
session: bm25sae_m150_a1_c3_from_b_v1
run: bm25sae-m150-a1-c3-from-b-v1
log: /home/huoju/leask/logs/bm25sae_m150_a1_c3_from_b_v1.log
```

Artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-a1-c3-from-b-v1
/home/huoju/leask/runs/bm25sae-m150-a1-c3-from-b-v1-stageb-train-full-corpus
/home/huoju/leask/runs/bm25sae-m150-a1-c3-from-b-v1-stageb-eval-full-corpus
/home/huoju/leask/runs/bm25sae-m150-a1-c3-from-b-v1-candidate-rows
/home/huoju/leask/runs/bm25sae-m150-a1-c3-from-b-v1-eval-candidate-rows
/home/huoju/leask/runs/bm25sae-m150-a1-c3-from-b-v1-full-corpus-eval-doc64-q80
```

Design correction relative to C0/C1/C2:

- Start from normalized Stage B again, not from C0/C1/C2. This avoids inheriting
  a local Stage C endpoint bias.
- Build candidate rows from full-corpus Stage B rankings with uncompressed SAE
  active-k (`doc_active_k=0`, `query_active_k=0`), preserving the broad recall
  surface that made C0 better than C1.
- Keep hard negatives in source-ranking order and increase row sampling weight
  for full-corpus failure categories:

  | Category | Row weight |
  | --- | ---: |
  | `bm25_sae_hit` | 1.0 |
  | `dense_miss` | 2.5 |
  | `candidate_hit_score_low` | 2.0 |
  | `not_retrieved_by_controls` | 0.5 |

- Use a larger candidate row (`candidate_k=160`, `source_top_k=300`) so the
  training surface has enough room to represent top-100 ranking pressure.
- Train with `--weighted-row-sampling`, `retrieval_k=20`, normalized SAE fusion,
  and fusion-scale regularization toward `SAE=1.0`, `BM25=0.5`.

The C3 question is narrower than C2:

```text
Can a fresh-from-B, miss-aware Stage C preserve C0 recall while keeping C2-like
fanout control?
```

Final full-corpus `doc64/query80` quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3170 | 0.2953 | 0.2209 | 0.1460 |
| SAE | 0.3054 | 0.3745 | 0.2471 | 0.1546 |
| BM25+SAE RRF | 0.3207 | 0.3620 | 0.2400 | 0.1438 |
| BM25+SAE score fusion | 0.3252 | 0.3900 | 0.2583 | 0.1629 |

Physical profile:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| A1 C3 from-B miss-aware | 996,014 | 471,339 | 0.628 s |

Miss taxonomy:

| Item | Count |
| --- | ---: |
| Queries | 886 |
| Relevant docs | 23,475 |
| Dense relevant hits | 3,449 |
| BM25+dense relevant hits | 3,410 |
| BM25+SAE relevant hits | 3,583 |
| Dense hit but SAE candidate missed | 550 |
| Dense-only candidate missed | 286 |
| Candidate hit but score low | 1,128 |
| Not retrieved by controls | 17,928 |

Delta for C3:

| Comparison | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Versus BM25+dense score fusion | +0.0081 | +0.0948 | +0.0374 | +0.0169 |
| Versus M130 Stage C `doc64/query80` | -0.0097 | +0.0212 | +0.0062 | +0.0003 |
| Versus C0 calibrated balance | -0.0088 | +0.0142 | +0.0021 | +0.0027 |
| Versus C1 calibrated ranking | -0.0026 | +0.0152 | +0.0015 | -0.0005 |
| Versus C2 mixed-surface | -0.0042 | +0.0209 | +0.0038 | +0.0015 |

Decision:

- C3 is the strongest M150 ranking endpoint so far: it sets the best MRR@20
  and NDCG@10 observed in this line, and it slightly beats M130 Stage C on
  MAP@100.
- C3 is not a single-profile replacement because Recall@100 regresses versus
  C0 balance, C0 recall, and M130 Stage C.
- The recall loss is visible in standalone SAE as well as BM25+SAE fusion, so
  the issue is not only fixed-weight fusion. The representation became too
  hard-negative shaped and too selective for broad top-100 coverage.
- Do not continue C3 blindly. The next useful step is a calibration/gating
  sweep over existing C0/C1/C2/C3 endpoints or a C4 recall-preserving refresh
  from normalized Stage B with lower hard-negative pressure and an explicit
  top-100 coverage gate.

### C0/C1/C2/C3 Endpoint Oracle Probe

After C3 completed, a non-deployable per-query oracle was computed over the
existing C0 recall, C0 balance, C1 ranking, C2 mixed, and C3 miss-aware
`bm25_sae_score_fusion` top-100 lists. This does not represent a product
strategy because it uses relevance labels to choose the endpoint per query, but
it measures whether the endpoints contain complementary signal.

| Endpoint | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| C0 recall | 0.3374 | 0.3596 | 0.2468 | 0.1577 |
| C0 balance | 0.3340 | 0.3758 | 0.2562 | 0.1607 |
| C1 ranking | 0.3278 | 0.3748 | 0.2568 | 0.1639 |
| C2 mixed | 0.3294 | 0.3691 | 0.2545 | 0.1619 |
| C3 miss-aware | 0.3252 | 0.3900 | 0.2583 | 0.1635 |

Oracle upper bounds:

| Oracle objective | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Best per-query NDCG@10 | 0.3347 | 0.4713 | 0.3064 | 0.1872 |
| Best per-query Recall@100 | 0.3613 | 0.3964 | 0.2665 | 0.1688 |
| Best per-query MAP@100 | 0.3546 | 0.4641 | 0.3012 | 0.1898 |

Endpoint choices under the Recall@100 oracle:

| Endpoint | Queries selected |
| --- | ---: |
| C0 recall | 87 |
| C0 balance | 13 |
| C1 ranking | 59 |
| C2 mixed | 122 |
| C3 miss-aware | 605 |

Interpretation:

- The endpoints are meaningfully complementary. The recall oracle reaches
  `0.3613`, which is far above every fixed endpoint and above M130 Stage C.
- C3 is selected for most queries even under the recall oracle, so C3 is not a
  dead end. The failure is that a fixed endpoint loses the queries where C0/C2
  preserve broader top-100 coverage.
- The next step should be a runtime-safe endpoint/weight gate using query
  diagnostics, not another blind Stage C continuation.

### Runtime-Safe Gate Probe

A follow-up gate probe tested whether runtime-safe query diagnostics could
recover the oracle gap without another checkpoint. Two selectors were tried:

- `ridge`: linear ridge utility prediction over endpoint/weight action
  features.
- `stump`: a single threshold gate over query diagnostics, trained directly to
  maximize the fold objective.

Artifacts:

```text
/home/huoju/leask/runs/m150-endpoint-gate-*
/home/huoju/leask/runs/m150-endpoint-gate-stump-*
```

Best observed selector rows:

| Selector | Scope | Objective | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| Ridge | C3 only | Balanced | 0.3227 | 0.3951 | 0.2599 | 0.1631 |
| Stump | C3 only | Balanced | 0.3247 | 0.3930 | 0.2593 | 0.1615 |
| Stump | C3 only | Recall | 0.3232 | 0.3931 | 0.2595 | 0.1620 |
| Stump | All endpoints | Balanced | 0.3243 | 0.3920 | 0.2586 | 0.1611 |

Reference fixed rows from the same action grid:

| Fixed row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| C0 recall, `BM25=0.1`, `SAE=1.0` | 0.3374 | 0.3596 | 0.2468 | 0.1573 |
| C1 MAP, `BM25=0.5`, `SAE=1.0` | 0.3278 | 0.3748 | 0.2568 | 0.1634 |
| C3 ranking, `BM25=0.65`, `SAE=1.0` | 0.3251 | 0.3922 | 0.2584 | 0.1607 |
| C3 MRR, `BM25=0.8`, `SAE=1.0` | 0.3238 | 0.3950 | 0.2579 | 0.1605 |

Decision:

- The gate can slightly improve C3 ranking metrics, but it does not recover
  top-100 recall.
- The all-endpoint gate also fails to approach the oracle despite access to
  C0/C1/C2/C3 actions.
- Therefore the available runtime-safe diagnostics are not predictive enough
  to choose the recall endpoint reliably. The next useful step is not more
  gate fitting; it is a conservative C4 refresh that preserves C0 recall while
  importing C3 ranking signal.

## A1 C4 Recall-Preserve Launch

Date: 2026-05-30

Status: completed.

Run:

```text
scripts/run_m150_a1_c4_recall_preserve_spark.sh
session: bm25sae_m150_a1_c4_recall_preserve_v1
run: bm25sae-m150-a1-c4-recall-preserve-v1
log: /home/huoju/leask/logs/bm25sae_m150_a1_c4_recall_preserve_v1.log
ClearML: http://100.116.110.26:8080/projects/636bc925317e42c08c922e016144c012/experiments/e415ac09d457485da893bf654039072f/output/log
```

Artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-a1-c4-recall-preserve-v1
/home/huoju/leask/runs/bm25sae-m150-a1-c4-recall-preserve-v1-candidate-rows
/home/huoju/leask/runs/bm25sae-m150-a1-c4-recall-preserve-v1-eval-candidate-rows
/home/huoju/leask/runs/bm25sae-m150-a1-c4-recall-preserve-v1-full-corpus-eval-doc64-q80
```

Design:

- Initialize from C0, the current recall/cost anchor.
- Merge C0 rows with C3 miss-aware rows.
- Do not use weighted-row sampling. C3 proved ranking signal, but its hard
  negative weighting damaged broad top-100 recall.
- Use a lower learning rate (`1e-5`), stronger recall loss, and stronger
  fusion-scale regularization toward `SAE=1.0`, `BM25=0.5`.
- Select checkpoints with `hit@20` weighted higher than MRR, so candidate
  surface selection does not overfit top-rank quality at the expense of recall.

Candidate-surface checks:

| Item | Value |
| --- | ---: |
| Train rows | 11,240 |
| Validation rows | 1,772 |
| Step-1 validation BM25+SAE hit@20 | 0.8113 |
| Step-1 validation BM25+SAE MRR@20 | 0.4770 |
| Step-2800 validation BM25+SAE hit@20 | 0.8249 |
| Step-2800 validation BM25+SAE MRR@20 | 0.4996 |
| Step-1 SAE scale | 1.0000 |
| Step-1 BM25 scale | 0.5000 |
| Step-2800 SAE scale | 1.0175 |
| Step-2800 BM25 scale | 0.4946 |

Full-corpus result:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | SAE postings/query |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 | n/a |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 | n/a |
| BM25+dense score fusion | 0.3170 | 0.2953 | 0.2209 | 0.1460 | n/a |
| C0 balance | 0.3340 | 0.3758 | 0.2562 | 0.1602 | 942,155 |
| C1 ranking | 0.3278 | 0.3748 | 0.2568 | 0.1634 | 950,920 |
| C2 mixed | 0.3294 | 0.3691 | 0.2545 | 0.1614 | 892,505 |
| C3 miss-aware | 0.3252 | 0.3900 | 0.2583 | 0.1629 | 996,014 |
| C4 recall-preserve | 0.3309 | 0.3776 | 0.2582 | 0.1628 | 924,079 |
| M130 Stage C `doc64/query80` | 0.3349 | 0.3688 | 0.2521 | 0.1626 | 1,536,353 |

Decision:

- C4 is a useful Pareto point, but it is not a promoted replacement.
- Compared with C3, C4 recovers `+0.0058` Recall@100 while giving back
  `-0.0125` MRR@20. NDCG@10 and MAP@100 are effectively flat.
- Compared with C0 balance, C4 loses `-0.0030` Recall@100 but improves
  MRR@20 by `+0.0018`, NDCG@10 by `+0.0019`, and MAP@100 by `+0.0026`.
- Compared with M130 Stage C `doc64/query80`, C4 is much cheaper and improves
  ranking metrics, but it is still `-0.0039` Recall@100. This is not enough to
  claim a clean Stage C win.
- The row-mixture strategy has mostly reached its limit. The next improvement
  should target representation coverage or a richer runtime-safe signal, not
  another small C0/C3 mixture.

### C4 Active-Budget Diagnostic

After C4 completed, the first follow-up was a post-hoc active-budget sweep.
This checks whether the remaining Recall@100 gap is caused by representation
coverage or by query/document atom clipping.

Artifacts:

```text
scripts/run_m150_a1_c4_active_sweep_spark.sh
/home/huoju/leask/runs/bm25sae-m150-a1-c4-active-sweep-v1
/home/huoju/leask/runs/bm25sae-m150-a1-c4-active-sweep-doc-v1
```

Sweep rows:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | SAE postings/query |
| --- | ---: | ---: | ---: | ---: | ---: |
| C4 `doc64/query80` | 0.3309 | 0.3776 | 0.2582 | 0.1628 | 924,079 |
| C4 `doc64/query96` | 0.3363 | 0.3833 | 0.2606 | 0.1647 | 1,026,844 |
| C4 `doc80/query96` | 0.3403 | 0.3993 | 0.2684 | 0.1708 | 1,202,786 |
| C4 `doc96/query96` | 0.3409 | 0.4073 | 0.2720 | 0.1711 | 1,368,358 |
| C4 `doc128/query96` | 0.3409 | 0.4073 | 0.2720 | 0.1711 | 1,368,358 |
| M130 Stage C `doc64/query80` | 0.3349 | 0.3688 | 0.2521 | 0.1626 | 1,536,353 |

Decision:

- The remaining C4 recall problem is active clipping, not a hard representation
  failure. Increasing query active atoms from `80` to `96` improves every
  tracked quality metric.
- Increasing document active atoms from `64` to `80` and then `96` improves the
  ranking profile further. `doc128/query96` is identical to `doc96/query96`, so
  there is no evidence for going beyond the model's effective `96`-atom budget.
- `doc96/query96` cleanly beats M130 Stage C `doc64/query80` on Recall@100,
  MRR@20, NDCG@10, and MAP@100 while still touching fewer SAE postings.
- Active-budget candidate before DF pruning: C4 `doc96/query96`.
- The broad `query0` sweep was stopped because unclipped query atoms create a
  poor fanout/latency diagnostic. The focused continuation tests
  `doc80/96/128 × query96` to decide whether document budget should also move.

### C4 DF-Pruning Diagnostic

After the active-budget sweep, the next question was whether high-DF SAE atoms
were still adding noise and fanout. This diagnostic keeps the C4
`doc96/query96` active budget fixed and filters query-side SAE atoms whose
document frequency is above a fixed corpus ratio. The document index is not
rebuilt for each threshold; this is a query-time pruning policy that maps
directly to native posting metadata.

Artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150-a1-c4-df-sweep-doc96q96-v1
/home/huoju/leask/runs/bm25sae-m150-a1-c4-df-bracket-doc96q96-v1
```

Sweep rows:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | SAE postings/query |
| --- | ---: | ---: | ---: | ---: | ---: |
| C4 `doc96/query96`, no DF pruning | 0.3409 | 0.4073 | 0.2720 | 0.1711 | 1,368,358 |
| C4 `doc96/query96`, `max_df_ratio=0.20` | 0.3458 | 0.4050 | 0.2743 | 0.1719 | 1,200,702 |
| C4 `doc96/query96`, `max_df_ratio=0.18` | 0.3458 | 0.4050 | 0.2743 | 0.1719 | 1,200,702 |
| C4 `doc96/query96`, `max_df_ratio=0.15` | 0.3458 | 0.4050 | 0.2743 | 0.1719 | 1,200,702 |
| C4 `doc96/query96`, `max_df_ratio=0.12` | 0.3458 | 0.4050 | 0.2742 | 0.1719 | 1,196,693 |
| C4 `doc96/query96`, `max_df_ratio=0.10` | 0.3497 | 0.3911 | 0.2706 | 0.1710 | 1,113,886 |
| C4 `doc96/query96`, `max_df_ratio=0.05` | 0.3161 | 0.3083 | 0.2246 | 0.1534 | 872,475 |
| M130 Stage C `doc64/query80` | 0.3349 | 0.3688 | 0.2521 | 0.1626 | 1,536,353 |

Decision:

- High-DF SAE atoms are a real noise and cost source. A threshold around
  `0.12-0.20` improves Recall@100, NDCG@10, MAP@100, and physical cost versus
  the unfiltered C4 `doc96/query96` profile.
- The effective balanced point is `max_df_ratio=0.12`: it gives the same
  ranking profile as `0.15/0.18/0.20`, a tiny additional posting reduction,
  and the best MAP@100 among the balanced rows.
- `max_df_ratio=0.10` is a recall-max profile, but it trades away too much
  MRR@20 and NDCG@10 to be the default.
- `max_df_ratio=0.05` is below the safe pruning boundary and collapses quality.
- Current M150 balanced candidate profile: C4 `doc96/query96` with
  `max_df_ratio=0.12`.
- This policy is runtime-safe. It only requires per-atom document frequency in
  the query path, which the native sparse posting payload already needs for
  diagnostics and block selection.

## M150A2 Stage B Strategy Probe

Date: 2026-05-30

M150A2 changed the Stage A objective to preserve BM25-complement semantic
coverage more directly. That worked for standalone SAE recall, but the first
Stage B continuation exposed a new problem: the old Stage B objective does not
fit the new A2 score/fanout shape.

Baseline A2 Stage A artifact:

```text
/home/huoju/leask/runs/bm25sae-m150a2-pplx16384k96-stagea-v1
/home/huoju/leask/runs/bm25sae-m150a2-pplx16384k96-stagea-v1-full-corpus-eval-doc64-q80
```

A2 Stage A `doc64/query80` full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3152 | 0.2815 | 0.2086 | 0.1375 |
| SAE | 0.2693 | 0.2097 | 0.1667 | 0.1239 |
| BM25+SAE score fusion | 0.2997 | 0.2750 | 0.2021 | 0.1389 |
| BM25+SAE RRF | 0.2970 | 0.2993 | 0.2035 | 0.1341 |

Physical profile:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| M150A2 Stage A `doc64/query80` | 2,350,608 | 746,601 | 1.731 s |

Interpretation:

- A2 raises standalone SAE Recall@100 versus earlier M150 Stage A, but the
  BM25+SAE fusion row does not improve proportionally.
- The issue is therefore not only "SAE recall is low". The stronger issue is
  that A2's semantic atoms have a different score/fanout shape, so the old
  fusion and Stage B training objective are misaligned.

The direct A2 Stage B continuation confirmed this. It completed, but the best
checkpoint was selected at step `1`, which means optimization immediately
moved away from the useful surface.

Direct A2 Stage B artifact:

```text
/home/huoju/leask/runs/bm25sae-m150a2-pplx16384k96-stageb-from-best-v1
/home/huoju/leask/runs/bm25sae-m150a2-pplx16384k96-stageb-from-best-v1-full-corpus-eval
```

Direct A2 Stage B full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3152 | 0.2816 | 0.2087 | 0.1376 |
| SAE | 0.2752 | 0.2193 | 0.1831 | 0.1363 |
| BM25+SAE score fusion | 0.3020 | 0.2798 | 0.2074 | 0.1394 |
| BM25+SAE RRF | 0.2991 | 0.2935 | 0.2061 | 0.1331 |

Physical profile:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| Direct A2 Stage B | 3,352,665 | 773,448 | 1.768 s |

Interpretation:

- Direct Stage B improves over A2 Stage A slightly, but with worse physical
  cost and still far below dense on ranking quality.
- This is not a viable continuation path.

### B0 Frozen Fusion Calibrator Probe

To isolate whether the immediate blocker was encoder drift or fusion
calibration, a frozen calibrator was tested:

```text
scripts/research_sae_m150_b0_frozen_calibrator.py
scripts/run_m150a2_b0_frozen_calibrator_spark.sh
```

Run:

```text
bm25sae-m150a2-b0-frozen-calibrator-v1
```

Remote artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150a2-b0-frozen-calibrator-v1
/home/huoju/leask/runs/bm25sae-m150a2-b0-frozen-calibrator-v1-eval-candidate-rows
```

The calibrator keeps the A2 encoder frozen and learns only a query-level
BM25/SAE gate from runtime-safe features:

- SAE score entropy.
- BM25 score entropy.
- SAE and BM25 top-score gaps.
- SAE/BM25 score correlation.
- Top-10 overlap.
- SAE and BM25 score spans.

Candidate-surface best event:

| Item | Value |
| --- | ---: |
| Best step | 1 |
| Mean SAE gate | 0.5906 |
| Candidate Hit@20 | 0.6646 |
| Candidate MRR@20 | 0.4816 |

This candidate-surface result looked strong, but applying the same step-1
calibrator to the existing full-corpus A2 rankings did not beat simple fixed
fusion:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean SAE gate |
| --- | ---: | ---: | ---: | ---: | ---: |
| A2 equal fixed fusion | 0.2997 | 0.2750 | 0.2021 | 0.1389 | n/a |
| B0 frozen calibrated fusion | 0.2992 | 0.2644 | 0.1994 | 0.1396 | 0.5512 |

Decision:

- B0 is useful as a diagnostic, but not promoted.
- Candidate-surface calibration is still too weak as a training signal. It can
  overstate ranking quality even when full-corpus metrics do not improve.
- A2 needs Stage B training rows and model selection tied to full-corpus
  ranking/fanout behavior, not only candidate-constrained CE/KL.

### A2 Static Fusion Sweep

A static score-fusion sweep over the existing A2 Stage A full-corpus rankings
shows the current Pareto shape:

| SAE weight | BM25 weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.90 | 1.00 | 0.2992 | 0.2807 | 0.2011 | 0.1353 |
| 1.00 | 1.00 | 0.2997 | 0.2750 | 0.2021 | 0.1389 |
| 1.25 | 1.00 | 0.2991 | 0.2664 | 0.2008 | 0.1413 |
| 1.50 | 1.00 | 0.3016 | 0.2519 | 0.1954 | 0.1404 |

Interpretation:

- More SAE weight can raise Recall@100 slightly, but it hurts early ranking.
- Lower SAE weight improves MRR but loses MAP/Recall.
- The full-corpus optimum is multi-objective; a single fixed ratio cannot
  solve A2 cleanly.

### Revised Stage B Direction For M150A2

The next A2 Stage B should be a reset, not another continuation:

1. Build A2-aware hard rows from actual full-corpus misses, especially:
   dense/BM25+dense hits missed by BM25+SAE, A2 candidates scored too low, and
   high-fanout SAE false positives.
2. Train with explicit deployment-shaped targets:
   `doc64/query80` first, because that is the cost surface that matters.
3. Use a multi-objective selection score, not candidate Hit@10/MRR@10 alone:
   Recall@100, NDCG@10, MAP@100, postings/query, and accumulator entries/query.
4. Keep encoder updates small at first:
   start with fusion/score calibration and candidate-score correction, then
   unfreeze query-side atoms only if full-corpus selection improves.
5. Treat static fusion endpoints as controls:
   `0.9/1.0` for ranking, `1.25/1.0` for MAP, and `1.5/1.0` for recall.

Status:

- M150A2 Stage A is promising for coverage.
- The old Stage B objective is rejected for A2.
- Frozen B0 calibration is rejected as a promotion path.
- The next valid experiment is A2-aware hard-row Stage B with full-corpus
  selection, not another candidate-surface-only rerun.

## M150A2 Hard-Row Stage B Launch

Date: 2026-05-30

The next A2 experiment is now implemented and launched:

```text
scripts/run_m150a2_hard_stage_b_spark.sh
session: bm25sae_m150a2_hard_stageb_v1
log: /home/huoju/leask/logs/bm25sae_m150a2_hard_stageb_v1.log
run: bm25sae-m150a2-hard-stageb-v1
```

Code changes supporting this run:

- `research_sae_m130_build_pplx_candidate_rows.py` now supports
  hard-negative ordering and `row_weight` assignment.
- `research_sae_m109_diffsae_aligned_train.py` now loads and reports
  `row_weight` while keeping default behavior unchanged.
- `research_sae_m130_bm25sae_stage_b_train.py` now supports optional weighted
  row sampling and a configurable candidate-surface selection metric.

Run shape:

| Item | Value |
| --- | --- |
| Init checkpoint | `bm25sae-m150a2-pplx16384k96-stagea-v1/bm25sae_stagea_best.pt` |
| Ranking profile | `doc64/query80` |
| Ranking fusion | SAE `1.25`, BM25 `1.0` |
| Candidate K | `120` |
| Hard ordering | enabled |
| Weighted sampling | enabled |
| Dense miss row weight | `3.0` |
| Candidate-hit-score-low row weight | `2.5` |
| BM25+SAE-hit row weight | `0.7` |
| Not-retrieved row weight | `0.3` |

This experiment intentionally starts by generating A2 full-corpus train/eval
rankings. That is slower than candidate-only probes, but it is the correct
surface: the prior failure was caused by optimizing a candidate-constrained
objective that did not transfer to full-corpus ranking.

Acceptance criteria:

- It must beat direct A2 Stage B on full-corpus quality without increasing
  fanout materially.
- It should close the A2 gap toward M150 C0/C1 and M130 Stage C, especially
  Recall@100 and MAP@100.
- If it improves only candidate-surface Hit/MRR but not full-corpus metrics,
  reject it like B0.

Completed artifacts:

```text
/home/huoju/leask/runs/bm25sae-m150a2-hard-stageb-v1
/home/huoju/leask/runs/bm25sae-m150a2-hard-stageb-v1-full-corpus-eval-doc64-q80
```

Best candidate-surface checkpoint:

| Item | Value |
| --- | ---: |
| Best step | `3000` |
| Best metric | `0.9338` |
| Learned SAE fusion weight | `1.2908` |
| Learned BM25 fusion weight | `0.9651` |

Final `doc64/query80` full-corpus quality:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3154 | 0.2828 | 0.2098 | 0.1383 |
| A2 SAE | 0.2728 | 0.2295 | 0.1740 | 0.1234 |
| A2 BM25+SAE score fusion | 0.3028 | 0.2796 | 0.2066 | 0.1426 |
| A2 BM25+SAE RRF | 0.2975 | 0.3086 | 0.2110 | 0.1368 |

Physical profile:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| A2 hard-row Stage B `doc64/query80` | 2,512,803 | 699,841 | 1.118 s |

Miss taxonomy summary:

| Item | Count |
| --- | ---: |
| Relevant docs | 23,475 |
| Dense relevant hits | 3,449 |
| BM25+dense relevant hits | 3,351 |
| BM25+SAE relevant hits | 2,849 |
| Dense hit but SAE candidate missed | 749 |
| A2 candidate hit but scored low | 1,025 |
| Not retrieved by controls | 18,374 |

Decision:

- A2 hard-row Stage B improves slightly over direct A2 Stage B, especially
  MAP@100 (`0.1394` -> `0.1426`), and reduces the direct A2 physical cost.
- It still fails the promotion gate: Recall@100, MRR@20, NDCG@10, and MAP@100
  remain below the A1 C0/C1 profiles, while SAE postings/query remain much
  higher than A1 C0/C1.
- The useful A2 idea is the BM25-complement-aware Stage A / hard-row
  construction. The current A2 checkpoint and Stage B geometry should not
  replace the A1 mainline.
