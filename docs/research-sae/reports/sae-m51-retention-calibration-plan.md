# SAE M51 Retention-Calibrated Hard-Query Plan

Status: planned.

## M50 Evidence

M50 answered the first blocker question:

```text
Is the trec-covid/msmarco regression caused by too little semantic retention?
```

Answer:

```text
partly yes
```

Larger query-atom retention can repair current M46/M49 regressions on both
datasets, but pure retention does not beat BM25 everywhere and is too expensive
as a default.

## New Hypothesis

The remaining blocker is not one scalar issue. It is the interaction of:

```text
semantic retention
+ lexical/BM25 top-rank preservation
+ query-specific semantic scale
```

For large or broad corpora, the model needs enough SAE atoms to keep semantic
neighborhoods available. But once those atoms are available, the scoring path
must not let weaker semantic matches demote BM25-strong top documents unless
the semantic evidence is decisive.

## M51 Strategy

M51 should add a runtime-safe hard-query profile:

```text
default profile:
    M46/M49 fanout_to_m44 selector

semantic-retention rescue profile:
    pool_active_dims   = 96
    export_active_dims = 96
    fanout_power       = 0.00-0.10
    low_sae_weight     = 0.30
    high_sae_weight    = 0.75
```

Then add BM25-preserving calibration to the rescue profile:

```text
score = bm25_norm + sae_scale(query) * sae_norm + lexical_floor(query, doc)
```

The floor should be runtime-safe and dataset-agnostic. Candidate signals:

- BM25 concentration;
- top BM25 score gap;
- query content mean DF;
- query high-DF share;
- predicted SAE postings;
- predicted candidate docs;
- overlap between BM25 candidates and SAE candidates.

## Candidate Calibration Rules

Start with deterministic rules before training:

1. **BM25 floor**:
   - if a document is in the BM25 top-N, add a small preservation bonus;
   - decay the bonus by BM25 rank or normalized score.

2. **Semantic confidence gate**:
   - let SAE override BM25 only when `sae_norm` is high and BM25 confidence is
     low;
   - otherwise use a lower `sae_scale`.

3. **High-fanout damping**:
   - when predicted SAE postings are very high, keep the larger retained atoms
     but reduce their score scale;
   - this separates candidate recall from ranking force.

4. **Overlap-aware scale**:
   - if BM25 and SAE candidate sets overlap strongly, SAE can help rerank;
   - if overlap is low, treat SAE as candidate expansion and preserve BM25
     ordering more strongly.

## Acceptance Gates

M51 is only useful if it improves the target datasets without becoming a
two-dataset overfit.

Target gate:

- `trec-covid` NDCG@10 and MAP@100 must be at least BM25-level or within
  `0.005` of BM25 while beating current M46/M49.
- `msmarco` NDCG@10 and MAP@100 must beat BM25.
- both datasets must beat current M46/M49 on Recall@100.

Full15 gate:

- full15 NDCG@10 and MAP@100 must not regress below M46/M49 by more than
  `0.005`;
- no new hard-dataset collapse on `dbpedia-entity` or `nfcorpus`;
- average SAE postings should be much closer to M46/M49 than to the raw M50
  high-retention diagnostic row.

## Implementation Plan

1. Add a M51 evaluation script that composes:
   - current M46/M49 selector;
   - M50 semantic-retention rescue profile;
   - deterministic BM25 floor / semantic confidence rules.

2. Run first on `trec-covid`, `msmarco`, `dbpedia-entity`, and `nfcorpus`.

3. If target quality improves, rerun full15.

4. Only after deterministic calibration forms a frontier, consider a tiny
   runtime-safe learned calibration head. Do not retrain the encoder first.

## Non-Goals

- Do not route by dataset id.
- Do not introduce multiple encoders.
- Do not freeze SQL/API.
- Do not promote the expensive M50 high-retention profile as a default.
