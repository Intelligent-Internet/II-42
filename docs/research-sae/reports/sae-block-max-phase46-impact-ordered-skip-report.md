# SAE Block-Max Phase 4.6 Impact-Ordered Skip Report

Date: 2026-05-12

## Purpose

Phase 4.6 tests the next candidate after the negative document-range
super-block result: exact impact-ordered sparse threshold skipping.

The goal is to answer one narrow question before touching the native C reader:
if SAE postings are traversed in descending impact order, can a residual
frontier bound stop much earlier than the flat doc-bound entry scan while
preserving exact top-k?

## Prototype

The new script is:

```text
scripts/research_sae_impact_ordered_skip.py
```

It builds the same native-shaped SAE artifact as the current read-only path,
then evaluates a threshold algorithm:

```text
query dimensions
  -> per-dimension postings sorted by impact descending
  -> global frontier = sum(next impact * query weight)
  -> first-seen document exact random-access score
  -> stop when frontier < visible kth score
```

The implementation is deliberately a simulator. It separates the algorithmic
question from native payload design. Exact top-k parity remains required.

## Full Query Result

Default run: 64 query dimensions, `top_k = 100`.

| Dataset | Exact | Query ms | Entry visits | Doc-bound visits | Visit ratio | Unique docs | Exact score terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 100/100 | 13.624 | 7601.29 | 7761.96 | 0.979 | 1878.53 | 120225.92 |
| `scidocs` | 100/100 | 12.860 | 7168.79 | 7318.08 | 0.980 | 1870.44 | 119689.84 |
| `nfcorpus` | 100/100 | 12.431 | 6516.15 | 6761.80 | 0.964 | 1898.43 | 121071.89 |
| `arguana` | 100/100 | 13.712 | 7483.85 | 7719.65 | 0.969 | 1867.08 | 119493.12 |
| `fiqa` | 100/100 | 11.922 | 6492.08 | 6620.86 | 0.981 | 1804.64 | 115496.96 |

The exact threshold algorithm is correct but weak. It only saves about `2-4%`
of entry visits and still first-sees almost the entire 2k-document corpus.

## Query Fanout Sweep

The active-dimension sweep is exact relative to the truncated query used for
that run. It is not a quality claim against the original 64-dimension query.

| Query dims | Mean visit ratio | Mean entry visits | Mean doc-bound visits | Mean unique docs | Mean query ms |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 0.960 | 1771.1 | 1845.0 | 1045.1 | 2.794 |
| 32 | 0.967 | 3572.4 | 3692.9 | 1517.0 | 6.032 |
| 48 | 0.972 | 5325.7 | 5479.7 | 1745.1 | 9.487 |
| 64 | 0.975 | 7052.4 | 7236.5 | 1863.8 | 12.910 |

Reducing query fanout lowers absolute work, but the impact-ordered residual
frontier still saves only a small fraction relative to the flat scan.

## Decision

Do not port this exact impact-ordered threshold algorithm into the native C
reader as-is. It would add random-access exact scoring requirements and more
payload complexity while saving too few sequential entry visits.

The failure mode is now clear across two physical skip attempts:

- document-range super-blocks fail because their max-impact bounds are too
  loose;
- impact-order threshold fails because many query dimensions keep the residual
  frontier high until most documents have been seen.

This points back to representation and query fanout. A physical traversal layer
cannot recover enough if each query activates broad high-DF latent dimensions.

## Next Direction

The next useful work should combine representation and physical design instead
of adding another exact traversal variant:

- retrieval-aware or asymmetric query encoder that emits fewer, sharper query
  latents without losing recall;
- candidate-budget-aware training objective that penalizes high-DF query fanout;
- approximate high-impact candidate generation followed by exact doc-bound
  rerank, with recall/quality measured explicitly;
- optional row-wise compact doc payload only if candidate exact scoring becomes
  valuable after the representation improves.

For the native index roadmap, keep the current default:

```text
SBMXM001 v1
micro_block_size = 1
doc_super_block_size = 0
doc-bound exact fast path
```

## Verification

Commands run:

```bash
python3 -m py_compile scripts/research_sae_impact_ordered_skip.py

python3 scripts/research_sae_impact_ordered_skip.py \
  --datasets scifact \
  --max-queries 10 \
  --output-dir results/sae/phase46/impact-ordered-skip-smoke

python3 scripts/research_sae_impact_ordered_skip.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --output-dir results/sae/phase46/impact-ordered-skip

for active in 16 32 48; do
  python3 scripts/research_sae_impact_ordered_skip.py \
    --datasets scifact scidocs nfcorpus arguana fiqa \
    --query-active-dims "$active" \
    --output-dir "results/sae/phase46/impact-ordered-active-${active}"
done
```
