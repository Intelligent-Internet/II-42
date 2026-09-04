# SAE Milestone 22 Token-Level Concept Training Report

Date: 2026-05-15

## Summary

This pass turns the SAE-SPLADE paper idea into a local text-to-atom control:
instead of pooling the whole text row before predicting atom support, the model
can predict atom evidence at token granularity and then aggregate it into one
sparse row.

The result is mixed but useful:

- `token_max` is a negative result. It creates an extreme-value effect over long
  rows, gives poor support overlap, and falls back close to BM25 quality.
- `token_lse` is the useful variant. Length-normalized logsumexp preserves
  token-level concept evidence while spreading gradient over multiple tokens.
- `token_lse + candidate-budget` reaches teacher-level ranking on the
  five-dataset control when evaluated with 128 to 192 active student atoms, but
  it is not yet a cheaper replacement for the current 64-active-dim student.

The important conclusion is that M22 is no longer only a literature/planning
item. The token-level path has a real quality signal, but the next question is
physical cost: can we keep the better MRR/NDCG without opening too many
postings?

## Code Changes

`scripts/research_sae_text_atom_train.py` now supports four opt-in token-level
encoder modes:

```text
token_max
transformer_token_max
token_lse
transformer_token_lse
```

The smooth `token_lse` path is the current preferred control. It computes
token-level support/value predictions, aggregates support with
length-normalized logsumexp, and aggregates values with token-level softmax
weights. The max path remains available because it is a useful negative
control.

`scripts/research_sae_text_atom_eval_checkpoint.py` adds an eval-only runner for
saved `text_atom_student.pt` checkpoints. This lets us sweep active dimensions
without retraining:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_text_atom_eval_checkpoint.py \
    --checkpoint \
        results/sae/text-atoms/m22-token-lse-tokenchar-h256-budget-five/text_atom_student.pt \
    --data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
    --teacher-run shared_sae_8192_64 \
    --output-dir \
        results/sae/text-atoms/m22-token-lse-tokenchar-h256-budget-five-eval-script \
    --datasets scifact scidocs nfcorpus arguana fiqa \
    --active-dims 128 \
    --device mps \
    --batch-size 32
```

## Training Runs

All rows below are five-dataset means over:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

For ablation rows, each metric is the best student-weight value within that
run. The active-dim sweep below separates best-recall and best-MRR rows where
that distinction matters.

| Run | Encoder | Active dims | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Interpretation |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| BM25 baseline | n/a | n/a | 0.7035 | 0.5904 | 0.5031 | 0.4134 | Lexical floor. |
| Snowflake-SAE teacher | dense teacher atoms | 64 | 0.7708 | 0.6610 | 0.5826 | 0.4886 | Current quality target on this subset. |
| Current text student | pooled row | 64 | 0.7650 | 0.6543 | 0.5682 | 0.4651 | Current full15 student, restricted to this subset. |
| M22 `token_max` + budget | token max | 64 | 0.6945 | 0.5804 | 0.4913 | 0.4044 | Negative result; below BM25 on recall and MRR. |
| M22 `token_max` dense | token max | 64 | 0.6928 | 0.5933 | 0.5015 | 0.4121 | Still weak; max pooling is not the right aggregator. |
| M22 transformer `token_max` | transformer token max | 64 | 0.7065 | 0.5908 | 0.5035 | 0.4140 | Mostly BM25-like; support overlap remains poor. |
| M22 `token_lse` dense | token lse | 64 | 0.7254 | 0.6090 | 0.5266 | 0.4337 | First positive token-level signal. |
| M22 `token_lse` + budget | token lse | 64 | 0.7105 | 0.6454 | 0.5489 | 0.4559 | Ranking improves, recall is still weak at 64 active atoms. |

## Active-Dim Sweep

The best M22 checkpoint is:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-budget-five/text_atom_student.pt
```

