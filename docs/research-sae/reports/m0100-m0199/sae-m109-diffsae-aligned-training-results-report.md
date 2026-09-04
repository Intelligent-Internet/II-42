# SAE M109 DiffSAE-Aligned Training Results Report

Date: 2026-05-23

## Decision

M109 confirms that the `diffsae-codex` aligned training idea is useful on our
data when the candidate surface is cleaned and the training objective matches
the sparse retrieval contract.

The best M109 run is `8192/k64` with the default DiffSAE-style retrieval loss:

- M97 held-out all-candidate `MRR@10`: `0.6685`
- Dense baseline `MRR@10`: `0.6673`
- BM25 baseline `MRR@10`: `0.5972`
- M97 held-out all-candidate `hit@10`: `0.8591`
- Dense baseline `hit@10`: `0.8129`
- BM25 baseline `hit@10`: `0.7400`

This is the first clean evidence in this reset that a retrieval-aware sparse
encoder can match dense candidate-set ranking on the M97 held-out surface while
substantially improving candidate coverage. The result is still not a product
claim because it is candidate-set evaluation, not full-corpus sparse-index
evaluation.

## Setup

Script:

```text
scripts/research_sae_m109_diffsae_aligned_train.py
```

Training surface:

```text
/home/huoju/leask/runs/m107-m81-fulltrain-m96-materialized/m21_payload_root/m107_m81_fulltrain
```

Evaluation surface:

```text
/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval
```

The script normalizes the candidate rows by:

- requiring non-empty query embeddings,
- requiring non-empty document embeddings for the retained candidates,
- truncating every row to `candidate_k=80`,
- requiring the qrel positive to remain inside the retained top 80,
- evaluating validation/holdout/family groups separately.

Usable data:

| Surface | Raw Rows | Kept Rows | Notes |
| --- | ---: | ---: | --- |
| M107 train | 3920 | 3683 | 237 rows dropped mostly due missing query embeddings |
| M97 eval | 886 | 823 | 63 rows dropped mostly due missing query embeddings |

The raw candidate rows place the qrel positive at position zero. The report
therefore records `row_order_leak_check`, but this is not a meaningful
baseline. The model does not receive position features, so it still has to
score documents from embeddings; however candidate-set metrics alone remain
weaker evidence than full-corpus sparse-index metrics.

## Runs

| Run | Path | Features | Active K | Loss Variant | Best Step |
| --- | --- | ---: | ---: | --- | ---: |
| M109A | `/home/huoju/leask/runs/m109-diffsae-aligned-4096k64-v1` | 4096 | 64 | default | 1750 |
| M109B | `/home/huoju/leask/runs/m109-diffsae-aligned-8192k64-v1` | 8192 | 64 | default | 2000 |
| M109C | `/home/huoju/leask/runs/m109-diffsae-aligned-8192k64-rankheavy-v1` | 8192 | 64 | rank-heavy CE/KL | 2000 |

Default loss:

- recall: `1.0`
- single-positive CE: `0.2`
- multi-positive CE: `0.7`
- dense-teacher KL: `0.2`
- reconstruction: `0.02`
- k-budget: `0.001`

Rank-heavy loss:

- recall: `0.8`
- single-positive CE: `0.6`
- multi-positive CE: `1.0`
- dense-teacher KL: `0.4`
- reconstruction: `0.02`
- k-budget: `0.001`

## Overall M97 Candidate-Set Metrics

| Run | hit@1 | hit@10 | MRR@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| BM25 baseline | 0.5249 | 0.7400 | 0.5972 | 0.6004 |
| Dense baseline | 0.5930 | 0.8129 | 0.6673 | 0.6693 |
| M109A 4096/k64 | 0.5674 | 0.8712 | 0.6649 | 0.6682 |
| M109B 8192/k64 | 0.5796 | 0.8591 | 0.6685 | 0.6715 |
| M109C 8192/k64 rank-heavy | 0.5796 | 0.8530 | 0.6672 | 0.6716 |

Interpretation:

- `4096/k64` gives the strongest `hit@10`, but remains slightly under dense on
  `MRR@10`.
