# M1520C URSI Codebook Capacity

Decision: **stop this representation source at M1520C**. Do not start M1521
text transcodability or any later URSI training stage from this codebook.

## Question

M1520C tested whether frozen token states from a retrieval-pretrained BGE-base
encoder could be converted, without qrels or retrieval training, into a 32K
corpus vocabulary with a non-indexed background channel. The locked surface
used document K=96, query K=24, non-negative impacts, and no routes.

The gate required at least two of NFCorpus, SciFact, and FiQA to satisfy all
of the following:

- exact mean touched-document ratio <=30%;
- unified candidate upper bound >=97% of the stronger BM25/dense reference;
- recovery of >=90% of dense-recoverable BM25 misses;
- no universal indexed key or indexed background mass.

## Locked Artifact

The qrels-free codebook was fit from MS MARCO documents using BGE-base token
states. Global mean and eight background components were removed, common and
low-residual tokens were assigned to a non-indexed background channel, and
the remaining vectors were clustered hierarchically.

The first fixed `256 x 128` implementation exposed a real construction error:
one coarse tail contained only 38 observations and could not support 128
distinct fine clusters. The corrected implementation retained exactly 32K
concepts but allocated 9-805 fine concepts per coarse bucket in proportion to
its population. This is a construction repair, not a parameter search.

| Field | Value |
| --- | ---: |
| Source documents used | 3,301 |
| Token states collected | 262,144 |
| Semantic token states | 138,167 |
| Semantic token ratio | 0.527065 |
| Concepts | 32,768 |
| Coarse empty clusters | 0 |
| Fine empty clusters | 0 |
| Codebook bytes | 50,758,897 |
| SHA-256 | `e699d795a78c8fe7b117fac1027684d80202f730528f93424306b7de77de550d` |

The binary remains a run artifact on spark-1. Its JSON manifest, exact
surfaces, encoder manifests, logs, and gate output are mirrored under
`runs/m1520c_ursi_codebook_capacity_v1/`.

## Exact Capacity Result

The evaluation uses the locked shared canary corpora: 2,063 NFCorpus
documents, 2,000 SciFact documents, 2,000 FiQA documents, and 100 queries per
dataset. Native BM25 candidates were exported from the matching
`ii42_shared15` relations. A separate full-corpus export was not used because
mixing it with the shared canary would invalidate candidate upper bounds.

| Dataset | Gate | Touch mean | Touch p95 | Max DF | BM25 CUB | Dense CUB | Unified CUB | Dense-miss recovery |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | fail | 0.057799 | 0.258871 | 0.325254 | 0.308015 | 0.355160 | 0.359612 | 86/624 = 0.137821 |
| SciFact | fail | 0.138780 | 0.370175 | 0.265000 | 0.991379 | 1.000000 | 0.991379 | 0/1 = 0.000000 |
| FiQA | fail | 0.115365 | 0.235300 | 0.169000 | 0.928839 | 0.977528 | 0.955056 | 6/17 = 0.352941 |

All three rows passed the mean-touch, unified-CUB, background, and universal
DF checks. All three failed the independently locked 90% residual-recovery
gate, so the required result is 0/3 rather than 2/3.

Semantic-only retrieval also remained weak. Its Recall@100 was 0.096119 on
NFCorpus, 0.636667 on SciFact, and 0.446631 on FiQA. These values are
diagnostic only; semantic-only quality was not substituted for the residual
capacity gate.

## Interpretation

The experiment separates cost capacity from retrieval alignment:

- The 32K vocabulary is sparse enough. Mean exact touch is 5.8-13.9%, and no
  row is blocked by the 30% mean-touch budget.
- The source has some useful semantic signal. It adds enough non-BM25
  positives to keep the unified CUB near or above the stronger reference.
- It does not preserve the particular query-document alignment supplied by
  dense retrieval. The strongest evidence is NFCorpus, where only 86 of 624
  dense-recoverable BM25 misses are present in the semantic candidate set.
- BGE's retrieval objective acts primarily on the pooled embedding. Its token
  states are not guaranteed to form a query/document-shared vocabulary.
  Unsupervised residual quantization therefore cannot be assumed to transfer
  pooled dense geometry into posting membership.

This is a representation-source failure, not evidence that the vocabulary is
too small, that routing is required, or that the head needs more epochs.
Routing would only partition the same misaligned source. M1521 would train a
text head to imitate a capacity target that already failed.

## Stop Decision

Per the M1520 contract, no threshold grid, K adjustment, contextual routing,
or retrieval training is authorized after this oracle-capacity failure.
M1521-M1526 are closed for this source.

The reusable outputs are the exact measurement contract, native BM25 export,
background-aware 32K construction code, variable hierarchical allocation,
empty-document publisher behavior, and the negative source diagnosis. A
future attempt must change the representation source so that shared posting
membership is learned from paired retrieval evidence; it would be a new
route and goal, not a continuation of this M1520C artifact.

## Truth Boundary

- Codebook fit and all document/query postings were locked without qrels.
- Canary qrels were read only by the exact evaluator after artifact lock.
- The three canaries were evaluated once; no result changed K, vocabulary,
  background threshold, or source selection.
- Empty documents emit zero semantic postings and remain in the index
  lifecycle; empty queries remain invalid.
