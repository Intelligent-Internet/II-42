# SAE Candidate-Budget Training and Physical Simulation Report

Date: 2026-05-11

## Scope

This phase extends the incremental candidate-budget gate in four directions:

1. train a query encoder with the same candidate-budget gate used at export;
2. evaluate train/export consistency on all current benchmark corpora;
3. add an impact-ordered MaxScore-style physical simulator;
4. compare physical traversal for the original baseline and budget-gated
   query latents.

This is still a research prototype. It does not change the native extension.

## New Scripts

```text
scripts/research_sae_candidate_budget_train.py
scripts/research_sae_impact_ordered_sim.py
scripts/research_sae_block_max_sim.py
```

`research_sae_candidate_budget_train.py` keeps the document SAE fixed, trains a
query encoder with dense top-k teacher distributions, and uses the incremental
candidate-budget gate both during query-teacher training and final query
latent export.

`research_sae_impact_ordered_sim.py` is an approximate MaxScore-style
candidate-generation simulator. It traverses SAE postings by impact and stops
when a global unseen-impact bound drops below the current top-k threshold. This
is not an exact WAND implementation, but it is a useful physical-layer pressure
test.

`research_sae_block_max_sim.py` is an SAE-only block-max simulator. It lays out
documents into blocks, stores max SAE impact per block and latent dimension,
opens blocks by descending upper bound, and stops when the next block cannot
enter the current top-k threshold. This keeps the scoring contract exact while
testing whether block metadata can reduce physical document visits.

## Candidate-Budget-Aware Training

Configuration:

```text
candidate_budget_weight: 0.010
epochs: 3
learning_rate: 2e-5
teacher_sample_k: 32
pseudo_fraction: 0.10
base run: baseline_dcost10_mask999
datasets: scifact, scidocs, nfcorpus, arguana, fiqa
```

Aggregate result:

| Path | Direct R@100 | Direct MRR@20 | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| post-training gate | `0.9242` | `0.7709` | `0.9308` | `0.7669` | `1249.2` | `2930.6` |
| trained query encoder | `0.9227` | `0.7484` | `0.9255` | `0.7559` | `1142.9` | `2481.6` |

Per dataset:

| Dataset | Path | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | post-training gate | `1.0000` | `0.5496` | `745.4` | `1385.6` |
| `arguana` | trained query encoder | `1.0000` | `0.5684` | `659.5` | `1145.3` |
| `fiqa` | post-training gate | `0.9800` | `0.7643` | `1619.0` | `4278.5` |
| `fiqa` | trained query encoder | `0.9675` | `0.7419` | `1586.0` | `3767.7` |
| `nfcorpus` | post-training gate | `0.7520` | `0.8570` | `1960.3` | `5490.9` |
| `nfcorpus` | trained query encoder | `0.7461` | `0.8325` | `1918.8` | `4921.7` |
| `scidocs` | post-training gate | `0.9220` | `0.7860` | `938.6` | `1746.2` |
| `scidocs` | trained query encoder | `0.9140` | `0.7673` | `764.3` | `1296.3` |
| `scifact` | post-training gate | `1.0000` | `0.8776` | `982.5` | `1751.8` |
| `scifact` | trained query encoder | `1.0000` | `0.8695` | `785.9` | `1277.0` |

Interpretation:

```text
the training loop learns cheaper query latents.
the current teacher loss sacrifices ranking quality.
post-training candidate-budget gate remains the better default.
training-aware integration is promising but not solved.
```

The direct failure mode is held-out ranking quality. The model can reduce
postings, but dense-teacher KL alone over the budget-gated sparse scores pulls
the query encoder away from the stronger original document-SAE query geometry.

The next training attempt should add a retention term:

```text
loss =
    dense_teacher_kl
    + alpha * preserve_original_budget_gated_scores
    + beta * qrel_positive_margin
    + gamma * activation_l1
```

The qrel margin is important because dense teacher lists alone can optimize for
teacher-neighborhood smoothness while weakening task-specific relevant hits.

## Retention-Aware Candidate-Budget Training

The next iteration adds an explicit retention teacher to the same training
script. The retention teacher is the initial query encoder before training,
run through the same candidate-budget gate. This is deliberately conservative:
it tells the new query encoder to learn from dense teacher neighborhoods
without forgetting the budget-gated SAE geometry that already works.

Additional loss terms:

```text
loss =
    dense_teacher_kl
    + retention_weight * initial_budget_gated_score_kl
    + qrel_margin_weight * qrel_positive_margin
    + activation_l1_weight * activation_l1
```

