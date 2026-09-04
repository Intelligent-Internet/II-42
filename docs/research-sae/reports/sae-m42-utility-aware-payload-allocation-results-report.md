# SAE M42 Utility-Aware Payload Allocation Results Report

Status: closed as a major positive engineering/modeling direction.

## Summary

M42 found a stronger route than additional M41-style fanout loss sweeps:

```text
M40 lexical-DF query model
+ utility/fanout-aware query atom export selection
  -> same or slightly better ranking quality
  -> much lower SAE postings
```

The best M42 result is not from the new utility-loss training run. It is from
export-time payload allocation on the existing M40 model:

```text
M40 sweep:
NDCG@10 0.7887, MAP@100 0.7596, SAE postings 2734.7

M42 M40-pruned, active=48, fanout_power=0.5:
NDCG@10 0.7889, MAP@100 0.7597, SAE postings 1388.7
```

This is a clear improvement: ranking quality is preserved or slightly improved
while SAE postings drop by roughly `49%`.

## Implementation

M42 adds a training-side utility loss to:

```text
scripts/research_sae_m31_joint_final_ranking_train.py
```

New opt-in arguments:

```text
--atom-utility-loss-weight
--atom-utility-top-dims
--atom-utility-fanout-power
--atom-utility-qrel-mix
--atom-utility-teacher-prior
--atom-utility-support-temperature
--atom-utility-restrict-teacher-dims
--broad-atom-utility-multiplier
--ordinary-atom-utility-multiplier
```

M42 also adds the export-selection sweep:

```text
scripts/research_sae_m42_atom_utility_prune_sweep.py
```

The sweep reuses generated query latents and tests:

```text
query_atom_selection_score = query_atom_weight / df(atom)^alpha
```

This is closer to the physical engine problem than training loss alone because
it directly controls which atom postings the query will touch.

## Training Utility Loss Results

M42 trained two utility-loss variants on the M39/M40 training surface.

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M42 utility `w0p03` | 0.8670 | 0.8980 | 0.7882 | 0.7575 | 0.4737 | 2965.4 | 2896.3 |
| M42 utility `fp2` | 0.8678 | 0.8991 | 0.7881 | 0.7589 | 0.4652 | 2967.1 | 2933.9 |

Interpretation:

- utility loss has ranking signal, especially for `trec-covid`;
- but it tends to select high-impact/high-fanout teacher atoms;
- stronger fanout discount and lower teacher prior did not solve the physical
  cost issue in the training loss itself.

Therefore the utility loss is not promoted as the main M42 result.

## Export Selection Results

The key result comes from pruning M40 query payloads by `weight / df^alpha`.

| Run | Active | Fanout power | Threshold | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M40 sweep | 64 | 0.0 | 0.15 | 0.8670 | 0.8985 | 0.7887 | 0.7596 | 0.4701 | 2948.5 | 2734.7 |
| M42 M40-pruned best | 48 | 0.5 | 0.15 | 0.8666 | 0.9000 | 0.7889 | 0.7597 | 0.4595 | 2845.3 | 1388.7 |
| M42 M40-pruned TREC-lean | 48 | 0.5 | 0.12 | 0.8667 | 0.8995 | 0.7889 | 0.7595 | 0.4664 | 2845.3 | 1388.7 |
| M42 lower-cost point | 40 | 0.25 | 0.15 | 0.8661 | 0.8994 | 0.7878 | 0.7591 | 0.4608 | 2828.3 | 1279.0 |
| M42 TREC-cost point | 56 | 0.5 | 0.12 | 0.8668 | 0.8971 | 0.7880 | 0.7597 | 0.4749 | 2899.3 | 1980.3 |

The best all-around default candidate is:

```text
active_dims=48
fanout_power=0.5
df_gate_threshold=0.15
low_sae_weight=0.35
high_sae_weight=0.75
```

The TREC-lean variant changes only the threshold to `0.12`. It gives better
`trec-covid` MAP while keeping the same physical cost, but the all-around score
is slightly lower.

## Interpretation

M42 changes the next direction materially.

Before M42, the apparent path was:

```text
train better query atoms
-> use fanout regularization
-> hope cost improves
```

After M42, the better path is:

```text
train a high-quality query atom pool
-> choose the physical payload by utility/fanout
-> score with lexical-DF gate
```

This is more aligned with the unified sparse engine. The model can produce a
rich candidate atom pool, while the runtime/export layer chooses a smaller
physical payload using index statistics.

## Decision

M42 is positive and should replace M41 as the current main direction.

Promoted M42 idea:

```text
utility/fanout-aware payload allocation
```

Not promoted:

```text
M42 utility-loss training as currently implemented
```

The current best research profile is the M40-trained model with M42 export
selection. This is not yet a dense-removal product gate pass, but it is the
strongest quality-cost point so far.

## Next Direction

M43 should turn the export-selection result into a cleaner canonical design:

1. Move utility/fanout payload selection from a sweep script into the reusable
   query-latent export path.
2. Add score-model selection using:

```text
NDCG + MAP - lambda * normalized_sae_postings
```

3. Test whether the model should emit a larger atom pool, for example top-96,
   and let payload allocation choose top-48.
4. Compare `active=40/48/56` on full15 plus efficiency-only real-corpus
   artifacts.
5. Do not continue utility-loss training until export selection is canonical.