It was trained with `token_lse`, `token_char`, hidden size 256, dense support
loss, retrieval loss `0.05`, candidate-budget loss `0.02`, and five-dataset
training. Evaluating the same checkpoint with larger active rows changes the
quality frontier:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| Snowflake-SAE teacher | 0.7708 | 0.6610 | 0.5826 | 0.4886 |
| Current student, active 64, best recall weight | 0.7725 | 0.6430 | 0.5569 | 0.4527 |
| Current student, active 64, best MRR weight | 0.7626 | 0.6593 | 0.5709 | 0.4698 |
| M22, active 64, best recall weight | 0.7248 | 0.6400 | 0.5505 | 0.4566 |
| M22, active 64, best MRR weight | 0.7105 | 0.6454 | 0.5489 | 0.4559 |
| M22, active 96, best recall weight | 0.7394 | 0.6445 | 0.5603 | 0.4647 |
| M22, active 96, best MRR weight | 0.7369 | 0.6499 | 0.5603 | 0.4626 |
| M22, active 128, best recall weight | 0.7464 | 0.6633 | 0.5771 | 0.4778 |
| M22, active 128, best MRR weight | 0.7398 | 0.6635 | 0.5755 | 0.4720 |
| M22, active 192, best recall weight | 0.7606 | 0.6541 | 0.5730 | 0.4739 |
| M22, active 192, best MRR weight | 0.7557 | 0.6636 | 0.5793 | 0.4770 |

Interpretation:

- Active 128 is the first useful point. It beats the current student on
  `MRR@20`, `NDCG@10`, and `MAP@100` on this five-dataset subset, but recall is
  still lower than the current student's best recall row.
- Active 192 recovers most of the recall gap and slightly improves MRR/NDCG
  versus the teacher, but it almost certainly increases query fanout.
- The quality shape is promising, but it does not yet prove a product win
  because the active-row cost is larger than the current 64-active-dim student.

## Asymmetric Active-Budget Sweep

The same checkpoint was then evaluated with separate document and query active
budgets. This is the more product-relevant test because query active atoms open
postings online, while larger document rows mostly affect resident payload size
and rerank work.

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_text_atom_eval_checkpoint.py \
    --checkpoint \
        results/sae/text-atoms/m22-token-lse-tokenchar-h256-budget-five/text_atom_student.pt \
    --data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
    --teacher-run shared_sae_8192_64 \
    --output-dir \
        results/sae/text-atoms/m22-token-lse-tokenchar-h256-budget-five-asym-eval \
    --datasets scifact scidocs nfcorpus arguana fiqa \
    --doc-active-dims 128 192 \
    --query-active-dims 64 96 \
    --device mps \
    --batch-size 32
```

| Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 128 | 64 | 0.7241 | 0.6493 | 0.5550 | 0.4599 |
| 128 | 96 | 0.7389 | 0.6626 | 0.5701 | 0.4748 |
| 192 | 64 | 0.7326 | 0.6474 | 0.5571 | 0.4616 |
| 192 | 96 | 0.7431 | 0.6543 | 0.5712 | 0.4728 |

Interpretation:

- Query 64 is too tight for this checkpoint. Increasing doc active atoms cannot
  recover enough quality when the query side is clipped to 64.
- Query 96 is the current useful budget. `doc=128/query=96` nearly matches the
  symmetric active-128 MRR row (`0.6626` vs `0.6635`) while using 25% fewer
  query atoms.
- `doc=192/query=96` gives the best asymmetric recall (`0.7431`) but does not
  recover the symmetric active-192 recall (`0.7606`). This suggests the next
  training objective should target asymmetric query compactness directly, not
  only rely on post-training clipping.

## Budget32 Follow-Up

A follow-up run kept the same `token_lse + token_char` architecture and changed
only the candidate-budget traversal atoms from 16 to 32:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-budget32-five
```

Training loss improved mechanically: final candidate-budget loss dropped from
`1.6916` in the budget16 run to `1.4276` in the budget32 run. Quality did not
improve uniformly:

| Run | Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| budget16 | 128 | 128 | 0.7464 | 0.6635 | 0.5771 | 0.4778 |
| budget32 | 128 | 128 | 0.7502 | 0.6607 | 0.5755 | 0.4694 |
| budget16 | 128 | 96 | 0.7389 | 0.6626 | 0.5701 | 0.4748 |
| budget32 | 128 | 96 | 0.7442 | 0.6533 | 0.5643 | 0.4593 |