SciDocs smoke and small grid:

| Variant | All R@100 | All MRR@20 | Test R@100 | Test MRR@20 | Query dims |
| --- | ---: | ---: | ---: | ---: | ---: |
| no retention, 2 epochs | `0.9080` | `0.7931` | `0.6933` | `0.4649` | `21.75` |
| retention `10`, qrel `1` | `0.9060` | `0.7958` | `0.6867` | `0.4332` | `21.84` |
| retention `50`, qrel `1` | `0.9100` | `0.8032` | `0.7000` | `0.4330` | `22.45` |
| retention `50`, qrel `0` | `0.9100` | `0.8032` | `0.7000` | `0.4330` | `22.49` |
| retention `100`, qrel `0` | `0.9060` | `0.8009` | `0.6867` | `0.4323` | `22.84` |

`retention_weight=50` is the first useful point. The qrel margin did not help
in this short SciDocs run, so the five-dataset matrix uses retention only.

Five-dataset retention-only matrix:

```text
candidate_budget_weight: 0.010
retention_weight: 50.0
qrel_margin_weight: 0.0
epochs: 2
learning_rate: 2e-5
teacher_sample_k: 32
```

| Dataset | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | `1.0000` | `0.5606` | `666.8` | `1192.8` |
| `fiqa` | `0.9700` | `0.7605` | `1581.1` | `3907.5` |
| `nfcorpus` | `0.7523` | `0.8570` | `1943.7` | `5152.5` |
| `scidocs` | `0.9140` | `0.7765` | `856.6` | `1516.4` |
| `scifact` | `1.0000` | `0.8726` | `856.8` | `1447.4` |
| average | `0.9273` | `0.7654` | `1181.0` | `2643.3` |

This is a better training-aware compromise than the first trained query
encoder. It recovers most of the post-training gate ranking quality while
keeping a meaningful postings reduction.

## Impact-Ordered Physical Simulation

I compared the original baseline query latents against the post-training
candidate-budget `0.010` query latents.

Five-dataset aggregate:

| Path | Recall@100 | MRR@20 | Postings touched | Touch ratio | Candidate docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| baseline | `0.9262` | `0.7685` | `2786.0` | `0.889` | `0.609` |
| candidate-budget `0.010` | `0.9252` | `0.7709` | `2605.2` | `0.893` | `0.580` |
| retention `50`, 2 epochs | `0.9236` | `0.7672` | `2348.9` | `0.895` | `0.546` |

Dataset summary for candidate-budget `0.010`:

| Dataset | Recall@100 | MRR@20 | Postings touched | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | `1.0000` | `0.5967` | `1202.7` | `0.338` |
| `fiqa` | `0.9630` | `0.8211` | `3475.9` | `0.706` |
| `nfcorpus` | `0.7510` | `0.8213` | `5107.4` | `0.937` |
| `scidocs` | `0.9120` | `0.8174` | `1608.0` | `0.443` |
| `scifact` | `1.0000` | `0.7982` | `1632.1` | `0.473` |

Interpretation:

```text
candidate-budget query gating improves physical candidate coverage.
impact-ordered traversal still touches most available postings.
NFCorpus remains the hardest physical-index case.
better query gates alone will not create a fast native index.
```

The key systems result is that `candidate_budget_weight=0.010` reduces average
postings touched while preserving average quality. The limitation is that the
touch ratio stays near `0.89`, which is too high for the final native-index
goal.

Retention-aware training improves the postings count again, from `2605.2` to
`2348.9`, but does not improve the touch ratio. That means it reduces query
fanout, but the postings that remain are still not physically prunable enough.

## SAE Block-Max Physical Simulation

The next physical check uses block-max metadata instead of global
impact-ordered traversal. This is closer to a native sparse index design:

```text
layout documents by SAE dominant dimensions
store max impact per block and latent dim
open blocks by safe upper bound
stop when the next bound cannot enter top-k
```

Configuration:

```text
score_mode: normalized_idf_dot
top_k: 100
block_sizes: 8,16,32,64,128
layouts: natural, sae_primary, sae_pair, random
```

Aggregate result for the best opened-doc layout per dataset:

| Path | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact candidates | Exact |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | `0.9260` | `0.7737` | `0.524` | `0.434` | `0.651` | `500/500` |
| candidate-budget `0.010` | `0.9242` | `0.7709` | `0.505` | `0.410` | `0.619` | `500/500` |
| retention `50`, 2 epochs | `0.9234` | `0.7676` | `0.490` | `0.389` | `0.585` | `500/500` |

