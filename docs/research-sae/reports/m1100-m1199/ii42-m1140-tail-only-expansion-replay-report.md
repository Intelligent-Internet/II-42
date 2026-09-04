# ii42 M1140 Tail-Only Expansion Replay

## Objective

Test a structural rank-composition policy instead of another gamma, alpha, or selector loop. The policy keeps the base top ranks fixed and only uses the tail run as an expansion source.

## Macro Results

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| `tail` | 0.782350 | 0.762162 | 0.698423 | 0.598732 |
| `protect10_then_tail` | 0.783111 | 0.780256 | 0.700210 | 0.608367 |
| `protect20_then_tail` | 0.783112 | 0.779772 | 0.700210 | 0.607856 |
| `protect50_then_tail` | 0.782081 | 0.779772 | 0.700210 | 0.607032 |
| `protect75_then_tail` | 0.783917 | 0.779772 | 0.700210 | 0.607425 |
| `protect90_then_tail` | 0.780045 | 0.779772 | 0.700210 | 0.606947 |

## Delta Versus Base

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tail` | +0.009952 | -0.017610 | -0.001542 | -0.007453 |
| `protect10_then_tail` | +0.010713 | +0.000484 | +0.000245 | +0.002182 |
| `protect20_then_tail` | +0.010714 | +0.000000 | +0.000245 | +0.001672 |
| `protect50_then_tail` | +0.009683 | +0.000000 | +0.000245 | +0.000847 |
| `protect75_then_tail` | +0.011519 | +0.000000 | +0.000245 | +0.001240 |
| `protect90_then_tail` | +0.007647 | +0.000000 | +0.000245 | +0.000762 |

## `protect10_then_tail` Per-Dataset Delta

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | -0.000205 | +0.000000 | +0.000430 |
| `climate-fever` | -0.008333 | +0.001754 | +0.000000 | -0.005553 |
| `cqadupstack` | +0.048937 | +0.000000 | +0.000000 | +0.003304 |
| `dbpedia-entity` | +0.018477 | +0.000000 | +0.000000 | +0.028112 |
| `fever` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `fiqa` | +0.058333 | +0.004044 | +0.000000 | +0.000167 |
| `hotpotqa` | +0.000000 | +0.000000 | +0.000000 | -0.002119 |
| `msmarco` | -0.055517 | +0.000000 | +0.000000 | -0.044908 |
| `nfcorpus` | +0.066618 | -0.000097 | +0.003668 | +0.013484 |
| `nq` | +0.000000 | +0.000000 | +0.000000 | +0.000558 |
| `quora` | +0.000000 | +0.000000 | +0.000000 | -0.000214 |
| `scidocs` | +0.005000 | -0.001260 | +0.000000 | -0.007485 |
| `scifact` | -0.006667 | +0.003030 | +0.000000 | +0.000551 |
| `trec-covid` | +0.011918 | +0.000000 | +0.000000 | +0.039735 |
| `webis-touche2020` | +0.021935 | +0.000000 | +0.000000 | +0.006670 |

## Query-Level Classes

- `clean_gain`: 84
- `damage`: 66
- `mixed`: 8
- `unchanged`: 245

## Protect Underfill

For the selected policy, `2` queries had fewer base candidates than the protected rank count. In those cases the tail run fills the missing ranks.

## Verdict

The protected-tail structure is positive. Use `protect20_then_tail` as the
next validation candidate, not `protect5` or `protect10`.

`protect10` has the best heldout MAP among the conservative policies, but it
has a tiny train-split MRR regression. `protect5` is stronger on heldout but
regresses train NDCG/MAP, so it is too aggressive. `protect20` improves
Recall/MAP on both heldout and train while keeping MRR/NDCG flat or positive.

This should still be replayed on another split/seed or through the native path
before becoming a training target.

## Protect-K Stability

Heldout delta versus base:

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `protect5_then_tail` | +0.010038 | +0.001412 | +0.006421 | +0.002771 |
| `protect10_then_tail` | +0.010713 | +0.000484 | +0.000245 | +0.002182 |
| `protect15_then_tail` | +0.010593 | +0.000355 | +0.000245 | +0.002191 |
| `protect20_then_tail` | +0.010714 | +0.000000 | +0.000245 | +0.001672 |
| `protect25_then_tail` | +0.010483 | +0.000000 | +0.000245 | +0.001142 |
| `protect30_then_tail` | +0.009979 | +0.000000 | +0.000245 | +0.001221 |
| `protect40_then_tail` | +0.010080 | +0.000000 | +0.000245 | +0.001048 |

Train-split sanity delta versus base:

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `protect5_then_tail` | +0.002409 | -0.000136 | -0.001368 | -0.000637 |
| `protect10_then_tail` | +0.002602 | -0.000108 | +0.000000 | +0.000329 |
| `protect15_then_tail` | +0.002610 | -0.000025 | +0.000000 | +0.000599 |
| `protect20_then_tail` | +0.002833 | +0.000000 | +0.000000 | +0.000963 |
| `protect25_then_tail` | +0.003568 | +0.000000 | +0.000000 | +0.001351 |
| `protect30_then_tail` | +0.003397 | +0.000000 | +0.000000 | +0.001363 |
| `protect40_then_tail` | +0.003469 | +0.000000 | +0.000000 | +0.001478 |
