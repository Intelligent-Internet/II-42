# M303 Atom Reliability Training Report

Date: 2026-06-14

## Purpose

M303 tests the next step after M302: instead of learning a residual scorer over
aggregate atom-derived features, it trains reliability deltas for actual BM25
token atoms and SAE latent atoms.

The intended product shape is:

```text
score(doc, query) =
    sum(query_atom_weight * doc_atom_impact * reliability(atom))
```

The implementation keeps a raw atom-impact score for every matched atom and
learns bounded deltas:

- per-atom reliability delta;
- family-level BM25/SAE delta;
- residual score against the existing BM25+SAE score-fusion baseline.

Unseen held-out atoms keep their raw impact instead of being dropped. This is
important because LODO held-out datasets can have unseen BM25 tokens and SAE
atoms.

Runner:

```text
scripts/research_sae_m303_atom_reliability_train.py
```

## Runs

All runs use the same five-dataset smoke surface as M302:

```text
nfcorpus, scifact, arguana, scidocs, fiqa
```

Remote root:

```text
/home/huoju/leask/runs
```

| Run | Preserve | Alpha | Training | Path |
| --- | ---: | ---: | --- | --- |
| M303 rawp0 | 0 | 0.10 | no training, raw atom impact only | `ii42-m303-atom-reliability-5ds-rawp0-v1` |
| M303 p0 | 0 | 0.10 | trained per-atom reliability | `ii42-m303-atom-reliability-5ds-a10p0-v1` |
| M303 p50 | 50 | 0.10 | trained per-atom reliability | `ii42-m303-atom-reliability-5ds-a10p50-v1` |

## Macro Results

Baseline is the existing BM25+SAE score-fusion row on the same surface.

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| M302 p50 | +0.000901 | +0.000000 | +0.000000 | +0.000053 | safe atom-feature residual |
| M302 p0 | +0.001373 | +0.000111 | +0.000576 | +0.000508 | useful top-rank signal, local harm |
| M303 rawp0 | +0.000313 | -0.000302 | +0.000131 | -0.000083 | raw atom impact is not enough |
| M303 p0 | +0.000384 | -0.000092 | +0.000420 | +0.000167 | training helps raw, still weaker than M302 |
| M303 p50 | +0.000344 | +0.000000 | +0.000000 | +0.000003 | safe but weaker than M302 p50 |

## Per-Dataset Result

M303 p0:

```text
nfcorpus: +0.001521 recall@100, +0.003685 MRR@20, +0.004183 NDCG@10, +0.001783 MAP@100
scifact:  +0.000000 recall@100, +0.001242 MRR@20, +0.000215 NDCG@10, +0.001192 MAP@100
arguana:  +0.000000 recall@100, -0.000320 MRR@20, -0.000550 NDCG@10, -0.000230 MAP@100
scidocs:  +0.000400 recall@100, -0.003989 MRR@20, -0.001124 NDCG@10, -0.001068 MAP@100
fiqa:     +0.000000 recall@100, -0.001079 MRR@20, -0.000623 NDCG@10, -0.000841 MAP@100
```

M303 p50:

```text
nfcorpus: +0.001521 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000112 MAP@100
scifact:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000003 MAP@100
arguana:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000000 MAP@100
scidocs:  +0.000200 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000036 MAP@100
fiqa:     +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000057 MAP@100
```

## Interpretation

M303 gives two useful signals.

First, raw unified atom impact is not enough. It helps nfcorpus/scifact but
hurts scidocs/fiqa. This means direct matched-atom mass is not a stable ranking
replacement by itself.

Second, training per-atom reliability improves over raw, but still does not
beat M302. The trained folds repeatedly learn:

```text
BM25 family delta: negative
SAE family delta:  positive
```

This suggests the model is not learning robust token-level utility. Instead, it
mostly learns a broad family-scale correction. That is not enough for a
product-grade unified posting engine.

The likely reason is generalization. BM25 token ids are highly local to the
training datasets, while held-out datasets contain many unseen lexical atoms.
SAE atom ids are more reusable, but per-id reliability still overfits some
dataset/query styles. Keeping unseen atoms at raw impact avoids catastrophic
zeroing, but it does not produce a strong learned utility model.

## Decision

Do not expand M303 per-id reliability to 10 datasets.

The direction remains correct at the engine-design level, but this exact
training abstraction is not strong enough:

- M303 p0 is weaker than M302 p0.
- M303 p50 is weaker than M302 p50.
- The family-scale behavior shows the model is not yet learning useful
  atom-local semantics.

## Next Step

The next useful experiment should not continue per-id reliability tuning.

M304 should train reliability from atom features and qrel win/loss surfaces,
not from atom id alone:

1. BM25 token utility should use reusable features: IDF bucket, field/source
   strength, query coverage, term rarity, and BM25 false-positive rate.
2. SAE atom utility should use reusable features: DF/fanout bucket, contribution
   mass, dense-miss recovery rate, high-DF noise rate, and query/document mass.
3. Training labels should be final admission/ranking win/loss:
   qrel-positive recovered documents versus high-scoring false positives.
4. The learned utility should be applied during unified posting scoring, not as
   a late source-score mixture.

In short: train atom utility as a generalizable posting-impact function, not as
a per-dataset atom-id lookup table.