Retention `50` per-dataset best runs:

| Dataset | Layout | Block | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact candidates |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `sae_primary` | `8` | `1.0000` | `0.6119` | `0.227` | `0.151` | `0.333` |
| `fiqa` | `sae_pair` | `8` | `0.9550` | `0.8084` | `0.527` | `0.460` | `0.791` |
| `nfcorpus` | `sae_primary` | `8` | `0.7521` | `0.8272` | `0.828` | `0.797` | `0.942` |
| `scidocs` | `sae_pair` | `8` | `0.9100` | `0.8032` | `0.434` | `0.266` | `0.428` |
| `scifact` | `sae_pair` | `8` | `1.0000` | `0.7874` | `0.432` | `0.270` | `0.428` |

This changes the physical-index picture:

```text
block-max is much more useful than the first impact-ordered traversal.
SAE-primary / SAE-pair document layout matters.
small blocks are consistently best in this sample.
NFCorpus remains the hardest dataset because candidate coverage is near full.
```

The positive signal is exact pruning: all `500/500` queries match exact sparse
top-k under the simulated bound. The negative signal is that average opened
docs are still `0.490`, not the `0.05-0.20` range needed for an extreme native
index. The next breakthrough is therefore block layout and representation
selectivity, not score fusion.

## SAE Layout Learning Follow-Up

I added three stronger layout candidates to the same simulator:

- `sae_signature`: sort by the top latent signature instead of only top one or
  two dimensions.
- `simhash`: sort by a stable weighted SimHash over latent dimensions.
- `sae_tree`: recursively split documents by balanced latent presence and
  weight, with signature ordering as the leaf fallback.

The new layout sweep keeps the same exact block-max scoring contract.

Aggregate comparison:

| Path | Layout set | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact candidates | Exact |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | dominant only | `0.9260` | `0.7737` | `0.524` | `0.434` | `0.651` | `500/500` |
| baseline | + learned layouts | `0.9260` | `0.7737` | `0.471` | `0.403` | `0.651` | `500/500` |
| candidate-budget `0.010` | dominant only | `0.9242` | `0.7709` | `0.505` | `0.410` | `0.619` | `500/500` |
| candidate-budget `0.010` | + learned layouts | `0.9242` | `0.7709` | `0.454` | `0.382` | `0.619` | `500/500` |
| retention `50`, 2 epochs | dominant only | `0.9234` | `0.7676` | `0.490` | `0.389` | `0.585` | `500/500` |
| retention `50`, 2 epochs | + learned layouts | `0.9234` | `0.7676` | `0.435` | `0.358` | `0.585` | `500/500` |

Retention `50` learned-layout per-dataset best runs:

| Dataset | Layout | Block | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact candidates |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `sae_primary` | `8` | `1.0000` | `0.6119` | `0.227` | `0.151` | `0.333` |
| `fiqa` | `sae_tree` | `8` | `0.9550` | `0.8084` | `0.455` | `0.425` | `0.791` |
| `nfcorpus` | `sae_tree` | `8` | `0.7521` | `0.8272` | `0.707` | `0.690` | `0.942` |
| `scidocs` | `sae_tree` | `8` | `0.9100` | `0.8032` | `0.397` | `0.263` | `0.428` |
| `scifact` | `sae_tree` | `8` | `1.0000` | `0.7874` | `0.387` | `0.262` | `0.428` |

This is the strongest physical-index result so far. Layout learning is an
independent win: it improves baseline, post-training budget, and retention
queries without changing quality or exactness. `sae_tree` is the most useful
new layout; `simhash` was not competitive in the SciDocs smoke and should stay
as a negative control for now.

The native implementation design draft is:

```text
unified-sparse-block-max-native-index-design.md
```

## Current Direction

This report is now an input to the final design-exploration closure:

```text
sae-native-index-design-exploration-summary.md
```

The strongest current path is:

```text
baseline document SAE
+ post-training incremental query candidate-budget gate as the safest default
+ retention-aware query training as the cost-favoring candidate
+ SAE block-max metadata and recursive latent-tree document layout
```

Do not replace the current post-training gate as the default yet:
`retention_weight=50` still gives up some Recall@100. It is now the best
training-aware cost branch, so the next productive iteration should focus on
block layout training, query-aware block ordering, and retention-first
curricula, not another plain dense-teacher training run.
