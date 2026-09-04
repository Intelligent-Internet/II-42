# SAE Unified Impact Next-Phase Report

Date: 2026-05-11

## Objective

This phase tests a more aggressive hypothesis than ordinary hybrid retrieval:

```text
Can BM25 + SAE latent sparse impacts replace dense vector retrieval as the
main semantic recall path, and can the result become one fast boundable
physical index?
```

The work intentionally stays at the index layer. It does not add external
business fields such as recency, source type, author, or task metadata.

PostgreSQL was not used in this phase because the local PostgreSQL instance is
reserved for the `bm25v` debug line. All experiments are offline over cached
BEIR/Snowflake/SAE artifacts.

## New Harnesses

Two research scripts were added:

```text
scripts/research_sae_dense_replacement_eval.py
scripts/research_sae_impact_bound_sim.py
```

The first script evaluates dense replacement quality and fanout. It compares:

- dense retrieval;
- BM25 only;
- SAE only;
- BM25 + SAE with per-query max normalization;
- BM25 + SAE with raw fixed impact scoring;
- BM25 + SAE with fixed saturation scoring.

The second script simulates a single physical sparse impact index. It opens
row blocks by conservative BM25+SAE upper bounds and stops only when the next
block cannot enter top-k. This is a safe but simple physical-index proxy for a
future block-max/WAND-style engine.

## Quality Result

The strongest current quality signal is still positive.

Configuration:

```text
base embedding: Snowflake/snowflake-arctic-embed-m-v2.0
embedding dims: 768
SAE latent dims: 8192
document active dims: 64
query active dims: 64
SAE score mode: normalized_idf_dot
sample: five BEIR datasets, 2000 docs / 100 queries each
```

Cached command:

```bash
python3 scripts/research_sae_dense_replacement_eval.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/ii42_sae_dense_replacement \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --latent-dims 8192 \
  --active-dims 64 \
  --score-mode normalized_idf_dot \
  --bm25-mode plain \
  --top-k 100
```

Result:

| Source family | Mean Recall@100 | Mean MRR@20 |
| --- | ---: | ---: |
| dense | `0.7913` | `0.7116` |
| best per-query-normalized BM25+SAE | `0.7947` | `0.6835` |
| best fixed saturation BM25+SAE | `0.7919` | `0.6731` |
| SAE only | `0.7848` | `0.6268` |
| BM25 only | `0.7035` | `0.5904` |

Interpretation:

- SAE is strong enough to be treated as a serious dense-replacement candidate
  for recall.
- A fixed monotonic saturation formula is almost as strong as dense on
  Recall@100, and it is much more suitable for a native index than per-query
  max normalization.
- Dense still wins first-page ranking. This means dropping dense completely is
  plausible for recall-oriented candidate generation, but not yet proven for
  final ranking quality.

## Fanout Result

The negative result is just as important: the current SAE representation is
too broad for an extreme-performance single index.

Best fixed saturation Recall@100 by dataset:

| Dataset | Dense R@100 | Best fixed BM25+SAE R@100 | Mean candidates | Mean postings touched |
| --- | ---: | ---: | ---: | ---: |
| scifact | `0.9800` | `0.9800` | `1992.4` | `13012.5` |
| scidocs | `0.6920` | `0.7070` | `1981.3` | `11185.4` |
| nfcorpus | `0.3635` | `0.3704` | `1958.1` | `7932.2` |
| arguana | `1.0000` | `1.0000` | `2000.0` | `44397.9` |
| fiqa | `0.9211` | `0.9226` | `1972.6` | `11750.8` |

The quality is close to ideal, but the candidate set is nearly the whole
sample. That means a naive unified postings table would not be a breakthrough.
It would mostly move dense-like broad matching into sparse form.

## Query Sparsity Tests

I tested query-side SAE sparsity while keeping document active dims at `64`.

### Query top-16 dims

Command output:

```text
/tmp/ii42_sae_dense_replacement_q16/dense_replacement_eval.md
```

Result:

| Dataset | Best fixed R@100 | Mean candidates | Mean postings touched |
| --- | ---: | ---: | ---: |
| scifact | `0.9800` | `1938.6` | `7233.5` |
| scidocs | `0.6800` | `1870.9` | `5776.1` |
| nfcorpus | `0.3663` | `1376.4` | `2852.2` |
| arguana | `1.0000` | `2000.0` | `38583.2` |
| fiqa | `0.9186` | `1862.3` | `6876.5` |

Query sparsity reduces work, but quality loss appears on SciDocs and FiQA.

### Query top-32 dims

Command output:

```text
/tmp/ii42_sae_dense_replacement_q32/dense_replacement_eval.md
```

Result:

| Dataset | Best fixed R@100 | Mean candidates | Mean postings touched |
| --- | ---: | ---: | ---: |
| scifact | `0.9800` | `1971.0` | `9208.7` |
| scidocs | `0.6965` | `1934.1` | `7628.4` |
| nfcorpus | `0.3598` | `1710.5` | `4584.9` |
| arguana | `1.0000` | `2000.0` | `40604.3` |
| fiqa | `0.9021` | `1922.8` | `8534.5` |

Top-32 dims are a better tradeoff than top-16, but still not enough. The
candidate set remains broad and FiQA drops too much.

## SAE Stoplist Test

I tested a query-side SAE max-DF stoplist.

At `max_df_ratio=0.20`, the result was effectively unchanged. The highest
query activations are not mostly extreme global stop dimensions.