- `8192/k64` closes and slightly exceeds dense `MRR@10/MRR@20`, while keeping a
  large `hit@10` advantage.
- The rank-heavy run does not improve `hit@1` and loses `hit@10`, so directly
  increasing CE/KL pressure is not the right top-rank fix.

## Split Metrics

| Run | Split | hit@1 | hit@10 | MRR@10 | MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| M109A 4096/k64 | holdout | 0.5656 | 0.8770 | 0.6647 | 0.6683 |
| M109A 4096/k64 | validation | 0.5689 | 0.8665 | 0.6650 | 0.6682 |
| M109B 8192/k64 | holdout | 0.5601 | 0.8689 | 0.6607 | 0.6638 |
| M109B 8192/k64 | validation | 0.5952 | 0.8512 | 0.6747 | 0.6776 |
| M109C rank-heavy | holdout | 0.5519 | 0.8716 | 0.6520 | 0.6560 |
| M109C rank-heavy | validation | 0.6018 | 0.8381 | 0.6794 | 0.6841 |

The rank-heavy run looks better on validation top-rank but worse on holdout.
That is exactly the overfitting pattern we want to avoid.

## Family Metrics

| Run | Family | hit@1 | hit@10 | MRR@10 | MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| M109A 4096/k64 | BEIR current | 0.5663 | 0.8675 | 0.6471 | 0.6522 |
| M109A 4096/k64 | broad generated | 0.5676 | 0.8716 | 0.6669 | 0.6700 |
| M109B 8192/k64 | BEIR current | 0.5542 | 0.8313 | 0.6368 | 0.6381 |
| M109B 8192/k64 | broad generated | 0.5824 | 0.8622 | 0.6720 | 0.6752 |
| M109C rank-heavy | BEIR current | 0.5542 | 0.7952 | 0.6314 | 0.6347 |
| M109C rank-heavy | broad generated | 0.5824 | 0.8595 | 0.6712 | 0.6758 |

The best aggregate run still has a BEIR-current weakness. This matters because
the broad-generated surface dominates row count. The next experiment must not
optimize only the aggregate.

## What This Explains

M109 clarifies why `diffsae-codex` can make progress with a simpler training
loop:

1. It trains the representation directly through the retrieval rule.
2. It uses the same sparse-dot contract for training and serving.
3. It avoids late residual correction before the sparse representation is good.
4. It isolates the candidate-set retrieval objective from full SQL/runtime
   engineering.

Our earlier route repeatedly optimized adjacent objectives: dense-shape
preservation, sparse support compression, candidate generation, residual
ranking, and surface transfer. Those are all useful components, but they are
not a single retrieval contract. M109 suggests the core sparse retrieval model
should be trained first, then BM25 complementarity and runtime rankers should be
added around that stable sparse model.

## Negative Findings

1. **Candidate-set eval can be misleading.** The raw rows contain a row-order
   positive leak, so any row-order baseline is invalid. The model does not use
   position, but full-corpus evaluation is still required.
2. **More latent capacity is helpful but not decisive.** `8192/k64` improves
   MRR over `4096/k64`, but `4096/k64` has better `hit@10`.
3. **Naive rank-heavy loss is not the top-rank answer.** It improves validation
   top-rank but worsens holdout and BEIR-current family metrics.
4. **BEIR-current remains weaker than broad-generated.** This is the main
   robustness gap to track next.

## Next Step

M110 should move from candidate-set proof to index-contract proof:

1. Export the best M109B checkpoint into hard sparse query/doc activations.
2. Build the same sparse inverted index path used by `diffsae-codex` or our
   EATMH payload harness over the M97 eval corpus.
3. Evaluate full-corpus retrieval without candidate rows placing positives in
   front.
4. Compare:
   - BM25 candidate retrieval,
   - dense candidate retrieval,
   - M109 sparse retrieval,
   - BM25 + M109 sparse candidate union.
5. Keep split/family reporting, especially BEIR-current versus broad-generated.

If full-corpus sparse retrieval preserves the M109 candidate-set gain, the
DiffSAE-aligned route should become the main model line. If it collapses, the
issue is not the training loop itself but the candidate-bank/full-corpus gap.
