# M302 Atom-Level Unified Impact Smoke Report

Date: 2026-06-14

## Purpose

M302 tests a deeper BM25+SAE fusion surface than M300/M301.

M300 used aggregate BM25/SAE source evidence. M301 added lexical overlap but
still treated SAE mostly as an aggregate source. M302 computes candidate
features from actual matched evidence in both namespaces:

- BM25 query tokens matched against document text tokens.
- SAE query latent atoms matched against document latent atoms.
- Cross-family agreement and complement mass.

This is still not the final unified posting engine. It remains a
candidate-level residual scorer over existing candidate rows. The purpose is to
answer a narrower question: do real BM25-token and SAE-atom features carry
additional search/ranking signal beyond the existing BM25+SAE score fusion?

Runner:

```text
scripts/research_sae_m302_atom_level_unified_impact.py
```

## Input Surface

Default checkpoint:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1/bm25sae_stageb_best.pt
```

Default official ranking rows:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1-official-full-corpus-gate/all-test
```

Default replay corpus:

```text
/home/huoju/leask/runs/ii42-m190-replay-actual-beir15-pplx1024
```

The runner uses the official gate dataset folders first because those folders
contain the query embeddings used by the evaluated rankings. The replay corpus
is used for document text and fallback artifacts.

## Feature Contract

M302 features are computed from matched tokens/atoms, not from free source
rank/score columns:

| Group | Features |
| --- | --- |
| BM25 token evidence | `bm25_token_score`, `bm25_token_idf_sum`, `bm25_token_overlap`, `bm25_token_coverage`, `bm25_token_max_idf`, `bm25_token_tf_mass` |
| SAE atom evidence | `sae_atom_dot`, `sae_atom_idf_dot`, `sae_atom_overlap`, `sae_query_mass_matched`, `sae_doc_mass_matched`, `sae_atom_max_contribution` |
| Unified/complement evidence | `unified_atom_sum`, `bm25_only_atom_mass`, `sae_only_atom_mass`, `balanced_atom_agreement` |

The learned score is constrained as a residual over the existing BM25+SAE score
fusion:

```text
score = base + alpha * tanh(unified_atom_score - base)
```

This keeps the experiment focused on whether true atom-level evidence can
improve admission or top-rank order without replacing the whole retrieval
pipeline.

## Runs

All runs use the five-dataset smoke surface:

```text
nfcorpus, scifact, arguana, scidocs, fiqa
```

Remote root:

```text
/home/huoju/leask/runs
```

| Run | Preserve | Alpha | Path |
| --- | ---: | ---: | --- |
| M302 p50 | 50 | 0.10 | `ii42-m302-atom-level-unified-5ds-a10p50-v1` |
| M302 p0 | 0 | 0.10 | `ii42-m302-atom-level-unified-5ds-a10p0-v1` |
| M302 a05p0 | 0 | 0.05 | `ii42-m302-atom-level-unified-5ds-a05p0-v1` |

## Macro Results

Baseline is the existing BM25+SAE score-fusion row on the same surface.

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| M300 p50 | +0.004150 | +0.000000 | +0.000000 | +0.000159 | best safe aggregate proxy |
| M301 p50 | +0.002497 | +0.000000 | +0.000000 | +0.000112 | lexical proxy, weaker |
| M302 p50 | +0.000901 | +0.000000 | +0.000000 | +0.000053 | safe, weak atom residual |
| M302 p0 | +0.001373 | +0.000111 | +0.000576 | +0.000508 | top-rank signal, local harm |
| M302 a05p0 | +0.000646 | -0.000316 | +0.000406 | +0.000412 | smaller alpha, not better |

## Per-Dataset Safety

M302 p50 was safe but weak:

```text
nfcorpus: +0.002363 recall@100, +0.000166 MAP@100
scifact:  +0.000000 recall@100, +0.000003 MAP@100
arguana:  +0.002141 recall@100, +0.000023 MAP@100
scidocs:  +0.000000 recall@100, +0.000033 MAP@100
fiqa:     +0.000000 recall@100, +0.000040 MAP@100
```

M302 p0 introduced useful top-rank signal, but it was not no-harm:

```text
nfcorpus: +0.005039 recall@100, +0.001141 MRR@20, +0.001928 NDCG@10, +0.001728 MAP@100
scifact:  +0.000000 recall@100, +0.000229 MRR@20, +0.001110 NDCG@10, +0.000266 MAP@100
arguana:  +0.001428 recall@100, -0.001141 MRR@20, -0.001906 NDCG@10, -0.001048 MAP@100
scidocs:  +0.000400 recall@100, -0.000095 MRR@20, +0.000061 NDCG@10, -0.000010 MAP@100
fiqa:     +0.000000 recall@100, +0.000420 MRR@20, +0.001689 NDCG@10, +0.001606 MAP@100
```

M302 a05p0 reduced some movement but did not improve the tradeoff:

```text
nfcorpus: +0.002516 recall@100, -0.002220 MRR@20, +0.000615 NDCG@10, +0.000990 MAP@100
scifact:  +0.000000 recall@100, +0.000032 MRR@20, +0.000000 NDCG@10, +0.000048 MAP@100
arguana:  +0.000714 recall@100, -0.000424 MRR@20, -0.000110 NDCG@10, -0.000330 MAP@100
scidocs:  +0.000000 recall@100, +0.000000 MRR@20, +0.000098 NDCG@10, +0.000005 MAP@100
fiqa:     +0.000000 recall@100, +0.001034 MRR@20, +0.001426 NDCG@10, +0.001348 MAP@100
```

## Interpretation

M302 confirms that real atom-level evidence has signal, but this formulation is
not yet the right deep-fusion mechanism.

Useful evidence:

- p0 improves macro Recall@100, MRR@20, NDCG@10, and MAP@100 at the same time.
- The model is not only appending tail candidates; it can affect top-rank
  ordering.
- Learned weights consistently use SAE atom mass and matched SAE dot features,
  so the atom surface is active.

Negative evidence:

- p50 is safe but weaker than both M300 and M301.
- p0 has local harm on arguana and tiny harm on scidocs.
- reducing alpha to 0.05 does not create a better Pareto point.
- BM25 token features remain underpowered relative to SAE atom features, which
  suggests the current feature normalization still does not express lexical
  utility deeply enough.

The current M302 scorer is still too shallow because it learns over candidate
features after candidate formation. It does not yet learn an atom-local
reliability table, a true posting traversal upper bound, or direct qrel
win/loss utility for each token/latent atom.

## Decision

Do not expand M302 to 10 datasets in its current form.

The result is valuable as a diagnostic:

- real atom evidence is worth pursuing;
- simple residual feature scoring is not enough;
- hand-tuning alpha/preserve settings is not the correct next step.

## Next Step: M303

The next experiment should move from feature-level residual scoring to true
atom utility/reliability learning.

Proposed M303:

1. Build train-time qrel win/loss stats over actual atom ids.
2. Learn separate reliability for BM25 token atoms and SAE latent atoms.
3. Score candidates by direct atom impact:

```text
score(doc, query) =
    sum(query_atom_weight * doc_atom_impact * reliability(atom))
```

4. Use query-conditioned family scale only as a small calibration layer.
5. Evaluate the same p50 and p0 contracts against M300/M302.

This is closer to the product direction: BM25 token postings and SAE atom
postings become one sparse evidence engine, instead of two source rankings
being mixed after retrieval.

