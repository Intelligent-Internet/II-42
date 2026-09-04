# M1820A PPLX Joint Token Router Report

Date: 2026-07-12

## Decision

**Stop M1820 after S0.** The frozen PPLX-root joint route/payload head learns a
small scoring improvement, but it does not learn the full-corpus admission
policy required by the predeclared signal gate.

S1 capacity expansion, corpus-global load training, and native BEIR closure
are not authorized.

## Tested Structure

```text
raw text
    -> frozen official PPLX forward
    -> canonical pooled-root summary token plus contextual tokens
    -> jointly trained 4,096-key router and 32-d payload projection
    -> qTopK=1 / dTopK=5
    -> one key -> (document, route weight, fp16 payload) index
```

The pooled root is a routed posting payload, not an ANN/CLS side branch. No
BM25, qrels, dataset identity, or external reranker was used.

The canary used 512 training queries with 4,607 documents and 128 disjoint
validation queries with 1,152 documents. Both train and validation root-summary
cosine were exactly `1.0`, so the result is not caused by the earlier raw-root
or token-truncation confounds.

## Fixed Gate

S0 required one non-initial checkpoint to satisfy both:

- direct held-out dense O@100 gain at least `+0.05` over initialization;
- dense candidate-upper O@100 at least `0.80`.

Neither gate passed.

## Result Matrix

| Step | Direct O@10 | Direct O@100 | Direct O@256 | Upper O@100 | Positive MRR | Score r | Max DF | KiB/query |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 0.661719 | 0.375469 | 0.354004 | 0.600000 | 0.639323 | 0.587804 | 0.561632 | 100.09 |
| 10 | 0.664062 | 0.376797 | 0.353058 | 0.600781 | 0.643350 | 0.588901 | 0.554688 | 99.995 |
| 25 | 0.666406 | 0.384453 | 0.353577 | 0.597422 | 0.648261 | 0.593240 | 0.560764 | 99.20 |
| 50 | 0.668750 | 0.388516 | 0.356079 | 0.596328 | 0.678161 | 0.592136 | 0.566840 | 98.64 |
| 100 | 0.674219 | 0.395078 | 0.362091 | 0.595859 | 0.660586 | 0.593312 | 0.543403 | 98.58 |
| **200** | 0.666406 | **0.405156** | **0.365295** | 0.595156 | 0.665246 | **0.597388** | 0.473090 | **97.32** |
| 400 | **0.675781** | 0.399688 | 0.365234 | 0.596484 | **0.676751** | 0.596770 | **0.451389** | 97.64 |

The selected checkpoint is step 200 because it has the best direct O@100.
Its gain is only `+0.0296875`, below the required `+0.05`. Candidate upper
falls from `0.600000` to `0.595156`, far below the `0.80` floor.

## Interpretation

### What learned

- Listwise loss fell from the early multi-point range to below `1.0` at some
  milestones.
- Direct O@100 improved modestly.
- Positive MRR and score Pearson improved.
- Max unique-document DF fell from `0.5616` to `0.4731` even without an
  explicit load penalty.

This confirms that the signed payload scorer and route weights receive a real
gradient. The implementation is not inert.

### What did not learn

Candidate admission did not improve. The model became better at ordering part
of an almost unchanged candidate pool, while the query/document key collision
needed to expose the dense tail stayed absent.

The index estimate also remains above the later product floor at about
`21.55 KiB/document`, before any load-constrained stage. A load penalty cannot
repair candidates that the current route never admits.

### Why more steps are not supported

Step 400 improves shallow O@10 and positive MRR but regresses direct O@100 from
the step-200 best. Candidate upper remains flat. This is not the monotonic
held-out trajectory expected from training-depth underfit.

The same causal split has now appeared in independent forms:

- M1610 scalar contextual routing had strong source oracles but weak direct
  query observability;
- M1803/M1804 query-only adapters could not generalize route selection;
- M1820 jointly trains route and payload heads from the correct PPLX root, but
  local listwise improvement still does not transfer to corpus-wide admission.

## Route Consequence

Do not add a DF penalty, expand to 8,192 keys, run FiQA, or perform another
head-only loss sweep. Those operations occur after the failed S0 admission
gate.

The external CITADEL result still proves that learned-key/vector-payload
retrieval is a valid architecture when the retriever is trained end to end at
large scale. M1820 shows that it is not obtained by attaching a small jointly
trained head to a frozen PPLX root on the available distillation surface.

A future continuation would require broad end-to-end retriever pretraining in
which route admission, payload geometry, and corpus load are learned together.
That is a new base-model training program, not another P1 compiler iteration,
and it should not be launched without a separate data/compute justification.

## Reproducibility

- Host: `spark-1`; `spark-2` was not disturbed.
- Container: `nvcr.io/nvidia/pytorch:26.03-py3`.
- ClearML task: `346e595b1f9b499c8c8abd64533f4725`.
- Run root:
  `/home/huoju/leask/runs/ii42-m1820-pplx-joint-router-v1`.
- Elapsed experiment time after launch: `110.75` seconds, including uncached
  official PPLX encoding and training.
