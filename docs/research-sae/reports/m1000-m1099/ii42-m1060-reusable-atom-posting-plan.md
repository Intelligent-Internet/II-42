# M1060 Reusable Atom Posting Plan

Date: 2026-06-17

## Goal

M1050 proved that direct atom-posting training can keep fanout low across
multiple datasets, but atom-only heldout retrieval remains weak. The current
failure mode is not high df anymore; it is over-narrow low-df atoms that behave
like query/task-specific discriminators instead of reusable semantic posting
terms.

M1060 keeps the same high-level direction:

```text
text -> token hidden states -> atom posting encoder -> BM25/IDF posting terms
```

but changes the loss so that useful atoms must be reused across documents and
queries.

## What Changes From M1050

M1050 optimized:

- positive above BM25 hard negative;
- low background overlap;
- low fanout.

That can produce atoms with excellent df/fanout statistics but poor heldout
coverage.

M1060 adds a reuse band:

```text
for atoms selected by queries:
    df must not be too high
    df must not be too low
```

This is not dataset-specific. It uses runtime-safe posting statistics inside
the training batch:

- positives;
- BM25 hard negatives;
- random background documents.

The intended effect is to prevent query-specific atoms while still avoiding
whole-corpus head atoms.

## M1060.0 Canary

Training datasets:

- `nfcorpus`;
- `scifact`;
- `fiqa`.

Initial eval datasets:

- `nfcorpus`;
- `scifact`.

Reason: `fiqa` full token-hidden eval takes much longer. We use it in training
because its documents are important for broad atom reuse, but defer full eval
until the small eval surface has a positive signal.

## Pass/Fail

Pass signal:

- atom-only heldout improves over M1050 on `nfcorpus/scifact`;
- unified low-scale heldout improves without hurting rank metrics;
- df remains bounded and avg postings remain far below BM25.

Fail signal:

- reuse band increases df/fanout but does not improve atom-only heldout;
- unified quality reverts to lexical BM25 or gets worse;
- train/heldout gap remains large.

If M1060.0 fails, the next route should add a semantic teacher neighborhood,
not more df-band tuning.

## Current Status

Implementation:

- `scripts/research_sae_m1060_reusable_atom_posting.py`
- `scripts/run_m1060_reusable_atom_posting_spark.sh`

Execution target:

- first attempted on `spark-2`, then moved to ASA to avoid interfering with
  active mainline jobs on `spark-2`;
- ASA output roots:
  - `/home/leask/dev/runs/ii42-m1060-reusable-atom-posting-v1`
  - `/home/leask/dev/runs/ii42-m1060-reusable-atom-posting-s1050-v1`

Local copied artifacts:

- `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1060-asa/`
- `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1060-s1050-asa/`

## Results

Two runs were executed:

1. `seed=1060`, the native M1060 default.
2. `seed=1050`, a fair-split run aligned with M1050's default split.

The `seed=1060` run showed better `scifact` heldout, but lexical BM25
heldout also changed because the split differed from M1050. That result is
useful as a smoke test only, not as a model comparison.

The fair-split `seed=1050` run is the reliable comparison:

| Dataset | Surface | M1050 R@100 | M1060 R@100 | M1050 NDCG@10 | M1060 NDCG@10 | M1050 MAP@100 | M1060 MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | atom-only | 0.0705 | 0.0900 | 0.0633 | 0.0731 | 0.0277 | 0.0191 |
| nfcorpus | unified 0.5 | 0.2611 | 0.2638 | 0.2872 | 0.2996 | 0.1301 | 0.1381 |
| scifact | atom-only | 0.4689 | 0.4611 | 0.2837 | 0.2790 | 0.2748 | 0.2569 |
| scifact | unified 0.5 | 0.8552 | 0.8352 | 0.6240 | 0.6091 | 0.5938 | 0.5760 |

Cost/df diagnostics:

| Dataset | Model | Atom postings/query | Touched docs/query | Max df ratio |
| --- | --- | ---: | ---: | ---: |
| nfcorpus | M1050 | 164.5 | 160.9 | 0.1519 |
| nfcorpus | M1060 seed1050 | 697.0 | 650.3 | 0.4822 |
| scifact | M1050 | 270.3 | 246.8 | 0.2126 |
| scifact | M1060 seed1050 | 166.5 | 157.1 | 0.2402 |

Training signal was healthy but not sufficient:

```text
reuse epoch 1/8 loss=1.987024 rank=0.687818 reuse=2.740163 fanout=25.889025
reuse epoch 8/8 loss=0.242509 rank=0.219380 reuse=0.000861 fanout=0.152480
```

## Decision

M1060 is not promoted.

The reuse-band objective can make atoms more reusable on a narrow dataset
(`nfcorpus` heldout improves slightly), but it is not robust:

- `scifact` degrades on both atom-only and unified scoring;
- `nfcorpus` improvement comes with much higher atom posting fanout and a high
  max-df atom (`0.4822`);
- all-query ranking metrics show the same pattern: recall/admission may move
  slightly, but ranking utility is weaker than M1050.

This means the failure is not just "atoms are too rare". Reuse constraints
alone push atoms toward broader terms but do not teach them which broader
semantic posting terms are useful for retrieval ranking.

Next route:

- keep M1050 as the better baseline for low-fanout posting atoms;
- add semantic teacher neighborhood or final admission/ranking supervision;
- avoid more blind df-band tuning unless paired with a retrieval utility loss.