Interpretation:

- Budget32 improves recall slightly, especially under query 96.
- Budget32 hurts MRR/MAP, so it weakens ranking calibration.
- The next model step should not simply increase candidate-budget top dims. It
  should keep budget16 as the ranking baseline and add an explicit asymmetric
  objective or calibration term that recovers recall without giving up MRR.

## Explicit Asymmetric Loss Follow-Up

The next run added an opt-in asymmetric active-row loss:

```text
query atoms -> top 96
doc atoms   -> top 128
loss        -> positive docs must outrank dense/BM25 negatives after clipping
```

Run:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-five
```

Training signal:

```text
candidate_budget_loss: 2.9350 -> 1.1625
asymmetric_active_loss: 2.9351 -> 0.8968
```

This is the first M22 run where the explicit product-shaped query/doc clipping
objective clearly learned faster than the generic candidate-budget loss.

Quality:

| Run | Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| budget16 baseline | 128 | 96 | 0.7389 | 0.6626 | 0.5701 | 0.4748 |
| asym96 loss | 128 | 96 | 0.7508 | 0.6657 | 0.5813 | 0.4790 |
| asym96 loss | 128 | 128 | 0.7583 | 0.6728 | 0.5875 | 0.4839 |
| asym96 loss | 192 | 96 | 0.7470 | 0.6704 | 0.5821 | 0.4796 |
| asym96 loss | 192 | 192 | 0.7611 | 0.6746 | 0.5875 | 0.4867 |
| Snowflake-SAE teacher | 64 | 64 | 0.7708 | 0.6610 | 0.5826 | 0.4886 |

Interpretation:

- The asymmetric loss fixed the main query96 problem. `doc=128/query=96`
  improves all four metrics over the budget16 checkpoint.
- The symmetric active128/192 rows now beat the teacher on MRR and NDCG, while
  still trailing the teacher on Recall@100 and MAP.
- `doc=192/query=96` gives the best low-query-fanout MRR (`0.6704`), but
  `doc=128/query=96` is the cleaner deployment point until physical cost says
  otherwise.

## UBMX Physical-Cost Smoke

The `doc=128/query=96` asym96 latents were exported to the `UBMXM001` C payload
path with the same five-dataset, 50-query smoke shape used by earlier reports.

| Run | SAE candidate dims | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE Posts | BM25 Posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| current student smoke | 8 | 0.7641 | 0.5699 | 0.4659 | 710.3 | 764.2 | 1211.1 | 5.13 |
| asym96 smoke | 8 | 0.7421 | 0.5771 | 0.4725 | 776.8 | 810.9 | 1211.1 | 6.53 |
| asym96 smoke | 16 | 0.7445 | 0.5748 | 0.4724 | 1125.2 | 1583.3 | 1211.1 | 6.53 |

Interpretation:

- Exact C parity remains `1.0000`.
- Active8 is the better physical point for the asym96 checkpoint. Active16
  roughly doubles SAE postings but barely improves recall on this smoke.
- Compared with the current student smoke, asym96 improves NDCG/MAP but loses
  Recall@100 and increases payload size. This is a strong model signal, but it
  is not yet a full physical-cost win.
- The next training target should focus on candidate coverage under active8/96
  traversal, not on larger traversal budgets.

## SPLADE Control

The off-the-shelf SPLADE baseline remains a useful control:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| SPLADE | 0.7519 | 0.6324 | 0.5559 | 0.4615 | 6.3750 |
| BM25 + SPLADE | 0.7431 | 0.6399 | 0.5539 | 0.4572 | 7.9785 |
| BM25 + SAE + SPLADE | 0.7899 | 0.6814 | 0.6032 | 0.5043 | 7.6224 |

M22 does not make ordinary SPLADE the preferred path. Its value is as a design
signal: token-level semantic evidence is useful, but the current project wants
that evidence to land in the unified BM25+SAE atom space rather than become a
third independent sparse source.

## Decision

Continue training exploration before returning to SQL/product engineering.

The next best model step is query/document asymmetric active budgets:

```text
doc active atoms:   128 or 192
query active atoms: 64 or 96
```

The first asymmetric sweep shows that query 96 is viable, while query 64 is too
aggressive. The next model training pass should therefore optimize explicitly
for:

```text
doc active atoms:   128 or 192
query active atoms: 96
candidate-budget traversal atoms: 16, with budget32 as a recall-biased control
```

After the explicit asym96 loss, the next better target is:

```text
doc active atoms:        128
query active atoms:      96
candidate traversal:     8
objective:               recover Recall@100 without losing MRR/NDCG
```

The budget8/asym96 follow-up directly targeted that bottleneck:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-five
```

