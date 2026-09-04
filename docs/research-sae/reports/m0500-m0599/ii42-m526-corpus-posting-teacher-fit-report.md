# M526 Corpus Posting Teacher-Fit Report

M526 revises the text-to-posting route into a query-independent corpus
distillation gate:

```text
document text -> PPLX dense root -> posting_head(dense) -> dense-derived posting
```

No qrels, BM25, route positives, or ranking losses are used.  The target is
the deterministic signed posting surface derived from the materialized dense
embedding.  This directly tests whether the encoder can reproduce the posting
product we already know how to compute from dense.

## Why M526 Exists

The previous M52x route trained a separate support head from PPLX hidden states:

```text
PPLX hidden -> dense_head
PPLX hidden -> support_head
```

That was the wrong decomposition for the dense-derived posting target.  If the
posting is derived from dense, the posting head should sit after the dense
output.  Otherwise the support head is forced to rediscover dense structure
from hidden states, while the dense head already has it.

## Runs

| Run | Tasks | Rows | Adapter | Posting source | Support Cos | Active Recall | Active Jaccard | Sign Acc | Dense Cos |
| --- | --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| `fiqa_head_smoke` | FiQA2018 | 512 | head | hidden -> support MLP | 0.30487 | 0.19806 | 0.11044 | 0.73807 | 0.98823 |
| `fiqa_head_denseout_smoke` | FiQA2018 | 512 | head | dense -> posting head | 0.97107 | 0.82495 | 0.70481 | 0.99994 | 0.98727 |
| `fiqa_lora_denseout_smoke` | FiQA2018 | 512 | LoRA | dense -> posting head | 0.97106 | 0.82474 | 0.70452 | 0.99994 | 0.98727 |
| `broad4_head_denseout_e2` | ArguAna, FiQA2018, SCIDOCS, TRECCOVID | 8192 | head | dense -> posting head | 0.97216 | 0.82547 | 0.70594 | 0.99993 | 0.98708 |

## Interpretation

M526 strongly supports the corrected route.  The first hidden-to-support smoke
failed despite high dense cosine, which means the prior support-head placement
was the main shape error.  Moving the posting head behind the dense output
raises support cosine from `0.30487` to `0.97107` and signed active recall from
`0.18494` to `0.82495` on the same FiQA sample.

The broad4 run keeps the same shape at larger corpus scale.  Increasing from
512 FiQA rows to 8192 broad4 rows did not degrade the teacher-fit metrics:
support cosine is `0.97216`, active recall is `0.82547`, and target-active sign
accuracy is essentially perfect at `0.99993`.

The one-epoch LoRA smoke did not improve the ceiling.  It matches the head-only
result almost exactly.  This suggests the current remaining miss is not fixed by
light LoRA alone.  The likely bottleneck is the surface mismatch between the
direct AutoModel mean-pooled dense output and the materialized official PPLX
dense vectors.  Dense cosine is high (`~0.987`) but active support is
sign-sensitive, so the remaining dense-surface mismatch still flips about
17.5% of active coordinates.

## Decision

Stop the hidden-to-support MLP route for dense-derived posting.  The corrected
mainline is:

```text
official/text dense surface -> deterministic or learned dense-output posting head
```

M527 should focus on dense-surface alignment before another route/NDCG gate:

1. Verify whether the official SentenceTransformer/PPLX wrapper reproduces the
   materialized dense rows better than direct AutoModel mean pooling.
2. If the official wrapper is usable, use it as the root surface and rerun the
   corpus posting teacher-fit gate.
3. If the wrapper is not usable in the runtime path, train a dense-surface
   adapter from AutoModel pooled output to materialized dense over large corpus
   rows, then derive posting from that dense adapter.
4. Only after active recall materially exceeds the current `~0.825` ceiling
   should this line return to route/ranking evaluation.

## Artifacts

- `outputs/m526/corpus_posting_teacher_fit/m526_fiqa_head_smoke_seed5260.json`
- `outputs/m526/corpus_posting_teacher_fit/m526_fiqa_head_denseout_smoke_seed5260.json`
- `outputs/m526/corpus_posting_teacher_fit/m526_fiqa_lora_denseout_smoke_seed5260.json`
- `outputs/m526/corpus_posting_teacher_fit/m526_broad4_head_denseout_e2_seed5260.json`
- `scripts/research_sae_m526_corpus_posting_teacher_fit.py`
