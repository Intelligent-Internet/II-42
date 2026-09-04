# M1610 Retrieval-Aligned Posting Source Contract

## Decision Boundary

M1610 does not tune the closed M1600A low-rank router. It tests whether the
posting source itself can be constructed so that useful query/document key
agreement is locally observable from encoded text.

The product constraint remains strict:

- one encoder checkpoint;
- one unified posting namespace and inverted index;
- no ANN side index, external BM25 source, qrels-time gate, or dataset-specific
  policy;
- query-time keys and impacts must be direct encoder outputs.

Exact dense scoring is allowed only as a source-capacity upper bound before a
source is promoted. It is not a deployable M1610 score.

## Evidence

The local program establishes four constraints:

1. M1500-M1557 repeatedly found useful oracle actions that pooled heads could
   not predict out of sample.
2. M1560-M1572 showed that dense score reconstruction is not the bottleneck;
   bounded dense-neighbour admission is.
3. M1600 showed that a balanced 4,096-key source can contain almost all dense
   top256 at 0.15x reads, but a static query router captures only about 1% of
   the source-oracle gap.
4. More data and more steps did not repair M1600. A new source must change
   local observability rather than capacity or optimization depth.

The arXiv corpus was current through 2026-07-10 when this contract was locked.
The relevant mechanism evidence is:

- EHI (`arXiv:2310.08891`) jointly trains query/document index paths with
  semantic, path-contrastive, and intra-leaf objectives.
- CITADEL (`arXiv:2211.10411`) routes contextual token vectors to shared keys,
  trains query/document router overlap directly, and controls routing load.
- ASI++ (`arXiv:2405.14280`) treats balanced ID utilization and query/document
  matching as a joint constrained objective rather than post-hoc clustering.
- CoRGII (`arXiv:2510.22479`) provides evidence that differentiable discrete
  tokenization, learned impacts, and co-occurrence multiprobing can support an
  inverted-index retrieval surface.

## Branch A: Hierarchical Path-Aligned Source

M1610A is a deliberately bounded EHI-style canary over the existing frozen
BGE dense-root cache:

```text
pooled dense-root vector
    -> shared conditional tree indexer
    -> one hard document leaf
    -> bounded query leaf beam
    -> one leaf posting namespace
```

The source starts from hierarchical spherical k-means. Training uses only
qrels-free dense-neighbour positive and hard-negative pairs. The objective is:

```text
L = L_query_to_positive_path
  + lambda_symmetric * L_document_to_query_path
  + lambda_margin * L_positive_negative_path_margin
  + lambda_balance * L_conditional_child_balance
```

This differs from M1600's independent grouped codes by conditioning each path
decision on its parent and directly matching query/document index paths.

### M1610A Gates

Run 128/64 integration first, then one 4,000/500 heldout canary. At fixed
0.075x and 0.15x read caps, promotion requires:

- the source oracle reaches O@100 >= 0.95 and O@256 >= 0.90;
- a trained checkpoint improves O@100 by at least 0.01 and O@256 by at least
  0.005 over initialization at the same read cap;
- O@10 does not decrease by more than 0.001;
- max DF is at most 0.05;
- a trained checkpoint, not initialization, is selected.

Only a passing canary receives an exact second seed. Only two passing seeds
authorize the unchanged 10,000/1,000 surface.

## Branch B: Contextual Token-Routed Source

M1610B is authorized only if M1610A stops or proves that hierarchy alone is
insufficient. It is a CITADEL-style source canary, not a full multi-vector
product claim:

```text
contextual token states
    -> shared sparse key router
    -> query top-K / document top-L latent keys
    -> bounded posting union
```

The router must use direct query/document contrastive key agreement, hard
negative separation, explicit DF/load balance, and hard Top-K/Top-L routing.
The initial gate measures candidate overlap with an exact dense upper after
the union. A later scalar impact scorer is authorized only after source
admission passes.

The B branch must beat both the matched-cost static lexical-token source and
the M1610A/M1600 source frontier. Any configuration that touches more than 30%
of the corpus, depends on a residual vector payload, or improves only a soft
router loss is rejected.

## Stop Rules

Stop a branch when any of these occurs:

- only its source oracle improves;
- loss or soft path agreement improves without hard O@100/O@256;
- the best checkpoint is initialization;
- a second seed removes the gain;
- larger data does not amplify a canary gain;
- DF balance requires sacrificing heldout overlap;
- a result needs qrels, dataset identity, a post-retrieval dense guard, or
  per-row thresholds;
- the next proposal changes only rank, depth, loss weight, or training steps.

One mechanism repair is allowed per branch, and only after a measured failure
anatomy identifies a missing source property.

## Final Decision

A passing M1610 source must eventually beat the M1565 route2048 frontier
(O@100 0.906235, O@256 0.842683, reads 0.045015x) on a matched official
surface before encoder integration. If neither branch produces a replicated
heldout deterministic gain, close M1610 and retain its tooling as evidence
that the pure single-pass unified-posting constraint remains unresolved.
