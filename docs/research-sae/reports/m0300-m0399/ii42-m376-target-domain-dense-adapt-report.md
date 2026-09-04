# II-42 M376 Target-Domain Dense Adapt Report

## Goal

M375 showed that M372 mostly generalizes, but `trec-covid` fails badly in
leave-dataset-out mode even though its candidate dense-rerank upper bound stays
excellent. M376 tests whether target-domain dense-teacher adaptation can fix
that scorer calibration failure.

Rules:

- no BM25;
- no qrel training;
- target train queries and target eval queries are disjoint;
- dense teacher labels are allowed for target train queries only;
- evaluation uses qrels only on target eval queries.

## Commands

Primary split:

```bash
python3 scripts/research_sae_m376_target_domain_dense_adapt.py
```

Second split:

```bash
python3 scripts/research_sae_m376_target_domain_dense_adapt.py \
  --output-dir /tmp/ii42-m376b-target-domain-dense-adapt-seed2376 \
  --json-name m376b_target_domain_adapt.json \
  --report-name m376b_target_domain_adapt.md \
  --split-seed 2376
```

Outputs:

- `/tmp/ii42-m376-target-domain-dense-adapt/m376_target_domain_adapt.json`
- `/tmp/ii42-m376-target-domain-dense-adapt/m376_target_domain_adapt.md`
- `/tmp/ii42-m376b-target-domain-dense-adapt-seed2376/m376b_target_domain_adapt.json`
- `/tmp/ii42-m376b-target-domain-dense-adapt-seed2376/m376b_target_domain_adapt.md`

## Setup

- Target dataset: `trec-covid`
- Source datasets: all other 14 local shared BEIR15 datasets
- Target train pool queries: 25
- Target eval queries: 25
- Rotation: `pca_doc`
- Active dims: `128`
- Budget: `0.10`
- Source max train examples: 500000
- Target max train examples: 100000
- Epochs: 6

## Results

| Split | Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense NDCG@10 | Dense O@10 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1376 | target0.00 | 0.1813 | 0.9533 | 0.8075 | 0.1555 | 0.8650 | 0.4160 |
| 1376 | target0.25 | 0.1903 | 1.0000 | 0.8637 | 0.1679 | 0.8650 | 0.6400 |
| 1376 | target0.50 | 0.1939 | 1.0000 | 0.8807 | 0.1716 | 0.8650 | 0.5600 |
| 1376 | target1.00 | 0.1952 | 1.0000 | 0.8701 | 0.1747 | 0.8650 | 0.6440 |
| 2376 | target0.00 | 0.1729 | 0.9500 | 0.7986 | 0.1492 | 0.8548 | 0.4440 |
| 2376 | target0.25 | 0.1830 | 0.9600 | 0.8516 | 0.1625 | 0.8548 | 0.6120 |
| 2376 | target0.50 | 0.1857 | 0.9733 | 0.8429 | 0.1655 | 0.8548 | 0.6280 |
| 2376 | target1.00 | 0.1856 | 0.9700 | 0.8421 | 0.1652 | 0.8548 | 0.6480 |

`target0.25` means only 6 target train queries were used. `target0.50` means
12 target train queries were used. These target train queries are disjoint from
the 25 eval queries.

## Interpretation

Target-domain dense-teacher adaptation works.

The source-only scorer is weaker on `trec-covid` eval splits, with NDCG@10
around `0.80`. Adding only 6 target-domain dense-teacher queries lifts it to
`0.85-0.86`, essentially dense quality on both eval splits. This uses no qrels.

The M375 trec-covid failure therefore should not be interpreted as a failure of
the dense-only posting route. It is a failure of pure zero-shot score
calibration on a domain-shifted corpus.

## Product Direction

The highest-value route is now:

1. Keep M372 as the clean baseline scorer.
2. Add per-corpus dense-teacher distillation as an expected offline preparation
   step.
3. Preserve the 10% touched-doc target.
4. Next, run M377 multi-dataset target adaptation: for every dataset, train on
   source datasets plus a small disjoint target train split, then evaluate on
   target eval split.

If M377 holds broadly, this becomes the new mainline: dense-only, BM25-free,
per-corpus distilled posting scorer.
