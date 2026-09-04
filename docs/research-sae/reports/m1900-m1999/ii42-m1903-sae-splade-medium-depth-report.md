# M1903 SAE-SPLADE Medium-Depth Report

Date: 2026-07-12

Decision: **retain the ranking signal, reject the checkpoint, and run one
full-FLOPS-ramp diagnostic before deciding whether to acquire official data.**

## Result

M1903 kept the M1902 architecture, teacher, data, losses, and absolute
6,000-step FLOPS ramp. It increased SAE training to 4,000 steps and IR training
to 2,000 steps on the existing 10,000 MS MARCO rows.

The reconstruction stage selected step 4,000:

| SAE step | Reconstruction MSE | Dead ratio | Raw doc nnz | Raw doc maxDF |
| ---: | ---: | ---: | ---: | ---: |
| initial | 132.280073 | 0.000000 | 389.9 | 0.999132 |
| 100 | 124.470923 | 0.000000 | 271.6 | 0.999132 |
| 500 | 81.668720 | 0.000000 | 156.8 | 1.000000 |
| 1,000 | 65.291077 | 0.000000 | 149.3 | 1.000000 |
| 2,000 | 52.728572 | 0.000000 | 154.3 | 1.000000 |
| 4,000 | 48.209460 | 0.718811 | 162.5 | 1.000000 |

IR selection preferred the final 2,000-step checkpoint:

| Step | KL | Positive top1 | Pairwise | Doc FLOPS | Doc nnz | Doc maxDF |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 1.453278 | 0.343750 | 0.709961 | 90.455 | 116.8 | 1.000000 |
| 250 | 1.173488 | 0.500000 | 0.764648 | 79.431 | 93.6 | 1.000000 |
| 500 | 1.048807 | 0.468750 | 0.792969 | 65.686 | 65.8 | 1.000000 |
| 1,000 | 1.136703 | 0.500000 | 0.811523 | 54.167 | 72.0 | 1.000000 |
| 1,500 | 0.964748 | 0.539062 | 0.823242 | 41.695 | 114.2 | 1.000000 |
| 2,000 | 0.961855 | 0.554688 | 0.824219 | 30.521 | 121.9 | 1.000000 |

## Disjoint Confirmation

| Surface | KL | Positive top1 | Pairwise | Spearman | Doc FLOPS | Doc nnz | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M1902 SAE | 1.368354 | 0.285156 | 0.707520 | 0.325839 | 23.024 | 308.4 | 0.999132 |
| M1903 SAE | 0.994030 | 0.484375 | 0.809814 | 0.464628 | 30.549 | 122.7 | 1.000000 |
| OpenSearch control | 1.250686 | 0.675781 | 0.924072 | 0.719015 | 0.618 | 177.4 | 0.063802 |

M1903 improved pairwise agreement by `+0.102295`, positive top1 by
`+0.199219`, and KL by `-0.374325` relative to M1902 on unseen rows. This is a
real depth signal, not a final-step-only or split-specific effect.

The checkpoint still fails the product shape. Document FLOPS is `49.45x` the
OpenSearch control and maxDF remains `1.0`. Ranking also remains below the
control by `0.114258` pairwise and `0.191406` positive top1.

## Corrected Interpretation

The original M1903 stop rule attributed unchanged maxDF to insufficient SAE
specialization. That interpretation is incomplete. The variable that directly
penalizes sequence-level corpus activation is the IR FLOPS term, but M1903
stopped at step 2,000 while the fixed paper schedule reaches full strength only
at step 6,000.

M1903 therefore proves that more exposure improves retrieval behavior, while
leaving the high-DF question unresolved. It does not authorize official data
or native FiQA, but it justifies M1904: a single 10,000-step IR run from the
frozen M1903 SAE with no objective or data change.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1903-sae-splade-medium-depth-contract.md`
- ClearML task: `4c93d8c914964752a966128e275e8f3f`
- Selected SAE: remote `sae_stage_selected_step4000.pt`
- Selected IR checkpoint: remote `sae_splade_selected_step2000.pt`
- Follow-up: `docs/research-sae/reports/m1900-m1999/ii42-m1904-sae-splade-full-ramp-contract.md`
