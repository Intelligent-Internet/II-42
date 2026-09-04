# M1682 Official Absolute FLOPS-Ramp Control

## Why M1681 Does Not Close VT Yet

M1681 compressed the official SPLADE FLOPS ramp from 50,000 optimizer steps to
167. The VT branch reached 36% of final sparsity pressure by step 100 and then
collapsed from 19,329/22,769 query/document nonzeros to 0/0.007. The `k=8`
control survived because its initial score and activation scales were much
smaller.

That result closes the scaled-ramp canary but does not reproduce the official
optimization kinetics. The Naver reference uses `T=50,000` during 150,000
training steps. M1682 restores this absolute schedule without changing any
model, data, loss, lambda, learning rate, row order, or negative selection.

## Locked Test

Repeat the paired 500-step M1681 run with only:

```text
flops_ramp_steps = 50000
```

At step 500 the FLOPS multipliers are only `1e-4` of their final values. This
is intentionally a ranking-learnability and collapse-control test, not a
sparse-index result.

## Gate

Each branch must improve full-eight-negative margin MSE by at least 5%, retain
sign/top1 within 0.005 of initialization, remain finite, and retain at least
10% of initial query and document nonzeros.

VT must additionally finish with at least 5% lower margin MSE than `k=8`, with
sign and top1 no more than 0.005 worse. A pass authorizes a longer run on the
unchanged official absolute schedule. It does not authorize FiQA indexing.

## Stop Rules

- No lambda, ramp, shift, negative, or learning-rate grid.
- If VT still collapses or loses to `k=8`, close complete VT retrieval training.
- If VT wins only because representations remain dense, require a longer
  official schedule before any engine or product claim.