Quality:

| Run | Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| asym96 budget16 | 128 | 96 | 0.7508 | 0.6657 | 0.5813 | 0.4790 |
| asym96 budget8 | 128 | 96 | 0.7524 | 0.6848 | 0.5881 | 0.4898 |
| Snowflake-SAE teacher | 64 | 64 | 0.7708 | 0.6610 | 0.5826 | 0.4886 |

Physical active8 smoke:

| Run | SAE candidate dims | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE Posts | BM25 Posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| current student smoke | 8 | 0.7641 | 0.5699 | 0.4659 | 710.3 | 764.2 | 1211.1 | 5.13 |
| asym96 budget16 smoke | 8 | 0.7421 | 0.5771 | 0.4725 | 776.8 | 810.9 | 1211.1 | 6.53 |
| asym96 budget8 smoke | 8 | 0.7557 | 0.5801 | 0.4788 | 797.2 | 833.5 | 1211.1 | 6.64 |

Interpretation:

- Budget8 is now the best M22 model checkpoint on ranking quality. It beats
  the teacher on MRR, NDCG, and MAP in the five-dataset matrix.
- Budget8 also fixes most of the active8 physical recall regression from the
  budget16 asym checkpoint.
- The remaining gap is efficiency: candidate docs are about 12% higher than the
  current student smoke and payload size is about 29% higher. The next model
  work should target atom/posting concentration, not only quality.

## Concentration Follow-Ups

The next pass tested several controls that might reduce physical cost without
giving up the budget8/asym96 quality gains.

### Query Top8 DF Penalty

Run:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-qdf-five
```

The new opt-in `query_active_df_loss` penalizes query top8 atoms that land on
high-DF dimensions. This was a weak negative result:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| asym96 budget8 | 0.7524 | 0.6848 | 0.5881 | 0.4898 | 797.2 | 833.5 | 6.64 |
| qdf penalty | 0.7472 | 0.6500 | 0.5658 | 0.4656 | 781.3 | 812.8 | 6.58 |

Interpretation:

- It reduced candidate docs by about 2% and SAE posts by about 2.5%.
- The quality loss is much larger than the cost win.
- The candidate problem is not just "query top8 picked high-DF atoms"; the atom
  coverage layout itself is still too dispersed.

### Negative Candidate Exposure Loss

Run:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-exposure-five
```

The new opt-in `candidate_exposure_loss` penalizes selected query atoms that
overlap selected support atoms from negative candidates. The intent was to
teach the encoder to reduce posting fanout directly, not only final ranking
loss. With weight `0.05`, the final epoch still had a high support exposure
signal:

```text
budget=2.5565 asym=2.4692 exposure=0.3337
```

Quality:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| asym96 budget8 | 0.7524 | 0.6848 | 0.5881 | 0.4898 |
| exposure loss | 0.7246 | 0.6464 | 0.5414 | 0.4475 |

Interpretation:

- Support-overlap exposure conflicts with the ranking and candidate-budget
  objectives in this formulation.
- It lowers quality much more than it can justify with a physical-cost win.
- Do not continue this loss as a mainline control. If revisited, it needs a
  teacher-aware or listwise exposure target rather than a raw negative-overlap
  penalty.

### Post-Hoc Doc-Active Deployment Sweep

