# M735A Causal Atom Replay

M735A replays single-atom query-local posting deltas through the
native scorer.  The purpose is to test atom-level causal signal before
training any deeper compiler.

## Outputs

- Rows: `runs/m735a_causal_atom_replay_v1/m735a_rows.jsonl`
- JSON: `runs/m735a_causal_atom_replay_v1/m735a_summary.json`

## Surface Summary

| Surface | Split | Rows | Safe | Productive | Danger | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head_risk_atoms_s0.005` | `full` | 320 | 0.559375 | 0.046875 | 0.440625 | -0.000769 | -0.000154 | -0.000124 | -0.000461 | -0.000589 | -0.000781 | -0.000452 |
| `head_risk_atoms_s0.005` | `holdout` | 40 | 0.450000 | 0.025000 | 0.550000 | +0.000000 | +0.000011 | +0.000000 | +0.000000 | +0.000000 | -0.000000 | -0.001953 |
| `source_atoms_s0.005` | `full` | 640 | 0.695312 | 0.046875 | 0.304688 | +0.000240 | +0.000064 | -0.000211 | +0.000327 | -0.000914 | -0.000750 | -0.000177 |
| `source_atoms_s0.005` | `holdout` | 80 | 0.650000 | 0.012500 | 0.350000 | +0.000000 | +0.000013 | +0.000000 | +0.000000 | +0.000000 | -0.000125 | -0.000830 |

## Decision

M735A produced atom-level causal rows.  Proceed to M735B to test safe/productive/danger separability; do not infer deployability from mean deltas.
