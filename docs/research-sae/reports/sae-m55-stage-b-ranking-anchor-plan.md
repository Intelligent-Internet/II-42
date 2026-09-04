# SAE M55 Stage-B Ranking Anchor Plan

## Summary

M54 established that teacher-shape distillation works, but the full15 sanity
matrix also showed that more Stage-A epochs do not automatically improve
ranking. M55 therefore starts Stage B:

```text
M54 e2-clean balanced checkpoint
-> supervised ranking fine-tuning
-> teacher-shape anchor kept active
-> hard exported atoms evaluated through the existing BM25+SAE sparse ranking
```

The goal is not to continue teacher imitation. The goal is to see whether
supervised ranking can improve NDCG/MAP without losing the semantic atom shape
that M54 recovered.

## Starting Point

Use `M54 e2-clean` as the default starting checkpoint:

```text
checkpoint = /Volumes/Betty/Tmp/ii42_sae_m54/checkpoints/m54-e2-clean-text_atom_student.pt
doc_active = 128
query_active = 96
teacher = shared_sae_8192_64
```

Keep `M54 e4-cont` as a semantic-fidelity control only. It has better support
fidelity but weaker ranking sanity than `e2-clean`.

## Loss Design

M55 uses two groups of loss terms.

### Teacher-shape anchor

These prevent supervised ranking from destroying the Stage-A semantic shape:

```text
support BCE          = 0.55
support margin       = 0.08
value MSE            = 3.00
fanout penalty       = 0.002
safe exposure        = 0.001
```

### Ranking pressure

These terms apply supervised and teacher-neighborhood ranking pressure:

```text
qrels retrieval pairwise       = 0.04
listwise teacher distribution  = 0.08
multi-positive coverage        = 0.10
soft top-k recall surrogate    = 0.10
candidate budget ranking       = 0.08
asymmetric active ranking      = 0.05
```

The ranking weights are deliberately smaller than the anchor. Earlier runs
showed that aggressive ranking pressure can raise Recall while hurting
MRR/NDCG/MAP and robustness.

## Smoke Gate

The first run is a small smoke on `scifact` and `nfcorpus`.

Promote to a larger Stage-B run only if:

- at least one ranking metric improves without broad regression;
- teacher support recall/Jaccard does not materially collapse;
- the best student SAE weight remains in a normal range, preferably `0.25` or
  `0.5`;
- no dataset shows obvious ranking collapse.

If the smoke only improves Recall but hurts MRR/NDCG/MAP, do not scale. The
next adjustment should be ranking-loss calibration, not more epochs.
