# M1338 Atom-Risk Tail Penalty

## Question

M1337 found that M1336 tail-blend harm is partly concentrated in a small group
of atoms, but those atoms span rows and are mixed with helpful cases.

M1338 tests the simplest source-level repair:

> Learn risky candidate-tail atoms on training folds, then drop or downweight
> them on heldout rows.

This is a hard-row LODO smoke, not a full `shared15` replay.

## Run

```bash
python3 scripts/replay_m1338_atom_risk_tail_penalty.py \
  --output-root runs/m1338_atom_risk_tail_penalty_smoke_v1
```

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

Query count: `249`.

## Result

| Variant | Selected | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1338_candidate_tail_s0p5` | 4.683 | 1 | 4 | +0.001847 | +0.000109 | -0.000083 | +0.000807 | +0.000000 | +0.010185 |
| `m1338_downweight_risk0p7` | 4.683 | 1 | 4 | +0.001847 | +0.000141 | -0.000096 | +0.000807 | +0.000000 | +0.010120 |
| `m1338_drop_risk0p85` | 4.522 | 1 | 4 | +0.001847 | +0.000134 | -0.000096 | +0.000807 | +0.000015 | +0.010114 |
| `m1338_soft_risk0p85` | 4.683 | 1 | 4 | +0.001847 | +0.000131 | -0.000096 | +0.000807 | +0.000015 | +0.010106 |
| `m1338_downweight_risk0p85` | 4.683 | 1 | 4 | +0.001847 | +0.000132 | -0.000096 | +0.000807 | +0.000000 | +0.010091 |
| `m1338_drop_risk0p7` | 4.285 | 1 | 4 | +0.001772 | +0.000135 | -0.000096 | +0.000829 | +0.000015 | +0.009781 |
| `m1338_drop_risk0p55` | 3.831 | 1 | 3 | +0.001901 | +0.000336 | +0.000082 | +0.001101 | -0.000788 | -0.007606 |
| `rank_prefix` | 1.309 | 1 | 1 | +0.001977 | +0.000862 | +0.000654 | +0.000695 | -0.001606 | -0.026600 |

The best variant remains the unpenalized tail blend.

The closest risk-penalty variants change small details but do not beat it:

- `downweight_risk0p7`: slightly better MAP, worse NDCG, lower score.
- `drop_risk0p85`: small CUB gain, worse NDCG, lower score.
- `drop_risk0p55`: fewer dataset negatives but loses CUB enough to become
  negative overall.

## Best Variant Dataset Shape

Best variant: `m1338_candidate_tail_s0p5`.

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 3 | +0.000922 | -0.001605 | -0.001070 | -0.000471 | +0.000000 |
| `scidocs` | 0 | +0.004000 | +0.001748 | +0.000278 | +0.002482 | +0.000000 |
| `webis-touche2020` | 1 | -0.000658 | +0.000262 | +0.001196 | +0.000000 | +0.000000 |

## Interpretation

M1338 rejects the simple risky-atom repair.

The M1337 harm concentration is real, but it does not transfer cleanly as a
global fold-trained atom blacklist or downweighting rule.  This means the local
branch should not continue with:

- more thresholds;
- more risk precision cutoffs;
- simple atom blacklists;
- post-hoc query selectors over the same source.

## Decision

Do not run full `shared15` for M1338.

Keep the retained facts:

- `candidate_tail` remains a recall-bearing source;
- low-weight whole-tail blending remains a structural signal;
- simple atom-risk penalties are not enough.

The next useful route, if continuing this family, must redesign the generated
tail objective so harm separation is learned inside the source itself.  A
plain risk penalty over existing tail atoms has now failed.

## Artifacts

- Script:
  `scripts/replay_m1338_atom_risk_tail_penalty.py`
- JSON:
  `runs/m1338_atom_risk_tail_penalty_smoke_v1/m1338_atom_risk_tail_penalty.json`
- Markdown:
  `runs/m1338_atom_risk_tail_penalty_smoke_v1/m1338_atom_risk_tail_penalty.md`
