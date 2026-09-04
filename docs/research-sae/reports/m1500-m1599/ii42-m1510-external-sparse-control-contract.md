# M1510 External Sparse Control Contract

## Question

Does a retrieval-trained token-to-sparse model avoid the failure modes seen in
M1020 reconstruction-only pooled SAE and M1401 untrained raw-token MaxSim?

## Artifact Boundary

The SPLARE paper does not publish an official checkpoint. M1510 therefore uses
the independent Apache-2.0 reproduction at
`aswath86/splare-finetune-opensearch-demo`, with Gemma-2-2B, Gemma Scope
layer-18 width-65k L0-116 SAE, and the reproduction's LoRA adapter. The result
must be labeled an independent reproduction, not an official SPLARE score.

The reproduction reports approximately 89k MIRACL and Mr.TyDi multilingual
training rows. `nfcorpus`, `scifact`, and `fiqa` are not listed training
sources, so they are useful OOD controls. The checked-in adapter model card is
incomplete, however, so the rows are not promoted as formally proven
clean-heldout results.

SSR has public code but no public checkpoint as of 2026-07-09. Its absence is
an artifact-availability result, not a model-quality failure.

## Evaluation Surface

- Full official `nfcorpus`, `scifact`, and `fiqa` corpora.
- Document TopK 400 and query TopK 40, matching the SPLARE inference shape.
- Sparse dot product over a single inverted index.
- No qrels in encoding, pruning, indexing, candidate generation, or scoring.
- Compare with native BM25, dense, M549U, and P1-a0125 rows from M603.
- P1 rows are explicitly `seen_regression`; they are not clean-heldout claims.

## Required Outputs

- NDCG@10, MAP@100, Recall@100, MRR@20.
- Candidate upper bound at 1000 and over all posting-touched documents.
- Total postings, active dimensions, max DF ratio, head 1% posting share.
- Posting touches, unique touched documents, and retrieval latency per query.
- Resumable sparse JSONL artifacts with strict source/model/config manifests.

## Gates

Continue the SPLARE-like branch only if at least two independent datasets show
a credible quality/cost frontier and the index does not reproduce the M1020
`max_df`/head-atom collapse. A failed independent reproduction does not by
itself falsify the paper, but it blocks expensive M1512 training until the
artifact gap is resolved.

Continue to M1511 only after all three full-corpus rows and provenance checks
are complete. M1511 labels may diagnose information loss but must not train or
select M1510 representations.
