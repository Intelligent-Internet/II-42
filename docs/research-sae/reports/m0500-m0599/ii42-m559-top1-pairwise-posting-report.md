# M559 Top1 + Pairwise Posting Encoder Report

M559 tests whether the two best DREAM-lite proxy signals combine cleanly:

- M555 pairwise dense-order loss: weak positive over M551 on broad10, but
  seed-stable gains are small.
- M558 teacher-top1 candidate competition: cleaner NDCG/Recall gain on the
  seed552 three-task smoke, but weaker MAP/MRR than M555.

The hypothesis was that top1 competition would protect top-rank behavior while
pairwise order loss kept the broader candidate-set ordering useful.

## Implementation

M559 uses the shared M551/M558 code path without changing training logic:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m559_top1_pairwise_posting_spark.sh
```

Fixed M559 defaults:

```text
TOP1_WEIGHT=0.10
PAIRWISE_WEIGHT=0.20
UTILITY_WEIGHT=0.0
SELECTION_TOP1_WEIGHT=0.10
SELECTION_PAIRWISE_WEIGHT=0.0
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
EPOCHS=4
```

The surface is BM25-free and qrels-free during training.  Qrels are used only
for held-out evaluation.

## Seed552 Smoke

Seed552 looked promising on the same three-task smoke surface:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

| Model | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M558 | 0.44655 | 0.20243 | 0.43790 | 0.58969 | 0.54875 |
| M559 | 0.44713 | 0.20308 | 0.43830 | 0.59019 | 0.54896 |

Interpretation: M559 was the best balanced seed552 result.  It improved
NDCG/Recall over M555 and M558, nearly tied M555 MAP, and kept MRR between
M558 and M555.  This justified a cheap three-seed check, not broad10
promotion.

## Three-Seed Check

The three-seed check completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m559-top1-pairwise-posting-v1/
```

Rows below are deltas against matching M551 seeds on the same three-task
surface.  M555 is included as the current pairwise baseline.

| Seed | Model | dNDCG | dMAP | dR | dMRR | dO |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | M555 | +0.00047 | -0.00119 | -0.00025 | -0.00197 | -0.00011 |
| 551 | M559 | -0.00074 | -0.00163 | -0.00033 | -0.00218 | +0.00022 |
| 552 | M555 | -0.00040 | +0.00140 | +0.00087 | +0.00158 | +0.00093 |
| 552 | M559 | +0.00067 | +0.00137 | +0.00139 | +0.00078 | +0.00072 |
| 553 | M555 | +0.00224 | -0.00042 | +0.00007 | -0.00019 | -0.00132 |
| 553 | M559 | +0.00181 | -0.00034 | -0.00088 | +0.00062 | -0.00265 |

Three-seed mean deltas against M551:

| Model | dNDCG | dMAP | dR | dMRR | dO |
| --- | ---: | ---: | ---: | ---: | ---: |
| M555 | +0.00077 | -0.00007 | +0.00023 | -0.00019 | -0.00017 |
| M559 | +0.00058 | -0.00020 | +0.00006 | -0.00026 | -0.00057 |

Mean M559 - M555:

| dNDCG | dMAP | dR | dMRR | dO |
| ---: | ---: | ---: | ---: | ---: |
| -0.00019 | -0.00013 | -0.00017 | -0.00007 | -0.00040 |

## Decision

Stop M559 as a promotion route.  The seed552 signal was real but not stable.
Across three seeds on the same smoke surface, M559 is weaker than M555 on every
tracked mean delta.

Do not scale M559 to broad10.  Keep the evidence because it narrows the next
step: the useful DREAM-lite signal is still candidate competition, but simple
top1-plus-pairwise proxy weighting is not a stable improvement over M555.

The next route should not be another local weight sweep.  It should either:

- return to M555 as the current stable weak-positive baseline and improve
  candidate construction/support selection; or
- implement a closer DREAM-style interface diagnostic, where the training
  signal comes from a fixed retrieval-relevant interface rather than generic
  dense-score proxy targets.
