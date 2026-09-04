# M734A Teacher/Interface Audit

M734A replays source-aware and head-risk atom interfaces through the
native unified-posting scorer at tiny fixed scales.  It creates the
teacher surface used by M734B; it does not train a compiler.

## Outputs

- Rows: `runs/m734a_teacher_interface_audit_v1/m734a_rows.jsonl`
- JSON: `runs/m734a_teacher_interface_audit_v1/m734a_summary.json`

## Surface Summary

| Surface | Split | Rows | Hard Safe | Productive Safe | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head_risk_atoms_s0.005` | `full` | 400 | 0.385000 | 0.057500 | -0.000481 | +0.002818 | +0.001606 | +0.002140 | -0.000519 | -0.001650 | -0.000225 |
| `head_risk_atoms_s0.005` | `holdout` | 70 | 0.457143 | 0.057143 | -0.000000 | +0.000944 | -0.000358 | -0.007317 | +0.000000 | -0.000857 | +0.000670 |
| `head_risk_atoms_s0.01` | `full` | 400 | 0.375000 | 0.055000 | -0.000481 | +0.002740 | +0.001616 | +0.002557 | -0.000019 | -0.001550 | -0.000186 |
| `head_risk_atoms_s0.01` | `holdout` | 70 | 0.442857 | 0.057143 | -0.000000 | +0.000942 | -0.000358 | -0.007317 | +0.000000 | -0.000857 | +0.000781 |
| `source_atoms_s0.005` | `full` | 400 | 0.340000 | 0.052500 | +0.000010 | +0.001085 | -0.000094 | +0.000485 | +0.000000 | -0.001700 | -0.000283 |
| `source_atoms_s0.005` | `holdout` | 70 | 0.371429 | 0.071429 | -0.002857 | +0.000319 | -0.000898 | -0.007317 | +0.000000 | -0.002000 | +0.000446 |
| `source_atoms_s0.01` | `full` | 400 | 0.337500 | 0.052500 | +0.000010 | +0.000902 | -0.000512 | +0.000320 | +0.000000 | -0.001875 | -0.000293 |
| `source_atoms_s0.01` | `holdout` | 70 | 0.357143 | 0.071429 | -0.002857 | +0.000155 | -0.003578 | -0.007923 | +0.000000 | -0.002143 | +0.000335 |

## Decision

M734A produced a native delta teacher surface.  Proceed to M734B only to test whether hard-safe deltas are separable; do not treat mean retrieval deltas as a promotion signal.
