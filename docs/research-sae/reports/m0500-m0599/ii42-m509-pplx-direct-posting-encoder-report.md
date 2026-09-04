# II-42 M509 PPLX Direct Posting Encoder Report

## Summary

M509 starts the raw text stage after M508.  The goal is not to add a posthoc
gate and not to run RL.  The goal is to train a PPLX-root encoder to emit the
dense and posting surfaces required by the M508 direct target.

This first implementation is a smoke gate:

- PPLX root: `perplexity-ai/pplx-embed-v1-0.6B`;
- target: row-int8 dense plus M508 direct support/posting head;
- training inputs: raw FiQA text only;
- no BM25, qrels, or dataset-name optimization in the loss;
- variants: frozen PPLX head-only and one last-layer transfer smoke.

Result: the pipeline is valid, but this is not yet a breakthrough.  Dense
preservation is already strong with head-only training.  The hard part remains
the support/posting shape.  Last-layer transfer helps in one small candidate
recall smoke but is not stable enough to promote.  The next serious step should
be proper LoRA/adapters with a longer supervised dense/posting objective, not
RL.

## Results

| Run | Layers | Rows | Trainable Params | Doc Dense Cos | Query Dense Cos | Doc Support Cos | Query Support Cos | Doc Active J | Query Active J | Subset Cand R@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa_smoke_headonly` | 0 | 384 | 5,251,072 | 0.94804 | 0.95330 | 0.33928 | 0.28714 | 0.10525 | 0.11334 | 0.57719 |
| `fiqa_smoke_last1` | 1 | 192 | 20,982,016 | 0.96432 | 0.95188 | 0.22463 | 0.17300 | 0.09593 | 0.09466 | 0.64250 |
| `fiqa_gate_last1_e2` | 1 | 384 | 20,982,016 | 0.94648 | 0.95150 | 0.32498 | 0.27728 | 0.13415 | 0.13954 | 0.56156 |

Artifacts:

- `outputs/m509/fiqa_smoke_headonly/m509_fiqa_smoke_headonly.json`;
- `outputs/m509/fiqa_smoke_last1/m509_fiqa_smoke_last1.json`;
- `outputs/m509/fiqa_gate_last1_e2/m509_fiqa_gate_last1_e2.json`.

## Interpretation

The dense side is not the blocker.  Even with the PPLX root frozen, the dense
head reaches about 0.95 doc/query cosine against the row-int8 dense teacher.
That supports the user's intuition that the root model already contains most
of the dense capability.

The support/posting side is still the blocker.  Support active Jaccard is only
about 0.10-0.14 in these smoke runs.  Candidate recall on the sampled subset is
above random and can reach 0.64 in the small last-layer smoke, but it is not
stable under the larger same-shape run.

This means the route is not dead, but the current head-only/last-layer probe is
not enough.  The evidence points to representation adaptation, not ranking
optimization:

- do not start RL yet;
- do not add BM25 to the first-stage encoder objective;
- do not keep tuning posthoc gates;
- move to LoRA/adapters and train longer against the direct dense/posting
  target.

## Decision

Promote M509 only as a pipeline/proof-of-wiring milestone.

Do not promote the current M509a head-only or last-layer checkpoint as a
retrieval model.

Next stage:

1. Implement proper LoRA/adapters on PPLX attention/MLP projections instead of
   full last-layer unfreezing.
2. Keep supervised dense/posting preservation as the first-stage loss.
3. Train longer on multiple tasks after a FiQA LoRA smoke passes support
   active-Jaccard and candidate-recall gates.
4. Add ranking/BM25-aware training only as a second stage after support
   preservation is stable.

## Proposed M510 Gate

M510 should be a PPLX LoRA direct-posting gate:

- LoRA rank 8 or 16 on the last 2-4 transformer blocks;
- dense head initialized from the raw hidden-to-dense head path;
- support head trained against M508 direct target;
- first gate on FiQA with no qrels in training;
- promotion metrics: support active Jaccard, support cosine, subset candidate
  recall, dense cosine;
- only then scale to Broad4/Broad10 and qrels evaluation.
