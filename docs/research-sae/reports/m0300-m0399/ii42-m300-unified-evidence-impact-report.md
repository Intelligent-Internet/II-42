# M300 Unified Evidence Impact Smoke Report

Date: 2026-06-14

## Purpose

M300 tests a stronger form of BM25+SAE integration than late score fusion.
Instead of treating BM25 and SAE as two independently ranked sources and
mixing scores after retrieval, this smoke treats BM25 and SAE matches as one
unified evidence surface.

The current implementation is intentionally a first-step proxy:

- frozen encoder and frozen candidate rows;
- no direct token-id or SAE-atom-id posting rows yet;
- model inputs restricted to aggregate BM25/SAE evidence features and direct
  overlap/complement features;
- late-fusion/scorer columns are not used as model inputs;
- existing BM25+SAE score fusion is used only as a distillation/preserve
  reference.

Runner:

```text
scripts/research_sae_m300_unified_evidence_impact.py
```

## Feature Contract

The model sees only these feature groups from the existing M218 row surface:

| Group | Meaning |
| --- | --- |
| `bm25_*` | lexical evidence proxy |
| `sae_*` | semantic evidence proxy |
| `bm25_sae_overlap` | lexical and semantic agreement |
| `bm25_only_mass` | lexical-only evidence |
| `sae_only_mass` | semantic-only evidence |

It does not see:

- M212 source scorer score/rank features;
- late-fusion score/rank features as model input.

This makes the smoke closer to a unified posting impact model than the earlier
free source scorers.

## Runs

All runs use the 10-dataset M190/M212 LODO surface unless noted.

Remote root:

```text
/home/huoju/leask/runs
```

| Run | Datasets | Preserve | Alpha | Result |
| --- | ---: | ---: | ---: | --- |
| `ii42-m300-unified-evidence-impact-5ds-a10p80-v1` | 5 | 80 | 0.10 | safe tail recall gain |
| `ii42-m300-unified-evidence-impact-10ds-a10p80-v1` | 10 | 80 | 0.10 | safe tail recall gain |
| `ii42-m300-unified-evidence-impact-5ds-a10p50-v1` | 5 | 50 | 0.10 | stronger safe tail recall gain |
| `ii42-m300-unified-evidence-impact-10ds-a10p50-v1` | 10 | 50 | 0.10 | best no-harm admission result |
| `ii42-m300-unified-evidence-impact-5ds-a10p0-v1` | 5 | 0 | 0.10 | top-rank signal with small MRR harm |
| `ii42-m300-unified-evidence-impact-10ds-a10p0-v1` | 10 | 0 | 0.10 | macro top-rank gain, unsafe local harm |
| `ii42-m300-unified-evidence-impact-10ds-a05p0-v1` | 10 | 0 | 0.05 | safer but still tiny local harm |

## 10-Dataset Macro Results

Baseline is the existing BM25+SAE score-fusion row on the same surface.

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| `a10p80` | +0.001900 | +0.000000 | +0.000000 | +0.000098 | safe admission |
| `a10p50` | +0.002649 | +0.000000 | +0.000000 | +0.000180 | best safe admission |
| `a10p0` | +0.000406 | +0.000384 | +0.001489 | +0.000894 | promising but unsafe |
| `a05p0` | +0.000092 | +0.000298 | +0.000765 | +0.000511 | less harmful, still not no-harm |

## M301 Lexical-SAE Proxy Follow-Up

M301 tested the next simplest version of the same idea:

- keep the same preserved-head residual scoring contract;
- add true BM25-side lexical evidence from document/query token overlap,
  query-token IDF, rare-token overlap, and lexical/SAE agreement;
- keep SAE as aggregate ranking evidence, because the current M190 replay
  root does not expose stable per-document SAE atom rows.

Runner:

```text
scripts/research_sae_m301_lexical_sae_unified_impact.py
```

Run:

```text
/home/huoju/leask/runs/ii42-m301-lexical-sae-unified-5ds-a10p50-v1
```

