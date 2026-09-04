# M1322 Head-Preserving Two-Stage Risk7

## Question

M1321 found a real risk7-level structural signal:
`signed_sum_top8` as the candidate stage plus a lower-pressure fill ranker made
all macro metrics positive. The remaining blocker was row-level harm.

M1322 tests whether that harm is mostly caused by over-reranking the candidate
head. The probe preserves the first `K` candidate-stage documents and reranks
only the tail with the same ranking query.

This is deliberately a source/operator check, not another selector or gate
variant.

## Command

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --candidate-stage signed_sum_top8 \
  --preserve-head-values 0,5,10,20 \
  --output-root runs/m1322_head_preserving_two_stage_risk7_v1
```

Elapsed: `477.54s`.

## Macro Results

| Variant | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1321_rank_fill_fs0p5` | 0 | 7 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | +0.000059 | +0.011376 |
| `m1321_rank_prefix_uniform_l1` | 1 | 8 | -0.000231 | +0.001486 | +0.001230 | +0.000662 | +0.000059 | +0.007148 |
| `m1321_candidate_self_signed_sum` | 1 | 10 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | +0.005682 |
| `m1322_preserve20_rank_fill_fs0p5` | 1 | 11 | +0.000428 | +0.001152 | +0.001789 | -0.000961 | +0.000059 | -0.002292 |
| `m1322_preserve10_rank_fill_fs0p5` | 1 | 11 | +0.000428 | +0.001018 | +0.001789 | -0.001087 | +0.000059 | -0.004213 |
| `m1322_preserve5_rank_fill_fs0p5` | 1 | 11 | +0.000428 | +0.000670 | +0.000403 | -0.001119 | +0.000059 | -0.008412 |

## Best Variant Row Anatomy

Best remains the unpreserved M1321 ranking variant:
`m1321_rank_fill_fs0p5`.

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 2 | -0.001233 | +0.000431 | +0.000758 | -0.000655 | +0.000000 |
| `dbpedia-entity` | 3 | -0.000543 | +0.003014 | +0.000952 | -0.004474 | -0.000461 |
| `fiqa` | 0 | +0.000000 | +0.003117 | +0.001067 | +0.000374 | +0.000000 |
| `nfcorpus` | 0 | +0.001847 | +0.001520 | +0.005170 | +0.008281 | +0.000339 |
| `scidocs` | 1 | +0.002000 | +0.001512 | +0.000362 | +0.000687 | -0.000500 |
| `trec-covid` | 1 | +0.000525 | +0.000398 | -0.001964 | +0.000000 | +0.001947 |
| `webis-touche2020` | 0 | +0.000475 | +0.001155 | +0.000817 | +0.000000 | +0.000000 |

## Interpretation

Head preservation is not the missing safety mechanism.

Preserving the candidate head keeps CUB positive but damages MRR and increases
dataset-level negative metrics. This means the row harm is not simply
"ranker moved too many good head documents." The harmful behavior is already
coupled to the candidate/ranking source geometry: preserving head order removes
some helpful MAP/NDCG movement while failing to protect the hard rows.

This supports the current review direction:

- M1000+ is not empty progress; it narrowed the bottleneck.
- Useful added atoms and action-conditioned structures exist.
- Ordinary selector/gate/filter/protector variants are unlikely to solve the
  remaining problem with current observability.
- The next useful work should move harm separation into candidate/source or
  objective construction, not bolt another post-hoc guard onto M1321.

## Decision

Do not promote M1322.

Keep M1321 as a structural positive signal:
`signed_sum_top8` candidate source plus lower-pressure rank fill can make risk7
macro all-positive.

Stop this specific head-preserving branch. The next probe should be a cleaner
source/objective change that creates harm separation before replay. A viable
next direction is to build an action-conditioned two-stage source where the
candidate stage carries signed-sum CUB support, but the ranking stage is trained
or constructed against row-safe movement constraints rather than constrained by
static head preservation.