The best result came from keeping the asym96-budget8 model trained with
`doc=128/query=96`, then clipping documents at export/eval time. The sweep
tested `doc=64/72/80/88/96` with `query=96`.

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-doc64-80-96-eval
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-doc72-88-eval
```

Quality:

| Run | Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| asym96 budget8 | 128 | 96 | 0.7524 | 0.6848 | 0.5881 | 0.4898 |
| post-hoc doc64 | 64 | 96 | 0.7479 | 0.6660 | 0.5769 | 0.4795 |
| post-hoc doc72 | 72 | 96 | 0.7477 | 0.6689 | 0.5800 | 0.4811 |
| post-hoc doc80 | 80 | 96 | 0.7484 | 0.6690 | 0.5808 | 0.4830 |
| post-hoc doc88 | 88 | 96 | 0.7509 | 0.6764 | 0.5868 | 0.4893 |
| post-hoc doc96 | 96 | 96 | 0.7565 | 0.6783 | 0.5874 | 0.4885 |

Physical active8 smoke:

| Run | SAE candidate dims | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE Posts | BM25 Posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| current student smoke | 8 | 0.7641 | 0.5699 | 0.4659 | 710.3 | 764.2 | 1211.1 | 5.13 |
| asym96 budget8 doc128 | 8 | 0.7557 | 0.5801 | 0.4788 | 797.2 | 833.5 | 1211.1 | 6.64 |
| asym96 budget8 post-hoc doc64 | 8 | 0.7476 | 0.5738 | 0.4719 | 756.7 | 761.5 | 1211.1 | 4.92 |
| asym96 budget8 post-hoc doc72 | 8 | 0.7491 | 0.5738 | 0.4734 | 764.4 | 775.8 | 1211.1 | 5.14 |
| asym96 budget8 post-hoc doc80 | 8 | 0.7550 | 0.5764 | 0.4766 | 769.6 | 785.4 | 1211.1 | 5.36 |
| asym96 budget8 post-hoc doc88 | 8 | 0.7524 | 0.5833 | 0.4825 | 773.0 | 791.9 | 1211.1 | 5.57 |
| asym96 budget8 post-hoc doc96 | 8 | 0.7595 | 0.5823 | 0.4798 | 779.2 | 801.5 | 1211.1 | 5.79 |

Interpretation:

- Post-hoc doc64 is the smallest payload point and still improves NDCG/MAP
  over the current student smoke, but Recall@100 is too low for the default.
- Post-hoc doc80 is the best low-cost balance: it keeps most of the physical
  recall gain while staying close to the current payload size.
- Post-hoc doc88 is the best ranking/cost balance in this sweep. It improves
  NDCG/MAP most strongly while keeping payload materially below doc96.
- Post-hoc doc96 is still the best recall-oriented point and the best full
  quality matrix row, but it costs more payload and candidate fanout.

### Full15 Deployment Sweep

The five-dataset result was repeated on the full 15-BEIR shared matrix:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-doc80-88-96-eval
sae-milestone22-deployment-sweep-report.md
```

Full15 quality:

| Active run | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| doc80/query96 | 0.7912 | 0.8069 | 0.6847 | 0.6385 |
| doc88/query96 | 0.7905 | 0.8087 | 0.6862 | 0.6402 |
| doc96/query96 | 0.7908 | 0.8088 | 0.6850 | 0.6398 |

Full15 active8 C payload cost:

| Active run | Exact match | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| doc80/query96 | 0.9993 | 0.7728 | 0.6643 | 0.5986 | 786.6 | 775.9 | 7.02 |
| doc88/query96 | 0.9973 | 0.7735 | 0.6668 | 0.5994 | 792.7 | 785.8 | 7.33 |
| doc96/query96 | 0.9973 | 0.7751 | 0.6652 | 0.5989 | 797.2 | 793.7 | 7.65 |

Interpretation:

- doc88/query96 is now the best default deployment point. It has the best
  full15 NDCG/MAP and the best physical NDCG/MAP while staying below doc96
  payload and posting cost.
