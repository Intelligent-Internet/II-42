# M1812 CITADEL Full-Corpus BEIR Contract

## Scope

Run fixed M1811B policies on complete official SciFact and NFCorpus roots. No
dataset-specific tuning, qrels-driven selection, ANN, BM25 candidate source,
or CLS dense score is allowed in the unified-posting rows.

Fixed policies selected before BEIR evaluation:

- quality: query top-1, document top-5, route weight greater than `0.5`, read
  budget `1x documents`, int8 32-dimensional payload;
- compact: query top-1, document top-2, route weight greater than `0.5`, read
  budget `1x documents`, int8 32-dimensional payload.

## Outputs

For each dataset report full qrels NDCG@10, MAP@100, Recall@100, MRR@20,
candidate upper bound, learned-route overlap, touched entries, query bytes,
index bytes, maximum posting length, and index postings per document.

The existing PPLX embedding stored in the official roots is evaluated as a
separate dense quality reference. CITADEL's CLS branch is diagnostic-only.

## Gates

Mechanism authorization requires, on both datasets:

- budgeted direct O@100 against exhaustive learned routing at least `0.90`;
- direct Recall@100 no more than `0.02` below exhaustive learned routing;
- direct NDCG@10, MAP@100, and MRR@20 each no more than `0.02` below exact;
- at most `1 MiB/query` and `10 KiB/document`.

Product-root replacement is a separate, stricter decision. It additionally
requires the quality policy's macro Recall@100 to reach at least `95%` of the
PPLX dense reference without a per-dataset Recall loss greater than `0.05`.

Passing only the mechanism gate authorizes training a P1-root learned router;
it does not authorize shipping the external CC-BY-NC checkpoint.
