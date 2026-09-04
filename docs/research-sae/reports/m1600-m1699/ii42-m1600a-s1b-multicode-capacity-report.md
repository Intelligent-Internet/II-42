# M1600A-S1B Multi-Code Source Capacity Report

## Decision

**Authorize one balanced query-router training stage over a frozen document
source.** The finer multi-code source passes the dense-equivalence oracle gate,
but nearest-code deterministic query routing cannot expose that capacity.

This is the first M1600 result that changes the causal boundary established by
M1560-M1572: a balanced bounded posting source now exists at practical cost.
The unresolved problem is query key ordering.

## Locked Surface

- Training corpus for codebook fitting: 35,831 qrels-free MS MARCO documents.
- Heldout audit: 500 queries and 4,498 disjoint documents.
- Dense root: `BAAI/bge-base-en-v1.5`.
- Posting namespace: 8 groups x 512 keys.
- Document source: one or two nearest keys per group.
- No qrels, BM25, cross-encoder labels, or dataset identity.
- Candidate ranking: exact dense upper after the posting union, used only to
  isolate source admission.

## Deterministic Frontier

| Document keys/group | Query policy | O@100 | O@256 | Reads | Max DF |
| ---: | --- | ---: | ---: | ---: | ---: |
| 1 | g8/p2 | 0.472722 | 0.424773 | 0.044320x | 0.012228 |
| 1 | g8/p4 | 0.619271 | 0.467473 | 0.082516x | 0.012228 |
| 1 | g8/p8 | 0.765580 | 0.602965 | 0.155242x | 0.012228 |
| 2 | g8/p2 | 0.620368 | 0.476218 | 0.086353x | 0.017563 |
| 2 | g8/p4 | 0.765560 | 0.602390 | 0.165724x | 0.017563 |
| 2 | g8/p8 | 0.879460 | 0.758203 | 0.316423x | 0.017563 |

Two document assignments spend almost twice the reads and do not improve the
quality-cost shape enough to justify the extra posting. The continuation keeps
one document key per group.

## Dense-Teacher Source Oracle

| Document keys/group | Read cap | O@100 | O@256 | Actual reads |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0.075x | 0.994520 | 0.662094 | 0.074799x |
| 1 | 0.150x | **1.000000** | **0.967211** | 0.145435x |
| 1 | 0.300x | 1.000000 | 1.000000 | 0.163042x |
| 2 | 0.150x | 1.000000 | 0.887992 | 0.148750x |
| 2 | 0.300x | 1.000000 | 1.000000 | 0.208845x |

The one-key source passes O@100 0.95 and O@256 0.90 below the 0.30x contract,
and nearly saturates the target at 0.15x. The gap to deterministic g8/p8 is
0.234420 O@100 and 0.364246 O@256 at almost identical reads.

## Interpretation

The result separates three hypotheses:

1. **Vocabulary capacity:** passed. The 4,096-key namespace has enough
   selective posting lists; max DF is only 1.23%.
2. **Document assignment:** passed with one key per group. Extra document
   replication is not the main missing mechanism.
3. **Query observability:** failed for nearest-centroid ordering. Useful lists
   are present, but query cosine to individual centroids does not rank them by
   dense-neighborhood coverage per read.

This differs from M1570's terminal product-cell result because M1600 now has a
measured low-cost source oracle and an explicit trainable target: key utility
over dense top256, normalized by posting DF. The next model does not guess an
unobservable qrels action. It distills a deterministic qrels-free teacher from
the dense root into query key scores.

## Authorized S1C

- freeze the document codebook and every document assignment;
- derive per-query key utility from dense top256, with stronger head weight and
  posting-length normalization;
- train only a shared low-rank residual over the existing query key logits;
- use a fixed 0.15x posting-read cap at evaluation;
- select checkpoints only when both heldout O@100 and O@256 improve;
- reject initialization fallback and loss-only improvements.

If the teacher key policy itself cannot approach the measured oracle, repair
the teacher construction before training. If the teacher passes but the
low-rank router cannot transfer to heldout queries, stop this source rather
than increasing depth or adding a post-hoc gate.
