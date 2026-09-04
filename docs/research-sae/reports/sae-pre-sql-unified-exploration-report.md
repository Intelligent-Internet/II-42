# SAE Pre-SQL Unified Exploration Report

Date: 2026-05-12

## Purpose

This pass closes the remaining pre-SQL questions before moving the unified
BM25+SAE idea into a PostgreSQL/native implementation.

The goal is not another late-fusion sweep. The target is still one generic
sparse-impact retrieval engine:

```text
BM25 lexical impacts
+ SAE latent semantic impacts
  -> one weighted impact namespace
  -> one boundable scorer
  -> one exact top-k traversal
```

The runner added in this pass is:

```text
scripts/research_sae_pre_sql_unified_exploration.py
```

It consumes the existing five-dataset SAE quality matrix and writes:

```text
results/sae/phase5/pre-sql-unified-exploration/
```

## Configuration

- Datasets: `scifact`, `scidocs`, `nfcorpus`, `arguana`, `fiqa`
- Corpus sample: current five-dataset matrix artifacts
- Base profile: `Snowflake 768 + SAE 8192/64`
- SAE score mode: `normalized_idf_dot`
- BM25 mode: `plain`
- Top-k: `100`
- Block size: `8`
- Layouts: `natural`, `sae_tree`, `bm25_primary`, `mixed_primary`, `random`

Command:

```bash
python3 scripts/research_sae_pre_sql_unified_exploration.py \
  --output-dir results/sae/phase5/pre-sql-unified-exploration
```

## Important Fix

The first run exposed a harness bug rather than a model result. SAE posting
dimension keys were loaded as strings while query SAE dimensions were integers,
so the SAE branch did not match any posting. The runner now normalizes SAE
posting keys back to integer dimensions before scoring. The report below is
from the corrected run.

## 1. Mixed-Source Bound Tightness

The mixed-source bound is exact in this simulator, but it is not selective
enough yet.

| Layout | Exact match rate | Opened doc fraction | Scored doc fraction | Opened blocks |
| --- | ---: | ---: | ---: | ---: |
| `natural` | 1.0000 | 0.9809 | 0.9674 | 246.7180 |
| `sae_tree` | 1.0000 | 0.9756 | 0.9619 | 245.4280 |
| `bm25_primary` | 1.0000 | 0.9899 | 0.9752 | 249.0380 |
| `mixed_primary` | 1.0000 | 0.9808 | 0.9671 | 246.7420 |
| `random` | 1.0000 | 0.9942 | 0.9791 | 250.1400 |

Interpretation:

- The bound contract itself is viable.
- Current simple document/block layouts still open almost the whole corpus.
- SQL/native implementation should not start from this layout as the final
  performance design.

## 2. Impact Calibration

Fixed source-level saturation remains the best native scorer candidate because
it is monotonic and upper-boundable without per-query max normalization.

| SAE weight | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `0.5` | 0.6747 | 0.7645 | 0.6618 | 0.5746 | 0.4781 |
| `1.0` | 0.6931 | 0.7783 | 0.6748 | 0.5903 | 0.4911 |
| `1.5` | 0.6956 | 0.7853 | 0.6748 | 0.5932 | 0.4968 |
| `2.0` | 0.6971 | 0.7934 | 0.6742 | 0.5952 | 0.4987 |
| `2.5` | 0.6988 | 0.7937 | 0.6795 | 0.6001 | 0.5019 |
| `3.0` | 0.6997 | 0.7913 | 0.6766 | 0.5980 | 0.4988 |

Decision:

- Keep fixed saturation as the native contract.
- Use `sae_weight` around `2.0` to `2.5` as the first native research range.
- Do not use per-query max normalization for the first exact native path.

## 3. BM25+SAE Block Layout

The layout probe tested BM25-first, SAE-first, mixed, natural, and random
ordering. None solved the opened-document problem.

The difference between the best simple layout and natural ordering is too
small:

```text
natural opened doc fraction: 0.9809
sae_tree opened doc fraction: 0.9756
mixed_primary opened doc fraction: 0.9808
```