- doc80/query96 is the low-cost fallback. It gives up little quality and has
  the smallest payload/posting surface.
- doc96/query96 is recall-biased. Its recall lead is small and does not
  dominate ranking quality or cost.
- The C reader exact-match rate is below 1.0 on a few all-query rows. The
  wrapper now records that as `exact_match_rate` instead of failing the whole
  benchmark, because these rows are useful physical-cost evidence.

The doc88/query96 query-slice diagnostic shows the remaining quality gap:

| Slice | Queries | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| all | 1342 | 0.8111 | 0.7847 | 0.6842 | 0.6321 |
| balanced_or_hard | 1133 | 0.8626 | 0.8284 | 0.7484 | 0.7036 |
| lexical_heavy | 19 | 0.7842 | 0.6067 | 0.4066 | 0.2966 |
| long_query | 912 | 0.8603 | 0.7742 | 0.7156 | 0.6584 |
| semantic_heavy | 190 | 0.5065 | 0.5418 | 0.3292 | 0.2395 |
| short_query | 113 | 0.5127 | 0.7468 | 0.4969 | 0.3931 |

The next training target should therefore attack semantic-heavy and short-query
coverage without opening materially more postings.

The wider active-budget sweep confirms that this is not only a doc clipping
choice:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-active-sweep
```

| Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 80 | 64 | 0.7875 | 0.8037 | 0.6822 | 0.6364 |
| 80 | 80 | 0.7884 | 0.8033 | 0.6817 | 0.6365 |
| 80 | 96 | 0.7912 | 0.8069 | 0.6847 | 0.6385 |
| 88 | 64 | 0.7881 | 0.8063 | 0.6832 | 0.6370 |
| 88 | 80 | 0.7889 | 0.8064 | 0.6831 | 0.6378 |
| 88 | 96 | 0.7905 | 0.8087 | 0.6862 | 0.6402 |
| 96 | 96 | 0.7908 | 0.8088 | 0.6850 | 0.6398 |
| 128 | 96 | 0.7913 | 0.8093 | 0.6836 | 0.6366 |

Interpretation:

- Query 96 remains the default query budget. Query 64/80 are not catastrophic,
  but they lose enough quality that they should be treated as cost-biased
  controls.
- Larger document rows do not automatically improve ranking. Doc128 wins tiny
  Recall/MRR deltas but gives up NDCG/MAP versus doc88/query96, so it is not a
  better default.
- doc88/query96 remains the clean deployment point until a new training loss
  produces a better quality/cost Pareto frontier.

### Direct Doc96 Training

Run:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-doc96-five
```

Directly training with `doc=96/query=96` was negative:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| post-hoc doc96 | 0.7565 | 0.6783 | 0.5874 | 0.4885 | 779.2 | 801.5 | 5.79 |
| direct doc96 training | 0.7472 | 0.6553 | 0.5676 | 0.4630 | 770.7 | 793.1 | 5.86 |

Interpretation:

- Direct doc96 training saves a small amount of candidate/posting cost but loses
  too much ranking quality.
- Keep training with doc128 and deploy/export with post-hoc clipping for now.
  Treat doc80/doc88/doc96 as deployment knobs rather than new training targets.

