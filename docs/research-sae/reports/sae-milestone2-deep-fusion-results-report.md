# SAE Milestone 2 Deep-Fusion Results Report

Date: 2026-05-12

Base checkpoint:

```text
sae-milestone-1
```

## Purpose

Milestone 2 starts the deep-fusion line after milestone 1. The question is
whether BM25 tokens and SAE latent activations can become one source-light or
source-blind evidence-atom index, instead of staying as separate source scores.

This pass runs first versions of all three proposed routes:

1. source-blind single atom namespace;
2. token-latent co-activation graph and mixed atoms;
3. lightweight learned atom-reliability / query-budget proxy.

Runner:

```text
scripts/research_sae_milestone2_deep_fusion.py
```

Output:

```text
results/sae/milestone2/deep-fusion/
```

Command:

```bash
python3 scripts/research_sae_milestone2_deep_fusion.py \
  --output-dir results/sae/milestone2/deep-fusion
```

## Configuration

- Datasets: `scifact`, `scidocs`, `nfcorpus`, `arguana`, `fiqa`
- Corpus sample: current five-dataset matrix artifacts
- Base profile: `Snowflake 768 + SAE 8192/64`
- SAE score mode: `normalized_idf_dot`
- BM25 mode: `plain`
- Top-k: `100`

The milestone-1 row is the source-aware fixed-saturation scorer:

```text
bm25_weight * saturate(bm25_raw, tau_bm25)
+ sae_weight * saturate(sae_raw, tau_sae)
```

The deep-fusion rows use one atom scorer:

```text
score(doc) = sum(query_impact(atom) * doc_impact(atom))
```

## Mean Quality Matrix

| Run | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `milestone1_fixed_saturation` | 0.6971 | 0.7934 | 0.6742 | 0.5952 | 0.4987 |
| `route1_single_atom_raw` | 0.6227 | 0.7344 | 0.6199 | 0.5291 | 0.4386 |
| `route1_single_atom_scaled` | 0.7094 | 0.7951 | 0.6790 | 0.6029 | 0.5011 |
| `route2_graph_expansion` | 0.6938 | 0.7992 | 0.6090 | 0.5562 | 0.4560 |
| `route2_mixed_atoms` | 0.7092 | 0.7954 | 0.6681 | 0.5939 | 0.4944 |
| `route3_learned_atom_budget_32` | 0.6865 | 0.7788 | 0.6316 | 0.5612 | 0.4590 |
| `route3_learned_atom_budget_64` | 0.7022 | 0.7927 | 0.6550 | 0.5845 | 0.4796 |

## Mean Cost Proxies

These are Python research-loop counters, not native index latency claims. They
are useful for comparing query atom count, touched postings, and candidate
fanout across routes.

| Run | Mean ms | Query atoms | Postings touched | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| `milestone1_fixed_saturation` | 13.1777 | 91.2740 | 17655.7580 | 1980.8960 |
| `route1_single_atom_raw` | 3.5381 | 91.2740 | 17655.7580 | 1980.8960 |
| `route1_single_atom_scaled` | 3.3972 | 91.2740 | 17655.7580 | 1980.8960 |
| `route2_graph_expansion` | 5.4455 | 616.0080 | 26900.1960 | 2004.7680 |
| `route2_mixed_atoms` | 3.2263 | 105.3040 | 17705.5260 | 1980.8960 |
| `route3_learned_atom_budget_32` | 2.4218 | 58.6640 | 14016.3960 | 1888.9280 |
| `route3_learned_atom_budget_64` | 2.9112 | 83.1900 | 16770.8420 | 1976.8560 |

## Route 1: Single Atom Namespace

### Result

`route1_single_atom_scaled` is the strongest milestone-2 signal:

```text
Recall@100: 0.7951 vs 0.7934 baseline
MRR@20:     0.6790 vs 0.6742 baseline
NDCG@10:    0.6029 vs 0.5952 baseline
MAP@100:    0.5011 vs 0.4987 baseline
```

The raw source-blind atom score fails:

```text
Recall@100: 0.7344
MRR@20:     0.6199
```

### Interpretation

This is the clearest answer from the first pass:

```text
Source-blind scoring is viable only after calibration.
```

The scaled atom scorer bakes the milestone-1 source calibration into atom
impacts, then runs with one accumulator. That is not fully learned and not yet
the final elegant solution, but it proves the physical abstraction is plausible:
BM25 token atoms and SAE latent atoms can compete in one scorer without losing
quality.

### Limitation

The fanout is unchanged:

```text
postings touched: 17655.8
candidate docs:   1980.9
```

