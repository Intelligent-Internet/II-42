# SAE Block-Max Phase 3.2 WAND and Impact-Ordered Traversal Report

## Scope

Phase 3.2 tests whether the next physical design should replace tiny block-max
blocks with a postings-native traversal.

Implemented research simulators:

```text
scripts/research_sae_wand_sim.py
scripts/research_sae_impact_ordered_sim.py
```

The goal is not to add another SQL wrapper. The goal is to understand whether a
compact postings traversal can keep the `block_size=1-2` pruning quality while
avoiding the large block-entry metadata cost.

## Baseline From Phase 3.1

On the five prepared benchmark artifacts:

| Dataset | Block-2 opened docs | Block-4 opened docs |
| --- | ---: | ---: |
| `scifact` | 0.252 | 0.729 |
| `scidocs` | 0.238 | 0.734 |
| `nfcorpus` | 0.207 | 0.623 |
| `arguana` | 0.180 | 0.559 |
| `fiqa` | 0.203 | 0.624 |

This means the target for the next physical layout is clear:

```text
keep block-2-like pruning,
but avoid block-1/block-2 block-entry metadata explosion.
```

## Document-Level WAND Prototype

Command:

```bash
python3 scripts/research_sae_wand_sim.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/sae_phase32_wand_all_exact \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --top-k 100 \
  --layouts sae_tree,simhash,natural \
  --score-mode normalized_idf_dot \
  --latent-dims 8192 \
  --active-dims 64
```

Best exact `sae_tree` runs:

| Dataset | Exact | Scored docs | WAND bytes | Block-1 bytes |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | 100/100 | 0.429 | 1,064,624 | 6,184,000 |
| `scidocs` | 100/100 | 0.422 | 1,067,152 | 6,184,000 |
| `nfcorpus` | 100/100 | 0.469 | 1,097,176 | 6,378,796 |
| `arguana` | 100/100 | 0.410 | 1,063,872 | 6,184,000 |
| `fiqa` | 100/100 | 0.395 | 1,069,728 | 6,184,000 |

Interpretation:

- WAND-style postings are much more compact than `block_size=1` block-max.
- The candidate pruning is materially worse than `block_size=2`.
- Layout still matters; the exact, stable path in this run is `sae_tree`.

So plain document-level WAND is not the final answer. It is useful as a compact
posting representation, not as the whole pruning strategy.

## Impact-Ordered Threshold Prototype

Command:

```bash
python3 scripts/research_sae_impact_ordered_sim.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/sae_phase32_impact_all \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --top-k 100 \
  --score-mode normalized_idf_dot \
  --latent-dims 8192 \
  --active-dims 64 \
  --check-interval 64
```

Results:

| Dataset | Exact | Touched docs | Postings touched |
| --- | ---: | ---: | ---: |
| `scifact` | 100/100 | 0.945 | 7,762.0 |
| `scidocs` | 100/100 | 0.941 | 7,318.1 |
| `nfcorpus` | 100/100 | 0.929 | 6,761.8 |
| `arguana` | 100/100 | 0.940 | 7,719.6 |
| `fiqa` | 100/100 | 0.909 | 6,620.9 |

Interpretation:

- Impact-ordered traversal reads relatively few postings.
- But the high-impact postings are spread across most documents.
- It therefore touches nearly the whole corpus and is not a good RAG candidate
  filter by itself.

This is a useful negative result: impact ordering helps posting I/O, but not
document candidate concentration.

## Combined Conclusion

The next physical design should not be:

```text
block_size=1 block-max only
plain document-level WAND only
impact-ordered threshold only
```

The best direction is a hybrid physical layout:

```text
logical micro-blocks of 2 docs for safe bound pruning
+ compact postings-style storage to avoid block-entry explosion
+ optional super-blocks only for I/O grouping
```

In other words:

- pruning unit: micro-block, likely size `2`;
- storage unit: compressed postings / super-block pages;
- traversal: block-bound first, then score exact docs inside opened micro-blocks;
- impact ordering: secondary optimization inside pages, not the primary
  candidate-generation mechanism.

## Next Step

Phase 3.3 should prototype a packed micro-block generation:

- `micro_block_size=2`;
- one compact block-entry range per dimension;
- block entries store `block_id`, `max_impact`, and posting slice using delta or
  varint-friendly integers;
- postings store `doc_delta` within the micro-block plus quantized impact;
- query returns the same TID/score/diagnostics contract as Phase 3.1.

The success criterion should be:

```text
opened docs close to block-2 simulator,
metadata closer to WAND postings than block-size-1 block-max,
exact top-k parity on all five artifacts.
```
