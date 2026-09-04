# M304 Atom Feature Utility Report

Date: 2026-06-15

## Purpose

M304 continues the deep BM25+SAE fusion line after M302 and M303.

M303 trained per-atom-id reliability. That was too local: held-out datasets
contain unseen BM25 token atoms and shifted SAE atom distributions, so the
model mostly learned a broad family-scale correction instead of robust
atom-local utility.

M304 changes the abstraction. It does not use atom id lookup. Instead, it
trains a reusable utility function over serving-time atom features:

- evidence family: BM25 token or SAE latent;
- matched atom impact;
- DF/fanout ratio;
- IDF-like rarity;
- impact and rarity interactions;
- high/mid/rare DF buckets.

This is closer to a productable unified posting engine because the same utility
function can score unseen token/latent atoms from their index statistics and
matched contribution.

Runner:

```text
scripts/research_sae_m304_atom_feature_utility_train.py
```

## Runs

All runs use the same five-dataset smoke surface:

```text
nfcorpus, scifact, arguana, scidocs, fiqa
```

Remote root:

```text
/home/huoju/leask/runs
```

| Run | Preserve | Alpha | Path |
| --- | ---: | ---: | --- |
| M304 p0 | 0 | 0.10 | `ii42-m304-atom-feature-utility-5ds-a10p0-v2` |
| M304 p50 | 50 | 0.10 | `ii42-m304-atom-feature-utility-5ds-a10p50-v1` |

The first `p0` attempt failed due to a row-wise minmax implementation bug. The
fixed result is `v2`.

## Macro Results

Baseline is the existing BM25+SAE score-fusion row on the same surface.

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| M302 p50 | +0.000901 | +0.000000 | +0.000000 | +0.000053 | safe atom-feature residual |
| M302 p0 | +0.001373 | +0.000111 | +0.000576 | +0.000508 | top-rank signal, local harm |
| M303 p50 | +0.000344 | +0.000000 | +0.000000 | +0.000003 | safe but weak per-id reliability |
| M303 p0 | +0.000384 | -0.000092 | +0.000420 | +0.000167 | trained per-id reliability, still weak |
| M304 p50 | +0.001049 | +0.000000 | +0.000000 | +0.000039 | best safe admission smoke |
| M304 p0 | +0.001009 | +0.000069 | +0.001119 | +0.000824 | best top-rank smoke, local harm |

## Per-Dataset Result

M304 p0:

```text
nfcorpus: +0.005045 recall@100, +0.000455 MRR@20, +0.002602 NDCG@10, +0.001160 MAP@100
scifact:  +0.000000 recall@100, +0.002655 MRR@20, +0.002259 NDCG@10, +0.002649 MAP@100
arguana:  +0.000000 recall@100, -0.000669 MRR@20, -0.000426 NDCG@10, -0.000581 MAP@100
scidocs:  +0.000000 recall@100, -0.002416 MRR@20, -0.000625 NDCG@10, -0.000488 MAP@100
fiqa:     +0.000000 recall@100, +0.000321 MRR@20, +0.001785 NDCG@10, +0.001380 MAP@100
```

M304 p50:

```text
nfcorpus: +0.005045 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000207 MAP@100
scifact:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000001 MAP@100
arguana:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000000 MAP@100
scidocs:  +0.000200 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000001 MAP@100
fiqa:     +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000011 MAP@100
```

## Interpretation

M304 is the first deep-fusion variant that improves both sides of the earlier
split:

- p50 is the best safe admission smoke among M302/M303/M304.
- p0 gives the strongest macro top-rank metrics among the atom-level variants.
- The feature utility abstraction is more plausible than per-id reliability
  because it can generalize to unseen tokens and latent atoms.

The result is still not product-ready:

- p0 still has local top-rank harm on arguana and scidocs.
- p50 is safe, but it only affects tail admission and leaves top-rank unchanged.
- The current training surface is still candidate-level, not a true posting
  traversal objective.
- Runtime cost is higher than M302 because each matched atom goes through an
  MLP; a product route would need to compile this into bucketed impacts or
  precomputed per-atom feature tables.

## Decision

M304 should replace M303 as the active deep-fusion direction.

Do not continue per-id reliability lookup. The next meaningful line is to keep
the M304 feature-utility abstraction and make the training objective closer to
final search:

1. Train on final admission win/loss: qrel positives missed by base versus
   high-scoring false positives admitted by BM25/SAE.
2. Add query-level safety features so p0 can avoid the arguana/scidocs harm
   without preserving a hard top50.
3. Compile feature utility into deterministic buckets for serving:
   family, DF bucket, impact bucket, and rarity bucket.
4. Re-run 10-dataset gate only after p50 or a soft-preserve variant passes the
   five-dataset safety gate with non-trivial gain.

## Next Step: M305

M305 should test a soft safety gate instead of hard `preserve_base_top_k`.

The desired product shape is:

```text
final_score = base_score + gated_atom_utility_delta
```

where the gate is learned from query/document safety features and is penalized
for hurting high-confidence base hits. This should preserve top-rank safety
without forcing a fixed top50 head.