Five-dataset macro result versus the same BM25+SAE score-fusion baseline:

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| `m301-5ds-a10p50` | +0.002497 | +0.000000 | +0.000000 | +0.000112 | safe, weaker than M300 |

Per-dataset deltas were non-negative:

```text
nfcorpus: +0.003475 recall@100, +0.000219 MAP@100
scifact:  +0.006667 recall@100, +0.000062 MAP@100
arguana:  +0.001428 recall@100, +0.000017 MAP@100
scidocs:  +0.000400 recall@100, +0.000122 MAP@100
fiqa:     +0.000514 recall@100, +0.000142 MAP@100
```

This is safe but weaker than M300's corresponding 5-dataset `a10p50` run:

```text
M300 5ds a10p50: +0.004150 recall@100, +0.000159 MAP@100
M301 5ds a10p50: +0.002497 recall@100, +0.000112 MAP@100
```

Decision: do not expand M301 to the 10-dataset gate. The result is useful as a
negative control: adding lexical overlap features on top of aggregate SAE
ranking evidence does not beat the cleaner aggregate M300 proxy. The next
iteration should build real BM25-token and SAE-atom posting rows instead of
adding more lexical overlap proxies.

## No-Harm Check

`a10p50` had no negative per-dataset delta in Recall@100, MRR@20, NDCG@10, or
MAP@100 on the 10-dataset surface:

```text
min Recall@100 delta = 0.000000
min MRR@20 delta     = 0.000000
min NDCG@10 delta    = 0.000000
min MAP@100 delta    = 0.000021
```

`a10p0` improved macro top-rank metrics but introduced local harm:

```text
arguana:          NDCG@10 -0.000208
cqadupstack:      MRR@20  -0.000002
nfcorpus:         MRR@20  -0.001311
webis-touche2020: NDCG@10 -0.001363
```

`a05p0` reduced harm but did not eliminate it:

```text
arguana:   NDCG@10 -0.000010
cqadupstack: MRR@20 -0.000048
scidocs:     MRR@20 -0.000051
```

## Interpretation

The hypothesis has a real signal:

- a unified BM25/SAE evidence impact model can safely admit additional
  relevant documents after the preserved head;
- unlike the recent free calibrator/scorer runs, the `a10p50` setting had no
  negative 10-dataset deltas;
- unrestricted unified scoring (`preserve=0`) can improve macro MRR/NDCG/MAP,
  which means the model is not merely a recall-only tail append.

But the current version is not yet a product scorer:

- `preserve=0` still has small but real dataset-level harm;
- current rows are aggregate evidence proxies, not real BM25-token and
  SAE-atom posting rows;
- the top-rank improvement is too small to justify replacing the existing
  score fusion directly.

## Decision

Promote M300 only as a research direction, not a final ranking policy.

Current best safe setting:

```text
ii42-m300-unified-evidence-impact-10ds-a10p50-v1
```

Current best exploratory scorer setting:

```text
ii42-m300-unified-evidence-impact-10ds-a10p0-v1
```

The correct next step is not more late-fusion tuning. The next useful M300
iteration should build real unified atom rows:

1. BM25 token atom id, query impact, doc impact, df/idf, field/source stats.
2. SAE latent atom id, query impact, doc impact, df, fanout, utility stats.
3. Optional token-latent overlap atoms only when they reduce fanout or improve
   qrel admission.
4. A constrained scorer that learns atom utility and query-conditioned
   evidence scaling.
5. A no-harm selection gate that prevents unrestricted top-rank replacement
   unless it passes per-family safety.

## Practical Product Implication

For the current product route, the safe conclusion remains:

- use BM25+SAE as a candidate/admission layer;
- preserve the stable head or send candidates to reranker;
- do not let this first M300 scorer own final top-rank ordering yet.

For the research route, this is the strongest evidence so far that deeper
BM25-token/SAE-atom unification is worth pursuing.
