# M735A Causal Atom Replay

M735A replays single-atom query-local posting deltas through the
native scorer.  The purpose is to test atom-level causal signal before
training any deeper compiler.

## Outputs

- Rows: `runs/m736b_positive_enriched_atom_replay_v1/m735a_rows.jsonl`
- JSON: `runs/m736b_positive_enriched_atom_replay_v1/m735a_summary.json`

## Surface Summary

| Surface | Split | Rows | Safe | Productive | Danger | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head_risk_atoms_s0.005` | `full` | 800 | 0.638750 | 0.047500 | 0.361250 | -0.000308 | +0.000517 | -0.000109 | +0.000416 | -0.000236 | -0.000375 | +0.000039 |
| `head_risk_atoms_s0.005` | `holdout` | 116 | 0.681034 | 0.051724 | 0.318966 | +0.001724 | -0.000494 | -0.000370 | -0.000454 | +0.000000 | +0.000259 | +0.000337 |
| `source_atoms_s0.005` | `full` | 1600 | 0.752500 | 0.042500 | 0.247500 | +0.000096 | -0.000080 | -0.000566 | +0.000128 | -0.000490 | -0.000331 | +0.000139 |
| `source_atoms_s0.005` | `holdout` | 232 | 0.793103 | 0.038793 | 0.206897 | +0.000862 | -0.000835 | -0.000553 | -0.000238 | +0.000000 | +0.000172 | +0.000370 |

## Decision

M735A produced atom-level causal rows.  Proceed to M735B to test safe/productive/danger separability; do not infer deployability from mean deltas.
