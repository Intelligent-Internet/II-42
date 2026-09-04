# M735A Causal Atom Replay

M735A replays single-atom query-local posting deltas through the
native scorer.  The purpose is to test atom-level causal signal before
training any deeper compiler.

## Outputs

- Rows: `runs/m738b_shared8_atom_replay_v1/m735a_rows.jsonl`
- JSON: `runs/m738b_shared8_atom_replay_v1/m735a_summary.json`

## Surface Summary

| Surface | Split | Rows | Safe | Productive | Danger | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head_risk_atoms_s0.005` | `full` | 960 | 0.617708 | 0.092708 | 0.382292 | +0.000109 | -0.000389 | -0.000420 | -0.000438 | -0.000160 | -0.000115 | -0.000081 |
| `head_risk_atoms_s0.005` | `holdout` | 160 | 0.568750 | 0.131250 | 0.431250 | +0.002092 | +0.000192 | +0.001090 | -0.000112 | -0.000314 | +0.000063 | -0.000659 |
| `source_atoms_s0.005` | `full` | 1920 | 0.719792 | 0.073958 | 0.280208 | +0.000266 | -0.000255 | -0.000426 | -0.000085 | -0.000118 | -0.000266 | +0.000075 |
| `source_atoms_s0.005` | `holdout` | 320 | 0.728125 | 0.084375 | 0.271875 | +0.001051 | -0.000019 | +0.000418 | -0.000099 | -0.000066 | -0.000000 | -0.000183 |

## Decision

M735A produced atom-level causal rows.  Proceed to M735B to test safe/productive/danger separability; do not infer deployability from mean deltas.
