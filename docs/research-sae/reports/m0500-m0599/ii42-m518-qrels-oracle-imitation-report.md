# M518 Qrels-Oracle Imitation Report

M518 tests whether the M517 qrels-oracle prefix frontier is learnable from
simple qrels-free query/route features.

This is a diagnostic only.  The policy is trained with train-query qrels, so it
is not a deployable final retrieval method.  At heldout inference it only sees
the same qrels-free feature family used by M516: p64 touched ratio, support
entropy, and support flatness.

## Run Surface

- Script: `scripts/research_sae_m518_qrels_oracle_imitation.py`
- Local output:
  `outputs/m518/fiqa_qrels_oracle_imitation/m518_fiqa_qrels_oracle_imitation.json`
- Remote host: `spark-1`
- Task: `FiQA2018`
- Eval queries: `64`
- Policy train queries: `64`
- Route prefixes: `64, 96, 128`
- Route docs: `8539 / 57638`

## Heldout Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch | Prefix Hist |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `m517_lora_fixed_p128` | 0.46996 | 0.77407 | 0.57366 | 0.41626 | 0.83781 | 0.50703 | `{}` |
| `m518_qrels_policy_rank0` | 0.46996 | 0.77928 | 0.57366 | 0.41656 | 0.83344 | 0.50057 | `{'64': 2, '128': 62}` |
| `m518_qrels_policy_rank1` | 0.46996 | 0.78709 | 0.57366 | 0.41699 | 0.82641 | 0.49480 | `{'64': 4, '128': 60}` |
| `m518_qrels_policy_rank2` | 0.46996 | 0.78709 | 0.57366 | 0.41699 | 0.82641 | 0.49480 | `{'64': 4, '128': 60}` |
| `m517_oracle_qrels_match_p128` | 0.47045 | 0.83188 | 0.57522 | 0.42024 | 0.73391 | 0.38934 | `{'64': 47, '96': 9, '128': 8}` |
| `m517_oracle_teacher_match_p128` | 0.46996 | 0.77407 | 0.57366 | 0.41626 | 0.83781 | 0.50352 | `{'96': 4, '128': 60}` |

Best learned policy:

- Source: `m518_qrels_policy_rank1`
- Feature weights: `(1.0, 0.0, 0.0)`
- Thresholds: `mid=0.44319`, `high=0.44319`
- Train histogram: `{'64': 7, '128': 57}`
- Heldout histogram: `{'64': 4, '128': 60}`
- Touch: `0.49480`
- Recall@100: `0.78709`
- MAP@100: `0.41699`

## Interpretation

M518 finds a weak learnable signal, but not enough to justify continuing this
controller family as a main line.

Compared with fixed p128, the best qrels-imitation policy:

- Keeps NDCG@10 unchanged: `0.46996 -> 0.46996`
- Improves Recall@100: `0.77407 -> 0.78709`
- Improves MAP@100 slightly: `0.41626 -> 0.41699`
- Reduces touched ratio: `0.50703 -> 0.49480`

But it recovers only a small fraction of the M517 qrels-oracle frontier:

- Learned policy touch: `0.49480`
- Qrels-oracle touch: `0.38934`
- Learned policy p64 choices: `4 / 64`
- Qrels-oracle p64 choices: `47 / 64`

The current feature family is therefore too conservative.  Even when trained
with qrels-derived objectives, it mostly falls back to p128 and cannot learn the
large p64-heavy oracle frontier.

## Decision

Stop the p64/p96/p128 threshold-controller branch unless a materially richer
query signal is introduced.

The most useful next direction is not another scalar threshold grid.  It should
be either:

- richer policy features that can inspect route score margins/top-score
  stability without fully materializing p128, or
- a return to the core encoder objective, because the current controller only
  trims fanout by a few percent and does not improve dense-level quality.

For the active encoder route, this result says the bottleneck is still route
quality/support shape, not simple post-hoc prefix selection.
