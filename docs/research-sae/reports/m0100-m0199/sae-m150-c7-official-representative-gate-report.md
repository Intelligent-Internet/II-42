# SAE M150 C7 Official Representative Gate Report

Date: 2026-05-31

## Status

The official C7 representative gate is running on Spark:

```text
tmux: m150_c7_official_gate
log: /home/huoju/leask/logs/bm25sae-m150-a1-c7-official-gate-v1.log
run: /home/huoju/leask/runs/bm25sae-m150-a1-c7-official-gate-v1
```

Completed so far:

| Dataset | State |
| --- | --- |
| `fiqa` | completed |
| `scidocs` | completed |
| `cqadupstack` | completed |
| `trec-covid` | completed |
| `msmarco` | attempted, no completed artifact |

The partial local collector output is:

```text
results/m150-c7-official-profile-gate/all-test-partial/
```

## Partial Matrix

This table uses official full-corpus artifacts with the C6 checkpoint and
`doc96/query96/max_df_ratio=0.12`. It is a partial result, not a promotion
decision.

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.2533 | 0.3706 | 0.4174 | 0.3331 | 0.1571 |
| Dense | 0.4170 | 0.5687 | 0.5994 | 0.5264 | 0.2907 |
| SAE-only | 0.3613 | 0.5007 | 0.5277 | 0.4443 | 0.2362 |
| C6 learned BM25+SAE | 0.3759 | 0.5061 | 0.5579 | 0.4696 | 0.2511 |
| `rank_boost` | 0.3640 | 0.5136 | 0.5301 | 0.4474 | 0.2393 |
| `ndcg_boost` | 0.3654 | 0.5143 | 0.5342 | 0.4515 | 0.2419 |
| `recall_boost` | 0.3665 | 0.5136 | 0.5413 | 0.4551 | 0.2457 |
| `recall20_boost` | 0.3692 | 0.5112 | 0.5441 | 0.4575 | 0.2482 |

## Early Reading

- Fixed C7 profiles improve Recall@100 versus standalone SAE and C6 learned
  BM25+SAE, but they still do not close the dense gap.
- Dense is still ahead on all macro metrics for the completed official
  datasets.
- The C6 continuity-surface win does not automatically transfer to official
  full-corpus distributions. This validates the decision to run the official
  representative gate before more training.
- Current fixed profiles are useful diagnostic controls, but they are not yet a
  product promotion result.

## Deep Sparse Coverage Diagnostic

To separate semantic candidate coverage from top-100 ranking/fusion, a
deterministic `cqadupstack` sample diagnostic was run with C6 SAE top1000:

```text
run: /home/huoju/leask/runs/bm25sae-m150-c7-deep-sparse-coverage-sample-v1/all-test/cqadupstack
sample queries: 2000
top_k: 1000
```

Sample result:

| Source | Recall@100 | Query hit@100 |
| --- | ---: | ---: |
| Dense | 0.7818 | 0.8250 |
| BM25 | 0.5361 | 0.5970 |
| BM25+SAE learned fusion | 0.6983 | 0.7515 |
| SAE-only | 0.6827 | 0.7365 |

SAE deep-candidate curve:

| SAE depth | Recall | Query hit |
| --- | ---: | ---: |
| `@20` | 0.5356 | 0.5910 |
| `@100` | 0.6827 | 0.7365 |
| `@200` | 0.7531 | 0.8020 |
| `@500` | 0.8251 | 0.8635 |
| `@1000` | 0.8697 | 0.9000 |

Key diagnostic rates:

| Rate | Value |
| --- | ---: |
| dense hit but SAE miss@100 | 0.0995 |
| dense hit and recovered by SAE@1000 | 0.0870 |
| SAE hit@100 but dense miss@100 | 0.0110 |
| BM25+SAE suppresses SAE hit@100 | 0.0373 |

Interpretation:

- This is not a simple semantic-coverage failure. SAE@1000 is far above
  dense@100 on the sample.
- The primary failure is ranking/cutoff: relevant documents often exist in the
  SAE candidate pool, but below rank 100.
