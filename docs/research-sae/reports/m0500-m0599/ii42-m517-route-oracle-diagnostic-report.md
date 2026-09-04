# M517 Route Oracle Diagnostic Report

M517 follows the failed M516 teacher-adaptive policy with an oracle diagnostic.
The purpose is to decide whether adaptive prefix fanout is inherently exhausted
or whether the policy supervision signal is the bottleneck.

The script recomputes the same FiQA canary surface and evaluates p64/p96/p128
with three oracle selectors:

- `teacher_match_p128`: qrels-free oracle choosing the smallest prefix that
  matches fixed-p128 dense-teacher top100 candidate coverage.
- `qrels_match_p128`: diagnostic oracle choosing the smallest prefix that
  preserves fixed-p128 per-query NDCG@10 and Recall@100.
- `qrels_best_ndcg`: diagnostic oracle choosing the best per-query NDCG@10,
  then Recall@100, then smaller fanout.

The qrels oracles are not deployable. They are upper bounds for whether a
better policy target exists.

## Run Surface

- Script: `scripts/research_sae_m517_route_oracle_diagnostic.py`
- Local output:
  `outputs/m517/fiqa_route_oracle_diagnostic/m517_fiqa_route_oracle_diagnostic.json`
- Remote host: `spark-1`
- Task: `FiQA2018`
- Eval queries: `64`
- Policy train queries: `64`
- Route prefixes: `64, 96, 128`
- Route docs: `8539 / 57638`

## FiQA Oracle Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch | Prefix Hist |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `m517_lora_fixed_p128` | 0.46996 | 0.77407 | 0.57366 | 0.41626 | 0.83781 | 0.50703 | `{}` |
| `m517_oracle_teacher_match_p128` | 0.46996 | 0.77407 | 0.57366 | 0.41626 | 0.83781 | 0.50352 | `{'96': 4, '128': 60}` |
| `m517_oracle_qrels_match_p128` | 0.47045 | 0.83188 | 0.57522 | 0.42024 | 0.73391 | 0.38934 | `{'64': 47, '96': 9, '128': 8}` |
| `m517_oracle_qrels_best_ndcg` | 0.47045 | 0.83188 | 0.57522 | 0.42024 | 0.73391 | 0.38934 | `{'64': 47, '96': 9, '128': 8}` |
| `m516_lora_policy_rank0` | 0.46391 | 0.77147 | 0.57421 | 0.40893 | 0.81781 | 0.48380 | `{'64': 8, '128': 56}` |
| `m517_lora_fixed_p96` | 0.43941 | 0.73873 | 0.54017 | 0.38607 | 0.78125 | 0.44302 | `{}` |
| `m517_lora_fixed_p64` | 0.40675 | 0.73248 | 0.50925 | 0.35270 | 0.68594 | 0.35784 | `{}` |

## Interpretation

The adaptive-prefix route surface is not mathematically exhausted.  The qrels
oracle can pick mostly p64/p96 and still slightly beat fixed p128 on NDCG,
MRR, MAP, and Recall while reducing touched ratio from `0.50703` to `0.38934`.

However, the deployable teacher-candidate supervision used by M516 is almost
non-informative for this compression.  The qrels-free teacher-match oracle
chooses p128 for 60/64 queries and saves only `0.00351` touched ratio with no
metric gain.  That explains why M516 failed: matching dense-teacher top100
candidate coverage is too conservative and does not identify the queries where
small prefixes are actually sufficient for qrels ranking.

Candidate recall is also not the right scalar target for this route family:
the qrels oracle has much lower dense-teacher candidate recall (`0.73391`) than
fixed p128 (`0.83781`) while improving qrels metrics.  Continuing to optimize
teacher top100 coverage will preserve the wrong thing for this adaptive
fanout problem.

## Decision

Do not continue M516-style teacher-candidate threshold tuning.

The next useful test is an oracle-imitation diagnostic:

- Learn from train queries to predict the qrels-oracle prefix using only
  qrels-free route/query features.
- Evaluate on heldout queries.
- Treat this as a diagnostic, not a deployable final method, because the labels
  are qrels-derived.

If qrels-oracle imitation generalizes, then the route surface has learnable
query features and the next deployable target should approximate the same
signal without direct dataset qrels.  If it does not generalize, the qrels
oracle is probably too query-idiosyncratic and adaptive prefix fanout should be
stopped in favor of encoder/route-quality work.