So Route 1 improves conceptual cleanliness and scoring simplicity, but does
not solve the central systems bottleneck yet.

## Route 2: Token-Latent Graph And Mixed Atoms

### Result

Graph expansion increases Recall@100:

```text
route2_graph_expansion Recall@100 = 0.7992
```

but hurts ranking and cost:

```text
MRR@20:           0.6090
postings touched: 26900.2
query atoms:      616.0
```

Mixed atoms are better behaved:

```text
route2_mixed_atoms Recall@100 = 0.7954
route2_mixed_atoms MRR@20     = 0.6681
```

but they still do not beat Route 1 on ranking quality.

### Interpretation

The co-activation graph is not ready as a query expansion mechanism. It adds
semantic recall, but it also injects too many weak atoms and damages first-page
ranking.

Mixed atoms are more promising than raw graph expansion because they keep
query atom count and postings close to Route 1. The first implementation,
however, only adds a small quality signal. It does not yet reduce candidate
fanout.

### Next Adjustment

If Route 2 continues, do not expand broad query atoms directly. The next graph
variant should be selective:

```text
token-latent edge must pass lift/PMI threshold
+ edge must reduce candidate fanout
+ mixed atom must have lower DF than both parent atoms
```

The useful direction is not "more expansion". It is "more selective mixed
evidence".

## Route 3: Learned Atom Budget Proxy

### Result

The lightweight learned query-budget proxy cuts postings:

```text
q32 postings touched: 14016.4 vs 17655.8 baseline
q64 postings touched: 16770.8 vs 17655.8 baseline
```

but quality drops:

```text
q32 Recall@100: 0.7788
q32 MRR@20:     0.6316

q64 Recall@100: 0.7927
q64 MRR@20:     0.6550
```

### Interpretation

The current learned-reliability proxy is not strong enough. It can reduce
postings, but it loses too much first-page quality. This means a real mixed
sparse encoder cannot merely learn global atom reliability. It needs a
query-conditional objective.

### Next Adjustment

If Route 3 continues, the next model should learn:

```text
query-conditioned atom selection
+ lexical anchor preservation
+ dense/BM25+SAE teacher ranking
+ candidate-budget penalty
```

The proxy result is useful because it shows where the simple version fails:
global atom usefulness is not enough to preserve ranking.

## Per-Dataset Notes

### SciFact

Route 1 and Route 2 mixed atoms both match Recall@100 and slightly improve
MRR@20:

```text
baseline MRR@20: 0.8096
Route 1 scaled:  0.8127
Route 2 mixed:   0.8141
```

### SciDocs

Route 1 scaled and Route 2 mixed both improve Recall@100:

```text
baseline:       0.6965
Route 1 scaled: 0.7045
Route 2 mixed:  0.7085
```

Graph expansion reaches `0.7160` Recall@100 but MRR collapses to `0.5230`,
which is a clear ranking warning.

### NFCorpus

Route 2 graph expansion improves Recall@100 to `0.3971`, but MRR drops. Route
1 scaled preserves baseline quality with a small MRR improvement:

```text
baseline MRR@20: 0.6829
Route 1 scaled:  0.6905
```

### Arguana

Recall saturates for most calibrated routes. Ranking remains weak, and graph
expansion is especially harmful:

```text
baseline MRR@20: 0.4961
graph MRR@20:    0.3958
```

### FiQA

Route 1 scaled is strongest:

```text
baseline MRR@20: 0.7371
Route 1 scaled:  0.7528
```

Route 3 q64 preserves Recall@100 but hurts MRR, again showing the budget proxy
is not ranking-safe yet.

## Decision

Route 1 should become the primary milestone-2 continuation:

```text
source-aware fixed saturation
  -> source-calibrated atom impacts
  -> source-blind single accumulator
```

Route 2 should continue only in the narrower mixed-atom direction. Query graph
expansion as implemented here is too noisy.

Route 3 should not continue as global reliability. It needs a real
query-conditioned mixed sparse encoder before it can justify more effort.

## Next Milestone-2 Steps

1. Turn Route 1 into a stricter source-blind scorer contract:
   `atom_id`, `impact`, `atom_tau/reliability`, one accumulator.
2. Add block-bound and impact-head simulation for Route 1, because quality is
   already good enough; the unsolved issue is candidate fanout.
3. Rework Route 2 mixed atoms so a mixed atom is kept only if its DF is lower
   than both parent atoms and it has positive held-out retrieval lift.
4. Archive Route 2 graph expansion as a negative first result.
5. Replace Route 3 global reliability with a query-conditioned training plan;
   do not spend more time on global atom weights.