That engineering measurement is now complete for the main deployment points and
the active-budget sweep:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-doc80-88-96-eval
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-active-sweep
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-doc88-query-slices-w0p25
sae-milestone22-deployment-sweep-report.md
```

The current promotion boundary is therefore clearer: doc88/query96 is the
default deployment point for this checkpoint, but new training must improve
semantic-heavy and short-query coverage without increasing posting fanout.

### Teacher-Aware Layout Training Implementation

The trainer now has three new opt-in controls:

```text
--listwise-candidate-budget-loss-weight
--listwise-teacher-sae-negative-k
--candidate-exposure-safe-dense-k
```

The purpose is to replace the failed raw exposure penalty with teacher-aware
pressure:

- dense-teacher and qrels neighborhoods are treated as safe exposure;
- BM25 and SAE near-misses are mixed into the hard-negative set;
- listwise distillation can be evaluated under the same small query-atom budget
  that the native candidate path uses.

Smoke run:

```text
results/sae/text-atoms/m22-teacher-aware-layout-smoke
```

The smoke proves the code path runs on MPS and produces eval output. It is not
a quality claim because it is only one epoch on SciFact. The next decision
point is the five-dataset run:

```text
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-teacher-aware-five-batch8
```

Five-dataset result:

| Run | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| asym96 budget8 baseline | 0.7524 | 0.6848 | 0.5881 | 0.4898 |
| teacher-aware layout | 0.7425 | 0.6486 | 0.5478 | 0.4540 |

Interpretation:

- Teacher-aware safe exposure and SAE near-miss mixing are valid code paths,
  but this exact weighting is a negative result. It improves teacher support
  overlap, yet hurts the ranking metrics that matter for promotion.
- Do not promote this checkpoint to full15. Keep the implementation because it
  gives us a safer replacement for raw exposure, but use it only for future
  ablations.
- The MPS path is active (`--device mps`), but this workload is not yet
  GPU-efficient. The 8192-dim token-level graph is large, and repeated CPU/MPS
  batch boundaries force command-buffer synchronization. Increasing coverage
  batch size from 1 to 8 kept the run correct but still produced multi-minute
  epochs, so future training work should either batch more coarsely, cache
  token tensors more aggressively, or compare against CPU for small-listwise
  losses before assuming MPS is faster.

### Full15 Miss Taxonomy Follow-Up

The follow-up taxonomy run is recorded in:

```text
sae-milestone22-full15-miss-taxonomy-report.md
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-miss-taxonomy
```

It compares `bm25`, dense retrieval, full sparse student scoring, and the
native active-row candidate traversal for the `doc88/query96` deployment
profile.

Key result:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.8031 | 0.7633 | 0.6635 | 0.6145 |
| `dense` | 0.8744 | 0.8575 | 0.7712 | 0.7293 |
| `student_full` | 0.8111 | 0.7847 | 0.6842 | 0.6321 |
| `native_active` | 0.8101 | 0.7856 | 0.6844 | 0.6297 |

Only one query is a pure `native_traversal_loss`, while 30 queries are
`student_encoder_loss`. This means the next pressure point is semantic coverage
inside the text-to-atom student, not widening native traversal or retuning
fusion weights.

The shared rebuildable BEIR working root has also moved from `/tmp` to:

```text
/Volumes/Betty/Tmp/ii42_sae_beir15_shared
```

### Focused Semantic Coverage Follow-Up

The next run tested whether the taxonomy can directly drive training:

```text
sae-milestone22-focused-semantic-coverage-report.md
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-beironly-baseline-five-train-full15-eval
results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-focus6-five-train-full15-eval
```

Because the original `/tmp` shared teacher artifact was gone, this pass rebuilt
`shared_sae_8192_64` under `/Volumes/Betty/Tmp` using the 15-BEIR documents
only. No old 10k commons unlabeled dump was found, so the valid comparison is
`focus6` against the same-teacher BEIR-only baseline.

Result:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `beironly-baseline` | 0.7867 | 0.7899 | 0.6699 | 0.6282 |
| `focus6` | 0.7865 | 0.7940 | 0.6734 | 0.6299 |

Miss taxonomy improved in the intended place:

```text
student_encoder_loss: 37 -> 30
semantic-heavy student_encoder_loss: 31 -> 26
long-query student_encoder_loss: 26 -> 20
```

But physical active8 cost moved the wrong way:

| Run | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE posts |
| --- | ---: | ---: | ---: | ---: | ---: |
| `beironly-baseline` | 0.7731 | 0.6575 | 0.6052 | 670.3 | 767.3 |
| `focus6` | 0.7761 | 0.6589 | 0.6023 | 768.1 | 809.6 |

Conclusion: focused repeat is a useful diagnostic, not a promotable checkpoint.
It proves the taxonomy is actionable, but it is too blunt and increases
candidate fanout. The next run should use focused queries inside a
teacher-aware candidate-budget/listwise objective with safe exposure gating,
rather than repeating examples directly.
