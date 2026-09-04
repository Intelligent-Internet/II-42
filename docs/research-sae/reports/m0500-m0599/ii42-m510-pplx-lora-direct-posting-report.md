# II-42 M510 PPLX LoRA Direct Posting Report

## Summary

M510 tests the next step after M509: keep PPLX as the semantic root, but adapt
it with lightweight internal LoRA instead of relying only on heads or full
last-layer unfreezing.

This is still a first-stage encoder test:

- no BM25 in the training objective;
- no qrels in the training objective;
- target is row-int8 dense plus the M508 direct posting/support target;
- LoRA is implemented locally, without PEFT, to avoid changing the torch stack.

Result: positive enough to continue.  LoRA with support-heavy loss improves the
posting/support shape and generalizes from FiQA to a Broad4 smoke.  It is not a
finished retrieval model yet because we still need full route/qrels evaluation,
but it is clearly better than M509 head-only and simple last-layer transfer.

## Runs

| Run | Tasks | Epochs | Rows | Trainable Params | Doc Dense Cos | Query Dense Cos | Doc Support Cos | Query Support Cos | Doc Active J | Query Active J | Subset Cand R@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa_lora2_e2` | 1 | 2 | 384 | 5,611,520 | 0.93961 | 0.94391 | 0.32804 | 0.27327 | 0.13784 | 0.13654 | 0.56266 |
| `fiqa_lora2_support_e4` | 1 | 4 | 384 | 5,611,520 | 0.93503 | 0.93915 | 0.45249 | 0.41990 | 0.15510 | 0.17172 | 0.62000 |
| `broad4_lora2_support_e4` | 4 | 4 | 768 | 5,611,520 | 0.92694 | 0.92844 | 0.35586 | 0.35892 | 0.17351 | 0.19364 | 0.64258 |

Artifacts:

- `outputs/m510/fiqa_lora2_e2/m510_fiqa_lora2_e2.json`;
- `outputs/m510/fiqa_lora2_support_e4/m510_fiqa_lora2_support_e4.json`;
- `outputs/m510/broad4_lora2_support_e4/m510_broad4_lora2_support_e4.json`.

## Comparison To M509

M509 head-only FiQA:

- subset candidate recall: 0.57719;
- doc/query support cosine: 0.33928 / 0.28714;
- doc/query active Jaccard: 0.10525 / 0.11334.

M509 last-layer/e2 FiQA:

- subset candidate recall: 0.56156;
- doc/query support cosine: 0.32498 / 0.27728;
- doc/query active Jaccard: 0.13415 / 0.13954.

M510 support-heavy FiQA:

- subset candidate recall: 0.62000;
- doc/query support cosine: 0.45249 / 0.41990;
- doc/query active Jaccard: 0.15510 / 0.17172.

M510 Broad4 support-heavy:

- subset candidate recall: 0.64258;
- doc/query support cosine: 0.35586 / 0.35892;
- doc/query active Jaccard: 0.17351 / 0.19364.

This is the first raw-PPLX-root variant in this branch that improves the
support/posting shape rather than only preserving dense vectors.

## Broad4 Task Rows

| Task | Doc Dense Cos | Query Dense Cos | Doc Support Cos | Query Support Cos | Doc Active J | Query Active J | Subset Cand R@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `ArguAna` | 0.91859 | 0.90360 | 0.30367 | 0.29227 | 0.18976 | 0.18904 | 0.58531 |
| `FiQA2018` | 0.94077 | 0.92562 | 0.40935 | 0.32749 | 0.17249 | 0.16644 | 0.63219 |
| `SCIDOCS` | 0.92007 | 0.91048 | 0.28101 | 0.22556 | 0.16770 | 0.16928 | 0.54312 |
| `TRECCOVID` | 0.92835 | 0.97407 | 0.42941 | 0.59037 | 0.16409 | 0.24980 | 0.80969 |

## Interpretation

The user hypothesis is supported at this stage: using PPLX as the root and
adapting it toward posting output is more promising than training a small
student from scratch or adding posthoc gates.

The dense function is not the bottleneck.  M510 keeps dense cosine around
0.93-0.94 while changing the support shape.  The support shape is still not
fully learned, but LoRA support-heavy training moves it in the right direction.

This also argues against starting RL now.  RL would optimize a noisy retrieval
surface before the encoder has learned stable support coordinates.  The better
next step is to add a route-level evaluation gate and continue supervised LoRA
training with better support targets.

## Decision

Promote M510 as the current best raw-PPLX-root route.

Do not promote it as a final retrieval model yet.

Stop:

- full last-layer unfreezing as the main adaptation mechanism;
- head-only training as the main solution;
- RL before support preservation.

Continue:

- LoRA/adapters on the PPLX root;
- support-heavy direct posting loss;
- no-BM25 first-stage encoder training;
- route/qrels evaluation as the next gate.

## Next Gate

M511 should evaluate the LoRA-produced support in an actual posting route:

1. Project all docs/queries for a small heldout task or Broad4 subset.
2. Use learned support as candidate coordinates.
3. Keep structural dense-tail scorer fixed.
4. Report NDCG@10, Recall@100, MRR@20, MAP@100, Dense O@100, candidate recall,
   and Touch.
5. Only if this route gate improves over M509/M508-derived baselines should we
   scale training size or introduce second-stage ranking/BM25 losses.