At `max_df_ratio=0.05`, fanout improves modestly but quality drops:

| Dataset | Best fixed R@100 | Mean candidates | Mean postings touched |
| --- | ---: | ---: | ---: |
| scifact | `0.9780` | `1919.4` | `6655.4` |
| scidocs | `0.6520` | `1851.3` | `5346.8` |
| nfcorpus | `0.3467` | `1378.3` | `2765.2` |
| arguana | `1.0000` | `2000.0` | `38347.8` |
| fiqa | `0.9163` | `1859.1` | `6820.9` |

Conclusion: SAE latent stopwords exist as a possible systems tool, but naive
DF filtering is not a sufficient default policy.

## Block-Bound Simulation

The current physical-index proxy is conservative row-block upper-bound
simulation.

Command:

```bash
python3 scripts/research_sae_impact_bound_sim.py \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output-dir /tmp/ii42_sae_impact_bound \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --latent-dims 8192 \
  --active-dims 64 \
  --score-mode normalized_idf_dot \
  --bm25-mode plain \
  --score-contract sat \
  --sae-weight 3.0 \
  --top-k 100 \
  --block-sizes 16,32,64,128 \
  --layouts natural,sae_primary,sae_pair,random
```

Top-100 result:

| Dataset | Best layout | Block | Opened docs | Candidate docs |
| --- | --- | ---: | ---: | ---: |
| scifact | natural | `16` | `1.000` | `0.996` |
| scidocs | natural | `16` | `1.000` | `0.991` |
| nfcorpus | natural | `16` | `0.996` | `0.949` |
| arguana | natural | `16` | `1.000` | `1.000` |
| fiqa | sae_primary | `16` | `1.000` | `0.986` |

Top-20 with query top-16 dims:

| Dataset | Best layout | Block | Opened docs | Candidate docs |
| --- | --- | ---: | ---: | ---: |
| scifact | sae_primary | `16` | `0.912` | `0.969` |
| scidocs | sae_pair | `16` | `0.890` | `0.935` |
| nfcorpus | natural | `16` | `0.668` | `0.667` |
| arguana | sae_primary | `16` | `0.793` | `1.000` |
| fiqa | sae_pair | `16` | `0.880` | `0.931` |

Interpretation:

- The score contract is exact under the simulated upper bounds.
- The upper bounds are too loose to create a fast physical scan.
- Smaller blocks help a little, but not enough.
- SAE-primary clustering helps slightly on some datasets, but it is not the
  main breakthrough.

This means the naive row-block version of a unified sparse impact index is not
enough. The next design should move toward impact-ordered postings,
MaxScore/WAND, or a more selective latent representation.

## Active-Dim Scaling Check

I reused the existing SciDocs active-128 cache to test whether larger SAE
capacity changes the conclusion.

Result:

| Profile | Best fixed R@100 | Best fixed MRR@20 | Mean candidates | Mean postings touched |
| --- | ---: | ---: | ---: | ---: |
| SciDocs active64 sample | `0.7070` | `0.6635` | `1981.3` | `11185.4` |
| SciDocs active128 sample | `0.6815` | `0.6799` | `1998.6` | `19042.1` |

Active-128 is a quality/ranking route, not an efficiency route. It increases
postings and makes the bound simulation worse:

```text
top20 opened docs: 0.982
candidate docs: 0.999
```

## Current Conclusion

The hypothesis splits into two parts:

```text
Quality hypothesis: BM25 + SAE can replace dense for recall.
Systems hypothesis: BM25 + SAE can become one extremely fast physical index.
```

The quality hypothesis is now credible. On the sampled matrix, fixed
saturation BM25+SAE essentially matches dense Recall@100 and beats dense on
several datasets.

The systems hypothesis is not yet proven. Current SAE latents are too broad:
they produce near-full-corpus candidate coverage, and conservative block
upper bounds cannot prune enough work.

This is useful because it narrows the real breakthrough target. The next
breakthrough is not another late-fusion formula. It is a representation and
physical-layout problem:

```text
Train or derive semantic sparse impacts that preserve dense recall while
remaining selective enough for MaxScore/WAND-style pruning.
```

## Recommended Next Direction

This section is now superseded by
`sae-native-index-design-exploration-summary.md`. The recommendations below
were executed far enough to justify a native sparse-impact design draft:
candidate-budget training, query gating, learned block layout, and block-max
physical simulation.

The next phase should focus on three changes.

1. Retrieval-aware SAE training

The current SAE is an autoencoder over dense embeddings. It reconstructs dense
space, but it is not trained to create selective postings. Add a retrieval
regularizer that penalizes high-DF latents and rewards relevant query/doc
collisions.

Target property:

```text
same or better Recall@100, much lower candidate-doc coverage
```

2. Query/document asymmetric sparse encoding

The document side can keep more active dims for recall, but the query side
needs a learned sparse gate. Simple top-k query activation and naive DF
stoplists are not enough.

Target property:

```text
query active dims <= 16 or 32, with dense-level recall retained
```

3. Impact-ordered WAND simulator

The current block simulation is row-block based. A real sparse engine should
use impact-ordered postings and MaxScore/WAND-like skipping. The next harness
should simulate that directly and report:

- postings decoded per query;
- exact top-k match;
- recall/MRR;
- candidate docs scored;
- upper-bound stop reason.

If WAND still scans too broadly, the problem is definitely latent selectivity.
If WAND works, the next native index design becomes much clearer.
