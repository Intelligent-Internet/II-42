# M305 Soft Safety Gate Report

Date: 2026-06-15

## Purpose

M305 tests whether the M304 atom-feature utility can be made safer without a
hard `preserve_base_top_k` rule.

M304 showed two useful but incomplete behaviors:

- `p0` improved top-rank macro metrics, but caused local harm on `arguana` and
  `scidocs`.
- `p50` was safe, but mostly behaved like tail-only admission and did not move
  top-rank metrics.

M305 replaces the hard preserve rule with a learned soft gate:

```text
final_score =
    base_score
    + residual_alpha
      * soft_gate(query_doc_safety_features)
      * tanh(atom_feature_utility_score - base_score)
```

The intended product shape was a runtime-safe scorer that can use atom utility
when the base BM25+SAE score is uncertain, while avoiding harm to
high-confidence base hits.

Runner:

```text
scripts/research_sae_m305_soft_safety_gate.py
```

## Feature Contract

Atom utility features reuse the M304 serving-safe atom statistics:

- evidence family: BM25 token or SAE latent;
- matched atom impact;
- DF/fanout ratio;
- IDF-like rarity;
- impact and rarity interactions;
- high/mid/rare DF buckets.

The new gate uses only runtime-safe query/document features:

- base, atom, BM25, and SAE reciprocal-rank signals;
- atom score minus base score;
- positive and negative atom margins;
- base top10/top20/top50 flags;
- BM25/SAE agreement and disagreement flags.

No dataset id, qrels, dense embedding, or benchmark-only feature is used at
runtime.

## Runs

All runs use the same five-dataset smoke surface as M304:

```text
nfcorpus, scifact, arguana, scidocs, fiqa
```

Remote run root:

```text
/home/huoju/leask/runs
```

| Run | Path | Key settings |
| --- | --- | --- |
| M305 v1 | `ii42-m305-soft-safety-gate-5ds-v1` | `residual_alpha=0.10`, `gate_bias=-1.25`, `high_base_preserve_k=20` |
| M305 v2 | `ii42-m305-soft-safety-gate-5ds-v2` | `residual_alpha=0.06`, `gate_bias=-1.80`, `high_base_preserve_k=50`, stronger gate/preserve penalties |

## Macro Result

Baseline is the existing BM25+SAE score-fusion row on the same surface.

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| M304 p0 | +0.001009 | +0.000069 | +0.001119 | +0.000824 | best top-rank smoke, local harm |
| M304 p50 | +0.001049 | +0.000000 | +0.000000 | +0.000039 | best safe admission smoke |
| M305 v1 | +0.000137 | -0.000608 | -0.000239 | -0.000105 | failed soft gate |
| M305 v2 | -0.000069 | -0.000556 | -0.000117 | -0.000067 | safer but still worse than base |

## Per-Dataset Result

M305 v1:

```text
nfcorpus: +0.000687 recall@100, -0.002580 MRR@20, -0.000442 NDCG@10, -0.000108 MAP@100
scifact:  +0.000000 recall@100, +0.000021 MRR@20, +0.000000 NDCG@10, +0.000021 MAP@100
arguana:  +0.000000 recall@100, -0.000483 MRR@20, -0.000752 NDCG@10, -0.000444 MAP@100
scidocs:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000000 MAP@100
fiqa:     +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000004 MAP@100
```

M305 v2:

```text
nfcorpus: -0.000345 recall@100, -0.002580 MRR@20, -0.000418 NDCG@10, -0.000138 MAP@100
scifact:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000002 MAP@100
arguana:  +0.000000 recall@100, -0.000201 MRR@20, -0.000168 NDCG@10, -0.000201 MAP@100
scidocs:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000000 MAP@100
fiqa:     +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000002 MAP@100
```

## Interpretation

M305 did not solve the M304 p0 safety problem.

The soft gate learned to reduce movement, but it did not learn a useful
positive/negative intervention boundary:

- v1 still moved enough to hurt `nfcorpus` and `arguana` top-rank metrics.
- v2 reduced movement but remained slightly harmful and lost the M304 p50
  recall gain.
- Neither run approached M304 p0's MAP/NDCG improvement.
- Neither run beat the hard `p50` preserve policy as a safe admission method.

This means the current feature set is not enough to decide when atom utility
should override the base score. The issue is not just preserve strength. The
gate lacks a sharper training signal for true admission wins versus
high-confidence false interventions.

## Decision

Close M305 as a negative result.

Do not expand M305 to 10-dataset or official full-corpus gates. The current
best deep-fusion evidence remains:

- M304 p50 for safe tail admission.
- M304 p0 for top-rank signal with local harm.

The next useful direction should avoid another generic soft gate. It should
train directly on final admission/ranking decisions:

1. Construct explicit win/loss pairs:
   qrel positives admitted only by atom utility versus high-confidence base
   hits or false positives that atom utility would demote/promote.
2. Separate tail admission from head reranking:
   the model should first learn where it is allowed to touch the head.
3. Use query/document type features as constraints, not benchmark-specific
   profiles:
   lexical-heavy, semantic-heavy, high-BM25 concentration, high-SAE entropy,
   and BM25/SAE disagreement.
4. Keep M304 atom-feature utility as the underlying atom abstraction, but
   replace M305's generic safety gate with a supervised admission policy.