- Fusion still matters, but it is secondary. BM25+SAE learned fusion lifts
  SAE@100 from `0.6827` to `0.6983`, while SAE@500 reaches `0.8251`.
- The next strategy should target sparse score calibration, candidate-window
  promotion, and top-100 compression, not another broad Stage-A semantic
  coverage retrain.

## Top-100 Root-Cause Diagnostic

The first hypothesis was that official evaluation underestimates hybrid
retrieval because it uses one `top_k` as both candidate window and final
evaluation cutoff. That hypothesis is only partially true.

Follow-up diagnostic:

```text
run: /home/huoju/leask/runs/bm25sae-m150-top100-root-cause-sample-v1/all-test/cqadupstack
local: results/m150-top100-root-cause-sample/cqadupstack/
sample queries: 2000
top_k: 1000
```

Recovered positives below SAE top100 are usually not far from the rank-100
cutoff:

| Statistic | Value |
| --- | ---: |
| recovered positives after rank 100 | 327 |
| score / rank100 cutoff p50 | 0.8721 |
| score / rank100 cutoff p75 | 0.9471 |
| score / rank100 cutoff p90 | 0.9751 |
| gap to rank100 cutoff p50 | 0.1279 |

However, simply widening the SAE candidate window does not close the dense gap:

| Fusion candidate window | Recall@100 | Query hit@100 |
| --- | ---: | ---: |
| `SAE top100 + BM25 top100 -> final top100` | 0.6983 | 0.7515 |
| `SAE top200 + BM25 top100 -> final top100` | 0.7090 | 0.7630 |
| `SAE top500 + BM25 top100 -> final top100` | 0.7108 | 0.7640 |
| `SAE top1000 + BM25 top100 -> final top100` | 0.7139 | 0.7675 |

Noise signals are similar across buckets, with missed queries only slightly
higher fanout:

| First SAE hit bucket | Mean postings | Mean high-DF mass | Mean top atom DF |
| --- | ---: | ---: | ---: |
| `001_020` | 668685.0 | 2.5322 | 0.0791 |
| `021_100` | 684698.9 | 2.6227 | 0.0791 |
| `101_200` | 681881.0 | 2.5841 | 0.0804 |
| `201_500` | 668478.6 | 2.5221 | 0.0805 |
| `501_1000` | 695603.7 | 2.6579 | 0.0821 |
| `miss` | 690098.7 | 2.6868 | 0.0800 |

Interpretation:

- The current failure is not broad semantic coverage: SAE top500/top1000 can
  recover many dense hits.
- It is also not only the official candidate-window issue: widening the window
  helps, but only modestly.
- The main blocker is top-100 score compression and calibration. Relevant docs
  often score close to the cutoff, but current SAE scoring and BM25+SAE fusion
  do not separate qrel-positive deep candidates from near-neighbor false
  positives strongly enough.
- High-DF atom noise contributes, but the bucket-level signals do not indicate a
  single simple DF threshold fix.

## C8 Sparse Scorer Probe

A C8 probe tested whether simple runtime-safe sparse scoring features can fix
top-100 compression without changing the encoder. It reranked the same
`cqadupstack` sample using SAE top1000 candidates, BM25 top100, atom-IDF,
low-DF weighting, shared atom count, and a small train/eval linear candidate
ranker.

```text
run: /home/huoju/leask/runs/bm25sae-m150-c8-linear-ranker-probe-sample-v1/all-test/cqadupstack
local: results/m150-c8-linear-ranker-probe-sample/cqadupstack/
sample queries: 2000
split: deterministic query-level train/eval
```

Eval split result:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.7496 | 0.3795 | 0.3969 | 0.3526 |
| Existing BM25+SAE | 0.6595 | 0.3403 | 0.3522 | 0.3117 |
| Raw SAE | 0.6455 | 0.3005 | 0.3119 | 0.2743 |
| `bm25_sae_score_win1000` | 0.6768 | 0.3474 | 0.3557 | 0.3172 |
| `bm25_idf_win1000` | 0.6791 | 0.3450 | 0.3531 | 0.3150 |
| `linear_candidate_ranker` | 0.6818 | 0.3485 | 0.3578 | 0.3175 |
| `mlp_candidate_ranker` | 0.6811 | 0.3514 | 0.3584 | 0.3198 |

