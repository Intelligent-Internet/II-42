# M516 Teacher-Adaptive Route LoRA Report

M516 tested whether a qrels-free, teacher-supervised adaptive fanout
controller can recover the M515 FiQA-local touch reduction without losing the
fixed-prefix route quality on a heldout qrels evaluation.

The controller is intentionally small: it keeps the M512/M515 route model
fixed, computes query-shape features from the p64 route surface, and learns
thresholds from dense-teacher top100 candidate coverage on train queries. BM25
and qrels are not used for policy selection.

## Run Surface

- Script: `scripts/research_sae_m516_teacher_adaptive_route_lora.py`
- Local output:
  `outputs/m516/fiqa_teacher_adaptive_route_lora/m516_fiqa_teacher_adaptive_route_lora.json`
- Remote host: `spark-1`
- Task: `FiQA2018`
- Eval queries: `64`
- Policy train queries: `64`
- Route prefixes: `64, 96, 128`
- Route docs: `8539 / 57638`

## FiQA Gate

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m516_lora_fixed_p128` | 0.46996 | 0.77407 | 0.57366 | 0.41626 | 0.83781 | 0.50703 |
| `m516_lora_policy_rank0` | 0.46391 | 0.77147 | 0.57421 | 0.40893 | 0.81781 | 0.48380 |
| `m516_lora_policy_rank1` | 0.46391 | 0.77147 | 0.57421 | 0.40893 | 0.81781 | 0.48380 |
| `m516_lora_policy_rank2` | 0.46391 | 0.77147 | 0.57421 | 0.40893 | 0.81781 | 0.48380 |
| `m516_lora_fixed_p96` | 0.43941 | 0.73873 | 0.54017 | 0.38607 | 0.78125 | 0.44302 |
| `m516_lora_fixed_p64` | 0.40675 | 0.73248 | 0.50925 | 0.35270 | 0.68594 | 0.35784 |

Best learned policy:

- Feature weights: `(1.0, 0.0, 0.0)`
- Thresholds: `mid=0.423147`, `high=0.423147`
- Train prefix histogram: `{'64': 10, '128': 54}`
- Eval prefix histogram: `{'64': 8, '128': 56}`
- Train candidate recall: `0.81797`
- Train touched ratio: `0.48752`

The three selected policies are effectively the same boundary under different
feature-weight parameterizations, so M516 did not discover a stronger
controller family.

## Verdict

M516 does not pass the FiQA promotion gate.

The learned controller reduces touched ratio from `0.50703` to `0.48380`, but
it also loses:

- NDCG@10: `0.46996 -> 0.46391`
- Recall@100: `0.77407 -> 0.77147`
- Candidate Recall@100: `0.83781 -> 0.81781`
- MAP@100: `0.41626 -> 0.40893`

This means the M515 hand-threshold result was not enough evidence for a
general policy. The teacher-supervised version still spends too little fanout
on queries where candidate support matters.

Broad4 scale-up is skipped because the FiQA gate failed against the same-run
fixed p128 route.

## Next Diagnostic

The next useful experiment is M517, not another threshold-feature tweak.

M517 should run an oracle diagnostic on the same fixed-prefix route surfaces:

- Choose the smallest prefix per eval query that preserves candidate recall or
  dense-teacher coverage against fixed p128.
- Compare oracle policy quality and touched ratio against fixed p128.
- If the oracle cannot beat fixed p128, prefix-adaptive fanout is exhausted for
  this route family.
- If the oracle can beat fixed p128, the bottleneck is controller
  expressiveness or policy supervision, not the route surface itself.

This keeps the stop rule clean: do not continue micro-tuning adaptive fanout
unless the oracle proves a reachable frontier exists.
