# SAE M150 Stage C C5 Rescue Plan

Date: 2026-05-31

## Summary

The official BEIR full-corpus gate shows that C4 is not the final M150
endpoint. C4 was tuned on the M130 continuity surface and then improved with
active clipping and query-time DF pruning. That made it a useful balanced
candidate, but the official gate now exposes a different failure mode:

- `cqadupstack/all-test` has dense `Recall@100 = 0.7826`, standalone SAE
  `Recall@100 = 0.6782`, `BM25+dense = 0.7686`, and `BM25+SAE = 0.6993`.
- `fiqa`, `scidocs`, and `trec-covid` also show a consistent gap where SAE
  misses semantic positives that dense retrieves.
- The gap is too large to treat as only a fixed-fusion-weight problem. Fusion
  can improve ranking when both candidates are present, but it cannot recover
  dense-only positives missing from the sparse semantic surface.

C5 therefore restarts Stage C from the cleaner normalized Stage-B checkpoint
instead of continuing from C4.

## Decision

Keep broad Stage A. Do not restart representation pretraining yet.

Keep normalized Stage B as the C5 initialization point:

```text
bm25sae-m150-pplx16384k96-stageb-fusionnorm-v1
```

Do not train on official test qrels. The official BEIR gate remains an
evaluation surface. C5 uses the existing train/eval corpus rows and converts
the observed official failure into training geometry:

- increase dense and BM25+dense teacher pressure;
- reduce self-distillation from the previous BM25+SAE endpoint;
- oversample `dense_miss` and `candidate_hit_score_low` rows;
- generate rows from the same active/DF-pruned deployment geometry that the
  official gate uses.

## C5 Configuration

Runner:

```text
scripts/run_m150_a1_c5_stage_c_rescue_spark.sh
```

Base runner:

```text
scripts/run_m150_bm25sae_stage_c_spark.sh
```

Run name:

```text
bm25sae-m150-a1-c5-stagec-rescue-v1
```

Key settings:

| Setting | Value | Reason |
| --- | ---: | --- |
| Stage-B init | `stageb-fusionnorm-v1` | cleaner than C4, less over-shaped by old surface |
| Row-generation doc/query active | `96/96` | match current C4 official profile |
| Row-generation max DF ratio | `0.12` | expose training to deployment pruning |
| Eval doc/query active | `96/96` | same official candidate profile |
| Eval max DF ratio | `0.12` | same official candidate profile |
| Candidate K | `160` | allow dense positives missed by C4 into rows |
| Source top K | `300` | expose deeper dense/BM25+dense misses |
| Dense teacher weight | `0.70` | rescue semantic tail |
| BM25+dense rank weight | `0.65` | train toward the current gate baseline |
| BM25+SAE rank weight | `0.20` | avoid self-reinforcing C4 failure |
| SAE rank weight | `0.10` | keep sparse continuity but not as primary teacher |
| Dense-miss row weight | `3.0` | focus official-like semantic misses |
| Score-low row weight | `2.0` | fix candidates present but under-ranked |
| BM25+SAE-hit row weight | `0.55` | avoid overtraining already-solved rows |
| Steps | `7000` | larger row surface needs more optimization |
| Batch size | `10` | candidate width is larger than C0/C4 |

Training flags:

```text
--fusion-normalize-sae
--weighted-row-sampling
--loss-recall 1.2
--loss-multi-ce 0.9
--loss-teacher-kl 0.55
--loss-complement 0.65
--loss-scale-regularization 0.01
--target-sae-scale 1.0
--target-bm25-scale 0.5
--selection-hit-k 20
--selection-hit-weight 1.2
--selection-mrr-weight 0.8
--selection-scale-drift-weight 0.05
```

## Evaluation Plan

The runner first writes an M130-surface full-corpus matrix for quick comparison
against previous M150 checkpoints. This is not the final official claim.

After C5 finishes:

1. Compare C5 against Stage B, C3, C4, and BM25+dense on the M130 continuity
   matrix.
2. If C5 improves Recall@100 without collapsing MRR/NDCG/MAP, run the same
   official BEIR representative gates as C4:
   `fiqa`, `scidocs`, `trec-covid`, and `cqadupstack`.
3. Only if representative gates improve, run the full official BEIR matrix.

## Promotion Criteria

C5 can replace C4 only if:

- M130 continuity surface Recall@100 is at least C4-level;
- MRR/NDCG/MAP do not regress materially from C4;
- official representative gates close the `BM25+dense` gap on at least
  `fiqa`, `scidocs`, and `cqadupstack`;
- SAE postings/query remains within the C4 `doc96/query96/max_df_ratio=0.12`
  cost envelope or shows a clear quality/cost Pareto improvement.

## Known Risks

- Dense-miss oversampling may recover Recall@100 while harming NDCG/MAP.
- More candidate width can make candidate-surface validation too easy; the
  full-corpus matrix remains the gate.
- Training on deployment-pruned rows may overfit to `max_df_ratio=0.12`; if
  C5 improves only under that exact cap, follow-up should sweep DF ratios before
  promotion.