Linear ranker weights show the strongest useful signals are shared atom count,
atom-IDF score, BM25 presence/rank, and low-DF score:

```text
features: sae_score, idf, sqrt_idf, low_df, shared, bm25_norm,
          bm25_rank, sae_rank, has_bm25
weights:  0.0622, 0.2641, 0.0687, 0.1378, 0.4769, 0.1122,
          0.1520, 0.0205, 0.1569
```

Interpretation:

- Simple atom-IDF / low-DF reranking helps only slightly over widening the raw
  candidate window.
- The linear runtime-safe ranker improves eval Recall@100 from existing
  BM25+SAE `0.6595` to `0.6818`, but still leaves a large gap to dense
  `0.7496`.
- A small MLP/listwise ranker does not materially beat the linear ranker:
  `0.6811` Recall@100 and slightly better MAP/NDCG. This suggests the current
  runtime-safe feature set does not contain enough signal to close the gap.
- Therefore, the current blocker is not just a missing scalar IDF factor,
  shallow fusion formula, or small ranker capacity. The remaining gap likely
  requires better atom-level utility signals or a Stage-C objective that changes
  the sparse representation itself for near-neighbor discrimination.

## Next

The next stage should move beyond scalar C8 calibration while preserving the
root-cause evidence:

- Build atom-utility diagnostics from qrel wins/losses: identify atoms that
  repeatedly push false positives above qrel positives and atoms that reliably
  identify positives.
- Feed that signal back into Stage-C training or atom scoring, rather than only
  applying post-hoc DF/IDF scalars.
- Evaluate whether additional active atoms or a different Stage-C objective can
  improve near-neighbor ordering without exploding posting fanout.
- If atom-utility also fails, the remaining issue is likely representation
  allocation: the encoder can find semantically related documents but not enough
  task-specific discrimination for official qrels.

## Atom Win/Loss Probe

The follow-up atom-utility probe learned atom weights from train-split qrel
positives versus top-ranked non-qrel candidates, then evaluated the utility
scorer on the held-out query split.

```text
run: /home/huoju/leask/runs/bm25sae-m150-atom-win-loss-rerank-sample-v1/all-test/cqadupstack
local: results/m150-atom-win-loss-rerank-sample/cqadupstack/
sample queries: 2000
```

Eval split result:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.7496 | 0.3795 | 0.3969 | 0.3526 |
| Existing BM25+SAE | 0.6595 | 0.3403 | 0.3522 | 0.3117 |
| Raw SAE | 0.6455 | 0.3005 | 0.3119 | 0.2743 |
| Best atom-utility BM25 blend | 0.6766 | 0.3475 | 0.3561 | 0.3174 |

Interpretation:

- Simple qrel win/loss atom utility also fails to close the gap.
- The best atom-utility blend is roughly tied with the C8 IDF/window probes and
  below the linear/MLP ranker.
- This makes the post-hoc scoring route unattractive as the main fix. The
  remaining problem is more likely in Stage-C representation allocation:
  qrel-positive and false-positive candidates are not separated enough in the
  sparse atom space before scoring.

## Current Root-Cause Verdict

The current evidence rules out three shallow explanations:

1. **Not broad semantic coverage:** SAE top500/top1000 recovers many dense hits.
2. **Not just candidate-window size:** widening SAE candidates helps but does
   not approach dense.
3. **Not just post-hoc scoring:** DF/IDF, linear/MLP rankers, and atom win/loss
   utility all plateau near `0.676-0.682` Recall@100 on the held-out split.

The next meaningful work should change training, not just serving-time scoring:

- Stage-C should directly optimize near-neighbor discrimination inside SAE
  candidates, especially dense-hit/SAE-low and qrel-positive/false-positive
  pairs.
- The loss should reshape atom allocation or atom values, not only learn a
  shallow reranker after the fact.
- The promotion gate remains official full-corpus BEIR; these 2000-query probes
  are only diagnostic controls.
- Keep the official full-corpus matrix as the promotion gate; use this sample
  diagnostic only to design the next training/calibration objective.
