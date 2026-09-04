# M522 Dense-Signed Prefix Sweep

M522 follows the M521 broad4 gate and tests whether the remaining
`dense_signed` gap is mostly prefix capacity.  It keeps the same dense-only
training setup and evaluates larger route prefixes: `128,160,192`.

No BM25 and no qrels are used in training.  Qrels remain evaluation-only.

## Run

- Host: `spark-1`
- Remote run dir:
  `/home/huoju/leask/runs/ii42-m522-dense-signed-prefix-sweep-v1`
- Local output: `outputs/m522/dense_signed_prefix_sweep/`
- Tasks: `ArguAna, FiQA2018, SCIDOCS, TRECCOVID`
- Target mode: `dense_signed`
- Route prefixes: `128,160,192`
- Route training groups: `121`
- Elapsed: `1205.276s`

The run completed cleanly and left no active `tmux` or Docker task on
`spark-1`.

The same PPLX warning from M520/M521 remained: Transformers reported remote
code download messages and tokenizer regex warning.  This must be fixed by
pinning model revision and tokenizer options before a larger promotion.

## Macro Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m511_lora_support_candidates_p128` | 0.47213 | 0.65634 | 0.51985 | 0.24663 | 0.90510 | 0.66063 |
| `m511_lora_support_candidates_p160` | 0.47492 | 0.65925 | 0.52399 | 0.24753 | 0.92555 | 0.70371 |
| `m511_lora_support_candidates_p192` | 0.47640 | 0.66053 | 0.52394 | 0.24817 | 0.94054 | 0.73666 |
| `m506_structural_candidates_p128` | 0.47668 | 0.66254 | 0.52335 | 0.24932 | 0.97676 | 0.89803 |
| `m506_structural_candidates_p160` | 0.47650 | 0.66391 | 0.52316 | 0.24951 | 0.98847 | 0.93017 |
| `m506_structural_candidates_p192` | 0.47674 | 0.66194 | 0.52316 | 0.24950 | 0.99308 | 0.94991 |
| `route_subset_materialized_dense` | 0.48043 | 0.66173 | 0.53335 | 0.25565 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.48032 | 0.66040 | 0.53348 | 0.25556 | 1.00000 | 1.00000 |

## Per-Task Learned Best

| Task | Best learned source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `ArguAna` | `p128` | 0.41843 | 1.00000 | 0.28431 | 0.28472 | 0.99875 | 0.87184 |
| `FiQA2018` | `p128` | 0.46704 | 0.87786 | 0.57169 | 0.41680 | 0.97875 | 0.74981 |
| `SCIDOCS` | `p192` | 0.22718 | 0.58516 | 0.34644 | 0.15826 | 0.94828 | 0.74603 |
| `TRECCOVID` | `p192` | 0.79297 | 0.17126 | 0.89333 | 0.13267 | 0.82480 | 0.44852 |

## Interpretation

Prefix capacity is a real lever.  From p128 to p192, learned candidate recall
rises from `0.90510` to `0.94054`, and macro NDCG rises from `0.47213` to
`0.47640`.

M522 passes the narrow M521 continuation rule.  The p128 learned gap to
row-int8 dense was `0.00819` NDCG; p192 reduces it to `0.00392`, which closes
slightly more than half the gap while keeping touch below dense
(`0.73666` vs `1.0`).

It is still not a dense replacement.  Structural p128 is nearly identical in
NDCG (`0.47668`) while having much higher candidate recall (`0.97676`) and
higher touch (`0.89803`).  This means the learned route is useful as a lower
touch compression surface, but its candidate support is still weaker than the
deterministic structural route.

The next bottleneck is not just prefix size.  p160 and p192 improve smoothly,
but candidate recall remains far below dense and structural support.  Blindly
increasing prefix will likely buy quality by spending touch.  The next step
should improve support prediction quality at a fixed or modestly larger
prefix.

## Next Step

M523 should keep the same broad4 gate but change the support learner rather
than only increasing prefix:

1. Pin the PPLX model revision and tokenizer behavior to remove run-to-run
   code drift before further promotion.
2. Train a higher-capacity support head or longer route phase at prefixes
   `160/192`, judging by `Candidate R@100`, NDCG, and touch together.
3. Keep dense-only training for this stage.  BM25/hybrid should stay out until
   the text-to-support route stops leaving obvious candidate recall on the
   table.

Promotion rule for M523: improve p192 candidate recall toward structural p128
while keeping touch materially below structural p128.  A useful next gate would
be `Cand R@100 >= 0.96`, `NDCG@10 >= 0.477`, and `Touch < 0.85` on this broad4
surface.