Decision:

- Layout ordering alone is not the breakthrough.
- The next physical design should use impact-ordered traversal, tighter
  source-aware upper bounds, and candidate-budget-aware scoring.

## 4. Budgeted Exactness

Budgeting query/document dimensions can reduce work, but it changes the result
set. Treat these as systems profiles, not exact replacements for full SAE.

| Budget | Recall@100 | MRR@20 | Overlap@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: |
| `baseline` | 0.7934 | 0.6742 | 1.0000 | 1.0000 |
| `q16_d64` | 0.7793 | 0.6606 | 0.7401 | 0.6950 |
| `q32_d48` | 0.7894 | 0.6616 | 0.7903 | 0.7540 |
| `q32_d64` | 0.7917 | 0.6708 | 0.8185 | 0.7905 |
| `q64_d48` | 0.7894 | 0.6707 | 0.8998 | 0.8829 |
| `q64_d64` | 0.7934 | 0.6742 | 1.0000 | 1.0000 |

Decision:

- `q32_d64` is a reasonable first candidate-budget profile.
- `q16_d64` is too lossy as a default.
- `q64_d48` is useful if resident doc-vector memory must be reduced.

## 5. Real Workload Qrels

The runner found no canonical arxiv/pubmed/commons qrels wired into this
branch.

Decision:

- Do not claim production dense replacement from BEIR-only evidence.
- Build a small arxiv/pubmed/commons query/qrel set before making product
  routing decisions.

## 6. BGE-M3 Sparse Baseline

The probe found local BGE-M3 artifact directories, but they are dense-vector
BEIR profiles. No stored sparse weights were detected.

Decision:

- BGE-M3 sparse remains a useful baseline, but it requires a real sparse export
  pass, for example with `FlagEmbedding`.
- It is not a blocker for the first BM25+SAE native design because the index
  abstraction is already source-generic.

## 7. Stronger SPLADE And Larger Active Dims

The previous SPLADE v2-distil matrix is still the relevant current checkpoint:

```text
BM25+SAE fixed-saturation Recall@100 = 0.7934
BM25+SPLADE fixed-saturation Recall@100 = 0.7431
BM25+SAE+SPLADE fixed-saturation Recall@100 = 0.7899
```

That does not reject SPLADE in general. It only says the current
`naver/splade_v2_distil` run should not enter SQL/native integration ahead of
BM25+SAE.

The active-dim line is also already informative:

- active-128 can improve some ranking metrics;
- it increases postings and weakens the systems case;
- it should remain a quality profile, not the first systems profile.

Decision:

- Keep SPLADE as an offline baseline.
- Reopen it only for stronger checkpoints, larger active budgets, or real
  workload qrels showing unique useful coverage.

## 8. Payload Size At Real Scale

The corrected five-dataset estimate:

| Metric | Value |
| --- | ---: |
| bytes per doc | 1738.63 |
| GB per 1M docs | 1.7386 |
| GB per 10M docs | 17.3863 |

Interpretation:

- The raw payload scale is plausible for a server-side resident index.
- This is not the full production memory budget; metadata, alignment,
  generation headers, and doc-vector duplication still matter.
- Compression should focus on resident doc vectors and high-frequency
  dimension metadata before mutable maintenance.

## Decision Summary

The next direction is clear enough:

1. Keep BM25+SAE fixed saturation as the first unified native scorer.
2. Do not move naive block layout into SQL as the expected final performance
   path.
3. Prioritize impact-ordered/source-aware traversal and tighter block bounds.
4. Keep `q32_d64` and `q64_d48` as candidate-budget profiles for systems
   evaluation.
5. Build real arxiv/pubmed/commons qrels before making product replacement
   claims.
6. Keep SPLADE and BGE-M3 sparse as pluggable future sources, not immediate SQL
   work.

The most important negative result is also the main guide: SAE is accurate
enough to justify unified sparse-index work, but the current representation
still opens too many documents. The next phase should solve representation and
layout selectivity, not tune fusion weights.
